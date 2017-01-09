package com.sprd.voicetrigger.provider;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.util.Log;

import com.sprd.voicetrigger.SensibilityActivity;

public class ContentProviderHelper {

    private static final String TAG = "ContentProviderHelper";

    public static void setSensibilityValue(Context context, int value) {
        try {
            ContentValues mContentValues = new ContentValues();
            mContentValues.put(MyProviderMetaData.ConfigTableMetaData.SENSIBILITY, value);
            context.getContentResolver().update(MyProviderMetaData.ConfigTableMetaData.CONTENT_URI, mContentValues, null, null);
            Log.d(TAG, "setSensibilityValue: setSensibilityValue = " + value);
        } catch (Exception e) {
            Log.e(TAG, "setSensibilityValue: setSensibilityValue error ", e);
        }
    }

    public static int getSensibilityValue(Context context) {
        Cursor cursor = context.getContentResolver().query(MyProviderMetaData.ConfigTableMetaData.CONTENT_URI,
                new String[]{MyProviderMetaData.ConfigTableMetaData.SENSIBILITY,
                        MyProviderMetaData.ConfigTableMetaData.ISOPENSWITCH,
                        MyProviderMetaData.ConfigTableMetaData.ISDEFAULTMODE,
                }, null, null, null);
        if (cursor.moveToFirst()) {
            int index = cursor.getColumnIndex(MyProviderMetaData.ConfigTableMetaData.SENSIBILITY);
            int sensibilityValue = cursor.getInt(index);
            cursor.close();
            if (sensibilityValue < 0) {
                sensibilityValue = 0;
            } else if (sensibilityValue > 100) {
                sensibilityValue = 100;
            }
            Log.d(TAG, "getSensibilityValue: getSensibilityValue = " + sensibilityValue);
            return sensibilityValue;
        } else {
            Log.e(TAG, "query sensibility value error, use default value!");
            cursor.close();
            return SensibilityActivity.SENSIBILITY_DEFAULT_VALUE;
        }
    }

    public static void setIsOpenSwitchStatus(Context context, boolean isChecked) {
        try {
            ContentValues mContentValues = new ContentValues();
            mContentValues.put(MyProviderMetaData.ConfigTableMetaData.ISOPENSWITCH, isChecked);
            context.getContentResolver().update(MyProviderMetaData.ConfigTableMetaData.CONTENT_URI, mContentValues, null, null);
            Log.d(TAG, "setIsOpenSwitchStatus: setIsOpenSwitchStatus = " + isChecked);
        } catch (Exception e) {
            Log.e(TAG, "setIsOpenSwitchStatus: setIsOpenSwitchStatus error", e);
        }
    }

    public static boolean isOpenSwitch(Context context) {
        Cursor cursor = context.getContentResolver().query(MyProviderMetaData.ConfigTableMetaData.CONTENT_URI,
                new String[]{MyProviderMetaData.ConfigTableMetaData.SENSIBILITY,
                        MyProviderMetaData.ConfigTableMetaData.ISOPENSWITCH,
                        MyProviderMetaData.ConfigTableMetaData.ISDEFAULTMODE
                }, null, null, null);
        if (cursor.moveToFirst()) {
            int index = cursor.getColumnIndex(MyProviderMetaData.ConfigTableMetaData.ISOPENSWITCH);
            boolean isOpenSwitch = cursor.getInt(index) == 0 ? false : true;
            cursor.close();
            Log.d(TAG, "isOpenSwitch: isOpenSwitch = " + isOpenSwitch);
            return isOpenSwitch;
        } else {
            Log.e(TAG, "query isOpenSwitch value error, use default value!");
            cursor.close();
            return false;
        }
    }

    public static void setMode(Context context, boolean isDefaultMode) {
        try {
            ContentValues mContentValues = new ContentValues();
            mContentValues.put(MyProviderMetaData.ConfigTableMetaData.ISDEFAULTMODE, isDefaultMode);
            context.getContentResolver().update(MyProviderMetaData.ConfigTableMetaData.CONTENT_URI, mContentValues, null, null);
            Log.d(TAG, "setWakeUpWordMode: setWakeUpWordMode = " + isDefaultMode);
        } catch (Exception e) {
            Log.e(TAG, "setWakeUpWordMode: setWakeUpWordMode error", e);
        }
    }

    public static boolean isDefaultMode(Context context) {
        Cursor cursor = context.getContentResolver().query(MyProviderMetaData.ConfigTableMetaData.CONTENT_URI,
                new String[]{MyProviderMetaData.ConfigTableMetaData.SENSIBILITY,
                        MyProviderMetaData.ConfigTableMetaData.ISOPENSWITCH,
                        MyProviderMetaData.ConfigTableMetaData.ISDEFAULTMODE,
                }, null, null, null);
        if (cursor.moveToFirst()) {
            int index = cursor.getColumnIndex(MyProviderMetaData.ConfigTableMetaData.ISDEFAULTMODE);
            boolean isDefaultMode = cursor.getInt(index) == 0 ? false : true;
            cursor.close();
            Log.d(TAG, "isDefaultMode: isDefaultMode = " + isDefaultMode);
            return isDefaultMode;
        } else {
            Log.e(TAG, "query isDefaultMode value error, use default value = true!");
            cursor.close();
            return true;
        }
    }
}
