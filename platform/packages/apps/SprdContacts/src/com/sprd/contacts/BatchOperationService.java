
package com.sprd.contacts;

/* XXX: Please trim these imports, make them in
 * order to improve readability of our codes. */
import com.sprd.contacts.activities.CancelBatchOperationActivity;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.android.contacts.common.model.account.AccountType;
import com.android.contacts.common.model.dataitem.DataKind;
import com.android.contacts.common.model.RawContactDelta;
import com.android.contacts.common.model.ValuesDelta;
import com.android.contacts.common.model.RawContactDeltaList;
import com.sprd.contacts.common.model.account.PhoneAccountType;
import com.sprd.contacts.common.model.account.SimAccountType;
import com.sprd.contacts.common.model.account.USimAccountType;
import com.sprd.contacts.util.AccountRestrictionUtils;
import com.android.contacts.common.util.Constants;

import com.android.internal.telephony.IccPBForMimetypeException;
import com.android.internal.telephony.IccPBForRecordException;

import com.android.contacts.R;
import android.provider.ContactsContract.RawContacts;
import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentProviderResult;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.database.Cursor;
/* The MatrixCursor is of the patch of Wei(Optimize performance) */
import android.database.MatrixCursor;
import android.database.sqlite.SQLiteDiskIOException;
import android.database.sqlite.SQLiteFullException;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.IBinder;
import android.os.Binder;
import android.os.PowerManager;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.provider.ContactsContract.*;
import android.provider.ContactsContract.CommonDataKinds.*;
import android.provider.ContactsContract.Contacts;
import android.support.v4.app.NotificationCompat;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.lang.Iterable;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.WeakHashMap;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;

import android.app.ActivityManager;

public class BatchOperationService extends Service {
    private static final String TAG = "BatchOperationService";

    public static final String KEY_MODE = "mode";
    public static final String KEY_NOTIFICATION_TAG = "tag";

    private static final String KEY_DELETE_TAG = "delete";

    public static final int MODE_START_BATCH_IMPORT_EXPORT = 1;
    public static final int MODE_START_BATCH_DELETE = 2;
    public static final int MODE_START_BATCH_STARRED = 3;
    public static final int MODE_CANCEL = 4;

    public static final int PHONE_TYPE = 2;
    public static final int PHONE_OTHER_TYPE = 7;
    public static final int EMAIL_TYPE = 1;
    public static final int EVENT_TYPE = 3;
    public static final int IM_TYPE = 4;
    public static final int ORGANIZATION_TYPE = 5;
    public static final String mark = "MARK&MARK&";

    public String mImportExportTag;
    private BroadcastReceiver mCallTaskReceiver;
    private AccountWithDataSet mDestAccount;

    private Map<String, AsyncTask> mTasks = new WeakHashMap<String, AsyncTask>();

    private MyBinder mBinder = new MyBinder();

    private PowerManager mPowerManager = null;
    private PowerManager.WakeLock mWakeLock = null;

    private ActivityManager mActivityManager;
    /**
     * SPRD: add iLog Original Android code:
     *
     * @{
     */
    private String mFilterName = null;
    /**
     * @}
     */

    private boolean isSimAccount(AccountWithDataSet destAccount) {
        return destAccount != null && (SimAccountType.ACCOUNT_TYPE.equals(destAccount.type) ||
                USimAccountType.ACCOUNT_TYPE.equals(destAccount.type));
    }

    public class MyBinder extends Binder {
        public BatchOperationService getService() {
            return BatchOperationService.this;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public int onStartCommand(Intent intent, int flag, int startId) {
        if (intent == null) {
            return START_NOT_STICKY;
        }
        IntentFilter filter = new IntentFilter(
                "android.intent.action.ACTION_STK_REFRESH_SIM_CONTACTS");
        mCallTaskReceiver = new CancelTaskReceiver();
        registerReceiver(mCallTaskReceiver, filter);
        /*
         * SPRD: Bug 334280 when Import phone Contacts to SIM ,kill the process,the task will stop.
         * @{
         */
        mActivityManager = (ActivityManager) this
                .getSystemService(Context.ACTIVITY_SERVICE);
        /*
         * @}
         */
        int mode = intent.getIntExtra(KEY_MODE, -1);
        Log.i(TAG, "BatchOperationService: mode:" + mode);
        switch (mode) {
            case MODE_START_BATCH_IMPORT_EXPORT: {
                mImportExportTag = UUID.randomUUID().toString();
                ArrayList<String> ids = intent.getStringArrayListExtra("result_alternative");
                AccountWithDataSet account = (AccountWithDataSet) (intent
                        .getParcelableExtra("dst_account"));
                /**
                 * SPRD: add iLog Original Android code:
                 *
                 * @{
                 */
                mFilterName = (String) intent.getStringExtra("filter");
                /**
                 * @}
                 */
                BatchImportExportTask task = new BatchImportExportTask(this, account, ids,
                        mImportExportTag);
                mTasks.put(mImportExportTag, task);
                task.executeOnExecutor(AsyncTask.SERIAL_EXECUTOR);
            }
                break;
            case MODE_START_BATCH_DELETE: {
                String notificationTag = KEY_DELETE_TAG;
                ArrayList<String> lookupKeys = intent.getStringArrayListExtra("result");
                /**
                 * SPRD: add iLog Original Android code:
                 *
                 * @{
                 */
                mFilterName = (String) intent.getStringExtra("filter");
                /**
                 * @}
                 */
                BatchDeleteTask task = new BatchDeleteTask(this, lookupKeys, notificationTag);
                mTasks.put(notificationTag, task);
                Log.i(TAG, "BatchOperationService: execute task: " + notificationTag);
                task.executeOnExecutor(AsyncTask.SERIAL_EXECUTOR);
            }
                break;
            case MODE_START_BATCH_STARRED: {
                String notificationTag = UUID.randomUUID().toString();
                ArrayList<String> lookupKeys = intent.getStringArrayListExtra("result");
                BatchStarredTask task = new BatchStarredTask(this, lookupKeys, notificationTag);
                mTasks.put(notificationTag, task);
                Log.i(TAG, "BatchOperationService: execute task: " + notificationTag);
                task.executeOnExecutor(AsyncTask.SERIAL_EXECUTOR);
            }
                break;

            case MODE_CANCEL: {
                String notificationTag = intent.getStringExtra(KEY_NOTIFICATION_TAG);
                AsyncTask task = mTasks.get(notificationTag);
                Log.i(TAG, "BatchOperationService: cancel task: " + notificationTag);
                if (task != null) {
                    mTasks.remove(notificationTag);
                    task.cancel(false);
                }
            }
                break;
            default:
                Log.i(TAG, "BatchOperationService: unknown mode:" + mode);
                break;
        }
        return START_NOT_STICKY;
    }

    public void onDestroy() {
        unregisterReceiver(mCallTaskReceiver);
    }

    public boolean isRunning() {
        return mTasks.size() > 0;
    }

    class BatchDeleteTask extends AsyncTask<Void, Integer, Integer> {
        private NotificationManager mNotificationManager;
        private NotificationCompat.Builder mBuilder;
        private int mMax;
        private ArrayList<String> mLookupKeys;
        private Resources mResources;
        private Context mContext;
        private String mTag;
        private PendingIntent mCancelPendingIntent;
        /**
         * SPRD:Bug442330 Add notification for batchDelete contacts when sqlite full.
         * @{
         */
        private Exception mDeleteLastException = null;
        /**
         * @}
         */

        private static final int sTransactionSize = 100;

        public BatchDeleteTask(Context context, ArrayList<String> lookupKeys, String tag) {
            mContext = context;
            mLookupKeys = lookupKeys;
            mMax = lookupKeys.size();
            mTag = tag;
            Intent intent = new Intent(mContext, CancelBatchOperationActivity.class);
            intent.putExtra(BatchOperationService.KEY_NOTIFICATION_TAG, tag);
            intent.putExtra(BatchOperationService.KEY_MODE,
                    BatchOperationService.MODE_START_BATCH_DELETE);
            // NOTE: use tag.hashCode() as the requestCode is error prone.
            mCancelPendingIntent = PendingIntent.getActivity(mContext, tag.hashCode(), intent,
                    PendingIntent.FLAG_UPDATE_CURRENT);
            mNotificationManager = (NotificationManager) mContext
                    .getSystemService(Context.NOTIFICATION_SERVICE);
            mBuilder = new NotificationCompat.Builder(mContext);
            mResources = mContext.getResources();
        }

        public void onPreExecute() {
            if (true/* android.os.Debug.isDebugOptimizing() */)
                Log.v(TAG,
                        "DELETE_START: " + " SystemClock: " + android.os.SystemClock.uptimeMillis());
            String title = mResources.getString(R.string.working_batch_delete);
            mNotificationManager.cancel(mTag, 0);
            publishProgress(0);

            try {
                mPowerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
                mWakeLock = mPowerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "appBackup");
                mWakeLock.acquire();
            } catch (SecurityException e) {
                Log.w(TAG, "No permission to acquire wake lock");
                e.printStackTrace();
                mWakeLock = null;
            }
        }

        public Integer doInBackground(Void... v) {
            /**
             * SPRD: add iLog Original Android code:
             *
             * @{
             */
            /*
             * if (trueLog.isIloggable()){
             *   Log.startPerfTracking(Constants.PERFORMANCE_TAG +
             *         String.format(": Start deleting %d contacts from %s", mMax, mFilterName));
             * }
             *
            /**
             * @}
             */
            /*
             * SPRD: Bug 378691 when Import phone Contacts to SIM ,kill the process,the task will
             * stop.
             * @{
             */
//            mActivityManager.setSelfProtectStatus(ActivityManager.PROCESS_STATUS_MAINTAIN);//sprdporting
            /*
             * @}
             */
            ContentResolver resolver = mContext.getContentResolver();
            int progress = 0;
            ArrayList<String> lookupKeys = mLookupKeys;

            ArrayList<ContentProviderOperation> operations = new ArrayList<ContentProviderOperation>();
            int count = 0;
            for (String key : lookupKeys) {
                if (isCancelled()) {
                    return 0;
                }
                operations.add(ContentProviderOperation
                        .newDelete(Uri.withAppendedPath(Contacts.CONTENT_LOOKUP_URI, key))
                        .withYieldAllowed(false)
                        .build());
                progress++;
                if (operations.size() > sTransactionSize) {
                    try {
                        resolver.applyBatch(ContactsContract.AUTHORITY, operations);
                    } catch (Exception e) {
                        // throw new
                        // RuntimeException("Failed to delete contact", e);
                        Log.e(TAG, "Failed to delete contact:" + e);
                        /**
                         * SPRD:Bug442330 Add notification for batchDelete contacts when sqlite full.
                         * @{
                         */
                        mDeleteLastException = e;
                        /**
                         * @}
                         */
                    }
                    operations.clear();
                    publishProgress(progress);
                }
            }

            try {
                resolver.applyBatch(ContactsContract.AUTHORITY, operations);
            } catch (Exception e) {
                // throw new RuntimeException("Failed to delete contact", e);
                Log.e(TAG, "Failed to delete contact:" + e);
                /**
                 * SPRD:Bug442330 Add notification for batchDelete contacts when sqlite full.
                 * @{
                 */
                mDeleteLastException = e;
                /**
                 * @}
                 */
            }
            return 0;
        }

        public void onProgressUpdate(Integer... progress) {
            String title = mResources.getString(R.string.working_batch_delete);
            /*
             * SPRD: Bug355247 Batch delete too slow,The progress bar set initial value
             * @{
             */
            if (mMax > 10 && progress[0] == 0) {
                progress[0] = progress[0] + 10;
            }
            /*
             * @}
             */
            mNotificationManager.notify(mTag, 0, mBuilder.setAutoCancel(false)
                    .setContentTitle(title)
                    .setContentText(title)
                    .setTicker(title)
                    .setOngoing(true)
                    .setSmallIcon(android.R.drawable.stat_sys_warning)
                    .setProgress(mMax, progress[0], false)
                    .setContentIntent(mCancelPendingIntent)
                    .build());
        }

        @Override
        public void onCancelled(Integer result) {
            Log.i(TAG, "BatchOperationService: onCancelled:" + mTag);
            String title = mResources.getString(R.string.batch_operation_canceled);
            mNotificationManager.notify(
                    mTag,
                    0,
                    mBuilder.setAutoCancel(true)
                            .setContentTitle(title)
                            .setContentText(title)
                            .setTicker(title)
                            .setOngoing(false)
                            .setProgress(0, 0, false)
                            .setSmallIcon(android.R.drawable.stat_sys_warning)
                            .setContentIntent(
                                    PendingIntent.getActivity(mContext, mTag.hashCode(),
                                            new Intent(), PendingIntent.FLAG_CANCEL_CURRENT))
                            .build());
            if (mWakeLock != null) {
                Log.i(TAG, "onPostExecute : release wake lock ");
                mWakeLock.release();
                mWakeLock = null;
            }
            /*
             * SPRD: Bug 378691 when Import phone Contacts to SIM ,kill the process,the task will
             * stop.
             * @{
             */
//            mActivityManager.setSelfProtectStatus(ActivityManager.PROCESS_STATUS_IDLE);//sprdporting
            /*
             * @}
             */
        }

        public void onPostExecute(Integer i) {
            // SPRD: debug optimizing test log
            if (true/* android.os.Debug.isDebugOptimizing() */)
                Log.v(TAG,
                        "DELETE_END: " + " SystemClock: " + android.os.SystemClock.uptimeMillis());
            /**
             * SPRD: add iLog Original Android code:
             *
             * @{
             */
            if (true/* Log.isIloggable() */) {
                /*
                 * Log.stopPerfTracking(Constants.PERFORMANCE_TAG +
                 * String.format(": Successfully finish deleting %d contacts from %s", mMax,
                 * mFilterName));
                 */// sprd_4.4
            }
            /**
             * @}
             */
            String title = mResources.getString(R.string.delete_done);
            /**
             * SPRD:Bug442330 Add notification for batchDelete contacts when sqlite full.
             * @{
             */
            if (mDeleteLastException instanceof SQLiteFullException ||
                    mDeleteLastException instanceof SQLiteDiskIOException) {
                title = mResources.getString(R.string.sqlite_full);
            }
            /**
             * @}
             */
            mNotificationManager.notify(
                    mTag,
                    0,
                    mBuilder.setAutoCancel(true)
                            .setContentTitle(title)
                            .setContentText(title)
                            .setTicker(title)
                            .setOngoing(false)
                            .setProgress(0, 0, false)
                            .setSmallIcon(android.R.drawable.stat_sys_warning)
                            .setContentIntent(
                                    PendingIntent.getActivity(mContext, mTag.hashCode(),
                                            new Intent(), PendingIntent.FLAG_CANCEL_CURRENT))
                            .build());
            if (mWakeLock != null) {
                Log.i(TAG, "onPostExecute : release wake lock ");
                mWakeLock.release();
                mWakeLock = null;
            }
            mTasks.remove(mTag);
            /*
             * SPRD: Bug 378691 when Import phone Contacts to SIM ,kill the process,the task will
             * stop.
             * @{
             */
//            mActivityManager.setSelfProtectStatus(ActivityManager.PROCESS_STATUS_IDLE);//sprdporting
            /*
             * @}
             */
        }
    }

    class BatchImportExportTask extends AsyncTask<Void, Integer, Integer> {
        AccountManager mAm;
        private NotificationManager mNotificationManager;
        private NotificationCompat.Builder mBuilder;
        private int mMax;
        private Resources mResources;
        private AccountType mAccountType;
        private int mSuccCount = 0;
        private int mFailedCount = 0;
        private Context mContext;
        private Exception mLastException = null;
        private ArrayList<String> mContactIds;
        private String mTag;
        private PendingIntent mCancelPendingIntent;
        private static final int sTransactionSize = 100;
        private boolean isAccountFull = false;
        boolean isValidContact = false;
        int mProcedding = 0;
        int mProgress = 0;
        int mProceddingForContact = 0;

        public BatchImportExportTask(Context context, AccountWithDataSet account,
                ArrayList<String> ids, String tag) {
            if (context == null || account == null || ids == null || tag == null) {
                return;
            }
            mContext = context;
            mAm = AccountManager.get(mContext);
            mDestAccount = account;
            mContactIds = ids;
            mMax = mContactIds.size();
            mTag = tag;
            Intent intent = new Intent(mContext, CancelBatchOperationActivity.class);
            intent.putExtra(BatchOperationService.KEY_NOTIFICATION_TAG, tag);
            intent.putExtra(BatchOperationService.KEY_MODE,
                    BatchOperationService.MODE_START_BATCH_IMPORT_EXPORT);
            mCancelPendingIntent = PendingIntent.getActivity(context, tag.hashCode(), intent,
                    PendingIntent.FLAG_UPDATE_CURRENT);
            mNotificationManager = (NotificationManager) mContext
                    .getSystemService(Context.NOTIFICATION_SERVICE);
            mBuilder = new NotificationCompat.Builder(mContext);
            mResources = mContext.getResources();
            mAccountType = AccountTypeManager.getInstance(mContext).getAccountType(account.type,
                    null);
            int vacancies = getAccountVacancies(context, account);
            if (vacancies >= 0 && mMax > vacancies) {
                Toast.makeText(context, R.string.beyond_sim_card_capacity, Toast.LENGTH_LONG)
                        .show();
            }
        }

        public void onPreExecute() {
            mSuccCount = 0;
            isAccountFull = false;
            String title = mResources.getString(R.string.working_batch_import);

            mNotificationManager.notify(mTag, 0, mBuilder.setAutoCancel(false)
                    .setContentTitle(title)
                    .setTicker(title)
                    .setSmallIcon(android.R.drawable.stat_sys_download)
                    .setProgress(mMax, 0, false)
                    .setContentIntent(mCancelPendingIntent)
                    .setOngoing(true)
                    .build());

            try {
                mPowerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
                mWakeLock = mPowerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "appBackup");
                mWakeLock.acquire();
            } catch (SecurityException e) {
                Log.w(TAG, "No permission to acquire wake lock");
                e.printStackTrace();
                mWakeLock = null;
            }
        }

        private Map<String, MatrixCursor> query(ContentResolver resolver) {

            Map<String, MatrixCursor> ret = new HashMap<String, MatrixCursor>();
            StringBuilder sb = new StringBuilder();
            sb.append(RawContacts.CONTACT_ID + " in (");
            boolean isInitial = true;
            for (String key : mContactIds) {
                if (!isInitial) {
                    sb.append(",");
                }
                isInitial = false;
                sb.append("\"" + key + "\"");
            }
            sb.append(")");
            Cursor cursor = resolver.query(
                    RawContactsEntity.CONTENT_URI,
                    new String[] {
                            RawContacts.CONTACT_ID, Data.MIMETYPE, Data.DATA1, Data.DATA2,
                            Data.DATA14, Data.DATA3, Data.DATA4, Data.DATA5, Data.DATA6,
                            Data.DATA7, Data.DATA8, Data.DATA9
                    },
                    sb.toString(), null, null);

            try {
                if (cursor.moveToFirst()) {
                    do {
                        String key = cursor.getString(0);
                        MatrixCursor tmpCursor = ret.get(key);
                        if (tmpCursor == null) {
                            tmpCursor = new MatrixCursor(new String[] {
                                    Data.MIMETYPE, Data.DATA1, Data.DATA2, Data.DATA14, Data.DATA3,
                                    Data.DATA4, Data.DATA5, Data.DATA6, Data.DATA7, Data.DATA8,
                                    Data.DATA9
                            });
                            ret.put(key, tmpCursor);
                        }
                        tmpCursor.addRow(
                                new String[] {
                                        cursor.getString(1),
                                        cursor.getString(2),
                                        cursor.getString(3),
                                        cursor.getString(4),
                                        cursor.getString(5),
                                        cursor.getString(6),
                                        cursor.getString(7),
                                        cursor.getString(8),
                                        cursor.getString(9),
                                        cursor.getString(10),
                                        cursor.getString(11)
                                }
                                );
                    } while (cursor.moveToNext());
                }
            } finally {
                cursor.close();
            }
            return ret;
        }

        public Integer doInBackground(Void... v) {
            /**
             * SPRD: add iLog Original Android code:
             *
             * @{
             */
            /*
             * if (Log.isIloggable()) { Log.startPerfTracking(Constants.PERFORMANCE_TAG +
             * String.format(": Start copying %d contacts from %s to %s", mMax, mFilterName,
             * mDestAccount.name)); }
             */// sprd_4.4
            /**
             * @}
             */
            /*
             * SPRD: Bug 378691 when Import phone Contacts to SIM ,kill the process,the task will
             * stop.
             * @{
             */
//            mActivityManager.setSelfProtectStatus(ActivityManager.PROCESS_STATUS_MAINTAIN);//sprdporting
            /*
             * @}
             */
            ContentResolver resolver = mContext.getContentResolver();
            ContentValues accountContentValues = new ContentValues();
            accountContentValues.put(RawContacts.ACCOUNT_NAME, mDestAccount.name);
            accountContentValues.put(RawContacts.ACCOUNT_TYPE, mDestAccount.type);
            ValuesDelta accountValues = ValuesDelta.fromAfter(accountContentValues);
            ArrayList<ContentProviderOperation> operations = new ArrayList<ContentProviderOperation>();
            Map<String, MatrixCursor> datas = query(resolver);
            for (String key : mContactIds) {
                if (isCancelled() || isAccountFull) {
                    break;
                }
                mProgress++;
                if (Constants.DEBUG)
                    Log.d(TAG, "mProgress = " + mProgress);
                ContactDataSplilter splitter = new ContactDataSplilter(datas.get(key));

                for (Map<String, List<String>> entity : splitter) {
                    if (isCancelled()) {
                        break;
                    }

                    ContentProviderOperation operationForRawContact = tryInsertToAccount(makeEntityDeltaForSingleContact(
                            accountValues, entity));
                    if (isAccountFull) {
                        break;
                    } else if (mLastException != null) {
                        continue;
                    }
                    mProcedding++;
                    if (Constants.DEBUG)
                        Log.d(TAG, "mProcedding = " + mProcedding);
                    operations.addAll(makeContentProviderOperationsForSingleContact(
                            operationForRawContact, operations.size(), entity));
                    isValidContact = true;
                }
                if (isValidContact) {
                    mProceddingForContact++;
                    isValidContact = false;
                } else {
                    if (!isAccountFull) {
                        mFailedCount++;
                    }
                }
                if (isAccountFull) {
                    mFailedCount = (mMax - mSuccCount - mProceddingForContact);
                }

                /*
                 * The patch of Wei has this operation but not in Sprdroid4.1 TODO: check the
                 * effect. if(operations.size() < sTransactionSize) { continue; }
                 */

                publishProgress(mProgress);
                if (operations.size() >= sTransactionSize) {
                    if (!applyBatch(operations)) {
                        return 0;
                    }
                }
            }
            applyBatch(operations);
            return 0;
        }

        private ArrayList<ContentProviderOperation> makeContentProviderOperationsForSingleContact(
                ContentProviderOperation operationForRawContact,
                int rawContactBackReference,
                Map<String, List<String>> entity) {
            ArrayList<ContentProviderOperation> ret = new ArrayList<ContentProviderOperation>();
            ret.add(operationForRawContact);
            Set<String> mimeTypes = entity.keySet();
            for (String mimeType : mimeTypes) {
                if (mimeType.equals(GroupMembership.CONTENT_ITEM_TYPE)) {
                    continue;
                }

                List<String> values = entity.get(mimeType);
                if (mimeType.equals(StructuredName.CONTENT_ITEM_TYPE) && ((values.size() > 0) && !values.isEmpty())) {
                    ContentValues nameContentValues = makeNameContentValuesForSingleContact(values,
                            mimeType);
                    ret.add(ContentProviderOperation.newInsert(Data.CONTENT_URI)
                            .withValueBackReference(Data.RAW_CONTACT_ID, rawContactBackReference)
                            .withValues(nameContentValues)
                            .withYieldAllowed(false)
                            .build());
                } else {
                    for (String value : values) {
                        ContentValues contentValues = new ContentValues();
                        if (mimeType.equals(Photo.CONTENT_ITEM_TYPE)) {
                            makeContentValuesForPhoto(contentValues, Long.valueOf(value));
                        } else {
                            contentValues.put(Data.MIMETYPE, mimeType);
                            if (mimeType.equals(Phone.CONTENT_ITEM_TYPE)) {
                                String valueType[] = value.split(mark);
                                int type = PHONE_TYPE;
                                if (valueType.length == 2) {
                                    value = valueType[0];
                                    type = Integer.parseInt(valueType[1]);
                                }
                                contentValues.put(Data.DATA2, type);
                            }
                            if (mimeType.equals(Email.CONTENT_ITEM_TYPE)) {
                                String valueType[] = value.split(mark);
                                int type = EMAIL_TYPE;
                                if (valueType.length == 2) {
                                    value = valueType[0];
                                    type = Integer.parseInt(valueType[1]);
                                }
                                if (valueType.length == 3) {
                                    value = valueType[0];
                                    type = Integer.parseInt(valueType[1]);
                                    contentValues.put(Data.DATA3, valueType[2]);
                                }
                                contentValues.put(Data.DATA2, type);
                            }
                            if (mimeType.equals(Event.CONTENT_ITEM_TYPE)) {
                                contentValues.put(Data.DATA2, EVENT_TYPE);
                            }
                            /* SPRD: Bug606604 During replication between accounts, some information has lost. @{ */
                            if (mimeType.equals(Im.CONTENT_ITEM_TYPE)) {
                                String valueType[] = value.split(mark);
                                int type = IM_TYPE;
                                if (valueType.length == 2) {
                                    value = valueType[0];
                                    type = Integer.parseInt(valueType[1]);
                                }
                                contentValues.put(Data.DATA5, type);
                            }
                            if (mimeType.equals(Organization.CONTENT_ITEM_TYPE)) {
                                String valueType[] = value.split(mark);
                                String type = "";
                                if (valueType.length == 2) {
                                    value = valueType[0];
                                    type = valueType[1];
                                }
                                contentValues.put(Data.DATA4, type);
                            }
                            /* @{ */
                            contentValues.put(Data.DATA1, value);
                        }
                        ret.add(ContentProviderOperation
                                .newInsert(Data.CONTENT_URI)
                                .withValueBackReference(Data.RAW_CONTACT_ID,
                                        rawContactBackReference)
                                .withValues(contentValues)
                                .withYieldAllowed(false)
                                .build());
                    }
                }
            }
            return ret;
        }

        private ContentValues makeNameContentValuesForSingleContact(List<String> values,
                String mimeType) {
            ContentValues contentValues = new ContentValues();
            contentValues.put(Data.MIMETYPE, mimeType);
            contentValues.put(Data.DATA1, values.get(0));
            contentValues.put(Data.DATA2, values.get(1));
            contentValues.put(Data.DATA14, values.get(2));
            contentValues.put(Data.DATA3, values.get(3));
            contentValues.put(Data.DATA4, values.get(4));
            contentValues.put(Data.DATA5, values.get(5));
            contentValues.put(Data.DATA6, values.get(6));
            /*
             * SPRD: Bug 596466 Pinyin can't copy to SIM account 
             * 
             * @{
             */
            if (!USimAccountType.ACCOUNT_TYPE.equals(mDestAccount.type)
                    && !SimAccountType.ACCOUNT_TYPE.equals(mDestAccount.type)) {
                contentValues.put(Data.DATA7, values.get(7));
                contentValues.put(Data.DATA8, values.get(8));
                contentValues.put(Data.DATA9, values.get(9));
            }
            /*
             * @}
             */
            return contentValues;
        }

        private RawContactDelta makeEntityDeltaForSingleContact(ValuesDelta accountValues,
                Map<String, List<String>> entity) {
            RawContactDelta ret = new RawContactDelta(accountValues);
            Set<String> mimeTypes = entity.keySet();
            for (String mimeType : mimeTypes) {
                if (mimeType.equals(GroupMembership.CONTENT_ITEM_TYPE)) {
                    // group membership is not duped
                    continue;
                }

                List<String> values = entity.get(mimeType);
                if (mimeType.equals(StructuredName.CONTENT_ITEM_TYPE) && ((values.size() > 0) && !values.isEmpty())) {
                    ContentValues nameContentValues = makeNameContentValuesForSingleContact(values,
                            mimeType);
                    ret.addEntry(ValuesDelta.fromAfter(nameContentValues));
                } else {
                    for (String value : values) {
                        ContentValues contentValues = new ContentValues();
                        if (mimeType.equals(Photo.CONTENT_ITEM_TYPE)) {
                            makeContentValuesForPhoto(contentValues, Long.valueOf(value));
                        } else {
                            contentValues.put(Data.MIMETYPE, mimeType);
                            if (mimeType.equals(Phone.CONTENT_ITEM_TYPE)) {
                                String valueType[] = value.split(mark);
                                int type = PHONE_TYPE;
                                if (valueType.length == 2) {
                                    value = valueType[0];
                                    type = Integer.parseInt(valueType[1]);
                                }
                                contentValues.put(Data.DATA2, type);
                            }
                            if (mimeType.equals(Email.CONTENT_ITEM_TYPE)) {
                                String valueType[] = value.split(mark);
                                int type = EMAIL_TYPE;
                                if (valueType.length == 2) {
                                    value = valueType[0];
                                    type = Integer.parseInt(valueType[1]);
                                }
                                if (valueType.length == 3) {
                                    value = valueType[0];
                                    type = Integer.parseInt(valueType[1]);
                                    contentValues.put(Data.DATA3, valueType[2]);
                                }
                                contentValues.put(Data.DATA2, type);
                            }
                            if (mimeType.equals(Event.CONTENT_ITEM_TYPE)) {
                                contentValues.put(Data.DATA2, EVENT_TYPE);
                            }
                            /* SPRD: Bug606604 During replication between accounts, some information has lost. @{ */
                            if (mimeType.equals(Im.CONTENT_ITEM_TYPE)) {
                                String valueType[] = value.split(mark);
                                int type = IM_TYPE;
                                if (valueType.length == 2) {
                                    value = valueType[0];
                                    type = Integer.parseInt(valueType[1]);
                                }
                                contentValues.put(Data.DATA5, type);
                            }
                            if (mimeType.equals(Organization.CONTENT_ITEM_TYPE)) {
                                String valueType[] = value.split(mark);
                                String type = "";
                                if (valueType.length == 2) {
                                    value = valueType[0];
                                    type = valueType[1];
                                }
                                contentValues.put(Data.DATA4, type);
                            }
                            /* @{ */
                            contentValues.put(Data.DATA1, value);
                        }
                        ret.addEntry(ValuesDelta.fromAfter(contentValues));
                    }
                }
            }
            return ret;
        }

        private ContentProviderOperation tryInsertToAccount(RawContactDelta delta) {
            mLastException = null;
            ContentProviderOperation ret = null;
            ContentValues proxyValues = null;
            try {
                ContentProviderOperation.Builder builder = ContentProviderOperation
                        .newInsert(RawContacts.CONTENT_URI)
                        .withValue(RawContacts.ACCOUNT_NAME, mDestAccount.name)
                        .withValue(RawContacts.ACCOUNT_TYPE, mDestAccount.type)
                        .withValue(RawContacts.AGGREGATION_MODE,
                                RawContacts.AGGREGATION_MODE_DISABLED)
                        .withYieldAllowed(false);

                proxyValues = AccountRestrictionUtils.get(mContext).tryInsert(delta);
                if (proxyValues != null) {
                    for (String proxyKey : proxyValues.keySet()) {
                        builder.withValue(proxyKey, proxyValues.getAsString(proxyKey));
                    }
                }
                ret = builder.build();
            } catch (Exception e) {
                if (Constants.DEBUG)
                    Log.d(TAG, "tryInsertToAccount exception = " + e.getMessage());
                mLastException = e;
                if (e instanceof IccPBForRecordException) {
                    IccPBForRecordException exception = (IccPBForRecordException) e;
                    if (Constants.DEBUG)
                        Log.d(TAG, "tryInsertToAccount exception = " + exception.mErrorCode);
                    if (exception.mErrorCode == IccPBForRecordException.ADN_RECORD_CAPACITY_FULL) {
                        isAccountFull = true;
                    }
                }

            }
            return ret;
        }

        private boolean applyBatch(ArrayList<ContentProviderOperation> operations) {
            boolean successful = false;
            try {
                mContext.getContentResolver().applyBatch(ContactsContract.AUTHORITY, operations);
                successful = true;
            } catch (Exception e) {
                mLastException = e;
                return false;
            } finally {
                if (successful) {
                    mSuccCount += mProceddingForContact;
                } else {
                    mFailedCount += mProceddingForContact;
                }
                mProcedding = 0;
                mProceddingForContact = 0;
                operations.clear();
            }
            return true;
        }

        private void makeContentValuesForPhoto(ContentValues contentValues, long displayPhotoId) {
            if (displayPhotoId <= 0 || contentValues == null) {
                Log.e(TAG, "Photo file id is invalid!");
                return;
            }
            final Uri displayPhotoUri = ContentUris.withAppendedId(
                    DisplayPhoto.CONTENT_URI, displayPhotoId);
            if (displayPhotoUri == null) {
                return;
            }
            try {
                AssetFileDescriptor fd = getContentResolver()
                        .openAssetFileDescriptor(displayPhotoUri, "r");
                FileInputStream fis = fd.createInputStream();
                ByteArrayOutputStream originalPhoto = new ByteArrayOutputStream();
                byte[] buffer = new byte[16 * 1024];

                try {
                    int size;
                    while ((size = fis.read(buffer)) != -1) {
                        originalPhoto.write(buffer, 0, size);
                    }
                    contentValues.put(Photo.PHOTO,
                            originalPhoto.toByteArray());
                } finally {
                    fis.close();
                    fd.close();
                }
            } catch (IOException ioe) {
                // Just fall back to the case below.
                Log.e(TAG,
                        "Problem writing stream into an ParcelFileDescriptor: "
                                + ioe.toString());
                return;
            }
            contentValues.put(Data.IS_SUPER_PRIMARY, 1);
            contentValues.put(Photo.MIMETYPE, Photo.CONTENT_ITEM_TYPE);
            return;
        }

        public void onProgressUpdate(Integer... progress) {
            String title = mResources.getString(R.string.working_batch_import);

            mNotificationManager.notify(mTag, 0, mBuilder.setAutoCancel(false)
                    .setContentTitle(title)
                    .setTicker(title)
                    .setSmallIcon(android.R.drawable.stat_sys_download)
                    .setProgress(mMax, progress[0], false)
                    .setContentIntent(mCancelPendingIntent)
                    .setOngoing(true)
                    .build());
        }

        @Override
        public void onCancelled(Integer result) {
            Log.i(TAG, "BatchOperationService: onCancelled:" + mTag);
            String title = mResources.getString(R.string.batch_operation_canceled);

            mNotificationManager.notify(mTag, 0, mBuilder.setAutoCancel(true)
                    .setContentTitle(title)
                    .setContentText(title)
                    .setTicker(title)
                    .setOngoing(false)
                    .setSmallIcon(android.R.drawable.stat_sys_warning)
                    .setProgress(0, 0, false)
                    .setContentIntent(
                            PendingIntent.getActivity(mContext, mTag.hashCode(),
                                    new Intent(), 0))
                    .build());
            if (mWakeLock != null) {
                Log.i(TAG, "onPostExecute : release wake lock ");
                mWakeLock.release();
                mWakeLock = null;
            }
            /*
             * SPRD: Bug 378691 when Import phone Contacts to SIM ,kill the process,the task will
             * stop.
             * @{
             */
//            mActivityManager.setSelfProtectStatus(ActivityManager.PROCESS_STATUS_IDLE);//sprdporting
            /*
             * @}
             */
        }

        public void onPostExecute(Integer i) {
            /**
             * SPRD: add iLog Original Android code:
             *
             * @{
             */
            if (true/* Log.isIloggable() */) {
                /*
                 * Log.stopPerfTracking(Constants.PERFORMANCE_TAG +
                 * String.format(": Successfully finish copying %d contacts from %s to %s", mMax,
                 * mFilterName, mDestAccount.name));
                 */// sprd_4.4
            }
            /**
             * @}
             */
            String title = "";
            String text = mResources
                    .getString(R.string.import_failed_tmp, mSuccCount, mFailedCount);

            if (mLastException == null) {
                title = mResources.getString(R.string.import_done);
            } else if (mLastException instanceof IccPBForRecordException) {

                IccPBForRecordException exception = (IccPBForRecordException) mLastException;
                if (Constants.DEBUG)
                    Log.d(TAG, "IccPBForRecordException:ErrorCode = " + exception.mErrorCode);
                if (exception.mErrorCode == IccPBForRecordException.ADN_RECORD_CAPACITY_FULL) {
                    title = mResources.getString(R.string.violate_capacity);
                } else if (exception.mErrorCode == IccPBForRecordException.ANR_RECORD_CAPACITY_FULL) {
                    title = mResources.getString(R.string.anr_is_full);
                } else {
                    title = mResources.getString(R.string.import_failed);
                }
            } else if (mLastException instanceof IccPBForMimetypeException) {
                IccPBForMimetypeException exception = (IccPBForMimetypeException) mLastException;
                if (Constants.DEBUG)
                    Log.d(TAG, "IccPBForMimetypeException:ErrorCode = " + exception.mErrorCode
                            + " mimetype = " + exception.mMimeType);
                if (exception.mErrorCode == IccPBForMimetypeException.CAPACITY_FULL) {
                    if (Email.CONTENT_ITEM_TYPE.equals(exception.mMimeType)) {
                        if (SimAccountType.ACCOUNT_TYPE.equals(mDestAccount.type)) {
                            title = mResources
                                    .getString(R.string.contactSavedErrorToastSimEmailFull);
                        } else if (USimAccountType.ACCOUNT_TYPE.equals(mDestAccount.type)) {
                            title = mResources
                                    .getString(R.string.contactSavedErrorToastUsimEmailFull);
                        } else {
                            title = mResources.getString(R.string.contactSavedErrorToastEmailFull);
                        }
                    }
                } else if (exception.mErrorCode == IccPBForMimetypeException.OVER_LENGTH_LIMIT) {
                    if (Phone.CONTENT_ITEM_TYPE.equals(exception.mMimeType)) {
                        title =
                                mResources.getString(R.string.over_phone_length_limit);
                    } else if (CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE
                            .equals(exception.mMimeType)) {
                        title = mResources.getString(R.string.contactSavedErrorToastNameLength);
                    }
                } else {
                    title = mResources.getString(R.string.import_failed);
                }

            } else if (mLastException instanceof SQLiteFullException ||
                    mLastException instanceof SQLiteDiskIOException)
            {
                title = mResources.getString(R.string.sqlite_full);
            } else {
                title = mResources.getString(R.string.import_failed);
            }

            mNotificationManager.notify(mTag, 0, mBuilder.setAutoCancel(true)
                    .setContentTitle(title)
                    .setContentText(text)
                    .setTicker(title)
                    .setOngoing(false)
                    .setSmallIcon(android.R.drawable.stat_sys_download_done)
                    .setProgress(0, 0, false)
                    .setContentIntent(
                            PendingIntent.getActivity(mContext, mTag.hashCode(), new Intent(), 0))
                    .build());
            mTasks.remove(mTag);
            if (mWakeLock != null) {
                Log.i(TAG, "onPostExecute : release wake lock ");
                mWakeLock.release();
                mWakeLock = null;
            }
            /*
             * SPRD: Bug 378691 when Import phone Contacts to SIM ,kill the process,the task will
             * stop.
             * @{
             */
//            mActivityManager.setSelfProtectStatus(ActivityManager.PROCESS_STATUS_IDLE);//sprdporting
            /*
             * @}
             */
        }

        private int getAccountVacancies(Context context, AccountWithDataSet account) {
            String accountCapacity = AccountRestrictionUtils.get(mContext).getUserData(
                    new Account(account.name, account.type), "capacity");
            int accountCapacityInt = 0;
            if (accountCapacity != null) {
                accountCapacityInt = Integer.parseInt(accountCapacity);
            } else {
                accountCapacityInt = -1;
            }
            if (accountCapacityInt == -1) {
                return -1;
            }

            ContentResolver cr = context.getContentResolver();
            int providerCapacity = -1;
            Cursor cursor = cr.query(RawContacts.CONTENT_URI.buildUpon()
                    .appendQueryParameter(RawContacts.ACCOUNT_NAME, account.name)
                    .appendQueryParameter(RawContacts.ACCOUNT_TYPE, account.type)
                    .build(),
                    null, "deleted=0", null, null
                    );

            if (cursor != null) {
                providerCapacity = cursor.getCount();
                cursor.close();
            }
            return accountCapacityInt - providerCapacity;
        }

        class ContactDataSplilter implements Iterable<Map<String, List<String>>> {
            class Entity {
                List<String> mDatas;
                int mStep;
                int mCurrent;

                public Entity(List<String> datas, int step) {
                    mCurrent = 0;
                    mStep = step;
                    mDatas = datas;
                }

                public boolean onProcess() {
                    return mCurrent < mDatas.size();
                }
            }

            private Map<String, Entity> mEntities = new HashMap<String, Entity>();

            /**
             * SPRD:Bug537707 Change the type of even numbers to fixed number.
             * @{
             */
            private boolean mEvenPhoneNumber;
            /**
             * @}
             */
            public ContactDataSplilter(Cursor cursor) {
                /**
                 * SPRD:Bug537707 Change the type of even numbers to fixed number.
                 * @{
                 */
                mEvenPhoneNumber = true;
                /**
                 * @}
                 */
                Map<String, List<String>> entities = new HashMap<String, List<String>>();
                Map<String, Integer> steps = new HashMap<String, Integer>();
                final String[] projections = new String[] {
                        Data.MIMETYPE, Data.DATA1, Data.DATA2, Data.DATA14
                };

                if (cursor != null && cursor.moveToFirst()) {
                    do {
                        String mimeType = cursor.getString(0);
                        if (TextUtils.isEmpty(mimeType)) {
                            continue;
                        }

                        // typeOverallMax doesn't work for GroupMembership, so skip
                        // it
                        if (mimeType.equals(GroupMembership.CONTENT_ITEM_TYPE)) {
                            continue;
                        }

                        DataKind kind = mAccountType.getKindForMimetype(mimeType);
                        // dest account doesn't support this mimeType
                        if (kind == null) {
                            continue;
                        }
                        List<String> datas = entities.get(mimeType);
                        if (datas == null) {
                            datas = new ArrayList<String>();
                            entities.put(mimeType, datas);
                        }
                        String value = cursor.getString(1);
                        if (!TextUtils.isEmpty(value)) {
                            AccountManager am = mAm;
                            Account account = new Account(mDestAccount.name, mDestAccount.type);
                            String tmp = AccountRestrictionUtils.get(mContext).getUserData(account,
                                    mimeType + "_length");
                            int max = -1;
                            if (tmp != null) {
                                max = Integer.parseInt(tmp);
                                if (Constants.DEBUG)
                                    Log.d(TAG, mDestAccount.name + "account mimeType_length = "
                                            + max);
                            }
                            int byteLen = 1;
                            if (max != -1
                                    && AccountRestrictionUtils.getGsmAlphabetBytes(value).length > max) {
                                for (int i = 0; i < max; i++) {
                                    String tempStr = String.valueOf(value.charAt(i));
                                    byteLen = AccountRestrictionUtils.getGsmAlphabetBytes(tempStr).length;
                                    if (byteLen > 1) {
                                        if (StructuredName.CONTENT_ITEM_TYPE.equals(mimeType)) {
                                            max = max - 1;
                                        }
                                        break;
                                    }
                                }
                                value = value.substring(0, max / byteLen);
                            }
                        }
                        if (!TextUtils.isEmpty(value)) {
                            if (mimeType.equals(StructuredName.CONTENT_ITEM_TYPE)) {
                                for (int icolumn = 1; icolumn <= 10; icolumn++) {
                                    String values = cursor.getString(icolumn);
                                    datas.add(values);
                                }
                            } else {
                                value = addType(mimeType, value, cursor);
                                datas.add(value);
                            }
                        }
                        // if dest account support photo
                        if (mimeType.equals(Photo.CONTENT_ITEM_TYPE)) {
                            String photoFileId = cursor.getString(3);
                            if (photoFileId != null) {
                                datas.add(photoFileId);
                            }
                        }
                        if (!steps.containsKey(mimeType)) {
                            int typeOverallMax = AccountRestrictionUtils.get(mContext)
                                    .getTypeOverallMax(mDestAccount, mimeType);
                            if (typeOverallMax != 0) {
                                kind.typeOverallMax = typeOverallMax;
                            }
                            steps.put(mimeType, kind.typeOverallMax);
                        }
                    } while (cursor.moveToNext());
                }
                if (cursor != null) {
                    cursor.close();
                }

                Set<String> mimeTypes = entities.keySet();
                for (String mimeType : mimeTypes) {
                    mEntities
                            .put(mimeType, new Entity(entities.get(mimeType), steps.get(mimeType)));
                }
            }

            private String addType(String mimeType, String value, Cursor cursor) {
                String type = null;
                if (mimeType.equals(Phone.CONTENT_ITEM_TYPE)) {
                    type = cursor.getString(2);
                    if (type != null) {
                        /**
                         * SPRD:Bug537707 Change the type of even numbers to fixed number.
                         * SPRD:Bug598129 Copy sim to local phone, the fixed number not correct.
                         * @{
                         */
                        if (isSimAccount(mDestAccount)) {
                            if (mEvenPhoneNumber) {
                                type = String.valueOf(PHONE_TYPE);
                            } else {
                                type = String.valueOf(Phone.TYPE_FIXED_NUMBER);
                            }
                            mEvenPhoneNumber = !mEvenPhoneNumber;
                        } else {
                            if (type.equals(String
                                    .valueOf(Phone.TYPE_FIXED_NUMBER))) {
                                type = String.valueOf(PHONE_TYPE);
                            }
                        }
                        /**
                         * @}
                         */
                        value += mark + type;
                    }
                }
                if (mimeType.equals(Email.CONTENT_ITEM_TYPE)) {
                    if (isSimAccount(mDestAccount)) {
                        type = String.valueOf(EMAIL_TYPE);
                    } else {
                        type = cursor.getString(2);
                    }
                    if (type != null) {
                        value += mark + type;
                        if (type.equals("0") && cursor.getString(4) != null) {
                            value += mark + cursor.getString(4);
                        }
                    }
                }
                /* SPRD: Bug606604 During replication between accounts, some information has lost. @{ */
                if (mimeType.equals(Im.CONTENT_ITEM_TYPE)) {
                    type = cursor.getString(6);
                    if (type != null) {
                        value += mark + type;
                    }
                }
                if (mimeType.equals(Organization.CONTENT_ITEM_TYPE)) {
                    type = cursor.getString(5);
                    if (type != null) {
                        value += mark + type;
                    }
                }
                /* @{ */
                return value;
            }

            public Iterator<Map<String, List<String>>> iterator() {
                Iterator<Map<String, List<String>>> ret = new Iterator<Map<String, List<String>>>() {
                    public boolean hasNext() {
                        Set<String> mimeTypes = mEntities.keySet();
                        for (String mimeType : mimeTypes) {
                            Entity entity = mEntities.get(mimeType);
                            if (entity.onProcess()) {
                                return true;
                            }
                        }
                        return false;
                    }

                    public Map<String, List<String>> next() {
                        Map<String, List<String>> ret = new HashMap<String, List<String>>();
                        Set<String> mimeTypes = mEntities.keySet();
                        for (String mimeType : mimeTypes) {
                            Entity entity = mEntities.get(mimeType);

                            int current = entity.mCurrent;
                            int step = entity.mStep;
                            List<String> datas = entity.mDatas;
                            int max = datas.size();

                            if (mimeType.equals(StructuredName.CONTENT_ITEM_TYPE)) {
                                // always put StructuredName
                                ret.put(mimeType, datas);
                                entity.mCurrent = max;
                                continue;
                            }

                            if (current == max) {
                                continue;
                            }

                            if (step <= 0) {
                                // unlimited
                                ret.put(mimeType, datas);
                                entity.mCurrent = max;
                                continue;
                            }

                            if (current + step < max) {
                                entity.mCurrent = current + step;
                                ret.put(mimeType, datas.subList(current, current + step));
                                continue;
                            } else {
                                entity.mCurrent = max;
                                ret.put(mimeType, datas.subList(current, max));
                            }
                        }
                        return ret;
                    }

                    public void remove() {

                    }
                };
                return ret;
            }
        }
    }

    class BatchStarredTask extends AsyncTask<Void, Integer, Integer> {
        private NotificationManager mNotificationManager;
        private int mMax;
        private ArrayList<String> mLookupKeys;
        private Resources mResources;
        private Context mContext;
        private String mTag;
        private final String BATCH_OPERATION_FINISH_TAG = "BatchOperationFinished";
        private PendingIntent mCancelPendingIntent;
        private NotificationCompat.Builder mBuilder;

        private static final int sTransactionSize = 100;

        public BatchStarredTask(Context context, ArrayList<String> lookupKeys, String tag) {
            mContext = context;
            mLookupKeys = lookupKeys;
            mMax = lookupKeys.size();
            mTag = tag;
            Intent intent = new Intent(mContext, CancelBatchOperationActivity.class);
            intent.putExtra(BatchOperationService.KEY_NOTIFICATION_TAG, tag);
            intent.putExtra(BatchOperationService.KEY_MODE,
                    BatchOperationService.MODE_START_BATCH_STARRED);
            mCancelPendingIntent = PendingIntent.getActivity(mContext, tag.hashCode(), intent,
                    PendingIntent.FLAG_UPDATE_CURRENT);
            mNotificationManager = (NotificationManager) mContext
                    .getSystemService(Context.NOTIFICATION_SERVICE);
            mBuilder = new NotificationCompat.Builder(mContext);
            mResources = mContext.getResources();
        }

        public void onPreExecute() {
            String title = mResources.getString(R.string.working_batch_starred);
            mNotificationManager.notify(mTag, 0, mBuilder.setAutoCancel(false)
                    .setContentTitle(title)
                    .setTicker(title)
                    .setSmallIcon(android.R.drawable.stat_sys_warning)
                    .setProgress(mMax, 0, false)
                    .setContentIntent(mCancelPendingIntent)
                    .setOngoing(true)
                    .build());

            try {
                mPowerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
                mWakeLock = mPowerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "appBackup");
                mWakeLock.acquire();
            } catch (SecurityException e) {
                Log.w(TAG, "No permission to acquire wake lock");
                e.printStackTrace();
                mWakeLock = null;
            }
        }

        public Integer doInBackground(Void... v) {
            /*
             * SPRD: Bug 378691 when Import phone Contacts to SIM ,kill the process,the task will
             * stop.
             * @{
             */
//            mActivityManager.setSelfProtectStatus(ActivityManager.PROCESS_STATUS_MAINTAIN);//sprdporting
            /*
             * @}
             */
            ContentResolver resolver = mContext.getContentResolver();
            int progress = 0;
            ArrayList<String> lookupKeys = mLookupKeys;

            ArrayList<ContentProviderOperation> operations = new ArrayList<ContentProviderOperation>
                    ();
            int count = 0;
            for (String key : lookupKeys) {
                if (isCancelled()) {
                    return 0;
                }
                /*
                 * SPRD: Bug 353789 Deleted favorite contacts added by clicking the
                 * 'menu_add_favorite' item, add again in the same way, can't display in quick
                 * dialer.
                 * @{
                 */
                // Undemote the contact if necessary
                final Cursor c = resolver.query(
                        Uri.withAppendedPath(Contacts.CONTENT_LOOKUP_URI, key), new String[] {
                            Contacts._ID
                        },
                        null, null, null);
                try {
                    if (c != null) {
                        while (c.moveToNext()) {
                            final long id = c.getLong(0);
                            if (id < Profile.MIN_ID) {
                                PinnedPositions.undemote(resolver,id);
                            }
                        }
                    }
                } finally {
                    c.close();
                }
                /*
                 * @}
                 */
                operations.add(ContentProviderOperation
                        .newUpdate(Uri.withAppendedPath(Contacts.CONTENT_LOOKUP_URI, key))
                        .withYieldAllowed(false)
                        .withValue(Contacts.STARRED, true)
                        .build());
                progress++;
                if (operations.size() > sTransactionSize) {
                    try {
                        resolver.applyBatch(ContactsContract.AUTHORITY, operations);
                    } catch (Exception e) {
                        // throw new
                        // RuntimeException("Failed to delete contact", e);
                        Log.e(TAG, "Failed to delete contact:" + e);
                    }
                    operations.clear();
                    publishProgress(progress);
                }
            }

            try {
                resolver.applyBatch(ContactsContract.AUTHORITY, operations);
            } catch (Exception e) {
                // throw new RuntimeException("Failed to delete contact", e);
                /*
                 * SPRD: Bug 353789 Deleted favorite contacts added by clicking the
                 * 'menu_add_favorite' item, add again in the same way, can't display in quick
                 * dialer. Original Android code: Log.e(TAG, "Failed to delete contact:" + e);
                 * @{
                 */
                Log.e(TAG, "Failed to star contact:" + e);
                /*
                 * @}
                 */
            }
            return 0;
        }

        public void onProgressUpdate(Integer... progress) {
            String title = mResources.getString(R.string.working_batch_starred);
            mNotificationManager.notify(mTag, 0, mBuilder.setAutoCancel(false)
                    .setContentTitle(title)
                    .setContentText(title)
                    .setTicker(title)
                    .setSmallIcon(android.R.drawable.stat_sys_warning)
                    .setProgress(mMax, progress[0], false)
                    .setContentIntent(mCancelPendingIntent)
                    .setOngoing(true).build());
        }

        @Override
        public void onCancelled(Integer result) {
            Log.i(TAG, "BatchOperationService: onCancelled:" + mTag);
            String title = mResources.getString(R.string.batch_operation_canceled);
            mNotificationManager.notify(mTag, 0, mBuilder.setAutoCancel(true)
                    .setContentTitle(title)
                    .setContentText(title)
                    .setTicker(title)
                    .setOngoing(false)
                    .setProgress(0, 0, false)
                    .setSmallIcon(android.R.drawable.stat_sys_warning)
                    .setContentIntent(
                            PendingIntent.getActivity(mContext, mTag.hashCode(), new Intent(), 0))
                    .build());
            if (mWakeLock != null) {
                Log.i(TAG, "onPostExecute : release wake lock ");
                mWakeLock.release();
                mWakeLock = null;
            }
            /*
             * SPRD: Bug 378691 when Import phone Contacts to SIM ,kill the process,the task will
             * stop.
             * @{
             */
//            mActivityManager.setSelfProtectStatus(ActivityManager.PROCESS_STATUS_IDLE);//sprdporting
            /*
             * @}
             */
        }

        public void onPostExecute(Integer i) {
            String title = mResources.getString(R.string.star_done);
            mNotificationManager.notify(mTag, 0, mBuilder.setAutoCancel(true)
                    .setContentTitle(title)
                    .setContentText(title)
                    .setTicker(title)
                    .setOngoing(false)
                    .setProgress(0, 0, false)
                    .setSmallIcon(android.R.drawable.stat_sys_warning)
                    .setContentIntent(
                            PendingIntent.getActivity(mContext, mTag.hashCode(), new Intent(), 0))
                    .build());
            if (mWakeLock != null) {
                Log.i(TAG, "onPostExecute : release wake lock ");
                mWakeLock.release();
                mWakeLock = null;
            }
            mTasks.remove(mTag);
            /*
             * SPRD: Bug 378691 when Import phone Contacts to SIM ,kill the process,the task will
             * stop.
             * @{
             */
//            mActivityManager.setSelfProtectStatus(ActivityManager.PROCESS_STATUS_IDLE);//sprdporting
            /*
             * @}
             */
        }
    }

    public class CancelTaskReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            AsyncTask task = mTasks.get(mImportExportTag);
            Log.i(TAG, "BatchOperationService: cancel task: " + mImportExportTag);
            if (task != null
                    && (mDestAccount.type != null)
                    && (SimAccountType.ACCOUNT_TYPE.equals(mDestAccount.type) || USimAccountType.ACCOUNT_TYPE
                            .equals(mDestAccount.type))) {
                mTasks.remove(mImportExportTag);
                task.cancel(false);
            }
        }
    }
}
