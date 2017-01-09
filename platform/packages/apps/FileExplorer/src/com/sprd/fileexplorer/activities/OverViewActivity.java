
package com.sprd.fileexplorer.activities;

import java.io.File;
import java.util.List;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ContentUris;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.provider.MediaStore;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnCreateContextMenuListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ListView;
import android.widget.Toast;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.activities.FileExploreActivity.StorageStatus;
import com.sprd.fileexplorer.adapters.FileAdapter.EmptyViewListener;
import com.sprd.fileexplorer.adapters.FileListAdapter;
import com.sprd.fileexplorer.adapters.QuickScanCursorAdapter;
import com.sprd.fileexplorer.drmplugin.OverViewActivityUtils;
import com.sprd.fileexplorer.util.ActivityUtils;
import com.sprd.fileexplorer.util.ActivityUtils.CopyFileListener;
import com.sprd.fileexplorer.util.FileDeleteTask;
import com.sprd.fileexplorer.util.FileSort;
import com.sprd.fileexplorer.util.FileType;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexplorer.util.FileUtilTask.OnFinishCallBack;
import com.sprd.fileexplorer.util.IStorageUtil.StorageChangedListener;
import com.sprd.fileexplorer.util.IntentUtil;
import com.sprd.fileexplorer.util.OnPastePathChangedListener;
import com.sprd.fileexplorer.util.StorageUtil;

import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;
import static android.Manifest.permission.READ_EXTERNAL_STORAGE;

public class OverViewActivity extends Activity implements OnItemClickListener, OnPastePathChangedListener {
    
    private static final String TAG = "OverViewActivity";
    
    private final int MENU_MULTISELECT = 0;
    private final int MENU_SORT = 1;
    private final int MENU_INSTALL_ALL = 2;
    public static final int MENU_COPY = 10;
    public static final int MENU_CUT = 11;
    public static final int MENU_DELETE = 12;
    public static final int MENU_SHARE = 13;
    public static final int MENU_RENAME = 14;
    public static final int MENU_DETAIL = 15;
    public static final int MENU_SETAS = 16;

    private static final int FILE_LIMIT = 40;
    // SPRD: Add for bug500477.
    private static final int OVERVIEW_STORAGE_PERMISSION_REQUEST_CODE = 3;
    private static final String REQUEST = "request";
    private boolean mRequested = false;

    private int mFileType;
    private ListView mListView;
    private View mEmptyView;
    private QuickScanCursorAdapter mAdapter;
    private Context mContext;
    private int mSortBy;
    
    private Handler handler = new Handler();
    // SPRD: Add for bug616271.
    private long mLastClickTime = 0;

    StorageChangedListener mStorageChangedListener = new StorageChangedListener() {

        // SPRD: Modify for bug509242.
        @Override
        public void onStorageChanged(String path, boolean available, boolean sdcard) {
            Log.d(TAG, "StorageChanged: path = " + path + " available = " + available + "; sdcard = " + sdcard);
            if(available) {
                return;
            }
            handler.postDelayed(new Runnable() {

                @Override
                public void run() {
                    OverViewActivity.this.onStorageChanged();
                }
            }, 1000);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "start onCreate()");
        setContentView(R.layout.activity_over_view);
        mContext = OverViewActivity.this;
        // SPRD: Add for bug616801.
        FileUtil.addPastePathChangeListener(this);
        StorageUtil.addStorageChangeListener(mStorageChangedListener);
        /* SPRD: Modify for bug519899, save the latest state with request permission. @{ */
        if (savedInstanceState != null) {
            mRequested = savedInstanceState.getBoolean(REQUEST, false);
        }
        Intent data = getIntent();
        if (null != data) {
            /* SPRD: Modify for bug522274，check the runtime permission. @{ */
            if (checkSelfPermission(WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
                init(data);
            }
            /* @} */
        } else {
            finish();
        }
        /* @} */

    }

    /* SPRD: Modify for bug522274，check the runtime permission. @{ */
    @Override
    protected void onStart() {
        // TODO Auto-generated method stub
        super.onStart();
        Log.d(TAG, "onStart(): mRequested = " + mRequested);
        if (!mRequested && checkSelfPermission(WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            mRequested = true;
            /** SPRD Fix BUG# 526464, Changed file name incorrect after reset APP preferences {@ **/
            FileUtil.dismissDialog();
            /** @} **/
            // SPRD: bug535304 destroyLoader when request permissions in onStart()
            if (mAdapter != null) {
                mAdapter.end();
            }
            requestPermissions(new String[] { WRITE_EXTERNAL_STORAGE }, OVERVIEW_STORAGE_PERMISSION_REQUEST_CODE);
        }
    }
    /* @} */

    /* SPRD: Modify for bug519899, save the latest state with request permission. @{ */
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        Log.d(TAG, "onSaveInstanceState(): mRequested = " + mRequested);
        outState.putBoolean(REQUEST, mRequested);
    }
    /* @} */

    /* SPRD: Modify for bug500477. @{ */
    private void init(Intent intent) {
        Log.i(TAG, "start init()");
        ActionBar actionbar = getActionBar();
        actionbar.setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP | ActionBar.DISPLAY_SHOW_TITLE);
        mFileType = intent.getIntExtra("fileType", 0);
        mListView = (ListView) findViewById(R.id.overview_file_list);
        mEmptyView = findViewById(R.id.overview_empty_view);
        switch (mFileType) {
            case FileType.FILE_TYPE_AUDIO:
                mAdapter = new QuickScanCursorAdapter(this,
                        QuickScanCursorAdapter.RESOURCE_AUDIO, mEmptyView, mListView, false);
                actionbar.setTitle(R.string.quickscan_audio);
                actionbar.setLogo(R.drawable.logo_quickscan_audio_ic);
                break;
            case FileType.FILE_TYPE_IMAGE:
                mAdapter = new QuickScanCursorAdapter(this,
                        QuickScanCursorAdapter.RESOURCE_IMAGE, mEmptyView, mListView, false);
                actionbar.setTitle(R.string.quickscan_image);
                actionbar.setLogo(R.drawable.logo_quickscan_image_ic);
                break;
            case FileType.FILE_TYPE_DOC:
                mAdapter = new QuickScanCursorAdapter(this,
                        QuickScanCursorAdapter.RESOURCE_DOC, mEmptyView, mListView, false);
                actionbar.setTitle(R.string.quickscan_doc);
                actionbar.setLogo(R.drawable.logo_quickscan_doc_ic);
                break;
            case FileType.FILE_TYPE_PACKAGE:
                mAdapter = new QuickScanCursorAdapter(this,
                        QuickScanCursorAdapter.RESOURCE_APK, mEmptyView, mListView, false);
                actionbar.setTitle(R.string.quickscan_apk);
                actionbar.setLogo(R.drawable.logo_quickscan_apk_ic);
                break;
            case FileType.FILE_TYPE_VIDEO:
                mAdapter = new QuickScanCursorAdapter(this,
                        QuickScanCursorAdapter.RESOURCE_VIDEO, mEmptyView, mListView, false);
                actionbar.setTitle(R.string.quickscan_video);
                actionbar.setLogo(R.drawable.logo_quickscan_video_ic);
                break;
        }

        mAdapter.setEmptyViewListener(new EmptyViewListener() {
            @Override
            public void onEmptyStateChanged(boolean isEmpty) {
                if (mAdapter != null && !mAdapter.mIsLoading) {
                    if (isEmpty) {
                        mListView.setVisibility(View.GONE);
                        mEmptyView.setVisibility(View.VISIBLE);
                    } else {
                        mListView.setVisibility(View.VISIBLE);
                        mEmptyView.setVisibility(View.GONE);
                    }
                }
            }
        });
        mListView.setAdapter(mAdapter);
        mListView.setOnItemClickListener(this);
        /* SPRD: bug fix 518998 need check runtime permission @{ */
        mListView.setOnItemLongClickListener(new ListView.OnItemLongClickListener() {
            public boolean onItemLongClick(AdapterView<?> arg0, View arg1,
                    int pos, long id) {
                if (!(checkSelfPermission(WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED)) {
                    Toast.makeText(mContext, R.string.error_permissions, Toast.LENGTH_SHORT).show();
                    return true;
                }
                return false;
            }
        });
        /* @} */
        mListView.setOnCreateContextMenuListener(mFileListOnCreateContextMenuListener);
    }

    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        Log.i(TAG, "start onRequestPermissionsResult: requestCode = " + requestCode + "; mRequested = " + mRequested);
        // Modify for bug519899
        mRequested = false;
        switch (requestCode) {
            case OVERVIEW_STORAGE_PERMISSION_REQUEST_CODE:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    if (null != getIntent()) {
                        init(getIntent());
                        // SPRD: Add for bug513479, recreate the options menu.
                        invalidateOptionsMenu();
                    } else {
                        finish();
                    }
                } else {
                    showConfirmDialog();
                }
                break;
            default:
                break;
        }
    }


    public void showConfirmDialog() {
        Log.d(TAG, "start showConfirmDialog()");
        AlertDialog builder = new AlertDialog.Builder(this)
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
    /* @} */

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position,
            long id) {
        Log.d(TAG, "onItemClick " + position);
        /* SPRD: Add for bug616271. @{ */
        long currentTime = System.currentTimeMillis();
        if (currentTime - mLastClickTime < FileUtil.MIN_CLICK_DELAY_TIME) {
            return;
        }
        /* @} */
        if (mAdapter == null || mAdapter.mIsLoading) {
            return;
        }
        File file = mAdapter.getFileList().get(position);
        final Intent intent = IntentUtil.getIntentByFileType(this, mFileType, file);
        //is click drm file
        if(OverViewActivityUtils.getInstance().clickDRMFile(mAdapter,file,view,this)){
            return;
        }
        if (intent != null) {
            // SPRD: Add for bug616271.
            mLastClickTime = currentTime;
            this.startActivity(intent);
        } else {
            Toast.makeText(view.getContext(), R.string.msg_invalid_intent, Toast.LENGTH_SHORT).show();
        }
    }

    private final OnCreateContextMenuListener mFileListOnCreateContextMenuListener = new OnCreateContextMenuListener() {
        @Override
        public void onCreateContextMenu(ContextMenu menu, View v,
                ContextMenuInfo menuInfo) {
            AdapterView.AdapterContextMenuInfo info;
            try {
                info = (AdapterView.AdapterContextMenuInfo) menuInfo;
            } catch (ClassCastException e) {
                return;
            }
            if (mAdapter != null && mAdapter.mIsLoading) {
                return;
            }
            final int position = info.position;
            File selectedFile = null;
            Uri selectedFileUri = null;
            FileListMenuClickListener l = null;
            if (mAdapter instanceof QuickScanCursorAdapter) {
                selectedFile = mAdapter.getFileList().get(position);
                selectedFileUri = mAdapter.getFileUri(position);
                l = new FileListMenuClickListener(selectedFile, selectedFileUri);
            }
            if (selectedFile == null){
                return;
            }
            menu.setHeaderTitle(R.string.file_option);

            //drm file not copy menu item
            if(OverViewActivityUtils.getInstance().DRMFileCopyMenu(selectedFile)){
                menu.add(0, MENU_COPY, 0, R.string.menu_copy).setOnMenuItemClickListener(l);
            }
            
            menu.add(0, MENU_CUT, 0, R.string.menu_cut).setOnMenuItemClickListener(l);
            menu.add(0, MENU_DELETE, 0, R.string.operate_delete).setOnMenuItemClickListener(l);
            menu.add(0, MENU_RENAME, 0, R.string.operate_rename).setOnMenuItemClickListener(l);
            //has menu item only sd type in drm file
            if(OverViewActivityUtils.getInstance().DRMFileShareMenu(selectedFile)){
                menu.add(0, MENU_SHARE, 0, R.string.operate_share).setOnMenuItemClickListener(l);
            }
            if (mFileType == FileType.FILE_TYPE_IMAGE) {
              //drm file not setas menu item
                if(OverViewActivityUtils.getInstance().DRMFileSetAsMenu(selectedFile)){
                    menu.add(0, MENU_SETAS, 0, R.string.set_as).setOnMenuItemClickListener(l);
                }
            }
            menu.add(0, MENU_DETAIL, 0, R.string.operate_viewdetails).setOnMenuItemClickListener(l);
            //drm file add protect menu item
            OverViewActivityUtils.getInstance().protectMenu(menu,selectedFile,mContext);
        }
    };

    public void onStorageChanged() {
        finish();
    }


    /* SPRD: add for Bug 431677, OptionsMenu is not closed when activity changed @{ */
    @Override
    protected void onPause() {
        super.onPause();
        closeOptionsMenu();
    }
    /* @} */

    /* SPRD: Add for bug465956. @{ */
    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data){
        switch(resultCode){
            case ActivityUtils.COPY_PATH_RESULT:
                if (data != null) {
                    String pastePath = data.getStringExtra(FilePasteActivity.PASTE_DEST_PATH);
                    Intent intent = new Intent();
                    intent.putExtra(FilePasteActivity.PASTE_DEST_PATH, pastePath);
                    setResult(ActivityUtils.COPY_PATH_RESULT, intent);
                    finish();
                }
                break;
            default:
                break;
        }
    }
    /* @} */

    @Override
    protected void onDestroy() {
        Log.d(TAG, "start onDestroy()");

        if (mAdapter != null) {
            mAdapter.destroyThread();
            mAdapter = null;
        }
        // SPRD: Add for bug616801.
        FileUtil.removePastePathChangeListener(this);
        StorageUtil.removeStorageChangeListener(mStorageChangedListener);
        super.onDestroy();
    };

    private final class FileListMenuClickListener implements
            MenuItem.OnMenuItemClickListener {
        private File mClickedFile;
        private Uri mClickedFileUri;

        public FileListMenuClickListener(File selectedFile, Uri selectedFileUri) {
            mClickedFile = selectedFile;
            mClickedFileUri = selectedFileUri;
        }

        private CopyFileListener copySingleFileListener = new CopyFileListener(OverViewActivity.this);

        @Override
        public boolean onMenuItemClick(MenuItem item) {
            if (mClickedFile == null && mClickedFileUri == null) {
                return false;
            }
            int itemId = item.getItemId();
            switch (itemId) {
                case MENU_COPY:
                case MENU_CUT: {
                    copySingleFileListener.setCut(itemId == MENU_CUT);
                    copySingleFileListener.addOpFile(mClickedFile);
                    /* SPRD 454659  @{ */
                    //if (StorageUtil.getExternalStorageState()) {
                    if(ActivityUtils.getAvailableStatus() > 1){
                    /* @} */
                        ActivityUtils.showDestSelectDialog(mContext, copySingleFileListener);
                    }else{
                        copySingleFileListener.onClick(null, 0);
                    }
                    return true;
                }
                case MENU_DELETE:
                    new AlertDialog.Builder(mContext)
                    .setTitle(R.string.operate_delete)
                    .setMessage(R.string.confirm_delete)
                    .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {

                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            new FileDeleteTask.Build(OverViewActivity.this, false, false)
                            .addOperateFile(mClickedFile)
                            .setOnFinishCallBack(new OnFinishCallBack() {

                                // SPRD: Modify for bug465956.
                                @Override
                                public void onFinish(boolean cancelTask, String path) {
                                    if(!cancelTask) {
                                        mAdapter.getFileList().remove(mClickedFile);
                                        mAdapter.notifyDataSetChanged();
                                    }
                                }

                            }).creatTask().start();
                        }

                    })
                    .setNegativeButton(android.R.string.cancel, null)
                    .show();
                    return true;
                case MENU_SHARE:
                    Intent intent = new Intent(Intent.ACTION_SEND);
                    String type = FileType.getFileType(OverViewActivity.this)
                            .getShareTypeByFile(mClickedFile);
                    //get drm file mimeType
                    String mimeType = OverViewActivityUtils.getInstance().DRMFileShareClick(mClickedFile);
                    if(mimeType != null){
                        type = mimeType;
                    }
                    /* SPRD 403117 @{ */
                    //Uri uri = Uri.parse("file://" + mClickedFile.getPath());
                    /* SPRD 459367 @{ */
                    //Uri uri = FileUtil.getFileUri(mClickedFile);
                    // SPRD: Add for bug597676.
                    Uri uri = FileUtil.getSharedFileUri(mClickedFile,type,mContext);
                    /* @} */
                    /* @} */
                    intent.setType(type);
                    intent.putExtra(Intent.EXTRA_STREAM, uri);
                    startActivity(Intent.createChooser(intent, mContext
                            .getResources().getString(R.string.operate_share)));
                    return true;
                case MENU_RENAME:
                    if (mAdapter instanceof QuickScanCursorAdapter) {
                        FileUtil.rename(mClickedFile, mClickedFileUri,
                                (QuickScanCursorAdapter) mAdapter, mContext);
                    }
                    return true;
                case MENU_DETAIL:
                    FileUtil.viewDetails(mClickedFile, mContext);
                    return true;
                case MENU_SETAS:
                    /* SPRD: Modify for bug627798. @{ */
                    Intent asIntent = new Intent(Intent.ACTION_ATTACH_DATA)
                            .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                    String filetype = FileType.getFileType(OverViewActivity.this).getTypeByFile(
                            mClickedFile);
                    Uri contentUri = null;
                    Cursor cursor = null;
                    long id = -1;
                    try {
                        contentUri = MediaStore.Files.getContentUri("external");
                        cursor = mContext.getContentResolver().query(contentUri,
                                new String[] { MediaStore.Files.FileColumns._ID },
                                MediaStore.Files.FileColumns.DATA + "=?",
                                new String[] { mClickedFile.getPath() },
                                null);
                        if (cursor != null && cursor.getCount() != 0) {
                            cursor.moveToFirst();
                            id = cursor.getLong(0);
                        }
                    } catch (Exception e) {
                        Log.i(TAG, "onMenuItemClick(): Have an exception, query fail!", e);
                    } finally {
                        if (cursor != null) {
                            cursor.close();
                        }
                    }

                    if (id != -1) {
                        asIntent.setDataAndType(ContentUris.withAppendedId(contentUri, id), filetype);
                        mContext.startActivity(Intent.createChooser(asIntent, mContext
                                .getResources().getString(R.string.set_as)));
                    } else {
                        Toast.makeText(mContext, mContext.getResources().getString(R.string.failed_query),
                                Toast.LENGTH_LONG).show();
                    }
                    return true;
                    /* @} */
                default:
                    return false;
            }
        }

    }

    /* SPRD: add for Bug 431677, OptionsMenu is not closed when activity changed  @{ */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        /* SPRD: Modify for bug463013. @{ */
        if (menu != null) {
            menu.removeGroup(1);
        }
        switch (this.mFileType) {
            case FileType.FILE_TYPE_IMAGE:
            case FileType.FILE_TYPE_AUDIO:
            case FileType.FILE_TYPE_VIDEO:
            case FileType.FILE_TYPE_DOC:
                if (menu != null) {
                    menu.add(1, this.MENU_MULTISELECT, 0,
                            R.string.overview_menu_multiselect);
                    menu.add(1, this.MENU_SORT, 0, R.string.menu_sort_type);
                }
                break;
            case FileType.FILE_TYPE_PACKAGE:
                if (menu != null) {
                    menu.add(1, this.MENU_MULTISELECT, 0,
                            R.string.overview_menu_multiselect);
                    if(mAdapter != null && mAdapter.getCount() <= FILE_LIMIT){
                        menu.add(1, this.MENU_INSTALL_ALL, 0,
                                R.string.overview_menu_install_all);
                    }
                    menu.add(1, this.MENU_SORT, 0, R.string.menu_sort_type);
                }
                break;
        }
        if (menu != null) {
            return super.onCreateOptionsMenu(menu);
        } else {
            return false;
        }
        /* @} */
    }


    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        if (mAdapter != null && mAdapter.getCount() > 0) {
            menu.setGroupEnabled(1, true);
        } else {
            menu.setGroupEnabled(1, false);
        }
        return super.onPrepareOptionsMenu(menu);
    }
    /* @} */

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        /* SPRD: bug519834 need check runtime permission @{ */
        if (checkSelfPermission(WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            Toast.makeText(mContext, R.string.error_permissions, Toast.LENGTH_SHORT).show();
            return true;
        }
        /* @} */
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
            case MENU_MULTISELECT:
                Intent multiSelectIntent = new Intent(mContext, MultiSelectActivity.class);
                multiSelectIntent.putExtra("fileType", mFileType);
                multiSelectIntent.putExtra("position", mListView.getFirstVisiblePosition());
                startActivity(multiSelectIntent);
                break;
            case MENU_SORT:
                AlertDialog.Builder sortTypeDialog = new AlertDialog.Builder(mContext);
                sortTypeDialog.setTitle(R.string.menu_sort_type);
                final int sortType;
                final SharedPreferences settings = mContext.getSharedPreferences(
                        FileSort.FILE_SORT_KEY, 0);
                sortType = settings.getInt(FileSort.FILE_SORT_KEY, FileSort.SORT_BY_NAME);
                int selectItem = FileSort.getSelectItemByType(sortType);
                mSortBy = selectItem;
                sortTypeDialog.setSingleChoiceItems(R.array.sort_type, selectItem,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                mSortBy = whichButton;
                            }
                        });
                sortTypeDialog.setNegativeButton(R.string.sort_by_asc,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                switch (mSortBy) {
                                    case 0:
                                        mSortBy = FileSort.SORT_BY_NAME;
                                        break;
                                    case 1:
                                        mSortBy = FileSort.SORT_BY_TYPE;
                                        break;
                                    case 2:
                                        mSortBy = FileSort.SORT_BY_TIME_ASC;
                                        break;
                                    case 3:
                                        mSortBy = FileSort.SORT_BY_SIZE_ASC;
                                        break;
                                }
                                settings.edit().putInt(FileSort.FILE_SORT_KEY, mSortBy).commit();
                                FileSort.getFileListSort().setSortType(mSortBy);
                                if (mSortBy == FileSort.SORT_BY_TYPE) {
                                    new Thread(new Runnable() {
                                        @Override
                                        public void run() {
                                            // TODO Auto-generated method stub
                                            FileSort.getFileListSort().sort(mAdapter.getFileList());
                                            handler.post(new Runnable() {

                                                @Override
                                                public void run() {
                                                    // TODO Auto-generated
                                                    // method stub
                                                    mAdapter.notifyDataSetChanged();
                                                }

                                            });
                                        }
                                    }).start();

                                } else {
                                    mAdapter.refresh();
                                }
                            }
                        });
                sortTypeDialog.setPositiveButton(R.string.sort_by_desc,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                switch (mSortBy) {
                                    case 0:
                                        mSortBy = FileSort.SORT_BY_NAME_DESC;
                                        break;
                                    case 1:
                                        mSortBy = FileSort.SORT_BY_TYPE_DESC;
                                        break;
                                    case 2:
                                        mSortBy = FileSort.SORT_BY_TIME_DESC;
                                        break;
                                    case 3:
                                        mSortBy = FileSort.SORT_BY_SIZE_DESC;
                                        break;
                                }
                                settings.edit().putInt(FileSort.FILE_SORT_KEY, mSortBy).commit();
                                FileSort.getFileListSort().setSortType(mSortBy);
                                if (mSortBy == FileSort.SORT_BY_TYPE_DESC) {
                                    new Thread(new Runnable() {
                                        @Override
                                        public void run() {
                                            // TODO Auto-generated method stub
                                            FileSort.getFileListSort().sort(mAdapter.getFileList());
                                            handler.post(new Runnable() {
                                                @Override
                                                public void run() {
                                                    // TODO Auto-generated
                                                    // method stub
                                                    mAdapter.notifyDataSetChanged();
                                                }
                                            });
                                        }
                                    }).start();
                                } else {
                                    mAdapter.refresh();
                                }
                            }
                        });
                sortTypeDialog.show();
                break;
            case MENU_INSTALL_ALL:
                if (mAdapter instanceof QuickScanCursorAdapter) {
                    List<File> apkList = mAdapter.getFileList();
                    for (File file : apkList) {
                        Intent intent = IntentUtil.getIntentByFileType(mContext, mFileType, file);
                        if (intent != null) {
                            mContext.startActivity(intent);
                        } else {
                            Toast.makeText(mContext, R.string.msg_invalid_intent,
                                    Toast.LENGTH_SHORT).show();
                        }
                    }
                }
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    /* SPRD: Add for bug616801. @{ */
    @Override
    public void onPasteFinsish(String path) {
        Log.d(TAG, "onPasteFinsish(): path = " + path);
        if (path != null) {
            Intent intent = new Intent();
            intent.putExtra(FilePasteActivity.PASTE_DEST_PATH, path);
            setResult(ActivityUtils.COPY_PATH_RESULT, intent);
            finish();
        }
    }
    /* @} */
}
