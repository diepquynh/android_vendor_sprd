package com.android.callsettings.callforward;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

public class CallForwardTimeUtils {

    private static final String CFT_SHARED_PREFERENCES_NAME = "ctf_prefs_name";
    public static final String CFT_STARTTIME_KEY = "cft_starttime_";
    public static final String CFT_STOPTIME_KEY = "cft_stoptime_";
    private static final String CFT_NUMBERS_KEY = "cft_numbers_";
    private static final String CFT_STATUS_ACTIVE_KEY = "cft_status_active_";
    private static final String CFT_REAL_NUMBERS_KEY = "cft_real_numbers_";
    private static SharedPreferences mCFTPrefs;

    public static long readTime(Context context, boolean isStartTime, String iccId) {
        mCFTPrefs = context.getSharedPreferences(CFT_SHARED_PREFERENCES_NAME, Context.MODE_PRIVATE);
        if (isStartTime) {
            return mCFTPrefs.getLong(CFT_STARTTIME_KEY + iccId, 0);
        } else {
            return mCFTPrefs.getLong(CFT_STOPTIME_KEY + iccId, 0);
        }
    }

    public static void writeTime(Context context, boolean isStartTime, String iccId, long when) {
        mCFTPrefs = context.getSharedPreferences(CFT_SHARED_PREFERENCES_NAME, Context.MODE_PRIVATE);
        Editor editor = mCFTPrefs.edit();
        if (isStartTime) {
            editor.putLong(CFT_STARTTIME_KEY + iccId, when);
        } else {
            editor.putLong(CFT_STOPTIME_KEY + iccId, when);
        }
        editor.apply();
    }

    public static String readNumber(Context context, String iccId) {
        mCFTPrefs = context.getSharedPreferences(CFT_SHARED_PREFERENCES_NAME, Context.MODE_PRIVATE);
        return mCFTPrefs.getString(CFT_NUMBERS_KEY + iccId, "");
    }

    public static void writeNumber(Context context, String iccId, String number) {
        mCFTPrefs = context.getSharedPreferences(CFT_SHARED_PREFERENCES_NAME, Context.MODE_PRIVATE);
        Editor editor = mCFTPrefs.edit();
        editor.putString(CFT_NUMBERS_KEY + iccId, number);
        editor.apply();
    }

    public static int readStatus(Context context, String iccId) {
        mCFTPrefs = context.getSharedPreferences(CFT_SHARED_PREFERENCES_NAME, Context.MODE_PRIVATE);
        return mCFTPrefs.getInt(CFT_STATUS_ACTIVE_KEY + iccId, 0);
    }

    public static void writeStatus(Context context, String iccId, int status) {
        mCFTPrefs = context.getSharedPreferences(CFT_SHARED_PREFERENCES_NAME, Context.MODE_PRIVATE);
        Editor editor = mCFTPrefs.edit();
        editor.putInt(CFT_STATUS_ACTIVE_KEY + iccId, status);
        editor.apply();
    }

    public static String readCFTNumber(Context context, String iccId) {
        mCFTPrefs = context.getSharedPreferences(CFT_SHARED_PREFERENCES_NAME, Context.MODE_PRIVATE);
        return mCFTPrefs.getString(CFT_REAL_NUMBERS_KEY + iccId, "");
    }

    public static void writeCFTNumber(Context context, String iccId, String number) {
        mCFTPrefs = context.getSharedPreferences(CFT_SHARED_PREFERENCES_NAME, Context.MODE_PRIVATE);
        Editor editor = mCFTPrefs.edit();
        editor.putString(CFT_REAL_NUMBERS_KEY + iccId, number);
        editor.apply();
    }

}
