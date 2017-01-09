/*
 *Created by spreadst
 */

package android.net.wifi;

import android.content.Context;
import android.content.res.Resources;
import android.provider.Settings;
import android.util.Log;

public class WifiManagerEx {

    private static final String TAG = "WifiManagerEx";

    /**
     * SPRD: add for cmcc wifi feature, 0 is auto switch, 1 is manual switch, 2 is always ask.
     * @hide
     */
    public static final String WIFI_MOBILE_TO_WLAN_POLICY = "wifi_mobile_to_wlan_policy";

    /**
     * SPRD: Broadcast intent action indicating the ap is removed from scan_results.
     * @hide
     */
    public static final String WIFI_SCAN_RESULT_BSS_REMOVED_ACTION
        = "sprd.net.wifi.BSS_REMOVED_ACTION";

    /**
     * SPRD: Broadcast intent action indicating the connected ap is absent.
     * @hide
     */
    public static final String WIFI_CONNECTED_AP_ABSENT_ACTION
        = "sprd.net.wifi.WIFI_CONNECTED_AP_ABSENT";

    /**
     * SPRD: Broadcast intent action indicating wifi is disabling with connected state.
     * @hide
     */
    public static final String WIFI_DISABLED_WHEN_CONNECTED_ACTION
        = "sprd.net.wifi.WIFI_DISABLED_WHEN_CONNECTED";

    /**
     * SPRD: Broadcast intent action indicating the ap is removed from scan_results.
     * @hide
     */
    public static final String WIFI_SCAN_RESULTS_AVAILABLE_ACTION
        = "sprd.net.wifi.SCAN_RESULTS_AVAILABLE";

    /**
     * SPRD: intent action indicating setting alarm to connect wifi
     * @hide
     */
    public static final String ALARM_FOR_CONNECT_WIFI_ACTION = "sprd.wifi.alarm.CONNECT_WIFI";

    /**
     * SPRD: intent action indicating setting alarm to disconnect wifi
     * @hide
     */
    public static final String ALARM_FOR_DISCONNECT_WIFI_ACTION = "sprd.wifi.alarm.DISCONNECT_WIFI";

    // wifi connect alarm flag, and its hour and minute flags.
    /** @hide */
    public static final String WIFI_CONNECT_ALARM_FLAG = "wifi_connect_alarm_flag";
    /** @hide */
    public static final String WIFI_CONNECT_ALARM_HOUR = "wifi_connect_alarm_hour";
    /** @hide */
    public static final String WIFI_CONNECT_ALARM_MINUTE = "wifi_connect_alarm_minute";

    // wifi disconnect alarm flag, and its hour and minute flags.
    /** @hide */
    public static final String WIFI_DISCONNECT_ALARM_FLAG = "wifi_disconnect_alarm_flag";
    /** @hide */
    public static final String WIFI_DISCONNECT_ALARM_HOUR = "wifi_disconnect_alarm_hour";
    /** @hide */
    public static final String WIFI_DISCONNECT_ALARM_MINUTE = "wifi_disconnect_alarm_minute";
    /** @hide */
    public static final int INTERVAL_MILLIS = 1000 * 60 * 60 * 24; //24h

    /** @hide */
    public static final String WIFI_AUTO_CONNECT = "wifi_auto_connect";

    private Context mContext;
    private String mWifiOperator;

    public WifiManagerEx(Context  context) {
        mContext = context;
        try{
            mWifiOperator = context.getResources().getString(com.android.internal.R.string.config_wifi_operator);
        } catch (Resources.NotFoundException e) {
            Log.e(TAG, "config_wifi_operator = null");
        }
    }

    /** @hide */
    public void setAutoConnect(boolean autoConnect) {
        if (isAutoConnect() != autoConnect) {
            Settings.Global.putInt(mContext.getContentResolver(), WIFI_AUTO_CONNECT, (autoConnect ? 1 : 0));
        }
    }

    /** @hide */
    public boolean isAutoConnect() {
        return Settings.Global.getInt(mContext.getContentResolver(), WIFI_AUTO_CONNECT, 1) == 1;
    }

    /** @hide */
    public boolean isSupportCMCC() {
        return "cmcc".equals(mWifiOperator);
    }

    /** @hide */
    public boolean isSupportCUCC() {
        return "cucc".equals(mWifiOperator);
    }

}
