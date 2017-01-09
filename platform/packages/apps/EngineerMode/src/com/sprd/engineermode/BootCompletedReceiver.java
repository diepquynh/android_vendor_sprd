
package com.sprd.engineermode;

import android.os.SystemProperties;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.net.LocalSocketAddress;
import android.content.SharedPreferences.Editor;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.utils.SocketUtils;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.text.TextUtils;
import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.util.Calendar;

import android.os.SystemClock;
import android.os.HandlerThread;
import android.net.Uri;
import java.util.Calendar;

import com.sprd.engineermode.utils.ShellUtils;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;
import com.sprd.engineermode.debuglog.slogui.SlogService;
import com.sprd.engineermode.telephony.userinfo.NetInfoRecordActivity;
import com.sprd.engineermode.telephony.userinfo.NetinfoRecordService;
import com.sprd.engineermode.telephony.userinfo.BehaviorInfoRecordActivity;
import com.sprd.engineermode.telephony.userinfo.BehaviorRecordService;

import com.sprd.engineermode.telephony.NotificationBroadcastReceiver;
import android.app.NotificationManager;
import android.app.Notification;
import android.app.PendingIntent;
import android.widget.RemoteViews;
import android.os.RemoteException;
import android.os.ServiceManager;

public class BootCompletedReceiver extends BroadcastReceiver {

    public static final String TAG = "EMBootCompletedReceiver";
    private static final String ADC_CALIBRATION = "adb_calibration";
    private static final String FIRST_BOOT_FLAG = "persist.sys.isfirstboot";
    private static final String CHANGE_NETMODE_BY_EM = "persist.sys.cmccpolicy.disable";
    private static final String PROPERTY_CP2_LOG = "persist.sys.cp2log";
    private static final String RF_CALIBRATION = "rf_calibration";
    private static final String DEVICE_INFO_ACTION = "com.sprd.engineermode.action.DEVICE_INFO";
    private static final String UNKNOWN = "UNKNOWN";

    private static final String KEY_BACKGROUNDSEARCH = "lte_backgroundsearch";
    private static final String KEY_TIMER = "lte_timer";
    private static final String KEY_RSRP = "lte_rsrp";

    /** BEGIN BUG547014 zhijie.yang 2016/05/09 SPRD:add mipi log function **/
    private static final String PROPERTIES_MIPI_CHANNEL = "persist.sys.mipi.channel";
    private static final String CHANNEL_CLOSE = "0";
    private static final String CHANNEL_TRANNING = "1";
    private static final String CHANNEL_WTL = "2";
    private static final String MIPI_LOG_FILE_PATH = "/sys/devices/soc/soc:mm/63a00000.sprd-mipi-log/channel";
    /** END BUG547014 zhijie.yang 2016/05/09 SPRD:add mipi log function **/

    private boolean isCP2On = false;
    private boolean isValidAdcCalibrate = true;
    private boolean isValidRfCalibrate = true;
    private static final int MSG_DEVICE_INFO = 1;
    private static final int MSG_AT_FAIL = 2;
    private int RFCalibration = 0;
    private Context mContext;
    private Context mCurContext;
    private final int NOTIFICATION_ID = 0x1123;
    private NotificationManager mNotifiCation;
    private static final String NEW_MESSAGE = "Notice From EngineerMode!";
    private static final String CLOSE_NOTICE ="After test,please close this notificaiton";

    /* SPRD: bug596031 modem loglevel setiing @{ */
    private static final String LOG_USER_CMD = "1,0,"
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\""
            + ","
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\"";

    private static final String LOG_DEBUG_CMD = "1,3,"
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\""
            + ","
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\"";
    /* @} */

    /*
     * private PowerManager.WakeLock wl; private KeyguardManager mKeyguardManager; private
     * KeyguardLock mLock; private PowerManager pm;
     */

    private static final String PLATFORM_VERIFY[][] = {
            {
                    "sc8830", "1C1FF"
            },
            {
                    "scx15", "1FF"
            },
    };

    @Override
    public void onReceive(Context context, Intent intent) {

        /**
         * create modem assert listener thread under situation as follow: 1.boot completed 2.when
         * EngineerMode Process has been killed
         */
        Uri uri = intent.getData();
        boolean isUser = SystemProperties.get("ro.build.type").equalsIgnoreCase("user");
        boolean isFirstBoot = SystemProperties.getBoolean(FIRST_BOOT_FLAG, true);
        mCurContext = context.getApplicationContext();
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(mCurContext);
        SharedPreferences setting = mCurContext.getSharedPreferences("settings",
                Context.MODE_PRIVATE);
        boolean qos = false;
        if (pref != null) {
            qos = pref.getBoolean("qos_switch", false);
        }
        if (qos) {
            SystemProperties.set("persist.sys.qosstate", "1");
        } else {
            SystemProperties.set("persist.sys.qosstate", "0");
        }

        SystemProperties.set("persist.sys.modemassertdump", "true");

        if(SystemProperties.getBoolean("persist.sys.open.net.record",false)){
            NetInfoRecordActivity.serviceIntent=new Intent(context.getApplicationContext(),NetinfoRecordService.class);
            //NetInfoRecordActivity.serviceIntent.setAction("com.sprd.userinfo.NETINFO_RECORD_ACTION");
            NetInfoRecordActivity.mIsOpenNetinfoRecord=true;
            context.startService(NetInfoRecordActivity.serviceIntent);
        }
        if(SystemProperties.getBoolean("persist.sys.open.user.record",false)){
            BehaviorInfoRecordActivity.bServiceIntent=new Intent(context.getApplicationContext(),BehaviorRecordService.class);
            //NetInfoRecordActivity.serviceIntent.setAction("com.sprd.userinfo.NETINFO_RECORD_ACTION");
            BehaviorInfoRecordActivity.mIsOpenBehaviorRecord=true;
            context.startService(BehaviorInfoRecordActivity.bServiceIntent);
        }
        if (isFirstBoot) {
            /** Begin 549198 */
            IATUtils.sendATCmd(engconstents.ENG_AUTO_ANSWER + "0", "atchannel0");
            Log.d(TAG, "CLOSE_AUTO_ANSWER..");
            /** End 549198 */
            SystemProperties.set(FIRST_BOOT_FLAG, "false");
            SystemProperties.set(CHANGE_NETMODE_BY_EM, "false");
            /* SPRD: bug596031 modem loglevel setiing@{ */
            if (!isUser) {
                IATUtils.sendATCmd(engconstents.ENG_SET_LOG_LEVEL + LOG_DEBUG_CMD, "atchannel0");
                enableCPLogs();
            } else {
                IATUtils.sendATCmd(engconstents.ENG_SET_LOG_LEVEL + LOG_USER_CMD, "atchannel0");
                disableCPLogs();
            }
            /* @} */
            SystemProperties.set("persist.radio.fd.disable", "0");
        } else {
            Editor editor = pref.edit();
            editor.putString(KEY_BACKGROUNDSEARCH, null);
            editor.putString(KEY_TIMER, null);
            editor.putString(KEY_RSRP, null);
            // usb charge in Hardware invalid when UE restart
            editor.putBoolean("usb_charge", true);
            // manual assert enable when restart
            editor.putBoolean("key_manualassert", true);
            // restart UE DNS Filter invalid
            editor.putBoolean("dns_filter", false);
            editor.putBoolean("audio_switch", false);
            editor.putBoolean("communicate_switch", false);
            editor.commit();
            // slog storage notify listener invalid when UE restart
            Editor settingEditor = setting.edit();
            settingEditor.putBoolean("log_path_chage_to_internel", false).commit();
        }
        SystemProperties.set("persist.sys.nosleep.enabled", "0");
        if (!SystemProperties.get("ro.modem.wcn.enable", "0").equals("0")) {
            modifyCP2Log();
        }
        if (SystemProperties.getBoolean("persist.sys.engineermode.dns", false)) {
            Log.d(TAG, "start enable_dns");
            SystemProperties.set("ctl.start", "enable_dns");
        }
        new Thread(new Runnable() {
            public void run() {
                String armLogSwitch = IATUtils.sendATCmd(engconstents.ENG_AT_GETARMLOG1,
                        "atchannel0");
                Log.d(TAG, "ARM LOG Switch status is: " + armLogSwitch);
            }
        }).start();
        /** BEGIN BUG547014 zhijie.yang 2016/05/09 SPRD:add mipi log function **/
        final String mipiChannel = SystemProperties.get(PROPERTIES_MIPI_CHANNEL, CHANNEL_CLOSE);
        Log.d(TAG, "Get the channel of mipi is: " + mipiChannel);
        if (!CHANNEL_CLOSE.equals(mipiChannel)) {
            new Thread(new Runnable() {
                public void run() {
                    if (ShellUtils.writeToFile(MIPI_LOG_FILE_PATH, mipiChannel)) {
                        Log.d(TAG, "BootCompleted set mipi sucess");
                    } else {
                        Log.d(TAG, "BootCompleted set mipi fail");
                    }
                }
            }).start();
        }
        /** END BUG547014 zhijie.yang 2016/05/09 SPRD:add mipi log function **/
        /**
         * Modify Bug 358798 when insert sim in slot 2, the default value of netmode selection
         * depends cmcc policy
         */
        if (TelephonyManager.from(context).getSimState(1) == TelephonyManager.SIM_STATE_READY) {
            Log.d(TAG, "slot 2 has card the persist.sys.cmccpolicy.disable is false");
            SystemProperties.set(CHANGE_NETMODE_BY_EM, "false");
        }
        Log.d(TAG, "get vSystemProperties is " +SystemProperties.get("persist.radio.engtest.enable"));
        if(SystemProperties.get("persist.radio.engtest.enable").contains("true"))
        {
             initNotifiCation();
        }
    }
    private void initNotifiCation() {
        mNotifiCation = (NotificationManager)mCurContext.getSystemService(mCurContext.NOTIFICATION_SERVICE);
        RemoteViews mRemoteViews = new RemoteViews(mCurContext.getPackageName(), R.layout.notification);
        mRemoteViews.setTextViewText(R.id.mt_notification,NEW_MESSAGE+"\n"+"    "+CLOSE_NOTICE);
        Intent clickIntent = new Intent(mCurContext, NotificationBroadcastReceiver.class);
        clickIntent.putExtra("notificationId", NOTIFICATION_ID);
        PendingIntent pendingIntent= PendingIntent.getBroadcast(mCurContext, 0, clickIntent, PendingIntent.FLAG_UPDATE_CURRENT);
        mRemoteViews.setOnClickPendingIntent(R.id.bt_notification, pendingIntent);
        Notification notify = new Notification.Builder(mCurContext)
            .setContent(mRemoteViews)
            .setAutoCancel(true)
            .setTicker(NEW_MESSAGE)
            .setContentTitle(NEW_MESSAGE)
            .setPriority(Notification.PRIORITY_DEFAULT)
            .setOngoing(true)
            .setSmallIcon(R.drawable.cg)
            .build();
        mNotifiCation.notify(NOTIFICATION_ID,notify);
    }

    // the first power on, userdebug it need to open the arm log、cp2 log、DSP log、cap log
    private void enableCPLogs() {
        new Thread(new Runnable() {
            public void run() {
                SystemProperties.set(PROPERTY_CP2_LOG, "1");
                IATUtils.sendATCmd(engconstents.ENG_AT_SETARMLOG1 + "1", "atchannel0");
                IATUtils.sendATCmd(engconstents.ENG_AT_SETCAPLOG1 + "1", "atchannel0");
                IATUtils.sendATCmd(engconstents.ENG_AT_SETDSPLOG1 + "2", "atchannel0");
            }
        }).start();
    }

    // the first power on, user it need to close the arm log、cp2 log、DSP log、cap log
    private void disableCPLogs() {
        new Thread(new Runnable() {
            public void run() {
                SystemProperties.set(PROPERTY_CP2_LOG, "0");
                IATUtils.sendATCmd(engconstents.ENG_AT_SETARMLOG1 + "0", "atchannel0");
                IATUtils.sendATCmd(engconstents.ENG_AT_SETCAPLOG1 + "0", "atchannel0");
                IATUtils.sendATCmd(engconstents.ENG_AT_SETDSPLOG1 + "0", "atchannel0");
            }
        }).start();
    }

    private void modifyCP2Log() {
        // bug 311782 & 355088
        new Thread(new Runnable() {
            @Override
            public void run() {
                String cp2LogStatus = SystemProperties.get(PROPERTY_CP2_LOG, "");
                String cp2Power = SocketUtils.sendCmdAndRecResult("wcnd",
                        LocalSocketAddress.Namespace.ABSTRACT, "wcn poweron");
                Log.d(TAG, "cp2Power status ->" + cp2Power);
                if (cp2Power != null && cp2Power.contains(SocketUtils.OK)) {
                    isCP2On = true;
                } else {
                    return;
                }
                String cp2LogCurr = SocketUtils.sendCmdAndRecResult("wcnd",
                        LocalSocketAddress.Namespace.ABSTRACT, "wcn at+armlog?");
                /*
                 * SPRD: modify 20140605 Spreadtrum of 317471 NullPointer Exception @{
                 */
                if (cp2LogCurr == null) {
                    Log.d(TAG, "cp2LogCurr = null , return");
                    return;
                }
                /* @} */
                /*
                 * SPRD:This is for marlin cp2 default power off and log closed when cp2 power off,
                 * send wcnd at+armlog, return FAIL WCN-CP2-CLOSED
                 */
                if (cp2LogCurr != null && cp2LogCurr.contains("FAIL")) {
                    Log.d(TAG, "cp2 is power off");
                    return;
                }
                String[] str = cp2LogCurr.split(":");

                // check cp2LogCurr to avoid engineermode crash when the cp2LogCurr value is wrong
                if (str.length < 2) {
                    Log.d(TAG, "cp2LogCurr value error , return");
                    return;
                }
                cp2LogCurr = str[1].trim();
                Log.d(TAG, "cp2LogStatus->" + cp2LogStatus + ",cp2LogCurr->" + cp2LogCurr);

                if (!"".equals(cp2LogStatus)) {
                    if (!(("1".equals(cp2LogStatus) && "1".equals(cp2LogCurr)) || ("0"
                            .equals(cp2LogStatus) && "0".equals(cp2LogCurr)))) {
                        if ("1".equals(cp2LogStatus)) {
                            SocketUtils.sendCmdAndRecResult("wcnd",
                                    LocalSocketAddress.Namespace.ABSTRACT, "wcn at+armlog=1");
                            Calendar c = Calendar.getInstance();
                            int month = c.get(Calendar.MONTH) + 1;
                            int day = c.get(Calendar.DAY_OF_MONTH);
                            int hour = c.get(Calendar.HOUR);
                            int minute = c.get(Calendar.MINUTE);
                            int second = c.get(Calendar.SECOND);
                            int millisecond = c.get(Calendar.MILLISECOND);
                            Log.d(TAG, "month: " + month + "day: " + day + "hour: " + hour
                                    + "minute: " + minute + "second: " + second + "millisecond: "
                                    + millisecond);
                            String cmd = "wcn " + "at+aptime=" + month + "," + day + "," + hour
                                    + "," + minute + "," + second + "," + millisecond + "\r";
                            SocketUtils.sendCmdAndRecResult("wcnd",
                                    LocalSocketAddress.Namespace.ABSTRACT, cmd);
                        } else if ("0".equals(cp2LogStatus)) {
                            SocketUtils.sendCmdAndRecResult("wcnd",
                                    LocalSocketAddress.Namespace.ABSTRACT, "wcn at+armlog=0");
                        }
                    }
                }
                if (isCP2On) {
                    isCP2On = false;
                    String cp2PowerOff = SocketUtils.sendCmdAndRecResult("wcnd",
                            LocalSocketAddress.Namespace.ABSTRACT, "wcn poweroff");
                    Log.d(TAG, "cp2PowerOff status ->" + cp2PowerOff);
                }
            }
        }).start();
    }

    private void handleAction(Context context, String action) {
        if (context != null & action != null) {
            mContext = context;
            boolean isUser = SystemProperties.get("ro.build.type").equalsIgnoreCase("user");
            if (!isUser) {
                startVerify();
                Log.d(TAG, "this version is userdebug");
            } else {
                return;
            }
        }
    }

    Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_DEVICE_INFO: {
                    if (isValidAdcCalibrate && isValidRfCalibrate) {
                        /**
                         * All Verify OK,so no need to start Paint service,do nothing
                         */
                    } else {
                        /*
                         * mKeyguardManager = (KeyguardManager)
                         * getSystemService(Context.KEYGUARD_SERVICE); pm=(PowerManager)
                         * getSystemService(Context.POWER_SERVICE); deviceInfoShowPrepare();
                         */
                        Intent intent = new Intent(DEVICE_INFO_ACTION);
                        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                                | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
                        intent.putExtra(ADC_CALIBRATION, isValidAdcCalibrate);
                        intent.putExtra(RF_CALIBRATION, isValidRfCalibrate);
                        mContext.startActivity(intent);
                    }
                }
                    break;
                case MSG_AT_FAIL: {
                    Log.e(TAG, "At command Fail!");
                    // Toast.makeText(mContext, "At command Fail!",
                    // Toast.LENGTH_SHORT).show();
                }
                    break;
            }
        }
    };

    /*
     * private void deviceInfoShowPrepare(){ if(mKeyguardManager.inKeyguardRestrictedInputMode()){
     * if(mLock == null) { mLock = mKeyguardManager.newKeyguardLock("EngineerMode");
     * mLock.disableKeyguard(); } if(wl == null) { wl =pm.newWakeLock(PowerManager
     * .ACQUIRE_CAUSES_WAKEUP|PowerManager.SCREEN_DIM_WAKE_LOCK, "WakeLock"); wl.acquire(); } try {
     * Thread.sleep(2000); } catch (InterruptedException e) {} } }
     */

    private void startVerify() {
        new Thread(new Runnable() {
            public void run() {
                String str = IATUtils.sendATCmd("AT+SGMR=0,0,4", "atchannel0");
                Log.d(TAG, "startVerify get result str = " + str);
                if (str.contains(IATUtils.AT_OK)) {
                    String[] paser = str.split("\n");
                    String[] paser1 = paser[0].split(":");
                    String bitString = getBitStr(paser1[1].trim());
                    Log.d(TAG, "bitString: " + bitString);

                    isValidAdcCalibrate = ((Long.parseLong(bitString, 16) & Long.parseLong(
                            "00000200", 16)) != 0);
                    isValidRfCalibrate = true;
                    Log.d(TAG, "isValidAdcCalibrate = " + isValidAdcCalibrate
                            + ",isValidRfCalibrate = " + isValidRfCalibrate);
                    mHandler.sendEmptyMessage(MSG_DEVICE_INFO);
                } else {
                    mHandler.sendEmptyMessage(MSG_AT_FAIL);
                }
            }

            private String getBitStr(String str) {
                String result = null;
                int ind = str.indexOf("0x");
                result = str.substring(ind + 2);
                return result.trim();
            }
        }).start();
    }

    /**
     * Verify the ADC Calibrate info is Valid
     * 
     * @param deviceInfo: the version of calibrate info
     * @return boolean
     */
    private boolean verifyAdcInfo(String deviceInfo) {
        if (deviceInfo.equals("") || !deviceInfo.contains(IATUtils.AT_OK)) {
            return false;
        }
        Log.d(TAG, "deviceInfo is " + deviceInfo);
        String str[] = deviceInfo.split("\n");
        if (str[10].contains("BIT9")) {
            Log.d(TAG, "ADC CALIBRATION SUCCESS");
            return true;
        } else {
            Log.d(TAG, "ADC CALIBRATION FAIL");
            return false;
        }
    }

    /**
     * Verify the ADC Calibrate info is Valid
     * 
     * @param deviceInfo: the version of calibrate info
     * @return boolean
     */
    private boolean verifyRfInfo(String deviceInfo) {
        int result = 0;
        if (deviceInfo.equals("")) {
            return false;
        }
        Log.d(TAG, "deviceInfo is " + deviceInfo);
        boolean verisionValue = getVerisionValue(deviceInfo);
        return verisionValue;
    }

    private boolean getVerisionValue(String deviceInfo) {
        if (deviceInfo == null) {
            return false;
        } else {
            String ver = SystemProperties.get("ro.board.platform").trim();
            if (ver != null && ver.length() > 0) {
                Log.d(TAG, "ro.board.platform = " + ver);
                if (ver.contains("scx15")) {
                    if (deviceInfo.contains("BIT0") && deviceInfo.contains("BIT1")
                            && deviceInfo.contains("BIT2") &&
                            deviceInfo.contains("BIT3") && deviceInfo.contains("BIT4")
                            && deviceInfo.contains("BIT5") &&
                            deviceInfo.contains("BIT6") && deviceInfo.contains("BIT7")
                            && deviceInfo.contains("BIT8")) {
                        return true;
                    } else {
                        return false;
                    }
                } else if (ver.contains("sc8830")) {
                    if (deviceInfo.contains("BIT0") && deviceInfo.contains("BIT1")
                            && deviceInfo.contains("BIT2") &&
                            deviceInfo.contains("BIT3") && deviceInfo.contains("BIT4")
                            && deviceInfo.contains("BIT5") &&
                            deviceInfo.contains("BIT6") && deviceInfo.contains("BIT7")
                            && deviceInfo.contains("BIT8") &&
                            deviceInfo.contains("BIT14") && deviceInfo.contains("BIT15")
                            && deviceInfo.contains("BIT16")) {
                        return true;
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
    }
}
