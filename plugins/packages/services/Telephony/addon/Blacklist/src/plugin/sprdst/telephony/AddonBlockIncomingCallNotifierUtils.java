
package plugin.sprdst.telephony;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map.Entry;

import com.android.internal.telephony.PhoneConstants;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.bluetooth.IBluetoothHeadsetPhone;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.drawable.Drawable;
import android.location.CountryDetector;
import android.net.sip.SipManager;
import android.provider.BaseColumns;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.CommonDataKinds.SipAddress;
import android.text.TextUtils;

import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.Toast;

import com.android.internal.telephony.CallerInfo;
import com.android.internal.telephony.CallerInfoAsyncQuery;
import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.CellBroadcastHandler;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.MmiCode;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneBase;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.SmsStorageMonitor;
import com.android.internal.telephony.TelephonyCapabilities;
import com.android.internal.telephony.TelephonyProperties;
import com.android.internal.telephony.cdma.CdmaConnection;
import com.android.internal.telephony.sip.SipPhone;
import com.android.services.telephony.plugin.BlockIncomingCallNotifierUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.ConcurrentModificationException;
import java.util.Date;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.List;
import java.lang.Exception;
import com.android.phone.CallNotifier;
import android.media.AudioManager;
import android.media.RingtoneManager;
import android.media.ToneGenerator;
import android.os.Debug;
import android.os.Message;
import android.os.PowerManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.SystemVibrator;
import android.os.Vibrator;
import android.preference.PreferenceManager;
import android.provider.CallLog.Calls;
import android.provider.Settings;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.util.EventLog;
import android.media.MediaPlayer;
import android.os.RemoteException;
import android.net.Uri;
import android.os.AsyncResult;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDiskIOException;
import android.database.sqlite.SQLiteFullException;
import android.graphics.Color;
import android.widget.TextView;
import android.widget.EditText;
import android.telephony.PhoneNumberUtils;
import android.text.TextUtils;
import android.view.Menu;
import android.util.Log;
import android.os.Handler;
import android.provider.ContactsContract.PhoneLookup;
import android.app.Activity;
import android.app.AddonManager;
import android.os.Build;

public class AddonBlockIncomingCallNotifierUtils extends BlockIncomingCallNotifierUtils implements
AddonManager.InitialCallback {

    private static final String LOGTAG = "AddonBlockIncomingCallNotifierUtils";
    private Context mAddonContext;
    // SPRD: add for new feature for import call log to blacklist
    private boolean mIsCallFireWallInstalled;
    private static final int CALL_SHIFT = 1;
    private static final int VT_SHIFT = 2;
    private static final int CALL_SELECT = 1 << CALL_SHIFT;
    private static final int VT_SELECT = 1 << VT_SHIFT;
    // SPRD: add for bug493481
    public static final boolean DEBUG = "userdebug".equals(Build.TYPE) || "eng".equals(Build.TYPE);

    public AddonBlockIncomingCallNotifierUtils() {

    }

    public static boolean checkIsBlockNumber(Context context, String number) {
        Log.d(LOGTAG, "checkIsBlockNumber ");
        ContentResolver cr = context.getContentResolver();
        if (cr == null) {
            return false;
        }
        String icoming_value = PhoneNumberUtils.toCallerIDMinMatch(number);
        String min_match = new String();
        int block_type;
        String[] columns = new String[] {
                BlackColumns.BlackMumber.MIN_MATCH,
                BlackColumns.BlackMumber.BLOCK_TYPE
        };

        Cursor cursor = cr.query(BlackColumns.BlackMumber.CONTENT_URI, columns,
                null, null, null);
        try {
            if (cursor != null && cursor.moveToFirst()) {
                do {
                    min_match = cursor.getString(0);
                    block_type = cursor.getInt(1);

                    if (PhoneNumberUtils.compare(icoming_value, min_match)) {
                        if ((CALL_SELECT & block_type) == CALL_SELECT) {
                            return true;
                        }
                        if ((VT_SELECT & block_type) == VT_SELECT) {
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

    public static void putToBlockList(Context context, String number) {
        ContentResolver cr = context.getContentResolver();
        if (cr == null) {
            return;
        }

        long date = (new Date()).getTime();
        String name = "";
        Uri uri = Uri.withAppendedPath(PhoneLookup.CONTENT_FILTER_URI,
                Uri.encode(number));
        Cursor cursor = cr.query(uri, null, null, null, null);
        try {
            if (cursor != null && cursor.moveToFirst()) {
                int index = cursor.getColumnIndex(PhoneLookup.DISPLAY_NAME);
                if (index != -1) {
                    name = cursor.getString(index);
                }
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        try {
            ContentValues values = new ContentValues();
            if (values != null) {
                values.put(BlackColumns.BlockRecorder.MUMBER_VALUE, number);
                values.put(BlackColumns.BlockRecorder.BLOCK_DATE, date);
                values.put(BlackColumns.BlockRecorder.NAME, name);
                cr.insert(BlackColumns.BlockRecorder.CONTENT_URI, values);
            }
        } catch (SQLiteFullException e) {
            e.printStackTrace();
        } catch (SQLiteDiskIOException e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static class BlackColumns {
        public static final String AUTHORITY = "com.sprd.providers.block";

        public static final class BlackMumber implements BaseColumns {
            public static final Uri CONTENT_URI = Uri
                    .parse("content://com.sprd.providers.block/black_mumbers");

            public static final String MUMBER_VALUE = "mumber_value";
            public static final String BLOCK_TYPE = "block_type"; // 0 block call, 1 block message,2
            // block call and message
            public static final String NOTES = "notes";
            public static final String NAME = "name";
            public static final String MIN_MATCH = "min_match";
        }

        public static final class BlockRecorder implements BaseColumns {
            public static final Uri CONTENT_URI = Uri
                    .parse("content://com.sprd.providers.block/block_recorded");

            public static final String MUMBER_VALUE = "mumber_value";
            public static final String CALL_TYPE = "call_type";
            public static final String BLOCK_DATE = "block_date";
            public static final String NAME = "name";
        }
    }

    private String mBlockNumber;

    public void setBlockNumber(String number) {
        mBlockNumber = number;
    }

    public String getBlockNumber() {
        return mBlockNumber;
    }

    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        // SPRD: add for new feature for import call log to blacklist
        mIsCallFireWallInstalled = isCallFireWallInstalled();
        return clazz;
    }

    /* SPRD: add for new feature to judgement that the CallFireWall was installed or not @{ */
    private boolean isCallFireWallInstalled() {
        boolean installed = false;
        try {
            mAddonContext.getPackageManager().getPackageInfo(
                    "com.sprd.firewall", PackageManager.GET_ACTIVITIES);
            installed = true;
        } catch (PackageManager.NameNotFoundException e) {
        }
        return installed;
    }
    /* @} */

    public void plugin_phone(Connection connection, Call call) {
        final String inComingNumber = connection.getAddress();
        // SPRD: add for new feature for import call log to blacklist
        if (mIsCallFireWallInstalled
                && !TextUtils.isEmpty(inComingNumber)) {
            // SPRD: add for bug493481
            if (DEBUG) {
                Log.d(LOGTAG, "inComingNumber = " + inComingNumber);
            }
            try {
                if (checkIsBlockNumber(mAddonContext, inComingNumber)) {
                    setBlockNumber(inComingNumber);
                    if(call != null && call.getState().isRinging()){
                        call.hangup();
                    }
                    new Thread(new Runnable() {
                        public void run() {
                            putToBlockList(mAddonContext, inComingNumber);
                        }
                    }).start();
                    return;
                } else {
                    setBlockNumber("");
                }
            } catch (Exception e) {
                Log.e(LOGTAG, "can't block for checking Call");
                e.printStackTrace();

            }

        }
    }
}
