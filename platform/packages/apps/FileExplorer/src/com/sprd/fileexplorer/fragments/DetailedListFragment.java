package com.sprd.fileexplorer.fragments;

import java.io.File;
import java.util.List;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ContentUris;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.provider.MediaStore;
import android.util.Log;
import android.util.SparseArray;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnCreateContextMenuListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.activities.FileExploreActivity;
import com.sprd.fileexplorer.activities.FilePickerActivity;
import com.sprd.fileexplorer.activities.FileSearchActivity;
import com.sprd.fileexplorer.activities.MultiSelectActivity;
import com.sprd.fileexplorer.adapters.FileAdapter.BackStackListener;
import com.sprd.fileexplorer.adapters.FileAdapter.EmptyViewListener;
import com.sprd.fileexplorer.adapters.FileAdapter.LoadingFileListener;
import com.sprd.fileexplorer.adapters.FileAdapter.SortListener;
import com.sprd.fileexplorer.adapters.FileListAdapter;
import com.sprd.fileexplorer.adapters.FileListAdapter.PathChangeListener;
import com.sprd.fileexplorer.drmplugin.DetailedListFragmentUtils;
import com.sprd.fileexplorer.util.ActivityUtils;
import com.sprd.fileexplorer.util.ActivityUtils.CopyFileListener;
import com.sprd.fileexplorer.util.FileDeleteTask;
import com.sprd.fileexplorer.util.FileSort;
import com.sprd.fileexplorer.util.FileType;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexplorer.util.FileUtilTask.OnFinishCallBack;
import com.sprd.fileexplorer.util.StorageUtil;

import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;

/**
 * The fragment explore files which is  based on File not database.
 */
public class DetailedListFragment extends BaseFragment implements
        OnItemClickListener {
    public static final String TAG = "DetailedListFragment";

    // SPRD: Add for bug510243.
    private static final int FRAGMENT_STORAGE_PERMISSION_REQUEST_CODE = 4;
    public static final int MENU_COPY = 1;
    public static final int MENU_CUT = 2;
    public static final int MENU_DELETE = 3;
    public static final int MENU_CLEAR = 4;
    public static final int MENU_SHARE = 5;
    public static final int MENU_RENAME = 6;
    public static final int MENU_DETAIL = 7;
    public static final int MENU_SETAS = 8;
    
    public static final int FRAGMENT_TYPE_INTERNAL = 0;
    public static final int FRAGMENT_TYPE_EXTERNAL = 1;
    public static final int FRAGMENT_TYPE_USB = 2;

    private static final File USB_LIST_DIRECTORY = new File("USB List");

    private FileExploreActivity mContext;
    private ListView mList;
    private View mListViewLayout;
    private View mEmptyView;
    private View mStandByView;
    private FileListAdapter mAdapter;
    private File mTopPath;
    private TextView mPathBar;
    private int mSortBy;
    private int mIconId;
    private ActionBar mActionBar;
    private CopyFileListener mCopyFileListener;

    private Handler mMainThreadHandler;
    //SPRD 441574
    private Dialog mDialog;
    // SPRD: Add for bug616271.
    private long mLastClickTime = 0;

    private Runnable mShowStandByRunnable = new Runnable() {
        @Override
        public void run() {
            mListViewLayout.setVisibility(View.GONE);
            mStandByView.setVisibility(View.VISIBLE);
            //setHasOptionsMenu(false);//SPRD:Bug 276734
        }
    };
    
    /**
     * @deprecated
     */
    public DetailedListFragment() {
        super();
    }
    
    private DetailedListFragment(FileExploreActivity context, int type) {
        mContext = context;
        switch(type) {
            case FRAGMENT_TYPE_EXTERNAL: {
                mTopPath = StorageUtil.getExternalStorage();
                mIconId = R.drawable.main_sd;
                break;
            }
            case FRAGMENT_TYPE_USB: {
                mTopPath = USB_LIST_DIRECTORY;
                mIconId = R.drawable.main_sd;
                break;
            }
            default: 
                mTopPath = StorageUtil.getInternalStorage();
                mIconId = R.drawable.main_device;
        }
    }

    private SortListener mSortListener = new SortListener() {

        @Override
        public void onSortStarting() {
            if (mListViewLayout != null && mStandByView != null) {
                mListViewLayout.setVisibility(View.GONE);
                mStandByView.setVisibility(View.VISIBLE);
            }
            setHasOptionsMenu(false);
        }

        @Override
        public void onSortFinished() {
            if (mStandByView != null) {
                mStandByView.setVisibility(View.GONE);
            }
            setHasOptionsMenu(true);
        }
    };

    private LoadingFileListener mLoadingFileListener = new LoadingFileListener() {
 
        @Override
        public void onLoadFileStart() {
            if (mMainThreadHandler != null) {
            	setHasOptionsMenu(false);//SPRD:Bug 276734
                mMainThreadHandler.postDelayed(mShowStandByRunnable, 800);
            }
        }

        @Override
        public void onLoadFileFinished() {
            if (mMainThreadHandler != null) {
                mMainThreadHandler.removeCallbacks(mShowStandByRunnable);
            }
            if (mAdapter == null || mStandByView == null
                    || mListViewLayout == null) {
                return;
            }
            mStandByView.setVisibility(View.GONE);
            mListViewLayout.setVisibility(View.VISIBLE);
            setHasOptionsMenu(true);
        }
    };

    private EmptyViewListener mEmptyViewListener = new EmptyViewListener() {
        @Override
        public void onEmptyStateChanged(boolean isEmpty) {
            if ((mAdapter != null) && !mAdapter.mIsLoading) {
                setEmptyView(isEmpty);
            }
        }
    };

    private BackStackListener mBackStackListener = new BackStackListener() {

        @Override
        public void onBackPrevious() {
            Log.d(TAG, "onBackPrevious");
            if (mAdapter == null) {
                return;
            }
            Log.d(TAG, "onBackPrevious:setSelection");
            mList.setVisibility(View.VISIBLE);
            mList.setSelection(mAdapter.restorePreviousPosition(mAdapter.getCurrentPath().getAbsolutePath()));
        }
    };
    
    @Override
    public void onCreate(Bundle state) {
        super.onCreate(state);
        Log.d(TAG, "DetailedListFragment:onCreate");
        setHasOptionsMenu(true);
        mCopyFileListener = new CopyFileListener(getActivity());
        FileUtil.addUSBRefreshListener(new FileUtil.USBRefreshListener() {
            @Override
            public void onChanged() {
                Log.d(TAG,"onChanged()");
                if (mAdapter != null) {
                    mAdapter.refresh();
                }
            }
        });
    }

    @Override
    public void onDestroyView() {
        Log.d(TAG, "DetailedListFragment:onDestroyView");
        super.onDestroyView();
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "DetailedListFragment:onDestroy");
        mAdapter.destroyThread();
        mAdapter = null;
        /* SPRD 441574 @{*/
        if(mDialog != null && mDialog.isShowing()){
            mDialog.dismiss();
        }
        /* @} */
        super.onDestroy();
    }

    /* SPRD: bug522179 need check runtime permission @{ */
    public void onStart(){
        super.onStart();
        if (mContext != null && mContext.checkSelfPermission(WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[] { WRITE_EXTERNAL_STORAGE }, FRAGMENT_STORAGE_PERMISSION_REQUEST_CODE);
            return;
        }
        // SPRD:bug545308 refresh UI
        if (mAdapter != null) {
            mAdapter.notifyDataSetChanged();
        }
    }
    /* @} */

    public void onResume(){
        super.onResume();
        Log.d(TAG, "onResume(): mAdapter = " + mAdapter);
        if(mAdapter != null){
            mAdapter.ensureRefresh();
            mAdapter.notifyDataSetChanged();
        }
    }
    
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_detailed_explore,
                container, false);

        if (view == null) {
            return super.onCreateView(inflater, container, savedInstanceState);
        } else {
            boolean needRefresh = true;
            mPathBar = (TextView) view.findViewById(R.id.path_bar);
            mList = (ListView) view.findViewById(R.id.detailed_file_list);
            mListViewLayout = view.findViewById(R.id.file_explore_layout);
            mEmptyView = view.findViewById(R.id.file_explore_empty_view);
            mStandByView = view.findViewById(R.id.file_explore_sorting_standby_layout);
            if (mAdapter == null) {
                mAdapter = new FileListAdapter(mContext, mList);
                mAdapter.attachFragment(this).initWithPath(mTopPath);
            } else {
                mAdapter.setContext(getActivity());
                needRefresh = false;
            }
            mPathBar.setText(mAdapter.getCurrentString());
            mAdapter.setFileSortListener(mSortListener);
            mAdapter.setEmptyViewListener(mEmptyViewListener);
            mAdapter.setLoadingFileListener(mLoadingFileListener);
            mAdapter.setBackStackListener(mBackStackListener);
            mAdapter.setPathChangeListener(new PathChangeListener() {
                @Override
                public void onPathChanged(String path) {
                    mPathBar.setText(path);
                }
            });
            mList.setAdapter(mAdapter);
            if (needRefresh) {
                mAdapter.refresh();
            } else {
                mAdapter.ensureRefresh();
            }
            mList.setOnItemClickListener((OnItemClickListener) this);
            mList.setOnCreateContextMenuListener(mFileListOnCreateContextMenuListener);
            mMainThreadHandler = new Handler(getActivity().getMainLooper());
            return view;
        }
    }

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

        mActionBar = mContext.getActionBar();
        mActionBar.setDisplayHomeAsUpEnabled(false);
        if (!mMainThreadHandler.hasCallbacks(mShowStandByRunnable)) {
            // SPRD: Add for bug616271.
            mLastClickTime = currentTime;
            mAdapter.execute(position, mList.getFirstVisiblePosition());
        }
    }

    @Override
    public void onAttach(Activity activity) {
        mContext = (FileExploreActivity) activity;
        super.onAttach(activity);
    }

    @Override
    public boolean onBackPressed() {
        if(mAdapter == null){
            Log.i(TAG,"mAdapter = null");
            return false;
        }
        if (mAdapter.getCurrentPath().getParent() != null
                && mAdapter.getCurrentPath().getParent().toString().equals(mAdapter.getTopPath().toString())){
            mActionBar = mContext.getActionBar();
            mActionBar.setDisplayHomeAsUpEnabled(false);
        }
        mAdapter.mCancelLoading = true;
        if (mAdapter.popupFolder()) {
            Log.i(TAG, "top path, exit ");
            mEnableExit = true;
            return true;
        } else {
            mAdapter.setNeedBack();
            mList.setVisibility(View.INVISIBLE);
            Log.i(TAG, "not top path, go tp parent folder");
            return false;
        }
    }

    public void onPrepareOptionsMenu(Menu menu) {
        Log.d(TAG, "onPrepareOptionsMenu");
        /* SPRD: Bug 432426 Menu shows wrong after language changed @{ */
//        mContext.getMenuInflater().inflate(R.menu.detailed_list_menu, menu);
        /* @} */
        MenuItem multiSelectItem = menu.findItem(R.id.file_menu_select_more);
        MenuItem sortItem = menu.findItem(R.id.file_menu_sort_type);
        MenuItem returnRootPath = menu.findItem(R.id.file_menu_return_root_path);
        if (mAdapter != null && mAdapter.getCount() > 0) {
            if (multiSelectItem != null) {
                multiSelectItem.setEnabled(true);
            }
            if (sortItem != null) {
                sortItem.setEnabled(true);
            }
        } else {
            if (multiSelectItem != null) {
                multiSelectItem.setEnabled(false);
            }
            if (sortItem != null) {
                sortItem.setEnabled(false);
            }
        }
        MenuItem mkdirItem = menu.findItem(R.id.file_menu_mkdir);
        MenuItem newfileItem = menu.findItem(R.id.file_menu_newfile);
        if (mAdapter.getCurrentPath().equals(USB_LIST_DIRECTORY)) {
            if (multiSelectItem != null) {
                multiSelectItem.setEnabled(false);
            }
            if (sortItem != null) {
                sortItem.setEnabled(false);
            }
            if (mkdirItem != null) {
                mkdirItem.setEnabled(false);
            }
            if (newfileItem != null) {
                newfileItem.setEnabled(false);
            }
        }
        if (mAdapter.getTopPath() != null && mAdapter.getCurrentPath() != null
                && mAdapter.getTopPath().getAbsolutePath().equals(mAdapter.getCurrentPath().getAbsolutePath())) {
            if (returnRootPath != null) {
                returnRootPath.setVisible(false);
            }
        } else {
            if (returnRootPath != null) {
                returnRootPath.setVisible(true);
            }
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        Log.d(TAG, "onOptionsItemSelected");
        if (mAdapter == null) {
            Log.d(TAG, "mAdapter == null, onOptionsItemSelected");
            return true;
        }
        switch (item.getItemId()) {
        case R.id.searchfile:
            mContext.startActivity(new Intent().setClass(
                    mContext.getApplicationContext(), FileSearchActivity.class));
            return true;
        case R.id.file_menu_mkdir:
            FileUtil.mkdir(mAdapter.getCurrentPath(), mAdapter, mContext);
            return true;
        case R.id.file_menu_newfile:
            FileUtil.newfile(mAdapter.getCurrentPath(), mAdapter, mContext);
            return true;
        case R.id.file_menu_select_more:
            Intent intent = new Intent(mContext, MultiSelectActivity.class);
            intent.putExtra("path", mAdapter.getCurrentPath().getPath());
            intent.putExtra("position", mList.getFirstVisiblePosition());
            startActivity(intent);
            return true;
        case R.id.file_menu_sort_type:
            AlertDialog.Builder sortTypeDialog = new AlertDialog.Builder(
                    mContext);
            sortTypeDialog.setTitle(R.string.menu_sort_type);
            final int sortType;
            final SharedPreferences settings = mContext.getSharedPreferences(FileSort.FILE_SORT_KEY, 0);
            sortType = settings.getInt(FileSort.FILE_SORT_KEY, FileSort.SORT_BY_NAME);
            int selectItem = FileSort.getSelectItemByType(sortType);
            mSortBy = selectItem;
            sortTypeDialog.setSingleChoiceItems(R.array.sort_type, selectItem,
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog,
                                int whichButton) {
                            mSortBy = whichButton;
                        }
                    });
            sortTypeDialog.setNegativeButton(R.string.sort_by_asc,
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog,
                                int whichButton) {
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
                            /* SPRD 441574 @{ */
                            if(mAdapter == null){
                                return;
                            }
                            /* @} */
                            settings.edit().putInt(FileSort.FILE_SORT_KEY, mSortBy).commit();
                            mAdapter.setSortType(mSortBy);
                            mAdapter.startSort();
                            //mAdapter.notifyDataSetChanged();//SPRD:Bug 276734

                        }
                    });
            sortTypeDialog.setPositiveButton(R.string.sort_by_desc,
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog,
                                int whichButton) {
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
                            /* SPRD 441574 @{ */
                            if(mAdapter == null){
                                return;
                            }
                            /* @} */
                            settings.edit()
                                    .putInt(FileSort.FILE_SORT_KEY, mSortBy)
                                    .commit();
                            mAdapter.setSortType(mSortBy);
                            mAdapter.startSort();
                            //mAdapter.notifyDataSetChanged();//SPRD:Bug 276734

                        }
                    });
            //SPRD 441574
            mDialog = sortTypeDialog.show();
            return true;
        case R.id.file_menu_storage_status: 
            startActivity(new Intent("android.settings.INTERNAL_STORAGE_SETTINGS"));
            return true;
        case R.id.file_menu_return_root_path:
            mAdapter.setCurrentPath(mAdapter.getTopPath());
            return true;
        default:
            return super.onOptionsItemSelected(item);
        }
    }

    private OnCreateContextMenuListener mFileListOnCreateContextMenuListener = new OnCreateContextMenuListener() {
        
        private FileListMenuClickListener fileListMenuClickListener = new FileListMenuClickListener();
        
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
            if (mAdapter == null) {
                return;
            }
            File selectedFile = (File) mAdapter.getItem(position);
            if (selectedFile == null || mAdapter.getCurrentPath().equals(USB_LIST_DIRECTORY)) {
                return;
            }
            String filePath = selectedFile.getPath();
            fileListMenuClickListener.mClickedFile = selectedFile;
            if (selectedFile.canRead()) {
                //drm file not copy item
                if(DetailedListFragmentUtils.getInstance().DRMFileCopyMenu(selectedFile)){
                    menu.add(0, MENU_COPY, 0, R.string.menu_copy)
                    .setOnMenuItemClickListener(fileListMenuClickListener);
                }
                menu.add(0, MENU_CUT, 0, R.string.menu_cut)
                        .setOnMenuItemClickListener(fileListMenuClickListener);
                menu.add(0, MENU_DELETE, 0, R.string.operate_delete)
                        .setOnMenuItemClickListener(fileListMenuClickListener);
                if (selectedFile.isDirectory()) {
                    menu.setHeaderTitle(R.string.folder_option);
                    menu.add(0, MENU_CLEAR, 0, R.string.operate_clear)
                            .setOnMenuItemClickListener(fileListMenuClickListener);
                } else {
                    menu.setHeaderTitle(R.string.file_option);
                    // has share menu item only sd type in drm file
                    if(DetailedListFragmentUtils.getInstance().DRMFileShareMenu(selectedFile,mContext)){
                        menu.add(0, MENU_SHARE, 0, R.string.operate_share)
                        .setOnMenuItemClickListener(fileListMenuClickListener);
                    }
                }
                menu.add(0, MENU_RENAME, 0, R.string.operate_rename)
                        .setOnMenuItemClickListener(fileListMenuClickListener);
                int mFileType = FileType.getFileType(mContext).getFileType2(selectedFile);
                if (mFileType == FileType.FILE_TYPE_IMAGE) {
                    //drm picture file not setas menu item
                    if(DetailedListFragmentUtils.getInstance().DRMFileSetAsMenu(selectedFile)){
                        menu.add(0, MENU_SETAS, 0, R.string.set_as)
                        .setOnMenuItemClickListener(fileListMenuClickListener);
                    }
                }
                menu.add(0, MENU_DETAIL, 0, R.string.operate_viewdetails)
                        .setOnMenuItemClickListener(fileListMenuClickListener);
                //drm file add protect menu item
                DetailedListFragmentUtils.getInstance().DRMFileProtectMenu(menu,selectedFile,mContext);
            } else {
                if (selectedFile.isDirectory()) {
                    menu.setHeaderTitle(R.string.folder_option);
                } else {
                    menu.setHeaderTitle(R.string.file_option);
                }
                menu.add(0, MENU_DETAIL, 0, R.string.operate_viewdetails)
                        .setOnMenuItemClickListener(fileListMenuClickListener);
            }
        }
    };

    private class FileListMenuClickListener implements MenuItem.OnMenuItemClickListener {

        File mClickedFile;

        @Override
        public boolean onMenuItemClick(MenuItem item) {
            if (mClickedFile == null) {
                return false;
            }
            /* SPRD: Add for bug510243 & bug516099. @{ */
            if (mContext.checkSelfPermission(WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                Log.d(TAG, "start request permissions!");
                requestPermissions(new String[] { WRITE_EXTERNAL_STORAGE }, FRAGMENT_STORAGE_PERMISSION_REQUEST_CODE);
                return false;
            }
            /* @} */
            int itemId = item.getItemId();
            switch (itemId) {
            case MENU_COPY:
            case MENU_CUT:{
                mCopyFileListener.setCut(itemId == MENU_CUT);
                mCopyFileListener.addOpFile(mClickedFile);
                /* SPRD 454659  @{ */
                //if (StorageUtil.getExternalStorageState()) {
                if(ActivityUtils.getAvailableStatus() > 1){
                /* @} */
                    ActivityUtils.showDestSelectDialog(mContext, mCopyFileListener);
                }else{
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
                .setPositiveButton(android.R.string.ok,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                /* SPRD: bug522179 need check runtime permission @{ */
                                if (mContext != null && mContext.checkSelfPermission(WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                                    Toast.makeText(mContext, R.string.error_permissions, Toast.LENGTH_SHORT).show();
                                    return;
                                }
                                /* @} */
                                new FileDeleteTask.Build(mContext, isClear, false)
                                .addOperateFile(mClickedFile)
                                .setOnFinishCallBack(new OnFinishCallBack() {

                                    // SPRD: Modify for bug465956.
                                    @Override
                                    public void onFinish(boolean cancelTask, String path) {
                                        /* SPRD 425657 @{*/
                                        //if(!mClickedFile.exists()){
                                        if(mClickedFile != null && mAdapter != null && !mClickedFile.exists() ){
                                        /* @} */
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
            }
            case MENU_SHARE:
                Intent intent=new Intent(Intent.ACTION_SEND);
                String type = FileType.getFileType(mContext).getShareTypeByFile(mClickedFile);

                //get drm file mimeType
                String mimeType = DetailedListFragmentUtils.getInstance().DRMFileShareClick(mClickedFile);

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
                /* SPRD: Add a judgement for bug 461497 @{ */
                if(mAdapter != null){
                    intent.setType(type);
                    intent.putExtra(Intent.EXTRA_STREAM, uri);
                    startActivity(Intent.createChooser(intent, mContext.getResources().getString(R.string.operate_share)));
                }else {
                    Toast.makeText(mContext, R.string.sdcard_had_unmounted, Toast.LENGTH_LONG).show();
                }
                /* @} */
                return true;
            case MENU_RENAME:
                FileUtil.rename(mClickedFile, mAdapter,mContext);
                return true;
            case MENU_DETAIL:
                FileUtil.viewDetails(mClickedFile,mContext);
                return true;
            case MENU_SETAS:
                Intent intent2 = new Intent(Intent.ACTION_ATTACH_DATA)
                        .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                String type2 = FileType.getFileType(mContext).getTypeByFile(
                        mClickedFile);
                Uri uri2 = MediaStore.Files.getContentUri("external");
                Cursor cursor = mContext.getContentResolver().query(uri2,
                        new String[] {
                            MediaStore.Files.FileColumns._ID
                        },
                        MediaStore.Files.FileColumns.DATA + "=?",
                        new String[] {
                            mClickedFile.getPath()
                        }, null);
                if (cursor.moveToFirst()){
                    long id = cursor.getLong(0);
                    cursor.close();
                    intent2.setDataAndType(ContentUris.withAppendedId(uri2, id),
                            type2);
                    mContext.startActivity(Intent.createChooser(intent2, mContext
                            .getResources().getString(R.string.set_as)));
                }else{
                    Toast.makeText(mContext, mContext.getResources().getString(R.string.failed_query), Toast.LENGTH_LONG).show();
                   cursor.close();
                }
                return true;
            default:
                return false;
            }
        }

    }

    private void setEmptyView(boolean isEmpty) {
        if (isEmpty) {
            mListViewLayout.setVisibility(View.GONE);
            mEmptyView.setVisibility(View.VISIBLE);
        } else {
            mListViewLayout.setVisibility(View.VISIBLE);
            mEmptyView.setVisibility(View.GONE);
        }
    }

    public FileListAdapter getAdapter(){
        return mAdapter;
    }
    public void startSort() {
        if (mAdapter == null) {
            return;
        }
        mAdapter.startSort();
        mAdapter.notifyDataSetChanged();
    }

    public void setTopPath(File f) {
        mTopPath = f;
    }

    @Override
    public int getIcon() {
        return mIconId;
    }
    
    private static SparseArray<DetailedListFragment> sAllFragment = new SparseArray<DetailedListFragment>();
    
    public static DetailedListFragment init(FileExploreActivity activity, int type) {
        DetailedListFragment value = sAllFragment.get(type);
        // SPRD: Modify for bug465956.
        if(value == null || value.getAdapter() == null || activity != value.getActivity()) {
            value = new DetailedListFragment(activity, type);
            sAllFragment.put(type, value);
        }
        return value;
    }

    public static DetailedListFragment getInstance(int type) {
        return sAllFragment.get(type);
    }
    
    public static void clearFragment() {
        sAllFragment.clear();
    }
}
