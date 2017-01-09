package com.sprd.fileexplorer.activities;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.R.integer;
import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnCreateContextMenuListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ListView;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.adapters.SearchListAdapter;
import com.sprd.fileexplorer.drmplugin.FileSearchResultActivityUtils;
import com.sprd.fileexplorer.util.ActivityUtils;
import com.sprd.fileexplorer.util.ActivityUtils.CopyFileListener;
import com.sprd.fileexplorer.util.FileDeleteTask;
import com.sprd.fileexplorer.util.FileSearch;
import com.sprd.fileexplorer.util.FileSort;
import com.sprd.fileexplorer.util.FileType;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexplorer.util.FileUtilTask.OnFinishCallBack;
import com.sprd.fileexplorer.util.IStorageUtil.StorageChangedListener;
import com.sprd.fileexplorer.util.StorageUtil;

public class FileSearchResultActivity extends Activity {
    
    public static final String TAG = "FileSearchResultActivity";
    public static final int MENU_COPY = 1;
    public static final int MENU_CUT = 2;
    public static final int MENU_DELETE = 3;
    public static final int MENU_CLEAR = 4;
    public static final int MENU_SHARE = 5;
    public static final int MENU_RENAME = 6;
    public static final int MENU_DETAIL = 7;
    
    private SearchListAdapter mAdapter;
    private ListView mListView = null;
    private View mEmptyView = null;
    private SearchTask mSearchTask = new SearchTask();
    private List<File> mSearchLoaction = new ArrayList<File>();
    public String mSearchKey;
    public ArrayList<Integer> mSearchType;
    public Context mContext;
    private ProgressDialog mWaitDialog;
    private CopyFileListener mCopyFileListener;
    private boolean mIsIncludeDrmFile = false;
    /* SPRD 440831 @{ */
    private static final int MAX_CHECK_COUNT = 100;
    private int mCheckCount = 0;
    /* @} */
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
        setContentView(R.layout.activity_file_searchresult);
        mContext = this;
        StorageUtil.addStorageChangeListener(mStorageChangedListener);
        mListView = (ListView) this.findViewById(R.id.list_paste);
        mEmptyView = (View) this.findViewById(R.id.emptyView);
        mAdapter = new SearchListAdapter(this, mListView);
        mListView.setAdapter(mAdapter);
        mListView.setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> arg0, View arg1, int arg2,
                    long arg3) {
                mAdapter.execute(arg2);
            }

        });
        mListView.setOnCreateContextMenuListener(mFileListOnCreateContextMenuListener);

        Intent data = getIntent();
        if (null != data) {
            Bundle bun = data.getBundleExtra(FileSearch.SEARCH_ATTACH);
            if (bun != null) {
                mSearchKey = bun.getString(FileSearch.SEARCH_KEY);
                mSearchType = bun.getIntegerArrayList(FileSearch.SEARCH_TYPE);
                int searchLocation = bun.getInt(FileSearch.SEARCH_LOCATION);
                getSearchLoaction(searchLocation);
                if (mSearchKey != null && !mSearchKey.isEmpty()
                        && this.mSearchLoaction != null) {
                    mSearchTask = new SearchTask();
                    mSearchTask.execute("");
                }
            }
        } else {
            finish();
        }
        ActionBar actionbar = getActionBar();
        actionbar.setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP | ActionBar.DISPLAY_SHOW_TITLE);
        mCopyFileListener = new CopyFileListener(this);
    }

    /* SPRD： Add for bug600052, refresh UI. @{ */
    @Override
    protected void onResume() {
        super.onResume();
        if (mAdapter != null) {
            mAdapter.notifyDataSetChanged();
        }
    }
    /* @} */

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
         switch (item.getItemId()) {
             case android.R.id.home:
                 onBackPressed();
                 return true;
             default :
                 return false;
         }
     }

    public void getSearchLoaction(int type) {
        if (mSearchLoaction == null)
            return;
        mSearchLoaction.clear();
        switch (type) {
        case FileSearch.SEARCH_ALL:
            mSearchLoaction.add(StorageUtil.getExternalStorage());
            mSearchLoaction.add(StorageUtil.getInternalStorage());
            break;
        case FileSearch.SEARCH_EXTERNAL:
            mSearchLoaction.add(StorageUtil.getExternalStorage());
            break;
        case FileSearch.SEARCH_INTERNAL:
            mSearchLoaction.add(StorageUtil.getInternalStorage());
            break;
        /* SPRD: Modify for bug607772, search the files in OTG devices. @{ */
        case FileSearch.SEARCH_USB:
            final List<File> usbList = new ArrayList<File>();
            File[] usbVolume = StorageUtil.getUSBVolume();
            for (File f : usbVolume) {
                mSearchLoaction.add(f);
            }
            break;
            /* @} */
        }
    }

    public class SearchTask extends AsyncTask<String, integer, List<File>> {
        /* SPRD 440831 @{*/
        private boolean isWaitForScanFinish = false;
        private String path = "";

        public SearchTask(){
            isWaitForScanFinish = false;
            path = "";
        }

        public SearchTask(boolean isWaitForScan,String filepath){
            isWaitForScanFinish = isWaitForScan;
            path = filepath;
        }
        /* @} */
        public List<File> filesList = new ArrayList<File>();
        @Override
        protected List<File> doInBackground(String... params) {
            Log.d(TAG, "start doInBackground()");
            try {
                Thread.sleep(800);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            /* SPRD 440831 @{*/
            if (isWaitForScanFinish) {
                refreshWaitForFile(path);
            }
            /* @} */
            Uri uri = MediaStore.Files.getContentUri("external");
            String[] projection = new String[] {
                MediaStore.Files.FileColumns.DATA
            };
            String selection = "(" + MediaStore.Files.FileColumns.DATA
                    + " like '%" + mSearchKey + "%' )";
            if (mSearchLoaction.size() == 1) {
                selection += " AND " + MediaStore.Files.FileColumns.DATA
                        + " like '" + mSearchLoaction.get(0).getPath() + "%'";
            }
            selection += " COLLATE NOCASE";
            final SharedPreferences sortPreference = mContext
                    .getSharedPreferences(FileSort.FILE_SORT_KEY, 0);
            int sort = sortPreference.getInt(FileSort.FILE_SORT_KEY,
                    FileSort.SORT_BY_NAME);
            SharedPreferences defaultSettings = PreferenceManager.getDefaultSharedPreferences(mContext);
            boolean displayHide = defaultSettings.getBoolean("display_hide_file", false);
            String sortBy = FileSort.getOrderStr(sort);
            Cursor cursor = null;
            try{
                cursor = mContext.getContentResolver().query(uri, projection, selection, null, sortBy);
            }catch(Exception e){
                Log.e(TAG,"query database unkown exception");
            }
            if (cursor == null) {
                return filesList;
            }
            if (cursor.getCount() == 0) {
                cursor.close();
                return filesList;
            }
            mSearchKey = mSearchKey.toLowerCase();
            while (cursor.moveToNext()) {
                String filePath = cursor.getString(0);
                File file = new File(filePath);
                int fileType = FileType.getFileType(FileSearchResultActivity.this).getFileType(file);
                if (mSearchType != null && !mSearchType.isEmpty()) {
                    
                    //is or not drm file
                    if (FileSearchResultActivityUtils.getInstance().isDRMFile(filePath)){
                        if(file.isHidden() && !displayHide) {
                            continue;
                        }
                        FileSearchResultActivityUtils.getInstance().fileSearchDRMFile(filePath, mContext,mSearchTask);
                    }else if (mSearchType.contains(fileType)) {
                        if(file.isHidden() && !displayHide) {
                            continue;
                        }
                        if ((file.getName().toLowerCase().contains(mSearchKey))) {
                            filesList.add(new File(filePath));
                        }
                    }
                }
            }
            cursor.close();
            SharedPreferences settings = mContext.getSharedPreferences(FileSort.FILE_SORT_KEY, 0);
            int sortType = settings.getInt(FileSort.FILE_SORT_KEY, FileSort.SORT_BY_NAME);
            FileSort sorter = FileSort.getFileListSort();
            sorter.setSortType(sortType);
            sorter.sort(filesList);
            return filesList;
        }

        @Override
        protected void onPreExecute() {
            super.onPreExecute();
            mAdapter.getFileList().clear();
            Log.d(TAG, "onPreExecute");
            mWaitDialog = new ProgressDialog(mContext);
            mWaitDialog.setTitle(R.string.dialog_hint_title);
            mWaitDialog.setMessage(mContext.getResources().getString(
                    R.string.dialog_hint_msg));
            mWaitDialog.setCancelable(false);
            mWaitDialog.show();
        }

        @Override
        protected void onPostExecute(List<File> fileList) {
            if (((FileSearchResultActivity) mContext).isFinishing()) {
                return;
            }
            super.onPostExecute(fileList);
            if(mAdapter != null && mAdapter.getFileList() != null){
                mAdapter.getFileList().clear();
            }
            //SPRD: Add for bug 600925
            if (mAdapter == null) {
                return;
            }
            final List<File> result = new ArrayList<File>(fileList); 
            for (int i = 0; i < result.size(); i++) {
                mAdapter.getFileList().add(result.get(i));
            }
            mAdapter.notifyDataSetChanged();
            mListView.setEmptyView(mEmptyView);
            if (mWaitDialog != null && mWaitDialog.isShowing()) {
                mWaitDialog.dismiss();
            }
           new Thread(new Runnable() {
                @Override
                public void run() {
                            if (result instanceof ArrayList<?>) {
                                List<File> invalidFiles = new ArrayList<File>();
                                for (File f : result) {
                                    if (!f.exists()) {
                                        invalidFiles.add(f);
                                    }
                                }
                                if (!invalidFiles.isEmpty()) {
                                    for (File f : invalidFiles) {
                                        result.remove(f);
                                    }
                                }
                            }
                    mHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            if(mAdapter != null && mAdapter.getFileList() != null){
                                mAdapter.getFileList().clear();
                                for (int i = 0; i < result.size(); i++) {
                                    mAdapter.getFileList().add(result.get(i));
                                }
                                mAdapter.notifyDataSetChanged();
                            }
                            mListView.setEmptyView(mEmptyView);
                        }
                    });
                }
            }).start();
            
        }
    }


    @Override
    public void onBackPressed() {
        mListView.setEmptyView(null);
        if (mAdapter.popupFolder()) {
            finish();
        } else {
            mAdapter.notifyDataSetChanged();
        }
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy");
        if (mAdapter != null) {
            mAdapter.destroyThread();
            mAdapter = null;
        }
        if (mWaitDialog != null && mWaitDialog.isShowing()) {
            mWaitDialog.dismiss();
        }
        StorageUtil.removeStorageChangeListener(mStorageChangedListener);
        super.onDestroy();
    }
    
    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case FileUtil.REFREAH_FILELIST:
                if (mAdapter.getInSearchUi()) {
                    searchFile();
                } else {
                    File file = mAdapter.getCurrentPath();
                    mAdapter.refresh(file);
                }
                break;
            default:
                break;
            }
        };
    };

    public void searchFile() {
        Intent data = getIntent();
        Bundle bun = data.getBundleExtra(FileSearch.SEARCH_ATTACH);
        if (bun != null) {
            mSearchKey = bun.getString(FileSearch.SEARCH_KEY);
            int searchLocation = bun.getInt(FileSearch.SEARCH_LOCATION);
            getSearchLoaction(searchLocation);
            if (mSearchKey != null && !mSearchKey.isEmpty()
                    && this.mSearchLoaction != null) {
                mSearchTask = new SearchTask();
                mSearchTask.execute("");
            }
        }
    }
    /* SPRD 440831 @{ */
    public void searchFile(boolean isWaitForRefresh,String path) {
        Intent data = getIntent();
        Bundle bun = data.getBundleExtra(FileSearch.SEARCH_ATTACH);
        if (bun != null) {
            mSearchKey = bun.getString(FileSearch.SEARCH_KEY);
            int searchLocation = bun.getInt(FileSearch.SEARCH_LOCATION);
            getSearchLoaction(searchLocation);
            if (mSearchKey != null && !mSearchKey.isEmpty()
                    && this.mSearchLoaction != null) {
                mSearchTask = new SearchTask(isWaitForRefresh,path);
                mSearchTask.execute("");
            }
        }
    }
    /* @} */
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
            final int position = info.position;
            File selectedFile = (File) mAdapter.getItem(position);
            if (selectedFile == null) {
                return;
            }
            String filePath = selectedFile.getPath();
            FileListMenuClickListener l = new FileListMenuClickListener(
                    selectedFile);
            //drm file not copy menu item
            if(FileSearchResultActivityUtils.getInstance().DRMFileCopyMenu(selectedFile)){
                menu.add(0, MENU_COPY, 0, R.string.menu_copy).setOnMenuItemClickListener(l);
            }
            menu.add(0, MENU_CUT, 0, R.string.menu_cut)
                    .setOnMenuItemClickListener(l);
            menu.add(0, MENU_DELETE, 0, R.string.operate_delete)
                    .setOnMenuItemClickListener(l);
            if (selectedFile.isDirectory()) {
                menu.setHeaderTitle(R.string.folder_option);
                menu.add(0, MENU_CLEAR, 0, R.string.operate_clear)
                        .setOnMenuItemClickListener(l);
            } else {
                menu.setHeaderTitle(R.string.file_option);
                //has share menu item only sd type in drm file
                if(FileSearchResultActivityUtils.getInstance().DRMFileShareMenu(selectedFile)){
                    menu.add(0, MENU_SHARE, 0, R.string.operate_share).setOnMenuItemClickListener(l);
                }
            }
            menu.add(0, MENU_RENAME, 0, R.string.operate_rename)
                    .setOnMenuItemClickListener(l);
            menu.add(0, MENU_DETAIL, 0, R.string.operate_viewdetails)
                    .setOnMenuItemClickListener(l);
            //drm file add protect menu item
            FileSearchResultActivityUtils.getInstance().DRMFileProtectMenu(menu,selectedFile,mContext);

        }
    };

    public final class FileListMenuClickListener implements
            MenuItem.OnMenuItemClickListener {
        private File mClickedFile;

        public FileListMenuClickListener(File selectedFile) {
            mClickedFile = selectedFile;
        }

        @Override
        public boolean onMenuItemClick(MenuItem item) {
            if (mClickedFile == null) {
                return false;
            }

            int itemId = item.getItemId();
            switch (itemId) {
            case MENU_COPY:
            case MENU_CUT: {
                mCopyFileListener.setCut(itemId == MENU_CUT);
                mCopyFileListener.addOpFile(mClickedFile);
                //drm file not copy
                if(FileSearchResultActivityUtils.getInstance().DRMFileCopyMenuClick(mContext,itemId,MENU_COPY)){
                    return true;
                }
                /* SPRD 454659  @{ */
                //if (StorageUtil.getExternalStorageState()) {
                if (ActivityUtils.getAvailableStatus() > 1) {
                /* @} */
                    ActivityUtils.showDestSelectDialog(mContext, mCopyFileListener);
                } else {
                    mCopyFileListener.onClick(null, 0);
                }
                return true;
            }
            case MENU_DELETE:
            case MENU_CLEAR: {
                final boolean isClear = itemId == MENU_CLEAR;
                new AlertDialog.Builder(mContext)
                .setTitle(isClear ? R.string.operate_clear : R.string.operate_delete)
                .setMessage(isClear ? R.string.msg_issure_clear : R.string.confirm_delete)
                .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        new FileDeleteTask.Build(FileSearchResultActivity.this, isClear, false)
                        .addOperateFile(mClickedFile)
                        .setOnFinishCallBack(new OnFinishCallBack() {

                            // SPRD: Modify for bug465956.
                            @Override
                            public void onFinish(boolean cancelTask, String path) {
                                if (mAdapter.getInSearchUi()) {
                                    searchFile();
                                } else {
                                    File file = mAdapter.getCurrentPath();
                                    mAdapter.refresh(file);
                                }
                            }
                            
                        }).creatTask().start();
                    }
                    
                }).setNegativeButton(android.R.string.cancel, null)
                .show();
                break;
            }
            case MENU_SHARE:
                Intent intent = new Intent(Intent.ACTION_SEND);
                String type = FileType.getFileType(
                        FileSearchResultActivity.this).getShareTypeByFile(
                        mClickedFile);
                //get drm file mimeType
                String mimeType =  FileSearchResultActivityUtils.getInstance().DRMFileShareClick(mClickedFile.getPath());
                if(mimeType != null){
                    type = mimeType;
                }
                Uri uri = Uri.parse("file://" + mClickedFile.getPath());
                intent.setType(type);
                intent.putExtra(Intent.EXTRA_STREAM, uri);
                startActivity(Intent.createChooser(intent, mContext
                        .getResources().getString(R.string.operate_share)));
                return true;
            case MENU_RENAME:
                FileUtil.rename(mClickedFile, mAdapter,
                        FileSearchResultActivity.this);
                return true;
            case MENU_DETAIL:
                FileUtil.viewDetails(mClickedFile,
                        FileSearchResultActivity.this);
                return true;
            default:
                return false;
            }
            return true;
        }

    }
    /* SPRD 440831 @{ */
    private void refreshWaitForFile(String filepath) {
        mCheckCount = 0;
        while (mCheckCount < MAX_CHECK_COUNT) {
            Cursor cursor = null;
            try {
                cursor = getContentResolver().query(
                        MediaStore.Files.getContentUri("external"),
                        new String[] { MediaStore.Files.FileColumns.DATA },
                        MediaStore.Files.FileColumns.DATA + "=?",
                        new String[] { filepath },
                        null);
                mCheckCount++;
                if (cursor != null && cursor.getCount() == 1) {
                    Log.i(TAG, "refreshWaitForFile mCheckCount "+mCheckCount);
                    break;
                }
            } catch (Exception ex) {
                mCheckCount++;
                Log.i(TAG, "refreshWaitForFile Exception "+ex.toString());
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }
        }
    }
    /* @} */
}
