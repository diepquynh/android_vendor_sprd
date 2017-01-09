package com.sprd.firewall.db;

import android.app.IntentService;
import android.content.Context;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.os.Build;

import com.sprd.firewall.util.BlackCallsUtils;

/**
 * Service for updating M's block data to N.
 */
public class BlockDBUpdateService extends IntentService {

    private static final String PREF_BLOCK_DB_UPDATE = "block_db_update";
    private static final String TAG_IS_ALREADY_UPDATE = "is_already_update";

    public BlockDBUpdateService() {
        super(BlockDBUpdateService.class.getSimpleName());
        setIntentRedelivery(true);
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        // Currently this service only handles one type of update.
        ContentResolver cr = this.getContentResolver();

        String[] columns = new String[] {
            BlackColumns.BlackMumber.MUMBER_VALUE,
            BlackColumns.BlackMumber.BLOCK_TYPE,
            BlackColumns.BlackMumber.NAME
        };

        Cursor cursor = null;
        try {
            cursor = cr.query(BlackColumns.BlackMumber.CONTENT_URI, columns, null, null, null);
            BlackCallsUtils blackcallsUtils = new BlackCallsUtils(this);
            if (cursor != null && cursor.moveToFirst()) {
                do {
                    String  phone_num = cursor.getString(
                            cursor.getColumnIndex(BlackColumns.BlackMumber.MUMBER_VALUE));
                    int type = cursor.getInt(
                            cursor.getColumnIndex(BlackColumns.BlackMumber.BLOCK_TYPE));
                    String name = cursor.getString(
                            cursor.getColumnIndex(BlackColumns.BlackMumber.NAME));
                    blackcallsUtils.AddBlackCalls(phone_num, type, name);
                } while (cursor.moveToNext());
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
            }
            SharedPreferences prefs = this.getSharedPreferences(
                    PREF_BLOCK_DB_UPDATE, Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = prefs.edit();
            editor.putBoolean(TAG_IS_ALREADY_UPDATE, true);
            editor.commit();
        }
    }
}
