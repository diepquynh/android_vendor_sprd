
package com.sprd.fileexplorer.activities;

import java.io.File;
import java.io.IOException;

import android.annotation.SuppressLint;
import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.ContentUris;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.Intent;
import android.database.Cursor;
import android.media.MediaPlayer;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.provider.MediaStore;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.adapters.CopyDestListAdapter;
import com.sprd.fileexplorer.adapters.FileAdapter.BackStackListener;
import com.sprd.fileexplorer.adapters.FileAdapter.EmptyViewListener;
import com.sprd.fileexplorer.adapters.FileAdapter.LoadingFileListener;
import com.sprd.fileexplorer.adapters.FileAdapter.SortListener;
import com.sprd.fileexplorer.adapters.FileListAdapter;
import com.sprd.fileexplorer.adapters.FileListAdapter.PathChangeListener;
import com.sprd.fileexplorer.drmplugin.FilePickerActivityUtils;
import com.sprd.fileexplorer.util.ActivityUtils;
import com.sprd.fileexplorer.util.FileType;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexplorer.util.IStorageUtil.StorageChangedListener;
import com.sprd.fileexplorer.util.StorageUtil;

import android.view.KeyEvent;
import android.Manifest.permission;
import android.content.pm.PackageManager;


public class FilePickerActivity extends Activity implements OnItemClickListener {
    private File mTopPath;
    private FileListAdapter mAdapter;
    private boolean bIsSdCard = false;
    private ListView mListView = null;
    private View mEmptyView = null;
    private TextView mPathBar;
    private static String TAG = "FilePickerActivity";
    final private static int CROP_IMAGE = 100;
    private MediaPlayer mLocalPlayer;
    private ProgressBar mStandbyView;
    private ActionBar mActionBar;
    private MenuItem mReturnHome = null;
    // SPRD: Add for bug617889.
    private AlertDialog mAlertDialog;
    //SPRD: for bug501992
    private static final int STORAGE_PERMISSION_REQUEST_CODE = 2;
    private static final File USB_LIST_DIRECTORY = new File("USB List");

    private EmptyViewListener mEmptyViewListener = new EmptyViewListener() {

        @Override
        public void onEmptyStateChanged(boolean isEmpty) {
            if (mAdapter != null && mAdapter.mIsLoading) {
                return;
            }
            Log.d(TAG, "onEmptyStateChanged, isEmpty:"+isEmpty);
            if (isEmpty) {
                mEmptyView.setVisibility(View.VISIBLE);
                mListView.setVisibility(View.GONE);
            } else {
                mEmptyView.setVisibility(View.GONE);
                mListView.setVisibility(View.VISIBLE);
            }
        }
    };

    private LoadingFileListener mLoadingFileListener = new LoadingFileListener() {

        @Override
        public void onLoadFileStart() {
            mStandbyView.setVisibility(View.VISIBLE);
            mEmptyView.setVisibility(View.GONE);
            mListView.setVisibility(View.GONE);
        }

        @Override
        public void onLoadFileFinished() {
            Log.d(TAG, "onLoadFileFinished, mAdapter.getCurrentString():"+mAdapter.getCurrentString());
            mStandbyView.setVisibility(View.GONE);
            ensureEmptyView();
            mPathBar.setText(mAdapter.getCurrentString());
        }
    };

    private SortListener mSortListener = new SortListener() {

        @Override
        public void onSortStarting() {
            mStandbyView.setVisibility(View.VISIBLE);
            mListView.setVisibility(View.GONE);
            mEmptyView.setVisibility(View.GONE);
        }

        @Override
        public void onSortFinished() {
            mStandbyView.setVisibility(View.GONE);
            ensureEmptyView();
        }
    };

    private void ensureEmptyView() {
        Log.d(TAG, "ensureEmptyView, mAdapter.isEmpty():"+mAdapter.isEmpty());
        if (!mAdapter.isEmpty()) {
            mListView.setVisibility(View.VISIBLE);
            mEmptyView.setVisibility(View.GONE);
        } else {
            mEmptyView.setVisibility(View.VISIBLE);
            mListView.setVisibility(View.GONE);
        }
    }

    private BackStackListener mBackStackListener = new BackStackListener() {

        @Override
        public void onBackPrevious() {
            if (mAdapter == null) {
                return;
            }
            Log.d(TAG, "onBackPrevious:setSelection");
            mListView.setSelection(mAdapter.restorePreviousPosition(mAdapter.getCurrentPath().getAbsolutePath()));
        }
    };
    
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
        Log.i(TAG, "start onCreate");
        setContentView(R.layout.activity_file_paste);
        /* SPRD: for bug519680. @{ */
        StorageUtil.addStorageChangeListener(mStorageChangedListener);
        /* @} */
        mListView = (ListView) this.findViewById(R.id.list_paste);
        mListView.setOnItemClickListener(this);
        mEmptyView = (View) this.findViewById(R.id.emptyView);
        mPathBar = (TextView) findViewById(R.id.path_bar_file_explore_activity);
        mStandbyView = (ProgressBar) findViewById(R.id.standby);
        mActionBar = getActionBar();
        mActionBar.setDisplayHomeAsUpEnabled(true);
        Intent data = getIntent();
        if (null != data) {
            /* SPRD: Modify for bug607423. @{ */
            if (checkSelfPermission(permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                requestPermissions(new String[] {permission.READ_EXTERNAL_STORAGE},
                        STORAGE_PERMISSION_REQUEST_CODE);
            } else {
                if (StorageUtil.getExternalStorageState() || StorageUtil.getUSBStorageState()) {
                     DialogInterface.OnClickListener listener = new DialogInterface.OnClickListener() {
                         @Override
                         public void onClick(DialogInterface dialog, int which) {
                             bIsSdCard = which != 0;
                             /* SPRD 454659  @{ */
                             //init();
                             init(which);
                             /* @} */
                         }
                     };
                     Builder builder = new AlertDialog.Builder(this);
                     builder.setTitle(R.string.select_destination);
                     /* SPRD 454659  @{ */
                     //BaseAdapter adapter = new CopyDestListAdapter();
                     BaseAdapter adapter = new CopyDestListAdapter(ActivityUtils.getAvailableStatus());
                     /* @} */
                     builder.setAdapter(adapter, listener);
                     builder.setOnCancelListener(new OnCancelListener() {
                         @Override
                         public void onCancel(DialogInterface dialog) {
                             dialog.dismiss();
                             finish();
                         }
                     });
                     AlertDialog dialog = builder.create();
                     dialog.setCanceledOnTouchOutside(false);
                     dialog.show();
                /* SPRD: Modify for bug623005. @{ */
                } else if (StorageUtil.getInternalStorageState()) {
                    /* SPRD 454659 @{ */
                    // init();
                    // set internal storage root path as mTopPath.
                    init(0);
                    /* @} */
                } else {
                    finish();
                }
                /* @} */
            }
            /* @} */
        } else {
            finish();
        }

    }

    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        Log.i(TAG, "start onRequestPermissionsResult: requestCode = "+requestCode);
        /* SPRD: Add for bug617889. @{ */
        if (mAlertDialog != null && mAlertDialog.isShowing()) {
            mAlertDialog.dismiss();
        }
        /* @} */

        switch (requestCode) {
            case STORAGE_PERMISSION_REQUEST_CODE:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Log.i(TAG, "Allow the permission request.");
                    StorageUtil.addStorageChangeListener(mStorageChangedListener);
                    /* SPRD: Modify for bug607423. @{ */
                    if (getIntent() != null) {
                        if (StorageUtil.getExternalStorageState() || StorageUtil.getUSBStorageState()) {
                            DialogInterface.OnClickListener listener = new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    bIsSdCard = which != 0;
                                    init(which);
                                }
                            };
                            Builder builder = new AlertDialog.Builder(this);
                            builder.setTitle(R.string.select_destination);
                            BaseAdapter adapter = new CopyDestListAdapter(ActivityUtils.getAvailableStatus());
                            builder.setAdapter(adapter, listener);
                            builder.setOnCancelListener(new OnCancelListener() {
                                @Override
                                public void onCancel(DialogInterface dialog) {
                                    dialog.dismiss();
                                    finish();
                                }
                            });
                            AlertDialog dialog = builder.create();
                            dialog.setCanceledOnTouchOutside(false);
                            dialog.show();
                        /* SPRD: Modify for bug623005. @{ */
                        } else if (StorageUtil.getInternalStorageState()) {
                            // set internal storage root path as mTopPath.
                            init(0);
                        } else{
                            finish();
                        }
                        /* @} */
                    } else {
                        finish();
                    }
                    /* @} */
                } else {
                    Log.i(TAG, "Deny the permission request.");
                    showConfirmDialog();
                }
                break;
            default:
                break;
        }
    }

    public void showConfirmDialog() {
        Log.d(TAG, "start showConfirmDialog()");
        // SPRD: Modify for bug617889.
        mAlertDialog = new AlertDialog.Builder(this)
                .setTitle(getResources().getString(R.string.toast_fileexplorer_internal_error))
                .setMessage(getResources().getString(R.string.error_permissions))
                .setCancelable(false)
                .setOnKeyListener(new Dialog.OnKeyListener() {
                    @Override
                    public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                        if (keyCode == KeyEvent.KEYCODE_BACK) {
                            finish();
                        }
                        return true;
                    }
                })
                .setPositiveButton(getResources().getString(R.string.dialog_dismiss),
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                finish();
                            }
                        }).show();
    }
    /*@}*/
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.picker_activity_menu, menu);
        mReturnHome = menu.findItem(R.id.action_picker);
        mReturnHome.setVisible(false);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_picker:
                if (mAdapter != null){
                    mAdapter.setCurrentPath(mAdapter.getTopPath());
                    if (mAdapter.getCurrentPath().toString().equals(mAdapter.getTopPath().toString())){
                        mReturnHome.setVisible(false);
                    }
                }
                break;
            case android.R.id.home:
                onBackPressed();
        }
        return super.onOptionsItemSelected(item);
    }
    /* SPRD 454659  @{ */
    private void init(int which){
        if(which == 0){
            /* SPRD: Modify for bug633907. @{ */
            if (StorageUtil.getInternalStorageState()) {
                mTopPath = StorageUtil.getInternalStorage();
            } else if (StorageUtil.getExternalStorageState()) {
                mTopPath = StorageUtil.getExternalStorage();
            } else {
                mTopPath = USB_LIST_DIRECTORY;
            }
            /* @} */
        }
        if(which == 1){
            /* SPRD: Modify for bug607423. @{ */
            if (StorageUtil.getExternalStorageState()) {
                mTopPath = StorageUtil.getExternalStorage();
            } else {
                mTopPath = USB_LIST_DIRECTORY;
            }
            /* @} */
        }
        if(which == 2) {
            mTopPath = USB_LIST_DIRECTORY;
        }
        init();
    }
    /* @} */
    private void init() {
        // SPRD 454659
        //mTopPath = bIsSdCard ? StorageUtil.getExternalStorage() : StorageUtil.getInternalStorage();
        mAdapter = new FileListAdapter(FilePickerActivity.this, mListView);
        mAdapter.initWithPath(mTopPath);
        mAdapter.setLoadingFileListener(mLoadingFileListener);
        mAdapter.setEmptyViewListener(mEmptyViewListener);
        mAdapter.setFileSortListener(mSortListener);
        mAdapter.setBackStackListener(mBackStackListener);
        mAdapter.setPathChangeListener(new PathChangeListener() {
            @Override
            public void onPathChanged(String path) {
                mPathBar.setText(path);
            }
        });
        mPathBar.setText(mAdapter.getCurrentString());
        mListView.setAdapter(mAdapter);
    }

    @Override
    public void onBackPressed() {
        if (mAdapter != null && mAdapter.getCurrentPath().getParent() != null
                && mAdapter.getCurrentPath().getParent().toString().equals(mAdapter.getTopPath().toString())){
            mReturnHome.setVisible(false);
        }
        if (mAdapter == null || mAdapter.popupFolder()) {
            super.onBackPressed();
            return;
        } else {
            mAdapter.setNeedBack();
        }
        mPathBar.setText(mAdapter.getCurrentPath().toString());
    }

    @SuppressLint("DefaultLocale")
    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position,
            long id) {
        File file = mAdapter.getFileList().get(position);
        if (file.isDirectory()) {
            mReturnHome.setVisible(true);
            mAdapter.execute(position, mListView.getFirstVisiblePosition());
        } else if (file.isFile()) {
            String type = FileType.getFileType(FilePickerActivity.this).getTypeByFile(file);
            //get drm file mimeType
            String mimeType = FilePickerActivityUtils.getInstance().isDRMType(file);
            if(mimeType != null){
                int index = mimeType.indexOf("/");
                // SPRD: Add for bug 578953
                if (index >= 0)
                    type = mimeType.substring(0, index) + "/*";
            }
            String requiredType = getIntent().getType();
            Log.d(TAG, "selected file type is" + type + ", required Type is" + requiredType);
            if ("audio/mp3".equals(requiredType) && (getIntent() != null)
                    && Intent.ACTION_GET_CONTENT.equals(getIntent().getAction())) {
                mAdapter.execute(position, mListView.getFirstVisiblePosition());
                return;
            }
            /* SPRD 413768 handle the mimetype like image/jpeg,image/png,image/gif,image/bmp  @{ */
            if(requiredType != null && requiredType.contains(",")){
                requiredType = requiredType.split(",")[0];
                if(requiredType.indexOf("image") == 0){
                    requiredType = "image/*";
                }
            }
            Log.d(TAG, "final required Type is" + requiredType);
            /* @} */
            if ((type == null) || (requiredType != null
                    && !requiredType.equals("*/*") && !requiredType.toLowerCase().equals(type.toLowerCase()))) {
                Toast.makeText(this, R.string.file_type_wrong,
                        Toast.LENGTH_SHORT).show();
                return;
            }
            if (requiredType != null && requiredType.equals("*/*")) {
                Intent intent = new Intent();
                /* SPRD: Modify for bug626513. @{ */
                Uri uri = FileUtil.getFileUri(file, type, this);
                /* SPRD 459367 @{ */
                //intent.setDataAndType(FileUtil.getFileUri(file), type);
                intent.setDataAndType(uri, type);
                if (uri != null && uri.toString().contains(FileUtil.FILE_PROVIDER_URI_HEAD)) {
                    intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                }
                /* @} */
                /* @} */
                setResult(RESULT_OK, intent);
                finish();
                return;
            }

            String externalFilePath = file.getPath();
            if (externalFilePath.startsWith(StorageUtil.getExternalStorage().getPath())) {
                externalFilePath = externalFilePath.replaceFirst(StorageUtil.getExternalStorage().getPath(),
                        Environment.getExternalStorageDirectory().getPath());
                Log.d(TAG, "externalFilePath = " + externalFilePath);
            }
            /* SPRD: modify 20131211 Spreadtrum of 239043 install "wochacha.apk", two-dimension code cannot display @{ */
            /* SPRD 457559 the file must be in the table files,so only use the content uri of table files @{ */
            Uri contentUri = MediaStore.Files.getContentUri("external");
            /*if (type.indexOf("image") == 0) {
                contentUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
            }
            else if(type.indexOf("audio") == 0){
                contentUri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
            }
            else if(type.indexOf("video") == 0){
                contentUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
            }
            else{
                contentUri = MediaStore.Files.getContentUri("external");
            }*/
            /* @} */

            Log.d(TAG, "contentUri = " + contentUri.toString());
            /* @} */
            /* SPRD: Add for Bug 502687. @{ */
            Cursor cursor = null;
            try {
                cursor = getContentResolver().query(
                        contentUri,new String[] { MediaStore.Files.FileColumns._ID },
                        MediaStore.Files.FileColumns.DATA + "=?" + " or " + MediaStore.Files.FileColumns.DATA + "=?",
                        new String[] { file.getPath(), externalFilePath }, null);
            } catch (Exception e1) {
                Log.d(TAG, "Query database error!");
                e1.printStackTrace();
            }
            /* @} */
            Intent intent = new Intent();
            boolean hasRecord = false;
            long fileId = 0;
            if (cursor != null && cursor.moveToFirst()) {
                Log.i(TAG, "get the content uri");
                fileId = cursor.getLong(0);
                intent.setDataAndType(ContentUris.withAppendedId(contentUri, fileId), type);
                hasRecord = true;
                cursor.close();
            }else {
                if (cursor != null) {
                    cursor.close();
                }
                // SPRD: Add for bug601773.
                Log.w(TAG, file + " isn't in media database, fail to get the file uri");
                //intent.setDataAndType(Uri.parse("file://" + file.getPath()), type);
            }
            /* SPRD: Add for bug 506739 @{ */
            if (getIntent() != null && Intent.ACTION_GET_CONTENT.equals(getIntent().getAction())){
                /* SPRD: 602185 SD DRM file can set to slideshow music @{ */
                boolean isGallery2SlideMusic = getIntent().hasExtra("applyForSlideMusic");
                if (isGallery2SlideMusic && FilePickerActivityUtils.getInstance().isDrmFile(file.getPath())) {
                    Toast.makeText(this, R.string.error_choose_drm_for_gallery, Toast.LENGTH_SHORT).show();
                    return;
                }
                /* @} */
                /* SPRD: Add for bug 506739, support for sdcard file  @{ */
                if (FilePickerActivityUtils.getInstance().isSupportDrm(file.getPath(), this)){
                    return;
                }
            }
            /* @} */
            if (getIntent() != null && RingtoneManager.ACTION_RINGTONE_PICKER.equals(getIntent().getAction())) {
                boolean isFromMms = getIntent().getExtras().getBoolean("mms_audio_attachment");
                if (!type.startsWith("audio") || (!type.startsWith("audio") && !mimeType.equals("application/ogg"))) {
                    Toast.makeText(this, R.string.file_type_wrong, Toast.LENGTH_SHORT).show();
                    return;
                }
                //drm audio file not select except mms
                if (FilePickerActivityUtils.getInstance().isDRMFileSelect(externalFilePath,isFromMms,this)){
                    return;
                }
                /* SPRD : BugFix bug395317 & bug538447, add Ringtone, do not give a not support notify @{ */
                if (!isFromMms && !isMediaAvaliable(ContentUris.withAppendedId(contentUri, fileId))) {
                    Toast.makeText(this, R.string.unsupport_type, Toast.LENGTH_SHORT).show();
                    return;
                }
                if (hasRecord) {
                    intent.putExtra(RingtoneManager.EXTRA_RINGTONE_PICKED_URI,
                            ContentUris.withAppendedId(contentUri, fileId));
                }else {
                    /* SPRD: Add for bug601773. @{ */
                    Log.w(TAG, file + " isn't in media database, do not send the file uri");
                    //intent.putExtra(RingtoneManager.EXTRA_RINGTONE_PICKED_URI,
                    //        Uri.parse("file://" + file.getPath()));
                    /* @} */
                }
            }
            //set_wallpaper
            if (getIntent() != null
                    && ("android.intent.action.SET_WALLPAPER".equals(getIntent().getAction()) ||
                    "android.intent.action.SET_SPREAD_WALLPAPER".equals(getIntent().getAction()))) {
                if (!type.startsWith("image")) {
                    Toast.makeText(this, R.string.file_type_wrong, Toast.LENGTH_SHORT).show();
                    return;
                }
                try {
                    int targetId = getIntent().getIntExtra("wallpaper_target_id", 0);
                    int width = getWallpaperDesiredMinimumWidth();
                    int height = getWallpaperDesiredMinimumHeight();
                    intent.setAction("com.android.camera.action.CROP");
                    intent.putExtra("set-as-wallpaper", true);
                    intent.putExtra("wallpaper_target_id", targetId);
                    //drm file not set wallpaper from setting
                    Log.d("defang","before isDRMSetWallpaper");
                    if(FilePickerActivityUtils.getInstance().isDRMSetWallpaper(file.getPath(),this)){
                        return;
                    }else{
                        intent.putExtra("outputX",width);
                        intent.putExtra("outputY",height);
                        intent.putExtra("aspectX",width);
                        intent.putExtra("aspectY",height);
                        intent.putExtra("scale",true);
                        intent.putExtra("scaleUpIfNeeded",true);
                        startActivityForResult(intent, 100);
                    }
                    finish();
                } catch (ActivityNotFoundException e) {
                    Toast.makeText(this, R.string.failed_set_wallpaper,
                            Toast.LENGTH_SHORT).show();
                }
           }
            if (getIntent().hasExtra(MediaStore.EXTRA_OUTPUT)) {
                try {
                    Log.i(TAG, "has EXTRA_OUTPUT");
                    Intent selected = new Intent(null,intent.getData());
                  //drm image file not select when set concats profile
                    Log.d("defang","before isDRMToGalley");
                    if(FilePickerActivityUtils.getInstance().isDRMToGalley(file.getPath(),this)){
                        return;
                    }else{
                        setResult(RESULT_OK,selected);
                    }
                    finish();
                } catch (ActivityNotFoundException e) {
                    Log.e(TAG, "unable to crop the image");
                }
                return;
            }

            if (getIntent() != null && getIntent().hasExtra("applyForWallpaper")){
                
                //drm not set wallpaper from launcher
                Log.d("defang","before isDRMFromLauncher");
                if(FilePickerActivityUtils.getInstance().isDRMFromLauncher(file.getPath(),this)){
                    return;
                }else{
                    setResult(RESULT_OK, intent);
                    finish();
                }
            }
            setResult(RESULT_OK, intent);
            finish();
        }
    }

    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        Log.i(TAG, "onActivityResult");
        if (resultCode != Activity.RESULT_OK) {
            Log.i(TAG, "resultCode != this.RESULT_OK, return");
            return;
        }
        switch (requestCode) {
            case CROP_IMAGE:
                setResult(RESULT_OK, data);
                finish();
                return;
        }

    }

    private boolean isMediaAvaliable(Uri localUri) {
        if (localUri == null) {
            return false;
        }
        mLocalPlayer = new MediaPlayer();
        try {
            mLocalPlayer.setDataSource(this, localUri);
            // SPRD: Add for bug536957.
            mLocalPlayer.prepare();
        } catch (SecurityException e) {
            return false;
        } catch (IOException e) {
            return false;
        } finally {
            destroyLocalPlayer();
        }
        return true;
    }

    private void destroyLocalPlayer() {
        if (mLocalPlayer != null) {
            mLocalPlayer.reset();
            mLocalPlayer.release();
            mLocalPlayer = null;
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        StorageUtil.removeStorageChangeListener(mStorageChangedListener);
        if (mAdapter != null) {
            Log.i(TAG, "call mAdapter destroy");
            mAdapter.destroyThread();
            mAdapter.destroy();
        }
    }
}
