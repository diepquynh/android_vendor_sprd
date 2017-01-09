package com.sprd.appbackup.activities;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;
import java.util.TreeMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import android.app.ActionBar;
import android.app.ActionBar.Tab;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnKeyListener;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.app.PackageManagerEx;
import android.content.res.Resources;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Debug;
import android.os.FileObserver;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.RemoteException;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TabHost;
import android.widget.TabHost.TabSpec;
import android.widget.TextView;
import android.widget.Toast;
import android.support.v4.view.ViewPager;
import android.support.v4.app.NotificationCompat;

import java.util.concurrent.RejectedExecutionException;

import com.sprd.appbackup.AppBackupObserver;
import com.sprd.appbackup.AppBackupRestoreObserver;
import com.sprd.appbackup.AppInfo;
import com.sprd.appbackup.BackupFragment;
import com.sprd.appbackup.DataItem;
import com.sprd.appbackup.R;
import com.sprd.appbackup.RestoreFragment;
import com.sprd.appbackup.SprdAppBackupApplication;
import com.sprd.appbackup.TabsAdapter;
import com.sprd.appbackup.appbackup.LoadAppTask;
import com.sprd.appbackup.appbackup.PackageInstall;
import com.sprd.appbackup.appbackup.Utils;
import com.sprd.appbackup.service.Agent;
import com.sprd.appbackup.service.AppBackupManagerService;
import com.sprd.appbackup.service.Archive;
import com.sprd.appbackup.service.Category;
import com.sprd.appbackup.service.Config;
import com.sprd.appbackup.service.IAppBackupManager;
import com.sprd.appbackup.service.IScanAgentAndArchiveListener;
import com.sprd.appbackup.utils.DataUtil;
import com.sprd.appbackup.utils.FileUtils;
import com.sprd.appbackup.utils.StorageUtil;

import android.content.SharedPreferences;

import com.sprd.appbackup.utils.TarUtils;

import android.app.AlertDialog;
import android.widget.CheckBox;
import android.provider.MediaStore;
import android.database.Cursor;
import android.net.Uri;
import android.content.ContentResolver;
import android.provider.MediaStore.MediaColumns;

import com.android.internal.telephony.TelephonyIntents;

import android.app.Dialog;
import android.Manifest.permission;

public class MainActivity extends Activity implements StorageUtil.StorageChangedListener {
    /* SPRD: 476980 request storage permmison  @{ */
    public final static int PERMISSION_REQUEST_CODE = 0;
    public final static int MISS_PERMISSIONS_ALERT_DIALOG = 0;
    private Dialog mDialog = null;
    /* @} */

    private final static String TAG = "AppBackupActivity";
    private final static String TAG_NOTIFICATION = "AppBackupActivityNotification";
    private final static String CALENDAR_AGENT_NAME = "Calendar";
    private final static String MMS_AGENT_NAME = "Mms";
    private final static String GALLERY_AGENT_NAME = "Gallery";
    private final static String CONTACTS_AGENT_NAME = "Contact";

    private final static boolean DBG = true;
    private final static int CANCEL_FLAG = -2;
    private final static int FLAG_FAIL = -1;
    private final static int FLAG_SUCCESS = 0;
    private final static int FLAG_NODATA = -2;
    private final static int UPDATE_INTERVAL = 500;
    private final static int TAB_BACKUP = 0;
    private final static int TAB_RESTORE = 1;
    private IAppBackupManager mService;
    private List<DataItem> mDataList;
    private List<AppInfo> mAppList = new ArrayList<AppInfo>();;
    private Map<String, List<DataItem>> mRestoreDataDetail = new TreeMap<String, List<DataItem>>(
            new Comparator<String>() {
                @Override
                public int compare(String lhs, String rhs) {
                    return rhs.compareTo(lhs);
                }
            });
    private List<AppInfo> mRestoreApp = new ArrayList<AppInfo>();
    private LoadAppTask mLoadAppTask;
    private FileObserver mOldDataInternalPathObserver;
    private FileObserver mOldDataExternalPathObserver;
    private FileObserver mExternalFileObserver;
    private FileObserver mExternalApkObserver;
    private FileObserver mInternalFileObserver;
    private FileObserver mInternalApkObserver;

    public static List<String> mRestoreSelectedFile = new ArrayList<String>();

    private Agent[] mAgents;
    private ArrayList<Archive> mArchivesList = new ArrayList<Archive>();
    private ProgressDialog mPrgsDialog;
    private ProgressDialog mProgressBar;
    private final int MSG_DISPLAY_BACKUP_RESULT = 1;
    private final int MSG_UPDATE_APP_LISTVIEW = 2;
    private final int MSG_DATA_FILE_CHANGED = 3;
    private final int MSG_UPDATE_RESTORE_LIST = 4;
    private final int MSG_APK_FILE_CHANGED = 5;
    private final int MSG_DATA_AND_APP_LOAD_FINISH = 6;
    private final int MSG_UPDATE_AGENT = 8;
    private final int MSG_UPDATE_RESTORE_APP = 10;
    private final int MSG_LACK_OF_SPACE = 9;
    private final int MSG_UPDATE_PROGRESS = 11;
    private final int MSG_CLOSE_PROGRESS_DIALOG = 12;
    private final int MSG_DISPLAY_RESTORE_RESULT = 13;
    private final int MSG_SET_BACKUP_RESTORE_BUTTON = 14;
    /* SPRD: Bug 420899 No cancel when backup or restore. @{ */
    private final int MSG_DISMISS_PROGRESSDIALOG = 15;
    /* @} */
    private final int BACKUP_INTERVALL = 4;
    private final int MAX_BACKUP_INTERVALL_NUM = 200;

    private ArrayList<Map<String, Object>> mDuplicationResault = new ArrayList<Map<String, Object>>();
    private static final String AGENT_NAME = "agent_name";
    private static final String CATEGORY_CODE = "category_code";
    private static final String RESULT = "resultCode";

    private static final String REMINDUSER_FIRST = "reminder_entry_first";
    private static final String ZIP_TYPE_HEADER = "504b0304";
    private final int ZIP_TYPE_HEADER_SIZE = 10;

    private static final String REMINDUSER_NO_SD = "reminder_no_SD";

    private static boolean mCancel = false;
    public static boolean mScanAllCancel = false;
    private WakeLock mWakeLock;
    private NotificationManager mNotifyManager;
    private PendingIntent mPendingIntent;
    private boolean mActivate = true;
    private boolean mIsFirstResume = true;
    private boolean mSdCardUnmounted = false;
    private boolean mIsRestoring = false;
    private boolean mAppIsBackuping = false;
    private static boolean mInProgress = false;

    /* Enables horizontal swipe between Fragments*/
    private ViewPager mViewPager;
    private TabsAdapter mTabsAdapter;
    private BackupFragment mBackupFragment;
    private RestoreFragment mRestoreFragment;
    private ExecutorService mExec = Executors.newSingleThreadExecutor();
    private ExecutorService mExecFileThread = Executors.newSingleThreadExecutor();
    private ExecutorService mExecAppThread = Executors.newSingleThreadExecutor();
    private ExecutorService mExecUserSelectThread = Executors.newSingleThreadExecutor();
    public ExecutorService mExecScanRestoreThread = Executors.newSingleThreadExecutor();
    private AppBackupObserver mAppBackupObserver;
    private Timer mTimer;
    private TimerTask mTask;
    private AppBackupRetainData mAppBackupRetainData = new AppBackupRetainData();
    private String mInPorgressAgentName;

    private PackageManagerEx mPm = null;

    private SprdAppBackupApplication mApp;
    /*  SPRD: Bug #458089 Account list haven't been updated after sim card removed @{ */
    private Resources r = Resources.getSystem();

    String simState =  r.getString(com.android.internal.R.string.sim_added_title)+"/"
                    +r.getString(com.android.internal.R.string.sim_removed_title);
    private BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver(){
        @Override
        public void onReceive(Context context,Intent intent){
            if(intent.getAction().equals(TelephonyIntents.ACTION_SIM_STATE_CHANGED)){
                /* SPRD: 460898 modify to dismiss dialog @{ */
                if(mBackupFragment != null){
                    mBackupFragment.dismissDialogInAdapter();
                }
                /* @} */
            }
        }
    };
    /* @} */
    public Handler mHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
            case MSG_DATA_AND_APP_LOAD_FINISH:
                if(DBG) {
                    Log.d(TAG, "mHandler MSG_DATA_AND_APP_LOAD_FINISH"+"\n"
                            +"mAppList = "+mAppList+"\n"
                            +"mRestoreApp = "+mRestoreApp + "\n"
                            + "mRestoreDataDetail = " + mRestoreDataDetail);
                }
                if(mBackupFragment != null){
                    mBackupFragment.updateAppList(mAppList);
                    mBackupFragment.setBackupButtonEnable(!mInProgress);
                }
                if(mRestoreFragment != null){
                    Log.d(TAG, "mHandler MSG_DATA_AND_APP_LOAD_FINISH updateRestoreApkList"+"\n"
                            +"mRestoreApp = "+mRestoreApp);
                    mRestoreFragment.updateRestoreApkList(mRestoreApp);
                    mRestoreFragment.updateRestoreDataList(mRestoreDataDetail);
                    mRestoreFragment.setRestoreButtonEnable(!mInProgress);
                }
                if(DBG) Log.d(TAG, "time_cal MSG_DATA_AND_APP_LOAD_FINISH end");
                closeProgressDialog();
                break;
            case MSG_DISPLAY_BACKUP_RESULT:
                ArrayList<Map<String, Object>> backupResult = null;
                if (msg.obj != null) {
                    backupResult = (ArrayList<Map<String, Object>>)msg.obj;
                }
                displayBackupResult(backupResult);
                /* SPRD:607439,check ProgressDialog state. @{ */
                closeProgressDialog();
                break;
            case MSG_DISPLAY_RESTORE_RESULT:
                MessageForRestoreResult resultMsg = null;
                if (msg.obj != null && (msg.obj instanceof MessageForRestoreResult)) {
                    resultMsg = (MessageForRestoreResult)msg.obj;
                }
                if (resultMsg == null) {
                    Log.i(TAG, "resultMsg == null");
                    break;
                }
                displayRestoreResult(resultMsg.dataList, resultMsg.restoreResult);
                break;
            case MSG_UPDATE_APP_LISTVIEW:
                if (DBG) {Log.d(TAG, "mHandler MSG_UPDATE_APP_LISTVIEW mAppList = "+ mAppList);}
                if(mBackupFragment != null){
                    mBackupFragment.updateAppList(mAppList);
                }
                break;
            case MSG_DATA_FILE_CHANGED:
                if (DBG) {Log.d(TAG, "MSG_DATA_FILE_CHANGED handleMessage");}
                try{
                    if(mExecFileThread != null && !mExecFileThread.isShutdown()){
                        mExecFileThread.execute(new ScanBackupFileThread());
                    }
                }catch(RejectedExecutionException e){
                    e.printStackTrace();
                }
                break;
            case MSG_UPDATE_RESTORE_LIST:
                if (DBG) {Log.d(TAG, "MSG_UPDATE_RESTORE_LIST handleMessage");}
                if(mRestoreFragment != null){
                    mRestoreFragment.updateRestoreDataList(mRestoreDataDetail);
                }
                break;
            case MSG_APK_FILE_CHANGED:
                if (DBG) {
                    Log.d(TAG, "mHandler MSG_APK_FILE_CHANGED");
                    Log.d(TAG, "mAppIsBackuping = " + mAppIsBackuping);
                }
                if (!mAppIsBackuping) {
                    try {
                        if(mExecAppThread != null && !mExecAppThread.isShutdown()){
                            mExecAppThread.execute(new LoadRestoreAppThread());
                        }
                    } catch (RejectedExecutionException e) {
                        e.printStackTrace();
                    }
                }
                break;
            case MSG_UPDATE_AGENT:
                if(DBG) {Log.d(TAG, "mHandler MSG_UPDATE_AGENT mDataList = "+ mDataList);}
                if(mBackupFragment != null && mDataList != null){
                    mBackupFragment.updateAgentList(mDataList);
                }
                break;
            case AppBackupObserver.CONTENT_CHANGE:
                if (DBG) {
                    Log.d(TAG, "mHandler AppBackupObserver.CONTENT_CHANGE");
                    Log.d(TAG, "mIsRestoring = " + mIsRestoring);
                }
                if(!mIsRestoring){
                    try{
                        if(mExec != null && !mExec.isShutdown()){
                            mExec.execute(new ScanAgentThread());
                        }
                    }catch(RejectedExecutionException e){
                        e.printStackTrace();
                    }
                }
                break;
            case MSG_UPDATE_RESTORE_APP:
                if(DBG) {Log.d(TAG, "mHandler MSG_UPDATE_RESTORE_APP");}
                if(mRestoreFragment != null){
                    mRestoreFragment.updateRestoreApkList(mRestoreApp);
                }
                break;
            case MSG_LACK_OF_SPACE:
                if(DBG) {Log.d(TAG, "mHandler MSG_LACK_OF_SPACE");}
                if(mActivate){
                    Toast.makeText(MainActivity.this, R.string.lock_of_freespace, Toast.LENGTH_LONG).show();
                }
                cancelTimerTask();
                break;
            case MSG_UPDATE_PROGRESS:
                /* SPRD:437437,If dialog cancel,when receive MSG_UPDATE_PROGRESS,return directly. @{ */
                if (mCancel) {
                    break;
                }
                /* @} */
                MessageForProgressUpdate progressMsg = null;
                if (msg.obj != null && (msg.obj instanceof MessageForProgressUpdate)) {
                    progressMsg = (MessageForProgressUpdate)msg.obj;
                }
                if (progressMsg == null) {
                    break;
                }
                if (mProgressBar == null) {
                    createProgressDlg();
                }
                if (mProgressBar != null) {
                    mProgressBar.setProgressNumberFormat(progressMsg.numFormat);
                    if (progressMsg.title != null) {
                        mProgressBar.setTitle(progressMsg.title);
                    }
                    mProgressBar.setProgress(progressMsg.progress);
                    mProgressBar.setMax(progressMsg.max);
                    if (!mProgressBar.isShowing()) {
                        mProgressBar.show();
                        mProgressBar.setProgress(progressMsg.progress);
                    }
                } else {
                    Log.i(TAG, "mProgressBar == null");
                }
                break;
            case MSG_CLOSE_PROGRESS_DIALOG:
                closeProgressDlg();
                break;
            case MSG_SET_BACKUP_RESTORE_BUTTON:
                if (mBackupFragment != null) {
                    mBackupFragment.setBackupButtonEnable(msg.arg1 == 1);
                }
                if (mRestoreFragment != null) {
                    mRestoreFragment.setRestoreButtonEnable(msg.arg1 == 1);
                }
                break;
                /* SPRD: Bug 420899 No cancel when backup or restore. @{ */
            case MSG_DISMISS_PROGRESSDIALOG:
                if (mPrgsDialog != null && mPrgsDialog.isShowing()) {
                    mPrgsDialog.dismiss();
                    mPrgsDialog = null;
                }
                break;
                /* @} */
            default:
                break;
            }
        }
    };

    private void closeProgressDialog() {
        if (mPrgsDialog == null) {
            return;
        }
        if (mPrgsDialog.isShowing()) {
            mPrgsDialog.dismiss();
        }
        mPrgsDialog = null;
    }

    public boolean isBackupInProgress() {
        return mInProgress;
    }

    class ScanBackupFileThread implements Runnable{

        @Override
        public void run() {
            mRestoreDataDetail = getArchive();
            mHandler.sendEmptyMessage(MSG_UPDATE_RESTORE_LIST);
        }
    }

    class ScanUserSelectBackupThread implements Runnable{
        @Override
        public void run() {
            if(DBG) Log.d(TAG, "time_cal mLoadAppTask.loadRestoredAppinfos start");
            mRestoreApp = mLoadAppTask.loadRestoredAppinfos();
            if(DBG) Log.d(TAG, "time_cal mLoadAppTask.loadRestoredAppinfos end");
            mDataList = getEnabledAgent();
            if(DBG) Log.d(TAG, "time_cal getArchive start");
            mRestoreDataDetail = getArchive();
            if(DBG) Log.d(TAG, "time_cal getArchive end");
            mHandler.sendEmptyMessage(MSG_DATA_AND_APP_LOAD_FINISH);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if(DBG) {Log.d(TAG, "onCreate()");}
        /*  SPRD: Bug #458089 Account list haven't been updated after sim card removed  @{ */
        IntentFilter intentFilter = new IntentFilter(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        registerReceiver(mBroadcastReceiver,intentFilter);
        /* @} */

        remindUserFirstEntry();

        mPrgsDialog = new ProgressDialog(this);
        mPrgsDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        mPrgsDialog.setMessage(getString(R.string.please_holding));
        mPrgsDialog.setCanceledOnTouchOutside(false);
        mPrgsDialog.setCancelable(false);
        mPrgsDialog.show();

        mRestoreSelectedFile.clear();
        int progress = 0;
        int max = 0;
        String title = null;
        String numFormat = null;
        Object obj = getLastNonConfigurationInstance();
        if (obj != null && (obj instanceof AppBackupRetainData)) {
            mAppBackupRetainData = (AppBackupRetainData)obj;
            if (mAppBackupRetainData.backupAppTask != null) {
                mAppBackupRetainData.backupAppTask.setTaskHandler(mHandler);
                progress = mAppBackupRetainData.backupAppTask.getProgress();
                max = mAppBackupRetainData.backupAppTask.getMax();
                title = mAppBackupRetainData.backupAppTask.getTitle();
                numFormat = mAppBackupRetainData.backupAppTask.getNumFormat();
                mInProgress = true;
            } else if (mAppBackupRetainData.backupDataTask != null) {
                mAppBackupRetainData.backupDataTask.setTaskHandler(mHandler);
                progress = mAppBackupRetainData.backupDataTask.getProgress();
                max = mAppBackupRetainData.backupDataTask.getMax();
                title = mAppBackupRetainData.backupDataTask.getTitle();
                numFormat = mAppBackupRetainData.backupDataTask.getNumFormat();
                mInProgress = true;
            } else if (mAppBackupRetainData.restoreAppTask != null) {
                mAppBackupRetainData.restoreAppTask.setTaskHandler(mHandler);
                progress = mAppBackupRetainData.restoreAppTask.getProgress();
                max = mAppBackupRetainData.restoreAppTask.getMax();
                title = mAppBackupRetainData.restoreAppTask.getTitle();
                numFormat = mAppBackupRetainData.restoreAppTask.getNumFormat();
                mInProgress = true;
            } else if (mAppBackupRetainData.restoreDataTask != null) {
                mAppBackupRetainData.restoreDataTask.setTaskHandler(mHandler);
                progress = mAppBackupRetainData.restoreDataTask.getProgress();
                max = mAppBackupRetainData.restoreDataTask.getMax();
                title = mAppBackupRetainData.restoreDataTask.getTitle();
                numFormat = mAppBackupRetainData.restoreDataTask.getNumFormat();
                mInProgress = true;
            }
        }
        if (mInProgress) {
            closeProgressDlg();
            createProgressDlg();
            if (mProgressBar != null) {
                mProgressBar.setTitle(title);
                mProgressBar.setProgressNumberFormat(numFormat);
                mProgressBar.setProgress(progress);
                mProgressBar.setMax(max);
                if (!mProgressBar.isShowing()) {
                    mProgressBar.show();
                    mProgressBar.setProgress(progress);
                }
            }
        }
        initLayout();
        mAppBackupObserver = new AppBackupObserver(this, mHandler);
        mAppBackupObserver.registerObservers();
        startListenPackageManager();
        buildInternalFileObserver();
        buildExternalFileObserver();
        getNotificationManager();
        mPendingIntent = getPendingIntent();
        initService();
        StorageUtil.setStorageChangeListener(this);
        mPm = (PackageManagerEx) getPackageManager();
        mApp = (SprdAppBackupApplication) getApplication();
    }

    @Override
    public Object onRetainNonConfigurationInstance() {
        // TODO Auto-generated method stub
        if (!mInProgress || mAppBackupRetainData == null){
            return null;
        }
        if (mAppBackupRetainData.backupAppTask != null
                || mAppBackupRetainData.backupDataTask != null
                || mAppBackupRetainData.restoreAppTask != null
                || mAppBackupRetainData.restoreDataTask != null) {
            return mAppBackupRetainData;
        }
        return super.onRetainNonConfigurationInstance();
    }

    private AlertDialog mSDcardWarningDlg;
    @Override
    public void onStorageChanged(File path, boolean available) {
        if(DBG) {Log.d(TAG, "available = "+available+ ", path = "+path.getAbsolutePath());}
        if(available){
            if(path.equals(StorageUtil.getExternalStorage()) && StorageUtil.getExternalStorageState()){
                mSdCardUnmounted = false;
                buildExternalFileObserver();
                if(mSDcardWarningDlg != null){
                    mSDcardWarningDlg.dismiss();
                    mSDcardWarningDlg = null;
                }
                if(DBG) {Log.d(TAG, "onStorageChanged mHandler.removeMessages(MSG_APK_FILE_CHANGED);");}
                mHandler.removeMessages(MSG_APK_FILE_CHANGED);
                mHandler.sendEmptyMessageDelayed(MSG_APK_FILE_CHANGED, 1000);

                //fix bug360389,when plugout usb,restore data list not update
                updateRestoreFragment();
            }
        }else{
            if(path.equals(StorageUtil.getExternalStorage())){
                mSdCardUnmounted = true;
                if (mExternalFileObserver != null) {
                    mExternalFileObserver.stopWatching();
                }
                if (mExternalApkObserver != null) {
                    mExternalApkObserver.stopWatching();
                }
                if (mOldDataExternalPathObserver != null) {
                    mOldDataExternalPathObserver.stopWatching();
                }
                if(mSDcardWarningDlg == null && mActivate){
                    mSDcardWarningDlg = new AlertDialog.Builder(this).setTitle(R.string.hint)
                            .setMessage(R.string.sdcard_is_not_available)
                            .setPositiveButton(R.string.confirm, new OnClickListener() {

                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                }
                            }).setCancelable(false).create();
                }
                if(mSDcardWarningDlg != null){
                    mSDcardWarningDlg.show();
                }

                closeProgressDlg();
                cancelTimerTask();
                if(DBG) {Log.d(TAG, "onStorageChanged mInProgress = "+mInProgress+ ", path = "+path.getAbsolutePath());}
                if (!mInProgress) {
                    Message message = new Message();
                    message.arg1 = 0;
                    message.what = MSG_SET_BACKUP_RESTORE_BUTTON;
                    mHandler.sendMessage(message);
                }

                if(mRestoreFragment != null){
                    if (mRestoreApp != null) {
                        ArrayList<AppInfo> removeApp = new ArrayList<AppInfo>();
                        for (AppInfo info : mRestoreApp) {
                            if (info != null
                                    && info.getPackagePath() != null
                                    && info.getPackagePath().startsWith(StorageUtil.getExternalStorage().getAbsolutePath())) {
                                removeApp.add(info);
                            }
                        }
                        mRestoreApp.removeAll(removeApp);
                    }             
                    if (mRestoreDataDetail != null) {
                        ArrayList<String> removeData = new ArrayList<String>();
                        for (String str : mRestoreDataDetail.keySet()) {
                            if (mRestoreDataDetail.get(str) == null || str.startsWith(StorageUtil.getExternalStorage().getAbsolutePath())) {
                                removeData.add(str);
                            }
                        }
                        for (String str : removeData) {
                            mRestoreDataDetail.remove(str);
                        }
                    }
                    if (mHandler != null) {
                        mHandler.sendEmptyMessage(MSG_DATA_AND_APP_LOAD_FINISH);
                    }
                }
            }
        }
    }

    private PendingIntent getPendingIntent(){
        if(mPendingIntent == null){
            Intent intent = new Intent(this, MainActivity.class);
            mPendingIntent = PendingIntent.getActivity(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
        } 
        return mPendingIntent;
    }

    private NotificationManager getNotificationManager(){
        if(mNotifyManager == null){
            mNotifyManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        }
        return mNotifyManager;
    }

    private void buildInternalFileObserver() {
        if (!Config.IS_NAND && StorageUtil.getInternalStorageState()) {
            if(DBG) Log.d(TAG, "Config.EXTERNAL_APP_BACKUP_PATH="+Config.INTERNAL_APP_BACKUP_PATH);
            if(DBG) Log.d(TAG, "Config.INTERNAL_ARCHIVE_ROOT_PATH="+Config.INTERNAL_ARCHIVE_ROOT_PATH);

            File archiveFolderr = new File(Config.INTERNAL_ARCHIVE_ROOT_PATH);
            if(archiveFolderr != null && !archiveFolderr.exists()){
                archiveFolderr.mkdirs();
            }
            File oldArchiveFolderr = new File(Config.OLD_VERSION_DATA_PATH_INTERNAL);
            if(oldArchiveFolderr != null && !oldArchiveFolderr.exists()){
                oldArchiveFolderr.mkdirs();
            }
            File appFolder = new File(Config.INTERNAL_APP_BACKUP_PATH);
            if(appFolder != null && !appFolder.exists()){
                appFolder.mkdirs();
            }
            mOldDataInternalPathObserver = new FileObserver(Config.OLD_VERSION_DATA_PATH_INTERNAL){

                @Override
                public void onEvent(int event, String path) {
                    int el = event & FileObserver.ALL_EVENTS;
                    switch(el){
                    case FileObserver.DELETE:
                    case FileObserver.CREATE:
                    case FileObserver.MOVED_FROM:
                    case FileObserver.MOVED_TO:
                    case FileObserver.MODIFY:
                    case FileObserver.DELETE_SELF:
                        if(DBG) Log.d(TAG, "switch mInternalFileObserver event="+el);
                        mHandler.removeMessages(MSG_DATA_FILE_CHANGED);
                        mHandler.sendEmptyMessageDelayed(MSG_DATA_FILE_CHANGED, 1500);
                        break;
                    default:
                        break;
                    }
                }
            };

            mInternalFileObserver = new FileObserver(Config.INTERNAL_ARCHIVE_ROOT_PATH){

                @Override
                public void onEvent(int event, String path) {
                    int el = event & FileObserver.ALL_EVENTS;
                    switch(el){
                    case FileObserver.DELETE:
                    case FileObserver.CREATE:
                    case FileObserver.MOVED_FROM:
                    case FileObserver.MOVED_TO:
                    case FileObserver.MODIFY:
                    case FileObserver.DELETE_SELF:
                        if(DBG) Log.d(TAG, "switch mInternalFileObserver event="+el);
                        mHandler.removeMessages(MSG_DATA_FILE_CHANGED);
                        mHandler.sendEmptyMessageDelayed(MSG_DATA_FILE_CHANGED, 1500);
                        break;
                    default:
                        break;
                    }
                }
            };
            mInternalApkObserver = new FileObserver(Config.INTERNAL_APP_BACKUP_PATH){

                @Override
                public void onEvent(int event, String path) {
                    int el = event & FileObserver.ALL_EVENTS;
                    switch(el){
                    case FileObserver.DELETE:
                    case FileObserver.CREATE:
                    case FileObserver.MOVED_FROM:
                    case FileObserver.MOVED_TO:
                    case FileObserver.DELETE_SELF:
                        if(DBG) Log.d(TAG, "switch mInternalApkObserver event="+event);
                        mHandler.removeMessages(MSG_APK_FILE_CHANGED);
                        mHandler.sendEmptyMessageDelayed(MSG_APK_FILE_CHANGED, 1000);
                        break;
                    default:
                        break;
                    }
                }
            };
            mInternalFileObserver.stopWatching();
            mInternalApkObserver.stopWatching();
            mInternalFileObserver.startWatching();
            mInternalApkObserver.startWatching();
            mOldDataInternalPathObserver.stopWatching();
            mOldDataInternalPathObserver.startWatching();
        }
    }
    private void buildExternalFileObserver(){
        if(DBG) Log.d(TAG, "buildFileObserver()..");
        if (StorageUtil.getExternalStorageState()) {
            if(DBG) Log.d(TAG, "Config.EXTERNAL_APP_BACKUP_PATH="+Config.EXTERNAL_APP_BACKUP_PATH);
            if(DBG) Log.d(TAG, "Config.EXTERNAL_ARCHIVE_ROOT_PATH="+Config.EXTERNAL_ARCHIVE_ROOT_PATH);

            File archiveFolderr = new File(Config.EXTERNAL_ARCHIVE_ROOT_PATH);
            if(archiveFolderr != null && !archiveFolderr.exists()){
                archiveFolderr.mkdirs();
            }
            File oldArchiveFolderr = new File(Config.OLD_VERSION_DATA_PATH_EXTERNAL);
            if(oldArchiveFolderr != null && !oldArchiveFolderr.exists()){
                oldArchiveFolderr.mkdirs();
            }
            File appFolder = new File(Config.EXTERNAL_APP_BACKUP_PATH);
            if(appFolder != null && !appFolder.exists()){
                appFolder.mkdirs();
            }

            mExternalFileObserver = new FileObserver(Config.EXTERNAL_ARCHIVE_ROOT_PATH){

                @Override
                public void onEvent(int event, String path) {
                    int el = event & FileObserver.ALL_EVENTS;
                    switch(el){
                    case FileObserver.DELETE:
                    case FileObserver.CREATE:
                    case FileObserver.MOVED_FROM:
                    case FileObserver.MOVED_TO:
                    case FileObserver.MODIFY:
                    case FileObserver.DELETE_SELF:
                        if(DBG) Log.d(TAG, "switch mExternalFileObserver event="+el);
                        mHandler.removeMessages(MSG_DATA_FILE_CHANGED);
                        mHandler.sendEmptyMessageDelayed(MSG_DATA_FILE_CHANGED, 1500);
                        break;
                    default:
                        break;
                    }
                }
            };
            mOldDataExternalPathObserver = new FileObserver(Config.OLD_VERSION_DATA_PATH_EXTERNAL){

                @Override
                public void onEvent(int event, String path) {
                    int el = event & FileObserver.ALL_EVENTS;
                    switch(el){
                    case FileObserver.DELETE:
                    case FileObserver.CREATE:
                    case FileObserver.MOVED_FROM:
                    case FileObserver.MOVED_TO:
                    case FileObserver.MODIFY:
                    case FileObserver.DELETE_SELF:
                        if(DBG) Log.d(TAG, "switch mExternalFileObserver event="+el);
                        mHandler.removeMessages(MSG_DATA_FILE_CHANGED);
                        mHandler.sendEmptyMessageDelayed(MSG_DATA_FILE_CHANGED, 1500);
                        break;
                    default:
                        break;
                    }
                }
            };
            mExternalApkObserver = new FileObserver(Config.EXTERNAL_APP_BACKUP_PATH){

                @Override
                public void onEvent(int event, String path) {
                    int el = event & FileObserver.ALL_EVENTS;
                    switch(el){
                    case FileObserver.DELETE:
                    case FileObserver.CREATE:
                    case FileObserver.MOVED_FROM:
                    case FileObserver.MOVED_TO:
                    case FileObserver.DELETE_SELF:
                        if(DBG) Log.d(TAG, "switch mExternalApkObserver event="+event);
                        mHandler.removeMessages(MSG_APK_FILE_CHANGED);
                        mHandler.sendEmptyMessageDelayed(MSG_APK_FILE_CHANGED, 1000);
                        break;
                    default:
                        break;
                    }
                }
            };
            mOldDataExternalPathObserver.stopWatching();
            mOldDataExternalPathObserver.startWatching();
            mExternalFileObserver.stopWatching();
            mExternalApkObserver.stopWatching();
            mExternalFileObserver.startWatching();
            mExternalApkObserver.startWatching();
        }
    }
    /* SPRD: Bug 420899 No cancel when backup or restore. @{ */
    public void createProgress(){
        if (mPrgsDialog != null && !mPrgsDialog.isShowing()) {
            mPrgsDialog.setCancelable(false);
            mPrgsDialog.show();
        } else if( mPrgsDialog == null ) {
            mPrgsDialog = new ProgressDialog(MainActivity.this);
            mPrgsDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
            mPrgsDialog.setMessage(getString(R.string.please_holding));
            mPrgsDialog.setCancelable(false);
            mPrgsDialog.show();
        }
    }
    /* @} */

    private void createProgressDlg() {
        if (mProgressBar == null) {
            mProgressBar = new ProgressDialog(this);
        }
        mProgressBar.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        mProgressBar.setCanceledOnTouchOutside(false);
        /* SPRD: Bug 420899 No cancel when backup or restore. @{ */
        mProgressBar.setCancelable(true);
        mProgressBar.setOnKeyListener(new OnKeyListener() {
            @Override
            public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                // TODO Auto-generated method stub
                if(keyCode == KeyEvent.KEYCODE_BACK){
                    if(dialog != null){
                        dialog.dismiss();
                        dialog = null;
                    }
                    mCancel = true;
                    createProgress();
                    return true;
                }
                return false;
            }
        });
        /* @} */
        mProgressBar.setButton(ProgressDialog.BUTTON_NEGATIVE,
                getText(R.string.cancel), new OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if(DBG) {Log.d(TAG, "mProgressBar.onClick()");}
                        mCancel = true;
                        if(dialog != null){
                            dialog.dismiss();
                            dialog = null;
                        }
                        /* SPRD: Bug 420899 No cancel when backup or restore. @{ */
                        createProgress();
                        /* @} */
                    }
                });
    }

    private void initLayout() {
        if(DBG) Log.d(TAG, "initLayout()");
        setContentView(R.layout.main_activity);
        mViewPager = (ViewPager) findViewById(R.id.viewpager);
        final ActionBar bar = getActionBar();
        bar.setNavigationMode(ActionBar.NAVIGATION_MODE_TABS);
        mTabsAdapter = new TabsAdapter(this, mViewPager);
        mBackupFragment = new BackupFragment();
        mRestoreFragment = new RestoreFragment();
        if (mInProgress) {
            Message message = new Message();
            message.arg1 = 0;
            message.what = MSG_SET_BACKUP_RESTORE_BUTTON;
            mHandler.sendMessage(message);
        }
        mTabsAdapter.addTab(bar.newTab().setCustomView(getTabHostView(R.string.backup)), mBackupFragment, null);
        mTabsAdapter.addTab(bar.newTab().setCustomView(getTabHostView(R.string.restore)), mRestoreFragment, null);
    }
    private View getTabHostView(int resId){
        View tabViewRes = LayoutInflater.from(this).inflate(R.layout.custom_tab_view, null);
        TextView tvBackupRes = (TextView) tabViewRes.findViewById(R.id.txt_tab_title);
        tvBackupRes.setText(resId);
        return tabViewRes;
    }
    private void closeProgressDlg() {
        if (mProgressBar != null) {
            mProgressBar.dismiss();
            mProgressBar = null;
        }
    }
    public void acquireWakeLock() {
        PowerManager powerManager = (PowerManager) getApplicationContext().getSystemService(Context.POWER_SERVICE);
        if(mWakeLock == null){
            mWakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "appbackup");
            mWakeLock.acquire();
        }
    }
    public void releaseWakeLock(){
        if(mWakeLock != null){
            mWakeLock.release();
            mWakeLock = null;
        }
    }
    private void displayBackupResult(ArrayList<Map<String, Object>> backupResult) {
        if (DBG) Log.d(TAG, "displayBackupResult()");
        if(backupResult == null || backupResult.size() < 1){
            return;
        }
        int count = backupResult.size();
        String[] result = new String[count];
        int i = 0;
        for(Map<String, Object> module:backupResult){
            int categoryCode = -1;
            Object codeObj = module.get(CATEGORY_CODE);
            if (codeObj != null) {
                categoryCode = (Integer)codeObj;
            }
            String name = module.get(AGENT_NAME).toString();
            name = getBackupingString(name, categoryCode, false, true);
            int code = (Integer)module.get(RESULT);

            if(code == FLAG_SUCCESS){
                result[i++] = name + " " + getText(R.string.backup_success);
            }else if(code == FLAG_NODATA){
                result[i++] = name + " " + getText(R.string.no_data_to_backup);
            }else{
                result[i++] = name + " " + getText(R.string.backup_fail);
            }
        }
        if(mActivate){
            new AlertDialog.Builder(MainActivity.this).setTitle(R.string.backup_all_result)
            .setItems(result, null).setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener(){
                public void onClick (DialogInterface dialog, int which){
                    dialog.dismiss();
                }
            }).setOnDismissListener(new OnDismissListener(){
                @Override
                public void onDismiss(DialogInterface dialog) {
                    if (!StorageUtil.getExternalStorageState() ) {
//                        displayHintChangePhone(getApplicationContext(), R.string.no_SD_backup_hint, REMINDUSER_NO_SD, R.string.no_SD_backup_detail);
                    }
                }
            }).setCancelable(true).create().show();
        }

        Config.USE_STORAGE = Config.USE_EXTERNAL_STORAGE;
        Config.IS_SELECT_EXTERNAL_STORAGE = true;
    }

    /* SPRD:display detail hint . @{*/
    public void displayHintChangePhone(final Context context, final int resourceId, final String remindString, final int resourceIdDetail) {
        SharedPreferences noSDHint = getSharedPreferences(remindString, 0);

        if (noSDHint.getBoolean(remindString, true)){
            View linearLayout = LayoutInflater.from(context).inflate(R.layout.hint_dialog_layout, null);
            final CheckBox chkBox = (CheckBox) linearLayout.findViewById(R.id.nomore);

            new AlertDialog.Builder(context).setTitle(R.string.change_mobile_hint)
                    .setMessage(resourceId)
                    .setView(linearLayout)
                    .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            dialog.dismiss();
                        }
                    }).setNegativeButton(R.string.read_detail, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            displayHintDetail(context, resourceIdDetail);
                        }
                    }).setOnDismissListener(new OnDismissListener(){
                        @Override
                        public void onDismiss(DialogInterface dialog) {
                            if (chkBox.isChecked()) {
                                SharedPreferences noSetSDHint = getSharedPreferences(remindString, 0);
                                noSetSDHint.edit().putBoolean(remindString, false).apply();
                            }
                        }
                    })
                    .setCancelable(false).create().show();
        }
    }
    /* @} */

    /* SPRD:display detail hint . @{*/
    public void displayHintDetail(final Context context, int resourceId) {
        AlertDialog dlg = new AlertDialog.Builder(context).setTitle(R.string.display_detail)
        .setMessage(resourceId).setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        }).setCancelable(false).create();
        dlg.show();
    }
    /* @} */

    private void displayRestoreResult(final List<DataItem> dataList, ArrayList<Map<String, Object>> restoreResult){
        if (DBG) Log.d(TAG, "displayRestoreResult()");
        if(restoreResult == null || restoreResult.size() < 1){
            if (DBG) Log.d(TAG, "restoreResult == null || restoreResult.size() < 1");
            return;
        }
        if(dataList == null || dataList.size() < 1){
            if (DBG) Log.d(TAG, "dataList == null || dataList.size() < 1");
        }

        int count = restoreResult.size();
        String[] result = new String[count];
        int i = 0;
        for(Map<String, Object> module:restoreResult){
            int categoryCode = -1;
            Object codeObj = module.get(CATEGORY_CODE);
            if (codeObj != null) {
                categoryCode = (Integer)codeObj;
            }
            String name = module.get(AGENT_NAME).toString();
            name = getBackupingString(name, categoryCode, false, true);
            int code = (Integer)module.get(RESULT);

            if(code == FLAG_SUCCESS){
                result[i++] = name + " " + getText(R.string.restore_success);
            }else{
                result[i++] = name + " " + getText(R.string.restore_fail);
            }
        }
        if(mActivate){
            if(dataList != null && dataList.size()>0){
                new AlertDialog.Builder(MainActivity.this).setTitle(R.string.restore_complete)
                .setItems(result, null)
                .setPositiveButton(R.string.delete_redundancy, new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                            new DeduplicateTask(dataList).execute();
                        }
                    }).setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            dialog.dismiss();
                            /* SPRD: Bug 420899 No cancel when backup or restore. @{ */
                            mHandler.sendEmptyMessage(MSG_DISMISS_PROGRESSDIALOG);
                            /* @} */
                        }
                    }).setCancelable(true).create().show();
            }else{
                new AlertDialog.Builder(MainActivity.this).setTitle(R.string.restore_complete)
                .setItems(result, null)
                .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                    }).setCancelable(true).create().show();
            }
        }
    }

    private void displayDuplicationResault(){
        if (DBG) Log.d(TAG, "displayRestoreResult()");
        if(mDuplicationResault == null || mDuplicationResault.size() < 1){
            return;
        }
        int count = mDuplicationResault.size();
        String[] result = new String[count];
        int i = 0;
        for(Map<String, Object> module:mDuplicationResault){
            int categoryCode = -1;
            Object codeObj = module.get(CATEGORY_CODE);
            if (codeObj != null) {
                categoryCode = (Integer)codeObj;
            }
            String name = module.get(AGENT_NAME).toString();
            name = getBackupingString(name, categoryCode, false, true);
            int code = (Integer)module.get(RESULT);

            switch(code){
            case AppBackupRestoreObserver.FLAG_DUPLICATION_SUCCESS:
                result[i++] = name +" "+ getText(R.string.clear_complete); 
                break;
            case AppBackupRestoreObserver.FLAG_DUPLICATION_UNSUPPORT:
                result[i++] = name +" "+ getText(R.string.duplication_unsupport); 
                break;
            default:
                result[i++] = name +" "+ getText(R.string.clear_complete); 
                break;
            }
        }

        if(mActivate){
            new AlertDialog.Builder(MainActivity.this).setTitle(R.string.delete_redundancy)
            .setItems(result,null)
            .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener(){
                public void onClick (DialogInterface dialog, int which){
                    dialog.dismiss();
                }
            }).setCancelable(true).create().show();
        }    
    }

    private ServiceConnection mBackupManagerConnection;
    private IScanAgentAndArchiveListener mScanListener = new IScanAgentAndArchiveListener.Stub() {

        @Override
        public void onScanComplete() throws RemoteException {
            // TODO Auto-generated method stub
            new Thread() {
                @Override
                public void run() {
                    mLoadAppTask = new LoadAppTask(0, getApplicationContext());
                    /* SPRD: Modify code for Bug454707 @{ */
                    getScanRestoreFile(Config.INTERNAL_STORAGE_PATH);
                    getScanRestoreFile(Config.EXTERNAL_STORAGE_PATH);
                    /* @} */
                    mAppList = mLoadAppTask.loadInstalledAppinfos();
                    mRestoreApp = mLoadAppTask.loadRestoredAppinfos();
                    mDataList = getEnabledAgent();
                    mRestoreDataDetail = getArchive();
                    mHandler.sendEmptyMessage(MSG_DATA_AND_APP_LOAD_FINISH);
                    Log.d(TAG, "onScanComplete(), notify onAppLoadFinished !");
                }

            }.start();
        }

        @Override
        public void onServiceConnectionChanged(String agentName, boolean connected) throws RemoteException {
            mHandler.sendEmptyMessage(AppBackupObserver.CONTENT_CHANGE);
            if (!connected) {
                if (agentName.equals(mInPorgressAgentName)) {
                    mCancel = true;
                    mInPorgressAgentName = null;
                }
                String toastStr = getBackupingString(agentName, -1, false, true) + " "
                        + MainActivity.this.getResources().getString(R.string.agent_service_disconnected);
                Toast.makeText(MainActivity.this, toastStr, Toast.LENGTH_LONG).show();
            }
        }
    };

    private void initService() {
        if (DBG) {
            Log.d(TAG, "initService()");
        }
        mBackupManagerConnection = new ServiceConnection() {
            public void onServiceConnected(ComponentName name, IBinder service) {
                mService = IAppBackupManager.Stub.asInterface(service);
                if (DBG) {
                    Log.d(TAG, "<---onServiceConnected()--->");
                }
                try {
                    mService.setScanCompleteListener(mScanListener);
                } catch (RemoteException re) {
                    Log.d(TAG, "RemoteException, setScanCompleteListener failed !");
                }
            }

            public void onServiceDisconnected(ComponentName name) {

            }

        };
        bindService(new Intent(this, AppBackupManagerService.class), mBackupManagerConnection,
                BIND_AUTO_CREATE);
    }

    private BroadcastReceiver mPackageManagerReceiver;

    private void startListenPackageManager() {
        mPackageManagerReceiver = new BroadcastReceiver() {

            @Override
            public void onReceive(Context context, Intent intent) {
                if (DBG) Log.d(TAG, "mPackageManagerReceiver.onReceive()");
                updateAppListView();
            }
        };
        if (DBG) {Log.d(TAG, "mPackageManagerReceiver startListenPackageManager()");}
        IntentFilter filter = new IntentFilter();
        filter.addAction("android.intent.action.PACKAGE_ADDED");
        filter.addAction("android.intent.action.PACKAGE_REMOVED");
        filter.addDataScheme("package");
        registerReceiver(mPackageManagerReceiver, filter);
    }

    private void updateAppListView() {
        if (DBG) Log.d(TAG, "mPackageManagerReceiver.updateAppListView()");
        new Thread() {

            @Override
            public void run() {
                if (mLoadAppTask != null) {
                    if (DBG) Log.d(TAG, "mPackageManagerReceiver.loadInstalledAppinfos()");
                    mAppList = mLoadAppTask.loadInstalledAppinfos();
                }
                mHandler.sendEmptyMessage(MSG_UPDATE_APP_LISTVIEW);
            }
        }.start();
    }

    private void stopListenPackageManager() {
        if (mPackageManagerReceiver != null) {
            unregisterReceiver(mPackageManagerReceiver);
            mPackageManagerReceiver = null;
        }
    }

    private List<DataItem> getEnabledAgent() {
        if (DBG) Log.d(TAG, "getEnabledAgent()");
        List<DataItem> agentList = new ArrayList<DataItem>();
        try {
            mAgents = mService.getAgents();
        } catch (Exception e) {
        }
        if (mAgents == null || mAgents.length == 0) {
            if (DBG) Log.d(TAG, "mAgents == null || mAgents.length == 0");
            return agentList;
        }
        Category[] cate = null;
        for (Agent agent : mAgents) {
            if (agent != null) {
                if (DBG)
                    Log.d(TAG, "agent = " + agent.toString());
                try {
                    cate = mService.getCategory(agent);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
                if (cate == null) {
                    // Bug 554426 SprdAppbackup won't crash when some Apps lack of Permission
                    try {
                        agentList.add(DataUtil.getDataItem(this, mService, agent));
                    } catch (SecurityException e) {
                        // TODO: handle exception
                        Log.e(TAG, "some Apps lack of Permission");
                    }
                } else {
                    for (Category c : cate) {
                        if (c != null) {
                            agentList.add(DataUtil.getDataItem(this, mService,
                                    agent, c.getmCode(), c.getmDescription()));
                        }
                    }
                }
            }
        }
        return agentList;
    }

    private Map<String, List<DataItem>> getArchive() {
        if (DBG) {Log.d(TAG, "getArchive()..");}
        Map<String, List<DataItem>> restoreDataDetail = new TreeMap<String, List<DataItem>>(new Comparator<String>() {
            @Override
            public int compare(String lhs, String rhs) {
                return rhs.compareTo(lhs);
            }
        });
        scanArchives();
        ArrayList<Archive> archivesList = new ArrayList<Archive>(mArchivesList);
        List<DataItem> tmpList;
        if (archivesList != null) {
            if (DBG) Log.d(TAG, "archivesList.size() = " + archivesList.size());
            for (Archive a : archivesList) {
                if (a != null) {
                    // Bug 557309 SprdAppbackup won't crash when some Apps lack of Permission
                    try {
                        if (DBG) Log.d(TAG, "archive = " + a.getPath());
                        tmpList = DataUtil.getEnabledArchive(this, mService, mAgents, a.getPath());
                        if (DBG) Log.d(TAG, "getArchive--tmpList =" + tmpList);
                        if (tmpList != null && tmpList.size() > 0) {
                            if (DBG) Log.d(TAG, "tmpList != null && tmpList.size() > 0");
                            restoreDataDetail.put(a.getPath(), tmpList);
                        }
                    } catch (SecurityException e) {
                        // TODO: handle exception
                        Log.e(TAG, "some Apps lack of Permission");
                    }
                }
            }
        }
        return restoreDataDetail;
    }

    public void backupData(List<DataItem> listData, ArrayList<Map<String, Object>> backupResultList, Handler handler, int whichStorage) {
        if (DBG) Log.d(TAG, "backupData()--start-->");
        if (listData == null || listData.size() < 1) {
            if (DBG) Log.d(TAG, "backupData()-->listData == null");
            return;
        }
        mAppBackupRetainData.backupDataTask = new BackupDataTask(listData, backupResultList, handler, whichStorage);
        mAppBackupRetainData.backupDataTask.execute();
    }

    private class BackupDataTask extends AsyncTask<Void, Object, Integer> {

        Archive archive;
        AppBackupRestoreObserver observer;
        List<DataItem> dataList;
        int index;
        int progress = 0;
        int max = 1;
        String title = null;
        String numFormat = null;
        int categoryCode = -1;
        String agentName = null;
        Handler taskHandler = null;
        int useStorage = Config.USE_EXTERNAL_STORAGE;
        ArrayList<Map<String, Object>> taskBackupResult = new ArrayList<Map<String, Object>>();

        public void setTaskHandler(Handler handler) {
            this.taskHandler = handler;
        }

        public BackupDataTask(List<DataItem> data, ArrayList<Map<String, Object>> backupResultList,Handler handler, int whichStorage) {
            if(DBG) Log.d(TAG, "BackupDataTask create, data = "+data);
            dataList = data;
            this.taskHandler = handler;
            useStorage = whichStorage;
            agentName = dataList.get(0).getAgentName();
            categoryCode = dataList.get(0).getCategoryCode();
            title = getBackupingString(agentName, categoryCode, true, false);
            if (backupResultList == null || backupResultList.size() <= 0) {
                taskBackupResult = new ArrayList<Map<String, Object>>();
            } else {
                taskBackupResult = backupResultList;
            }
        }

        @Override
        protected Integer doInBackground(Void... params) {
            if(DBG) Log.d(TAG, "BackupDataTask.doInBackground()");
            int resultCode = 0;
            String contentText;

            if (useStorage == Config.USE_EXTERNAL_STORAGE) {
                archive = addArchive(Config.EXTERNAL_ARCHIVE_ROOT_PATH);
            } else if (useStorage == Config.USE_INTERNAL_STORAGE) {
                archive = addArchive(Config.INTERNAL_ARCHIVE_ROOT_PATH);
            } else if (useStorage == Config.USE_DEFINED_PATH) {
                archive = addArchive(mBackupFragment.getSelectBackupPath(false));
            }

            if (null == archive) {
                return -1;
            }
            if(DBG) Log.d(TAG, "dataList = "+dataList);
            for (DataItem di : dataList) {
                agentName = di.getAgentName();
                categoryCode = di.getCategoryCode();
                mInPorgressAgentName = di.getAgentName();
                title = getBackupingString(agentName, categoryCode, true, false);
                progress = 0;
                max = 100;
                numFormat = null;
                MessageForProgressUpdate msgObj = new MessageForProgressUpdate();
                msgObj.progress = progress;
                msgObj.max = max;
                msgObj.title = title;
                msgObj.numFormat = numFormat;
                Message msg = Message.obtain(taskHandler, MSG_UPDATE_PROGRESS, msgObj);
                taskHandler.sendMessage(msg);

                index = dataList.indexOf(di);
                if (mCancel || (Config.IS_SELECT_EXTERNAL_STORAGE && mSdCardUnmounted)) {
                    if(DBG) Log.d(TAG, "mCancel = "+mCancel +"--mSdCardUnmounted = "+(Config.IS_SELECT_EXTERNAL_STORAGE && mSdCardUnmounted));
                    deleteCurrentDir(di);
                    Archive.deleteEmptyDirs(archive);
                    /* SPRD: modify for bug 632148 @{ */
                    taskHandler.sendEmptyMessage(MSG_DISMISS_PROGRESSDIALOG);
                    /* @} */
                    return CANCEL_FLAG;
                }
                Map<String, Object> resultMap = new HashMap<String, Object>();
                resultMap.put(AGENT_NAME, di.getAgentName());
                resultMap.put(CATEGORY_CODE, di.getCategoryCode());

                observer = new AppBackupRestoreObserver();
                if (DBG) Log.d(TAG, "doInBackground index=" + index);
                if (DBG) Log.d(TAG, "doInBackground di=" + di);
                try {
                    resultCode = mService.requestBackup(archive, di.getAgent(),
                            observer, di.getCategoryCode(), di.getAccounts());
                    if (resultCode == -1) {
                        resultMap.put(RESULT, -1);
                        taskBackupResult.add(resultMap);
                        continue;
                    }
                    if (DBG) Log.d(TAG, "mService.requestBackup =  " + observer);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
                int tmp = 0;
                String agentName = di.getAgentName();
                int categoryCode = di.getCategoryCode();

                if (DBG) Log.d(TAG, "back first observer = " + observer);
                while (observer.getmCurrent() < observer.getmTotal()) {
                    if (mCancel || (Config.IS_SELECT_EXTERNAL_STORAGE && mSdCardUnmounted)) {
                        if(DBG) Log.d(TAG, "mCancel = " + mCancel
                            + ", mSdCardUnmounted = " + mSdCardUnmounted
                            + ", isSelectedExternalStorage : "
                            + Config.IS_SELECT_EXTERNAL_STORAGE);
                        if(DBG) Log.d(TAG, "BackupDataTask is Cancelled");
                        try {
                            mService.requestCancel(di.getAgent(), di.getCategoryCode());
                        } catch (RemoteException e) {
                            e.printStackTrace();
                        }
                        deleteCurrentDir(di);
                        Archive.deleteEmptyDirs(archive);
                        return CANCEL_FLAG;
                    }
                    if (observer.getmCurrent() != tmp) {
                        if (DBG) Log.d(TAG, "observer = " + observer);

                        if ((0 == (observer.getmCurrent() % BACKUP_INTERVALL) || observer.getmCurrent() < MAX_BACKUP_INTERVALL_NUM)) {
                            publishProgress(observer.getmCurrent(), observer.getmTotal(), observer.getUnit(), agentName, categoryCode);
                        }
                    }
                    tmp = observer.getmCurrent();
                    /* SPRD: modify for bug609378  @{ */
                    if (observer.getResult() == -1) break;
                    /* @} */
                }
                /* SPRD: modify for bug609378  @{ */
                if (observer.getResult() != -1) {
                    publishProgress(observer.getmTotal(), observer.getmTotal(), observer.getUnit(), di.getAgentName(), di.getCategoryCode());
                }
                /* @} */
                if (DBG) {Log.d(TAG, "back last observer = " + observer);}
                if (DBG) {if(observer != null)Log.d(TAG, "back last observer.getResult() = " + observer.getResult());}
                resultMap.put(RESULT, observer.getResult());
                taskBackupResult.add(resultMap);
            }
            return 0;
        }

        private void deleteCurrentDir(DataItem cancelData){
            if(DBG) {Log.d(TAG, "deleteCurrentDir() start");}
            String currentFolder = null;
            if(cancelData != null){
                if(DBG) {Log.d(TAG, ", agentName = "+cancelData.getAgentName());}
                if(cancelData.getAgentName().equals("Mms")){
                    if(DBG) {Log.d(TAG, "cancelData.getCategoryCode() = "+cancelData.getCategoryCode());}
                    if(cancelData.getCategoryCode() == 0){
                       currentFolder = "Sms";
                    }else if(cancelData.getCategoryCode() == 1){
                       currentFolder = "Mms";
                    }
                }else{
                    currentFolder = cancelData.getAgentName();
                }
                if(DBG) {Log.d(TAG, ", currentFolder = "+currentFolder);}
            }
            Archive.deleteDir(archive, currentFolder);
            if(DBG) {Log.d(TAG, "end delete agent folder");}
        }

        @Override
        protected void onPostExecute(Integer result) {
            if (DBG) Log.d(TAG, "onPostExecute()");
            mAppBackupRetainData.backupDataTask = null;
            mInProgress = false;
            taskHandler.sendEmptyMessageDelayed(MSG_CLOSE_PROGRESS_DIALOG, 700);
            releaseWakeLock();
            cancelTimerTask();
            Message message = new Message();
            message.arg1 = 1;
            message.what = MSG_SET_BACKUP_RESTORE_BUTTON;
            taskHandler.sendMessage(message);

            getNotificationManager().cancel(TAG, 0);
            if(result == CANCEL_FLAG){
                if(DBG) {Log.d(TAG, "BackupDataTask is cancelled");}
                Notification notification=new Notification.Builder(MainActivity.this).setAutoCancel(true)
                        .setContentTitle(MainActivity.this.getResources().getString(R.string.data_backup))
                        .setContentText(MainActivity.this.getResources().getString(R.string.backup_is_cancelled))
                        .setWhen(System.currentTimeMillis())
                        .setSmallIcon(R.drawable.icon)
                        .setProgress(0,0,false)
                        .setContentIntent(getPendingIntent())
                        .setOngoing(false).build();
                getNotificationManager().notify(TAG_NOTIFICATION, 0, notification);
                /* SPRD: Bug 420899 No cancel when backup or restore. @{ */
                taskHandler.sendEmptyMessage(MSG_DISMISS_PROGRESSDIALOG);
                /* @} */
                Toast.makeText(getApplicationContext(), R.string.backup_is_cancelled, Toast.LENGTH_SHORT).show();
            }else{
                Notification notification=new Notification.Builder(MainActivity.this).setAutoCancel(true)
                        .setContentTitle(MainActivity.this.getResources().getString(R.string.data_backup))
                        .setSmallIcon(R.drawable.icon)
                        .setContentText(MainActivity.this.getResources().getString(R.string.backup_complete))
                        .setWhen(System.currentTimeMillis())
                        .setProgress(0,0,false)
                        .setContentIntent(getPendingIntent())
                        .setOngoing(false).build();
                getNotificationManager().notify(TAG_NOTIFICATION, 0, notification);
                Message msg = Message.obtain(taskHandler, MSG_DISPLAY_BACKUP_RESULT, taskBackupResult);
                taskHandler.sendMessageDelayed(msg, 1000);
                taskHandler.removeMessages(MSG_DATA_FILE_CHANGED);
                taskHandler.sendEmptyMessageDelayed(MSG_DATA_FILE_CHANGED, UPDATE_INTERVAL);
            }
            /* SPRD: Modify code for Bug454690 @{ */
//            if (useStorage == Config.USE_DEFINED_PATH) {
                File rootFile = new File(mBackupFragment.getSelectBackupPath(false));
                //add file to database
                if (rootFile.isDirectory()) {
                    String path = rootFile.getPath();
                    Intent intent = new Intent("android.intent.action.MEDIA_SCANNER_SCAN_DIR");
                    Bundle bundle = new Bundle();
                    bundle.putString("scan_dir_path", path);
                    intent.putExtras(bundle);
                    getApplicationContext().sendBroadcast(intent);
                } else {
                    android.media.MediaScannerConnection.scanFile(getApplicationContext(),
                        new String[]{ rootFile.getAbsolutePath() }, null, null);
                }
//            }
            /* @} */
        }

        @Override
        protected void onPreExecute() {
            if (DBG) Log.d(TAG, "BackupDataTask.onPreExecute()");
            acquireWakeLock();
            mCancel = false;
            mInProgress = true;
            Message message = new Message();
            message.arg1 = 0;
            message.what = MSG_SET_BACKUP_RESTORE_BUTTON;
            taskHandler.sendMessage(message);

            title = getBackupingString(agentName, categoryCode, true, false);
            MessageForProgressUpdate msgObj = new MessageForProgressUpdate();
            msgObj.progress = progress;
            msgObj.max = max;
            msgObj.title = title;
            msgObj.numFormat = numFormat;
            Message msg = Message.obtain(taskHandler, MSG_UPDATE_PROGRESS, msgObj);
            taskHandler.sendMessage(msg);

            Notification notification=new Notification.Builder(MainActivity.this).setAutoCancel(true)
                    .setContentTitle(MainActivity.this.getResources().getString(R.string.data_backup))
                    .setContentText(MainActivity.this.getResources().getString(R.string.backup_is_ongoing))
                    .setSmallIcon(R.drawable.icon)
                    .setWhen(System.currentTimeMillis())
                    .setProgress(0,0,false)
                    .setContentIntent(getPendingIntent())
                    .setOngoing(true).build();
            getNotificationManager().notify(TAG, 0, notification);

        }

        @Override
        protected void onProgressUpdate(Object... values) {
            if (DBG) Log.d(TAG, "BackupDataTask.onProgressUpdate()");
            mInProgress = true;
            categoryCode = (Integer)values[4];
            agentName = (String)values[3];
            title = getBackupingString(agentName, categoryCode, true, false);
            MessageForProgressUpdate msgObj = new MessageForProgressUpdate();
            progress = (Integer)values[0];
            max = (Integer) values[1];
            msgObj.progress = progress;
            msgObj.max = max;
            msgObj.title = title;
            if (progress > 0) {
                String unit = (String) values[2];
                numFormat = "%1d" + unit + "/%2d" + unit;
                msgObj.numFormat = numFormat;
            }
            Message msg = Message.obtain(taskHandler, MSG_UPDATE_PROGRESS, msgObj);
            taskHandler.sendMessage(msg);
            Notification notification=new Notification.Builder(MainActivity.this)
                    .setContentTitle(MainActivity.this.getResources().getString(R.string.data_backup))
                    .setSmallIcon(R.drawable.icon)
                    .setContentText(title)
                    .setWhen(0)
                    .setProgress((Integer)values[1],(Integer)values[0],false)
                    .setContentIntent(getPendingIntent())
                    .setOngoing(true).build();
            notification.flags |= Notification.FLAG_NO_CLEAR;
            getNotificationManager().notify(TAG, 0, notification);
        }
        public int getMax() {
            return max;
        }
        public int getProgress() {
            return progress;
        }
        public String getNumFormat() {
            return numFormat;
        }
        public String getTitle() {
            title = getBackupingString(agentName, categoryCode, true, false);
            return title;
        }

    }

    public void backupAll(List<AppInfo> app, List<DataItem> data, int whichStorage) {
        if (DBG) {Log.d(TAG, "backupAll()--start-->");}
        if (app == null) {
            if (DBG) {Log.d(TAG, "backupAll()-App == null");}
            return;
        }
        /* SPRD : fix bug256962 @{ */
        if (data.size() > 0) {
            remindUseUserDataDialog(true, data, app, whichStorage, null);
        } else {
            /* SPRD: 453980 check if backup path is in external storage @{ */
            checkIsSelectExternalStorage(whichStorage);
            startObserverTask(!Config.IS_SELECT_EXTERNAL_STORAGE);
            /* @} */

            if(app.size()>0){
                mAppBackupRetainData.backupAppTask = new BackupAppTask(app, data, mHandler, whichStorage);
                mAppBackupRetainData.backupAppTask.execute();
            }
        }
        /* @} */
    }

    /* SPRD: 453980 check if backup path is in external storage @{ */
    private void checkIsSelectExternalStorage(int whichStorage) {
       Config.IS_SELECT_EXTERNAL_STORAGE = true;

       if (Config.USE_STORAGE == Config.USE_DEFINED_PATH) {
           String path = mBackupFragment.getSelectBackupPath(true);

           if (path != null && !path.startsWith(Config.EXTERNAL_STORAGE_PATH)) {
               Config.IS_SELECT_EXTERNAL_STORAGE = false;
           }
       } else if (Config.USE_STORAGE == Config.USE_INTERNAL_STORAGE) {
           Config.IS_SELECT_EXTERNAL_STORAGE = false;
       }
    }
    /* @} */

    private void startObserverTask(final boolean listenInternal){
        if(DBG) {Log.d(TAG, "startObserverTask()");}
        mTimer = new Timer(true);
        mTask = new TimerTask(){

            @Override
            public void run() {
                long freeSpace = 0;
                if (listenInternal) {
                    if (StorageUtil.getInternalStorageState()) {
                        freeSpace = StorageUtil.getAvailableInternalMemorySize()/1024/1024;
                        if(freeSpace < 5){
                            mCancel = true;
                            mHandler.sendEmptyMessage(MSG_LACK_OF_SPACE);
                            return;
                        }
                    }
                } else {
                    if (StorageUtil.getExternalStorageState()) {
                        freeSpace = StorageUtil.getAvailableExternalMemorySize()/1024/1024;
                        if(freeSpace < 5){
                            mCancel = true;
                            mHandler.sendEmptyMessage(MSG_LACK_OF_SPACE);
                            return;
                        }
                    }
                }
                if(DBG) {Log.d(TAG, "mFreeSpaceObserver > freeSpace = "+freeSpace+"m");}
            }

        };
        mTimer.schedule(mTask, 3000, 3000);
    }

    private class BackupAppTask extends AsyncTask<Void, Integer, Integer> {

        List<AppInfo> data;
        List<DataItem> dataItem;
        int progress = 0;
        int max = 1;
        String title = null;
        String numFormat = null;
        int useStorage = Config.USE_STORAGE;
        Handler taskHandler = null;
        ArrayList<Map<String, Object>> taskBackupResult = new ArrayList<Map<String, Object>>();

        public void setTaskHandler(Handler handler) {
            this.taskHandler = handler;
        }

        public BackupAppTask(List<AppInfo> listApp, List<DataItem> datalist, Handler handler, int whichStorage) {
            data = listApp;
            dataItem = datalist;
            this.taskHandler = handler;
            max = listApp.size();
            useStorage = whichStorage;
            if (DBG) Log.d(TAG, "BackupAppTask create, totalCount=" + max);
        }

        @Override
        protected void onPreExecute() {
            if (DBG) Log.d(TAG, "BackupAppTask.onPreExecute()");
            Message message = new Message();
            message.arg1 = 0;
            message.what = MSG_SET_BACKUP_RESTORE_BUTTON;
            taskHandler.sendMessage(message);
            mCancel = false;
            mInProgress = true;
            acquireWakeLock();
            title = MainActivity.this.getResources().getString(R.string.backup_application);
            MessageForProgressUpdate msgObj = new MessageForProgressUpdate();
            msgObj.progress = progress;
            msgObj.max = max;
            msgObj.title = title;
            msgObj.numFormat = numFormat;
            Message msg = Message.obtain(taskHandler, MSG_UPDATE_PROGRESS, msgObj);
            taskHandler.sendMessage(msg);

            Notification notification=new Notification.Builder(MainActivity.this).setAutoCancel(true)
                    .setContentTitle(MainActivity.this.getResources().getString(R.string.backup_application))
                    .setContentText(MainActivity.this.getResources().getString(R.string.backup_is_ongoing))
                    .setSmallIcon(R.drawable.icon)
                    .setWhen(System.currentTimeMillis())
                    .setProgress(0,0,false)
                    .setContentIntent(getPendingIntent())
                    .setOngoing(true).build();
            getNotificationManager().notify(TAG, 0, notification);
        }

        @Override
        protected void onProgressUpdate(Integer... values) {
            if (DBG) {
                Log.d(TAG, "BackupAppTask.onProgressUpdate()");
                Log.d(TAG, "progress values = " + (Integer) values[0]);
            }
            mInProgress = true;
            MessageForProgressUpdate msgObj = new MessageForProgressUpdate();
            progress = (Integer)values[0];
            msgObj.progress = progress;
            msgObj.max = max;
            msgObj.title = title;
            if (progress > 0) {
                numFormat = "%1d/%2d";
                msgObj.numFormat = numFormat;
            }
            Message msg = Message.obtain(taskHandler, MSG_UPDATE_PROGRESS, msgObj);
            taskHandler.sendMessage(msg);

            Notification notification=new Notification.Builder(MainActivity.this)
                    .setWhen(0)
                    .setContentTitle(MainActivity.this.getResources().getString(R.string.backup_application))
                    .setContentText(MainActivity.this.getResources().getString(R.string.backup_is_ongoing))
                    .setSmallIcon(R.drawable.icon)
                    .setProgress(max,(Integer)values[0],false)
                    .setContentIntent(getPendingIntent())
                    .setOngoing(true).build();
            notification.flags |= Notification.FLAG_NO_CLEAR;
            getNotificationManager().notify(TAG, 0, notification);
        }

        @Override
        protected Integer doInBackground(Void... params) {
            mAppIsBackuping = true;
            if(DBG) Log.d(TAG, "BackupAppTask.doInBackground()");
            String path = Config.EXTERNAL_APP_BACKUP_PATH;

            if (useStorage == Config.USE_INTERNAL_STORAGE) {
                path = Config.INTERNAL_APP_BACKUP_PATH;
            } else if (useStorage == Config.USE_DEFINED_PATH) {
                path = mBackupFragment.getSelectBackupPath(true);
            }

            int mSuccessCount = 0;
            if (data != null) {
                for (AppInfo appInfo : data) {
                    if (DBG) Log.d(TAG, "AppInfo = " + appInfo);
                    Map<String, Object> resultMap = new HashMap<String, Object>();
                    resultMap.put(AGENT_NAME, appInfo.getName());
                    if(DBG) Log.d(TAG, "mCancel = "+mCancel+"--mSdCardUnmounted = "+(Config.IS_SELECT_EXTERNAL_STORAGE && mSdCardUnmounted));

                    if(mCancel || (Config.IS_SELECT_EXTERNAL_STORAGE && mSdCardUnmounted)) {
                        if(DBG) {Log.d(TAG,"return CANCEL_FLAG before backup a Apk");}
                        /* SPRD: modify for bug 632148 @{ */
                        taskHandler.sendEmptyMessage(MSG_DISMISS_PROGRESSDIALOG);
                        /* @} */
                        return CANCEL_FLAG;//cancel flag;
                    }

                    if(!backupUserApp(path, appInfo)) {
                        resultMap.put(RESULT, FLAG_FAIL);
                    }else{
                        resultMap.put(RESULT, FLAG_SUCCESS);
                    }

                    taskBackupResult.add(resultMap);
                    if(DBG) {Log.d(TAG, "last progress = "+ mSuccessCount+"/total = "+ max);}
                    publishProgress(++mSuccessCount);
                    progress++;
                }
            }
            return 0;
        }

        @Override
        protected void onPostExecute(Integer result) {
            if (DBG) Log.d(TAG, "BackupAppTask.onPostExecute()");
            mAppIsBackuping = false;
            mAppBackupRetainData.backupAppTask = null;

            mInProgress = false;
            releaseWakeLock();
            Message message = new Message();
            message.arg1 = 1;
            message.what = MSG_SET_BACKUP_RESTORE_BUTTON;
            taskHandler.sendMessage(message);
            getNotificationManager().cancel(TAG, 0);
            if(result == CANCEL_FLAG){
                taskHandler.sendEmptyMessageDelayed(MSG_CLOSE_PROGRESS_DIALOG, 700);
                cancelTimerTask();
                if(DBG) Log.d(TAG, "BackupAppTask is cancelled");
                Notification notification=new Notification.Builder(MainActivity.this).setAutoCancel(true)
                        .setWhen(System.currentTimeMillis())
                        .setContentTitle(MainActivity.this.getResources().getString(R.string.backup_application))
                        .setContentText(MainActivity.this.getResources().getString(R.string.backup_is_cancelled))
                        .setProgress(0,0,false)
                        .setSmallIcon(R.drawable.icon)
                        .setContentIntent(getPendingIntent())
                        .setOngoing(false).build();
                getNotificationManager().notify(TAG_NOTIFICATION, 0, notification);
                /* SPRD: Bug 420899 No cancel when backup or restore. @{ */
                taskHandler.sendEmptyMessage(MSG_DISMISS_PROGRESSDIALOG);
                /* @} */
                Toast.makeText(getApplicationContext(), R.string.backup_is_cancelled, Toast.LENGTH_SHORT).show();
            }else{
                taskHandler.removeMessages(MSG_APK_FILE_CHANGED);
                taskHandler.sendEmptyMessageDelayed(MSG_APK_FILE_CHANGED, 1000);
                if (mCancel != true && dataItem != null && dataItem.size() > 0) {
                    taskHandler.postDelayed(new Runnable() {
                        public void run() {
                            backupData(dataItem, taskBackupResult, taskHandler, useStorage);
                        }
                    }, 700);
                } else {
                    taskHandler.sendEmptyMessageDelayed(MSG_CLOSE_PROGRESS_DIALOG, 700);
                    Notification notification=new Notification.Builder(MainActivity.this).setAutoCancel(true)
                            .setContentTitle(MainActivity.this.getResources().getString(R.string.backup_application))
                            .setContentText(MainActivity.this.getResources().getString(R.string.backup_complete))
                            .setSmallIcon(R.drawable.icon)
                            .setWhen(System.currentTimeMillis())
                            .setProgress(0,0,false)
                            .setContentIntent(getPendingIntent())
                            .setOngoing(false).build();
                    getNotificationManager().notify(TAG_NOTIFICATION, 0, notification);
                    Message msg = Message.obtain(taskHandler, MSG_DISPLAY_BACKUP_RESULT, taskBackupResult);
                    taskHandler.sendMessageDelayed(msg, 1000);
                }
            }
        }
        public int getMax() {
            return max;
        }
        public int getProgress() {
            return progress;
        }
        public String getNumFormat() {
            return numFormat;
        }
        public String getTitle() {
            title = MainActivity.this.getResources().getString(R.string.backup_application);
            return title;
        }

        private boolean backupUserApp(String dstPath, AppInfo info) {
            Utils.checkAndMakeFolder(dstPath);
            if (info.isChecked()) {
                boolean copyFile = false;
                String packagename = info.getPackageName();
                copyFile = copyFileToDir(info.getPackagePath(), dstPath,
                        packagename + ".sp");
                File tempApkFile = new File(dstPath + "/" + packagename + ".sp");
                File targetApkFile = new File(dstPath + "/" + packagename
                        + Config.SUFFIX_APK);
                if (DBG) Log.v(TAG, "tempApkFile = " + tempApkFile);
                if (DBG) Log.v(TAG, "targetApkFile = " + targetApkFile);

                //add file to database
                /* SPRD: Modify code for Bug454690 @{ */
//                if (useStorage == Config.USE_DEFINED_PATH) {
                    android.media.MediaScannerConnection.scanFile(getApplicationContext(),
                            new String[]{ targetApkFile.getAbsolutePath() }, null, null);
//                }
                /* @} */
                if (copyFile) {
                    tempApkFile.renameTo(targetApkFile);
                } else {
                    if (tempApkFile.exists()) {
                        tempApkFile.delete();
                    }
                    return false;
                }

                return backupUserAppData(packagename, dstPath);
            }

            return true;
        }

        private boolean backupUserAppData(String pkgName, String destPath) {
            File backupFolder = mApp.getBackupUserAppFolder(pkgName);
            File thirdAppBackupFolder = mApp.getBackupUserAppFolder();
            boolean ret = true;

        	if(backupFolder == null) {
        		return false;
        	}

        	int pmRet = mPm.backupAppData(pkgName, backupFolder.getPath());
        	if(pmRet != PackageManagerEx.BACKUPAPP_SUCCEED) {
                return false;
        	}

           	StringBuilder sb = new StringBuilder(destPath);
        	sb.append('/');
        	sb.append(pkgName);
        	sb.append(".tar");
        	File targetFile = new File(sb.toString());
        	if(targetFile.exists()) {
        		targetFile.delete();
        	}
            ret = FileUtils.compression(backupFolder, thirdAppBackupFolder, targetFile);
            /*
        	try {
                TarUtils.archive(backupFolder, targetFile);
            } catch (IOException e) {
                ret = false;
                Log.e(TAG, "in backupUserAppData, TarUtils.archive error ");
            }
            */
                if(!targetFile.exists()) {
        		try {
        			targetFile.createNewFile();
        		} catch (IOException e) {
        			ret = false;
        			Log.e(TAG, "in backupUserAppData, pkgName: " + pkgName + " create file: " + targetFile + " failed");
        		}
        	}
        	FileUtils.erase(backupFolder);
        	return ret;
        }

        private boolean copyFileToDir(String appPath, String dirPath,
                String appName) {
            boolean ret = false;
            File dirFile = new File(dirPath);
            if (!dirFile.exists()) {
                dirFile.mkdirs();
            }
            if(DBG) {Log.i(TAG, "appPath : " + appPath);}
            File appFile = new File(appPath);
            if(DBG) {Log.i(TAG, "appFile.length() : " + appFile.length());}
            long time = appFile.lastModified();
            File backupAppFile = new File(dirPath + "/" + appName);
            FileInputStream in = null;
            FileOutputStream out = null;
            try {
                in = new FileInputStream(appFile);
                out = new FileOutputStream(backupAppFile);
                byte[] buffer = new byte[1024];
                int length;
                while ((length = in.read(buffer)) != -1) {
                    // SPRD: 461604 remove the determine of /data/app
                    if (mCancel
                            || (Config.IS_SELECT_EXTERNAL_STORAGE && mSdCardUnmounted)) {
                        if(DBG) {Log.i(TAG, "copyFileToDir return due to cancel ");}
                        if (in != null) {
                            try {
                                in.close();
                            } catch (IOException e) {
                            }
                        }
                        if (out != null) {
                            try {
                                out.close();
                            } catch (IOException e) {
                            }
                        }
                        return false;
                    }
                    out.write(buffer, 0, length);
                }
                if(time > 0){
                    backupAppFile.setLastModified(time);
                }
                ret = true;
            } catch (FileNotFoundException e) {
                e.printStackTrace();
                ret = false;
            } catch (IOException e) {
                e.printStackTrace();
                ret = false;
            } finally {
                if (in != null) {
                    try {
                        in.close();
                    } catch (IOException e) {
                    }
                }
                if (out != null) {
                    try {
                        out.close();
                    } catch (IOException e) {
                    }
                }
            }
            return ret;
        }

    }

    public void clearDuplicationResault(){
        if(mDuplicationResault != null){
            mDuplicationResault.clear();
        }else{
            mDuplicationResault = new ArrayList<Map<String, Object>>();
        }
    }
    @Override
    protected void onDestroy() {
        if(DBG) Log.d(TAG, "onDestroy()..");
        if (mPrgsDialog != null) {
            mPrgsDialog.dismiss();
            mPrgsDialog = null;
        }
        if(mProgressBar != null){
            mProgressBar.dismiss();
            mProgressBar = null;
        }
        if (mOldDataExternalPathObserver != null) {
            mOldDataExternalPathObserver.stopWatching();
            mOldDataExternalPathObserver = null;
        }
        if(mExternalFileObserver != null){
            mExternalFileObserver.stopWatching();
            mExternalFileObserver = null;
        }
        if(mExternalApkObserver != null){
            mExternalApkObserver.stopWatching();
            mExternalApkObserver = null;
        }
        if(mInternalFileObserver != null){
            mInternalFileObserver.stopWatching();
            mInternalFileObserver = null;
        }
        if(mOldDataInternalPathObserver != null){
            mOldDataInternalPathObserver.stopWatching();
            mOldDataInternalPathObserver = null;
        }
        if(mInternalApkObserver != null){
            mInternalApkObserver.stopWatching();
            mInternalApkObserver = null;
        }
        if(mAppBackupObserver != null){
            mAppBackupObserver.unregisterObservers();
            mAppBackupObserver = null;
        }
        stopListenPackageManager();
        mActivate = false;
        mExec.shutdown();
        mExecFileThread.shutdown();
        mExecAppThread.shutdown();
        mExecUserSelectThread.shutdown();
        mExecScanRestoreThread.shutdown();
        StorageUtil.removeListener(this);
        if(mBackupManagerConnection != null){
            unbindService(mBackupManagerConnection);
        }
        cancelTimerTask();
        getNotificationManager().cancel(TAG_NOTIFICATION, 0);
        /*  SPRD: Bug #458089 Account list haven't been updated after sim card removed @{ */
        if (mBroadcastReceiver != null) {
            unregisterReceiver(mBroadcastReceiver);
        }
        /* @} */
        super.onDestroy();
    }

    private void cancelTimerTask(){
        if(DBG) {Log.d(TAG, "cancelTimerTask()");}
        if(mTimer != null){
            if(DBG) {Log.d(TAG, "mTimer.cancel()");}
            mTimer.cancel();
            mTimer = null;
        }
        if(mTask != null){
            if(DBG) {Log.d(TAG, "mTask.cancel()");}
            mTask.cancel();
            mTask = null;
        }
    }
    @Override
    protected void onResume() {
        super.onResume();
        if(mIsFirstResume){
            mIsFirstResume = false;
            if(DBG) Log.d(TAG, "first onResume()");
            //Fix bug 291712, do not checkAllAppData while first onResume.
            if(mBackupFragment != null && false){
                try{
                    mBackupFragment.checkAllAppData();
                }catch(Exception e){
                    e.printStackTrace();
                }
            }
        } else {
            if(DBG) Log.d(TAG, "onResume()..");
            updateRestoreFragment();
        }
        //fix bug359652,resume need update list
        updateAppListView();
    }
    public void updateRestoreFragment(){
        mHandler.removeMessages(MSG_DATA_FILE_CHANGED);
        mHandler.sendEmptyMessageDelayed(MSG_DATA_FILE_CHANGED, UPDATE_INTERVAL);
        /* SPRD: modify for bug 632148 @{ */
        mHandler.removeMessages(MSG_APK_FILE_CHANGED);
        mHandler.sendEmptyMessageDelayed(MSG_APK_FILE_CHANGED, UPDATE_INTERVAL);
        /* @ { */
    }

    class ScanAgentThread implements Runnable{
        public void run() {
            mDataList = getEnabledAgent();
            mHandler.sendEmptyMessage(MSG_UPDATE_AGENT);
        }
    }
    class LoadRestoreAppThread implements Runnable{
        @Override
        public void run() {
            if(mLoadAppTask != null){
                try{
                    mRestoreApp = mLoadAppTask.loadRestoredAppinfos();
                }catch(Exception e){
                    e.printStackTrace();
                }
                mHandler.sendEmptyMessage(MSG_UPDATE_RESTORE_APP);
            }
        }
    }

    public List<DataItem> getDataList() {
        return mDataList;
    }

    public List<AppInfo> getAppList() {
        return mAppList;
    }

    public Map<String, List<DataItem>> getRestoreDataDetail(){
        return mRestoreDataDetail;
    }
    public Agent[] getAgents() {
        return mAgents;
    }

    @Override
    public void onBackPressed() {
        new AlertDialog.Builder(this)
                .setTitle(this.getString(R.string.exit_title))
                .setMessage(R.string.do_you_wanna_exit_really)
                .setNegativeButton(R.string.cancel, new OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                }).setPositiveButton(R.string.confirm, new OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if(DBG) {Log.d(TAG, "Exit Bakcup by onBackPressed()");}
                        finish();
                    }
                }).create().show();
    }

    public List<AppInfo> getRestoreApp() {
        return mRestoreApp;
    }

    public List<DataItem> getRestoreDataByTimeStamp(String time){
        List<DataItem> listDateItem = DataUtil.getEnabledArchive(this, mService, mAgents, time);
        return listDateItem;
    }
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        if(DBG) {Log.d(TAG, "onSaveInstanceState()");}
//        super.onSaveInstanceState(outState);
    }

    public void restoreApp(List<AppInfo> app, List<DataItem> data, String time){
        if(DBG) Log.d(TAG, "restoreApp()");
        /* SPRD : fix bug256962 @{ */
        if (data.size() > 0) {
            remindUseUserDataDialog(false, data, app, Config.USE_INTERNAL_STORAGE, time);
        } else {
            if(app != null){
                if(DBG) Log.d(TAG, "restoreApp() app = "+ app.toString());
                if(DBG) Log.d(TAG, "restoreApp() timeStamp = "+ time);
                mAppBackupRetainData.restoreAppTask = new RestoreAppTask(app, data, time, mHandler);
                mAppBackupRetainData.restoreAppTask.execute();
            }
        }
        /* @} */

    }

    public void restoreData(List<DataItem> data, String time, ArrayList<Map<String, Object>> restoreResultList,Handler handler){
        if(DBG) Log.d(TAG, "restoreData()");
        if(data != null && time != null){
            if(DBG) Log.d(TAG, "restoreData() timeStamp = "+ time);
            if(DBG) Log.d(TAG, "restoreData() dataList = "+ data);
            mAppBackupRetainData.restoreDataTask = new RestoreDataTask(data, time, restoreResultList, handler);
            mAppBackupRetainData.restoreDataTask.execute();
        }else{
            if(DBG) Log.d(TAG, "restoreData() something is null, data or time");
        }
    }

    private class RestoreAppTask extends AsyncTask<Void, Integer, Integer> {

        List<AppInfo> appList;
        List<DataItem> dataList;
        String timeStamp;
        int progress = 0;
        int max = 1;
        String title = null;
        String numFormat = null;
        Handler taskHandler = null;
        ArrayList<Map<String, Object>> taskRestoreResult = new ArrayList<Map<String, Object>>();

        public void setTaskHandler(Handler handler) {
            this.taskHandler = handler;
        }

        public RestoreAppTask(List<AppInfo> app, List<DataItem> data, String time,Handler handler){
            appList = app;
            dataList = data;
            this.taskHandler = handler;
            timeStamp = time;
            max = appList.size();
        }
        @Override
        protected Integer doInBackground(Void... params) {
            if(DBG) { Log.d(TAG, "RestoreApp.doInBackground()");}
            String tempName = null;
            String tempPathName = null;
            int successCount = 0;
            int status = PackageInstall.SUCCESS;
            for (AppInfo info : appList) {
                if (DBG) {
                    Log.d(TAG, "appList = " + appList);
                    Log.d(TAG, "mCancel = " + mCancel + "--mSdCardUnmounted = " + mSdCardUnmounted);
                }
                // SPRD: bug_412000 judge whether the object is null.
                if (mCancel || (mSdCardUnmounted && (timeStamp != null) && timeStamp.contains(Config.EXTERNAL_STORAGE_PATH))) {
                    /* SPRD: modify for bug 632148 @{ */
                    taskHandler.sendEmptyMessage(MSG_DISMISS_PROGRESSDIALOG);
                    /* @} */
                    return CANCEL_FLAG;
                }
                Map<String, Object> resultMap = new HashMap<String, Object>();
                resultMap.put(AGENT_NAME, info.getName());
                tempName = info.getPackageName();
                tempPathName = info.getPackagePath();
                File apkFile = new File(tempPathName);
                PackageInstall pInstall = new PackageInstall(MainActivity.this, apkFile);
                status = pInstall.install();
                if (DBG) Log.v(TAG, "tempPathName: " + tempPathName + ",tempName: "
                            + tempName + ",status: " + status);
                if(status == PackageInstall.SUCCESS) {
                	status = restoreUserAppData(info.getPackageName(), apkFile.getParent()) ? PackageInstall.SUCCESS : PackageInstall.FAIL;
                }
                if (status != PackageInstall.SUCCESS) {
                    resultMap.put(RESULT, FLAG_FAIL);
                } else {
                    resultMap.put(RESULT, FLAG_SUCCESS);
                }
                // SPRD: bug_412000 judge whether the object is null.
                if(mCancel || (mSdCardUnmounted  && (timeStamp != null) && timeStamp.contains(Config.EXTERNAL_STORAGE_PATH))) {
                    if(DBG) {Log.d(TAG, "mCancel = "+mCancel+"--mSdCardUnmounted = "+mSdCardUnmounted);}
                    /* SPRD: modify for bug 632148 @{ */
                    taskHandler.sendEmptyMessage(MSG_DISMISS_PROGRESSDIALOG);
                    /* @} */
                    return CANCEL_FLAG;
                }
                taskRestoreResult.add(resultMap);
                publishProgress(++successCount);
                progress++;
            }
            return 0;
        }

        private boolean restoreUserAppData(String pkgName, String destPath) {
        	File tarFile = new File(destPath + "/" + pkgName + ".tar");
        	if(!tarFile.exists()) {
        		Log.e(TAG, "in restoreUserAppData, pkgName: " + pkgName + " is not exist, it impossibility!!");
        		return false;
        	} if(tarFile.length() <= 0) {
        		return true;
        	}
        	File destDir = mApp.getBackupUserAppFolder();
        	if(destDir == null) {
        		return false;
        	}
/*
            //for zip and tar
            String fileCode = AGENT_NAME;
            try {
                FileInputStream is = new FileInputStream(tarFile);
                byte[] zipHeader = new byte[ZIP_TYPE_HEADER_SIZE];
                try {
                    is.read(zipHeader, 0, zipHeader.length);
                } catch (IOException e){
                    Log.d(TAG, "in restoreUserAppData, b.length: " + zipHeader.length);
                }
                fileCode = bytesToHexString(zipHeader);
                Log.d(TAG, "in restoreUserAppData, fileCode: " + fileCode);
            } catch (FileNotFoundException e) {
                Log.d(TAG, "in restoreUserAppData, FileNotFoundException:" + tarFile);
            }

            if (fileCode.toLowerCase().startsWith(ZIP_TYPE_HEADER.toLowerCase())) {
                if(!FileUtils.decompression(tarFile, destDir)) {
                    Log.e(TAG, "in restoreUserAppData, decompression: " + tarFile + " failed");
                    return false;
                }
            } else {
                try {
                    Log.d(TAG, "in restoreUserAppData, dearchive: " + tarFile);
                    TarUtils.dearchive(tarFile, destDir);
                } catch (IOException e) {
                    Log.d(TAG, "in restoreUserAppData, dearchive: " + tarFile + " failed");
                    e.printStackTrace();
                    return false;
                }
            }
*/
            if(!FileUtils.decompression(tarFile, destDir)) {
                Log.e(TAG, "in restoreUserAppData, decompression: " + tarFile + " failed");
                return false;
            }
        	File file = new File(destDir, pkgName);
        	FileUtils.setPermissions(file, android.os.FileUtils.S_IRWXU | android.os.FileUtils.S_IRWXG
                    | android.os.FileUtils.S_IRWXO);
        	boolean ret = true;

        	int pmRet = mPm.restoreAppData(file.getPath(), pkgName);
        	if(pmRet != PackageManagerEx.BACKUPAPP_SUCCEED) {
                Log.d(TAG, "call restoreAppData failed, ret value: " + pmRet);
                ret = false;
        	}

        	FileUtils.erase(file);
        	return ret;
        }

        @Override
        protected void onPreExecute() {
            if(DBG) {Log.d(TAG, "RestoreApp.onPreExecute()");}
            super.onPreExecute();
            Message message = new Message();
            message.arg1 = 0;
            message.what = MSG_SET_BACKUP_RESTORE_BUTTON;
            taskHandler.sendMessage(message);
            mCancel = false;
            mInProgress = true;
            acquireWakeLock();
            Notification notification=new Notification.Builder(MainActivity.this).setAutoCancel(true)
                    .setContentTitle(MainActivity.this.getResources().getString(R.string.restore_application))
                    .setContentText(MainActivity.this.getResources().getString(R.string.restore_is_ongoing))
                    .setSmallIcon(R.drawable.icon)
                    .setWhen(System.currentTimeMillis())
                    .setProgress(0,0,false)
                    .setContentIntent(getPendingIntent())
                    .setOngoing(true).build();
            getNotificationManager().notify(TAG, 0, notification);
            title = MainActivity.this.getResources().getString(R.string.restore_application);
            MessageForProgressUpdate msgObj = new MessageForProgressUpdate();
            msgObj.progress = progress;
            msgObj.max = max;
            msgObj.title = title;
            msgObj.numFormat = numFormat;
            Message msg = Message.obtain(taskHandler, MSG_UPDATE_PROGRESS, msgObj);
            taskHandler.sendMessage(msg);
        }

        @Override
        protected void onProgressUpdate(Integer... values) {
            if(DBG) {
                Log.d(TAG, "RestoreApp.onProgressUpdate()");
                Log.d(TAG, "progress values = "+(Integer)values[0]);
            }
            mInProgress = true;
            MessageForProgressUpdate msgObj = new MessageForProgressUpdate();
            progress = (Integer)values[0];
            msgObj.progress = progress;
            msgObj.max = max;
            msgObj.title = title;
            if (progress > 0) {
                numFormat = "%1d/%2d";
                msgObj.numFormat = numFormat;
            }
            Message msg = Message.obtain(taskHandler, MSG_UPDATE_PROGRESS, msgObj);
            taskHandler.sendMessage(msg);

            Notification notification=new Notification.Builder(MainActivity.this)
                    .setWhen(0)
                    .setContentTitle(MainActivity.this.getResources().getString(R.string.restore_application))
                    .setContentText(MainActivity.this.getResources().getString(R.string.restore_is_ongoing))
                    .setSmallIcon(R.drawable.icon)
                    .setProgress(max,progress,false)
                    .setContentIntent(getPendingIntent())
                    .setOngoing(true).build();
            notification.flags |= Notification.FLAG_NO_CLEAR;
            getNotificationManager().notify(TAG, 0, notification);
        }

        @Override
        protected void onPostExecute(Integer result) {
            if(DBG) Log.d(TAG, "RestoreApp.onPostExecute()");
            mAppBackupRetainData.restoreAppTask = null;
            mInProgress = false;
            releaseWakeLock();
            Message message = new Message();
            message.arg1 = 1;
            message.what = MSG_SET_BACKUP_RESTORE_BUTTON;
            taskHandler.sendMessage(message);
            getNotificationManager().cancel(TAG, 0);
            if(result == CANCEL_FLAG){
                taskHandler.sendEmptyMessageDelayed(MSG_CLOSE_PROGRESS_DIALOG, 700);
                if(DBG) Log.d(TAG, "BackupAppTask is cancelled");
                Notification notification=new Notification.Builder(MainActivity.this).setAutoCancel(true)
                        .setWhen(System.currentTimeMillis())
                        .setContentTitle(MainActivity.this.getResources().getString(R.string.data_restore))
                        .setContentText(MainActivity.this.getResources().getString(R.string.restore_is_cancelled))
                        .setProgress(0,0,false)
                        .setContentIntent(getPendingIntent())
                        .setSmallIcon(R.drawable.icon)
                        .setOngoing(false).build();
                getNotificationManager().notify(TAG_NOTIFICATION, 0, notification);
                /* SPRD: Bug 420899 No cancel when backup or restore. @{ */
                taskHandler.sendEmptyMessage(MSG_DISMISS_PROGRESSDIALOG);
                /* @} */
                Toast.makeText(MainActivity.this, R.string.restore_is_cancelled, Toast.LENGTH_SHORT).show();
            }else{
                if(mCancel != true && timeStamp != null && dataList != null && dataList.size() > 0){
                    taskHandler.postDelayed(new Runnable() {
                        public void run() {
                            restoreData(dataList, timeStamp, taskRestoreResult, taskHandler);
                        }
                    }, 700);
                } else {
                    taskHandler.sendEmptyMessageDelayed(MSG_CLOSE_PROGRESS_DIALOG, 700);
                    Notification notification=new Notification.Builder(MainActivity.this).setAutoCancel(true)
                            .setContentTitle(MainActivity.this.getResources().getString(R.string.data_restore))
                            .setSmallIcon(R.drawable.icon)
                            .setWhen(System.currentTimeMillis())
                            .setContentText(MainActivity.this.getResources().getString(R.string.restore_complete))
                            .setProgress(0,0,false)
                            .setContentIntent(getPendingIntent())
                            .setOngoing(false).build();
                    getNotificationManager().notify(TAG_NOTIFICATION, 0, notification);
                    MessageForRestoreResult resultMsg = new MessageForRestoreResult();
                    resultMsg.dataList = null;
                    resultMsg.restoreResult = taskRestoreResult;
                    Message msg = Message.obtain(taskHandler, MSG_DISPLAY_RESTORE_RESULT, resultMsg);
                    taskHandler.sendMessageDelayed(msg, 1000);
                }
            }
        }

        public int getMax() {
            return max;
        }
        public int getProgress() {
            return progress;
        }
        public String getNumFormat() {
            return numFormat;
        }
        public String getTitle() {
            title = MainActivity.this.getResources().getString(R.string.restore_application);
            return title;
        }
    }

    private class RestoreDataTask extends AsyncTask<Void, Object, Integer> {

        List<DataItem> dataList;
        String timeStamp;
        Archive archive;
        int progress = 0;
        int max = 1;
        String title = null;
        String numFormat = null;
        int categoryCode = -1;
        String agentName = null;
        Handler taskHandler = null;
        boolean mSDCardStateBeforeRestore = false;
        ArrayList<Map<String, Object>> taskRestoreResult;

        public void setTaskHandler(Handler handler) {
            this.taskHandler = handler;
        }

        public RestoreDataTask(List<DataItem> checkedData, String time, ArrayList<Map<String, Object>> restoreResultList,Handler handler){
            dataList = checkedData;
            timeStamp = time;
            this.taskHandler = handler;
            agentName = dataList.get(0).getAgentName();
            categoryCode = dataList.get(0).getCategoryCode();
            title = getBackupingString(agentName, categoryCode, false, false);
            if (restoreResultList == null || restoreResultList.size() <= 0) {
                taskRestoreResult = new ArrayList<Map<String, Object>>();
            } else {
                taskRestoreResult = restoreResultList;
            }
            if(DBG) Log.d(TAG, "RestoreDataTask.checkedData = "+ checkedData);
            mSDCardStateBeforeRestore = StorageUtil.getExternalStorageState();
        }

        @Override
        protected Integer doInBackground(Void... params) {
            mIsRestoring = true;
            Log.i(TAG, "retoreTask.timeStamp = " + timeStamp);
            archive = Archive.get(null, timeStamp, true);
            if (null == archive) {
                return CANCEL_FLAG;
            }
            for(DataItem dat:dataList){
                mInPorgressAgentName = dat.getAgentName();
                if(mCancel || (mSdCardUnmounted && timeStamp.contains(Config.EXTERNAL_STORAGE_PATH))) {
                    if (DBG) {
                        Log.d(TAG, "mCancel = " + mCancel + "--mSdCardUnmounted = " + mSdCardUnmounted);
                        Log.d(TAG, "return CANCEL_FLAG at header");
                    }
                    /* SPRD: modify for bug 632148 @{ */
                    taskHandler.sendEmptyMessage(MSG_DISMISS_PROGRESSDIALOG);
                    /* @} */
                    return CANCEL_FLAG;
                }
                if (mSDCardStateBeforeRestore && mSdCardUnmounted && GALLERY_AGENT_NAME.equals(dat.getAgentName())) {
                    return CANCEL_FLAG;
                }
                Map<String, Object> resultRecovery = new HashMap<String, Object>();
                resultRecovery.put(AGENT_NAME, dat.getAgentName());
                resultRecovery.put(CATEGORY_CODE, dat.getCategoryCode());
                agentName = dat.getAgentName();
                categoryCode = dat.getCategoryCode();
                title = getBackupingString(agentName, categoryCode, false, false);
                progress = 0;
                max = 100;
                numFormat = null;
                MessageForProgressUpdate msgObj = new MessageForProgressUpdate();
                msgObj.progress = progress;
                msgObj.max = max;
                msgObj.title = title;
                msgObj.numFormat = numFormat;
                Message msg = Message.obtain(taskHandler, MSG_UPDATE_PROGRESS, msgObj);
                taskHandler.sendMessage(msg);

                AppBackupRestoreObserver observer = new AppBackupRestoreObserver();
                int index = dataList.indexOf(dat);
                int resultCode = 0;
                if(DBG) Log.d(TAG, "doInBackground index = "+ index);
                if(DBG) Log.d(TAG, "doInBackground dat = "+ dat);
                    try {
                        resultCode = mService.requestRestore(archive, dat.getAgent(), observer, dat.getCategoryCode());
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    }
                    if(resultCode == -1) {
                        resultRecovery.put(RESULT, FLAG_FAIL);
                        taskRestoreResult.add(resultRecovery);
                        continue;
                    }
                    int tmp = 0;
                    if(DBG) Log.d(TAG, "first observer = " + observer);
                    while(observer.getmCurrent() < observer.getmTotal()){
                        if(mCancel || (mSdCardUnmounted && timeStamp.contains(Config.EXTERNAL_STORAGE_PATH))) {
                            try {
                                mService.requestCancel(dat.getAgent(), dat.getCategoryCode());
                            } catch (RemoteException e) {
                                e.printStackTrace();
                            }
                            if (DBG) {
                                Log.d(TAG, "mCancel = " + mCancel+ "--mSdCardUnmounted = "+ mSdCardUnmounted);
                                Log.d(TAG, "return CANCEL_FLAG at looper");
                            }
                            return CANCEL_FLAG;
                        }
                        if (mSDCardStateBeforeRestore && mSdCardUnmounted && GALLERY_AGENT_NAME.equals(dat.getAgentName())) {
                            try {
                                mService.requestCancel(dat.getAgent(), dat.getCategoryCode());
                            } catch (RemoteException e) {
                                e.printStackTrace();
                            }
                            return CANCEL_FLAG;
                        }

                        if(tmp != observer.getmCurrent()){
                            if(DBG) Log.d(TAG, "observer = " + observer);
                            publishProgress(observer.getmCurrent(), observer.getmTotal(), observer.getUnit(), dat.getAgentName(), dat.getCategoryCode());
                        }
                        tmp = observer.getmCurrent();
                    }
                    publishProgress(observer.getmTotal(), observer.getmTotal(), observer.getUnit(), dat.getAgentName(), dat.getCategoryCode());
                    resultRecovery.put(RESULT, observer.getResult());
                    taskRestoreResult.add(resultRecovery);
                }
            return 0;
        }

        @Override
        protected void onPostExecute(Integer result) {
            if(DBG) Log.d(TAG, "onPostExecute()");
            mIsRestoring = false;
            mAppBackupRetainData.restoreDataTask = null;
            taskHandler.sendEmptyMessageDelayed(MSG_CLOSE_PROGRESS_DIALOG, 700);
            mInProgress = false;
            releaseWakeLock();
            Message message = new Message();
            message.arg1 = 1;
            message.what = MSG_SET_BACKUP_RESTORE_BUTTON;
            taskHandler.sendMessage(message);
            getNotificationManager().cancel(TAG, 0);
            if(result == CANCEL_FLAG){
                if(DBG) Log.d(TAG, "RestoreDataTask is cancelled");
                Notification notification=new Notification.Builder(MainActivity.this).setAutoCancel(true)
                        .setWhen(System.currentTimeMillis())
                        .setContentTitle(MainActivity.this.getResources().getString(R.string.data_restore))
                        .setContentText(MainActivity.this.getResources().getString(R.string.restore_is_cancelled))
                        .setProgress(0,0,false)
                        .setSmallIcon(R.drawable.icon)
                        .setContentIntent(getPendingIntent())
                        .setOngoing(false).build();
                getNotificationManager().notify(TAG_NOTIFICATION, 0, notification);
                /* SPRD: Bug 420899 No cancel when backup or restore. @{ */
                taskHandler.sendEmptyMessage(MSG_DISMISS_PROGRESSDIALOG);
                /* @} */
                Toast.makeText(MainActivity.this, R.string.restore_is_cancelled, Toast.LENGTH_SHORT).show();
            }else{
                Log.i(TAG, "result=" + result);
                Notification notification=new Notification.Builder(MainActivity.this).setAutoCancel(true)
                        .setWhen(System.currentTimeMillis())
                        .setContentTitle(MainActivity.this.getResources().getString(R.string.data_restore))
                        .setSmallIcon(R.drawable.icon)
                        .setContentText(MainActivity.this.getResources().getString(R.string.restore_complete))
                        .setProgress(0,0,false)
                        .setContentIntent(getPendingIntent())
                        .setOngoing(false).build();
                getNotificationManager().notify(TAG_NOTIFICATION, 0, notification);
                //displayRestoreResult(dataList);
                MessageForRestoreResult resultMsg = new MessageForRestoreResult();
                resultMsg.dataList = dataList;
                resultMsg.restoreResult = taskRestoreResult;
                Message msg = Message.obtain(taskHandler, MSG_DISPLAY_RESTORE_RESULT, resultMsg);
                taskHandler.sendMessageDelayed(msg, 1000);
                taskHandler.removeMessages(AppBackupObserver.CONTENT_CHANGE);
                taskHandler.sendEmptyMessageDelayed(AppBackupObserver.CONTENT_CHANGE, UPDATE_INTERVAL);
            }
        }

        @Override
        protected void onPreExecute() {
            if(DBG) Log.d(TAG, "RestoreDataTask.onPreExecute()");
            Message message = new Message();
            message.arg1 = 0;
            message.what = MSG_SET_BACKUP_RESTORE_BUTTON;
            taskHandler.sendMessage(message);
            mInProgress = true;
            title = getBackupingString(agentName, categoryCode, false, false);
            MessageForProgressUpdate msgObj = new MessageForProgressUpdate();
            msgObj.progress = progress;
            msgObj.max = max;
            msgObj.title = title;
            msgObj.numFormat = numFormat;
            Message msg = Message.obtain(taskHandler, MSG_UPDATE_PROGRESS, msgObj);
            taskHandler.sendMessage(msg);
            mCancel = false;
            Notification notification=new Notification.Builder(MainActivity.this).setAutoCancel(true)
                    .setContentTitle(MainActivity.this.getResources().getString(R.string.data_restore))
                    .setContentText(MainActivity.this.getResources().getString(R.string.restore_is_ongoing))
                    .setSmallIcon(R.drawable.icon)
                    .setWhen(System.currentTimeMillis())
                    .setProgress(0,0,false)
                    .setContentIntent(getPendingIntent())
                    .setOngoing(true).build();
            getNotificationManager().notify(TAG, 0, notification);
            acquireWakeLock();
        }

        @Override
        protected void onProgressUpdate(Object... values) {
            if(DBG) Log.d(TAG, "RestoreDataTask.onProgressUpdate() ");
            mInProgress = true;
            categoryCode = (Integer)values[4];
            agentName = (String)values[3];
            title = getBackupingString(agentName, categoryCode, false, false);
            MessageForProgressUpdate msgObj = new MessageForProgressUpdate();
            progress = (Integer)values[0];
            max =  (Integer) values[1];
            msgObj.progress = progress;
            msgObj.max = max;
            msgObj.title = title;
            if (progress > 0) {
                String unit = (String) values[2];
                numFormat = "%1d" + unit + "/%2d" + unit;
                msgObj.numFormat = numFormat;
            }
            Message msg = Message.obtain(taskHandler, MSG_UPDATE_PROGRESS, msgObj);
            taskHandler.sendMessage(msg);

            Notification notification=new Notification.Builder(MainActivity.this)
                    .setContentTitle(MainActivity.this.getResources().getString(R.string.data_restore))
                    .setContentText(MainActivity.this.getResources().getString(R.string.restore_is_ongoing))
                    .setSmallIcon(R.drawable.icon)
                    .setContentText(title)
                    .setWhen(0)
                    .setProgress((Integer)values[1],(Integer)values[0],false)
                    .setContentIntent(getPendingIntent())
                    .setOngoing(true).build();
            notification.flags |= Notification.FLAG_NO_CLEAR;
            getNotificationManager().notify(TAG, 0, notification);
        }
        public int getMax() {
            return max;
        }
        public int getProgress() {
            return progress;
        }
        public String getNumFormat() {
            return numFormat;
        }
        public String getTitle() {
            title = getBackupingString(agentName, categoryCode, false, false);
            return title;
        }

    }

    public void setInvisibleIndicator(int id){
        if(mRestoreFragment != null){
            mRestoreFragment.setInvisibleIndicator(id);
        } else {
            Log.e(TAG,"setInvisibleIndicator,mRestoreFragment is NULL");
        }
    }

    class DeduplicateTask extends AsyncTask<Void, Void, Integer>{

        List<DataItem> dataDup;
        ProgressDialog progDlg;
        AppBackupRestoreObserver observer;
        public DeduplicateTask(List<DataItem> data){
            dataDup = data;
            progDlg = new ProgressDialog(MainActivity.this);
        }
        @Override
        protected Integer doInBackground(Void... params) {
            if(DBG) {Log.d(TAG, "DeduplicateTask.doInBackground()");}

            for(DataItem di:dataDup){
                observer = new AppBackupRestoreObserver();
                Map<String, Object> resultRecovery = new HashMap<String, Object>();
                resultRecovery.put(AGENT_NAME, di.getAgentName());
                resultRecovery.put(CATEGORY_CODE, di.getCategoryCode());
                try{
                    if(DBG) {Log.d(TAG, "DeduplicateTask current DataItem = "+di);}
                    mService.requestDeduplicate(di.getAgent(), observer, di.getCategoryCode());
                    if(DBG){Log.d(TAG, "requestDeduplicate observer.getResult() ="+observer.getResult());}
                    while(observer.getResult() != AppBackupRestoreObserver.FLAG_DUPLICATION_SUCCESS
                            && observer.getResult() != AppBackupRestoreObserver.FLAG_DUPLICATION_UNSUPPORT){
                        //wait for finished.
                        //add some log for debug for 607313
                        Log.d(TAG, "observer.getResult() ="+observer.getResult());
                    }
                    resultRecovery.put(RESULT, observer.getResult());
                    mDuplicationResault.add(resultRecovery);
                }catch(Exception e){
                    if(DBG) {Log.d(TAG, "DeduplicateTask happen exception!");}
                    e.printStackTrace();
                    return -1;
                }
            }
            /* SPRD: Bug 420899 No cancel when backup or restore. @{ */
            mHandler.sendEmptyMessage(MSG_DISMISS_PROGRESSDIALOG);
            /* @} */
            return 0;
        }

        @Override
        protected void onPostExecute(Integer result) {
            if(progDlg != null){
                progDlg.dismiss();
                progDlg = null;
            }
            displayDuplicationResault();
        }

        @Override
        protected void onPreExecute() {
            if(progDlg != null){
                progDlg.setProgressStyle(ProgressDialog.STYLE_SPINNER);
                progDlg.setMessage(getString(R.string.please_holding));
                progDlg.setCanceledOnTouchOutside(false);
                progDlg.setCancelable(false);
                progDlg.show();
            }
        }
    }

    class MessageForProgressUpdate {
        int progress;
        int max;
        String title = null;
        String numFormat = null;
    }

    class MessageForRestoreResult {
        List<DataItem> dataList = null;
        ArrayList<Map<String, Object>> restoreResult = null;
    }

    class AppBackupRetainData {
        BackupDataTask backupDataTask = null;
        BackupAppTask backupAppTask = null;
        RestoreDataTask restoreDataTask = null;
        RestoreAppTask restoreAppTask = null;
    }

    private String getBackupingString(String agentName, int categoryCode, boolean isBackup, boolean isBackupRestoreFinished) {
        String title = null;
        Log.i(TAG, "agentName==" + agentName);
        Log.i(TAG, "categoryCode==" + categoryCode);

        if (isBackupRestoreFinished) {
            title = "";
        } else {
            if (isBackup) {
                title = getResources().getString(R.string.backup_is_ongoing) + " ";
            } else {
                title = getResources().getString(R.string.restore_is_ongoing) + " ";
            }
        }
        if (CALENDAR_AGENT_NAME.equals(agentName)) {
            title += getResources().getString(R.string.calendar_agent_name);
        } else if (GALLERY_AGENT_NAME.equals(agentName)) {
            title += getResources().getString(R.string.gallery_agent_name);
        } else if (CONTACTS_AGENT_NAME.equals(agentName)) {
            title += getResources().getString(R.string.contacts_agent_name);
        } else if (MMS_AGENT_NAME.equals(agentName)) {
            if (0 == categoryCode) {
                //sms
                title += getResources().getString(R.string.mms_agent_sms_name);
            } else if (1 == categoryCode) {
                //mms
                title += getResources().getString(R.string.mms_agent_mms_name);
            } else {
                title += getResources().getString(R.string.mms_agent_name);
            }
        } else {
            title = agentName;
        }
        return title;
    }

    private void scanArchives() {
        if (DBG) {
            Log.e(TAG, "scanArchives()");
        }

        String storePath = null;
        File rootFile = null;

        Log.e(TAG, "scanArchives() scanSize = " + mRestoreSelectedFile.size());
        if (mRestoreSelectedFile.size() != 0 /*&& Config.USE_STORAGE == Config.USE_DEFINED_PATH*/) {
            /* SPRD: Bug 440295 appbackup can not delete. @{  */
            ArrayList<String> newRestoreSelectedFile = (ArrayList) ((ArrayList)mRestoreSelectedFile).clone();
            for (String archive: newRestoreSelectedFile) {
                //storePath = archive.getAbsolutePath();
                Log.e(TAG, "scanArchives() storePath = " + archive);
                if (!scanArchiveList(archive)){
                    continue;
                }
            }
            /* @} */
        } else {
            for (int i = 0; i < 2; i++) {
                if (0 == i) {
                    if(Config.IS_NAND || !StorageUtil.getInternalStorageState()) {
                        continue;
                    }
                    storePath = Config.INTERNAL_ARCHIVE_ROOT_PATH;
                } else if (1 == i) {
                    if(!StorageUtil.getExternalStorageState()) {
                        continue;
                    }
                    storePath = Config.EXTERNAL_ARCHIVE_ROOT_PATH;
                } else {
                    return;
                }

                if (!scanArchiveList(storePath)){
                    continue;
                }
                /*
                rootFile = new File(storePath);
                if (!rootFile.exists()) {
                    continue;
                }
                if (!rootFile.isDirectory()) {
                    continue;
                }
                if (mArchivesList == null) {
                    mArchivesList = new ArrayList<Archive>();
                }
                File[] files = rootFile.listFiles();

                if (files == null) {
                    continue;
                }
                for (File f : files) {
                    if (!f.isDirectory()) {
                        continue;
                    }
                    Archive a = Archive.get(storePath, f.getName(), false);
                    if (a == null) {
                        continue;
                    }
                    mArchivesList.add(a);
                    if (DBG) {
                        Log.e(TAG, "scanArchives: " + a);
                    }
                }
                */
                if (DBG) {
                    Log.e(TAG, "scanArchives() end");
                }
            }
        }

        for (int i = 0; i < 2; i++) {
            if (0 == i) {
                if(Config.IS_NAND || !StorageUtil.getInternalStorageState()) {
                    continue;
                }
                storePath = Config.OLD_VERSION_DATA_PATH_INTERNAL;
            } else if (1 == i) {
                if(!StorageUtil.getExternalStorageState()) {
                    continue;
                }
                storePath = Config.OLD_VERSION_DATA_PATH_EXTERNAL;
            } else {
                return;
            }
            rootFile = new File(storePath);
            if (!rootFile.exists()) {
                continue;
            }
            if (!rootFile.isDirectory()) {
                continue;
            }
            if (mArchivesList == null) {
                mArchivesList = new ArrayList<Archive>();
            }
            File[] files = rootFile.listFiles();
            /* SPRD: Bug 411440 files may be null if unmount sdcard. @{ */
            if (files == null){
                continue;
            }
            /* @} */
            for (File f : files) {
                if (!f.getAbsolutePath().endsWith(".zip") || f.isDirectory()) {
                    continue;
                }
                DataUtil.unPackageZip(f, storePath);
            }
            files = rootFile.listFiles();
            for (File f : files) {
                if (!f.isDirectory()) {
                    continue;
                }
                Archive a = Archive.get(storePath, f.getName(), false);
                if (a == null) {
                    continue;
                }
                mArchivesList.add(a);
            }
        }
    }

    private boolean scanArchiveList(String storePath) {
        File rootFile = new File(storePath);

        Log.e(TAG, "scanArchiveList() storePath = " + storePath);
        if (!rootFile.exists()) {
            Log.e(TAG, "scanArchiveList() !rootFile.exists()");
            return false;
        }
        if (!rootFile.isDirectory()) {
            Log.e(TAG, "scanArchiveList() !rootFile.isDirectory()");
            return false;
        }
        if (mArchivesList == null) {
            mArchivesList = new ArrayList<Archive>();
        }
        File[] files = rootFile.listFiles();

        if (files == null) {
            return false;
        }
        for (File f : files) {
            if (!f.isDirectory()) {
                continue;
            }
            Archive a = Archive.get(storePath, f.getName(), false);
            Log.e(TAG, "scanArchiveList() a = " + a);
            if (a == null) {
                continue;
            }
            mArchivesList.add(a);
            if (DBG) {
                Log.e(TAG, "scanArchiveList: " + a);
            }
        }
        return true;
    }
    public Archive addArchive(String rootPath) {
        Archive archive = Archive.get(rootPath);
        if (null != archive) {
            mArchivesList.add(archive);
        }
        return archive;
    }

    /* SPRD : fix bug256962 @{ */
    public void remindUseUserDataDialog(final boolean isBackup, final List<DataItem> dataList, final List<AppInfo> app, final int whichStorage, final String time) {

        if (dataList == null || dataList.size() < 1) {
            return;
        }
        String title = "";
        String msgStr = "";
        String agentsString = "";
        int size = dataList.size();
        for (int i = 0; i < size; i++) {
            DataItem dataItem = dataList.get(i);
            if (dataItem == null) {
                continue;
            }
            agentsString += getBackupingString(dataItem.getAgentName(), dataItem.getCategoryCode(), isBackup, true);
            if (i == size - 1) {
                agentsString += " " + this.getString(R.string.remind_data);
            } else {
                agentsString += "/";
            }
        }
        if (isBackup) {
            title = this.getString(R.string.remind_read_userdata_title);
            msgStr = this.getString(R.string.remind_read_userdata_message);
        } else {
            title = this.getString(R.string.remind_write_userdata_title);
            msgStr = this.getString(R.string.remind_write_userdata_message);
        }
        msgStr += " " + agentsString;
        new AlertDialog.Builder(this)
        .setTitle(title)
        .setMessage(msgStr)
        .setNegativeButton(R.string.cancel, new OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        }).setPositiveButton(R.string.confirm, new OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int which) {
                if (isBackup) {
                    /* SPRD: 453980 check if backup path is in external storage @{ */
                    checkIsSelectExternalStorage(whichStorage);
                    startObserverTask(!Config.IS_SELECT_EXTERNAL_STORAGE);
                    /* @} */

                    if(app.size()>0){
                        mAppBackupRetainData.backupAppTask = new BackupAppTask(app, dataList, mHandler, whichStorage);
                        mAppBackupRetainData.backupAppTask.execute();
                    }else{
                        backupData(dataList, null, mHandler, whichStorage);
                    }
                } else {
                    if(app != null){
                        if(DBG) Log.d(TAG, "restoreApp() app = "+ app.toString());
                        if(DBG) Log.d(TAG, "restoreApp() timeStamp = "+ time);
                        mAppBackupRetainData.restoreAppTask = new RestoreAppTask(app, dataList, time, mHandler);
                        mAppBackupRetainData.restoreAppTask.execute();
                    }else{
                        restoreData(dataList, time, null, mHandler);
                        if(DBG) Log.d(TAG, "restoreApp() app, data, time is null at least");
                    }
                }
            }
        }).create().show();
    }
    /* @} */

    /* SPRD:Optimization to erase the animation on click. @{*/
    private void remindUserFirstEntry() {
        SharedPreferences firstEntry = getSharedPreferences(REMINDUSER_FIRST, 0);
        if (firstEntry.getBoolean(REMINDUSER_FIRST, false)) {
            if(DBG) Log.d(TAG, "remindUserFirstEntry: REMINDUSER_FIRST true");
            // sprd: 476980 check and request permissions
            checkAndRequestPermissions();
            return;
        } else {
            if(DBG) Log.d(TAG, "remindUserFirstEntry: REMINDUSER_FIRST false");
            firstEntry.edit().putBoolean(REMINDUSER_FIRST, true).apply();
        }

        new AlertDialog.Builder(this)
        .setTitle(this.getString(R.string.app_name))
        .setMessage(this.getString(R.string.first_entry_alert))
        .setPositiveButton(R.string.confirm, new OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int which) {
                // sprd: 476980 check and request permissions
                checkAndRequestPermissions();
                dialog.dismiss();
            }
        }).create().show();
    }
    /* @} */

    public static String bytesToHexString(byte[] src) {
        StringBuilder stringBuilder = new StringBuilder();
        if (src == null || src.length <= 0) {
            return null;
        }
        for (int i = 0; i < src.length; i++) {
            int v = src[i] & 0xFF;
            String hv = Integer.toHexString(v);
            if (hv.length() < 2) {
                stringBuilder.append(0);
            }
            stringBuilder.append(hv);
        }
        return stringBuilder.toString();
    }

    public void scanBackupFileList() {
        mLoadAppTask = new LoadAppTask(0, getApplicationContext());
        mAppList = mLoadAppTask.loadInstalledAppinfos();
        mRestoreApp = mLoadAppTask.loadRestoredAppinfos();
        mDataList = getEnabledAgent();
        mRestoreDataDetail = getArchive();
        mHandler.sendEmptyMessage(MSG_DATA_AND_APP_LOAD_FINISH);
        Log.d(TAG, "scanBackupFileList(), notify onAppLoadFinished ! mRestoreApp = " + mRestoreApp);
    }

    public void scanUserSelectBackupFileList() {
        mLoadAppTask = new LoadAppTask(0, getApplicationContext());
        mAppList = mLoadAppTask.loadInstalledAppinfos();

        try{
            if(mExecUserSelectThread != null && !mExecUserSelectThread.isShutdown()){
                mExecUserSelectThread.execute(new ScanUserSelectBackupThread());
            }
        }catch(RejectedExecutionException e){
            e.printStackTrace();
        }
        if(DBG) Log.d(TAG, "time_cal scanUserSelectBackupFileList end");
    }

    public List<String> getScanRestoreFile(String rootPath) {
        try {
            Thread.sleep(200);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        /*scan all disk, if user cancel scan, we should return null*/
        if (mScanAllCancel) {
            mRestoreSelectedFile.clear();
            return mRestoreSelectedFile;
        }
//        List<File> filesList = new ArrayList<File>();
        Uri uri = MediaStore.Files.getContentUri("external");
        String[] projection = new String[] {
            MediaStore.Files.FileColumns.DATA
        };
        String selection = "(" + MediaStore.Files.FileColumns.DATA
                + " like '%" + "/backup/" + "%' )";
        if (rootPath != null) {
            selection += " AND " + MediaStore.Files.FileColumns.DATA
                    + " like '" + rootPath + "%'";
        }
        if (DBG) {Log.d(TAG, "getScanRestoreFile selection = "+ selection + "  projection" + projection);}
        selection += " COLLATE NOCASE";

        Cursor cursor = null;
        try{
            cursor = getApplicationContext().getContentResolver().query(uri, projection, selection, null, MediaColumns.TITLE + " collate nocase, " + MediaColumns.DATA + " collate nocase");
        }catch(Exception e){
            Log.e(TAG,"query database unkown exception");
        }
        if (cursor == null) {
            return mRestoreSelectedFile;
        }
        if (cursor.getCount() == 0) {
            cursor.close();
            return mRestoreSelectedFile;
        }
        if (DBG) {Log.d(TAG, "getScanRestoreFile mRestoreSelectedFile = "+ mRestoreSelectedFile);}

        while (cursor.moveToNext()) {
            String filePath = cursor.getString(0);
            File file = new File(filePath);
            if (DBG) {Log.d(TAG, "getScanRestoreFile filePath = " + filePath);}
            if(file.isHidden()) {
                continue;
            }
            if (file.isDirectory() && (filePath.endsWith("/backup/Data") || filePath.endsWith("/backup/App"))) {
                filePath = filePath + "/";
                /* SPRD: 445202 avoid duplicate path @{ */
                if (mRestoreSelectedFile.contains(filePath)) {
                    continue;
                }
                /* @} */
                mRestoreSelectedFile.add(filePath);
                if (DBG) {Log.d(TAG, "getScanRestoreFile file.getName() = "+ file.getName() + " filePath = " + filePath);}
            }
        }
        cursor.close();

        if (DBG) {Log.d(TAG, "getScanRestoreFile end mRestoreSelectedFile = "+ mRestoreSelectedFile);}

        return mRestoreSelectedFile;
    }

    public boolean getScanAllStatus() {
        return mScanAllCancel;
    }

    public void setScanAllStatus(boolean isScanAll) {
        mScanAllCancel = isScanAll;
    }

    /* SPRD: 476980 check/request permissions {@ */
    @Override
    protected Dialog onCreateDialog(int id, Bundle args) {
        if (mDialog != null && mDialog.isShowing()) {
            mDialog.dismiss();
        }
        switch (id) {
        case MISS_PERMISSIONS_ALERT_DIALOG:
            final AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this)
                    .setTitle(R.string.app_name)
                    .setMessage(R.string.permission_missed_hint)
                    .setPositiveButton(R.string.confirm,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                    finish();
                                }
                            })
                    .setCancelable(false);
            mDialog = builder.create();
            return mDialog;
        }

        mDialog = super.onCreateDialog(id, args);
        return mDialog;
    }

    public boolean checkAndRequestPermissions() {
        ArrayList<String> permssionsToRequest = new ArrayList<String>();

        if (checkSelfPermission(permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            permssionsToRequest.add(permission.WRITE_EXTERNAL_STORAGE);
        }

        if (checkSelfPermission(permission.READ_CONTACTS) != PackageManager.PERMISSION_GRANTED) {
            permssionsToRequest.add(permission.READ_CONTACTS);
        }

        if (checkSelfPermission(permission.READ_CALENDAR) != PackageManager.PERMISSION_GRANTED) {
            permssionsToRequest.add(permission.READ_CALENDAR);
        }

        if (checkSelfPermission(permission.READ_SMS) != PackageManager.PERMISSION_GRANTED) {
            permssionsToRequest.add(permission.READ_SMS);
        }

        if (checkSelfPermission(permission.READ_PHONE_STATE) != PackageManager.PERMISSION_GRANTED) {
            permssionsToRequest.add(permission.READ_PHONE_STATE);
        }

        if (!permssionsToRequest.isEmpty()) {
            requestPermissions((String[])permssionsToRequest.toArray(new String[0]), PERMISSION_REQUEST_CODE);
            permssionsToRequest.clear();
            return false;
        }

        return true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
             String[] permissions, int[] grantResults) {
        switch (requestCode) {
        case PERMISSION_REQUEST_CODE:
            if (DBG) Log.d(TAG, "onRequestPermissionsResult(): permissions are "
                + Arrays.toString(permissions) + ", result is " + Arrays.toString(grantResults));
            boolean resultsAllGranted = true;
            if (grantResults.length > 0 ) {
                for (int result : grantResults) {
                     if (PackageManager.PERMISSION_GRANTED !=  result) {
                         resultsAllGranted = false;
                         break;
                     }
                }
            } else {
                resultsAllGranted = false;
            }

            if (!resultsAllGranted) {
                showDialog(MISS_PERMISSIONS_ALERT_DIALOG);
            }
            break;
        default:
            break;
        }
    }
    /* @} */
}
