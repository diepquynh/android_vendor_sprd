/*
 * Copyright © 2017 Spreadtrum Communications Inc.
 */

package com.android.server.am;

import android.app.usage.UsageStats;
import android.app.usage.UsageStatsManager;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Binder;
import android.os.Handler;
import android.os.Process;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.util.Slog;

import com.android.internal.app.procstats.ProcessStats;
import com.android.server.LocalServices;
import com.android.server.notification.NotificationManagerInternal;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;

/*
*  1. increase HOT application's OomAdj while it's in background(Adj 900).
*  2. decrease None-HOT applcation's OomAdj while it's in forground(Adj 200/600).
*/
public class LowRamAdjDrop {
    private Context mContext;
    private Handler mExHandler;
    private ActivityManagerServiceEx mService;
    private NotificationManagerInternal nmi;
    static final String TAG = "ActivityManagerEx";
    HashMap<String, UsageData> mUsageData = new HashMap<>();
    private ArrayList<Integer> mBusyUids = new ArrayList<>();
    private int LOWRAM_BALENCE = SystemProperties.getBoolean("ro.config.low_ram", false) ? 3 : 1;
    private int PERCEPTIBLE_NUM_LIMIT = 3;
    private long PERCEPTIBLE_PSS_LIMIT = 100 * 1024 / LOWRAM_BALENCE;//100MB
    private long SERVICE_PSS_LIMIT = 100 * 1024 / LOWRAM_BALENCE;
    private int SERVICE_NUM_LIMIT = 4;
    private long SERVICEB_PSS_LIMIT = 2 * 100 * 1024 / LOWRAM_BALENCE;
    private int SERVICEB_NUM_LIMIT = 7;
    private int CACHED_APP_MID_ADJ = 903;
    private long ONE_MINUTE = 60 * 1000;
    private long ONE_HOUR = 60 * ONE_MINUTE;
    private long ONE_DAY = 24 * ONE_HOUR;
    private long ONE_WEEK = 7 * ONE_DAY;
    private float THRE_LAUNCH_COUNT = 0.2f;
    private float THRE_FG_TIME = 0.6f;
    private boolean DEBUG_DROPADJ = false;
    private int BG_IO_PRIO_LOWEST = 7;
    private int BG_IO_PRIO_NORMAL = 0;
    private int HOT_APP_ADJ = ProcessList.SERVICE_B_ADJ;
    private boolean mDropAdjEnabled = SystemProperties.getBoolean("persist.sys.dropadj", true);
    private long IDLE_TIME_THROL_NORMAL = 5 * ONE_HOUR;
    private long IDLE_TIME_THROL_LOW = 30 * ONE_MINUTE;
    private long IDLE_TIME_THROL_CRITIAL = 10 * ONE_MINUTE;
    private long IDLE_TIME_THROL_MODERATE = 2 * ONE_HOUR;
    private long[] mIdleTileThrol = new long[]{IDLE_TIME_THROL_NORMAL,
            IDLE_TIME_THROL_MODERATE,
            IDLE_TIME_THROL_LOW,
            IDLE_TIME_THROL_CRITIAL};
    private int ADJ_DROP_INVALID = -1;
    private int ADJ_DROP_PERCEPTIBAL = 0;
    private int ADJ_DROP_SERVICEA = 1;
    private int ADJ_DROP_SERVICEB = 2;
    private int ADJ_DROP_COUNT = 3;

    class UsageData {
        public UsageStats stats;
        public float launchCountPos;
        public float fgTimePos;

        public UsageData(UsageStats stats, float launchCountPos, float fgTimePos) {
            this.stats = stats;
            this.launchCountPos = launchCountPos;
            this.fgTimePos = fgTimePos;
        }
    }

    class AdjDropParams {
        public int origAdj;
        public int[] dropTo;
        public long pssLimit;
        public int numbLimit;
        public long pssCurrent = 0;
        public int numbCurrent = 0;

        public AdjDropParams(int origAdj, int[] dropTo,
                             long pssLimit, int numbLimit) {
            this.origAdj = origAdj;
            this.dropTo = dropTo;
            this.pssLimit = pssLimit;
            this.numbLimit = numbLimit;
        }

        public void restCount() {
            pssCurrent = 0;
            numbCurrent = 0;
        }
    }

    public LowRamAdjDrop(Context context, Handler handler, ActivityManagerServiceEx service) {
        mExHandler = handler;
        mContext = context;
        mService = service;
        nmi = LocalServices.getService(NotificationManagerInternal.class);
    }

    private AdjDropParams[] mAdjDropParams = new AdjDropParams[]{
            //for perceptibal
            new AdjDropParams(ProcessList.PERCEPTIBLE_APP_ADJ,
                    new int[]{ProcessList.PERCEPTIBLE_APP_ADJ,
                            ProcessList.PERCEPTIBLE_APP_ADJ,
                            ProcessList.SERVICE_ADJ,
                            ProcessList.SERVICE_B_ADJ},
                    PERCEPTIBLE_PSS_LIMIT, PERCEPTIBLE_NUM_LIMIT),
            //for serviceA
            new AdjDropParams(ProcessList.SERVICE_ADJ,
                    new int[]{ProcessList.SERVICE_ADJ,
                            ProcessList.SERVICE_ADJ,
                            ProcessList.SERVICE_B_ADJ,
                            ProcessList.CACHED_APP_MIN_ADJ},
                    SERVICE_PSS_LIMIT, SERVICE_NUM_LIMIT),
            //for serviceB
            new AdjDropParams(ProcessList.SERVICE_B_ADJ,
                    new int[]{ProcessList.SERVICE_B_ADJ,
                            ProcessList.SERVICE_B_ADJ,
                            ProcessList.CACHED_APP_MIN_ADJ,
                            CACHED_APP_MID_ADJ},
                    SERVICEB_PSS_LIMIT, SERVICEB_NUM_LIMIT)
    };

    // get a list of 3rd-apps-usagestats sort by launch couts.
    // update every 2min
    public void schedUpdateUsageStat() {
        mExHandler.postDelayed(new Runnable() {
            public void run() {
                updateUsageStat();
            }
        }, 2 * ONE_MINUTE);
    }

    public void updateUsageStat() {
        Slog.d(TAG, "updateUsageStat");
        if (!mDropAdjEnabled) {
            return;
        }
        try {
            ArrayList<UsageStats> mUsageStatsByCounts = new ArrayList<>();
            ArrayList<UsageStats> mUsageStatsByTime = new ArrayList<>();
            UsageStatsManager um = (UsageStatsManager) mContext.getSystemService(Context.USAGE_STATS_SERVICE);
            Calendar cal = Calendar.getInstance();
            cal.add(Calendar.DAY_OF_MONTH, -30);
            List<UsageStats> us = um.queryUsageStats(UsageStatsManager.INTERVAL_BEST, cal.getTimeInMillis(), System.currentTimeMillis());
            Slog.d(TAG, "updateUsageStat query done");
            for (UsageStats stat : us) {
                try {
                    ApplicationInfo info = mContext.getPackageManager().getApplicationInfo(stat.getPackageName(), 0);
                    if ((info.flags & ApplicationInfo.FLAG_SYSTEM) == 0) {
                        mUsageStatsByCounts.add(stat);
                    }
                } catch (PackageManager.NameNotFoundException e) {
                    //e.printStackTrace();
                }
            }
            Collections.sort(mUsageStatsByCounts, new Comparator<UsageStats>() {
                @Override
                public int compare(UsageStats lhs, UsageStats rhs) {
                    if (lhs.mLaunchCount == rhs.mLaunchCount) {
                        return 0;
                    }
                    return lhs.mLaunchCount >= rhs.mLaunchCount ? -1 : 1;
                }
            });
            mUsageStatsByTime = new ArrayList<>(mUsageStatsByCounts);
            Collections.sort(mUsageStatsByTime, new Comparator<UsageStats>() {
                @Override
                public int compare(UsageStats lhs, UsageStats rhs) {
                    if (lhs.getTotalTimeInForeground() == rhs.getTotalTimeInForeground()) {
                        return 0;
                    }
                    return lhs.getTotalTimeInForeground() >= rhs.getTotalTimeInForeground() ? -1 : 1;
                }
            });
            Slog.d(TAG, "updateUsageStat sortdone ");
            synchronized (mUsageData) {
                for (UsageStats stats : mUsageStatsByCounts) {
                    int index = mUsageStatsByCounts.indexOf(stats);
                    float pos = 1.0f * (index + 1) / (mUsageStatsByCounts.size() + 1);
                    UsageData data = mUsageData.get(stats.getPackageName());
                    if (data == null) {
                        data = new UsageData(stats, 1, 1);
                    }
                    data.launchCountPos = pos;
                    mUsageData.put(stats.getPackageName(), data);
                }
                for (UsageStats stats : mUsageStatsByTime) {
                    int index = mUsageStatsByTime.indexOf(stats);
                    float pos = 1.0f * (index + 1) / (mUsageStatsByTime.size() + 1);
                    UsageData data = mUsageData.get(stats.getPackageName());
                    if (data == null) {
                        data = new UsageData(stats, 1, 1);
                    }
                    data.fgTimePos = pos;
                    mUsageData.put(stats.getPackageName(), data);
                }
            }
        } catch (Exception e) {
            //e.printStackTrace();
        }
        schedUpdateUsageStat();
        Slog.d(TAG, "updateUsageStat done ");
    }

    boolean isPackageMayBusyWithSth(ProcessRecord app, int memFactor) {
        if (nmi == null) {
            return true;
        }
        long now = System.currentTimeMillis();
        long lastPost = nmi.getAppLastActiveNotificationTime(app.info.packageName);
        if (false) {
            String lastDate = new java.text.SimpleDateFormat("yyyy MM-dd HH:mm:ss").format(new java.util.Date(lastPost));
            String nowDate = new java.text.SimpleDateFormat("yyyy MM-dd HH:mm:ss").format(new java.util.Date(System.currentTimeMillis()));
            Slog.d(TAG, "package --" + app.info.packageName + " last post = " + lastDate + "now = " + nowDate);
        }
        return ((now - lastPost) <= 2 * ONE_MINUTE);
    }

    boolean isPackageBeenNotUsedForLongTime(ProcessRecord app, int memFactor) {
        synchronized (mUsageData) {
            UsageData data = mUsageData.get(app.info.packageName);
            if (data == null) {
                //if applcation not in list, may not be start?
                return false;
            }
            long usedTime = System.currentTimeMillis() - data.stats.getLastTimeUsed();
            if (usedTime >= getIdleTimeLimit(memFactor)) {
                String lastUseDate = new java.text.SimpleDateFormat("yyyy MM-dd HH:mm:ss").format(new java.util.Date(data.stats.getLastTimeUsed()));
                String nowDate = new java.text.SimpleDateFormat("yyyy MM-dd HH:mm:ss").format(new java.util.Date(System.currentTimeMillis()));
                if (false) {
                    Slog.e(TAG, "package " + app.info.packageName + "lastuse = " + lastUseDate + "now :" + nowDate + " mLaunchCount = " + data.stats.mLaunchCount + "mem lvl" + memFactor + "ADJ:" + app.curAdj);
                }
                // has been idle for over 30min?, let it down..
                return true;
            }
        }
        return false;
    }

    int getPackageLaunchCount(String packageName) {
        synchronized (mUsageData) {
            UsageData data = mUsageData.get(packageName);
            if (data != null) {
                return data.stats.mLaunchCount;
            }
            return 0;
        }
    }

    long getPackageLastUseTime(String packageName) {
        synchronized (mUsageData) {
            UsageData data = mUsageData.get(packageName);
            if (data != null) {
                return data.stats.getLastTimeUsed();
            }
            return 0;
        }
    }

    private int adjToIndex(int adj) {
        switch (adj) {
            case ProcessList.PERCEPTIBLE_APP_ADJ:
                return ADJ_DROP_PERCEPTIBAL;
            case ProcessList.SERVICE_ADJ:
                return ADJ_DROP_SERVICEA;
            case ProcessList.SERVICE_B_ADJ:
                return ADJ_DROP_SERVICEB;
            default:
                return ADJ_DROP_INVALID;
        }
    }

    long getIdleTimeLimit(int factor) {
        if (factor < 0) {
            factor = 0;
        }
        return mIdleTileThrol[factor];
    }

    private void dropAdjLocked(ProcessRecord app, int memFactor, long now, long nowElapsed) {
        int index = adjToIndex(app.curAdj);
        if (index == ADJ_DROP_INVALID) {
            Slog.w(TAG, "sth bad happend...");
            return;
        }
        AdjDropParams parms = mAdjDropParams[index];
        app.adjDrop = false;
        if (parms.numbCurrent < parms.numbLimit && parms.pssCurrent + app.lastPss < parms.pssLimit) {
            //ok,let it be
            parms.numbCurrent++;
            parms.pssCurrent += app.lastPss;
        } else {
            //oh,full... let it down..
            if (app.curAdj != parms.dropTo[memFactor]) {
                if (DEBUG_DROPADJ) {
                    Slog.d(TAG, app.processName + " adj:" + app.curAdj + "  drop to " + parms.dropTo[memFactor] +
                            " current pss = " + parms.pssCurrent + ", numb = " + parms.numbCurrent + "current memPresure = " +
                            memFactor + "launch cout = " + getPackageLaunchCount(app.info.packageName) + " Sched:" + app.curSchedGroup);
                }
                app.adjDrop = true;
                app.curAdj = app.modifyRawOomAdj(parms.dropTo[memFactor]);
                mService.applyOomAdjLocked(app, true, now, nowElapsed);
                dropSchedGroupLocked(app);
            }
        }
    }

    // increase hot apps while adj >= 900, let it stay in memory
    private void increaseAdjLocked(ProcessRecord app, int memFactor, long now, long nowElapsed) {
        if (app.curAdj >= ProcessList.CACHED_APP_MIN_ADJ && app.adjDrop != true) {
            synchronized (mUsageData) {
                UsageData data = mUsageData.get(app.info.packageName);
                if (data != null && data.launchCountPos <= THRE_LAUNCH_COUNT) {
                    if (DEBUG_DROPADJ) {
                        Slog.d(TAG, "increaseAdjLocked:" + app.processName + " adj:" + app.curAdj + "current memPresure = " +
                                memFactor + "launch cout = " + getPackageLaunchCount(app.info.packageName) + " Sched:" + app.curSchedGroup);
                    }
                    app.curAdj = app.modifyRawOomAdj(HOT_APP_ADJ);
                    mService.applyOomAdjLocked(app, true, now, nowElapsed);
                }
            }
        }
    }
    private void dropSchedGroupLocked(ProcessRecord app) {
        //SchedGroup restored by applyOomAdjLocked.
        if (app.curSchedGroup == ProcessList.SCHED_GROUP_BACKGROUND) {
            return;
        }
        long oldId = Binder.clearCallingIdentity();
        try {
            Process.setProcessGroup(app.pid, Process.THREAD_GROUP_BG_NONINTERACTIVE);
            app.curSchedGroup = ProcessList.SCHED_GROUP_BACKGROUND;
        } catch (Exception e) {
        } finally {
            Binder.restoreCallingIdentity(oldId);
        }
    }

    private boolean isProcessAdjCare(int adj) {
        if (adj == ProcessList.PERCEPTIBLE_APP_ADJ ||
                adj == ProcessList.SERVICE_ADJ ||
                adj == ProcessList.SERVICE_B_ADJ) {
            // avoid cold app to be "persist"
            return true;
        }
        return false;
    }

    private boolean doAdjDropIfNeeded(ProcessRecord app, int memFactor) {
        // system apps always go first
        if ((app.info.flags & ApplicationInfo.FLAG_SYSTEM) != 0) {
            return false;
        }
        // if app holds a wakelock, let it(or them ) go, may bg playing music or bg download sth.
        if (isProcessHeldWakeLock(app)) {
            return false;
        }
        if (!isProcessAdjCare(app.curAdj)) {
            return false;
        }
        if (!isPackageBeenNotUsedForLongTime(app, memFactor)) {
            return false;
        }
        if (isPackageMayBusyWithSth(app, memFactor)) {
            return false;
        }
        return true;
    }

    public void doPerceptibalAppsTunningInnerLocked(boolean systemReady, int memFactor, ArrayList<ProcessRecord> lruProcesses) {
        //reduce nums of 'persist' Apps while under mem pressure.
        if (memFactor < ProcessStats.ADJ_MEM_FACTOR_MODERATE) {
            return;
        }
        if (mDropAdjEnabled == false) {
            return;
        }
        if (systemReady == false) {
            return;
        }

        final long nowElapsed = SystemClock.elapsedRealtime();
        final long now = SystemClock.uptimeMillis();
        final int N = lruProcesses.size();
        ArrayList<ProcessRecord> list = new ArrayList<>();
        for (int i = N - 1; i >= 0; i--) {
            ProcessRecord app = lruProcesses.get(i);
            if (doAdjDropIfNeeded(app, memFactor)) {
                list.add(app);
            } else {
                increaseAdjLocked(app, memFactor, now, nowElapsed);
            }
        }
        if (list.size() == 0) {
            //nothing to tunning
            return;
        }
        Collections.sort(list, new Comparator<ProcessRecord>() {
            @Override
            public int compare(ProcessRecord lhs, ProcessRecord rhs) {
                // TODO:special case is the "child process" like com.douniwan:push,now we use package launchCount.
                if (getPackageLaunchCount(lhs.info.packageName) == getPackageLaunchCount(rhs.info.packageName)) {
                    long lus = getPackageLastUseTime(lhs.info.packageName);
                    long rus = getPackageLastUseTime(rhs.info.packageName);
                    if (lus == rus) {
                        return 0;
                    } else {
                        return lus > rus ? -1 : 1;
                    }
                }
                return getPackageLaunchCount(lhs.info.packageName) > getPackageLaunchCount(rhs.info.packageName) ? -1 : 1;
            }
        });
        for (AdjDropParams parms : mAdjDropParams) {
            parms.restCount();
        }
        for (ProcessRecord app : list) {
            dropAdjLocked(app, memFactor, now, nowElapsed);
        }
    }

    private boolean isProcessHeldWakeLock(ProcessRecord app) {
        synchronized (mBusyUids) {
            return mBusyUids.contains(app.uid);
        }
    }

    // 0 add
    // 1 remove
    /*
    *@hide
    */
    public void updateWakeLockStatus(int opt, ArrayList<Integer> uids) {
        synchronized (mBusyUids) {
            if (uids == null) {
                return;
            }
            if (opt == 0) {
                for (int uid : uids) {
                    mBusyUids.add(uid);
                }
            } else {
                for (int uid : uids) {
                    for (int i = 0; i < mBusyUids.size(); i++) {
                        if (mBusyUids.get(i) == uid) {
                            mBusyUids.remove(i);
                            break;
                        }
                    }
                }
            }
        }
    }
}
