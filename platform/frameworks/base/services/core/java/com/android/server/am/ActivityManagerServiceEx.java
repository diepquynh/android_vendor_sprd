/*
 * Copyright © 2016 Spreadtrum Communications Inc.
 */

package com.android.server.am;

import android.app.ActivityManager;
import android.app.AppAs;
import android.app.IApplicationThread;
import android.app.LowmemoryUtils;
import android.app.usage.UsageStats;
import android.app.usage.UsageStatsManager;
import android.app.ProfilerInfo;
import android.app.ProcessProtection;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.os.Binder;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.os.ResultReceiver;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.util.Log;
import android.util.Slog;
import android.util.SparseIntArray;
import android.view.Display;
import android.view.WindowManager;

import com.android.server.am.ActivityStack.ActivityState;
import com.android.internal.app.procstats.ProcessStats;
import android.view.inputmethod.InputMethodInfo;
import com.android.internal.view.IInputMethodManager;

import java.io.File;
import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Calendar;
import java.util.Collections;
import java.util.Comparator;

public final class ActivityManagerServiceEx extends ActivityManagerService {

    static final String TAG = "ActivityManagerEx";
    static final boolean DEBUG_AMSEX = false && !IS_USER_BUILD;
    static final boolean DEBUG_PROCESS_PROTECT = true && !IS_USER_BUILD;

    //home key pressed and incall-screen come, flag to control whether kill-front-app or not.
    public static final boolean KILL_FRONT_APP = SystemProperties.getBoolean("sys.kill.frontapp",
            false);
    // SPRD:modify for Bug 653299 add switch for starting window.
    private static final String STARTING_WINDOW_ENABLED = "persist.sys.startingwindow";

    static final int KILL_STOP_TIMEOUT_DELAY = 7 * 1000;// 7s
    static final int KILL_STOP_TIMEOUT_DELAY_SHORT = 0 ;// 0s,adjust the delay time more convenient
    static final int KILL_STOP_TIMEOUT = 80;
    final ExHandler mExHandler;
    private int mStopingPid = -1;
    private boolean mIsKillStop = false;

    // 15M, if the cached CACH_PSS_MAX_VALUE , its adj will adjust to 8
    static final int CACH_PSS_MAX_VALUE = 15 * 1024;
    boolean mIsInHome;
    private static final boolean sSupportCompatibityBroadcast = true;
    private final String[] mSpecialActions;
    private final String[] mBroadcastPutForeground;
    //modify for Bug#618149  begin
    private List<RecentTaskThumbnail> mRecentTaskThumbnailList = new ArrayList<RecentTaskThumbnail>();
    private int mExcludeFlags = Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS;
    private String[] mRecentThumbnailPkgs = null;
    int mRecentThumbnailWidth;
    int mRecentThumbnailHeight;
    private boolean mRecentReady;
    private final int MSG_EX_SCHEDULE_UPDATE_THUMBNAIL = 0;
    private HandlerThread mThumbThread;
    private Handler mThumbnailHandler;
    //modify for Bug#618149  end
    LmKillerTracker mLmkTracker;
    LowRamAdjDrop mAdjDroper;
    HandlerThread mAdjDropThread;
    Handler mAdjDropH;
    private AppAsLaunchStatus mAppAsLaunchStatus;

    // SPRD: add for cmcc hide app <<
    private List<String> mHidePackages;
    // >>
    /**SPRD: Process name list which prevented to be killed in CACHE_EMPTY state*/
    private List<String> mCacheProtectList;

    public ActivityManagerServiceEx(Context systemContext) {
        super(systemContext);
        mIsInHome = true;
        mExHandler = new ExHandler((Looper)mHandler.getLooper());
        if(sSupportCompatibityBroadcast){
            BroadcastQueue[] broadcastQueues = new BroadcastQueue[4];
            broadcastQueues[0] = mBroadcastQueues[0];
            broadcastQueues[1] = mBroadcastQueues[1];
            broadcastQueues[2] = mCtsFgBroadcastQueue = new BroadcastQueue(this, this.mHandler, "foreground-comp", ActivityManagerService.BROADCAST_FG_TIMEOUT, false);
            broadcastQueues[3] = mCtsBgBroadcastQueue = new BroadcastQueue(this, this.mHandler, "background-comp", ActivityManagerService.BROADCAST_BG_TIMEOUT, false);
            mBroadcastQueues = broadcastQueues;
        }
        mSpecialActions = super.mContext.getResources().getStringArray(
                com.android.internal.R.array.special_broadcast_actions);
        mBroadcastPutForeground = super.mContext.getResources().getStringArray(
                com.android.internal.R.array.broadcast_put_foreground_actions);
        //init app as launch status
        mAppAsLaunchStatus = new AppAsLaunchStatus(mContext, mBgHandler, mUiHandler);
    }

    protected void doPerceptibalAppsTunningLocked(int memFactor) {
        if(mAdjDroper == null) {
            return;
        }
        try{
            mAdjDroper.doPerceptibalAppsTunningInnerLocked(mSystemReady, memFactor, mLruProcesses);
        }catch (Exception e) {
             //e.printStackTrace();
        }
    }
    // 0 add
    // 1 remove
    /*
    *@hide
    */
    public void updateWakeLockStatus(int opt, ArrayList<Integer> uids) {
        if(mAdjDroper == null) {
            return;
        }
        try{
            mAdjDroper.updateWakeLockStatus(opt, uids);
        }catch (Exception e) {
             e.printStackTrace();
        }
    }
    // 0 add
    // 1 remove
    /*
    *@hide
    */
    public void updateWakeLockStatusChanging(ArrayList<Integer> from, ArrayList<Integer> to) {
        if(mAdjDroper == null) {
            return;
        }
        try{
            mAdjDroper.updateWakeLockStatus(1, from);
            mAdjDroper.updateWakeLockStatus(0, to);
        }catch (Exception e) {
             e.printStackTrace();
        }
    }
    /**
     * @hide
     **/
    public boolean avoidGmsProcessStart(ProcessRecord caller, String targetProcess){
        if (mLastMemoryLevel > com.android.internal.app.procstats.ProcessStats.ADJ_MEM_FACTOR_MODERATE) {
            final ActivityRecord TOP_ACT = resumedAppLocked();
            final ProcessRecord TOP_APP = TOP_ACT != null ? TOP_ACT.app : null;
            if (TOP_APP != null && (!(TOP_APP.equals(caller))) && targetProcess.startsWith("com.google")) {
                return true;
            }
        }
        return false;
    }
    //modify for Bug#618149  begin
    private void initRecentTaskThumbnalWH() {
        int height = 0;
        int resId = mContext.getResources().getIdentifier("status_bar_height", "dimen", "android");
        if (resId > 0) {
            height = mContext.getResources().getDimensionPixelSize(resId);
        }
        WindowManager wm = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
        mRecentThumbnailWidth = wm.getDefaultDisplay().getWidth();
        mRecentThumbnailHeight = wm.getDefaultDisplay().getHeight() - height;
        Slog.d(TAG,"thumbnail mRecentThumbnailWidth = " + mRecentThumbnailWidth + ", mRecentThumbnailHeight = " + mRecentThumbnailHeight + ", height = " + height);
        mRecentThumbnailPkgs = mContext.getResources().getStringArray(com.android.internal.R.array.config_startingwindow_packages);
        mThumbThread = new HandlerThread("Am-ex-thumbnail",Process.THREAD_PRIORITY_BACKGROUND);
        mThumbThread.start();
        mThumbnailHandler = new Handler(mThumbThread.getLooper()) {
            @Override
            public void handleMessage(Message msg) {
                switch(msg.what) {
                    case MSG_EX_SCHEDULE_UPDATE_THUMBNAIL: {
                         float scale = 1.0f;
                         ActivityRecord ar = (ActivityRecord)msg.obj;
                         Slog.d(TAG,"try to update... current state :"+ar.state);
                         if(ar != null && mConfiguration.orientation == Configuration.ORIENTATION_PORTRAIT && ar.state == ActivityState.RESUMED) {
                             Bitmap thumb = mWindowManager.screenshotApplicationsEx(ar.appToken, Display.DEFAULT_DISPLAY,
                                    getRecentThumbnailWidth(), getRecentThumbnaiHeight(), scale);
                             updateRecentTaskThumbnail(ar,thumb);
                         }
                    } break;
                }
            }
        };
    }
    @Override
    protected int getRecentThumbnailWidth(){
        return mRecentThumbnailWidth;
    }
    @Override
    protected int getRecentThumbnaiHeight(){
        return mRecentThumbnailHeight;
    }
    @Override
    protected void clearRecentThumbNailList() {
        synchronized (mRecentTaskThumbnailList) {
            if(mRecentTaskThumbnailList != null){
                mRecentTaskThumbnailList.clear();
            }
        }
    }
    @Override
    protected void restoreTaskThumbnailFromFile(int userId) {
        initRecentTaskThumbnalWH();
        try{
            File dir = TaskPersister.getUserImagesDir(userId);
            File[] files = dir.listFiles();
        if (files != null){
            for (File file : files) {
                if (file.getName().endsWith(RecentTaskThumbnail.THUMBNAIL_XML_ENDWITH)
                    && file.getName().startsWith(RecentTaskThumbnail.THUMBNAIL_SUFFIX)
                    && file.canRead()) {
                    RecentTaskThumbnail task = RecentTaskThumbnail.restoreFromXML(file, this, userId);
                    if (task != null) {
                        synchronized (mRecentTaskThumbnailList) {
                            Slog.d(TAG, "restoreTaskThumbnailFromFile--->" + task);
                            mRecentTaskThumbnailList.add(task);
                        }
                    }
                }
            }
        }
        }catch(Exception e){
            Slog.e(TAG, "restoreTaskThumbnailFromFile " + e);
        }
        mRecentReady =true;
    }
    @Override
    protected boolean pkgSupportRecentThumbnail(String pkgName) {
        // SPRD:modify for Bug 653299 add switch for starting window.
        boolean supportStartingWindow = SystemProperties.getBoolean(STARTING_WINDOW_ENABLED, true);
        if (mRecentThumbnailPkgs != null) {
            for (String name : mRecentThumbnailPkgs) {
                if (name.equals(pkgName)) {
                    // SPRD:modify for Bug 653299 add switch for starting window.
                    return true && supportStartingWindow;
                }
            }
        }
        return false;
    }
    @Override
    protected void recentTaskThumbnailListHandleLocalChange(int userId) {
        //remove all thumbnail when local change
        Slog.d(TAG, "recentTaskThumbnailListHandleLocalChange userId = " + userId);
        if(!mRecentReady) {
            return;
        }
        synchronized (mRecentTaskThumbnailList) {
            mRecentTaskThumbnailList.clear();
            //File dir = new File(RecentTaskThumbnail.THUMBNAIL_DIR);
            File dir = TaskPersister.getUserImagesDir(userId);
            File[] files = dir.listFiles();
            if (files != null) {
                for (File file : files) {
                    if (file.getName().startsWith(RecentTaskThumbnail.THUMBNAIL_SUFFIX)) {
                        file.delete();
                    }
                }
            }
        }
    }

    protected void updateRecentTaskThumbnail(ActivityRecord who, Bitmap thumbnail) {
        Slog.d(TAG, "updateRecentTaskThumbnail----->" + who.info.taskAffinity + ", current config--->" + mConfiguration.orientation);
        if (who.intent != null && (who.intent.getFlags() & mExcludeFlags) == 0) {
            synchronized (mRecentTaskThumbnailList) {
                for (RecentTaskThumbnail recent : mRecentTaskThumbnailList) {
                    if (recent.matchRecord(who.intent)) {
                        recent.setLastThumbnail(thumbnail);
                        return;
                    }
                }
                RecentTaskThumbnail recent = new RecentTaskThumbnail(this, who.packageName, who.intent, who.userId);
                recent.setLastThumbnail(thumbnail);
                mRecentTaskThumbnailList.add(recent);
                recent.tryToSave();
            }
        }
    }
    /*
    *@hide
    */
    @Override
    public ActivityManager.TaskThumbnail getRecentTaskThumbnail(Intent intent) {
        if (mConfiguration.orientation != Configuration.ORIENTATION_PORTRAIT) {
            return null;
        }
        synchronized (mRecentTaskThumbnailList) {
            Slog.d(TAG, "getRecentTaskThumbnail----->" + intent);
            for (RecentTaskThumbnail recent : mRecentTaskThumbnailList) {
                if (recent.matchRecord(intent)) {
                    return recent.getTaskThumbnailLocked();
                }
            }
        }
        return null;
    }
    /*
    *@hide
    */
    @Override
    public void removePendingUpdateThumbTask() {
        Slog.w(TAG,"update thumbnail cancelled");
        mThumbnailHandler.removeMessages(MSG_EX_SCHEDULE_UPDATE_THUMBNAIL);
    }
    @Override
    protected void scheduleUpdateCurrentTaskThumbnail(ActivityRecord app) {
        mThumbnailHandler.removeMessages(MSG_EX_SCHEDULE_UPDATE_THUMBNAIL);
        Message msg = mThumbnailHandler.obtainMessage(MSG_EX_SCHEDULE_UPDATE_THUMBNAIL, app);
        long delay = SystemProperties.getLong("debug.snap",RecentTaskThumbnail.RECENT_THUMBNAIL_DELAY);
        mThumbnailHandler.sendMessageDelayed(msg,delay);
    }
    //modify for Bug#618149  end

    /**
     * Add the implement of API {@link ActivityManagerProxyEx.killStopFrontApp}
     * that for kill-stop front app when the phone call is coming in.
     * @params func 0 continue the stopped app;
     *              1 stop the front app ;
     *              2 Incall screen is displayed;
     *                remove the KILL_STOP_TIMEOUT msg
     */
    @Override
    public void killStopFrontApp(int func) {
        if (!KILL_FRONT_APP) {
            Slog.w(TAG,
                    "is not lowmemy version,will not killStop the front app,just return.");
            return;
        }

        if (mIsKillStop && func == 1) {
            Slog.w(TAG, "other thread is already call killStopFrontApp,return.");
            return;
        }
        synchronized (this) {
            final ActivityRecord topActivity = resumedAppLocked();
            if (func == LowmemoryUtils.KILL_STOP_FRONT_APP
                    && topActivity != null
                    && topActivity.app != null
                    && topActivity.app.info != null
                    && (topActivity.app.info.flags & (ApplicationInfo.FLAG_SYSTEM)) == 0
                    && !topActivity.isHomeActivity()) {
                mIsKillStop = true;
                int pid = topActivity.app.pid;
                Slog.w(TAG, "KILL_STOP_FRONT_APP.activity="
                        + topActivity.packageName + " pid=" + pid);

                if (pid > 0) {
                    if (KILL_FRONT_APP && mHomeProcess == null) {
                        mStopingPid = pid;
                        Slog.w(TAG, "kill the front app anyway, pid: " + pid);
                        if (!mExHandler.hasMessages(KILL_STOP_TIMEOUT)) {
                            Message msg = mExHandler.obtainMessage(KILL_STOP_TIMEOUT);
                            mExHandler.sendMessageDelayed(msg,KILL_STOP_TIMEOUT_DELAY_SHORT);
                        }
                        return;
                    }
                    mStopingPid = pid;
                    if (!mExHandler.hasMessages(KILL_STOP_TIMEOUT)) {
                        Slog.w(TAG, "send kill_stop_timeout message.");
                        Message msg = mExHandler.obtainMessage(KILL_STOP_TIMEOUT);
                        mExHandler.sendMessageDelayed(msg,
                                KILL_STOP_TIMEOUT_DELAY);
                    }
                }
            } else if (func == LowmemoryUtils.KILL_CONT_STOPPED_APP) {
                mIsKillStop = false;
                Slog.w(TAG, "KILL_CONT_STOPPED_APP.mStopingPid=" + mStopingPid);
                if (mStopingPid > 0) {
                    mExHandler.removeMessages(KILL_STOP_TIMEOUT);
                    mStopingPid = -1;
                }
            } else if (func == LowmemoryUtils.CANCEL_KILL_STOP_TIMEOUT) {
                // InCallUI is already displayed,remove timeout message
                Slog.w(TAG, "CANCEL_KILL_STOP_TIMEOUT,mStopingPid="
                        + mStopingPid);
                mExHandler.removeMessages(KILL_STOP_TIMEOUT);
            } else {
                Slog.w(TAG, "mResumeActivity is null or app is system app");
            }
        }

    }

    /**
     * Add the implement of API {@link ActivityManagerProxyEx.startHomePre}
     * that for kill-stop front app when the phone call is coming in.
     */
    @Override
    public void startHomePre() {
        if (!KILL_FRONT_APP) {
            return;
        }
        synchronized(this) {
            final ActivityRecord topActivity = resumedAppLocked();
            if (topActivity == null || topActivity.app == null
                    || (topActivity.app.info.flags & (ApplicationInfo.FLAG_SYSTEM)) != 0
                    || topActivity.isHomeActivity()) {
                return;
            }

            if (mHomeProcess == null) {
                Slog.w(TAG, "kill front app pid=" + topActivity.app.pid);
                final long origId = Binder.clearCallingIdentity();
                Process.killProcessQuiet(topActivity.app.pid);
                handleAppDiedLocked(topActivity.app, false, false);
                Binder.restoreCallingIdentity(origId);
            }
        }
    }

    final class ExHandler extends Handler {
        public ExHandler (Looper looper) {
            super(looper, null, true);
        }

        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
                //add for kill-stop process.->
                case KILL_STOP_TIMEOUT: {
                    Slog.w(TAG, "KILL_STOP_TIMEOUT,kill mStopingPid=" + mStopingPid);
                    if (mStopingPid > 0) {
                        Process.sendSignal(mStopingPid, Process.SIGNAL_KILL);
                        mStopingPid = -1;
                    }
                    break;
                }
                //<-
            }
        }
    }

    /*SPRD: 316990 { */
    protected void preBringUpPhoneLocked() {
        try {
            Slog.i(TAG, "pre start phone process");
            ApplicationInfo phoneInfo = mContext.getPackageManager().getApplicationInfo("com.android.phone", 0);
            addAppLocked(phoneInfo, false, null /* ABI override */);
        }catch (Exception e) {
            Slog.e(TAG, "pre bring up phone process have exception", e);
        }
    }
    /* } */
    boolean isTopHostApp(ProcessRecord app,  ProcessRecord top){
        if( top != null&& top != mHomeProcess&& app != mHomeProcess){
            for (int i = 0; i < top.activities.size(); i++){
               final ActivityRecord r = top.activities.get(i);
               if(r != null  && r.callerPid== app.pid){
                    return true;
               }
            }
        }
        return false;
   }

   /* AMS Optimization */
    public int computeOomAdjLocked(ProcessRecord app, int cachedAdj, ProcessRecord TOP_APP,
            boolean doingAll, long now) {
        mIsInHome = TOP_APP == mHomeProcess;
        int curadj = super.computeOomAdjLocked(app, cachedAdj, TOP_APP, doingAll, now);
        if (DEBUG_AMSEX) Slog.d(TAG, "computeOomAdjLocked enter app:" + app + " curadj:" + curadj);
        /* check State protection */
        switch (app.protectStatus) {
            case ProcessProtection.PROCESS_STATUS_RUNNING:
            case ProcessProtection.PROCESS_STATUS_MAINTAIN:
            case ProcessProtection.PROCESS_STATUS_PERSISTENT: {
                int value = sProcProtectConfig.get(app.protectStatus);
                if(curadj > value) curadj = value;
                break;
            }
            case ProcessProtection.PROCESS_PROTECT_CRITICAL:
            case ProcessProtection.PROCESS_PROTECT_IMPORTANCE:
            case ProcessProtection.PROCESS_PROTECT_NORMAL: {
                if (curadj >= app.protectMinAdj && curadj <= app.protectMaxAdj) {
                    int value = sProcProtectConfig.get(app.protectStatus);
                    if(curadj > value) curadj = value;
                }
                break;
            }
        }
        if(curadj > ProcessList.SERVICE_ADJ){
            if(app !=  TOP_APP && isTopHostApp(app, TOP_APP)){
                curadj = ProcessList.SERVICE_ADJ;
                app.cached = false;
                app.adjType = "host";
            }
        }
        if (DEBUG_AMSEX){
            Slog.d(TAG, "computeOomAdjLocked app.protectStatus:" + app.protectStatus + " app.protectMinAdj:" + app.protectMinAdj
                    + " protectMaxAdj:" + app.protectMaxAdj + " curadj:" + curadj);
        }
        if (curadj != app.curRawAdj) {
            // adj changed, adjust its other parameters:
            app.empty = false;
            app.cached = false;
            app.adjType = "protected";
            app.curProcState = ActivityManager.PROCESS_STATE_IMPORTANT_BACKGROUND;
            app.curSchedGroup = Process.THREAD_GROUP_DEFAULT;
            app.curAdj = curadj;
            if (DEBUG_AMSEX)Slog.d(TAG, "computeOomAdjLocked :" + app + " app.curAdj:" + app.curAdj);
        }
        return curadj;
    }

    final public float getTotalCpuUsage() {
        synchronized (mProcessCpuTracker) {
            final long now = SystemClock.uptimeMillis();
            if (MONITOR_CPU_USAGE && mLastCpuTime.get() < (now - MONITOR_CPU_MIN_TIME)) {
                mLastCpuTime.set(now);
                mProcessCpuTracker.update();
            }
        }
        if (DEBUG_AMSEX) Slog.d(TAG, "getTotalCpuUsage :" + mProcessCpuTracker.getTotalCpuPercent());
        return mProcessCpuTracker.getTotalCpuPercent();
    }

    private boolean checkProcessProtectPermisson(){
        int perm = PackageManager.PERMISSION_DENIED;
        perm = checkPermission(android.Manifest.permission.PROTECT_PROCESS,
                    Binder.getCallingPid(), Binder.getCallingUid());
        if (perm != PackageManager.PERMISSION_GRANTED) return false;
        return true;
    }

    public void setProcessProtectStatus(int pid, int status) {
        if(!checkProcessProtectPermisson()) {
            if (DEBUG_PROCESS_PROTECT) Slog.w(TAG, "Permission Denial: pid " + Binder.getCallingPid()
                    + " who want to setProcessProtectStatus requires "
                    + android.Manifest.permission.PROTECT_PROCESS);
            return;
        }
        ProcessRecord app = null;
        synchronized (mPidsSelfLocked) {
            app = mPidsSelfLocked.get(pid);
        }
        if (app != null) {
            if (DEBUG_PROCESS_PROTECT) Slog.d(TAG, "setProcessProtectStatus, app: " + app + " status: " + status + " preStatus: " + app.protectStatus);
            synchronized (this) {
                app.protectStatus = status;
            }
        }
    }

    public void setProcessProtectStatus(String appName, int status) {
        if(!checkProcessProtectPermisson()) {
            if (DEBUG_PROCESS_PROTECT) Slog.w(TAG, "Permission Denial: pid " + Binder.getCallingPid()
                    + " who want to setProcessProtectStatus requires "
                    + android.Manifest.permission.PROTECT_PROCESS);
            return;
        }
        if (appName == null)  return;
        if (DEBUG_PROCESS_PROTECT) Slog.d(TAG, "setProcessProtectStatus :" + appName + " status:" + status);
        for (int i = mLruProcesses.size() - 1; i >= 0; i--) {
            ProcessRecord rec = mLruProcesses.get(i);
            if (rec != null && appName.equals(rec.processName)) {
                rec.protectStatus = status;
                if (DEBUG_PROCESS_PROTECT) Slog.d(TAG, "setProcessProtectStatus find app:" + rec);
                break;
            }
        }

    }

    public void setProcessProtectArea(String appName, int minAdj, int maxAdj, int protectLevel) {
        if(!checkProcessProtectPermisson()) {
            if (DEBUG_PROCESS_PROTECT) Slog.w(TAG, "Permission Denial: pid " + Binder.getCallingPid()
                    + " who want to setProcessProtectArea requires "
                    + android.Manifest.permission.PROTECT_PROCESS);
            return;
        }
        if (DEBUG_PROCESS_PROTECT){
            Slog.d(TAG, "setProcessProtectStatus :" + appName + " minAdj:" + minAdj + " maxAdj:"
                    + maxAdj + " protectLevel:" + protectLevel);
        }
        if (appName == null) return;
        synchronized (mPidsSelfLocked) {
            for (int i = mLruProcesses.size() - 1; i >= 0; i--) {
                ProcessRecord rec = mPidsSelfLocked.valueAt(i);
                if (rec != null && appName.equals(rec.processName)) {
                    rec.protectStatus = protectLevel;
                    rec.protectMinAdj = minAdj;
                    rec.protectMaxAdj = maxAdj;
                    if (DEBUG_PROCESS_PROTECT) Slog.d(TAG, "setProcessProtectArea find app:" + rec);
                    break;
                }
            }
        }
    }

    @Override
    void updateLruProcessLocked(ProcessRecord app, boolean activityChange, ProcessRecord client) {
        super.updateLruProcessLocked(app, activityChange, client);
        if (mCacheProtectList == null || mCacheProtectList.size() <= 0
                || app.activities.size() > 0 || app.hasClientActivities
                || app.treatLikeActivity || app.services.size() > 0
                || app.persistent) {
            //Don't change lru list if this app is not empty.
            return;
        } else  {
            //Bump up the cache-protecting processes to the top of all CACHE_EMPTY processes
            int index = mLruProcesses.lastIndexOf(app);
            for (int i = index - 1; i >= 0; i--) {
                ProcessRecord cacheProc = mLruProcesses.get(i);
                if (mCacheProtectList.contains(cacheProc.processName)) {
                    mLruProcesses.remove(i);
                    mLruProcesses.add(index, cacheProc);
                    index--;
                }
            }
        }
    }

    public static class ProtectArea
    {
        int mMinAdj;
        int mMaxAdj;
        int mLevel;

        public ProtectArea(int minAdj, int maxAdj, int protectLevel)
        {
            mMinAdj = minAdj;
            mMaxAdj = maxAdj;
            mLevel = protectLevel;
        }

        @Override
        public String toString() {
            return "ProtectArea [mMinAdj=" + mMinAdj + ", mMaxAdj=" + mMaxAdj + ", mLevel=" + mLevel + "]";
        }
    }

    public List<AppAs.AppAsData> getAppAsLauncherList(String packageName){
        return mAppAsLaunchStatus.getAppAsLauncherList(packageName);
    }

    public List<AppAs.AppAsData> getAppAsList(){
        return mAppAsLaunchStatus.getAppAsList();
    }

    protected void readAppAsState(int userId){
        mAppAsLaunchStatus.readAppAsState(userId);
    }

    public List<AppAs.AppAsRecord> getAppAsRecordList() {
        return mAppAsLaunchStatus.getAppAsRecordList();
    }

    protected void writeAppAsData(int userId){
        mAppAsLaunchStatus.writeAppAsData(userId);
    }

    protected boolean addListAndJudgeAllowLocked(String base, String calling, String reason){
        return mAppAsLaunchStatus.addListAndJudgeAllowLocked(base, calling, reason);
    }
    public void setAppAsEnable(int flag, String packageName, String basePackage){
        mAppAsLaunchStatus.setAppAsEnable(flag, packageName, basePackage);
    }
    public int getAppAsStatus(String packageName, String basePackage){
        return mAppAsLaunchStatus.getAppAsStatus(packageName, basePackage);
    }


    static HashMap<String, ProtectArea> sPreProtectAreaList;
    static HashSet<String> sHasAlarmList;
    static HashSet<String> sNeedCleanPackages;
    static SparseIntArray sProcProtectConfig;

    static {
        sPreProtectAreaList = new HashMap<String, ProtectArea>();

        sHasAlarmList = new HashSet<String>();
        sNeedCleanPackages = new HashSet<String>();

        sProcProtectConfig = new SparseIntArray();
        sProcProtectConfig.put(ProcessProtection.PROCESS_STATUS_RUNNING, 0);
        sProcProtectConfig.put(ProcessProtection.PROCESS_STATUS_MAINTAIN, 2);
        sProcProtectConfig.put(ProcessProtection.PROCESS_STATUS_PERSISTENT, -12);
        sProcProtectConfig.put(ProcessProtection.PROCESS_PROTECT_CRITICAL, 0);
        sProcProtectConfig.put(ProcessProtection.PROCESS_PROTECT_IMPORTANCE, 2);
        sProcProtectConfig.put(ProcessProtection.PROCESS_PROTECT_NORMAL, 4);
    }

    @Override
    public void systemReady(final Runnable goingCallback) {
        if (mLmkTracker == null) {
            mLmkTracker = new LmKillerTracker();
            mLmkTracker.start();
        }

        //SPRD: Initiate the mCacheProtectList! @{
        String[] protectProcesses = super.mContext.getResources()
                                    .getStringArray(com.android.internal.R.array.cache_protect_processes);
        if (protectProcesses != null && protectProcesses.length > 0) {
            mCacheProtectList = Arrays.asList(protectProcesses);
        }
        //SPRD: @}

        super.systemReady(goingCallback);
        HandlerThread mAdjDropThread = new HandlerThread("Am-ex-drop",Process.THREAD_PRIORITY_BACKGROUND);
        mAdjDropThread.start();
        Handler mAdjDropH = new Handler(mAdjDropThread.getLooper());
        mAdjDroper = new LowRamAdjDrop(mContext, mAdjDropH, this);
        mAdjDroper.schedUpdateUsageStat();
    }

    @Override
    public void onShellCommand(FileDescriptor in, FileDescriptor out,
            FileDescriptor err, String[] args, ResultReceiver resultReceiver) {
        (new ActivityManagerShellCommandEx(this, false)).exec(
                this, in, out, err, args, resultReceiver);
    }

    @Override
    public void startProcessLocked(ProcessRecord app,
            String hostingType, String hostingNameStr) {
        super.startProcessLocked(app, hostingType, hostingNameStr);
        if (app.processName != null) {
            ProtectArea pa = sPreProtectAreaList.get(app.processName);
            if (pa != null) {
                app.protectStatus = pa.mLevel;
                app.protectMinAdj = pa.mMinAdj;
                app.protectMaxAdj = pa.mMaxAdj;
            }
            if (DEBUG_AMSEX){
                Slog.d(TAG, "startProcessLocked app.protectLevel :" + app.protectStatus
                        + " app.protectMinAdj :" + app.protectMinAdj
                        + " app.protectMaxAdj" + app.protectMaxAdj);
            }
        }
    }

    public void addProtectArea(final String processName, final ProtectArea area) {
        if(processName == null || area == null) {
            return;
        }
        if (DEBUG_AMSEX) Slog.d(TAG, "addProtectArea, processName: " + processName + " ProtectArea: " + area);
        mHandler.post(new Runnable() {

            @Override
            public void run() {
                synchronized (ActivityManagerServiceEx.this) {
                    sPreProtectAreaList.put(processName, area);
                    updateOomAdjLocked();
                }
            }
        });
    }

    public void removeProtectArea(final String processName) {
        if(processName == null) {
            return;
        }
        sPreProtectAreaList.remove(processName);
        mHandler.post(new Runnable() {

            @Override
            public void run() {
                synchronized (ActivityManagerServiceEx.this) {
                    ProcessRecord app = null;
                    for(int i = mLruProcesses.size() -1; i >= 0; i--) {
                        if(processName.equals(mLruProcesses.get(i).processName)) {
                            app = mLruProcesses.get(i);
                            break;
                        }
                    }
                    if (DEBUG_AMSEX) Slog.d(TAG, "removeProtectArea, processName: " + processName + " app: " + app);
                    if(app != null) {
                        updateOomAdjLocked(app);
                    }
                }
            }
        });
    }

    @Override
    public int startActivity(IApplicationThread caller, String callingPackage, Intent intent, String resolvedType,
                IBinder resultTo, String resultWho, int requestCode, int startFlags, ProfilerInfo profilerInfo, Bundle options) {
        IntentHooks.getInstance().hookIntentIfNeeded(intent, IntentHooks.COMPONENT_TYPE_ACTIVITY);
        return super.startActivity(caller, callingPackage, intent, resolvedType, resultTo, resultWho, requestCode, startFlags, profilerInfo, options);
    }

    boolean isPendingBroadcastProcessLocked(int pid) {
        if(!sSupportCompatibityBroadcast){
            return super.isPendingBroadcastProcessLocked(pid);
        }
        return super.isPendingBroadcastProcessLocked(pid)
                || mCtsFgBroadcastQueue.isPendingBroadcastProcessLocked(pid)
                || mCtsBgBroadcastQueue.isPendingBroadcastProcessLocked(pid);
    }

    public BroadcastQueue broadcastQueueForIntent(String callerPackage, Intent intent) {
        if(!sSupportCompatibityBroadcast){
            return broadcastQueueForIntent(intent);
        }
        final boolean isCtsPkg = isCtsPackage(callerPackage) || isSpecialAction(intent);
        final boolean isFg = isPutForeground(intent) || ((intent.getFlags() & Intent.FLAG_RECEIVER_FOREGROUND) != 0);
        if (ActivityManagerDebugConfig.DEBUG_BROADCAST_BACKGROUND && isCtsPkg) {
            Slog.i(TAG, "Check broadcast intent " + intent + " on "
                    + (isFg ? (isCtsPkg ? "foreground-comp" : "foreground") : (isCtsPkg ? "background-comp" : "background")) + " queue");
        }
        if(isFg == true && isCtsPkg == false){
            return mBroadcastQueues[0];
        }else if(isFg == false && isCtsPkg == false){
            return mBroadcastQueues[1];
        }else if(isFg == true && isCtsPkg == true){
            return mBroadcastQueues[2];
        }else{
            return mBroadcastQueues[3];
        }
    }

    private boolean isCtsPackage(String pkg) {
        if(!sSupportCompatibityBroadcast){
            return false;
        }
        if (pkg != null && pkg.contains("com.android.cts")) {
            return true;
        }
        return false;
    }

    private boolean isPutForeground(Intent intent) {
        if (intent.getAction() == null || mBroadcastPutForeground == null || mBroadcastPutForeground.length <= 0) {
            return false;
        }
        for (int i = 0; i < mBroadcastPutForeground.length; i++) {
            if (intent.getAction().equals(mBroadcastPutForeground[i])) {
                Slog.d(TAG,"put to foreground broadcast queue : "+intent.getAction());
                intent.setFlags(Intent.FLAG_RECEIVER_FOREGROUND);
                return true;
            }
        }
        return false;
    }

    private boolean isSpecialAction(Intent intent) {
        if (intent.getAction() == null || mSpecialActions == null || mSpecialActions.length <= 0) {
            return false;
        }
        for (int i = 0; i < mSpecialActions.length; i++) {
            if (intent.getAction().equals(mSpecialActions[i])) {
                Slog.d(TAG,"special broadcast add to cts broadcast list : "+intent.getAction());
                return true;
            }
        }
        return false;
    }

    public BroadcastQueue[] getFgOrBgQueues(int flags) {
        if(!sSupportCompatibityBroadcast){
            return super.getFgOrBgQueues(flags);
        }
        boolean foreground = (flags & Intent.FLAG_RECEIVER_FOREGROUND) != 0;
        return foreground ? new BroadcastQueue[] { mFgBroadcastQueue, mCtsFgBroadcastQueue } : new BroadcastQueue[] { mBgBroadcastQueue, mCtsBgBroadcastQueue };
    }

     @Override
    public List<ActivityManager.RunningAppProcessInfo> getRunningAppProcesses() {
        List<ActivityManager.RunningAppProcessInfo> runningAppList = super.getRunningAppProcesses();
        if (runningAppList == null || runningAppList.size() <= 0) return runningAppList;
        if (mHidePackages == null || mHidePackages.size() <= 0) return runningAppList;
        List<ActivityManager.RunningAppProcessInfo> loopList = new ArrayList<ActivityManager.RunningAppProcessInfo>(runningAppList);
        for (ActivityManager.RunningAppProcessInfo appInfo : loopList) {
            if (appInfo.pkgList == null || appInfo.pkgList.length <= 0) continue;
            for (String pkg : appInfo.pkgList) {
                if (mHidePackages.contains(pkg)) {
                    Slog.d(TAG, "mHidePackages contains " + pkg + ", remove it");
                    runningAppList.remove(appInfo);
                }
            }
        }
        return runningAppList;
    }

}
