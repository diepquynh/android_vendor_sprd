package com.sprd.fileexplorer.activities;

import java.io.File;
import java.util.ArrayList;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.adapters.MultiSelectFileAdapter;
import com.sprd.fileexplorer.adapters.QuickScanCursorAdapter;
import com.sprd.fileexplorer.adapters.QuickScanCursorAdapter.CursorChangeListener;
import com.sprd.fileexplorer.drmplugin.MultiSelectActivityUtils;
import com.sprd.fileexplorer.drmplugin.OverViewActivityUtils;
import com.sprd.fileexplorer.util.ActivityUtils;
import com.sprd.fileexplorer.util.ActivityUtils.CopyFileListener;
import com.sprd.fileexplorer.util.FileDeleteTask;
import com.sprd.fileexplorer.util.FileType;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexplorer.util.FileUtilTask.OnFinishCallBack;
import com.sprd.fileexplorer.util.IStorageUtil.StorageChangedListener;
import com.sprd.fileexplorer.util.IntentUtil;
import com.sprd.fileexplorer.util.StorageUtil;

import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;

public class MultiSelectActivity extends Activity implements OnClickListener,
        OnItemClickListener,CursorChangeListener {
    
    private static final String TAG = "MultiSelectActivity";

    // SPRD: Add for bug510243.
    private static final int MULTI_SELECT_STORAGE_PERMISSION_REQUEST_CODE = 5;
    public static final int MULTI_DELETE = 20;
    private static final int FILE_LIMIT = 40;
    private static final int MULTI_MAX_SELECT = 500;
    
    private ListView mListView;
    private MultiSelectFileAdapter mAdapter;
    private QuickScanCursorAdapter mAdapterForOverView;
    private CheckBox mSelectAllFileCb;
    private ArrayList<File> mSelectedFiles = new ArrayList<File>();
    private TextView selectAllFileTx;
    private FileDeleteTask mFileDeleteTask = null;

    private CopyFileListener mCopyFileListener;
    private AlertDialog mConfirmDeleteDialog = null;
    //SPRD 426100
    private boolean isFromItemClick =false;

    StorageChangedListener mStorageChangedListener = new StorageChangedListener() {

        // SPRD: Modify for bug509242.
        @Override
        public void onStorageChanged(String path, boolean available, boolean sdcard) {
            Log.d(TAG, "StorageChanged: path = " + path + " available = " + available + "; sdcard = " + sdcard);
            if(available) {
                return;
            }
            new Handler(getMainLooper()).postDelayed(new Runnable() {

                @Override
                public void run() {
                    finish();
                }
            }, 1000);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.multi_select_activity);
        StorageUtil.addStorageChangeListener(mStorageChangedListener);
        mListView = (ListView) findViewById(R.id.list);
        mSelectAllFileCb = (CheckBox) findViewById(R.id.select_all_cb);
        selectAllFileTx = (TextView) findViewById(R.id.selece_all_file);
        /* SPRD 426100 @{ */
//        mSelectAllFileCb.setOnClickListener(this);
        mSelectAllFileCb.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView,boolean isChecked) {
                // TODO Auto-generated method stub
                Log.i(TAG, "onCheckedChanged isFromItemClick:"+isFromItemClick);
                if (isFromItemClick) {
                    isFromItemClick = false;
                    return;
                }

                if (mAdapter != null) {
                    /* SPRD: bug 445916, uncheck all the view @{ */
                    mAdapter.checkAll(false);
                    /* @} */
                    if (isChecked) {
                        selectAllFileTx.setText(R.string.unselect_all);
                        mSelectedFiles.clear();
                        /* SPRD:429954 Copy more than 5000 pics, Fileexplorer occours ANR @{*/
                        if (mAdapter.getCount() <= MULTI_MAX_SELECT) {
                            for (int i = 0; i < mAdapter.getCount(); i++) {
                                mSelectedFiles.add((File) mAdapter.getItem(i));
                            }
                            mAdapter.checkAll(true);
                        }else{
                            /* SPRD 435241 @{ */
                            for (int i = 0; i < MULTI_MAX_SELECT; i++) {
                                mSelectedFiles.add((File) mAdapter.getItem(i));
                                mAdapter.setChecked(i, true);
                            }
                            /* @} */
                            Toast.makeText(MultiSelectActivity.this, R.string.failed_select_event, Toast.LENGTH_SHORT).show();
                            // SPRD 435241
                            //mSelectAllFileCb.setChecked(false);
                        }
                        /* @}*/
                    } else {
                        selectAllFileTx.setText(R.string.select_all);
                        mSelectedFiles.clear();
                        mAdapter.checkAll(false);
                    }
                    mAdapter.notifyDataSetChanged();
                } else if (mAdapterForOverView != null && mAdapterForOverView instanceof QuickScanCursorAdapter) {
                    /* SPRD: bug 445916, uncheck all the view @{ */
                    mAdapterForOverView.checkAll(false);
                    /* @} */
                    if (isChecked) {
                        selectAllFileTx.setText(R.string.unselect_all);
                        mSelectedFiles.clear();
                        /* SPRD:429954 Copy more than 5000 pics, Fileexplorer occours ANR @{*/
                        if (mAdapterForOverView.getCount() <= MULTI_MAX_SELECT) {
                            for (int i = 0; i < mAdapterForOverView.getCount(); i++) {
                                mSelectedFiles.add((File) mAdapterForOverView.getItem(i));
                            }
                            mAdapterForOverView.checkAll(true);
                        }else {
                            /* SPRD 435241 @{ */
                            for (int i = 0; i < MULTI_MAX_SELECT; i++) {
                                mSelectedFiles.add((File) mAdapterForOverView.getItem(i));
                                mAdapterForOverView.setChecked(i, true);
                            }
                            /* @}*/
                            Toast.makeText(MultiSelectActivity.this, R.string.failed_select_event, Toast.LENGTH_SHORT).show();
                            // SPRD 435241
                            //mSelectAllFileCb.setChecked(false);
                        }
                        /* @}*/
                    } else {
                        selectAllFileTx.setText(R.string.select_all);
                        mSelectedFiles.clear();
                        mAdapterForOverView.checkAll(false);
                    }
                    mAdapterForOverView.notifyDataSetChanged();
                }
                invalidateOptionsMenu();
            }
        });
        /* @} */
        ActionBar actionbar = getActionBar();
        actionbar.setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP | ActionBar.DISPLAY_SHOW_TITLE);
        if (getIntent().getStringExtra("path") != null) {
            String path = getIntent().getStringExtra("path");
            int top = getIntent().getIntExtra("position", 0);
            if (path != null) {
                File file = new File(path);
                mAdapter = new MultiSelectFileAdapter(this,mListView);
                mAdapter.init(file);
                mListView.setAdapter(mAdapter);
                mListView.setSelection(top);
                mListView.setOnItemClickListener(this);
            }
        } else if (getIntent().hasExtra("fileType")) {
            int fileType = getIntent().getIntExtra("fileType", -1);
            int top = getIntent().getIntExtra("position", 0);
            switch (fileType) {
            case FileType.FILE_TYPE_IMAGE:
                mAdapterForOverView = new QuickScanCursorAdapter(this,
                        QuickScanCursorAdapter.RESOURCE_IMAGE, top,
                        mListView, true);
                break;
            case FileType.FILE_TYPE_AUDIO:
                mAdapterForOverView = new QuickScanCursorAdapter(this,
                        QuickScanCursorAdapter.RESOURCE_AUDIO, top,
                        mListView, true);
                break;
            case FileType.FILE_TYPE_VIDEO:
                mAdapterForOverView = new QuickScanCursorAdapter(this,
                        QuickScanCursorAdapter.RESOURCE_VIDEO, top,
                        mListView, true);
                break;
            case FileType.FILE_TYPE_DOC:
                mAdapterForOverView = new QuickScanCursorAdapter(this,
                        QuickScanCursorAdapter.RESOURCE_DOC, top,
                        mListView, true);
                break;
            case FileType.FILE_TYPE_PACKAGE:
                mAdapterForOverView = new QuickScanCursorAdapter(this,
                        QuickScanCursorAdapter.RESOURCE_APK, top,
                        mListView, true);
            }
            mListView.setAdapter(mAdapterForOverView);
            mAdapterForOverView.setCursorChangeListener(this);
            mListView.setOnItemClickListener(this);
            this.invalidateOptionsMenu();
        }
        mCopyFileListener = new CopyFileListener(this);
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position,
            long id) {
        if (mAdapter != null) {
            if (mAdapter.isChecked(position)) {
                /* SPRD: bug 445916, change the judgment for the click event @{ */
                //if (mAdapter.isAllChecked()) {
                if (mSelectAllFileCb.isChecked()) {
                /* @} */
                    // SPRD 426100
                    isFromItemClick = true;
                    mSelectAllFileCb.setChecked(false);
                    selectAllFileTx.setText(R.string.select_all);
                }
                mAdapter.setChecked(position, false);
                mSelectedFiles.remove(mAdapter.getItem(position));
            } else {
                /* SPRD: bug 445916, show the Toast when the select files count more than 500 @{ */
                if (mAdapter.getCount() >= MULTI_MAX_SELECT
                        && mSelectedFiles.size() >= MULTI_MAX_SELECT) {
                    Toast.makeText(MultiSelectActivity.this, R.string.failed_select_event, Toast.LENGTH_SHORT).show();
                    return;
                }
                /* @} */
                mSelectedFiles.add((File) mAdapter.getItem(position));
                mAdapter.setChecked(position, true);
                if (mSelectedFiles.size() == mAdapter.getCount()) {
                    //SPRD 426100
                    isFromItemClick = true;
                    mSelectAllFileCb.setChecked(true);
                    selectAllFileTx.setText(R.string.unselect_all);
                }
            }
            /* SPRD 398404 After invalidateOptionsMenu() ,delete button will be able to click.
             * So the code must make sure adapter calls notifyDataSetChanged() when its content changes
             * before auto test can click delete button @{ */
            mAdapter.notifyDataSetChanged();
            /* @} */
            this.invalidateOptionsMenu();
            /* SPRD 398404 @{ */
            //mAdapter.notifyDataSetChanged();
            /* @} */
        } else if (mAdapterForOverView != null) {
            if (mAdapterForOverView instanceof QuickScanCursorAdapter) {
                if (mAdapterForOverView.isChecked(position)) {
                    /* SPRD: bug 445916, change the judgment for the click event @{ */
                    //if (mAdapterForOverView.isAllChecked()) {
                    if (mSelectAllFileCb.isChecked()) {
                    /* @} */
                        //SPRD 426100
                        isFromItemClick = true;
                        mSelectAllFileCb.setChecked(false);
                        selectAllFileTx.setText(R.string.select_all);
                    }
                    mAdapterForOverView.setChecked(position, false);
                    mSelectedFiles.remove(mAdapterForOverView.getFile(position));
                } else {
                    /* SPRD: bug 445916, show the Toast when the select files count more than 500 @{ */
                    if (mAdapterForOverView.getCount() >= MULTI_MAX_SELECT
                            && mSelectedFiles.size() >= MULTI_MAX_SELECT) {
                        Toast.makeText(MultiSelectActivity.this,
                                R.string.failed_select_event,
                                Toast.LENGTH_SHORT).show();
                        return;
                    }
                    /* @} */
                    mSelectedFiles.add(mAdapterForOverView.getFile(position));
                    mAdapterForOverView.setChecked(position, true);
                    if (mSelectedFiles.size() == mAdapterForOverView.getCount()) {
                        //SPRD 426100
                        isFromItemClick = true;
                        mSelectAllFileCb.setChecked(true);
                        selectAllFileTx.setText(R.string.unselect_all);
                    }
                }
                mAdapterForOverView.notifyDataSetChanged();
            }
            this.invalidateOptionsMenu();
        }

    }
    /* SPRD 427743 */
    @Override
    protected void onPause() {
        super.onPause();
        finish();
    }
    /* @} */
    @Override
    protected void onResume() {
        super.onResume();
        refreshAllSelect();
    }

    public void refreshAllSelect(){
        if (mSelectAllFileCb != null) {
            if (mAdapter != null) {
                mSelectAllFileCb.setChecked(mAdapter.isAllChecked());
            } else if (mAdapterForOverView != null) {
                mSelectAllFileCb.setChecked(mAdapterForOverView.isAllChecked());
                if (selectAllFileTx != null && mSelectAllFileCb.isChecked()) {
                    selectAllFileTx.setText(R.string.unselect_all);
                } else {
                    if(selectAllFileTx != null){
                        selectAllFileTx.setText(R.string.select_all);
                    }
                }
            }
        }
    }

    @Override
    protected void onDestroy() {
        if (mAdapter != null) {
            mAdapter.destroyThread();
            mAdapter = null;
        }
        if (mAdapterForOverView != null) {
            mAdapterForOverView.destroyThread();
            mAdapterForOverView = null;
        }
        StorageUtil.removeStorageChangeListener(mStorageChangedListener);
        if (mFileDeleteTask != null) {
            mFileDeleteTask.cancelTask(true);
        }
        super.onDestroy();
    }

    @Override
    public void onClick(View v) {
        if (v != mSelectAllFileCb) {
            return;
        }
        if (mAdapter != null) {
            if (mAdapter.isAllChecked() == true) {
                selectAllFileTx.setText(R.string.select_all);
                mSelectedFiles.clear();
                mAdapter.checkAll(false);
            } else {
                selectAllFileTx.setText(R.string.unselect_all);
                mSelectedFiles.clear();
                for (int i = 0; i < mAdapter.getCount(); i++) {
                    mSelectedFiles.add((File) mAdapter.getItem(i));
                }
                mAdapter.checkAll(true);
            }
            mAdapter.notifyDataSetChanged();
        } else if (mAdapterForOverView != null) {
            if (mAdapterForOverView instanceof QuickScanCursorAdapter) {
                if (mSelectAllFileCb.isChecked()) {
                    mSelectAllFileCb.setChecked(true);
                    selectAllFileTx.setText(R.string.unselect_all);
                    mSelectedFiles.clear();
                    for (int i = 0; i < mAdapterForOverView.getCount(); i++) {
                        mSelectedFiles.add((File) mAdapterForOverView.getFile(i));
                    }
                    mAdapterForOverView.checkAll(true);
                }else {
                    mSelectAllFileCb.setChecked(false);
                    selectAllFileTx.setText(R.string.select_all);
                    mSelectedFiles.clear();
                    mAdapterForOverView.checkAll(false);
                }
                mAdapterForOverView.notifyDataSetChanged();
            } 
        }
        invalidateOptionsMenu();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.multi_select_menu, menu);
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        if (menu != null) {
            MenuItem multiSelectItem = menu.findItem(R.id.menu_delete);
            MenuItem copySelectItem = menu.findItem(R.id.menu_copy);
            MenuItem cutSelectItem = menu.findItem(R.id.menu_cut);
            MenuItem installSelectItem = menu.findItem(R.id.menu_install);
            MenuItem shareSelectItem = menu.findItem(R.id.menu_share);
            if(getIntent().getIntExtra("fileType", -1) != FileType.FILE_TYPE_PACKAGE){
                if(installSelectItem != null){
                    installSelectItem.setVisible(false);
                }
            }
            if(mAdapterForOverView == null){
                if(shareSelectItem != null){
                shareSelectItem.setVisible(false);
                }
            }
            if (mSelectedFiles != null && !mSelectedFiles.isEmpty()) {
                if (multiSelectItem != null) {
                    multiSelectItem.setEnabled(true);
                }
                if (copySelectItem != null) {
                    copySelectItem.setEnabled(true);
                }
                if (cutSelectItem != null) {
                    cutSelectItem.setEnabled(true);
                }
                if(shareSelectItem != null && shareSelectItem.isVisible()){
                    shareSelectItem.setEnabled(true);
                }
                if(installSelectItem != null && installSelectItem.isVisible()){
                    installSelectItem.setEnabled(true);
                }
            } else {
                if (multiSelectItem != null) {
                    multiSelectItem.setEnabled(false);
                }
                if (copySelectItem != null) {
                    copySelectItem.setEnabled(false);
                }
                if (cutSelectItem != null) {
                    cutSelectItem.setEnabled(false);
                }
                if(installSelectItem != null){
                    if(installSelectItem.isVisible()){
                                    installSelectItem.setEnabled(false);
                            }
                }
                if(shareSelectItem != null){
                    if(shareSelectItem.isVisible()){
                                    shareSelectItem.setEnabled(false);
                            }
                }
            }
        }
        return super.onPrepareOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        /* SPRD: Add for bug510243. @{ */
        if (checkSelfPermission(WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            Log.d(TAG, "Start request permissions!");
            requestPermissions(new String[] { WRITE_EXTERNAL_STORAGE }, MULTI_SELECT_STORAGE_PERMISSION_REQUEST_CODE);
            return false;
        }
        /* @} */
        int itemId = item.getItemId();
        switch (itemId) {
        case R.id.menu_delete: {
            showConfirmDeleteDialog();
            return true;
        }
        case R.id.menu_copy:
        case R.id.menu_cut: {
            mCopyFileListener.setCut(itemId == R.id.menu_cut);
            mCopyFileListener.addOpFiles(mSelectedFiles);
            /* SPRD 454659  @{ */
            //if (StorageUtil.getExternalStorageState()) {
            if(ActivityUtils.getAvailableStatus() > 1){
            /* @} */
                ActivityUtils.showDestSelectDialog(this, mCopyFileListener);
            }else{
                mCopyFileListener.onClick(null, 0);
            }
            return true;
        }

        case R.id.menu_install:{
            if(mAdapterForOverView != null && mSelectedFiles != null && !mSelectedFiles.isEmpty()){
                if(mSelectedFiles.size() > FILE_LIMIT){
                    Toast.makeText(this, getString(R.string.install_apk_limited, FILE_LIMIT), Toast.LENGTH_SHORT).show();
                    return true;
                }
                for(File file: mSelectedFiles){
                    Intent intent = IntentUtil.getIntentByFileType(MultiSelectActivity.this, FileType.FILE_TYPE_PACKAGE, file);
                    if(intent != null) {
                        startActivity(intent);
                    } else {
                        Toast.makeText(MultiSelectActivity.this, R.string.msg_invalid_intent, Toast.LENGTH_SHORT).show();
                    }
                }
                finish();
            }

            return true;
        }
        case R.id.menu_share:{
            if(mSelectedFiles.size() > FILE_LIMIT){
                Toast.makeText(this, getString(R.string.share_file_limited, FILE_LIMIT), Toast.LENGTH_SHORT).show();
                return true;
            }
            /* SPRD 506545  @{ */
            Intent intent = null;
            File mSelectedFile = mSelectedFiles.get(0);
            /* SPRD 508339  @{ */
            if (mSelectedFiles.size() > 1) {
                intent = new Intent(Intent.ACTION_SEND_MULTIPLE);
                String type = FileType.getFileType(this).getShareTypeByFile(mSelectedFile);
                ArrayList<Uri> sharedUris = new ArrayList<Uri>();
                for (File file : mSelectedFiles) {
                    // drm file not share
                    if (MultiSelectActivityUtils.getInstance().DRMFileShareClick(file.getPath(), this)) {
                        return true;
                    }
                    /* SPRD 403117 @{ */
                    // sharedUris.add(Uri.fromFile(file));
                    /* SPRD 459367 @{ */
                    // sharedUris.add(FileUtil.getFileUri(file));
                    // SPRD: Add for bug597676.
                    sharedUris.add(FileUtil.getSharedFileUri(file, type, this));
                    /* @} */
                    /* @} */
                }
                intent.setType(type);
                intent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, sharedUris);

            } else {
                /* SPRD: Modify for bug511177. @{ */
                if (MultiSelectActivityUtils.getInstance().DRMFileShareClick(mSelectedFile.getPath(), this)) {
                    return true;
                }
                intent = new Intent(Intent.ACTION_SEND);
                String type = FileType.getFileType(this).getShareTypeByFile(mSelectedFile);
                String mimeType = MultiSelectActivityUtils.getInstance().getDrmFileDetailType(mSelectedFile);
                if(mimeType != null){
                    type = mimeType;
                }
                /* @} */
                // SPRD: Add for bug597676.
                Uri uri = FileUtil.getSharedFileUri(mSelectedFile, type, this);
                intent.setType(type);
                intent.putExtra(Intent.EXTRA_STREAM, uri);
            }
            /* @} */
            startActivity(Intent.createChooser(intent, this.getResources().getString(R.string.operate_share)));
            finish();
            return true;
        }
        case android.R.id.home:{
            onBackPressed();
            return true;
        }
        default:
            return false;
        }
        //return super.onOptionsItemSelected(item);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        ActionBar actionbar = getActionBar();
        actionbar.setTitle(R.string.app_name);
        this.invalidateOptionsMenu();

        if (selectAllFileTx != null && mSelectAllFileCb.isChecked()) {
            selectAllFileTx.setText(R.string.unselect_all);
        } else {
            if(selectAllFileTx != null){
                selectAllFileTx.setText(R.string.select_all);
            }
        }

        if (mAdapter != null) {
            mAdapter.notifyDataSetChanged();
        }
        if(mFileDeleteTask != null){
            mFileDeleteTask.onConfigChanged();
        }
        /* SPRD: add for bug 345830 refresh ConfirmDeleteDialog when ConfigurationChanged @{ */
        if (mConfirmDeleteDialog != null && mConfirmDeleteDialog.isShowing()) {
            mConfirmDeleteDialog.dismiss();
            showConfirmDeleteDialog();
        }
        /* @} */
    }

    public void onChange(int count){
        if(mAdapterForOverView != null && count != mSelectedFiles.size()){
            refreshAllSelect();
            /* SPRD: Bug 472941 When the file manager deletes the file,the file manager stops running. @{ */
            mAdapterForOverView.notifyDataSetChanged();
            /* @} */
        }
    }

    /* SPRD: add for bug 345830 refresh ConfirmDeleteDialog when ConfigurationChanged @{ */
    private void showConfirmDeleteDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this)
                .setTitle(R.string.operate_delete)
                .setMessage(R.string.confirm_delete)
                .setPositiveButton(android.R.string.ok,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                mFileDeleteTask = new FileDeleteTask.Build(
                                        MultiSelectActivity.this, false, true)
                                        .addOperateFiles(mSelectedFiles)
                                        .setOnFinishCallBack(
                                                new OnFinishCallBack() {

                                                   // SPRD: Modify for bug465956.
                                                    @Override
                                                    public void onFinish(
                                                            boolean cancelTask, String path) {
                                                        Log.d(TAG, "showConfirmDeleteDialog: cancelTask = " + cancelTask);
                                                        if (!cancelTask) {
                                                            finish();
                                                        /* SPRD: Add for Bug512310. @{ */
                                                        } else {
                                                            if (mAdapter != null) {
                                                                Log.d(TAG, "showConfirmDeleteDialog: refresh adapter");
                                                                mAdapter.refresh();
                                                            }
                                                        }
                                                        /* @} */
                                                    }

                                                }).creatTask();
                                mFileDeleteTask.start();
                            }

                        }).setNegativeButton(android.R.string.cancel, null);
        mConfirmDeleteDialog = builder.create();
        mConfirmDeleteDialog.show();
    }
    /* @} */
}
