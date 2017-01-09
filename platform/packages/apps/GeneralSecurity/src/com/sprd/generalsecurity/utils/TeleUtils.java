package com.sprd.generalsecurity.utils;

import android.content.Context;
import android.telephony.TelephonyManager;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionInfo;

import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Log;

import android.os.SystemProperties;

import java.text.DecimalFormat;

public class TeleUtils {
    public static int getSimCount(Context context) {
        SubscriptionManager subscriptionManager = SubscriptionManager.from(context);
        return subscriptionManager.getActiveSubscriptionInfoCount();
    }

    public static String getActiveSubscriberId(Context context, int slotIndex) {
        SubscriptionManager subscriptionManager = SubscriptionManager.from(context);
        int simCount = getSimCount(context);
        int subscriberid = -1;
        if (simCount == 2) {
            SubscriptionInfo subscriptionInfo;
            for (int i = 0; i <=1; i++) {
                subscriptionInfo = subscriptionManager.getActiveSubscriptionInfoForSimSlotIndex(i);
                if (subscriptionInfo.getSimSlotIndex() == (slotIndex - 1)) {
                    subscriberid = subscriptionInfo.getSubscriptionId();
                    break;
                }
            }
        } else if (simCount == 1){
            return getActiveSubscriberId(context);
        }

        TelephonyManager tele = TelephonyManager.from(context);
        String retVal = tele.getSubscriberId(subscriberid);

        return retVal;
    }

    public static int getPrimaryCard(Context context) {
        final TelephonyManager tele = TelephonyManager.from(context);
        return tele.getDefaultSim();
    }

    public static int getSimSlotCount(Context context) {
        final TelephonyManager tele = TelephonyManager.from(context);
        return tele.getPhoneCount();
    }

    public static String getSimNumber(Context context, int slot) {
        TelephonyManager telephonyManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        return telephonyManager.getLine1Number(slot);
//        SubscriptionManager mSubscriptionManager = SubscriptionManager.from(context);
//        final SubscriptionInfo sir = mSubscriptionManager
////                .getActiveSubscriptionInfoForSimSlotIndex(slot);
//        if (sir != null) {
//            return telephonyManager.getLine1NumberForSubscriber(sir.getSubscriptionId());
//        } else {
//            return null;
//        }

    }

    public enum NetworkType {
        SIM1, SIM2, WIFI, DISCONNECTED
    }

    public static NetworkType getCurrentNetworkType(Context context) {
        //Check network state
        ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo ns = cm.getActiveNetworkInfo();
        boolean isConnected = ns != null && ns.isConnectedOrConnecting();
        if (!isConnected) {
            return NetworkType.DISCONNECTED;
        }

        if (ns.getType() == ConnectivityManager.TYPE_MOBILE) {
            if (getPrimarySlot(context) == 1) {
                return NetworkType.SIM1;
            } else {
                return NetworkType.SIM2;
            }
        } else {
            return NetworkType.WIFI;
        }
    }

    static SubscriptionManager mSubscriptionManager;

    public static String getActiveSubscriberId(Context context) {
        final TelephonyManager tele = TelephonyManager.from(context);
        final String actualSubscriberId = tele.getSubscriberId();
        String TEST_SUBSCRIBER_PROP = "test.subscriberid";
        String retVal = SystemProperties.get(TEST_SUBSCRIBER_PROP, actualSubscriberId);

        return retVal;
    }

    public static int getPrimarySlot(Context context) {
        mSubscriptionManager = SubscriptionManager.from(context);
        final SubscriptionInfo sir = mSubscriptionManager.getDefaultDataSubscriptionInfo();
        if (sir != null) {
            return sir.getSimSlotIndex() + 1;
        }
        return -1;
    }
}