/** Create by Spreadst */

package com.spreadst.s2lockscreen;

import android.content.Context;
import android.database.Cursor;
import android.provider.BaseColumns;
import android.provider.CallLog.Calls;
import android.provider.Telephony.Mms;
import android.provider.Telephony.Sms;
import android.util.Log;

/**
 * query missed call and miss messages
 */
public class TelephoneInfoManager {
    private final static String TAG = "TelephoneInfoManager";

    // missed calls
    public static int getMissedCalls(Context context) {
        Cursor cur = null;
        try {
            if (context != null) {
                cur = context.getContentResolver().query(Calls.CONTENT_URI,
                        new String[] {
                                Calls.NUMBER, Calls.TYPE, Calls.NEW
                        },
                        "type=" + Calls.MISSED_TYPE + " AND new=1", null,
                        Calls.DEFAULT_SORT_ORDER);
                if (cur != null) {
                    Log.d(TAG, "missed call: " + cur.getCount());
                    return cur.getCount();
                }
            }
        } catch (Exception e) {
            Log.d(TAG, e.getMessage());
        } finally {
            if (cur != null && !cur.isClosed()) {
                cur.close();
            }
        }
        return 0;
    }

    public static int getUnReadMmsCount(Context context) {
        String selection = "(read=0 AND m_type != 134)";
        Cursor cur = null;
        try {
            if (context != null) {

                cur = context.getContentResolver().query(Mms.Inbox.CONTENT_URI,
                        new String[] {
                                BaseColumns._ID, Mms.DATE
                        }, selection,
                        null, "date desc");// limit
                if (cur != null) {
                    return cur.getCount();
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "SQLiteException in getUnReadMmsCount" + e.getMessage());
        } finally {
            if (cur != null && !cur.isClosed()) {
                cur.close();
            }
        }
        return 0;
    }

    public static int getUnReadSmsCount(Context context) {
        String selection = "(read=0 OR seen=0)";
        Cursor cur = null;
        try {
            if (context != null) {
                cur = context.getContentResolver().query(
                        Sms.Inbox.CONTENT_URI,
                        new String[] {
                                BaseColumns._ID, Sms.ADDRESS,
                                Sms.PERSON, Sms.BODY, Sms.DATE
                        }, selection,
                        null, "date desc");// limit
                if (cur != null) {
                    return cur.getCount();
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "SQLiteException in getUnReadSmsCount" + e.getMessage());
        } finally {
            if (cur != null && !cur.isClosed()) {
                cur.close();
            }
        }
        return 0;
    }

    // return MMS + SMS
    public static int getUnReadMessageCount(Context context) {
        return getUnReadMmsCount(context) + getUnReadSmsCount(context);
    }

}
