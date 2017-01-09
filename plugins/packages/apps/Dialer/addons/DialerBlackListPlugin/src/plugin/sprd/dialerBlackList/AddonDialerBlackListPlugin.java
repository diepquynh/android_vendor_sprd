package plugin.sprd.dialerBlackList;

import android.app.AddonManager;
import android.content.Context;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.BaseColumns;
import android.util.Log;
import android.text.TextUtils;
import android.telephony.PhoneNumberUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.dialer.DialerBlackListUtil;
import com.sprd.dialer.plugins.R;

/**
 * Various utilities for dealing with phone number strings.
 */
public class AddonDialerBlackListPlugin extends DialerBlackListUtil implements AddonManager.InitialCallback
{
    private static final int BLOCK_ALL = 3;
    private static final String TAG = "[AddonDialerBlackListPlugin]";
    private static final boolean DBG = true;
    private static final int DELETE_FROM_BLACKLIST = 0;
    private static final int ADD_TO_BLACKLIST = 1;
    private static final int REFRESH_BLACKLIST_TITLE = 2;

    private Context mAddonContext;
    private View mAddToBlacklistButtonView;
    private TextView mBlacklistButtonTv;
    private String mDisplayName;
    private Thread mThread;
    private Thread mRefreshTitleThread;

    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == DELETE_FROM_BLACKLIST) {
                boolean isDeleteSuccessful = msg.arg1 == 1;
                Toast.makeText(mAddonContext, mAddonContext.getResources()
                        .getString(isDeleteSuccessful ? R.string.success_removeFromBlacklist
                                : R.string.failed_removeFromBlacklist),
                        Toast.LENGTH_SHORT).show();
                mBlacklistButtonTv.setText(
                        mAddonContext.getResources().getString(
                                isDeleteSuccessful
                                ? R.string.call_log_add_to_blacklist
                                : R.string.call_log_delete_from_blacklist));
                mAddToBlacklistButtonView.setEnabled(true);
            } else if (msg.what == ADD_TO_BLACKLIST) {
                boolean isAddSuccessful = msg.arg1 == 1;
                Toast.makeText(mAddonContext, mAddonContext.getResources()
                        .getString(isAddSuccessful ? R.string.success_addToBlacklist
                                : R.string.failed_addToBlacklist),
                        Toast.LENGTH_SHORT).show();
                mBlacklistButtonTv.setText(
                        mAddonContext.getResources().getString(
                                isAddSuccessful
                                ? R.string.call_log_delete_from_blacklist
                                : R.string.call_log_add_to_blacklist));
                mAddToBlacklistButtonView.setEnabled(true);
            } else if (msg.what == REFRESH_BLACKLIST_TITLE) {
                boolean isBlackListnumber = msg.arg1 == 1;
                mBlacklistButtonTv.setText(
                        mAddonContext.getResources().getString(
                                isBlackListnumber
                                ? R.string.call_log_delete_from_blacklist
                                : R.string.call_log_add_to_blacklist));
            }
        }
    };

    public AddonDialerBlackListPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        log("clazz: " + clazz);
        mAddonContext = context;
        return clazz;
    }

    public static class BlackColumns {
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
        public static final class BlockRecorder implements BaseColumns {
            public static final Uri CONTENT_URI = Uri
                    .parse("content://com.sprd.providers.block/block_recorded");

            public static final String MUMBER_VALUE = "mumber_value";
            public static final String BLOCK_DATE = "block_date";
            public static final String NAME = "name";
        }
    }

    @Override
    public void addBlackItem(final Context context, View view,
            final String number, CharSequence displayName) {
        mAddToBlacklistButtonView = view;
        mDisplayName = displayName + "";
        mBlacklistButtonTv =  (TextView) mAddToBlacklistButtonView
                .findViewById(com.android.dialer.R.id.add_to_blacklist_title);
        mAddToBlacklistButtonView.setOnClickListener(new  OnClickListener() {
            @Override
            public void onClick(View v) {
                mAddToBlacklistButtonView.setEnabled(false);
                addOrDeleteBlackList(context, number);
            }
        });
        refreshBlackListTitle(context, number);
    }

    private void refreshBlackListTitle(final Context context,
            final String number) {
        if (mRefreshTitleThread != null) {
            mRefreshTitleThread.interrupt();
            mRefreshTitleThread = null;
        }
        mRefreshTitleThread = new Thread() {
            @Override
            public void run() {
                ContentResolver cr = context.getContentResolver();
                boolean isBlackListNumber = false;
                String mumber_value;
                int block_type;
                String[] columns = new String[] {
                        BlackColumns.BlackMumber.MUMBER_VALUE,
                        BlackColumns.BlackMumber.BLOCK_TYPE
                };
                Cursor cursor = cr.query(
                        BlackColumns.BlackMumber.CONTENT_URI,
                        columns,
                        BlackColumns.BlackMumber.MUMBER_VALUE + "='" + number.trim() + "'",
                        null,
                        null);
                try {
                    if (cursor != null && cursor.moveToFirst()) {
                        isBlackListNumber = true;
                    }
                } catch (Exception e) {
                    // process exception
                } finally {
                    if (cursor != null) {
                        cursor.close();
                    }else {
                        log("cursor == null");
                    }
                }

                Message msg = mHandler.obtainMessage(REFRESH_BLACKLIST_TITLE,
                        isBlackListNumber ? 1 : 0, 0);
                msg.sendToTarget();
            }
        };
        mRefreshTitleThread.start();
    }

    private void addOrDeleteBlackList(final Context context,
            final String number) {
        if (mThread != null) {
            mThread.interrupt();
            mThread = null;
        }
        mThread = new Thread() {
            @Override
            public void run() {
                if (checkIsBlackNumber(context, number)) {
                    deleteFromBlockList(context, number);
                } else {
                    putToBlockList(
                            context,
                            number,
                            BLOCK_ALL,
                            TextUtils.isEmpty(mDisplayName) ? number : mDisplayName);
                }
            }
        };
        mThread.start();
    }

    public boolean checkIsBlackNumber(Context context, String str) {
        ContentResolver cr = context.getContentResolver();
        String mumber_value;
        int block_type;
        String[] columns = new String[] {
                BlackColumns.BlackMumber.MUMBER_VALUE,
                BlackColumns.BlackMumber.BLOCK_TYPE
        };

        Cursor cursor = cr.query(
                BlackColumns.BlackMumber.CONTENT_URI,
                columns,
                BlackColumns.BlackMumber.MUMBER_VALUE + "='" + str.trim() + "'",
                null,
                null);
        try {
            if (cursor != null && cursor.moveToFirst()) {
                return true;
            }
        } catch (Exception e) {
            // process exception
        } finally {
            if (cursor != null) {
                cursor.close();
            }else {
                log("cursor == null");
            }
        }
        return false;
    }

    public void deleteFromBlockList(final Context context, final String phoneNumber) {
        ContentResolver cr = context.getContentResolver();
        String[] columns = new String[] {
                BlackColumns.BlackMumber._ID, BlackColumns.BlackMumber.MUMBER_VALUE
        };
        String mumber_value;
        int result = -1;
        try {
            result = cr.delete(BlackColumns.BlackMumber.CONTENT_URI,
                    BlackColumns.BlackMumber.MUMBER_VALUE
                            + "='" + phoneNumber.trim() + "'",
                    null);
        } catch (Exception e) {
            // process exception
            log(e.getMessage());
        }
        Message msg = mHandler.obtainMessage(DELETE_FROM_BLACKLIST,
                result < 0 ? 0 : 1, 0);
        msg.sendToTarget();
    }

    public void putToBlockList(final Context context, final String phoneNumber,
            final int Blocktype, final String name) {
        ContentResolver cr = context.getContentResolver();
        String normalizeNumber = PhoneNumberUtils.normalizeNumber(phoneNumber);
        ContentValues values = new ContentValues();
        if (values != null) {
            try {
                values.put(BlackColumns.BlackMumber.MUMBER_VALUE, phoneNumber);
                values.put(BlackColumns.BlackMumber.BLOCK_TYPE, Blocktype);
                values.put(BlackColumns.BlackMumber.NAME, name);
                values.put(BlackColumns.BlackMumber.MIN_MATCH,
                        PhoneNumberUtils.toCallerIDMinMatch(normalizeNumber));
            } catch (Exception e) {
                log("putToBlockList:exception");
            }
        }
        Uri result = null;
        try {
            result = cr.insert(BlackColumns.BlackMumber.CONTENT_URI, values);
        } catch (Exception e) {
            log("putToBlockList: provider == null");
        }
        Message msg = mHandler.obtainMessage(ADD_TO_BLACKLIST,
                result == null  ? 0 : 1, 0);
        msg.sendToTarget();
    }

    private void log(String msg) {
        if (DBG) Log.d(TAG, msg);
    }
}
