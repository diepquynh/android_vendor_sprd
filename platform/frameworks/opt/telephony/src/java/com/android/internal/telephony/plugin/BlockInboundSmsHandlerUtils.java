package com.android.internal.telephony.plugin;

import java.util.Date;

import android.app.Activity;

import com.android.internal.telephony.InboundSmsTracker;
import com.android.internal.telephony.InboundSmsHandler;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;

import com.android.internal.R;
import android.util.Log;
import android.net.Uri;
import android.provider.BaseColumns;
import android.provider.BlockedNumberContract;
import android.provider.BlockedNumberContract.BlockedNumbers;
import android.provider.Telephony.Sms.Intents;
import android.telephony.PhoneNumberUtils;
import android.telephony.Rlog;
import android.telephony.SmsMessage;
import android.text.TextUtils;

public class BlockInboundSmsHandlerUtils {

    private static final String LOGTAG = "BlockInboundSmsHandlerUtils";
    static BlockInboundSmsHandlerUtils sInstance;
    public static final int SMS_SHIFT = 0;
    public static final int SMS_SELECT = 1 << SMS_SHIFT;
    private static final int CALL_SHIFT = 1;
    private static final int CALL_SELECT = 1 << CALL_SHIFT;
    private Context mContext;

    public static BlockInboundSmsHandlerUtils getInstance(Context context) {
        if (sInstance != null) return sInstance;
        Log.d(LOGTAG, "BlockInboundSmsHandlerUtils getInstance");
        sInstance = new BlockInboundSmsHandlerUtils(context);

        return sInstance;

    }

    public static boolean checkIsBlockSMSNumber(Context context, String number) {
        Log.d(LOGTAG, "checkIsBlackNumber ");
        ContentResolver cr = context.getContentResolver();
        if (cr == null) {
            return false;
        }
        int block_type;
        String number_value;
        String[] columns = new String[] {BlockedNumbers.BLOCK_TYPE,
                BlockedNumbers.COLUMN_ORIGINAL_NUMBER};
        Cursor cursor = cr.query(BlockedNumberContract.BlockedNumbers.CONTENT_URI, columns,
                null, null, null);
        try {
            if (cursor != null && cursor.moveToFirst()) {
                do {
                    block_type = cursor.getInt(cursor.
                            getColumnIndex(BlockedNumbers.BLOCK_TYPE));
                    number_value = cursor.getString(cursor.
                            getColumnIndex(BlockedNumbers.COLUMN_ORIGINAL_NUMBER));
                    if (PhoneNumberUtils.compare(number, number_value)) {
                        if ((SMS_SELECT & block_type) == SMS_SELECT) {
                            return true;
                        }
                    }
                } while (cursor.moveToNext());
            } else {
                Log.e(LOGTAG, "Query black list cursor is null.");
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return false;
    }

    public static boolean checkIsBlockCallNumber(Context context, String number) {
        Log.d(LOGTAG, "checkIsBlackNumber ");
        ContentResolver cr = context.getContentResolver();
        if (cr == null) {
            return false;
        }
        int block_type;
        String number_value;
        String[] columns = new String[] {BlockedNumbers.BLOCK_TYPE,
                BlockedNumbers.COLUMN_ORIGINAL_NUMBER};
        Cursor cursor = cr.query(BlockedNumberContract.BlockedNumbers.CONTENT_URI, columns,
                null, null, null);
        try {
            if (cursor != null && cursor.moveToFirst()) {
                do {
                    block_type = cursor.getInt(cursor
                            .getColumnIndex(BlockedNumbers.BLOCK_TYPE));
                    number_value = cursor.getString(cursor.
                            getColumnIndex(BlockedNumbers.COLUMN_ORIGINAL_NUMBER));
                    if (PhoneNumberUtils.compare(number, number_value)) {
                        if ((CALL_SELECT & block_type) == CALL_SELECT) {
                            return true;
                        }
                    }
                } while (cursor.moveToNext());
            } else {
                Log.e(LOGTAG, "Query black list cursor is null.");
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return false;
    }

    public static Uri putToSmsBlackList(Context context, String phoneNumber, String message,
            long date) {

        ContentResolver cr = context.getContentResolver();
        ContentValues values = new ContentValues();
        values.put(SmsBlockRecorder.NUMBER_VALUE, phoneNumber);
        values.put(SmsBlockRecorder.BLOCK_SMS_CONTENT, message);
        values.put(SmsBlockRecorder.BLOCK_DATE, date);

        return cr.insert(SmsBlockRecorder.CONTENT_URI, values);
    }

    public static final class SmsBlockRecorder implements BaseColumns {
        public static final Uri CONTENT_URI = Uri
                .parse("content://com.sprd.providers.block/sms_block_recorded");

        public static final String NUMBER_VALUE = "number_value"; // block number

        public static final String BLOCK_SMS_CONTENT = "sms_content"; // block message content

        public static final String BLOCK_DATE = "block_date"; // block time,long type

        public static final String NAME = "name";
    }

    public BlockInboundSmsHandlerUtils(Context context) {
        mContext = context;
    }

    public void blockSms(String address,Intent intent) {
        if (checkIsBlockSMSNumber(mContext, address)) {
            SmsMessage[] messages = Intents.getMessagesFromIntent(intent);
            SmsMessage sms = messages[0];
            StringBuffer bodyBuffer = new StringBuffer();
            int count = messages.length;
            for (int i = 0; i < count; i++) {
                sms = messages[i];
                if (sms.mWrappedSmsMessage != null) {
                    bodyBuffer.append(sms.getDisplayMessageBody());
                }
            }
            String body = bodyBuffer.toString();
            putToSmsBlackList(mContext, address, body, (new Date()).getTime());
        }
    }

    public void blockMms(String address, String contentField) {
        if (checkIsBlockSMSNumber(mContext, address)
                && !isRedundantWapPushMessage(mContext, address, contentField)) {
            putToSmsBlackList(mContext,
                    address,
                    contentField,
                    (new Date()).getTime());
        }
    }

    public static boolean isRedundantWapPushMessage(
            Context context, String address, String contentField) {
        boolean retValue = false;
        ContentResolver cr = context.getContentResolver();
        if (cr != null) {
            //TODO: sms records should be distinguished from type sms and wap push.
            String[] columns = new String[] {
                SmsBlockRecorder.NUMBER_VALUE,
                SmsBlockRecorder.BLOCK_SMS_CONTENT
            };

            String number, field;
            Cursor cursor = cr.query(SmsBlockRecorder.CONTENT_URI, columns, null, null, null);
            try {
                if (cursor != null && cursor.moveToFirst()) {
                    do {
                        number = cursor.getString(
                                cursor.getColumnIndex(SmsBlockRecorder.NUMBER_VALUE));
                        if (PhoneNumberUtils.compare(address, number)) {
                            field = cursor.getString(
                                    cursor.getColumnIndex(SmsBlockRecorder.BLOCK_SMS_CONTENT));
                            if (TextUtils.equals(field, contentField)) {
                                Log.d(LOGTAG, "found redundant wap push record.");
                                retValue = true;
                                break;
                            }
                        }
                    } while (cursor.moveToNext());
                }
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }
        }

        return retValue;
    }
}
