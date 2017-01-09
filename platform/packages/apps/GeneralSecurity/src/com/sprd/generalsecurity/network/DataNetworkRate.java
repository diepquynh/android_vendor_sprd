
package com.sprd.generalsecurity.network;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.TrafficStats;
import android.telephony.TelephonyManager;
import android.util.Log;
import com.sprd.generalsecurity.R;

import java.lang.Throwable;

//network spread rate
public class DataNetworkRate {

    private static final String TAG = "DataNetworkRate";

    private final static int NETWORK_U = -1;
    // 4G
    private final static int NETWORK_W = 0;
    // 3G
    private final static int NETWORK_E = 1;
    // 2G
    private final static int NETWORK_G = 2;

    private final static String DEFAULT_RATE = "0 bps";
    private final Context mContext;

    public class NetworkTrafficTheoryPeak {
        public String upLinkTheoryPeak;
        public String downLinkTheoryPeak;
    }

    public class NetworkTrafficCurrentRate {
        public long upLinkRate;
        public long downLinkRate;
    }

    public DataNetworkRate(Context context) {
        this.mContext = context;
    }

    NetworkInfo getNetworkInfo(Context context) {
        NetworkInfo networkInfo = null;
        if (context != null) {
            ConnectivityManager connectivityManager = (ConnectivityManager) context
                    .getSystemService(Context.CONNECTIVITY_SERVICE);

            networkInfo = connectivityManager.getActiveNetworkInfo();
        }
        return networkInfo;
    }

    private boolean isNetworkConnected(NetworkInfo info) {

        return info != null && info.isAvailable();
    }

    boolean isMobileConnected(NetworkInfo info) {
        if (info != null) {
            Log.d(TAG, "info.getType() = " + info.getType());
        }
        return info != null && info.getType() == ConnectivityManager.TYPE_MOBILE;
    }

    boolean isWifiConnected(NetworkInfo info) {
        return info != null && info.getType() == ConnectivityManager.TYPE_WIFI;
    }

//    public NetworkTrafficTheoryPeak getCurrentNetworkTrafficTheoryPeak(Context context,
//                                                                       boolean isMobile) {
//        NetworkTrafficTheoryPeak networkTrafficTheoryPeak = null;
//
//        NetworkInfo networkInfo = getNetworkInfo(context);
//        Log.d(TAG,"networkInfo = "+networkInfo);
//        Log.d(TAG,"isMobile = "+isMobile);
//        if (context != null && isNetworkConnected(networkInfo)) {
//            // mobile connected
//            if (isMobile) {
//                if (isMobileConnected(networkInfo)) {
//
//                    networkTrafficTheoryPeak = getMobileTrafficTheoryPeak(getMobileNetworkType(networkInfo
//                            .getSubtype()));
//                }
//            }
//            // wifi
//            else {
//                if (isWifiConnected(networkInfo)) {
//                    networkTrafficTheoryPeak = getWifiTrafficTheoryPeak();
//                }
//            }
//        }
//        Log.d(TAG,"networkTrafficTheoryPeak = "+networkTrafficTheoryPeak);
//        return networkTrafficTheoryPeak;
//    }

    public NetworkTrafficCurrentRate getCurrentNetworkTrafficRate(Context context, boolean isMobile) {
        NetworkTrafficCurrentRate networkTrafficCurrentRate = null;
        NetworkInfo networkInfo = getNetworkInfo(context);
        if (context != null && isNetworkConnected(networkInfo)) {
            // mobile connected
            if (isMobile) {
                if (isMobileConnected(networkInfo)) {
                    networkTrafficCurrentRate = getMobileTrafficCurrentRate();
                }
            }
            // wifi
            else {
                if (isWifiConnected(networkInfo)) {
                    networkTrafficCurrentRate = getWifiTrafficCurrentRate();
                }
            }
        }
        return networkTrafficCurrentRate;
    }

    public NetworkTrafficCurrentRate getCurrentNetworkTrafficTotal(boolean isMobile) {
        if (isMobile) {
            return getMobileTrafficCurrentRate();
        } else {
            return getWifiTrafficCurrentRate();
        }
    }

    private long getMobileTrafficRxBytes() {
        return TrafficStats.getMobileRxBytes();
    }

    private long getMobileTrafficTxBytes() {
        return TrafficStats.getMobileTxBytes();
    }

    private long getWifiTrafficRxBytes() {
        return Math.max(0, TrafficStats.getTotalRxBytes() - TrafficStats.getMobileRxBytes());
    }

    private long getWifiTrafficTxBytes() {
        return Math.max(0, TrafficStats.getTotalTxBytes() - TrafficStats.getMobileTxBytes());
    }

    private NetworkTrafficCurrentRate getMobileTrafficCurrentRate() {
        NetworkTrafficCurrentRate currentRate = new NetworkTrafficCurrentRate();
        currentRate.upLinkRate = getMobileTrafficTxBytes();
        currentRate.downLinkRate = getMobileTrafficRxBytes();
        return currentRate;
    }

    private NetworkTrafficCurrentRate getWifiTrafficCurrentRate() {
        NetworkTrafficCurrentRate currentRate = new NetworkTrafficCurrentRate();
        currentRate.upLinkRate = getWifiTrafficTxBytes();
        currentRate.downLinkRate = getWifiTrafficRxBytes();
        return currentRate;
    }

//    private NetworkTrafficTheoryPeak getWifiTrafficTheoryPeak() {
//        NetworkTrafficTheoryPeak networkTrafficTheoryPeak = new NetworkTrafficTheoryPeak();
//        networkTrafficTheoryPeak.upLinkTheoryPeak = (mContext != null ? mContext
//                .getString(R.string.network_wifi_peak_uplink) : DEFAULT_RATE);
//        networkTrafficTheoryPeak.downLinkTheoryPeak = (mContext != null ? mContext
//                .getString(R.string.network_wifi_peak_downlink) : DEFAULT_RATE);
//        return networkTrafficTheoryPeak;
//    }
//
//    private NetworkTrafficTheoryPeak getMobileTrafficTheoryPeak(int network_type) {
//        NetworkTrafficTheoryPeak networkTrafficTheoryPeak = null;
//        Log.d(TAG,"network_type= "+network_type);
//        switch (network_type) {
//            case NETWORK_G: {
//                networkTrafficTheoryPeak = new NetworkTrafficTheoryPeak();
//                networkTrafficTheoryPeak.upLinkTheoryPeak = (mContext != null ? mContext
//                        .getString(R.string.network_mobile_peak_g_uplink) : DEFAULT_RATE);
//                networkTrafficTheoryPeak.downLinkTheoryPeak = (mContext != null ? mContext
//                        .getString(R.string.network_mobile_peak_g_downlink) : DEFAULT_RATE);
//            }
//            break;
//            case NETWORK_E: {
//                networkTrafficTheoryPeak = new NetworkTrafficTheoryPeak();
//                networkTrafficTheoryPeak.upLinkTheoryPeak = (mContext != null ? mContext
//                        .getString(R.string.network_mobile_peak_e_uplink) : DEFAULT_RATE);
//                networkTrafficTheoryPeak.downLinkTheoryPeak = (mContext != null ? mContext
//                        .getString(R.string.network_mobile_peak_e_downlink) : DEFAULT_RATE);
//            }
//            break;
//            case NETWORK_W: {
//                networkTrafficTheoryPeak = new NetworkTrafficTheoryPeak();
//                networkTrafficTheoryPeak.upLinkTheoryPeak = (mContext != null ? mContext
//                        .getString(R.string.network_mobile_peak_w_uplink) : DEFAULT_RATE);
//                networkTrafficTheoryPeak.downLinkTheoryPeak = (mContext != null ? mContext
//                        .getString(R.string.network_mobile_peak_w_downlink) : DEFAULT_RATE);
//            }
//            break;
//        }
//
//        return networkTrafficTheoryPeak;
//    }

    private int getMobileNetworkType(int mobileSubType) {
        Log.d(TAG,"mobileSubType = "+mobileSubType);
        int mobileConnectionType = NETWORK_U;
        int type = mobileSubType;
        switch (type) {
            case TelephonyManager.NETWORK_TYPE_GPRS:
            case TelephonyManager.NETWORK_TYPE_GSM:
            case TelephonyManager.NETWORK_TYPE_EDGE:
            case TelephonyManager.NETWORK_TYPE_CDMA:
            case TelephonyManager.NETWORK_TYPE_1xRTT:
            case TelephonyManager.NETWORK_TYPE_IDEN:{
                mobileConnectionType = NETWORK_G;
            }
            break;
            case TelephonyManager.NETWORK_TYPE_UMTS:
            case TelephonyManager.NETWORK_TYPE_EVDO_0:
            case TelephonyManager.NETWORK_TYPE_EVDO_A:
            case TelephonyManager.NETWORK_TYPE_HSDPA:
            case TelephonyManager.NETWORK_TYPE_HSUPA:
            case TelephonyManager.NETWORK_TYPE_HSPA:
            case TelephonyManager.NETWORK_TYPE_EVDO_B:
            case TelephonyManager.NETWORK_TYPE_EHRPD:
            case TelephonyManager.NETWORK_TYPE_HSPAP:{
                mobileConnectionType = NETWORK_E;
            }
            break;
            case TelephonyManager.NETWORK_TYPE_LTE:
            case TelephonyManager.NETWORK_TYPE_IWLAN:{
                mobileConnectionType = NETWORK_W;
            }
            break;
        }
        return mobileConnectionType;
    }
}
