package com.sprd.appbackup.activities;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.FileObserver;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.CheckBox;
import android.widget.ExpandableListView;
import android.widget.ExpandableListView.OnChildClickListener;
import android.widget.Toast;

import com.sprd.appbackup.AppInfo;
import com.sprd.appbackup.DeleteListAdapter;
import com.sprd.appbackup.R;
import com.sprd.appbackup.appbackup.LoadAppTask;
import com.sprd.appbackup.service.Archive;
import com.sprd.appbackup.service.Config;
import com.sprd.appbackup.utils.DataUtil;
import com.sprd.appbackup.utils.StorageUtil;

import android.widget.TextView;

public class DeleteActivity extends Activity implements StorageUtil.StorageChangedListener{

    private static final boolean DBG = true;
    private static final String TAG = "DeleteActivity";
    private static final int FAIL = -1;
    private static final int SUCCESS = 0;
    private static final int MESSAGE_LIST_UPDATE = 10;
    private static final int MESSAGE_RESTORE_DATA_CHANGED = 11;
    private static final int MESSAGE_SOURCE_LOAD = 12;
    /* SPRD: Bug 412731 com.sprd.appbackup stopped unexpectedly. @{ */
    private static final int MESSAGE_PROGRESS_RELEASED = 13;
    /* @} */
    private ExpandableListView mListView;
    private CheckBox mCheckbox;
    private TextView mSelectText;
    private List<AppInfo> mRestoredApp;
    private List<String> mRestoredFolder;
    private List<String> mHeader;
    private DeleteListAdapter mAdapter;
    private boolean mDeleteEnabled = false;
    private boolean mSdCardUnmounted;

//    private boolean mIsDeleting;
    private ProgressDialog mProgress;
    private Intent mIntent;
    private ExecutorService mExecAppThread = Executors.newSingleThreadExecutor();
    /* SPRD: Bug 412731 com.sprd.appbackup stopped unexpectedly. @{ */
    private ProgressDialog mProgressDialog;
    /* @} */
    public Handler mHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
            case MESSAGE_LIST_UPDATE:
                if(DBG) Log.d(TAG, "MESSAGE_LIST_UPDATE");
                if (null != mAdapter) {
                    mAdapter.notifyDataSetChanged();
                    updateMenu();
                    if(mAdapter.getCheckedSize() == (mAdapter.getChildrenCount(0)+mAdapter.getChildrenCount(1))){
                        mCheckbox.setChecked(true);
                        mSelectText.setText(R.string.cancle_select_all);
                    }else{
                        mCheckbox.setChecked(false);
                        mSelectText.setText(R.string.select_all);
                    }
                }

                break;
            case MESSAGE_RESTORE_DATA_CHANGED:
                if(DBG) Log.d(TAG, "MESSAGE_RESTORE_DATA_CHANGED");
                try{
                    mExecAppThread.execute(new LoadRestoreDataThread());
                }catch(RejectedExecutionException e){
                    e.printStackTrace();
                }
                break;
            case MESSAGE_SOURCE_LOAD:
                if(mAdapter != null){
                    mListView.setAdapter(mAdapter);
                    if(mAdapter.getGroupCount() == 2){
                        mListView.expandGroup(0);
                        mListView.expandGroup(1);
                    }
                    mCheckbox.setEnabled(true);
                }
                break;
            /* SPRD: Bug 412731 com.sprd.appbackup stopped unexpectedly. @{ */
            case MESSAGE_PROGRESS_RELEASED:
              if(null != mProgressDialog && mProgressDialog.isShowing()){
                  mProgressDialog.dismiss();
                  mProgressDialog = null;
              }
              /* @} */
                break;
            default :
                break;
            }
        }
    };

    class LoadRestoreDataThread implements Runnable{

        @Override
        public void run() {
            /* SPRD: 459529 check if mRestoredFolder and mRestoredApp is null or empty @{ */
            if (mRestoredFolder == null || mRestoredFolder.isEmpty()) {
                mRestoredFolder = getInternalAndExternalFolder();
            }
            /* @} */

            if(mRestoredApp != null){
                try{
                    mRestoredApp = new LoadAppTask(0, DeleteActivity.this).loadRestoredAppinfos();
                }catch(Exception e){
                    e.printStackTrace();
                }
            }
            if (null != mAdapter) {
                if (null != mRestoredApp) {
                    mAdapter.upadteAppList(mRestoredApp);
                    mHandler.sendEmptyMessage(MESSAGE_LIST_UPDATE);
                }
                if (null != mRestoredFolder) {
                    mAdapter.updateFolderList(mRestoredFolder);
                    mHandler.sendEmptyMessage(MESSAGE_LIST_UPDATE);
                }
            }
            /* SPRD: Bug 412731 com.sprd.appbackup stopped unexpectedly. @{ */
            mHandler.sendEmptyMessage(MESSAGE_PROGRESS_RELEASED);
            /* @} */
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.delete_activity);
        mIntent = getIntent();
        StorageUtil.setStorageChangeListener(this);
        Object obj = this.getLastNonConfigurationInstance();
        if (null != obj && (obj instanceof DeleteListAdapter)) {
            mAdapter = (DeleteListAdapter)obj;
        }
        new Thread(){

            @Override
            public void run() {
                loadSource();
                mHandler.sendEmptyMessage(MESSAGE_SOURCE_LOAD);
            }

        }.start();
        mProgress = new ProgressDialog(DeleteActivity.this);
        mListView = (ExpandableListView) findViewById(R.id.expandablelist_delete);
        mSelectText = (TextView) findViewById(R.id.select_text);
        mSelectText.setText(R.string.select_all);
        mCheckbox = (CheckBox) findViewById(R.id.chx_delete_select_all);
        mCheckbox.setEnabled(false);
        mCheckbox.setOnClickListener(new OnClickListener(){

            @Override
            public void onClick(View v) {
                if (v instanceof CheckBox) {
                    boolean isChecked = ((CheckBox)v).isChecked();
                    mAdapter.checkAll(isChecked);
                    if (isChecked) {
                        mSelectText.setText(R.string.cancle_select_all);
                    } else {
                        mSelectText.setText(R.string.select_all);
                    }
                    updateMenu();
                }
            }
        });
        mListView.setOnChildClickListener(new OnChildClickListener(){

            @Override
            public boolean onChildClick(ExpandableListView parent, View v,
                    int groupPosition, int childPosition, long id) {
                if(mAdapter.getStatus(groupPosition, childPosition)){
                    mAdapter.setStatus(groupPosition, childPosition, false);
                }else{
                    mAdapter.setStatus(groupPosition, childPosition, true);
                }
                if(mAdapter.getCheckedSize() == (mAdapter.getChildrenCount(0)+mAdapter.getChildrenCount(1))){
                    mCheckbox.setChecked(true);
                    mSelectText.setText(R.string.cancle_select_all);
                }else{
                    mCheckbox.setChecked(false);
                    mSelectText.setText(R.string.select_all);
                }
                updateMenu();
                return false;
            }
        });
        final ActionBar actionBar = getActionBar();
        actionBar.setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP, ActionBar.DISPLAY_HOME_AS_UP);
    }

    @Override
    protected void onResume() {
        // TODO Auto-generated method stub
        super.onResume();
        /* SPRD: Bug 412731 com.sprd.appbackup stopped unexpectedly. @{ */
        if (mProgressDialog != null && !mProgressDialog.isShowing()) {
            mProgressDialog.show();
        } else if( mProgressDialog == null ) {
            mProgressDialog = new ProgressDialog(DeleteActivity.this);
            mProgressDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
            mProgressDialog.setMessage(getString(R.string.please_holding));
            mProgressDialog.setCancelable(false);
            mProgressDialog.show();
        }
        /* @} */
        mHandler.removeMessages(MESSAGE_RESTORE_DATA_CHANGED);
        mHandler.sendEmptyMessageDelayed(MESSAGE_RESTORE_DATA_CHANGED, 500);
    }

    @Override
    public Object onRetainNonConfigurationInstance() {
        // TODO Auto-generated method stub
        if (null != mAdapter) {
            return mAdapter;
        }
        return super.onRetainNonConfigurationInstance();
    }

    private void updateMenu(){
        if(mAdapter.getCheckedSize()==0){
            mDeleteEnabled = false;
        }else{
            mDeleteEnabled = true;
        }
        invalidateOptionsMenu();
    }
    private List<String> getFolder(String filePath){
        if (filePath == null) {
            return null;
        }
        List<String> folder = new ArrayList<String>();
        File root = new File(filePath);
        if (!root.exists()) {
            return null;
        }
        if (!root.isDirectory()) {
            return null;
        }
        File[] zipFiles = root.listFiles();
        for (File f : zipFiles) {
            if (!f.getAbsolutePath().endsWith(".zip") || f.isDirectory()) {
                continue;
            }
            DataUtil.unPackageZip(f, filePath);
        }

        File[] files = root.listFiles();
        for (File f : files) {
            if (!f.isDirectory()) {
                continue;
            }
            Archive a = Archive.get(filePath, f.getName(), false);
            if (a == null) {
                continue;
            }
            File[] archiveFile = f.listFiles();
            boolean fileAvailable = false;

            for (File file : archiveFile) {
                if (!file.isDirectory()) {
                    continue;
                }
                String fileName = file.getName();
                if (fileName == null) {
                    continue;
                }
                File[] backupFileList = file.listFiles();
                if ("Gallery".equals(fileName)) {
                    for (File backupFile : backupFileList) {
                        if (backupFile.isFile()) {
                            if (backupFile.getName().endsWith(".zip")
                                    && backupFile.length() > 0) {
                                fileAvailable = true;
                                break;
                            }
                        }
                    }
                } else if ("Calendar".equalsIgnoreCase(fileName)) {
                    for (File backupFile : backupFileList) {
                        if (backupFile.isFile()) {
                            if (backupFile.getName().endsWith(".vcs")
                                    && backupFile.length() > 0) {
                                fileAvailable = true;
                                break;
                            }
                        }
                    }
                } else if ("Contact".equals(fileName) || "contacts".equals(fileName)) {
                    for (File backupFile : backupFileList) {
                        if (backupFile.isFile()) {
                            if (backupFile.getName().endsWith(".vcf")
                                    && backupFile.length() > 0) {
                                fileAvailable = true;
                                break;
                            }
                        }
                    }
                } else if ("Mms".equalsIgnoreCase(fileName)) {
                    for (File backupFile : backupFileList) {
                        if (backupFile.isFile()) {
                            if (backupFile.getName().endsWith(".pdu")
                                    && backupFile.length() > 0) {
                                fileAvailable = true;
                                break;
                            }
                        }
                    }
                } else if ("Sms".equalsIgnoreCase(fileName)) {
                    for (File backupFile : backupFileList) {
                        if (backupFile.isFile()) {
                            if ((backupFile.getName().endsWith(".vmsg") || backupFile.getName().startsWith("sms"))
                                    && backupFile.length() > 0) {
                                fileAvailable = true;
                                break;
                            }
                        }
                    }
                }
                if (fileAvailable) {
                    break;
                }
            }
            if (fileAvailable) {
                folder.add(filePath + f.getName());

            }
        }
        return folder;
    }

    private List<String> getInternalAndExternalFolder() {
        List<String> tempFolder = null;
        List<String> folder = new ArrayList<String>();
        if (StorageUtil.getExternalStorageState()) {
            tempFolder = getFolder(Config.EXTERNAL_ARCHIVE_ROOT_PATH);
            if (null != tempFolder) {
                folder.addAll(tempFolder);
            }
            tempFolder = getFolder(Config.OLD_VERSION_DATA_PATH_EXTERNAL);
            if (null != tempFolder) {
                folder.addAll(tempFolder);
            }

            /* SPRD:453154 query defined backup path @{ */
            SharedPreferences sharedPreferences = DeleteActivity.this.getSharedPreferences(Config.SHAREPREFERENCE_FILE_NAME, Context.MODE_PRIVATE);
            String definedPath = sharedPreferences.getString(Config.DEFINED_BACKUP_PATH_KEY, "");
            if (!definedPath.isEmpty()) {
                definedPath = definedPath + "/backup/Data/";
                tempFolder = getFolder(definedPath);
                if (null != tempFolder) {
                    folder.addAll(tempFolder);
                }
            }
            /* @} */
        }

        if (!Config.IS_NAND && StorageUtil.getInternalStorageState()) {
            tempFolder = getFolder(Config.INTERNAL_ARCHIVE_ROOT_PATH);
            if (null != tempFolder) {
                folder.addAll(tempFolder);
            }
            tempFolder = getFolder(Config.OLD_VERSION_DATA_PATH_INTERNAL);
            if (null != tempFolder) {
                folder.addAll(tempFolder);
            }
        }
        sortFolder(folder);
        return folder;
    }
    private void sortFolder(List<String> folder){
        if (folder != null && folder.size() > 1){
            Collections.sort(folder, new Comparator<String>(){

                @Override
                public int compare(String lhs, String rhs) {
                    String str1 = lhs.substring(lhs.lastIndexOf("/") + 1);
                    String str2 = rhs.substring(rhs.lastIndexOf("/") + 1);
                    return str2.compareTo(str1);
                }
            });
        }
    }

    private void loadSource(){
        mSdCardUnmounted = !StorageUtil.getExternalStorageState();
        mRestoredApp = new LoadAppTask(0, DeleteActivity.this).loadRestoredAppinfos();
        if(mIntent != null && mIntent.getExtras() != null){
            mRestoredFolder = (ArrayList<String>)mIntent.getExtras().getStringArrayList(DataUtil.DATA_FOLDER);
        }
        if(mRestoredFolder == null){
            mRestoredFolder = getInternalAndExternalFolder();
        }
        mHeader = new ArrayList<String>();
        mHeader.add(getResources().getString(R.string.application));
        mHeader.add(getResources().getString(R.string.data));

        if(mRestoredApp != null && mRestoredFolder != null){
            if (null != mAdapter) {
                mAdapter.upadteAppList(mRestoredApp);
                mAdapter.updateFolderList(mRestoredFolder);
                mAdapter.updateHeader(mHeader);
                return;
            }
            mAdapter = new DeleteListAdapter(this, mHeader, mRestoredApp, mRestoredFolder);
        }
    }
    @Override
    protected void onDestroy() {
        /* SPRD: Bug 412731 com.sprd.appbackup stopped unexpectedly. @{ */
        if(null != mProgressDialog && mProgressDialog.isShowing()){
            mProgressDialog.dismiss();
            mProgressDialog = null;
        }
        /* @} */
        StorageUtil.removeListener(this);
        mExecAppThread.shutdown();
        closeProgresDlg();
        super.onDestroy();
    }
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.confirm_delete_menu, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        MenuItem mi =  menu.findItem(R.id.confirm_menu_delete);
        if(mi != null){
            mi.setEnabled(mDeleteEnabled);
        }
        return super.onPrepareOptionsMenu(menu);
    }
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch(item.getItemId()){
        case R.id.cancle_menu_delete:
        case android.R.id.home:
            finish();
            break;
        case R.id.confirm_menu_delete:
            deleteCheckData();
            break;
        default: break;
        }
        return super.onOptionsItemSelected(item);
    }

    private void deleteCheckData() {
       new AlertDialog.Builder(this)
               .setTitle(R.string.delete_data_hint)
               .setMessage(R.string.data_cannot_recovery_after_delete)
               .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener() {

                   @Override
                   public void onClick(DialogInterface dialog, int which) {
                       final List<AppInfo> checkedApp = mAdapter.getApp();
                       final List<Bundle> checkedFolder = mAdapter.getData();

                       if (DBG) Log.d(TAG, "showDeleteDialog() confirm delete");
                       new AsyncTask<Void, Void, Integer>(){

                           @Override
                           protected Integer doInBackground(Void... params) {
                               if (DBG) Log.d(TAG, "doInBackground()");
                               try{
                                   for(AppInfo ai:checkedApp){
                                       String sourceDir = ai.getPackagePath();
                                       if((sourceDir == null) || (mSdCardUnmounted && ai.getPackagePath().contains(Config.EXTERNAL_APP_BACKUP_PATH))){
//                                           showSDcardWarning();
                                           return FAIL;
                                       }
                                       if(ai.isChecked()){
                                           File appFile = new File(ai.getPackagePath());
                                           boolean deleteRe = appFile.delete();
                                           if(DBG) {Log.d(TAG, "App = "+ ai + " deleted = "+deleteRe);}
                                       }
                                   }
                                   for(Bundle name : checkedFolder){
                                       String filePath = name.getString(DeleteListAdapter.KEY_NAME);
                                       if((filePath == null) || (mSdCardUnmounted && filePath.contains(Config.EXTERNAL_ARCHIVE_ROOT_PATH))){
//                                           showSDcardWarning();
                                           return FAIL;
                                       }
                                       if(name.getBoolean(DeleteListAdapter.KEY_CHECK_STATUS)){
                                           if(DBG) Log.d(TAG, "delete path = "+name.getString(DeleteListAdapter.KEY_NAME));
                                           File dir = new File(name.getString(DeleteListAdapter.KEY_NAME));
                                           Archive.deleteDir(dir);
                                       }
                                   }
                               }catch(Exception e){
                                   e.printStackTrace();
                                   return FAIL;
                               }
                               return SUCCESS;
                           }

                           @Override
                           protected void onPostExecute(Integer result) {
                               if(DBG) Log.d(TAG, "onPostExecute()");
                               closeProgresDlg();
                               if(result == SUCCESS){
                                   Toast.makeText(DeleteActivity.this, R.string.delete_complete, Toast.LENGTH_SHORT).show();
                                   /* SPRD: Bug 440295 appbackup can not delete. @{ */
                                   Intent intent = getIntent();
                                   setResult(RESULT_OK,intent);
                                   /* @} */
                                   DeleteActivity.this.finish();

                               }
                               if(result == FAIL){
                                   Toast.makeText(DeleteActivity.this, R.string.delete_fail, Toast.LENGTH_SHORT).show();
                               }
                           }

                           @Override
                           protected void onPreExecute() {
                               super.onPreExecute();
                               if(DBG) Log.d(TAG, "onPreExecute()");
                               if(mProgress == null){
                                   mProgress = new ProgressDialog(DeleteActivity.this);
                               }
                               mProgress.setProgressStyle(ProgressDialog.STYLE_SPINNER);
                               mProgress.setMessage(getString(R.string.deleting));
                               mProgress.setCanceledOnTouchOutside(false);
                               mProgress.show();
                           }
                           }.execute();
                   }
               }).setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
                   @Override
                   public void onClick(DialogInterface dialog, int which) {
                       dialog.dismiss();
                   }
               }).setCancelable(true).create().show();
    }
    @Override
    public void onStorageChanged(File path, boolean available) {
        if(DBG) {Log.d(TAG, "onStorageChanged() available = "+available+", path = "+path);}
        if(available){
            if(path.equals(StorageUtil.getExternalStorage()) && StorageUtil.getExternalStorageState()){
                mSdCardUnmounted = false;
            }
        }else{
            if(DBG) {Log.d(TAG, "onStorageChanged() path = "+path.getAbsolutePath());}
            if(path.equals(StorageUtil.getExternalStorage())){
                mSdCardUnmounted = true;
                showSDcardWarning();
            }
        }
    }
    private void showSDcardWarning(){
        new AlertDialog.Builder(DeleteActivity.this).setTitle(R.string.hint)
        .setMessage(R.string.sdcard_is_not_available)
        .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
                DeleteActivity.this.finish();
            }
        }).setCancelable(false).create().show();
    }
    private void closeProgresDlg(){
        if(mProgress != null){
            mProgress.dismiss();
            mProgress = null;
        }
    }
}
