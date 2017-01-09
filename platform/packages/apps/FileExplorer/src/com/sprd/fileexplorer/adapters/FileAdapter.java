
package com.sprd.fileexplorer.adapters;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Slog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.AbsListView.OnScrollListener;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.loadimage.ImageCache;
import com.sprd.fileexplorer.loadimage.ImageLoadThreadManager;
import com.sprd.fileexplorer.util.FileSort;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexplorer.util.FitSizeExpression;

public abstract class FileAdapter extends BaseAdapter implements
        View.OnClickListener {
    
    public static final String TAG = "FileAdapter";
    private static final int MAX_SIZE_USE_THREAD_SORT = 100;
    
    protected HandlerThread mHandlerThread = new HandlerThread("FileAdapterWorkingDeamon");
    protected Handler mHandler;
    protected boolean mScrolling = false;

    public interface SortListener {
        public void onSortStarting();

        public void onSortFinished();
    }

    public interface EmptyViewListener {
        public void onEmptyStateChanged(boolean isEmpty);
    }

    public interface LoadingFileListener {
        public void onLoadFileStart();

        public void onLoadFileFinished();
    }

    public interface BackStackListener {
        public void onBackPrevious();
    }
    
    public interface CheckListStateListener {
        public void onChangCursor();
    }
    
    private SortListener mSortListener = null;
    private EmptyViewListener mEmptyViewListener = null;
    private LoadingFileListener mLoadingFileListener = null;
    private BackStackListener mBackStackListener = null;
    private boolean mNeedBack = false;
    public volatile boolean mIsSorting = false;
    public boolean mIsLoading = false;
    public boolean mCancelLoading = false;
    protected List<File> mFileList;
    protected HashMap<String, Boolean> mIsDirectoryList;
    volatile protected String mCurrentString;
    protected String mTopString;
    protected Context mContext;
    boolean mLastEmptyViewState;
    public Handler mMainThreadHandler;
    private HashMap<String, Integer> mBackStackMap = new HashMap<String, Integer>();
    protected FitSizeExpression mFitSize = new FitSizeExpression(0);

    protected Runnable mNotifyRunnable = new Runnable() {
        @Override
        public void run() {
            notifyDataSetChanged();
        }
    };

    public void destroyThread() {
        mHandlerThread.quit();
    }

    private Set<String> mCheckedItems = new HashSet<String>();

    class SortRunnable implements Runnable {
        SortListener mListener;

        public SortRunnable(SortListener l) {
            mListener = l;
        }

        @Override
        public void run() {
            mMainThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mListener != null) {
                        mListener.onSortStarting();
                    }
                }
            });
            mIsSorting = true;
            final List<File> workingList = new ArrayList<File>();
            synchronized (mFileList) {
                for (File f : mFileList) {
                    workingList.add(f);
                }
            }
            FileSort.getFileListSort().sort(workingList);
            mIsSorting = false;
            mMainThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    updateFileList(workingList, UPDATE_FILES_CLONE);
                }
            });
        }
    }

    public FileAdapter(Context context) {
        super();
        mHandlerThread.start();
        mHandler = new Handler(mHandlerThread.getLooper());
        mFileList = new ArrayList<File>();
        mIsDirectoryList = new HashMap<String, Boolean>();
        mContext = context;
        mMainThreadHandler = new Handler(context.getApplicationContext().getMainLooper());
    }

    public void setContext(Context context) {
        mContext = context;
    }

    public void setEmptyViewListener(EmptyViewListener l) {
        mEmptyViewListener = l;
    }

    public void setLoadingFileListener(LoadingFileListener l) {
        mLoadingFileListener = l;
    }
    
    public void setBackStackListener(BackStackListener l) {
        mBackStackListener = l;
    }
    
    public static final int UPDATE_FILES_ADD = 1;
    public static final int UPDATE_FILES_REMOVE = 2;
    public static final int UPDATE_FILES_CLONE = 3;

    protected void updateFileList(List<File> list, int what) {
        Log.d(TAG, "updateFileList");
        if (list == null || list.isEmpty()) {
            Log.e(TAG, "Invalid list: " + list + ", stop");
            synchronized (mFileList) {
                mFileList.clear();
            }
            notifyDataSetChanged();
            return;
        }
        switch (what) {
            case UPDATE_FILES_ADD:
                synchronized (mFileList) {
                    for (File f : list) {
                        if (!mFileList.contains(f)) {
                            mFileList.add(f);
                        }
                    }
                }

                break;
            case UPDATE_FILES_REMOVE:
                synchronized (mFileList) {
                    for (File f : list) {
                        mFileList.remove(f);
                    }
                }
                break;
            case UPDATE_FILES_CLONE:
                synchronized (mFileList) {
                    mFileList.clear();
                    for (File f : list) {
                        mFileList.add(f);
                    }
                }
            default:
                break;
        }
        notifyDataSetChanged();
        notifySortingFinished();
        Log.d(TAG, "updateFileList, mNeedBack = " + mNeedBack);
        if (mNeedBack) {
            mMainThreadHandler.post(new Runnable() {

                @Override
                public void run() {
                    if (mBackStackListener != null) {
                        mBackStackListener.onBackPrevious();
                    }
                }
            });
            mNeedBack = false;
        }
    }

    protected void notifyLoadingStart() {
        mMainThreadHandler.post(mLoadStartRunnable);
    }

    private Runnable mLoadStartRunnable = new Runnable() {
        @Override
        public void run() {
            if (mLoadingFileListener != null) {
                mLoadingFileListener.onLoadFileStart();
            }
        }
    };

    private Runnable mLoadFinishRunnable = new Runnable() {
        @Override
        public void run() {
            if (mLoadingFileListener != null) {
                mLoadingFileListener.onLoadFileFinished();

            }

        }
    };

    protected void notifyLoadingFinish() {
        if (!mCancelLoading) {
            mMainThreadHandler.post(mLoadFinishRunnable);
        }
    }

    private Runnable mSortStartRunnable = new Runnable() {

        @Override
        public void run() {
            //notifyDataSetChanged();//SPRD:Bug 276734
            if (mSortListener != null) {
                mSortListener.onSortStarting();
            }
        }
    };

    protected void notifySortingStart() {
        mMainThreadHandler.post(mSortStartRunnable);
    }

    private Runnable mSortingEndRunnable = new Runnable() {

        @Override
        public void run() {
            //notifyDataSetChanged();//SPRD:Bug 276734
            if (mSortListener != null) {
                mSortListener.onSortFinished();
            }
        }
    };

    protected void notifySortingFinished() {
        mMainThreadHandler.post(mSortingEndRunnable);
    }
    
    public static class ViewHolder {
        public ImageView fileIcon;
        public TextView fileName;
        public TextView fileMessage;
        public CheckBox selectCb;
    }

    @Override
    public int getCount() {
        return mFileList.size();
    }

    @Override
    public Object getItem(int position) {
        if (position < mFileList.size()) {
            return mFileList.get(position);
        }else {
            return null;
        }
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder vHolder = null;
        if (convertView == null) {
            vHolder = new ViewHolder();
            convertView = LayoutInflater.from(parent.getContext()).inflate(
                    R.layout.detailed_filelist_item, null);
            vHolder.fileIcon = (ImageView) convertView
                    .findViewById(R.id.file_icon);
            vHolder.fileIcon.setOnClickListener(this);
            vHolder.fileName = (TextView) convertView
                    .findViewById(R.id.file_item_list_name);
            vHolder.fileMessage = (TextView) convertView
                    .findViewById(R.id.file_item_list_msg);
            vHolder.selectCb = (CheckBox) convertView
                    .findViewById(R.id.select_checkbox);
            convertView.setTag(R.id.action_paste_mode_cancel, vHolder);
        } else {
            vHolder = (ViewHolder) convertView
                    .getTag(R.id.action_paste_mode_cancel);
            vHolder.fileName.requestLayout();
        }
        if(mFileList.isEmpty()){
            return convertView;
        }
        File file = null;
        // SPRD 450763
        //synchronized (mFileList) {
            /* SPRD 430982 @{ */
            //file = mFileList.get(position);
        file = mFileList.get((position > mFileList.size() - 1) ? mFileList.size() - 1 : position);
            /* @} */
        // SPRD 450763
        //}
        /* SPRD: Add for bug607469. @{ */
        if (file == null) {
            return convertView;
        }
        /* @} */
        Boolean isDirectory = mIsDirectoryList.get(file.getAbsolutePath());
        if(isDirectory == null){
            isDirectory = file.isDirectory();
        }
        vHolder.fileName.setText(file.getName());
        Resources res = parent.getResources();
        String timeStr = FileUtil.SIMPLE_DATE_FOTMAT.format(file.lastModified());
        if (isDirectory) {
            vHolder.fileMessage.setText(res.getString(
                    R.string.file_list_flodermsg, timeStr));
        } else {
            mFitSize.changeSize(file.length());
            vHolder.fileMessage.setText(res.getString(
                    R.string.file_list_filemsg, timeStr, mFitSize.getExpression()));
        }
        convertView.setTag(R.id.viewtag_filepath, file.getPath());
        return convertView;
    }

    public static Drawable getApkIcon(Context context, String apkPath) {
        PackageManager pm = context.getPackageManager();
        PackageInfo info = pm.getPackageArchiveInfo(apkPath, PackageManager.GET_ACTIVITIES);
        if (info != null) {
            ApplicationInfo appInfo = info.applicationInfo;
            appInfo.sourceDir = apkPath;
            appInfo.publicSourceDir = apkPath;
            return appInfo.loadIcon(pm);
        }
        return null;
    }

    public boolean isEmptyList() {
        return mFileList.isEmpty();
    }

    public String getCurrentString() {
        return mCurrentString;
    }

    public String getTopString() {
        return mTopString;
    }

    public void setCurrentString(String current) {
        // XXX: Allow null here. Won't throw exception.
        if (current == null) {
            return;
        }
        mCurrentString = current;
    }

    public void setTopString(String top) {
        if (top == null) {
            return;
        }
        mTopString = top;
    }

    public boolean isChecked(int position) {
        File file = (File) getItem(position);
        if (file != null) {
            return mCheckedItems.contains(file.getAbsolutePath());
        } else {
            return false;
        }
    }

    public void setChecked(int position, boolean checked) {
        File file = (File) getItem(position);
        if (file != null) {
            if (checked) {
                mCheckedItems.add(file.getAbsolutePath());
            } else {
                mCheckedItems.remove(file.getAbsolutePath());
            }
        }
    }

    public boolean isEmptyFolder() {
        return mFileList.isEmpty();
    }

    public void checkAll(boolean checked) {
        if (checked) {
            int count = getCount();
            for (int i = 0; i < count; ++i) {
                File file = (File) getItem(i);
                if (file != null) {
                    mCheckedItems.add(file.getAbsolutePath());
                }
            }
        } else {
            mCheckedItems.clear();
        }
    }

    public boolean isAllChecked() {
        Slog.d("MultiSelectActivity", "mCheckedItems, getCount = " + mCheckedItems.size() + ", " + getCount());
        return (mCheckedItems.size() > 0 && mCheckedItems.size() == getCount()) ? true : false;
    }

    public List<File> getFileList() {
        return mFileList;
    }

    public void startSortInNewThread(SortListener l) {
        mHandler.post(new SortRunnable(l));
    }

    public void stopSort() {
        FileSort.getFileListSort().stopSort();
    }

    /**
     * Should not use it in main thread.
     */
    public void startSort() {
        startSort(null);
    }

    public void startSortInCurrentThread() {
        mIsSorting = true;
        FileSort.getFileListSort().sort(mFileList);
        Log.d(TAG, "startSortInCurrentThread, mIsSorting is false now");
        mIsSorting = false;
    }

    public void startSort(SortListener l) {
        if (null == mFileList || mFileList.isEmpty()) {
            Log.d(TAG, "file list is empty, stop sorting");
            mIsSorting = false;
            return;
        }
        if (mFileList.size() < MAX_SIZE_USE_THREAD_SORT) {
            startSortInCurrentThread();
        } else {
            startSortInNewThread(l);
        }

    }

    @Override
    public void notifyDataSetChanged() {
        super.notifyDataSetChanged();
        if (mIsSorting || mIsLoading) {
            notifyLoadingStart();
            if (mEmptyViewListener == null) {
                return;
            }
        } else {
            notifyLoadingFinish();
        }
        
        if (null != mEmptyViewListener) {
            Log.d(TAG, "notifyDataSetChanged, mFileList.isEmpty():"+mFileList.isEmpty());
            mEmptyViewListener.onEmptyStateChanged(mFileList.isEmpty());
        }
    }

    public void setFileSortListener(SortListener l) {
        mSortListener = l;
    }

    @Override
    public void onClick(View view) {
        if (view != null) {
            switch (view.getId()) {
                case R.id.file_icon:
                    if (view.getParent() != null) {
                        view.showContextMenu();
                    }
                    break;
            }
        }
    }

    public void setScrolling(boolean scrolling) {
        mScrolling = scrolling;
    }

    public void scrollStateChanged(ListView listView, int scrollState) {
        if (null == listView) {
            return;
        }
        ArrayList<String> newTaskUrl = new ArrayList<String>();
        int startPos = listView.getFirstVisiblePosition();
        int endPos = listView.getLastVisiblePosition();
        int fileListLen = mFileList.size();
        int pos = -1;
        File file = null;
        for (pos = startPos; pos <= endPos && pos < fileListLen; pos++) {//Bug229761
            file = mFileList.get(pos);
            if (file != null) {
                newTaskUrl.add(file.getPath());
            }
        }
        ImageLoadThreadManager.updateWorkQueue(newTaskUrl);
        if (OnScrollListener.SCROLL_STATE_IDLE == scrollState) {
            setScrolling(false);
            notifyDataSetChanged();
        } else {
            setScrolling(true);
        }
    }

    class FileExplorerScrollListener implements AbsListView.OnScrollListener {

        private ListView mListView;

        FileExplorerScrollListener(ListView listView) {
            this.mListView = listView;
        }

        @Override
        public void onScrollStateChanged(AbsListView view, int scrollState) {
            scrollStateChanged(mListView, scrollState);
        }

        @Override
        public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount,
                int totalItemCount) {
        }
    }

    class FileExplorerImageLoadCompleteListener implements ImageCache.OnImageLoadCompleteListener {

        private ListView mListView;

        FileExplorerImageLoadCompleteListener(ListView listView) {
            this.mListView = listView;
        }

        @Override
        public void OnImageLoadComplete(String fileUrl, boolean success,
                Bitmap bitmap) {
            if (null == mListView) {
                return;
            }
            if (success) {
                View view = mListView.findViewWithTag(fileUrl);
                if (view != null) {
                    int pos = -1;
                    int startPos = mListView.getFirstVisiblePosition();
                    int endPos = mListView.getLastVisiblePosition();
                    pos = mListView.getPositionForView(view);
                    if (pos < startPos || pos > endPos) {
                        return;
                    }
                    ImageView iv = (ImageView) view.findViewById(R.id.file_icon);
                    iv.setImageBitmap(bitmap);
                }
            } else {
                Log.w(TAG, "loadImageBitmap() == null, failed !");
            }
        }

    }

    public void saveCurPositionToBack (String filePath, int firstVisiblePosition) {
        mBackStackMap.put(filePath, firstVisiblePosition);
    }
    
    public int  restorePreviousPosition (String currentPath) {
        int firstVisiblePosition = 0;
        if (mBackStackMap != null && mBackStackMap.containsKey(currentPath)) {
            firstVisiblePosition = mBackStackMap.get(currentPath);
            mBackStackMap.remove(currentPath);
        }
        return firstVisiblePosition;
    }
    
    public void setNeedBack () {
        mNeedBack = true;
    }
    
    public void destroy(){
        if(mFileList != null) {
            mFileList.clear();
        }
        if (mBackStackMap != null) {
            mBackStackMap.clear();
        }
        if (mIsDirectoryList != null) {
            mIsDirectoryList.clear();
        }
    }
}
