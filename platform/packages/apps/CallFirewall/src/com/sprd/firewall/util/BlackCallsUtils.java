
package com.sprd.firewall.util;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Vector;

import com.sprd.firewall.db.BlackColumns;
import com.sprd.firewall.model.BlackNumberEntity;

import android.net.Uri;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.provider.BlockedNumberContract;
import android.provider.BlockedNumberContract.BlockedNumbers;
import android.telephony.PhoneNumberUtils;
import android.util.Log;

public class BlackCallsUtils {
    private final Context context;

    protected static final String TAG = "BlackCallsUtils";

    public static Vector<BlackNumberEntity> BlackNumberVector = new Vector<BlackNumberEntity>();

    public static final String BLOCK_TYPE = "block_type";

    public static final String NAME = "name";

    public static final String MIN_MATCH = "min_match";

    public BlackCallsUtils(Context context) {
        this.context = context;
    }

    public void AddBlackCalls(String BlackCallsNumber, int Type, String name) {
        ContentResolver cr = context.getContentResolver();
        ContentValues values = new ContentValues();
        String normalizedNumber = PhoneNumberUtils.normalizeNumber(BlackCallsNumber);
        values.put(BlockedNumberContract.BlockedNumbers.COLUMN_ORIGINAL_NUMBER, BlackCallsNumber);
        values.put(MIN_MATCH,
                PhoneNumberUtils.toCallerIDMinMatch(normalizedNumber));
        values.put(BLOCK_TYPE, Integer.valueOf(Type));
        values.put(NAME, name);
        cr.insert(BlockedNumberContract.BlockedNumbers.CONTENT_URI, values);
    }

    public boolean DelBlackCalls(String BlackCallsNumber) {
        ContentResolver cr = context.getContentResolver();
        return cr.delete(BlockedNumberContract.BlockedNumbers.CONTENT_URI,
                BlockedNumberContract.BlockedNumbers.COLUMN_ORIGINAL_NUMBER
                + "='" + BlackCallsNumber + "'", null) > 0;
    }

    public boolean DelNewBlackLogsFromId(Integer LogsId) {
        ContentResolver cr = context.getContentResolver();
        return cr.delete(BlackColumns.BlockRecorder.CONTENT_URI,
                BlackColumns.BlockRecorder._ID
                + "='" + LogsId + "'", null) > 0;
    }

    public boolean UpdateBlackNumber(String BlackCallsNumber, String NewBlackNumber, int Type,
            String BlackCallsName) {
        ContentResolver cr = context.getContentResolver();
        int result = -1;
        /* SPRD: add for CTS bug608407 @{ */
        int deleteResult = -1;
        Uri insertUri = null;
        /* @} */
        ContentValues values = new ContentValues();
        String normalizedNumber = PhoneNumberUtils.normalizeNumber(NewBlackNumber);
        /* SPRD: add for bug606644 @{ */
        Locale locale = context.getResources().getConfiguration().locale;
        String countryIso = locale.getCountry();
        String e164Number = PhoneNumberUtils.formatNumberToE164(normalizedNumber,countryIso);
        values.put(BlockedNumberContract.BlockedNumbers.COLUMN_E164_NUMBER, e164Number);
        /* @} */
        values.put(BlockedNumberContract.BlockedNumbers.COLUMN_ORIGINAL_NUMBER, NewBlackNumber);
        values.put(BLOCK_TYPE, Integer.valueOf(Type));
        values.put(MIN_MATCH, PhoneNumberUtils.toCallerIDMinMatch(normalizedNumber));
        values.put(NAME, BlackCallsName);
        String number_value = null;
        String[] columns = new String[] {
                BlockedNumberContract.BlockedNumbers.COLUMN_ID,
                BlockedNumberContract.BlockedNumbers.COLUMN_ORIGINAL_NUMBER,
                BLOCK_TYPE, NAME, MIN_MATCH};
        Cursor cursor = cr.query(BlockedNumberContract.BlockedNumbers.CONTENT_URI,
                columns, null, null, null);
        try {
            if (cursor != null && cursor.moveToFirst()) {
                do {
                    number_value = cursor.getString(
                        cursor.getColumnIndex(BlockedNumberContract.
                                BlockedNumbers.COLUMN_ORIGINAL_NUMBER));
                    if (number_value!= null
                            && PhoneNumberUtils.compareStrictly(BlackCallsNumber.trim(),
                                    number_value.trim())) {
                        /* SPRD: add for CTS bug608407 @{ */
                        deleteResult = cr.delete(BlockedNumberContract.BlockedNumbers.CONTENT_URI,
                                BlockedNumberContract.BlockedNumbers.COLUMN_ORIGINAL_NUMBER
                                + "='" + number_value + "'", null);
                        insertUri = cr.insert(BlockedNumberContract.BlockedNumbers.CONTENT_URI,
                                values);
                        if (insertUri != null) {
                            result = deleteResult;
                        }
                        /* @} */
                        break;
                    }
                } while (cursor.moveToNext());
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            } else {
                Log.v(TAG, "cursor == null");
            }
        }

        if (result <0 ) {
            return false;
        }
        return true;
    }

    public boolean DelSmsLogsFromId(Integer SmsId) {
        ContentResolver cr = context.getContentResolver();
        return cr.delete(BlackColumns.SmsBlockRecorder.CONTENT_URI,
                BlackColumns.SmsBlockRecorder._ID + "='" + SmsId + "'", null) > 0;
    }

    public String FindPhoneNamebyNumber(String phoneNumber) {
        ContentResolver cr = context.getContentResolver();
        Cursor cursor;
        String phoneName = null;
        String min_match = null;
        String[] columns = new String[] {NAME, MIN_MATCH};
        cursor = cr.query(BlockedNumberContract.BlockedNumbers.CONTENT_URI,
                columns, null, null, null);
        try {
            if (cursor != null && cursor.moveToFirst()) {
                do {
                    min_match = cursor.getString(cursor.getColumnIndex(MIN_MATCH));
                    String incoming = PhoneNumberUtils.toCallerIDMinMatch(phoneNumber);
                    if (PhoneNumberUtils.compareStrictly(incoming, min_match)) {
                        phoneName = cursor.getString(cursor.getColumnIndex(NAME));
                        break;
                    }
                } while (cursor.moveToNext());
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            } else {
                Log.v(TAG, "cursor == null");
            }
        }
        return phoneName;
    }

    public List<BlackNumberEntity> selectBlacklistByNumber(String phoneNumber) {
        ContentResolver cr = context.getContentResolver();
        List<BlackNumberEntity> result = new ArrayList<BlackNumberEntity>();
        String number_value = null;
        String[] columns = new String[] {
                BlockedNumberContract.BlockedNumbers.COLUMN_ID,
                BlockedNumberContract.BlockedNumbers.COLUMN_ORIGINAL_NUMBER,
                BLOCK_TYPE, NAME
        };
        Cursor cursor = cr.query(
                BlockedNumberContract.BlockedNumbers.CONTENT_URI,
                columns, null, null, null);
        try {
            if (cursor != null && cursor.moveToFirst()) {
                do {
                    number_value = cursor.getString(
                            cursor.getColumnIndex(
                            BlockedNumberContract.BlockedNumbers.COLUMN_ORIGINAL_NUMBER));
                    if (number_value != null
                            && PhoneNumberUtils.compareStrictly(phoneNumber.trim(),
                                    number_value.trim())) {
                        BlackNumberEntity BlackCalls = new BlackNumberEntity();
                        BlackCalls.setId(
                                Integer.valueOf(cursor.getInt(cursor.getColumnIndex(
                                        BlockedNumberContract.BlockedNumbers.COLUMN_ID))));
                        BlackCalls.setNumber(
                                cursor.getString(cursor.getColumnIndex(
                                BlockedNumberContract.BlockedNumbers.COLUMN_ORIGINAL_NUMBER)));
                        BlackCalls.setType(
                                Integer.valueOf(cursor.getString(cursor.getColumnIndex(BLOCK_TYPE))));
                        BlackCalls.setName(
                                cursor.getString(cursor.getColumnIndex(NAME)));
                        result.add(BlackCalls);
                    }
                } while (cursor.moveToNext());
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            } else {
                Log.v(TAG, "cursor = null");
            }
        }
        return result;
    }
}
