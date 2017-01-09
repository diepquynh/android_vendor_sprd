package com.sprd.appbackup;

import static android.Manifest.permission.READ_EXTERNAL_STORAGE;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import android.Manifest.permission;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Fragment;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.Button;
import android.widget.ExpandableListView;
import android.widget.ExpandableListView.OnGroupExpandListener;
import android.widget.ExpandableListView.OnGroupCollapseListener;
import android.widget.Toast;

import com.sprd.appbackup.activities.DeleteActivity;
import com.sprd.appbackup.activities.MainActivity;
import com.sprd.appbackup.service.Archive;
import com.sprd.appbackup.service.Config;
import com.sprd.appbackup.utils.DataUtil;
import com.sprd.appbackup.utils.StorageUtil;

import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.Context;

import java.util.concurrent.RejectedExecutionException;

import android.content.DialogInterface.OnDismissListener;
import android.content.pm.PackageManager;

public class RestoreFragment extends Fragment{

    private final static String TAG = "RestoreFragment";
    private final static boolean DBG = true;
    private MainActivity mContext;
    private ExpandableListView mListView;
    private List<AppInfo> mAppList;
    private List<String> mGroupName;
    private Map<String, List<DataItem>> mMapRestoreData;
    private RestoreAdapter mExAdapter;
    private Button mBtnStartRestore;
    private boolean mIsRestoring;
    private int mExpandPosition = 2;
    private static final String REMINDUSER_NO_SD_RESTORE = "reminder_no_SD_restore";
    private boolean mIsFirstRestore = true;
    private String mSelectRestorePath = null;
    public final static String EXTERNAL = "SD";
    public final static String INTERNAL = "Internal";
    private AlertDialog mWaitdlg = null;
    private static final int EXTERNAL_STORAGE = 0;
    private static final int INTERNAL_STORAGE = 1;
    private int mSavePathItem = 0;


    /* SPRD: Bug 440295 appbackup can not delete. @{ */
    @Override
    public void onActivityResult(int requestCode, int resultCode,Intent data){
        switch(resultCode){
        case Activity.RESULT_OK:
            // SPRD: Bug 548079 delete appbackup scanAllFile dialog.
            scanFile();
            break;
         default:
            break;
        }
    }
   /* @} */
    @Override
    public void onAttach(Activity activity) {
        if(DBG) {Log.d(TAG, "Restore onAttach()././.activity = "+activity);}
        super.onAttach(activity);
        mContext = (MainActivity) activity;
    }

    public RestoreFragment(){
        super();
        if(DBG) Log.d(TAG, "RestoreFragment empty create()././.");
    }
    @Override
    public void onDetach() {
        if(DBG) {Log.d(TAG, "Restore onDetach()././.");}
        super.onDetach();
    }
    @Override
    public void onStop() {
        if(DBG) {Log.d(TAG, "Restore onStop()././.");}
        super.onStop();
    }
    @Override
    public void onCreate(Bundle savedInstanceState) {
        if(DBG) {Log.d(TAG, "Restore onCreate()././.");}
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
        mGroupName = new ArrayList<String>();
        mGroupName.add(mContext.getString(R.string.apk_restore));
        mGroupName.add(mContext.getString(R.string.data_restore));
    }

    public void updateRestoreDataList(Map<String, List<DataItem>> dataList){
        if(DBG) Log.d(TAG, "updateRestoreDataList()");
        if(DBG) Log.d(TAG, "dataList.keySet().size()="+dataList.keySet().size());
        if(mListView != null && mExAdapter != null){
            if(DBG) Log.d(TAG, "updateRestoreList() mExAdapter != null");
            String folder = mExAdapter.getTimeStamp();
            mExAdapter.setDataDetail(dataList);
            List<String> foldArray = mExAdapter.getDataFolderName();
            if(DBG) {Log.d(TAG, "foldArray = "+foldArray);}
            String currentFolder = null;
            if(DBG) {Log.d(TAG, "mExpandPosition = "+mExpandPosition);}
            if(foldArray != null && foldArray.size()>0){
                int position = mExpandPosition-2;
                if(position < foldArray.size() && position >= 0){
                    currentFolder = foldArray.get(position);
                }
            }
            if(DBG) {Log.d(TAG, "folder = "+folder+",currentFolder = "+currentFolder);}
            if(folder == null || currentFolder == null){
                mExAdapter.clearCheckedData();
            } else {
                List<DataItem> checkdata = mExAdapter.getCheckedData();
                List<DataItem> newData = dataList.get(folder);
                if (currentFolder.equals(folder) && newData != null
                        && checkdata != null) {
                    for (DataItem dataNew : newData) {
                        for (DataItem dat : checkdata) {
                            if (dataNew.getCategoryName().equals(
                                    dat.getCategoryName())) {
                                dataNew.setChecked(dat.isChecked());
                                break;
                            }
                        }
                    }
                    mExAdapter.setCheckedData(newData);
                    dataList.put(folder, newData);
                } else {
                    mExAdapter.clearCheckedData();
                }
            }
            mExAdapter.setDataDetail(dataList);
            mExAdapter.notifyDataSetChanged();
        }else{
           if(DBG) Log.d(TAG, "updateRestoreList() mExAdapter == null"+"\n"+"dataList="+dataList.toString());
           if(mContext == null){
               if(DBG) {Log.d(TAG,"Activity is destory");}
               Activity activity = getActivity();
               if(activity != null && (activity instanceof MainActivity)){
                   mContext = (MainActivity) activity;
               }else{
                   return;
               }
           }
           mAppList = mContext.getAppList();
           if(mAppList != null && mGroupName!= null && mContext != null){
               mExAdapter = new RestoreAdapter(mContext, mGroupName, mAppList, dataList);
               if(mListView != null){
                   mListView.setAdapter(mExAdapter);
               }
               mExAdapter.notifyDataSetChanged();
           }
        }
    }
    public void updateRestoreApkList(List<AppInfo> listApp){
        if(DBG) Log.d(TAG, "updateRestoreApkList()");
        if (mWaitdlg != null) {
            mContext.setScanAllStatus(false);
            mWaitdlg.dismiss();
            mWaitdlg = null;
        }
        if(mListView != null && mExAdapter != null){
            if(DBG) Log.d(TAG, "updateApkList() mExAdapter != null");
            mExAdapter.setAppInfos(listApp);
            mExAdapter.notifyDataSetChanged();
            if(DBG) Log.d(TAG, "updateApkList() ok");
        }else{
           if(DBG) Log.d(TAG, "updateApkList() mExAdapter == null");
           if(mContext == null){
               if(DBG) {Log.d(TAG,"Activity is destory");}
               Activity activity = getActivity();
               if(activity != null && (activity instanceof MainActivity)){
                   mContext = (MainActivity) activity;
               }else{
                   return;
               }
           }
           mMapRestoreData = mContext.getRestoreDataDetail();
           mExAdapter = new RestoreAdapter(mContext, mGroupName, listApp, mMapRestoreData);
           if(mListView != null){
               mListView.setAdapter(mExAdapter);
           }
           mExAdapter.notifyDataSetChanged();
        }
    }
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        if(DBG) Log.d(TAG, "onCreateView()././.");
        View view = inflater.inflate(R.layout.restore_fragment_layout, container, false);
        mBtnStartRestore = (Button) view.findViewById(R.id.btn_start_restore_f);
        mListView = (ExpandableListView) view.findViewById(R.id.expandablelist_restore);
        mAppList = mContext.getRestoreApp();
        mMapRestoreData = mContext.getRestoreDataDetail();
        if (DBG && mMapRestoreData != null) {
            Log.d(TAG, "onCreate()-->" + "mMapRestoreData size=" + mMapRestoreData.size() + "  mAppList = " + mAppList);
        }
        if(mMapRestoreData != null && mAppList != null){
            mExAdapter = new RestoreAdapter(mContext, mGroupName, mAppList, mMapRestoreData);
        }
        mListView.setAdapter(mExAdapter);
        mBtnStartRestore.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                /* SPRD: 476980 check storage permission {@ */
                if (mContext != null && mContext.checkAndRequestPermissions()) {
                    restore();
                }
                /* @} */
            }
        });

        mListView.setOnChildClickListener(new ExpandableListView.OnChildClickListener(){

            @Override
            public boolean onChildClick(ExpandableListView parent, View v,
                    int groupPosition, int childPosition, long id) {
                if(groupPosition == 0){
                    if(mExAdapter.isApkChecked(childPosition)){
                        mExAdapter.selectApk(childPosition, false);
                    }else{
                        mExAdapter.selectApk(childPosition, true);
                    }
                    if(DBG) Log.d(TAG, "onChildClick mExAdapter.AppInfo = "+mExAdapter.getAppInfos().toString());
                }
                if(groupPosition > 1){
                    List<DataItem> mCheckedData = mExAdapter.getCurrentCheckedData(groupPosition);
                    if(mCheckedData.get(childPosition).isChecked()){
                        mExAdapter.setCheckedData(groupPosition, childPosition, false);
                    }else{
                        mExAdapter.setCheckedData(groupPosition, childPosition, true);
                    }
                    if(DBG) Log.d(TAG, "childPosition = "+childPosition+"--onChildClick mExAdapter.mCheckedData = "+mExAdapter.getCurrentCheckedData(groupPosition).toString());
                }
                return true;
            }
        });
        mListView.setOnItemLongClickListener(new OnItemLongClickListener(){

            @Override
            public boolean onItemLongClick(AdapterView<?> arg0, View arg1,
                    int arg2, long arg3) {
                int groupPos = (Integer)arg1.getTag(R.id.txt_folder_name_item);
                if(groupPos > 1){
                    String folder = (String)arg1.getTag(R.id.txt_folder_name);
                    showDeleteDialog(folder);
                }
                if(groupPos == 0){
                    int subPosition = (Integer) arg1.getTag(R.id.chkbx_delete_slected);
                    if(subPosition != -1){
                        showDeleteApkDialog(subPosition);
                    }
                }
                return true;
            }
        });

        mListView.setOnGroupExpandListener(new OnGroupExpandListener(){

            @Override
            public void onGroupExpand(int groupPosition) {
                mExAdapter.getCurrentCheckedData(groupPosition);
                if(groupPosition > 1){
                    mExpandPosition = groupPosition;
                    for(int i=2; i<mExAdapter.getGroupCount(); i++){
                        if(i != groupPosition){
                            mListView.collapseGroup(i);
                        } else {
                            mExAdapter.setCheckedData(groupPosition, true);
                        }
                    }
                }
            }

        });
     /* SPRD: fix bug 411629 @{*/
       /* mListView.setOnGroupCollapseListener(new OnGroupCollapseListener(){
            @Override
            public void onGroupCollapse(int groupPosition) {
                mExAdapter.getCurrentCheckedData(groupPosition);
                if(groupPosition > 1){
                    mExpandPosition = groupPosition;
                    for(int i=2; i<mExAdapter.getGroupCount(); i++){
                        if(i == groupPosition){
                            mExAdapter.setCheckedData(groupPosition, false);
                        }
                    }
                }
            }

        });*/
        /* @}*/

        IntentFilter intentFilterReceiveFilePath = new IntentFilter("com.sprd.appbackup.SelectRestorePath");
        mContext.registerReceiver(mReceiver, intentFilterReceiveFilePath);
        return view;
    }
    private void restore(){
        if(DBG) {Log.d(TAG, "restore()..");}
        if(null == mExAdapter){
        	Log.e(TAG, "mExAdapter is null and return");
        	return;
        }
        final List<AppInfo> checkedApp = mExAdapter.getAppInfos();
        final List<DataItem> listData = mExAdapter.getCheckedData();
        final String timeStamp = mExAdapter.getTimeStamp();
        if(checkedApp == null){
            if(DBG) Log.d(TAG,"checkedApp == null");
            return;
        }
        List<AppInfo> selectedApp = new ArrayList<AppInfo>();
        int appCount = 0;
        List<DataItem> selectedData = new ArrayList<DataItem>();
        int dataCount = 0;

        for(AppInfo ai: checkedApp){
            if(ai.isChecked()) {
                appCount++;
                selectedApp.add(ai);
            }
        }

        if(listData == null || timeStamp == null){
            if(DBG) Log.d(TAG, "restore() listData == null || checkedApp == null || timeStamp == null");
        }else{
            for (DataItem di : listData) {
                if (di.isChecked()) {
                    dataCount++;
                    selectedData.add(di);
                }
            }
        }
        if(DBG) Log.d(TAG, "selectedApp.size = " + appCount+"--selectedData= "+dataCount);
        if(appCount == 0 && dataCount == 0){
            Toast.makeText(mContext, R.string.please_select_app_or_data, Toast.LENGTH_SHORT).show();
        } else {
            if(!mIsRestoring){
                mIsRestoring = true;
                mContext.clearDuplicationResault();
                if (appCount > 0) {
                    mContext.restoreApp(selectedApp, selectedData, timeStamp);
                } else {
                    if(dataCount > 0){
                        mContext.restoreApp(null, selectedData, timeStamp);
                    }
                }
                mIsRestoring = false;
            }
        }
    }

    private void showDeleteApkDialog(final int index){
        new AlertDialog.Builder(mContext).setTitle(R.string.hint)
        .setTitle(R.string.hint)
        .setMessage(R.string.confirm_delete_apk)
        .setPositiveButton(R.string.delete, new OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int which) {
                if(DBG) Log.d(TAG, "showDeleteDialog() confirm delete");
                AppInfo app = mExAdapter.getAppInfos().get(index);
                File appFile = new File(app.getPackagePath());
                appFile.delete();
                if(DBG) Log.d(TAG, "onItemLongClick() appfile delete ok");
                /* SPRD: modify for bug 603032 @{ */
                RestoreFragment.this.scanFile();
                /* @} */
            }
        }).setNegativeButton(R.string.cancel, new OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        }).setCancelable(true).create().show();
    }

    private void showDeleteDialog(final String name){
        new AlertDialog.Builder(getActivity()).setTitle(R.string.hint)
        .setTitle(R.string.hint)
        .setMessage(R.string.confirm_delete)
        .setPositiveButton(R.string.delete, new OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int which) {
                if(DBG) Log.d(TAG, "showDeleteDialog() confirm delete");
                for(int i=2; i < mExAdapter.getGroupCount(); i++){
                    mListView.collapseGroup(i);
                }
                new Thread(){

                    @Override
                    public void run() {
                        File dir = new File(name);
                        Archive.deleteDir(dir);

                        /* SPRD: add for bug 421443 ,notify the main activity to update the data list @{ */
                        if (mContext != null) {
                            mContext.updateRestoreFragment();
                        }
                        /*@}*/

                    }
                }.start();
            }
        }).setNegativeButton(R.string.cancel, new OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        }).setCancelable(true).create().show();
    }

    @Override
    public void onDestroy() {
        if(DBG) Log.d(TAG, "restore onDestroy()././.");
        if (null != mReceiver) {
            if(DBG) Log.d(TAG, "onDestroy()././. null != mReceiver");
            mContext.unregisterReceiver(mReceiver);
            mReceiver = null;
        }
        super.onDestroy();
    }

    @Override
    public void onPause() {
        if(DBG) Log.d(TAG, "Restore onPause()././.");
        super.onPause();
    }

    @Override
    public void onResume() {
        if(DBG) Log.d(TAG, "Restore onResume()././.");
        if (mExAdapter != null && mExAdapter.getRestoreCount() == 0 && mIsFirstRestore) {
            if(DBG) Log.d(TAG, "Restore onResume() alert././.");
            if (!StorageUtil.getExternalStorageState()) {
                //not support SD card,interface need confirm later
                mContext.displayHintChangePhone(mContext, R.string.no_SD_backup_hint, REMINDUSER_NO_SD_RESTORE, R.string.no_SD_backup_detail);
            } else if (!StorageUtil.getExternalStorageState()) {
                //not insert SD card
                mContext.displayHintChangePhone(mContext, R.string.not_insert_SD_backup_hint, REMINDUSER_NO_SD_RESTORE, R.string.no_SD_backup_detail);
            }
            mIsFirstRestore = false;
        }
        super.onResume();
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        if(DBG) {Log.d(TAG, "onCreateOptionsMenu()././.");}
        inflater.inflate(R.menu.delete_menu, menu);
    }
    public void onPrepareOptionsMenu(Menu menu) {
        if(DBG) Log.d(TAG, "onPrepareOptionsMenu()././.");
        if(mExAdapter != null && menu != null){
            List<AppInfo> appList = mExAdapter.getAppInfos();
            List<String> folderList = mExAdapter.getDataFolderName();
            boolean withoutData = true;
            if(appList != null && folderList != null){
                withoutData = (appList.size() < 1 && folderList.size() < 1);
            }
            MenuItem deleteMenu = menu.findItem(R.id.restore_menu_delete);
            MenuItem deleteAllMenu = menu.findItem(R.id.restore_menu_delete_all);

            //MenuItem scanAllStorage = menu.findItem(R.id.restore_menu_scan_all);
            //MenuItem selectPath = menu.findItem(R.id.restore_menu_select_path);

            if(deleteMenu != null && deleteAllMenu != null){
                if(withoutData){
                    deleteMenu.setEnabled(false);
                    deleteAllMenu.setEnabled(false);
                }else{
                    deleteMenu.setEnabled(true);
                    deleteAllMenu.setEnabled(true);
                }
            }

        }
    }
    /* SPRD: Bug 440295 appbackup can not delete. @{ */
    public void scanAllFile(){
        mContext.mRestoreSelectedFile.clear();
        mWaitdlg = new AlertDialog.Builder(mContext).setTitle(R.string.scan_all_storage)
        .setMessage(R.string.scan_all_storage_please_holding)
        .setCancelable(false)
        .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                mContext.setScanAllStatus(false);
                if (mWaitdlg != null) {
                    mWaitdlg.dismiss();
                    mWaitdlg = null;
                }
            }
        })
        .setOnDismissListener(new OnDismissListener(){
            @Override
            public void onDismiss(DialogInterface dialog) {
                mContext.setScanAllStatus(false);
            }
        })
        .create();
        mWaitdlg.show();

        try{
            if(mContext.mExecScanRestoreThread != null && !mContext.mExecScanRestoreThread.isShutdown()){
                mContext.mExecScanRestoreThread.execute(new scanRestoreFile());
            }
        }catch(RejectedExecutionException e){
            e.printStackTrace();
        }
    }
    /* @} */

    /* SPRD: Bug 548079 delete appbackup scanAllFile dialog . @{ */
    public void scanFile() {
        mContext.mRestoreSelectedFile.clear();

        try {
            if (mContext.mExecScanRestoreThread != null && !mContext.mExecScanRestoreThread.isShutdown()) {
                mContext.mExecScanRestoreThread.execute(new scanRestoreFile());
            }
        } catch (RejectedExecutionException e) {
            e.printStackTrace();
        }
    }
    /* @} */

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if(DBG) Log.d(TAG, "onOptionsItemSelected()././. item.getItemId() = " + item.getItemId());
        /* SPRD: 476980 check storage permission {@ */
        if (!checkAndRequestStoragePermission()) {
            return super.onOptionsItemSelected(item);
        }
        /* @} */

        switch(item.getItemId()){
        case R.id.restore_menu_delete:
            int requestCode = 0;
            Intent intent = new Intent(mContext, DeleteActivity.class);
            if(mExAdapter!= null){
                Bundle extras = new Bundle();
                extras.putStringArrayList(DataUtil.DATA_FOLDER, (ArrayList<String>)mExAdapter.getDataFolderName());
                intent.putExtras(extras);
            }
            /* SPRD: Bug 440295 appbackup can not delete. @{ */
            startActivityForResult(intent,requestCode);
            /* @} */
            break;
        case R.id.restore_menu_delete_all:
            deleteAllFile();
            break;

        case R.id.restore_menu_scan_all:
            /* SPRD: Bug 440295 appbackup can not delete. @{ */
            scanAllFile();
            /* @} */
            break;
        case R.id.restore_menu_select_path:
            /* SPRD: 466608 check sdcard state @{ */
            if (!StorageUtil.getExternalStorageState()){
                Intent editPath = new Intent(mContext, SaveSelectPath.class);
                Bundle bundle = new Bundle();
                bundle.putString("storage", INTERNAL);
                mSavePathItem = INTERNAL_STORAGE;
                bundle.putString("selectbackupPath", "selectrestorePath");
                editPath.putExtras(bundle);
                startActivityForResult(editPath, 0);
                break;
            }
            /* @} */

            AlertDialog dialog = new AlertDialog.Builder(mContext)
            .setTitle(R.string.title_select_storage)
            .setSingleChoiceItems(R.array.select_path_items, 0,
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog,int which) {
                            // TODO Auto-generated method stub
                            mSavePathItem = which;
                            if(DBG) Log.d(TAG, "onOptionsItemSelected()././. mSavePathItem = " + mSavePathItem);
                        }
                    })
            .setPositiveButton(R.string.confirm, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    // TODO Auto-generated method stub
                    Intent editPath = new Intent();
                    editPath.setClassName( "com.sprd.appbackup", "com.sprd.appbackup.SaveSelectPath");
                    Bundle mbundle = new Bundle();
                    if(DBG) Log.d(TAG, "restore_menu_select_path././. mSavePathItem = " + mSavePathItem);
                    if (mSavePathItem == INTERNAL_STORAGE) {
                        mbundle.putString("storage", INTERNAL);
                    } else {
                        mbundle.putString("storage", EXTERNAL);
                    }
                    mSavePathItem = EXTERNAL_STORAGE;

                    mbundle.putString("selectbackupPath", "selectrestorePath"/*mDownloadPath.getText().toString()*/);
                    editPath.putExtras(mbundle);
                    startActivityForResult(editPath, 0);
                }
            }).create();
            dialog.show();

            break;
        default :
            if(DBG) Log.d(TAG, "onOptionsItemSelected()././. default = " + item.getItemId());
            break;
        }
        return super.onOptionsItemSelected(item);
    }

    class scanRestoreFile implements Runnable{
        @Override
        public void run() {
            mContext.getScanRestoreFile(Config.INTERNAL_STORAGE_PATH);
            mContext.getScanRestoreFile(Config.EXTERNAL_STORAGE_PATH);
            mContext.scanBackupFileList();
        }
    }

    private void deleteAllFile() {
        new AlertDialog.Builder(mContext)
                .setTitle(R.string.delete_data_hint)
                .setMessage(R.string.data_cannot_recovery_after_delete)
                .setPositiveButton(R.string.confirm, new OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if (DBG) Log.d(TAG, "showDeleteDialog() confirm delete");
                        final List<AppInfo> appList = new ArrayList<AppInfo>(mExAdapter.getAppInfos());
                        final List<String> dataFolder = new ArrayList<String>(mExAdapter.getDataFolderName());
                        new AsyncTask<Void, Void, Void>(){

                            ProgressDialog progress = new ProgressDialog(mContext);

                            @Override
                            protected Void doInBackground(Void... params) {
                                try{
                                    for(AppInfo ai:appList){
                                        File appFile = new File(ai.getPackagePath());
                                        appFile.delete();
                                    }
                                    if(DBG) {Log.d(TAG, "data folder size = "+dataFolder.size());}
                                    for(String name : dataFolder){
                                        File dir = new File(name);
                                        Archive.deleteDir(dir);
                                    }
                                }catch(Exception e){
                                    e.printStackTrace();
                                    if(DBG) {Log.d(TAG, "deleteAllFile() occur exception ");}
                                }
                                return null;
                            }

                            @Override
                            protected void onPostExecute(Void result) {
                                if(progress != null && progress.isShowing()){
                                    progress.dismiss();
                                    progress = null;
                                }
                                mExAdapter.getAppInfos().clear();
                                mExAdapter.getDataFolderName().clear();
                                mExAdapter.notifyDataSetChanged();
                            }

                            @Override
                            protected void onPreExecute() {
                                super.onPreExecute();
                                progress.setProgressStyle(ProgressDialog.STYLE_SPINNER);
                                progress.setMessage(getString(R.string.deleting));
                                progress.setCanceledOnTouchOutside(false);
                                progress.show();
                            }
                            }.execute();
                    }
                }).setNegativeButton(R.string.cancel, new OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                }).setCancelable(true).create().show();
    }
    @Override
    public void onStart() {
        super.onStart();
        if(DBG) Log.d(TAG, "onStart()././.");
    }

    public void setInvisibleIndicator(int id){
        if(mListView != null){
            // Add for Bug411616.
            /* SPRD: modify for bug 639809 @{ */
            mListView.setInvisibleIndicatorGroupId(id);
            /* @} */
        } else {
            Log.e(TAG,"setInvisibleIndicator,mListView is NULL");
        }

    }

    public void setRestoreButtonEnable(boolean enable){
        if(mBtnStartRestore != null){
            mBtnStartRestore.setEnabled(enable);
            if(enable){
                mBtnStartRestore.setText(R.string.start_restore);
            } else {
                mBtnStartRestore.setText(R.string.please_holding);
            }
        }
    }

    /*for bug 388383,cmcc new req,need default select all*/
    public void checkRestoreAllAppData(){
        if(DBG) Log.d(TAG, "checkRestoreAllAppData././. enter");
        if (null != mContext && !mContext.isBackupInProgress()) {
            if(DBG) Log.d(TAG, "checkRestoreAllAppData././.");
            mExAdapter.checkAllApp(true);
            //mExAdapter.checkAllData(true);
            mExAdapter.notifyDataSetChanged();
        }
    }
    /*end*/
    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals("com.sprd.appbackup.SelectRestorePath")) {
                Bundle bundle= intent.getExtras();
                String absolutepathStr = bundle.getString("absolutepath");
                setSelectRestorePath(absolutepathStr);
                if(DBG) Log.d(TAG, "BroadcastReceiver onReceive() restore absolutepathStr=" + absolutepathStr);
                //mContext.mRestoreSelectedFile.clear();
                if(DBG) Log.d(TAG, "time_cal mContext.getScanRestoreFile start");
                mContext.getScanRestoreFile(absolutepathStr);
                if(DBG) Log.d(TAG, "time_cal mContext.getScanRestoreFile end");
                if(DBG) Log.d(TAG, "time_cal mContext.scanBackupFileList start");
                mContext.scanUserSelectBackupFileList();
                if(DBG) Log.d(TAG, "time_cal mContext.scanBackupFileList end");
                //mExecFileThread.execute(new ScanBackupFileThread())
            }
        }
    };

    public void setSelectRestorePath(String path) {
        mSelectRestorePath = path;
        if(DBG) Log.d(TAG, "setSelectRestorePath mSelectPath=" + mSelectRestorePath);
    }

    public String getSelectRestorePath(boolean isAppRestore) {
        if(DBG) Log.d(TAG, "setSelectRestorePath mSelectPath=" + mSelectRestorePath);
        if (null == mSelectRestorePath && isAppRestore) {
            mSelectRestorePath = Config.EXTERNAL_APP_BACKUP_PATH;
        } else if (null == mSelectRestorePath && !isAppRestore) {
            mSelectRestorePath = Config.EXTERNAL_ARCHIVE_ROOT_PATH;
        }
        if (isAppRestore) {
            return mSelectRestorePath + "backup/App/";
        } else {
            return mSelectRestorePath + "backup/Data/";
        }
    }

    /* SPRD: 476980 start backup {@ */
    private boolean checkAndRequestStoragePermission() {
        if (mContext == null) {
            return false;
        }

        if (mContext.checkSelfPermission(permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            mContext.requestPermissions(new String[] {permission.WRITE_EXTERNAL_STORAGE},
                    MainActivity.PERMISSION_REQUEST_CODE);
            return false;
        }
        return true;
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
