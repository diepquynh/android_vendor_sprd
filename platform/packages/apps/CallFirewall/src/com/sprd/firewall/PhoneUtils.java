
package com.sprd.firewall;

import java.lang.reflect.Method;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.BlockedNumberContract;
import android.telephony.PhoneNumberUtils;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.internal.telephony.ITelephony;
import com.sprd.firewall.db.BlackColumns;

public class PhoneUtils {

    private static final int SMS_SHIFT = 0;
    private static final int CALL_SHIFT = 1;
    private static final int VT_SHIFT = 2;

    private static final int SMS_SELECT = 1 << SMS_SHIFT;
    private static final int CALL_SELECT = 1 << CALL_SHIFT;
    private static final int VT_SELECT = 1 << VT_SHIFT;
    private static final String MIN_MATCH = "min_match";
    public static final String BLOCK_TYPE = "block_type";

    private static final String TAG = "PhoneUtils";

    static public ITelephony getITelephony(TelephonyManager telMgr) throws Exception {
        Method getITelephonyMethod = telMgr.getClass().getDeclaredMethod("getITelephony");
        getITelephonyMethod.setAccessible(true);
        return (ITelephony) getITelephonyMethod.invoke(telMgr);
    }

    public static boolean CheckIsBlockNumber(Context context, String incoming,
            Boolean isBlockMessage, Boolean isVideoCall) {
        ContentResolver cr = context.getContentResolver();
        Log.v(TAG, "CheckIsBlockNumber,isVideoCall=" + isVideoCall);
        String min_match = null;
        String incoming_value = PhoneNumberUtils.toCallerIDMinMatch(incoming);
        int block_type;

        String[] columns = new String[] {
                MIN_MATCH, BLOCK_TYPE
        };

        Cursor cursor = cr.query(BlockedNumberContract.BlockedNumbers.CONTENT_URI, columns, null, null, null);
        try {
            if (cursor.moveToFirst()) {
                do {
                    min_match = cursor.getString(cursor
                            .getColumnIndex(MIN_MATCH));
                    block_type = cursor.getInt(cursor
                            .getColumnIndex(BLOCK_TYPE));
                    if (PhoneNumberUtils.compareStrictly(incoming_value, min_match)) {
                        if ((SMS_SELECT & block_type) == SMS_SELECT) {
                            if (isBlockMessage) {
                                return true;
                            }
                        }
                        if ((CALL_SELECT & block_type) == CALL_SELECT) {
                            if ((!isBlockMessage) && (!isVideoCall)) {
                                return true;
                            }
                        }
                        if ((VT_SELECT & block_type) == VT_SELECT) {
                            if (isVideoCall) {
                                return true;
                            }
                        }
                    }
                } while (cursor.moveToNext());
            }
        } finally {
            if (cursor != null)
                cursor.close();
            else
                Log.v(TAG, "cursor == null");
        }
        return false;
    }

    public static Uri putToBlockList(Context context, String phoneNumber, long date) {
        ContentResolver cr = context.getContentResolver();

        ContentValues values = new ContentValues();
        values.put(BlackColumns.BlockRecorder.NUMBER_VALUE, phoneNumber);
        values.put(BlackColumns.BlockRecorder.BLOCK_DATE, Long.valueOf(date));

        return cr.insert(BlackColumns.BlockRecorder.CONTENT_URI, values);
    }

    public static Uri putToSmsBlockList(Context context, String phoneNumber, String message,
            long date) {
        ContentResolver cr = context.getContentResolver();

        ContentValues values = new ContentValues();
        values.put(BlackColumns.SmsBlockRecorder.NUMBER_VALUE, phoneNumber);
        values.put(BlackColumns.SmsBlockRecorder.BLOCK_SMS_CONTENT, message);
        values.put(BlackColumns.SmsBlockRecorder.BLOCK_DATE, Long.valueOf(date));

        return cr.insert(BlackColumns.SmsBlockRecorder.CONTENT_URI, values);
    }

    public static boolean isPhoneNumber(String text) {
        char[] sChar = text.toCharArray();
        for (char c : sChar) {
            if (checkCharacter(c)) {
                continue;
            } else {
                return false;
            }
        }
        return true;
    }

    public static String filterNumbers(String originalText) {
        StringBuffer buffer = new StringBuffer(originalText);
        for (int i = 0; i < buffer.length(); i++) {
            char c = buffer.charAt(i);
            if (checkCharacter(c)) {
                continue;
            } else {
                buffer.deleteCharAt(i);
            }
        }
        return buffer.toString();
    }

    private static boolean checkCharacter(char c) {
        return ((c >= '0' && c <= '9') || c == ',' || c == ';' || c == '*' || c == '#' || c == '+'
                || c == '-' || c == '(' || c == ')' || c == ',' || c == '/' || c == 'N'
                || c == '.' || c == ' ' || c == ';');
    }
}
