package plugin.sprd.contactsdefaultcontact;

import android.app.IntentService;
import android.content.ContentProviderOperation;
import android.content.ContentValues;
import android.content.Intent;
import android.content.OperationApplicationException;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.RemoteException;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.Photo;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.RawContacts;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;

import javax.security.auth.PrivateCredentialPermission;
import android.text.TextUtils;
import plugin.sprd.contactsdefaultcontact.R;
import android.os.AsyncTask;

public class PreDefineContactsService extends IntentService {
    private static final String TAG = "DefaultContactAddon";

    final static String PHONE_ACCOUNT_TYPE = "sprd.com.android.account.phone";

    public PreDefineContactsService() {
        super(TAG);
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        (new LoaderTask()).execute();
    }
    private class LoaderTask extends AsyncTask<Void, Void, Boolean> {

        protected Boolean doInBackground(Void... v) {
            try{
                String INSERT_PRESET_NUMBER[] = getResources().getStringArray(R.array.items_preset_number);
                int INSERT_PRESET_NUMBER_COUNT = INSERT_PRESET_NUMBER.length;
                if (INSERT_PRESET_NUMBER_COUNT == 0) {
                    Log.d(TAG, "number count 0 return");
                    return true;
                }
                String INSERT_PRESET_NAME[] = getResources().getStringArray(R.array.items_preset_name);
                String INSERT_PRESET_PHOTO[] = getResources().getStringArray(R.array.items_preset_photo);
                String INSERT_PRESET_EMAIL[] = getResources().getStringArray(R.array.items_preset_email);
                for (int i = 0; i < INSERT_PRESET_NUMBER_COUNT; i++) {
                    int photoId = getResources().getIdentifier(INSERT_PRESET_PHOTO[i], "drawable",
                            "plugin.sprd.contactsdefaultcontact");
                    Log.d(TAG, "photoId: " + photoId);
                    final ArrayList<ContentProviderOperation> operationList = new ArrayList<ContentProviderOperation>();
                    ContentProviderOperation.Builder builder;
                    builder = ContentProviderOperation.newInsert(RawContacts.CONTENT_URI);
                    builder.withValue(RawContacts.RAW_CONTACT_IS_READ_ONLY, 1);
                    builder.withValue(RawContacts.AGGREGATION_MODE, RawContacts.AGGREGATION_MODE_DISABLED);
                    operationList.add(builder.build());

                    // preset number
                    if (!TextUtils.isEmpty(INSERT_PRESET_NUMBER[i])) {
                        builder = ContentProviderOperation.newInsert(Data.CONTENT_URI);
                        builder.withValueBackReference(Phone.RAW_CONTACT_ID, 0);
                        builder.withValue(Data.MIMETYPE, Phone.CONTENT_ITEM_TYPE);
                        builder.withValue(Phone.TYPE, Phone.TYPE_MOBILE);
                        builder.withValue(Phone.NUMBER, INSERT_PRESET_NUMBER[i]);
                        builder.withValue(Data.IS_PRIMARY, 1);
                        builder.withValue(Data.IS_READ_ONLY, 1);
                        operationList.add(builder.build());
                    }

                    // preset name
                    if (!TextUtils.isEmpty(INSERT_PRESET_NAME[i])) {
                        builder = ContentProviderOperation.newInsert(Data.CONTENT_URI);
                        builder.withValueBackReference(StructuredName.RAW_CONTACT_ID, 0);
                        builder.withValue(Data.MIMETYPE, StructuredName.CONTENT_ITEM_TYPE);
                        builder.withValue(StructuredName.DISPLAY_NAME, INSERT_PRESET_NAME[i]);
                        builder.withValue(Data.IS_READ_ONLY, 1);
                        operationList.add(builder.build());
                    }

                    // preset email
                    if (!TextUtils.isEmpty(INSERT_PRESET_EMAIL[i])) {
                        builder = ContentProviderOperation.newInsert(Data.CONTENT_URI);
                        builder.withValueBackReference(Email.RAW_CONTACT_ID, 0);
                        builder.withValue(Data.MIMETYPE, Email.CONTENT_ITEM_TYPE);
                        builder.withValue(Email.DATA, INSERT_PRESET_EMAIL[i]);
                        builder.withValue(Data.IS_READ_ONLY, 1);
                        operationList.add(builder.build());
                    }

                    // preset photo
                    if (photoId != 0) {
                        builder = ContentProviderOperation.newInsert(Data.CONTENT_URI);
                        builder.withValueBackReference(Photo.RAW_CONTACT_ID, 0);
                        builder.withValue(Data.MIMETYPE, Photo.CONTENT_ITEM_TYPE);
                        builder.withValue(Data.IS_READ_ONLY, 1);
                        Bitmap contactPhoto = BitmapFactory.decodeResource(getResources(), photoId);
                        ByteArrayOutputStream baos = new ByteArrayOutputStream();
                        contactPhoto.compress(Bitmap.CompressFormat.PNG, 100, baos);
                        byte[] photo = baos.toByteArray();
                        builder.withValue(Photo.PHOTO, photo);
                        operationList.add(builder.build());
                    }

                    try {
                        for (ContentProviderOperation operation : operationList) {
                            Log.d(TAG, "ContentProviderOperation: "+operation.toString());
                        }
                        getContentResolver().applyBatch(ContactsContract.AUTHORITY, operationList);
                        Log.d(TAG, "All Done");
                    } catch (RemoteException e) {
                        Log.e(TAG, String.format("%s: %s", e.toString(), e.getMessage()));
                    } catch (OperationApplicationException e) {
                        Log.e(TAG, String.format("%s: %s", e.toString(), e.getMessage()));
                    } catch (RuntimeException e) {
                        Log.e(TAG, "RuntimeException " + String.format("%s: %s", e.toString(), e.getMessage()));
                    }
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
            return true;
        }

        protected void execute() {
            executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, (Void[]) null);
        }
    }
}
