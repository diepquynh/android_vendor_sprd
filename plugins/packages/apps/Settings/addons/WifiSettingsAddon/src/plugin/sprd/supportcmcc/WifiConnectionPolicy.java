package plugin.sprd.supportcmcc;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collection;
import java.util.List;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.NetworkInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiConfiguration.Status;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManagerEx;
import android.os.Debug;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

public class WifiConnectionPolicy extends BroadcastReceiver {

    private static final String TAG = "WifiConnectionPolicy";
    private static final boolean DBG = true;

    // network select policy
    public static final int MOBILE_TO_WLAN_AUTO = 0;
    public static final int MOBILE_TO_WLAN_MANUAL = 1;
    public static final int MOBILE_TO_WLAN_ASK = 2;
    private static final int DIALOG_INTERVAL_MS = 60 * 60 * 1000;

    // network select dialog type
    static final int DIALOG_TYPE_WLAN_TO_WLAN = 0;
    static final int DIALOG_TYPE_MOBILE_TO_WLAN_AUTO = 1;
    static final int DIALOG_TYPE_MOBILE_TO_WLAN_MANUAL = 2;
    static final int DIALOG_TYPE_MOBILE_TO_WLAN_ALWAYS_ASK = 3;
    static final int DIALOG_TYPE_WLAN_TO_MOBILE = 4;
    static final int DIALOG_TYPE_CONNECT_TO_CMCC = 5;

    static final int YES_AND_REMEMBERED = 1;
    static final int NO_AND_REMEMBERED = 2;

    // do_not_prompt flag for network select dialog
    static final String DIALOG_WLAN_TO_WLAN = "dialog_wlan_to_wlan";
    static final String DIALOG_MOBILE_TO_WLAN_MANUAL = "dialog_mobile_to_wlan_manual";
    static final String DIALOG_MOBILE_TO_WLAN_ALWAYS_ASK = "dialog_mobile_to_wlan_always_ask";
    static final String DIALOG_WLAN_TO_MOBILE = "dialog_wlan_to_mobile";
    static final String AIRPLANE_MODE_WIFI_NOTIFICATION_FLAG = "airplane_mode_wifi_notification_flag";

    /** show dialog when value is 1, and do not show the dialog when the value is 0. */
    static final String DIALOG_CONNECT_TO_CMCC = "dialog_connect_to_cmcc";

    // dialog extra flag.
    static final String INTENT_EXTRA_DIALOG_TYPE = "dialogType";
    static final String INTENT_EXTRA_SSID_NAME = "ssid_name";
    static final String INTENT_EXTRA_SSID_ID = "ssid_id";
    static final String INTENT_EXTRA_SSIDS_NAME = "ssids_name";
    static final String INTENT_EXTRA_SSIDS_ID = "ssids_id";

    // wifi connect alarm flag, and its hour and minute flags.
    public static final String WIFI_CONNECT_ALARM_FLAG = "wifi_connect_alarm_flag";
    public static final String WIFI_CONNECT_ALARM_HOUR = "wifi_connect_alarm_hour";
    public static final String WIFI_CONNECT_ALARM_MINUTE = "wifi_connect_alarm_minute";

    // wifi disconnect alarm flag, and its hour and minute flags.
    public static final String WIFI_DISCONNECT_ALARM_FLAG = "wifi_disconnect_alarm_flag";
    public static final String WIFI_DISCONNECT_ALARM_HOUR = "wifi_disconnect_alarm_hour";
    public static final String WIFI_DISCONNECT_ALARM_MINUTE = "wifi_disconnect_alarm_minute";
    public static final int INTERVAL_MILLIS = 1000 * 60 * 60 * 24; //24h

    private static final String DEFAULT_CMCC_AP_NAME = "\"CMCC\"";

    private static WifiManager mWifiManager;
    private static WifiManagerEx mWifiManagerEx;
    private static TelephonyManager mTelephonyManager;
    private static AlarmManager mAlarmManager;

    private static long mTimer = -1;
    private static boolean mDialogIsShowing = false;
    private static boolean isWpsRunning = false;
    private static boolean manulConnect = false;

    // The absent ap will be removed from scan results list after 3 times scan results update.
    private static final int ABSENT_AP_REMOVED_TIMES = 3;
    private static WifiInfo mLastWifiInfo = null;
    private static int mScanTimes = 0;

    // if mManualDialogCancleFlag is ture, do not show manualSelect dialog or mobileToWlan dialog
    public static boolean mManualDialogCancleFlag = false;

    // if mWlanToWlanDialogCancleFlag is ture, do not show wlanToWlan dialog
    public static boolean mWlanToWlanDialogCancleFlag = false;

    public static void init(Context context) {
        mWifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        mWifiManagerEx = new WifiManagerEx(context);
        mAlarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        mTelephonyManager = TelephonyManager.from(context);
        applyWifiPolicy(context, true);
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        // initialize values
        if(mWifiManager == null) mWifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        if(mWifiManagerEx == null) mWifiManagerEx = new WifiManagerEx(context);
        if(mAlarmManager == null) mAlarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        if (mTelephonyManager == null) mTelephonyManager = TelephonyManager.from(context);

        // skip all the CMCC processing is wps is running
        if(isWpsRunning) {
            logd("WPS is running");
            return;
        }

        String action = intent.getAction();
        logd("Received action = " + action);
        // apply wifi policy after wifi is turned on
        if(WifiManager.WIFI_STATE_CHANGED_ACTION.equals(action)) {
            int wifiState = intent.getIntExtra(WifiManager.EXTRA_WIFI_STATE, WifiManager.WIFI_STATE_UNKNOWN);
            if (wifiState == WifiManager.WIFI_STATE_ENABLED) {
                manulConnect = false;
                mScanTimes = 0;
                mLastWifiInfo = null;
                mManualDialogCancleFlag = false;
                mWlanToWlanDialogCancleFlag = false;
                applyWifiPolicy(context, true);
            } else if (wifiState == WifiManager.WIFI_STATE_DISABLED) {
                if (Settings.Global.getInt(context.getContentResolver(), AIRPLANE_MODE_WIFI_NOTIFICATION_FLAG, 0) == 0
                        && Settings.Global.getInt(context.getContentResolver(), Settings.Global.AIRPLANE_MODE_ON, 0) == 1) {
                    Intent i = new Intent(context, AirplaneModeWifiAlertActivity.class);
                    i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    context.startActivity(i);
                    logd(" start AirplaneModeWifiAlertActivity");
                }
            }
        } else if(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION.equals(action)) {
            SupplicantState state = (SupplicantState) intent.getParcelableExtra(WifiManager.EXTRA_NEW_STATE);
            if (manulConnect && SupplicantState.isConnecting(state)) {
                manulConnect = false;
            } else if (SupplicantState.DISCONNECTED == state && getMobileToWlanPolicy(context) != MOBILE_TO_WLAN_AUTO) {
                disableAutoConnect(true);
            }
        } else if(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION.equals(action)
                || WifiManagerEx.WIFI_SCAN_RESULTS_AVAILABLE_ACTION.equals(action)) {
            // show dialogs according to CMCC's specification after scan results is received
            if (mLastWifiInfo != null && ++mScanTimes > ABSENT_AP_REMOVED_TIMES) {
                mScanTimes = 0;
                mLastWifiInfo = null;
            }
            showMobileToWlanDialog(context);
        } else if (WifiManagerEx.WIFI_CONNECTED_AP_ABSENT_ACTION.equals(action)) {
            mWlanToWlanDialogCancleFlag = false;
            // only care about wifi connection lost, for cmcc case 4.10
            mLastWifiInfo = (WifiInfo) intent.getParcelableExtra(WifiManager.EXTRA_WIFI_INFO);
            handleDisconnectEvent(context);
        } else if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
            final NetworkInfo networkInfo = (NetworkInfo) intent
                    .getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
            boolean mWifiConnected = networkInfo != null && networkInfo.isConnected();
            logd("networkInfo : " + networkInfo);
            if (mWifiConnected
                    && Settings.Global.getInt(context.getContentResolver(), DIALOG_CONNECT_TO_CMCC, 1) == 1) {
                WifiInfo info = (WifiInfo) intent.getParcelableExtra(WifiManager.EXTRA_WIFI_INFO);
                if (info == null) {
                    info = mWifiManager.getConnectionInfo();
                }
                if (info != null) {
                    String ssid = info.getSSID();
                    logd("ssid is " + ssid);
                    if (!TextUtils.isEmpty(ssid) && DEFAULT_CMCC_AP_NAME.equals(ssid)) {
                        Intent i = new Intent(context, WifiConnectionPolicyDialogActivity.class);
                        i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                        i.putExtra(INTENT_EXTRA_DIALOG_TYPE, DIALOG_TYPE_CONNECT_TO_CMCC);
                        context.startActivity(i);
                        logd("connectedToCmccDialog is displayed!");
                    }
                }
            }
        }
        else if (WifiManagerEx.WIFI_DISABLED_WHEN_CONNECTED_ACTION.equals(action)) {
            if (!handledRememberedFlag(context, DIALOG_TYPE_WLAN_TO_MOBILE)) {
                mLastWifiInfo = (WifiInfo) intent.getParcelableExtra(WifiManager.EXTRA_WIFI_INFO);
                popUpWlanToMobileDialog(context);
            }
        }
        else if (WifiManagerEx.WIFI_SCAN_RESULT_BSS_REMOVED_ACTION.equals(action)) {
            // reset mLastWifiInfo and mScanTimes if the removed bss is the bss of mLastWifiInfo.
            String removedBss = intent.getStringExtra(WifiManager.EXTRA_BSSID);
            logd("removedBss = " + removedBss);
            if (mLastWifiInfo != null && removedBss != null && removedBss.equalsIgnoreCase(mLastWifiInfo.getBSSID())) {
                mScanTimes = 0;
                mLastWifiInfo = null;
            }
        }

        /* SPRD: add for cmcc case SF-085 @{ */
        else if (action.equals(WifiManagerEx.ALARM_FOR_CONNECT_WIFI_ACTION)) {
            setConnectWifiAlarm(context);
        } else if (action.equals(WifiManagerEx.ALARM_FOR_DISCONNECT_WIFI_ACTION)) {
            setDisonnectWifiAlarm(context);
        } else if(Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())){
            setConnectWifiAlarm(context);
            setDisonnectWifiAlarm(context);
        }
        /* @} */

        // Show wlanToWlan dialog when rss < -85db.
        else if (intent.getAction().equals(WifiManager.RSSI_CHANGED_ACTION)) {
            int rss = intent.getIntExtra(WifiManager.EXTRA_NEW_RSSI, 0);
            if (rss < -85) {
                if (getMobileToWlanPolicy(context) == MOBILE_TO_WLAN_AUTO) {
                    autoConnectOtherAp(context);
                } else {
                    if (!handledRememberedFlag(context, DIALOG_TYPE_WLAN_TO_WLAN)) {
                        disableAutoConnect(true);
                        showWlanToWlanDialog(context);
                    }
                }
            }
        }
    }

    void handleDisconnectEvent(Context context) {

        if (mDialogIsShowing || manulConnect) {
            return;
        }

        // wlan to mobile
        if (getMatchedAccessPoints(context).size() == 0) {
            if (!handledRememberedFlag(context, DIALOG_TYPE_WLAN_TO_MOBILE)) {
                disableAutoConnect(true);
                popUpWlanToMobileDialog(context);
            }
            return;
        }

        int wifiState = mWifiManager.getWifiState();
        if(isWifiConnectingOrConnected() || wifiState != WifiManager.WIFI_STATE_ENABLED) {
            // do not show dialog when connection is under processing or etablished
            logd("returned because of connecting or connected");
            return;
        }

        // wlan to wlan
        int mobileToWlan = getMobileToWlanPolicy(context);
        if (mobileToWlan == MOBILE_TO_WLAN_MANUAL) {
            if (!handledRememberedFlag(context, DIALOG_TYPE_MOBILE_TO_WLAN_MANUAL)) {
                disableAutoConnect(true);
                if (!isTimerRunning()) {
                    showMatchedAccessPoints(context, null);
                }
            }
        } else if (mobileToWlan == MOBILE_TO_WLAN_ASK) {
            if (!handledRememberedFlag(context, DIALOG_TYPE_MOBILE_TO_WLAN_ALWAYS_ASK)) {
                disableAutoConnect(true);
                if (!isTimerRunning()) {
                    showRecommendAccessPoint(context);
                }
            }
        }
    }

    void popUpWlanToMobileDialog(Context context) {
        if (Settings.Global.getInt(context.getContentResolver(), Settings.Global.AIRPLANE_MODE_ON, 0) != 0) {
            logd("Data-connection can not be opened on airplane mode, return");
            return;
        }

        if (!hasIccCard(context)) {
            logd("popUpWlanToMobileDialog, has no sim card and return");
            return;
        }

        // do not show dialog when connection is under processing or established
        if (isWifiConnectingOrConnected()) {
            logd("popUpWlanToMobileDialog, ap is connecting then return");
            return;
        }

        Intent i = new Intent(context,WifiConnectionPolicyDialogActivity.class);
        i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        i.putExtra(INTENT_EXTRA_DIALOG_TYPE, DIALOG_TYPE_WLAN_TO_MOBILE);
        context.startActivity(i);
        logd("wlan2MobileDialog is displayed");
        mDialogIsShowing = true;
    }

    private boolean hasIccCard(Context context) {
        int num = mTelephonyManager.getSimCount();
        for (int i = 0; i < num; i++) {
            if (mTelephonyManager.hasIccCard(i)) {
                return true;
            }
        }
        return false;
    }

    static void setDialogShowing(boolean flag) {
        mDialogIsShowing = flag;
    }

    public static void setMobileToWlanPolicy(Context context, int value) {
        Settings.Global.putInt(context.getContentResolver(), WifiManagerEx.WIFI_MOBILE_TO_WLAN_POLICY, value);
        applyWifiPolicy(context, false);
    }

    public static void setManulConnectFlags(boolean enabled) {
        logd("setManulConnectFlags enabled = " + enabled);
        manulConnect = enabled;
    }
    public static int getMobileToWlanPolicy(Context context) {
        int value =Settings.Global.getInt(context.getContentResolver(),
               WifiManagerEx.WIFI_MOBILE_TO_WLAN_POLICY, MOBILE_TO_WLAN_AUTO);
        return value;
    }

    public static void setWpsIsRunning(boolean value) {
        isWpsRunning = value;
    }

    public static boolean isWifiConnectingOrConnected() {
        if(mWifiManager != null) {
            WifiInfo wifiInfo = mWifiManager.getConnectionInfo();
            logd("wifiInfo = " + wifiInfo);
            if (wifiInfo == null) return false;
            if (wifiInfo.getNetworkId() == -1
                    || (mLastWifiInfo != null && mLastWifiInfo.getNetworkId() == wifiInfo.getNetworkId())) {
                // If the current network is the last diconnected ap, return false.
                return false;
            }
            SupplicantState state = wifiInfo.getSupplicantState();
            if(state == SupplicantState.AUTHENTICATING) return true;
            if(state == SupplicantState.ASSOCIATING) return true;
            if(state == SupplicantState.ASSOCIATED) return true;
            if(state == SupplicantState.FOUR_WAY_HANDSHAKE) return true;
            if(state == SupplicantState.GROUP_HANDSHAKE) return true;
            if(state == SupplicantState.COMPLETED) return true;
        }
        return false;
    }

    private static void enableAutoConnect() {
        if (mDialogIsShowing == false) {
            logd("enableAutoConnect() mWifiManager.reconnect()");
            //mWifiManager.reconnect();
            mWifiManagerEx.setAutoConnect(true);
        }
    }

    private static void disableAutoConnect(boolean forceDisconnect) {
        logd("disableAutoConnect");
        //if(!forceDisconnect && isWifiConnectingOrConnected()) {
        //    logd("do not disconnect when connection is under processing or etablished");
        //} else {
        //    mWifiManager.disconnect();
        //}
        mWifiManagerEx.setAutoConnect(false);
    }

    static void setTimer(long value) {
        logd("setTimer:" + value);
        mTimer = value;
    }

    public static void resetTimer() {
        logd("resetTimer");
        mTimer = -1;
    }

    private static boolean isTimerRunning() {
        boolean isTimerRunning;
        if(mTimer < 0) {
            isTimerRunning = false;
        } else {
            isTimerRunning = System.currentTimeMillis() - mTimer < DIALOG_INTERVAL_MS;
        }
        logd("isTimerRunning: " + isTimerRunning + ", mTimer = " + mTimer);
        return isTimerRunning;
    }

    private static void applyWifiPolicy(Context context, boolean forceDisconnect) {
        int mobileToWlan = getMobileToWlanPolicy(context);
        logd("applyWifiPolicy() mobileToWlan: " + mobileToWlan);
        if(mobileToWlan == MOBILE_TO_WLAN_AUTO) {
            enableAutoConnect();
        } else {
            disableAutoConnect(forceDisconnect);
        }
    }

    private static void showMobileToWlanDialog(Context context) {
        int mobileToWlan = getMobileToWlanPolicy(context);
        int wifiState = mWifiManager.getWifiState();

        if(isWifiConnectingOrConnected() || wifiState != WifiManager.WIFI_STATE_ENABLED
                || mDialogIsShowing == true) {
            logd("showMobileToWlanDialog() returned because of connecting or connected");
            return; // do not show dialog when connection is under processing or etablished
        }

        if (mobileToWlan == MOBILE_TO_WLAN_AUTO) {
            enableAutoConnect();
        } else {
            if (manulConnect == true) {
                return;
            }
            if (mobileToWlan == MOBILE_TO_WLAN_MANUAL) {
                if (!handledRememberedFlag(context, DIALOG_TYPE_MOBILE_TO_WLAN_MANUAL)) {
                    disableAutoConnect(true);
                    if (!isTimerRunning()) {
                        showMatchedAccessPoints(context, null);
                    }
                }
            } else if (mobileToWlan == MOBILE_TO_WLAN_ASK) {
                if (!handledRememberedFlag(context, DIALOG_TYPE_MOBILE_TO_WLAN_ALWAYS_ASK)) {
                    disableAutoConnect(true);
                    if (!isTimerRunning()) {
                        showRecommendAccessPoint(context);
                    }
                }
            }
        }
    }

    private static void showRecommendAccessPoint(Context context) {
        int wifiState = mWifiManager.getWifiState();
        if(isWifiConnectingOrConnected() || wifiState != WifiManager.WIFI_STATE_ENABLED) {
            logd("showRecommendAccessPoint in connecting state, then return");
            return;
        }
        final AccessPoint mRecommendAccessPoint = getRecommendAccessPoint(context);
        if (mRecommendAccessPoint != null) {
            Intent mIntent = new Intent(context, WifiConnectionPolicyDialogActivity.class);
            mIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            mIntent.putExtra(INTENT_EXTRA_DIALOG_TYPE, DIALOG_TYPE_MOBILE_TO_WLAN_ALWAYS_ASK);
            mIntent.putExtra(INTENT_EXTRA_SSID_NAME, mRecommendAccessPoint.ssid);
            mIntent.putExtra(INTENT_EXTRA_SSID_ID, mRecommendAccessPoint.networkId);
            context.startActivity(mIntent);
            mDialogIsShowing = true;
            logd("mobile2wlan recommend dialog is displayed.");
        }
    }

    /** SPRD: show dialog for manual policy */
    private static void showMatchedAccessPoints(Context context, WifiInfo mWifiInfo) {
        if (mManualDialogCancleFlag) {
            logd("mManualDialogCancleFlag = true, return");
            return;
        }
        int wifiState = mWifiManager.getWifiState();
        if(isWifiConnectingOrConnected() || wifiState != WifiManager.WIFI_STATE_ENABLED) {
            logd("showMatchedAccessPoints in connecting state, then return");
            return;
        }

        Collection<AccessPoint> accessPoints = getMatchedAccessPoints(context);
        if(accessPoints.size() > 0) {
            String[] mApSSIDs = new String[accessPoints.size()];
            final int mApIDs[] = new int[accessPoints.size()];
            int i=0;
            for(AccessPoint accessPoint : accessPoints) {
                mApSSIDs[i] = accessPoint.ssid;
                mApIDs[i++] = accessPoint.networkId;
            }
            Intent intent = new Intent(context, WifiConnectionPolicyDialogActivity.class);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.putExtra(INTENT_EXTRA_DIALOG_TYPE, DIALOG_TYPE_MOBILE_TO_WLAN_MANUAL);
            intent.putExtra(INTENT_EXTRA_SSIDS_NAME, mApSSIDs);
            intent.putExtra(INTENT_EXTRA_SSIDS_ID, mApIDs);
            context.startActivity(intent);
            mDialogIsShowing = true;
        }
    }


    /** SPRD: show dialog for wlanToWlan policy */
    private static void showWlanToWlanDialog(Context context) {
        if (mWlanToWlanDialogCancleFlag) {
            logd("mWlanToWlanDialogCancleFlag = true, return");
            return;
        }

        WifiInfo wifiInfo = mWifiManager.getConnectionInfo();
        if (wifiInfo == null || wifiInfo.getNetworkId() == -1) {
            logd("showWlanToWlanDialog wlan is not connected, return.");
            return;
        }
        Collection<AccessPoint> accessPoints = getMatchedAccessPoints(context);
        if(accessPoints.size() > 1) {
            String[] mApSSIDs = new String[accessPoints.size() -1];
            final int mApIDs[] = new int[accessPoints.size() -1];
            int i=0;
            for(AccessPoint accessPoint : accessPoints) {
                if (wifiInfo.getNetworkId() == accessPoint.networkId) {
                    continue;
                }
                mApSSIDs[i] = accessPoint.ssid;
                mApIDs[i++] = accessPoint.networkId;
            }
            Intent intent = new Intent(context, WifiConnectionPolicyDialogActivity.class);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.putExtra(INTENT_EXTRA_DIALOG_TYPE, DIALOG_TYPE_WLAN_TO_WLAN);
            intent.putExtra(INTENT_EXTRA_SSIDS_NAME, mApSSIDs);
            intent.putExtra(INTENT_EXTRA_SSIDS_ID, mApIDs);
            context.startActivity(intent);
            mDialogIsShowing = true;
        }
    }

    /** SPRD: show dialog for wlanToWlan policy */
    private static void autoConnectOtherAp(Context context) {
        logd("autoConnectOtherAp");
        WifiInfo wifiInfo = mWifiManager.getConnectionInfo();
        if (wifiInfo == null || wifiInfo.getNetworkId() == -1) {
            logd("autoConnectOtherAp wlan is not connected, return.");
            return;
        }
        Collection<AccessPoint> accessPoints = getMatchedAccessPoints(context);
        for(AccessPoint accessPoint : accessPoints) {
            if (wifiInfo.getNetworkId() == accessPoint.networkId) {
                continue;
            } else if (accessPoint.getConfig().status == Status.ENABLED) {
                logd("Disable the current network, wifi will connect to other enabled network.");
                mWifiManager.disableNetwork(wifiInfo.getNetworkId());
                return;
            }
        }
    }

    private static AccessPoint getRecommendAccessPoint(Context context) {
        logd("getRecommendAccessPoint");
        final Collection<AccessPoint> accessPoints = getMatchedAccessPoints(context);
        AccessPoint recommendAccessPoint = null;
        int priority = -1;
        for(AccessPoint accessPoint : accessPoints) {
            if(priority < accessPoint.getConfig().priority) {
                priority = accessPoint.getConfig().priority;
                recommendAccessPoint = accessPoint;
            }
        }
        if(recommendAccessPoint != null) logd("getRecommendAccessPoint " + recommendAccessPoint.ssid);
        return recommendAccessPoint;
    }

    private static List<AccessPoint> getMatchedAccessPoints(Context context) {
        logd("getMatchedAccessPoints mLastWifiInfo = " + mLastWifiInfo);

        ArrayList<AccessPoint> configuredAPs = new ArrayList<AccessPoint>();
        ArrayList<AccessPoint> scannedAPs = new ArrayList<AccessPoint>();
        ArrayList<AccessPoint> matchedAPs = new ArrayList<AccessPoint>();

        final List<WifiConfiguration> configs = mWifiManager.getConfiguredNetworks();
        if (configs != null) {
            for (WifiConfiguration config : configs) {
                AccessPoint accessPoint = new AccessPoint(context, config);
                configuredAPs.add(accessPoint);
            }
        }
        final List<ScanResult> results = mWifiManager.getScanResults();
        if (results != null) {
            for (ScanResult result : results) {
                AccessPoint accessPoint = new AccessPoint(context, result);
                scannedAPs.add(accessPoint);
            }
        }
        for (AccessPoint configuredAP : configuredAPs) {
            // The disconnected ap should not be added to matchedAPs,
            // it may be removed from scan results after 3 times scan results update.
            if (mLastWifiInfo != null && mLastWifiInfo.getNetworkId() == configuredAP.networkId) {
                continue;
            }
            for (AccessPoint scannedAP : scannedAPs) {
                if (configuredAP.ssid != null && scannedAP.ssid != null) {
                    if(configuredAP.ssid.equals(scannedAP.ssid) && configuredAP.security == scannedAP.security) {
                        matchedAPs.add(configuredAP);
                        break;
                    }
                }
            }
        }
        logd("getMatchedAccessPoints matchedAPs.size = " + matchedAPs.size());
        return matchedAPs;
    }

    static int getRememberedFlag(Context context, String key) {
        return Settings.Global.getInt(context.getContentResolver(), key, 0);
    }

    /** Every thing has been done when this method return ture. */
    static boolean handledRememberedFlag(Context context, int dialogType) {
        int rememberedFlag = 0;
        switch (dialogType) {
            case DIALOG_TYPE_WLAN_TO_WLAN:
                rememberedFlag = getRememberedFlag(context, DIALOG_WLAN_TO_WLAN);
                if (rememberedFlag == YES_AND_REMEMBERED) {
                    logd("WLAN_TO_WLAN YES_AND_REMEMBERED, reconnect and return ture");
                    // TODO -- disable the connected network and reconnect actions may be needed
                    autoConnectOtherAp(context);
                    return true;
                } else if (rememberedFlag == NO_AND_REMEMBERED) {
                    logd("WLAN_TO_WLAN NO_AND_REMEMBERED, disconnect and return ture");
                    return true;
                }
                break;
            case DIALOG_TYPE_WLAN_TO_MOBILE:
                rememberedFlag = getRememberedFlag(context, DIALOG_WLAN_TO_MOBILE);
                if (rememberedFlag == YES_AND_REMEMBERED) {
                    logd("WLAN_TO_MOBILE YES_AND_REMEMBERED, enable mobile data and return true");
                    mTelephonyManager.setDataEnabled(true);
                    return true;
                } else if (rememberedFlag == NO_AND_REMEMBERED) {
                    logd("WLAN_TO_MOBILE NO_AND_REMEMBERED, disable mobile data and return ture");
                    mTelephonyManager.setDataEnabled(false);
                    return true;
                }
                break;
            case DIALOG_TYPE_MOBILE_TO_WLAN_MANUAL:
                rememberedFlag = getRememberedFlag(context, DIALOG_MOBILE_TO_WLAN_MANUAL);
                if (rememberedFlag == YES_AND_REMEMBERED) {
                    logd("MOBILE_TO_WLAN_MANUAL YES_AND_REMEMBERED, reconnect and return ture");
                    enableAutoConnect();
                    return true;
                } else if (rememberedFlag == NO_AND_REMEMBERED) {
                    logd("MOBILE_TO_WLAN_MANUAL NO_AND_REMEMBERED, disconnect and return ture");
                    disableAutoConnect(true);
                    return true;
                }
                break;
            case DIALOG_TYPE_MOBILE_TO_WLAN_ALWAYS_ASK:
                rememberedFlag = getRememberedFlag(context, DIALOG_MOBILE_TO_WLAN_ALWAYS_ASK);
                if (rememberedFlag == YES_AND_REMEMBERED) {
                    logd("MOBILE_TO_WLAN_ALWAYS_ASK YES_AND_REMEMBERED, reconnect and return ture");
                    enableAutoConnect();
                    return true;
                } else if (rememberedFlag == NO_AND_REMEMBERED) {
                    logd("MOBILE_TO_WLAN_ALWAYS_ASK NO_AND_REMEMBERED, disconnect and return ture");
                    disableAutoConnect(true);
                    return true;
                }
                break;
            default:
                break;
        }
        return false;
    }

    /**
     * SPRD: set a cancle flag when the dialog cancle button is clicked.
     * @param flag
     */
    static void setManualDialogCancleFlag(boolean flag) {
        mManualDialogCancleFlag = flag;
    }

    /**
     * SPRD: set a cancle flag when wlanTowlan dialog cancle button is clicked.
     * @param flag
     */
    static void setWlanToWLanDialogCancleFlag(boolean flag) {
        mWlanToWlanDialogCancleFlag = flag;
    }

    private void setConnectWifiAlarm(Context context) {
        if (Settings.Global.getInt(context.getContentResolver(), WIFI_CONNECT_ALARM_FLAG, 0) == 1) {
            setAlarmAndConnectWifi(context);
        }
    }

    private void setDisonnectWifiAlarm(Context context) {
        if (Settings.Global.getInt(context.getContentResolver(), WIFI_DISCONNECT_ALARM_FLAG, 0) == 1) {
            setAlarmAndDisconnectWifi(context);
        }
    }

    private Calendar getCalendar(int hourOfDay, int minute) {
        Calendar calendar = Calendar.getInstance();
        calendar.set(Calendar.HOUR_OF_DAY, hourOfDay);
        calendar.set(Calendar.MINUTE, minute);
        calendar.set(Calendar.SECOND, 0);
        calendar.set(Calendar.MILLISECOND, 1);
        return calendar;
    }

    private void setAlarmAndConnectWifi(Context context) {
        int hourOfDay =Settings.Global.getInt(context.getContentResolver(), WIFI_CONNECT_ALARM_HOUR, 0);
        int minute =Settings.Global.getInt(context.getContentResolver(), WIFI_CONNECT_ALARM_MINUTE, 0);
        Calendar calendar = getCalendar(hourOfDay, minute);
        long inMillis = calendar.getTimeInMillis();
        int isDismiss = isDismissCalendar(hourOfDay, minute);
        if (isDismiss == 0) {
            inMillis += INTERVAL_MILLIS;
            enableWifi();
        } else if (isDismiss == 1) {
            inMillis += INTERVAL_MILLIS;
        }
        logd("setAlarmAndConnectWifi inMillis = " + inMillis + ", isDismiss = " + isDismiss);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(context, 0, new Intent(
                WifiManagerEx.ALARM_FOR_CONNECT_WIFI_ACTION), 0);
        mAlarmManager.cancel(pendingIntent);
        mAlarmManager.setExact(AlarmManager.RTC_WAKEUP, inMillis, pendingIntent);
    }

    private void setAlarmAndDisconnectWifi(Context context) {
        int hourOfDay =Settings.Global.getInt(context.getContentResolver(), WIFI_DISCONNECT_ALARM_HOUR, 0);
        int minute =Settings.Global.getInt(context.getContentResolver(), WIFI_DISCONNECT_ALARM_MINUTE, 0);
        Calendar calendar = getCalendar(hourOfDay, minute);
        long inMillis = calendar.getTimeInMillis();
        int isDismiss = isDismissCalendar(hourOfDay, minute);
        if (isDismiss == 0) {
            inMillis += INTERVAL_MILLIS;
            disableWifi();
        } else if (isDismiss == 1) {
            inMillis += INTERVAL_MILLIS;
        }
        logd("setAlarmAndDisconnectWifi inMillis = " + inMillis + ", isDismiss = " + isDismiss);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(context, 0, new Intent(
                WifiManagerEx.ALARM_FOR_DISCONNECT_WIFI_ACTION), 0);
        mAlarmManager.cancel(pendingIntent);
        mAlarmManager.setExact(AlarmManager.RTC_WAKEUP, inMillis, pendingIntent);
    }

    private void disableWifi() {
        logd("disableWifi");
        if (mWifiManager.getWifiState() != WifiManager.WIFI_STATE_DISABLED) {
            mWifiManager.setWifiEnabled(false);
        }
    }

    private void enableWifi() {
        logd("enableWifi");
        if (mWifiManager.getWifiApState() != WifiManager.WIFI_AP_STATE_DISABLED) {
            mWifiManager.setWifiApEnabled(null, false);
        }
        if (mWifiManager.getWifiState() != WifiManager.WIFI_STATE_ENABLED) {
            mWifiManager.setWifiEnabled(true);
        }
    }

    private int isDismissCalendar(int hourOfDay, int minute) {
        Calendar calendar = Calendar.getInstance();
        int calendarH = calendar.get(Calendar.HOUR_OF_DAY);
        int calendarM = calendar.get(Calendar.MINUTE);
        logd("calendarH = " + calendarH + ", calendarM = "
                + calendarM + ", hourOfDay = " + hourOfDay + ", minute = " + minute);

        if (calendarH == hourOfDay) {
            if (calendarM == minute) {
                return 0;
            }
            else if(calendarM > minute){
                return 1;
            } else {
                return -1;
            }
        } else if (calendarH > hourOfDay) {
            return 1;
        }
        return -1;
    }

    private static void logd(String logString) {
        if (DBG) {
            Log.d(TAG, logString);
        }
    }
}
