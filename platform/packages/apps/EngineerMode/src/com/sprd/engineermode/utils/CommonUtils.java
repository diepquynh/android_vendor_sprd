package com.sprd.engineermode.utils;

import android.os.SystemProperties;
import android.util.Log;
import android.text.TextUtils;
import android.content.Context;
import android.content.SharedPreferences;
import com.sprd.engineermode.EMApplication;

public class CommonUtils {
    static final String TAG = "CommonUtils";

    public static boolean isModemATAvaliable() {
        String modemAssert = SystemProperties.get("ril.modem.assert", "0");
        if(TextUtils.isEmpty(modemAssert) || modemAssert.equals("0")) {
            Log.d(TAG, "check modem at ok");
            return true;
        }
        SharedPreferences sp = EMApplication.getContext().getSharedPreferences("com.sprd.engineermode_preferences", Context.MODE_PRIVATE);
        if(modemAssert.contains("1") && sp.getBoolean("key_manualassert", true)) {
            Log.d(TAG, "checked modem at ok");
            return true;
        }
        Log.d(TAG, "check modem at fail");
        return false;
    }
}
