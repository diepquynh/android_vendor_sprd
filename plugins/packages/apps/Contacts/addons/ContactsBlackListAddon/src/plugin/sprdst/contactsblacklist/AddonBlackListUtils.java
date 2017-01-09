
package plugin.sprdst.contactsblacklist;

import android.app.AddonManager;
import android.content.Context;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Map.Entry;
import android.graphics.Color;
import android.widget.TextView;
import android.widget.EditText;
import android.text.TextUtils;
import android.view.Menu;
import android.util.Log;
import android.app.Activity;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.app.Fragment;
import android.content.Context;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.widget.Toast;
import java.util.ArrayList;
import android.content.ContentValues;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.content.ContentResolver;
import android.database.Cursor;
import android.provider.BaseColumns;
import android.telephony.PhoneNumberUtils;
import android.net.Uri;
import com.android.contacts.common.model.Contact;
import com.sprd.contacts.plugin.BlacklistUtils;
import android.os.UserManager;
import android.content.pm.PackageManager;

public class AddonBlackListUtils extends BlacklistUtils implements AddonManager.InitialCallback {
    private static final String LOGTAG = "plugin_AddonBlackListUtils";
    private Context mAddonContext;
    private Context mContext;
    ArrayList<ContentValues> cvs;
    String displayName = "";
    boolean mOptionsMenuFilterable;
    public AddonBlackListUtils() {
    }
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
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
    public boolean isContactFilterable(ArrayList<ContentValues> mcvs, ArrayList<String> mPhones) {
        mPhones.clear();
        /**
         * SPRD:Bug505779 while switch to guest, Contact details menu still could have item of
         * 'add contact to blacklist'
         *
         * @{
         */
        UserManager userManager = (UserManager)mAddonContext
                .getSystemService(Context.USER_SERVICE);
        if (!userManager.isSystemUser()) {
            return false;
        }
        /**
         * @}
         */
	/**
         * SPRD:Bug507642 add for new feature to judgement that the CallFireWall was installed or not
         * @{
         */
        if (!isCallFireWallInstalled()) {
            return false;
        }
        /**
         * @}
         */
        for (ContentValues cv : mcvs) {
            String mimeType = cv.getAsString(Data.MIMETYPE);
            if (mimeType != null && mimeType.equals(Phone.CONTENT_ITEM_TYPE)) {
                String phone = cv.getAsString(Phone.NUMBER);
                /**
                * SPRD:Bug 500476 add a phone number contain extension number into blacklist, but this
                * phone number would not be interceptted
                *
                * @{
                */
                if (phone == null || phone.contains(",")) {
                    continue;
                } else {
                    mPhones.add(phone);
                }
                /**
                * @}
                */
            }
        }
        return mPhones.size() >= 1 ? true : false;
    }
    /**
     * SPRD:Bug507642 add for new feature to judgement that the CallFireWall was installed or not
     *
     * @{
     */
    private boolean isCallFireWallInstalled() {
        boolean installed = false;
        try {
            mAddonContext.getPackageManager().getPackageUid("com.sprd.firewall", 0);
            installed = true;
        } catch (PackageManager.NameNotFoundException e) {
        }
        return installed;
    }
    /**
     * @}
     */
    @Override
    public void addBlackItem(Context context, Menu menu, Contact mContactData) {
        menu.add(mAddonContext.getString(R.string.menu_addToBlacklist));
        MenuItem blackmeneitem = menu.getItem((int) menu.size() - 1);
        ArrayList<String> mPhones = new ArrayList<String>();
        mContext = context;
        if(mContactData == null){
            return;
        }
        cvs = mContactData.getAllContentValues();
        displayName = mContactData.getDisplayName();
        mOptionsMenuFilterable = isContactFilterable(cvs,mPhones);
        final class BlackListMenuItemsClickListener implements MenuItem.OnMenuItemClickListener {
          private Context lcontext;
          private ArrayList<String> lphones;
          private Context lmcontext;
          public BlackListMenuItemsClickListener(Context mContext, ArrayList<String> phones) {
              lphones = phones;
              lmcontext = mContext;
          }
          @Override
          public boolean onMenuItemClick(MenuItem item) {
              if (isContactBlocked(cvs,mContext)) {
                    onNotFilterRequested(lmcontext,lphones);
                } else {
                    onFilterRequested(lmcontext,lphones);
                }
              return true;
          }
      }
      blackmeneitem.setOnMenuItemClickListener(new BlackListMenuItemsClickListener(mContext,mPhones));
    }
    @Override
    public void blackOrNot(Context context, Menu menu, Contact mContactData) {
        if(mContactData == null){
            return;
         }
//      filter-able? (has any phone number)
        MenuItem blackmeneitem = menu.getItem((int) menu.size() - 1);

        blackmeneitem.setVisible(mOptionsMenuFilterable);
        if (isContactBlocked(cvs,mContext)) {
            blackmeneitem.setTitle(mAddonContext.getString(R.string.menu_deleteFromBlacklist));
        } else {
            blackmeneitem.setTitle(mAddonContext.getString(R.string.menu_addToBlacklist));
        }
        blackmeneitem.setEnabled(true);

    }

    public boolean isContactBlocked(ArrayList<ContentValues> mcvs, Context mContext) {
      for (ContentValues cv : mcvs) {
          String mimeType = cv.getAsString(Data.MIMETYPE);
          if (mimeType != null && mimeType.equals(Phone.CONTENT_ITEM_TYPE)) {
              String phone = cv.getAsString(Phone.NUMBER);
              if (!CheckIsBlackNumber(mContext, phone)) {
                  return false;
              }
          }

      }
      return true;
  }
    public boolean CheckIsBlackNumber(Context context, String str) {
        ContentResolver cr = context.getContentResolver();
        String mumber_value;
        int block_type;
        String[] columns = new String[] {
                BlackColumns.BlackMumber.MUMBER_VALUE,
                BlackColumns.BlackMumber.BLOCK_TYPE
        };

        Cursor cursor = cr.query(BlackColumns.BlackMumber.CONTENT_URI, columns, null, null, null);
        try {
            if (cursor != null && cursor.moveToFirst()) {
                do {
                    mumber_value = cursor.getString(cursor.getColumnIndex(
                            BlackColumns.BlackMumber.MUMBER_VALUE));
                    block_type = cursor.getInt(cursor.getColumnIndex(
                            BlackColumns.BlackMumber.BLOCK_TYPE));
                    if (PhoneNumberUtils.compare(str.trim(), mumber_value.trim())) {
                        return true;
                    }
                } while (cursor.moveToNext());
            }
        } catch (Exception e) {
            // process exception
        } finally {
            if (cursor != null)
                cursor.close();
            else
                Log.i(LOGTAG, "cursor == null");
        }
        return false;
    }
    public void onNotFilterRequested(Context mContext,ArrayList<String> phones) {
      // remove all phones to blacklist
      for (int i = 0; i < phones.size(); i++) {
          String phone = phones.get(i);
          if (CheckIsBlackNumber(mContext, phone)) {
              if (!deleteFromBlockList(mContext, phone)) {
                  Toast.makeText(mContext,mAddonContext.getString(R.string.failed_removeFromBlacklist),
                          Toast.LENGTH_SHORT).show();
                  return;
              }
          }
      }
      Toast.makeText(mContext,mAddonContext.getString(R.string.success_removeFromBlacklist),
              Toast.LENGTH_SHORT).show();
   }
    public void onFilterRequested(Context mContext,final ArrayList<String> phones) {
        final int BLOCK_ALL = 7;   //block mms,phone,vt
        try {
            for (int i = 0; i < phones.size(); i++) {
                String phone = phones.get(i);
                if (!CheckIsBlackNumber(mContext, phone)) {
                    if (!putToBlockList(mContext, phone, BLOCK_ALL,
                            displayName)) {
                        Toast.makeText(mContext,mAddonContext.getString(R.string.failed_addToBlacklist),
                                Toast.LENGTH_SHORT).show();
                        return;
                    }
                }
            }
            Toast.makeText(mContext,mAddonContext.getString(R.string.success_addToBlacklist),
                    Toast.LENGTH_SHORT).show();
        } catch (Exception e) {
            e.printStackTrace();
            Toast.makeText(mContext,mAddonContext.getString(R.string.failed_addToBlacklist),
                    Toast.LENGTH_SHORT).show();
        }
    }
    public boolean deleteFromBlockList(Context context, String phoneNumber) {
        ContentResolver cr = context.getContentResolver();
        String[] columns = new String[] {
                BlackColumns.BlackMumber._ID, BlackColumns.BlackMumber.MUMBER_VALUE,
        };
        String mumber_value;
        int result = -1;
        Cursor cursor = cr.query(BlackColumns.BlackMumber.CONTENT_URI, columns, null, null, null);
        try {
            if (cursor != null && cursor.moveToFirst()) {
                do {
                    mumber_value = cursor.getString(cursor.getColumnIndex(
                            BlackColumns.BlackMumber.MUMBER_VALUE));

                    if (PhoneNumberUtils.compare(phoneNumber.trim(), mumber_value.trim())) {

                        result = cr.delete(BlackColumns.BlackMumber.CONTENT_URI,
                                BlackColumns.BlackMumber.MUMBER_VALUE + "='" + mumber_value + "'",
                                null);
                        break;
                    }
                } while (cursor.moveToNext());
            }
        } catch (Exception e) {
            // process exception
            Log.e(LOGTAG, e.getMessage(), e);
        } finally {
            if (cursor != null) {
                cursor.close();
            } else {
                Log.i(LOGTAG, "cursor == null");
            }
        }
        if (result < 0) {
            return false;
        }
        return true;
    }
    public boolean putToBlockList(Context context, String phoneNumber, int Blocktype,
            String name) {
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
                Log.e(LOGTAG, "putToBlockList:exception");
            }
        }
        Uri result = null;
        try {
            result = cr.insert(BlackColumns.BlackMumber.CONTENT_URI, values);
        } catch (Exception e) {
            Log.e(LOGTAG, "putToBlockList: provider == null");
        }
        return result != null ? true : false;
    }
}
