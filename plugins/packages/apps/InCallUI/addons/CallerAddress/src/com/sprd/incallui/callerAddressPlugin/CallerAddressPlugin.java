package com.sprd.incallui.callerAddressPlugin;

import java.util.HashMap;

import android.app.AddonManager;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;

import android.telephony.PhoneNumberUtils;
import android.text.TextUtils;
import android.util.Log;

import com.android.contacts.common.GeoUtil;
import com.android.incallui.R;
import com.sprd.incallui.CallerAddressHelper;


public class CallerAddressPlugin extends CallerAddressHelper implements AddonManager.InitialCallback {

    private static final String TAG = "CallerAddressPlugin";
    private static final boolean DBG = true;
    private Context mContext;
    private static final int CMCC = R.string.china_mobile;
    private static final int CUCC = R.string.china_unicom;
    private static final int CTCC = R.string.china_telecom;
    private static final int FIXED = R.string.fixed;
    private static final String CMCC_OPERATOR_NUMBER = "10086";
    private static final String CUCC_OPERATOR_NUMBER = "10010";
    private static final String CUCC_RECHARGE_NUMBER = "10011";

    // 134(1340~1348),135~139,147,150~152,157~159,182,183,187,188
    private static final String[] CMCC_MAPPING = new String[]{
            "134", "135", "136", "137", "138", "139",
            "147",
            "150", "151", "152", "157", "158", "159",
            "182", "183", "187", "188"
    };

    // 130~132,145,155,156,185,186
    private static final String[] CUCC_MAPPING = new String[]{
            "130", "131", "132",
            "145",
            "155", "156",
            "185", "186"
    };

    // 133,153,180,181,189
    private static final String[] CTCC_MAPPING = new String[]{
            "133",
            "153",
            "180", "181", "189"
    };

    private static final String FIXED_LINE = "0";

    private static HashMap<String, Integer> OPERATOR_MAPPING;

    static {
        OPERATOR_MAPPING = new HashMap<String, Integer>();
        for (String colum : CMCC_MAPPING) {
            OPERATOR_MAPPING.put(colum, CMCC);
        }
        for (String colum : CUCC_MAPPING) {
            OPERATOR_MAPPING.put(colum, CUCC);
        }
        for (String colum : CTCC_MAPPING) {
            OPERATOR_MAPPING.put(colum, CTCC);
        }
        OPERATOR_MAPPING.put(FIXED_LINE, FIXED);
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    public void CallerAddressPlugin() {

    }

    @Override
    public boolean isSupportCallerAddress() {
        return true;
    }

    @Override
    public String getCallerAddress(Context context, String number) {
        String newDescription = "";
        final String geocode = GeoUtil.getGeocodedLocationFor(context, number);
        final String operatorName = getOperatorName(context, number);
        if (geocode != null) {
            newDescription = geocode + operatorName;
        }

        if (!TextUtils.isEmpty(number)
                && TextUtils.isEmpty(newDescription)
                && TextUtils.isDigitsOnly(number)
                && !PhoneNumberUtils.isEmergencyNumber(number)
                && !isOperatorNumber(number)) {
            try {
                newDescription = context.getString(R.string.unknown_area);
            } catch (NotFoundException e) {
                Log.d(TAG, "Can not found unknown_area");
                e.printStackTrace();
            }

        }
        return newDescription;
    }

    // Ip dialer segment
    private static final String[] NUMBER_SEGMENT = new String[]{
            "17951", "12593", "17911", "10193", "17909", "+86", "0086"
    };

    private static String clearIpOrCountryCode(String number) {
        for (String segment : NUMBER_SEGMENT) {
            if (number.startsWith(segment)) {
                return number.substring(segment.length(), number.length());
            }
        }
        return number;
    }

    public static String getOperatorName(Context context, String number) {
        if (TextUtils.isEmpty(number)) {
            return "";
        }
        number = PhoneNumberUtils.extractNetworkPortion(number);
        number = clearIpOrCountryCode(number);
        if (number.length() >= 11 && number.startsWith("1")) {
            String segment = number.substring(0, 3);
            Integer nameRes = OPERATOR_MAPPING.get(segment);
            if (nameRes != null) {
                try {
                    String name = context.getString(nameRes);
                    return name;
                } catch (NotFoundException e) {
                    Log.d(TAG, "Can not found nameRes : " + nameRes.intValue());
                    e.printStackTrace();
                }
            }
        }
        // for fixed number
        else if (number.length() >= 10 && number.startsWith("0")) {
            String segment = number.substring(0, 1);
            Integer nameRes = OPERATOR_MAPPING.get(segment);
            if (nameRes != null) {
                try {
                    String name = context.getString(nameRes);
                    return name;
                } catch (NotFoundException e) {
                    Log.d(TAG, "Can not found nameRes : " + nameRes.intValue());
                    e.printStackTrace();
                }
            }
        }
        return "";
    }

    public static boolean isOperatorNumber(String number) {
        if ((number.length() >= 5 && (number.startsWith(CMCC_OPERATOR_NUMBER)
                || number.startsWith(CUCC_OPERATOR_NUMBER)
                || number.equals(CUCC_RECHARGE_NUMBER)))) {
            return true;
        }
        return false;
    }

}
