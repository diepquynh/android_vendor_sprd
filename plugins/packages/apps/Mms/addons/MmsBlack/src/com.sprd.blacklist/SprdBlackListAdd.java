package com.sprd.blacklist;

import android.app.AddonManager;
import android.content.ActivityNotFoundException;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.telephony.PhoneNumberUtils;
import android.util.Log;
import android.view.Menu;
import android.widget.Toast;

import com.android.mms.R;
import com.android.mms.data.Contact;
import com.android.mms.ui.MessageUtils;
import com.sprd.mms.ui.SprdBlackListStub;


public class SprdBlackListAdd extends SprdBlackListStub{
    private static final String TAG = "SprdBlackListAdd";
    private static final int MENU_ADD_TO_BLACKLIST      = 42;
    private Context mContext;
    @Override
    public void addBlackMenu(Menu menu, Context context) {
        menu.add(0, MENU_ADD_TO_BLACKLIST, 0, R.string.add_to_blacklist);
        mContext = context;
    }
    @Override
    public void onBlackListClick(Contact contact) {
        if (contact != null) {
          int blockType = getBlockType(mContext, contact.getNumber());
          Log.i(TAG, "blockType = " + blockType);
          try {
              Intent blacklistIntent = new Intent(
                      "com.sprd.firewall.ui.BlackCallsListAddActivity.action");
              blacklistIntent
                      .putExtra("Click_BlackCalls_Number", contact.getNumber());
              blacklistIntent.putExtra("Click_BlackCalls_Name", contact.getName());
              blacklistIntent.putExtra("Click_BlackCalls_Type", blockType);
              mContext.startActivity(blacklistIntent);
          } catch (ActivityNotFoundException e) {
              Toast.makeText(this, R.string.failed_addToBlacklist, Toast.LENGTH_SHORT)
                      .show();
          }
      }
    }
    public static int getBlockType(Context context, String str) {
        ContentResolver cr = context.getContentResolver();
        String mumber_value;
        int block_type;
        String[] columns = new String[] {
                "mumber_value", "block_type"
        };
        Cursor cursor = cr.query(Uri.parse("content://com.sprd.providers.block/black_mumbers"),
                columns, null, null, null);
        try {
            if (cursor != null && cursor.moveToFirst()) {
                do {
                    mumber_value = cursor.getString(cursor.getColumnIndex("mumber_value"));
                    if (PhoneNumberUtils.compare(str.trim(), mumber_value.trim())) {
                        block_type = cursor.getInt(cursor.getColumnIndex("block_type"));
                        return block_type;
                    }
                } while (cursor.moveToNext());
            }
        } catch (Exception e) {
            Log.e(TAG, "getBlockType:exception");
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return 0;
    }
}
