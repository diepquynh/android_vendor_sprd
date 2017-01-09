package plugin.sprd.telephony;

import com.android.internal.telephony.plugin.BlockInboundSmsHandlerUtils;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map.Entry;
import com.android.internal.telephony.SmsStorageMonitor;
import com.android.internal.telephony.PhoneBase;

import android.net.Uri;
import android.os.AsyncResult;

import android.telephony.PhoneNumberUtils;
import android.util.Log;

import java.util.Date;

import android.provider.BaseColumns;

import com.android.internal.telephony.CellBroadcastHandler;
import com.android.internal.telephony.SmsMessageBase;
import com.android.internal.util.StateMachine;

import android.content.Intent;
import android.provider.Telephony;
import android.provider.Telephony.Sms.Intents;
import android.telephony.SmsMessage;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Color;
import android.widget.TextView;
import android.widget.EditText;
import android.text.TextUtils;
import android.view.Menu;
import android.util.Log;
import android.app.Activity;
import android.app.AddonManager;

public class AddonBlockInboundSmsHandlerUtils extends BlockInboundSmsHandlerUtils implements
AddonManager.InitialCallback {
    private static final String LOGTAG = "AddonBlockInboundSmsHandlerUtils";
    private Context  mAddonContext;
    private SmsStorageMonitor storageMonitor;
    private PhoneBase phone;
    public static final int SMS_SHIFT = 0;
    public static final int SMS_SELECT = 1 << SMS_SHIFT;

    public AddonBlockInboundSmsHandlerUtils(){
        Log.d(LOGTAG, "AddonBlockInboundSmsHandlerUtils enter");
    }

    public static boolean checkIsBlackNumber(Context context, String number) {
        ContentResolver cr = context.getContentResolver();
        int block_type;
        String min_match;
        String incoming = PhoneNumberUtils.toCallerIDMinMatch(number);
        String[] columns = new String[] {
                BlackMumber.MIN_MATCH, BlackMumber.BLOCK_TYPE
        };
        Cursor cursor = cr.query(BlackMumber.CONTENT_URI, columns, null, null, null);

        try{
            if (cursor!=null && cursor.moveToFirst()) {
                do {
                    min_match = cursor.getString(cursor
                            .getColumnIndex(BlackMumber.MIN_MATCH));
                    block_type = cursor.getInt(cursor
                            .getColumnIndex(BlackMumber.BLOCK_TYPE));
                    Log.d(LOGTAG,"BlackList intercept ");
                    if (PhoneNumberUtils.compareStrictly(incoming, min_match)) {
                        if ((SMS_SELECT & block_type) == SMS_SELECT) {
                            return true;
                        }
                    }
                } while (cursor.moveToNext());
            }
        } finally{
            if(cursor!=null){
                cursor.close();
            } else {
                Log.d(LOGTAG,"BlackList intercept :cursor == null");
            }
        }

        return false;
    }


    public static Uri putToSmsBlackList(Context context, String phoneNumber, String message,
            long date) {

        ContentResolver cr = context.getContentResolver();
        ContentValues values = new ContentValues();
        values.put(SmsBlockRecorder.MUMBER_VALUE, phoneNumber);
        values.put(SmsBlockRecorder.BLOCK_SMS_CONTENT, message);
        values.put(SmsBlockRecorder.BLOCK_DATE, date);

        return cr.insert(SmsBlockRecorder.CONTENT_URI, values);
    }
    public static final String AUTHORITY = "com.sprd.providers.block";
    public static final class BlackMumber implements BaseColumns {
        public static final Uri CONTENT_URI = Uri
                .parse("content://com.sprd.providers.block/black_mumbers");

        public static final String MUMBER_VALUE = "mumber_value";

        public static final String BLOCK_TYPE = "block_type";

        public static final String NOTES = "notes";

        public static final String NAME = "name";

        public static final String MIN_MATCH = "min_match";
    }

    public static final class SmsBlockRecorder implements BaseColumns {
        public static final Uri CONTENT_URI = Uri
                .parse("content://com.sprd.providers.block/sms_block_recorded");

        public static final String MUMBER_VALUE = "mumber_value"; // block number

        public static final String BLOCK_SMS_CONTENT = "sms_content"; // block message content

        public static final String BLOCK_DATE = "block_date"; // block time,long type

        public static final String NAME = "name";
    }


    public Class onCreateAddon(Context context, Class clazz){
        mAddonContext = context;
        return clazz;
    }


    public boolean plugin_sms(String address,Intent intent) {
        if (checkIsBlackNumber(mAddonContext, address)) {
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
            putToSmsBlackList(mAddonContext, address, body, (new Date()).getTime());
            return true;
        }
        return false;
    }
}
