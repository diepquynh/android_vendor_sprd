package com.sprd.appbackup;

import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Fragment;
import android.app.ProgressDialog;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.UserHandle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ExpandableListView;
import android.widget.Toast;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.SharedPreferences;
import android.os.Debug;

import com.sprd.appbackup.activities.MainActivity;
import com.sprd.appbackup.service.Account;
import com.sprd.appbackup.service.Config;
import com.sprd.appbackup.utils.StorageUtil;
import android.widget.CheckBox;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.Context;
/* SPRD: 445202 import @{ */
import android.content.SharedPreferences.Editor;

import java.io.File;
/* @} */

// SPRD: 476980 request storage permmison
import static android.Manifest.permission.READ_EXTERNAL_STORAGE;

public class BackupFragment extends Fragment{

    private final static String TAG = "BackupFragment";
    private final static boolean DBG = true;
    private MainActivity mContext;
    private Button mBtnStartBackup;
    private ExpandableListView mListView;
    private BackupAdapter mAdapter;
    private List<DataItem> mDataList;
    private List<AppInfo> mApkList;
    private List<String> mGroupName;
    private boolean mIsBacking = false;
    private static final int EXTERNAL_STORAGE = 0;
    private static final int INTERNAL_STORAGE = 1;
    private static final int USER_DEDINE_PATH = 2;
    private static final String REMINDUSER_NO_SD_BACKUP = "reminder_no_SD_backup";
    public final static String INTERNAL = "INTERNAL";
    public final static String EXTERNAL = "SD";
    public final static String SD = "SD";
    private String mSelectPath = null;
    /*  SPRD: Bug #458089 Account list haven't been updated after sim card removed @{ */
    public void dismissDialogInAdapter(){
       if(mAdapter != null){
           mAdapter.dimissAlertDialog();
       }
    }
    /* @} */
    public BackupFragment(){
       super();
       if(DBG) Log.d(TAG, "BackupFragment empty create()././.");
    }
    @Override
    public void onAttach(Activity activity) {
        if(DBG) {Log.d(TAG, "BackupFragment onAttach()././.activity = "+activity);}
        mContext = (MainActivity) activity;
        super.onAttach(activity);
    }

    @Override
    public void onDetach() {
        if(DBG) {Log.d(TAG, "onDetach()././.");}
        super.onDetach();
    }
    @Override
    public void onDestroy() {
        if(DBG) {Log.d(TAG, "onDestroy()././.");}
        if (null != mReceiver) {
            if(DBG) Log.d(TAG, "onDestroy()././. null != mReceiver");
            mContext.unregisterReceiver(mReceiver);
            mReceiver = null;
        }
        super.onDestroy();
    }
    @Override
    public void onStop() {
        if(DBG) {Log.d(TAG, "onStop()././.");}
        super.onStop();
    }
    @Override
    public void onCreate(Bundle savedInstanceState) {
        if(DBG) Log.d(TAG, "onCreate()././.");
        super.onCreate(savedInstanceState);
        mGroupName = new ArrayList<String>();
        mGroupName.add(mContext.getString(R.string.apk_backup));
        mGroupName.add(mContext.getString(R.string.data_backup));
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        if(DBG) Log.d(TAG, "onCreateView()././.");
        View view = inflater.inflate(R.layout.backup_fragment_layout, container, false);
        mBtnStartBackup = (Button) view.findViewById(R.id.btn_start_backup);
        mListView = (ExpandableListView) view.findViewById(R.id.expandablelist_backup);
        if(DBG) {Log.d(TAG, "onCreateView()-->mContext="+mContext);}
        mDataList = mContext.getDataList();
        mApkList = mContext.getAppList();
        if(DBG) Log.d(TAG, "onCreateView()-->"+"mApkList="+mApkList);
        if(DBG) Log.d(TAG, "onCreateView()-->"+"mDataList="+mDataList);
        if(mApkList != null && mDataList != null && mGroupName != null){
            mAdapter = new BackupAdapter(mContext, mGroupName, mApkList, mDataList);
        }
        mListView.setAdapter(mAdapter);
        mBtnStartBackup.setOnClickListener(new OnClickListener(){

            @Override
            public void onClick(View v) {
                /* SPRD: 476980 check permissions {@ */
                if (mContext != null && mContext.checkAndRequestPermissions()) {
                    startBackup();
                }
                /* @} */
            }
        });
        IntentFilter intentFilterReceiveFilePath = new IntentFilter("com.sprd.appbackup.SelectBackupPath");
        mContext.registerReceiver(mReceiver, intentFilterReceiveFilePath);
        return view;
    }

    public void updateAppList(final List<AppInfo> appList){
        if(DBG) Log.d(TAG, "BackupFragment.updateAppList()");
        if(mListView != null && mAdapter != null){
            if(DBG) Log.d(TAG, "updateAppList mAdapter !=  null");
            final List<AppInfo> orgData = mAdapter.getAppInfo();
            if(orgData != null && orgData.size() > 0){
                for(AppInfo newApp:appList){
                    for(AppInfo oldApp:orgData){
                        if(oldApp.getPackageName().equals(newApp.getPackageName())){
                            newApp.setChecked(oldApp.isChecked());
                            break;
                        }
                    }
                }
            }
            mAdapter.setAppsInfo(appList);
            mAdapter.notifyDataSetChanged();
            if(DBG) Log.d(TAG, "mAdapter.notifyDataSetChanged()");
        }else{
            if(mContext == null){
                if(DBG) Log.d(TAG, "updateAppList()  Activity == null");
                Activity activity = getActivity();
                if(DBG) Log.d(TAG, "updateAppList() getActivity ="+activity);
                if(activity != null && (activity instanceof MainActivity)){
                    mContext = (MainActivity) activity;
                }else{
                    return;
                }
            }
            mDataList = mContext.getDataList();
            if (null == mDataList) {
                return;
            }
            mAdapter = new BackupAdapter(mContext, mGroupName, appList, mDataList);
            if(mListView != null) mListView.setAdapter(mAdapter);
            if(DBG) Log.d(TAG, "updateAppList mAdapter ==  null");
        }
    }
    public void checkAllAppData(){
        if(DBG) Log.d(TAG, "BackupFragment checkAllAppData. mAppInfos");
        if (null != mContext && !mContext.isBackupInProgress()) {
            if(DBG) Log.d(TAG, "BackupFragment checkAllAppData. null != mContext");
            mAdapter.checkAllApp(true);
            mAdapter.checkAllData(true);
            mAdapter.notifyDataSetChanged();
        }

    }
    public void updateAgentList(List<DataItem> dataList){
        if(DBG) Log.d(TAG, "updataAgentList()");
        if(DBG) Log.d(TAG, "new dataList = "+dataList);
        if(dataList == null) return;
        if(mListView != null && mAdapter != null){
            if(DBG) Log.d(TAG, "updataAgentList. mAdapter !=  null");
            List<DataItem> oldDataList = mAdapter.getDataInfo();
            if(DBG) Log.d(TAG, "old dataList = "+oldDataList);
            if(DBG) Log.d(TAG, "mAdapter.setDataInfo(dataList)");
            if(oldDataList != null && oldDataList.size() > 0){
                for(DataItem newData:dataList){
                    for(DataItem oldData:oldDataList){
                        if(oldData.getCategoryName().equals(newData.getCategoryName())){
                            if(newData.isEnabled()){
                                newData.setChecked(oldData.isChecked());
                            }
                            break;
                        }
                    }
                }
            }
            mAdapter.setDataInfo(dataList);
            mAdapter.updateIdataCount();
            mAdapter.notifyDataSetChanged();
        }else{
            if(mContext == null){
                if(DBG) Log.d(TAG, "updataAgentList()  Activity == null");
                Activity activity = getActivity();
                if(DBG) {Log.d(TAG, "updataAgentList() getActivity ="+activity);}
                if(activity != null && (activity instanceof MainActivity)){
                    mContext = (MainActivity) activity;
                }else{
                    return;
                }
            }
            mApkList = mContext.getAppList();
            if (null == mApkList) {
                return;
            }
            mAdapter = new BackupAdapter(mContext, mGroupName, mApkList, dataList);
            if(mListView != null) mListView.setAdapter(mAdapter);
            if(DBG) Log.d(TAG, "updataAgentList. mAdapter ==  null");
        }
    }

    private void initStoragePath(final List<AppInfo> selectedApp, final List<DataItem> selectedData) {
        AlertDialog dialog = new AlertDialog.Builder(mContext)
        .setTitle(R.string.title_select_storage)
        .setSingleChoiceItems(R.array.storage_items, (Config.USE_STORAGE == Config.USE_EXTERNAL_STORAGE) ? 0 : 1,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog,int which) {
                        // TODO Auto-generated method stub
                        switch (which) {
                        case EXTERNAL_STORAGE:
                            Config.USE_STORAGE = Config.USE_EXTERNAL_STORAGE;
                            break;
                        case INTERNAL_STORAGE:
                            Config.USE_STORAGE = Config.USE_INTERNAL_STORAGE;
                            break;
                        case USER_DEDINE_PATH:
                            Config.USE_STORAGE = Config.USE_DEFINED_PATH;
                        }
                    }
                })
        .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                // TODO Auto-generated method stub
                /* SPRD:bug 571467,This app isn't allowed to access the external storage when the user isn't owner @{*/
                int userid = UserHandle.myUserId();
                if (DBG) Log.d(TAG,"userid = "+userid+"    Config.USE_STORAGE = "+Config.USE_STORAGE);
                if (Config.USE_STORAGE != Config.USE_INTERNAL_STORAGE && userid != 0) {
                    new AlertDialog.Builder(mContext)
                    .setMessage(R.string.requires_permissions_message)
                    .setCancelable(false)
                    .setNegativeButton(R.string.requires_permissions_close_button,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(final DialogInterface dialog, final int button) {
                                    //System.exit(0);
                                }
                            })
                    .show();
                    return;
                }
                /* @} */
                /* SPRD: Bug 453236 enter path select without SD card @{ */
                if (Config.USE_STORAGE != Config.USE_DEFINED_PATH && checkStorageAvailable()) {
                    /* SPRD: 461303 save selected path @{ */
                    if (Config.USE_STORAGE == Config.USE_EXTERNAL_STORAGE) {
                        setSelectBackupPath(Config.EXTERNAL_STORAGE_PATH, false);
                    } else {//Config.USE_INTERNAL_STORAGE
                        setSelectBackupPath(Config.INTERNAL_STORAGE_PATH, false);
                    }
                    /* @} */
                    mContext.backupAll(selectedApp, selectedData, Config.USE_STORAGE);
                } else if (Config.USE_STORAGE == Config.USE_DEFINED_PATH && checkStorageAvailable()) {
                /* @} */
                    Intent editPath = new Intent();
                    editPath.setClassName( "com.sprd.appbackup", "com.sprd.appbackup.SaveSelectPath");
                    Bundle mbundle = new Bundle();
                    mbundle.putString("storage", EXTERNAL);
                    mbundle.putString("selectbackupPath", "selectbackupPath"/*mDownloadPath.getText().toString()*/);
                    editPath.putExtras(mbundle);
                    startActivityForResult(editPath, 0);
                }
            }
        }).create();
        dialog.show();

    }

    /* SPRD:display detail hint . @{*/
    private void displayHintDetail() {
        AlertDialog dlg = new AlertDialog.Builder(mContext).setTitle(R.string.display_detail)
        .setMessage(R.string.not_insert_SD_backup_detail).setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        }).setCancelable(false).create();
        dlg.show();
    }
    /* @} */

    private boolean checkStorageAvailable() {
        Log.i(TAG, "DataUtil.IS_NAND==" + Config.IS_NAND);
        Log.i(TAG, "");
        if (Config.USE_STORAGE == Config.USE_EXTERNAL_STORAGE) {
            SharedPreferences noSDHint = mContext.getSharedPreferences(REMINDUSER_NO_SD_BACKUP, 0);

            if(DBG) Log.d(TAG, "checkStorageAvailable isexist = "+(!StorageUtil.getExternalStorageState()) + " m=" + noSDHint.getBoolean(REMINDUSER_NO_SD_BACKUP, true));
            if (!StorageUtil.getExternalStorageState() && noSDHint.getBoolean(REMINDUSER_NO_SD_BACKUP, true)){
                View linearLayout = LayoutInflater.from(mContext).inflate(R.layout.hint_dialog_layout, null);
                final CheckBox chkBox = (CheckBox) linearLayout.findViewById(R.id.nomore);

                AlertDialog dlgHint = new AlertDialog.Builder(mContext).setTitle(R.string.change_mobile_hint)
                        .setMessage(R.string.not_insert_SD_backup_hint)
                        .setView(linearLayout)
                        .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.dismiss();
                            }
                        }).setNegativeButton(R.string.read_detail, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                displayHintDetail();
                                }
                        })
                        .setCancelable(false).create();

                dlgHint.show();

                dlgHint.setOnDismissListener(new OnDismissListener(){
                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        mIsBacking = false;
                        if (chkBox.isChecked()) {
                            SharedPreferences noSetSDHint = mContext.getSharedPreferences(REMINDUSER_NO_SD_BACKUP, 0);
                            noSetSDHint.edit().putBoolean(REMINDUSER_NO_SD_BACKUP, false).apply();
                        }
                    }
                });

                mIsBacking = false;
                return false;
            } else if (!StorageUtil.getExternalStorageState()){
                AlertDialog dlg = new AlertDialog.Builder(mContext).setTitle(R.string.hint)
                        .setMessage(R.string.sdcard_is_not_available)
                        .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener() {

                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.dismiss();
                                mIsBacking = false;
                            }
                        }).setCancelable(false).create();
                dlg.show();
                dlg.setOnDismissListener(new OnDismissListener(){

                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        mIsBacking = false;
                    }
                });
                return false;
            }

            long freeSpace = StorageUtil.getAvailableExternalMemorySize()/1024/1024;
            if(DBG) Log.d(TAG, "backup > External freeSpace = "+freeSpace+"m");
            if(freeSpace < 10){
                if(DBG) Log.d(TAG, "backup > External freeSpace = "+freeSpace+"m");
                AlertDialog dlg = new AlertDialog.Builder(mContext).setTitle(R.string.warning)
                .setMessage(R.string.lock_of_freespace).setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener(){
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                        mIsBacking = false;
                    }
                }).setCancelable(false).create();
                dlg.show();
                dlg.setOnDismissListener(new OnDismissListener(){

                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        mIsBacking = false;
                    }
                });
                return false;
            }
            return true;
        } else if (Config.USE_STORAGE == Config.USE_INTERNAL_STORAGE) {
            if (Config.IS_NAND) {
                AlertDialog dlg = new AlertDialog.Builder(mContext).setTitle(R.string.hint)
                        .setMessage(R.string.use_external_storage)
                        .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener() {

                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.dismiss();
                                mIsBacking = false;
                            }
                        }).setCancelable(false).create();
                dlg.show();
                dlg.setOnDismissListener(new OnDismissListener(){

                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        mIsBacking = false;
                    }
                });
                return false;
            } else {
                if (!StorageUtil.getInternalStorageState()) {
                    AlertDialog dlg = new AlertDialog.Builder(mContext).setTitle(R.string.hint)
                            .setMessage(R.string.internal_storage_not_available)
                            .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener() {

                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                    mIsBacking = false;
                                }
                            }).setCancelable(false).create();
                    dlg.show();
                    dlg.setOnDismissListener(new OnDismissListener(){

                        @Override
                        public void onDismiss(DialogInterface dialog) {
                            mIsBacking = false;
                        }
                    });
                    return false;
                }
                long freeSpace = StorageUtil.getAvailableInternalMemorySize()/1024/1024;
                if(DBG) Log.d(TAG, "backup > Internal freeSpace = "+freeSpace+"m");
                if(freeSpace < 10){
                    if(DBG) Log.d(TAG, "backup > Internal freeSpace = "+freeSpace+"m");
                    AlertDialog dlg = new AlertDialog.Builder(mContext).setTitle(R.string.warning)
                    .setMessage(R.string.internal_lock_of_freespace).setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener(){
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            dialog.dismiss();
                            mIsBacking = false;
                        }
                    }).setCancelable(false).create();
                    dlg.show();
                    dlg.setOnDismissListener(new OnDismissListener(){

                        @Override
                        public void onDismiss(DialogInterface dialog) {
                            mIsBacking = false;
                        }
                    });
                    return false;
                }
                return true;
            }
        } else {
            //for user defined path
            SharedPreferences noSDHint = mContext.getSharedPreferences(REMINDUSER_NO_SD_BACKUP, 0);

            if (!StorageUtil.getExternalStorageState() && noSDHint.getBoolean(REMINDUSER_NO_SD_BACKUP, true)){
                View linearLayout = LayoutInflater.from(mContext).inflate(R.layout.hint_dialog_layout, null);
                final CheckBox chkBox = (CheckBox) linearLayout.findViewById(R.id.nomore);

                AlertDialog dlgHint = new AlertDialog.Builder(mContext).setTitle(R.string.change_mobile_hint)
                        .setMessage(R.string.not_insert_SD_backup_hint)
                        .setView(linearLayout)
                        .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.dismiss();
                            }
                        }).setNegativeButton(R.string.read_detail, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                displayHintDetail();
                                }
                        })
                        .setCancelable(false).create();

                dlgHint.show();

                dlgHint.setOnDismissListener(new OnDismissListener(){

                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        if (chkBox.isChecked()) {
                            SharedPreferences noSetSDHint = mContext.getSharedPreferences(REMINDUSER_NO_SD_BACKUP, 0);
                            noSetSDHint.edit().putBoolean(REMINDUSER_NO_SD_BACKUP, false).apply();
                        }
                    }
                });
                /* SPRD: Bug 453236 enter path select without SD card @{ */
                return false;
            } else if (!StorageUtil.getExternalStorageState()) {
                AlertDialog dlg = new AlertDialog.Builder(mContext).setTitle(R.string.hint)
                        .setMessage(R.string.sdcard_is_not_available)
                        .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener() {

                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.dismiss();
                                mIsBacking = false;
                            }
                        }).setCancelable(false).create();
                dlg.show();
                dlg.setOnDismissListener(new OnDismissListener() {

                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        mIsBacking = false;
                    }
                });
                return false;
            }
            /* @} */

            long freeInternalSpace = StorageUtil.getAvailableInternalMemorySize()/1024/1024;
            long freeExternalSpace = StorageUtil.getAvailableExternalMemorySize()/1024/1024;
            /* SPRD: modify for Bug 605516  @{ */
            final String path = getSelectBackupPath(true);
            if (path != null) {
                if (path.startsWith(Config.EXTERNAL_STORAGE_PATH)) {
                    if (freeExternalSpace < 10) {
                        if(DBG) Log.d(TAG, "backup > select path freeExternalSpace = " + freeExternalSpace);
                        AlertDialog dlg = new AlertDialog.Builder(mContext)
                                .setTitle(R.string.warning)
                                .setMessage(R.string.lock_of_freespace)
                                .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener(){
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.dismiss();
                                mIsBacking = false;
                            }
                        }).setCancelable(false).create();
                        dlg.show();
                        dlg.setOnDismissListener(new OnDismissListener(){

                            @Override
                            public void onDismiss(DialogInterface dialog) {
                                mIsBacking = false;
                            }
                        });
                        return false;
                    }
                } else if (path.startsWith(Config.INTERNAL_STORAGE_PATH)) {
                    if (freeInternalSpace < 10) {
                        if(DBG) Log.d(TAG, "backup > select path freeInternalSpace = "+freeInternalSpace +"m freeExternalSpace = " + freeExternalSpace);
                        AlertDialog dlg = new AlertDialog.Builder(mContext)
                                .setTitle(R.string.warning)
                                .setMessage(R.string.internal_lock_of_freespace)
                                .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener(){
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.dismiss();
                                mIsBacking = false;
                            }
                        }).setCancelable(false).create();
                        dlg.show();
                        dlg.setOnDismissListener(new OnDismissListener(){

                            @Override
                            public void onDismiss(DialogInterface dialog) {
                                mIsBacking = false;
                            }
                        });
                        return false;
                    }
                }
            }
            /*
            if (freeInternalSpace < 10 && freeExternalSpace < 10) {
                if(DBG) Log.d(TAG, "backup > select path freeSpace = "+freeInternalSpace +"m freeExternalSpace = " + freeExternalSpace);
                AlertDialog dlg = new AlertDialog.Builder(mContext).setTitle(R.string.warning)
                .setMessage(R.string.internal_lock_of_freespace).setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener(){
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                        mIsBacking = false;
                    }
                }).setCancelable(false).create();
                dlg.show();
                dlg.setOnDismissListener(new OnDismissListener(){

                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        mIsBacking = false;
                    }
                });
                return false;
            }
            */
            /* @} */
            return true;
        }
    }

    private void backup(int storage) {
        if(DBG) Log.d(TAG, "backup()..");
        if(mAdapter == null){
            if(DBG) Log.d(TAG, "mAdapter == null");
            mIsBacking = false;
            return;
        }

        if(DBG) Log.d(TAG,"checked data .");
        List<AppInfo> appList = mAdapter.getAppInfo();
        List<DataItem> dataList = new ArrayList<DataItem>(mAdapter.getDataInfo());
        List<AppInfo> selectedApp = new ArrayList<AppInfo>();
        List<DataItem> selectedData = new ArrayList<DataItem>();
        for(AppInfo ai:appList){
            if(ai.isChecked()){
                selectedApp.add(ai);
            }
        }
        for(DataItem di:dataList){
            if(di.isChecked() && di.isEnabled()){
                DataItem item = di.copy();
                selectedData.add(item);
            }
        }
        for(DataItem da:selectedData){
            List<Account> old = da.getAccounts();
            if(old != null){
                ArrayList<Account> checked = new ArrayList<Account>();
                for(Account a:old){
                    if(a.isChecked()){
                        checked.add(a);
                    }
                }
                da.setAccounts(checked);
            }
        }
        if(DBG) Log.d(TAG, "mAdapter.getDataInfo() "+mAdapter.getDataInfo());
        if(DBG) Log.d(TAG, "backup() selectedData = "+selectedData);
        if(DBG) Log.d(TAG,"selectedApp.size()="+selectedApp.size()+"--selectedData.size()="+selectedData.size());
        if(selectedApp.size() <= 0 && selectedData.size() <= 0){
            Toast.makeText(mContext, R.string.please_select_app_or_data, Toast.LENGTH_SHORT).show();
        } else if (storage == Config.USE_DEFINED_PATH) {
            mContext.backupAll(selectedApp, selectedData, Config.USE_DEFINED_PATH);
        } else {
            //mContext.backupAll(selectedApp, selectedData);
            Config.USE_STORAGE = Config.USE_EXTERNAL_STORAGE;
            initStoragePath(selectedApp, selectedData);
        }
        mIsBacking = false;
    }

    @Override
    public void onPause() {
        if(DBG) Log.d(TAG, "onPause()././.");
        super.onPause();
    }

    @Override
    public void onResume() {
        if(DBG) Log.d(TAG, "onResume()././.");
        super.onResume();
    }

    @Override
    public void onStart() {
        super.onStart();
        if(DBG) Log.d(TAG, "onStart()././.");
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        if(DBG) Log.d(TAG, "onActivityCreated()././.");
        super.onActivityCreated(savedInstanceState);
    }

    public void setBackupButtonEnable(boolean enable){
        if(mBtnStartBackup != null){
            mBtnStartBackup.setEnabled(enable);
            if(enable){
                mBtnStartBackup.setText(R.string.start_backup);
            } else {
                mBtnStartBackup.setText(R.string.please_holding);
            }
        }
    }

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if(DBG) Log.d(TAG, "BroadcastReceiver onReceive() action=" + action);
            if (action.equals("com.sprd.appbackup.SelectBackupPath")) {
                Bundle bundle= intent.getExtras();
                String absolutepathStr = bundle.getString("absolutepath");
                // SPRD: 461303 save defined path
                setSelectBackupPath(absolutepathStr, true);
                if(DBG) Log.d(TAG, "BroadcastReceiver onReceive() absolutepathStr=" + absolutepathStr);
                backup(Config.USE_DEFINED_PATH);
                //mContext.backupAll(selectedApp, selectedData, (Config.USE_STORAGE == Config.USE_EXTERNAL_STORAGE));
            }
        }
    };

    // SPRD: 461303 add flag isSelfDefined
    public void setSelectBackupPath(String path, boolean isSelfDefined) {
        /* SPRD: 445202 save defined backup path @{ */
        if (path != null && !path.isEmpty()) {
            if (!path.endsWith(File.separator)) {
                path = path + File.separator;
            }

            /* SPRD: 461303 modify for saving defined path @{ */
            if (isSelfDefined) {
                saveNewestDefinedPath(path);
            }
            /* @} */

            if (mContext.mRestoreSelectedFile != null) {
                String[] pathStrings = { path + "backup/App/", path + "backup/Data/"};

                for (String tmpString : pathStrings) {
                    if (!mContext.mRestoreSelectedFile.contains(tmpString)) {
                        mContext.mRestoreSelectedFile.add(tmpString);
                    }
                }

                pathStrings = null;
            }
        }
        /* @} */

        /* SPRD: 461303 set mSelectPath @{ */
        mSelectPath = path;
        if(DBG) Log.d(TAG, "setSelectBackupPath mSelectPath=" + mSelectPath);
        /* @} */
    }

    /* SPRD: 461303 add to save defined path @{ */
    private void saveNewestDefinedPath(String path) {
        SharedPreferences sharedPreferences =
            mContext.getSharedPreferences(Config.SHAREPREFERENCE_FILE_NAME, Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString(Config.DEFINED_BACKUP_PATH_KEY, path);
        editor.commit();
    }
    /* @} */

    /* SPRD: 461303 modify to get selected backup path @{ */
    public String getSelectBackupPath(boolean isAppbackup) {
        if(DBG) Log.d(TAG, "getSelectBackupPath mSelectPath=" + mSelectPath);

        String path = "";
        if (null == mSelectPath || mSelectPath.isEmpty()) {
            if (isAppbackup) {
                path = Config.EXTERNAL_APP_BACKUP_PATH;
            } else {
                path = Config.EXTERNAL_ARCHIVE_ROOT_PATH;
            }
        } else {
            if (isAppbackup) {
                path = mSelectPath + "backup/App/";
            } else {
                path = mSelectPath + "backup/Data/";
            }
        }

        return path;
    }
    /* @} */

    /* SPRD: 476980 start backup {@ */
    private void startBackup() {
        if (!mIsBacking){
            mIsBacking = true;
            backup(Config.USE_EXTERNAL_STORAGE);
        }else{
            Toast.makeText(mContext, R.string.backup_is_ongoing, Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
             String[] permissions, int[] grantResults) {
        if (mContext != null) {
            mContext.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }
    /* @} */
}
