package com.sprd.contacts.appbackup;

import java.io.OutputStreamWriter;
import java.io.BufferedWriter;
import java.io.File;
import java.io.Writer;
import java.io.OutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.WeakHashMap;

import android.content.Context;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Binder;
import android.os.PowerManager;
import android.app.Service;
import android.database.Cursor;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.RawContacts;

import com.android.contacts.R;
import com.android.vcard.VCardComposer;
import com.android.vcard.VCardConfig;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.android.vcard.VCardInterpreter;
import com.android.vcard.VCardEntryCommitter;
import com.android.vcard.VCardEntryConstructor;
import com.android.vcard.VCardEntry;
import com.android.vcard.VCardEntryCounter;
import com.android.vcard.VCardParser;
import com.android.vcard.VCardParser_V21;
import com.android.vcard.VCardParser_V30;
import com.android.vcard.exception.VCardException;
import com.android.vcard.exception.VCardVersionException;
import com.android.vcard.VCardEntryHandler;
import com.sprd.appbackup.service.IAppBackupRepository;
import com.sprd.appbackup.service.IAppBackupRestoreObserver;
import com.sprd.appbackup.service.AbstractAppBackupAgent;
import com.sprd.appbackup.service.Account;
import com.sprd.contacts.common.model.account.PhoneAccountType;

public class AppBackupService extends Service {

    private final static String TAG = "AppBackupService";
    private static final String UNIQUE_URI = "content://com.android.contacts/raw_contacts/unique";
    private static final String SUFFIX_VCF = ".vcf";
    private static final String NAME_CONTACT = "contact";
    static final String[] PROJECTION = new String[] {
            "_id"
    };
    public static final int MODE_BACKUP = 1;
    public static final int MODE_RESTORE = 2;
    public static final int MODE_UNIQUE = 3;

    public final static int FLAG_DUPLICATION_SUCCESS = 5;
    private final static String KEY_BACK_UP = "backup";
    private final static String KEY_RESTORE = "restore";
    private final static String KEY_UNIQUE = "unique";

    private int mCurrentCount = 0;
    private int mTotalCount = 0;
    private VCardParser mVCardParser;
    private int mVCardType;
    private AppBackupTask mTask;
    private Boolean isOldVersion = false;
    private Map<String, AsyncTask> mTasks = new WeakHashMap<String, AsyncTask>();

    public class MyBinder extends AbstractAppBackupAgent {
        public int onBackup(IAppBackupRepository repo, IAppBackupRestoreObserver observer,
                int categoryCode, List<Account> accounts) {
            Log.e(TAG, "onBackup");
            mTask = new AppBackupTask(AppBackupService.this, repo, observer, MODE_BACKUP,
                    accounts);
            mTasks.put(KEY_BACK_UP, mTask);
            mTask.execute();
            return 0;
        }

        public int onRestore(IAppBackupRepository repo, IAppBackupRestoreObserver observer) {
            Log.e(TAG, "onRestore");
            mTask = new AppBackupTask(AppBackupService.this, repo, observer, MODE_RESTORE, null);
            mTasks.put(KEY_RESTORE, mTask);
            mTask.execute();
            return 0;
        }

        public int onDeduplicate(IAppBackupRestoreObserver observer) {
            Log.e(TAG, "onDeduplicate");
            mTask = new AppBackupTask(AppBackupService.this, null, observer, MODE_UNIQUE, null);
            mTasks.put(KEY_UNIQUE, mTask);
            mTask.execute();
            return 0;
        }

        public int onCancel() {
            Log.e(TAG, "onCancel");
            if (mTask != null) {
                mTask.cancel(true);
                mTasks.clear();
            }
            if (mVCardParser != null) {
                mVCardParser.cancel();
            }
            return 0;
        }

        public String getBackupInfo(IAppBackupRepository repo) {
            return "hello from contacts";
        }

        public boolean isEnabled(int categoryCode) {
            Cursor cursor = AppBackupService.this.getContentResolver().query(
                    Contacts.CONTENT_URI, PROJECTION, null, null, null);
            boolean isEnable = (cursor != null && cursor.getCount() > 0);
            if (cursor != null) {
                cursor.close();
            }
            return isEnable;
        }

        public List<Account> getAccounts() {
            AccountTypeManager am = AccountTypeManager.getInstance(getApplicationContext());
            List<AccountWithDataSet> accountWithDataSets = am.getAccounts(false);
            List<Account> accounts = new ArrayList<Account>();
            int i = 0;
            for (AccountWithDataSet accountWithDataSet : accountWithDataSets) {
                Log.e(TAG, "AccountWithDataSet.name: " + accountWithDataSet.name);
                Log.e(TAG, "AccountWithDataSet.type: " + accountWithDataSet.type);
                Account account = new Account();
                account.setAccountId((i++) + "");
                account.setAccountType(accountWithDataSet.type);
                if (PhoneAccountType.ACCOUNT_TYPE.equals(accountWithDataSet.type)) {
                    account.setAccountName(AppBackupService.this
                            .getString(R.string.label_phone));
                } else {
                    account.setAccountName(accountWithDataSet.name);
                }
                accounts.add(account);
            }
            return accounts;
        }

        public AppBackupService getService() {
            return AppBackupService.this;
        }
    }

    @Override
    public Binder onBind(Intent intent) {
        Log.e(TAG, "contacts: onBind");
        return new MyBinder();
    }

    class AppBackupTask extends AsyncTask<Void, Integer, Integer> implements VCardEntryHandler {
        private ContentResolver mContentResolver;
        private IAppBackupRepository mRepo;
        private IAppBackupRestoreObserver mObserver;
        private Context mContext;
        private int mMode;
        private List<Account> mAccounts;
        private PowerManager.WakeLock mWakeLock;
        private PowerManager mPowerManager;

        public AppBackupTask(Context context, IAppBackupRepository repo,
                IAppBackupRestoreObserver observer, int mode, List<Account> accounts) {
            mContext = context;
            mRepo = repo;
            mObserver = observer;
            mContentResolver = mContext.getContentResolver();
            mMode = mode;
            mAccounts = accounts;
        }

        public void onPreExecute() {
            mPowerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
            mWakeLock = mPowerManager.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK |
                    PowerManager.ON_AFTER_RELEASE, TAG);
        }

        public Integer doInBackground(Void... v) {
            mWakeLock.acquire();
            try {
                switch (mMode) {
                    case MODE_BACKUP:
                        VCardComposer composer = null;
                        StringBuilder selection = new StringBuilder();
                        List<String> selectionArgs = new ArrayList<String>();
                        Writer write = null;
                        int current = 0;
                        if (mAccounts == null || mAccounts.size() == 0) {
                            return -1;
                        }
                        try {
                            /*SPRD: Bug604239 the backup app and the Contacts' vcard version should keep same*/
                            composer = new VCardComposer(AppBackupService.this,
                                    VCardConfig.VCARD_TYPE_V21_GENERIC, true);
                            selection.append(
                                    " EXISTS ("
                                            + "SELECT DISTINCT " + RawContacts.CONTACT_ID
                                            + " FROM view_raw_contacts WHERE ( "
                                            + "view_contacts." + Contacts._ID + "="
                                            + RawContacts.CONTACT_ID + " AND (");
                            boolean init = true;
                            for (Account account : mAccounts) {
                                Log.e(TAG, "account.isChecked(): " + account.isChecked());
                                if (!init) {
                                    selection.append(" OR ");
                                }
                                init = false;
                                selection.append("(" + RawContacts.ACCOUNT_TYPE + "=?");
                                selection.append(" AND ");
                                selection.append(RawContacts.ACCOUNT_NAME + "=?)");
                                /**
                                 * SPRD: Bug599779 these read only contacts can not
                                 * be backuped
                                 * 
                                 * @{
                                 */
                                selection.append(" AND ("
                                        + RawContacts.RAW_CONTACT_IS_READ_ONLY
                                        + "!=1 )");
                                /**
                                 * @}
                                 */
                                Log.e(TAG, "account.getAccountType(): " + account.getAccountType());
                                Log.e(TAG, "account.getAccountName(): " + account.getAccountName());
                                selectionArgs.add(account.getAccountType());
                                if (PhoneAccountType.ACCOUNT_TYPE.equals(account.getAccountType())) {
                                    selectionArgs.add(AccountTypeManager.getInstance(mContext)
                                            .getPhoneAccount().name);
                                } else {
                                    selectionArgs.add(account.getAccountName());
                                }
                            }
                            selection.append(")))");
                            if (!composer.init(selection.toString(),
                                    selectionArgs.toArray(new String[selectionArgs.size()]))) {
                                return -2;
                            }
                            ParcelFileDescriptor fd = mRepo.write("contact.vcf");
                            if (fd == null) {
                                return -1;
                            }
                            OutputStream out = new ParcelFileDescriptor.AutoCloseOutputStream(fd);
                            write = new BufferedWriter(new OutputStreamWriter(out));
                            while (!composer.isAfterLast()) {
                                if (isCancelled()) {
                                    return -1;
                                }
                                // write a vcard to disk;
                                write.write(composer.createOneEntry());
                                current++;
                                Log.e(TAG, "backup.current: " + current);
                                if (mObserver != null) {
                                    Log.e(TAG, "backup.composer.getCount(): " + composer.getCount());
                                    mObserver.onUpdate(current, composer.getCount());
                                }
                            }
                        } catch (OutOfMemoryError e) {
                            Log.e(TAG, "OutOfMemoryError thrown during backup", e);
                            return -1;
                        } catch (Exception e) {
                            final String errorReason = composer.getErrorReason();
                            Log.e(TAG, "Failed to read a contact: " + errorReason);
                            Log.e(TAG, "Failed to read a contact: " + e.toString());
                            return -1;
                        } finally {
                            if (composer != null) {
                                composer.terminate();
                                try {
                                    if (write != null)
                                        write.close();
                                } catch (Exception e) {
                                    e.printStackTrace();
                                }
                            }
                        }
                        break;
                    case MODE_RESTORE:
                        mTotalCount = 0;
                        boolean successful = false;
                        AccountWithDataSet account;
                        int size = 0;
                        String contactsDirPath = null;
                        List<String> namesList = null;
                        try {
                            isOldVersion = mRepo.isOldVersionFile();
                            Log.e(TAG, "isOldVersion: " + isOldVersion);
                            if (isOldVersion) {
                                contactsDirPath = mRepo.getOldVersionFilePath();
                                Log.e(TAG, "contactsDirPath: " + contactsDirPath);
                                File contactsFile = new File(contactsDirPath);
                                namesList = getChildFileNames(contactsFile);
                                if (null == namesList) {
                                    Log.d(TAG, "no contacts vcf files, restore fails.");
                                    return -1;
                                }
                                InputStream is = null;
                                for (String name : namesList) {
                                    Log.e(TAG, "name: " + name);
                                    Uri uri = Uri.parse("file://" + contactsDirPath
                                            + File.separator
                                            + name);
                                    is = mContentResolver.openInputStream(uri);
                                    mTotalCount += getTotalEntryCount(is);
                                    is.close();
                                    size++;
                                }
                            } else {
                                size = 1;
                                ParcelFileDescriptor fd = mRepo.read("contact.vcf");
                                if (fd == null) {
                                    return -1;
                                }
                                InputStream is = new ParcelFileDescriptor.AutoCloseInputStream(fd);
                                mTotalCount = getTotalEntryCount(is);
                                is.close();
                            }
                            if (mTotalCount == -1 || mTotalCount == 0) {
                                return -1;
                            }
                            Log.e(TAG, "mTotalCount: " + mTotalCount);
                            InputStream is = null;
                            for (int i = 0; i < size; i++) {
                                if (!isOldVersion) {
                                    ParcelFileDescriptor fd = mRepo.read("contact.vcf");
                                    if (fd == null) {
                                        return -1;
                                    }
                                    is = new ParcelFileDescriptor.AutoCloseInputStream(
                                            fd);
                                } else {
                                    Uri uri = Uri.parse("file://" + contactsDirPath
                                            + File.separator
                                            + namesList.get(i));
                                    is = mContentResolver.openInputStream(uri);
                                }
                                account = AccountTypeManager.getInstance(AppBackupService.this)
                                        .getPhoneAccount();
                                final VCardEntryConstructor constructor = new VCardEntryConstructor(
                                        mVCardType,
                                        (account != null ? account.getAccountOrNull() : null), null);
                                final VCardEntryCommitter committer = new VCardEntryCommitter(
                                        mContentResolver);
                                constructor.addEntryHandler(committer);
                                constructor.addEntryHandler(this);
                                successful = readOneVCard(is, mVCardType, constructor);
                                is.close();
                            }
                        } catch (Exception e) {
                            Log.e(TAG, "onRestore--Exception.e: " + e.toString());
                            return -1;
                        } finally {
                            mCurrentCount = 0;
                        }
                        break;
                    case MODE_UNIQUE:
                        Log.e(TAG, "MODE_UNIQUE");
                        mContentResolver.update(
                                Uri.parse(UNIQUE_URI), new ContentValues(), null, null);
                        return FLAG_DUPLICATION_SUCCESS;
                    default:
                        Log.i(TAG, "AppBackupTask: unknown mode:" + mMode);
                        break;
                }
            } finally {
                mWakeLock.release();
            }
            return 0;
        }

        public void onPostExecute(Integer i) {
            mTasks.clear();
            try {
                if (mObserver != null) {
                    mObserver.onResult(i);
                    if (i != 0) {
                        mObserver.onUpdate(-1, -1);
                    }
                }
            } catch (Exception e) {
                // TODO: handle exception
            }
        }

        private int getTotalEntryCount(InputStream is) {
            int count = 0;
            try {
                InputStreamReader inreader = new InputStreamReader(is);
                BufferedReader buffreader = new BufferedReader(inreader);
                String line = null;
                while ((line = buffreader.readLine()) != null) {
                    if (line.contains("END:VCARD")) {
                        ++count;
                    }
                    if (line.contains("VERSION")) {
                        mVCardType = line.contains("VERSION:3.0") ? VCardConfig.VCARD_TYPE_V30_GENERIC
                                : VCardConfig.VCARD_TYPE_V21_GENERIC;
                    }
                }
            } catch (IOException e) {
            } catch (Exception e) {
            }
            return count;
        }

        private boolean readOneVCard(InputStream is, int vcardType,
                final VCardInterpreter interpreter) {
            Log.e(TAG, "readOneVCard--vcardType: " + vcardType);
            boolean successful = false;
            mVCardParser = (vcardType == VCardConfig.VCARD_TYPE_V30_GENERIC ?
                    new VCardParser_V30(vcardType) :
                    new VCardParser_V21(vcardType));

            try {
                if ((interpreter instanceof VCardEntryConstructor)) {
                    // Let the object clean up internal temporary objects,
                    ((VCardEntryConstructor) interpreter).clear();
                }
                if (isCancelled()) {
                    Log.i(TAG, "AppBackupTask already recieves cancel request, so " +
                            "send cancel request to vCard parser too.");
                    mVCardParser.cancel();
                }
                mVCardParser.parse(is, interpreter);
                successful = true;
            } catch (VCardVersionException e) {
                Log.e(TAG, "VCardVersionException was emitted: " + e.getMessage());
            } catch (IOException e1) {
                Log.e(TAG, "IOException was emitted: " + e1.getMessage());
            } catch (VCardException e2) {
                Log.e(TAG, "VCardException.e: " + e2.toString());
            }
            return successful;
        }

        @Override
        public void onStart() {
            // do nothing
        }

        @Override
        public void onEnd() {
            // do nothing
        }

        @Override
        public void onEntryCreated(VCardEntry entry) {
            mCurrentCount++;
            Log.e(TAG, "restore.mCurrentCount: " + mCurrentCount);
            try {
                if (mObserver != null) {
                    mObserver.onUpdate(mCurrentCount, mTotalCount);
                }
            } catch (Exception e) {
                // TODO: handle exception
            }
        }

        protected List<String> getChildFileNames(File file) {
            if (file != null && file.exists() && file.isDirectory()) {
                String[] names = file.list();
                if (names == null) {
                    Log.d(TAG, "no contacts data to restore");
                    return null;
                }
                List<String> nameList = new ArrayList<String>(names.length);
                for (String name : names) {
                    if (name.startsWith(NAME_CONTACT) && name.endsWith(SUFFIX_VCF)) {
                        nameList.add(name);
                    }
                }
                return nameList;
            }
            return null;
        }
    }

    public boolean isRunning() {
        return mTasks.size() > 0;
    }
}