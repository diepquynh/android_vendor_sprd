package com.sprd.generalsecurity.network;

import android.app.AlarmManager;
import android.app.AlertDialog;
import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.INetworkStatsService;
import android.net.INetworkStatsSession;
import android.net.NetworkInfo;
import android.net.NetworkStats;
import android.net.NetworkTemplate;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.WindowManager;

import com.sprd.generalsecurity.data.BlockStateProvider;

import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.Contract;
import com.sprd.generalsecurity.utils.DateCycleUtils;
import com.sprd.generalsecurity.utils.DateCycleUtils.DataRestriction;
import com.sprd.generalsecurity.utils.TeleUtils;
import com.sprd.generalsecurity.utils.Formatter;

import java.text.DateFormat;
import java.text.FieldPosition;
import java.text.ParsePosition;
import java.util.Date;

import static android.net.NetworkTemplate.buildTemplateMobileAll;


public class DataFlowService extends IntentService {
    private static final String TAG = "DataFlowService";

    private static long REPEAT_TIME = 15 * 60 * 1000; //15 mins

    private static final String REMIND_ACTION = "com.sprd.generalsecurity.network.alert";

    public DataFlowService() {
        super("DataFlowService");
    }

    private static final String KEY_MONTH_REMIND = "key_restrict_month_reminder";
    private static final String KEY_DAY_REMIND = "key_restrict_day_reminder";

    private static final String KEY_EDIT_MONTH_USED = "key_edit_month_used";
    private static final String KEY_SEEK_BAR_PERCENT = "seek_bar_restrict";

    private static final String KEY_MONTH_REMIND_TIME_TRIGGER_WARN = "sim_month_remind_time_trigger_warn";
    private static final String KEY_DAY_REMIND_TIME_TRIGGER = "sim_day_remind_time_trigger";

    private static final String KEY_MONTH_REMIND_TIME_TRIGGER_OVER = "sim_month_remind_time_trigger_over";

    private static final String PREFRIX_SHARED_PREF = "sim";

    private static final String REMIND_TYPE = Contract.EXTRA_ALERT_TYPE;
    private static final int REMIND_MONTH = Contract.ALERT_TYPE_MONTH;
    private static final int REMIND_DAY = Contract.ALERT_TYPE_DAY;

    private static final String TRIGGER_INDEX = "trigger";
    private static final String WARNIN_MSG_MONTH = "msg_month";
    private static final String WARNIN_MSG_DAY = "msg_day";


    private static final int TRIGGER1 = 1;
    private static final int TRIGGER2 = 2;

    private static final String PERCENT = "percent";
    private static final String SIMCARD = "simcard";

    private int mPrimaryCard;

    @Override
    protected void onHandleIntent(Intent intent) {
        if (intent.getBooleanExtra(Contract.EXTRA_SIM_STATE, false)) {
            checkSimStateChange();
            return;
        }

        //Check network state
        ConnectivityManager cm = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo ns = cm.getActiveNetworkInfo();

        boolean isConnected = ns != null && ns.isConnectedOrConnecting();

        mPrimaryCard = TeleUtils.getPrimarySlot(this);
        if (ns != null) {
            Log.e(TAG, "pri card:" + mPrimaryCard + ":" + ns.getState());
        }

        AlarmManager am = (AlarmManager) getSystemService(ALARM_SERVICE);
        PendingIntent pi = PendingIntent.getService(this, 0, new Intent(this, DataFlowService.class), PendingIntent.FLAG_UPDATE_CURRENT);
        boolean remindDisabled = ifAllRemindDisabled();
        Log.e(TAG, "isconn:" + isConnected + ":" + remindDisabled);
        if (isConnected && !remindDisabled) {
            if (ns.getType() == ConnectivityManager.TYPE_MOBILE) {
                //TODO: check mobile network only
                notifyUserForMonthIfNeeded(mPrimaryCard);
                am.set(AlarmManager.RTC, System.currentTimeMillis() + REPEAT_TIME, pi);
                Log.e(TAG, "start alarm");
            }
        } else {
            //network disconnected
            am.cancel(pi);
            Log.d(TAG, " not notify user since isConnected:" + isConnected + "  remindDisabled:" + remindDisabled);
        }
    }

    /**
     * Check if all remind disabled
     * @return true: all remind disabled
     *         false: some remind is enabled
     */
    private boolean ifAllRemindDisabled() {
        int simCount = TeleUtils.getSimCount(this);
        if (simCount == 1) {
            SharedPreferences pref = this.getSharedPreferences(PREFRIX_SHARED_PREF + mPrimaryCard,
                    Context.MODE_PRIVATE);

            boolean monthRemindEnabled = pref.getBoolean(KEY_MONTH_REMIND, false);
            boolean dayRemindEnabled = pref.getBoolean(KEY_DAY_REMIND, false);

            if (monthRemindEnabled || dayRemindEnabled) {
                return false;
            } else {
                return true;
            }
        } else if (simCount == 2) {
            SharedPreferences pref1 = this.getSharedPreferences(PREFRIX_SHARED_PREF + 1, Context.MODE_PRIVATE);
            boolean monthRemindEnabled1 = pref1.getBoolean(KEY_MONTH_REMIND, false);
            boolean dayRemindEnabled1 = pref1.getBoolean(KEY_DAY_REMIND, false);

            if (monthRemindEnabled1 || dayRemindEnabled1) {
                return false;
            }

            SharedPreferences pref2 = this.getSharedPreferences(PREFRIX_SHARED_PREF + 2, Context.MODE_PRIVATE);
            boolean monthRemindEnabled2 = pref2.getBoolean(KEY_MONTH_REMIND, false);
            boolean dayRemindEnabled2 = pref2.getBoolean(KEY_DAY_REMIND, false);
            if (monthRemindEnabled2 || dayRemindEnabled2) {
                return false;
            } else {
                return true;
            }
        }

        return true;
    }

    /**
     * Remind user once a day if needed.
     * @param dataUsed
     * @param sim
     */
    private void notifyUserForMonthIfNeeded(int sim) {
        int simCount = TeleUtils.getSimCount(this);
        INetworkStatsSession statsSession;

        INetworkStatsService statsService = INetworkStatsService.Stub.asInterface(
                ServiceManager.getService(Context.NETWORK_STATS_SERVICE));
        try {
            statsSession = statsService.openSession();
            statsService.forceUpdate();
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }

        NetworkTemplate template;
        NetworkStats mStats;

        if (TeleUtils.getSimCount(this) == 1) {
            template = buildTemplateMobileAll(TeleUtils.getActiveSubscriberId(this));
        } else {
            template = buildTemplateMobileAll(TeleUtils.getActiveSubscriberId(this, sim));
        }

        //month data check
        long t = DateCycleUtils.getMonthCycleStart();
        try {
            mStats = statsSession.getSummaryForAllUid(template, t, System.currentTimeMillis(), false);
        } catch (RemoteException e) {
            e.printStackTrace();
            return;
        }

        NetworkStats.Entry entry = null;
        long dataUsed = 0;
        int size = mStats != null ? mStats.size() : 0;
        for (int i = 0; i < size; i++) {
            entry = mStats.getValues(i, entry);

            // Decide how to collapse items together
            final int uid = entry.uid;
            dataUsed += (entry.rxBytes + entry.txBytes);
        }
        Log.d(TAG, "data:----" + dataUsed);
        String sz = Formatter.formatFileSize(this, dataUsed, false);

        //get month restrict quota
        DataRestriction dt = DateCycleUtils.getInstance().getDataFlowRestriction(this, sim);

        SharedPreferences pref = this.getSharedPreferences(PREFRIX_SHARED_PREF + sim, Context.MODE_PRIVATE);

        boolean monthRemindEnabled = pref.getBoolean(KEY_MONTH_REMIND, false);
        long dataUsedSetByUser = (long)Float.parseFloat(pref.getString(KEY_EDIT_MONTH_USED, "0")) * 1024 *1024;
        float percent = (float)(pref.getInt(KEY_SEEK_BAR_PERCENT, 0) + 50) / 100;
        Log.e(TAG, "per:" + percent + ":" + pref.getInt(KEY_SEEK_BAR_PERCENT, 0) + 50);

        //intent to start DataFlowMainEntry when notification clicked.
        Intent resultIntent = new Intent(this, DataFlowMainEntry.class);
        resultIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
//        resultIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        PendingIntent resultPendingIntent = PendingIntent.getActivity(this, 0, resultIntent, PendingIntent.FLAG_CANCEL_CURRENT);

        if (monthRemindEnabled) {
            // get data used ,
            dataUsed += dataUsedSetByUser;
            long remindDiffTriggerWarn = pref.getLong(KEY_MONTH_REMIND_TIME_TRIGGER_WARN, 0) - DateCycleUtils.getInstance().getDayCycleStart();
            long remindDiffTriggerOver = pref.getLong(KEY_MONTH_REMIND_TIME_TRIGGER_OVER, 0) - DateCycleUtils.getInstance().getDayCycleStart();
            Date date = new Date(pref.getLong(KEY_MONTH_REMIND_TIME_TRIGGER_WARN, 0));
            Log.e(TAG, "dt is:" + date + ":::" + dataUsed);
            Log.e(TAG, "get restrict:" + ":" + Formatter.formatFileSize(this, dt.monthRestriction, false) + ":" + Formatter.formatFileSize(this, dataUsed, false)
                           + ": timeDiff=" + remindDiffTriggerWarn + ":" + remindDiffTriggerOver);
            if (remindDiffTriggerWarn < 0 && dt.monthRestriction > 0 &&
                    dataUsed >= dt.monthRestriction * percent && dataUsed < dt.monthRestriction){
                //trigger 1
                String msg = "";

                if (simCount == 1) {
                    /*if phone has only one sim card*/
                    msg = String.format(getResources().getString(R.string.warning_msg), "", (int)(percent * 100));
                } else if (simCount == 2) {
                     /*if phone has two sim cards*/
                    if (sim == 1) {
                        msg = String.format(getResources().getString(R.string.warning_msg), "SIM1", (int)(percent * 100));
                    } else if (sim == 2) {
                        msg = String.format(getResources().getString(R.string.warning_msg), "SIM2", (int)(percent * 100));
                    }
                }
                Intent it = new Intent(REMIND_ACTION);
                it.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                it.putExtra(REMIND_TYPE, REMIND_MONTH);
                it.putExtra(WARNIN_MSG_MONTH, msg);
                startActivity(it);

                // Month reminder notificatin
                Notification notification = new Notification.Builder(this)
                        .setContentTitle(getResources().getString(R.string.data_flow_management))
                        .setContentText(msg)
                        .setContentIntent(resultPendingIntent)
                        .setSmallIcon(R.drawable.month_remind)
                        .setAutoCancel(true)
                        .setStyle(new Notification.BigTextStyle().bigText(msg))
                        .build();
                NotificationManager nm = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
                nm.notify(0, notification);

                pref.edit().putLong(KEY_MONTH_REMIND_TIME_TRIGGER_WARN, System.currentTimeMillis()).commit();
                Log.e(TAG, "remind display1:" + sim);
            } else if (remindDiffTriggerOver < 0 && dt.monthRestriction > 0 && dataUsed >= dt.monthRestriction) {
                //trigger 2
                String msg_month_SIM = "";
                if (simCount == 1) {
                    /*if phone has only one sim card*/
                    msg_month_SIM = String.format(getResources().getString(R.string.warning_msg_month_SIM), "");
                } else if (simCount == 2) {
                     /*if phone has two sim cards*/
                    if (sim == 1) {
                        msg_month_SIM = String.format(getResources().getString(R.string.warning_msg_month_SIM), "SIM1");
                    } else if (sim == 2) {
                        msg_month_SIM = String.format(getResources().getString(R.string.warning_msg_month_SIM), "SIM2");
                    }
                }

                Intent it = new Intent(REMIND_ACTION);
                it.putExtra(WARNIN_MSG_MONTH, msg_month_SIM);
                it.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                it.putExtra(REMIND_TYPE, REMIND_MONTH);
                startActivity(it);

                Notification notification = new Notification.Builder(this)
                        .setContentTitle(getResources().getString(R.string.data_flow_management))
                        .setContentText(msg_month_SIM)
                        .setContentIntent(resultPendingIntent)
                        .setAutoCancel(true)
                        .setSmallIcon(R.drawable.month_remind)
                        .setStyle(new Notification.BigTextStyle().bigText(msg_month_SIM))
                        .build();
                NotificationManager nm = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
                nm.notify(0, notification);
                pref.edit().putLong(KEY_MONTH_REMIND_TIME_TRIGGER_OVER, System.currentTimeMillis()).commit();
                Log.e(TAG, "remind display2:" + sim);
            }
        }

        //Day data check
        t = DateCycleUtils.getDayCycleStart();
        try {
            mStats = statsSession.getSummaryForAllUid(template, t, System.currentTimeMillis(), false);
        } catch (RemoteException e) {
            e.printStackTrace();
            return;
        }

        entry = null;
        dataUsed = 0;
        size = mStats != null ? mStats.size() : 0;
        for (int i = 0; i < size; i++) {
            entry = mStats.getValues(i, entry);

            // Decide how to collapse items together
            final int uid = entry.uid;
            dataUsed += (entry.rxBytes + entry.txBytes);
        }
        boolean dayRemindEnabled = pref.getBoolean(KEY_DAY_REMIND, false);
        if (dayRemindEnabled) {
            // get data used ,
            long remindDayDiffTrigger = pref.getLong(KEY_DAY_REMIND_TIME_TRIGGER, 0) - DateCycleUtils.getInstance().getDayCycleStart();
            Log.e(TAG, "get day restrict:" + ":" + Formatter.formatFileSize(this, dt.dayRestriction, false) + ":" + Formatter.formatFileSize(this, dataUsed, false)
            + ":" + (remindDayDiffTrigger < 0));
            if (remindDayDiffTrigger < 0 && dt.dayRestriction > 0){
                //trigger 1
                if (dataUsed >= dt.dayRestriction) {
                    String msgDaySIM = "";
                    if (simCount == 1) {
                    /*if phone has only one sim card*/
                        msgDaySIM = String.format(getResources().getString(R.string.warning_msg_day_SIM), "");
                    } else if (simCount == 2) {
                     /*if phone has two sim cards*/
                        if (sim == 1) {
                            msgDaySIM = String.format(getResources().getString(R.string.warning_msg_day_SIM), "SIM1");
                        } else if (sim == 2) {
                            msgDaySIM = String.format(getResources().getString(R.string.warning_msg_day_SIM), "SIM2");
                        }
                    }

                    Intent it = new Intent(REMIND_ACTION);
                    it.putExtra(WARNIN_MSG_DAY, msgDaySIM);
                    it.putExtra(REMIND_TYPE, REMIND_DAY);
                    it.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    startActivity(it);

                   /* String msg = getResources().getString(R.string.warning_msg_day);*/
                    Notification notification = new Notification.Builder(this)
                            .setContentTitle(getResources().getString(R.string.data_flow_management))
                            .setAutoCancel(true)
                            .setContentText(msgDaySIM)
                            .setContentIntent(resultPendingIntent)
                            .setSmallIcon(R.drawable.day_remind)
                            .setStyle(new Notification.BigTextStyle().bigText(msgDaySIM))
                            .build();

                    NotificationManager nm = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
                    nm.notify(1, notification);
                    pref.edit().putLong(KEY_DAY_REMIND_TIME_TRIGGER, System.currentTimeMillis()).commit();
                    Log.e(TAG, "remind display3:" + sim);
                }
            }
        }
        Log.e(TAG, "notifyUserForMonthIfNeeded for sim" + sim);
    }

    private void checkSimStateChange() {
        Log.e(TAG, "checkSim:");
        SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
        //get current sim count
        int simCount = TeleUtils.getSimCount(this);
        if (simCount == 1) {
            String storedNum, currentNum;
            if (TeleUtils.getPrimaryCard(this) == 0) {
                //sim 1 is primary
                storedNum = sharedPreferences.getString("sim1_num", null);
                currentNum = TeleUtils.getSimNumber(this, 0);
                Log.e(TAG, "single Num:" + storedNum + ":" + currentNum);
            } else {
                //sim2 is primary
                storedNum = sharedPreferences.getString("sim2_num", null);
                currentNum = TeleUtils.getSimNumber(this, 1);
                Log.e(TAG, "single Num:" + storedNum + ":" + currentNum);
            }

            if (storedNum != null && currentNum != null
                && !storedNum.equalsIgnoreCase(currentNum) ){
                Log.e(TAG, "prompt user to reset");
                startSimAlert();
            }
            return;
        } else {
            String storedNum = sharedPreferences.getString("sim1_num", null);
            String currentNum = TeleUtils.getSimNumber(this, 0);
            Log.e(TAG, "Num:" + storedNum + ":" + currentNum);

            if (storedNum != null && currentNum != null
                    && !storedNum.equalsIgnoreCase(currentNum)){
                Log.e(TAG, "prompt user to reset 2");
                startSimAlert();
                return;
            }
            String storedNum2 = sharedPreferences.getString("sim2_num", null);
            String currentNum2 = TeleUtils.getSimNumber(this, 1);
            Log.e(TAG, "Num2:" + storedNum2 + ":" + currentNum2);
            if (storedNum2 != null && currentNum2 != null
                    && !storedNum2.equalsIgnoreCase(currentNum2) ){
                Log.e(TAG, "prompt user to reset 3");
                startSimAlert();
            }
        }

        // get shared sim1 num, current sim1
    }

    private void startSimAlert() {
        Intent it = new Intent(REMIND_ACTION);
        it.putExtra(Contract.EXTRA_SIM_PROMPT, true);
        it.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(it);
    }
}
