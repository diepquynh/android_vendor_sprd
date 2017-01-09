package com.sprd.fileexplorer.adapters;


import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.FileObserver;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.widget.Toast;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.drmplugin.FileListAdapterUtils;
import com.sprd.fileexplorer.fragments.BaseFragment;
import com.sprd.fileexplorer.loadimage.ImageCache;
import com.sprd.fileexplorer.util.FileSort;
import com.sprd.fileexplorer.util.FileType;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexplorer.util.IntentUtil;
import com.sprd.fileexplorer.util.StorageUtil;

public class FileListAdapter extends FileAdapter {

    public static final String TAG = "FileListAdapter";
    /* SPRD: Add for bug615101. @{ */
    private static final int FOLDER_DELETE = 1073742336;
    private static final int FOLDER_MOVED_FROM = 1073741888;
    private static final int MAX_CHECK_TIME = 200;
    /* @} */

    public interface PathChangeListener {
        void onPathChanged(String path);
    }

    private PathChangeListener mPathChangeListener = null;
    private File mCurrentPath;
    private File mCurrentObservePath = null;
    private File mTopPath;
    private FileSort mSorter = FileSort.newInstance();
    private PathObserver mFileObserver = null;
    private ExecuteRunnable mExecuteRunnable = null;
    
    private FileExplorerScrollListener mScrollListener;
    
    private FileExplorerImageLoadCompleteListener mLoadCompleteListener;

    private Drawable mDefaultImageIcon;
    private Drawable mDefaultFolderIcon;
    private Drawable mDefaultApkIcon;
    private Drawable mDefaultVideoIcon;
    // SPRD 453003
    private boolean isBackPress;
    private static final File USB_LIST_DIRECTORY = new File("USB List");
    class PathObserver extends FileObserver {

        public PathObserver(String path, int mask) {
            super(path, FileObserver.DELETE | FileObserver.CREATE
                    | FileObserver.CLOSE_WRITE | FileObserver.MOVED_TO | FileObserver.MOVED_FROM);
        }

        @Override
        public void onEvent(int event, String path) {
            Log.d(TAG, "onEvent:event = " + event + ", path is " + path);
            if (mCurrentPath.exists()) {
                File file = new File(mCurrentPath.getAbsolutePath() + File.separator + path);
                /* SPRD: Make sure that the file is deleted before refreshing UI. @{ */
                if (event == FileObserver.DELETE || event == FOLDER_DELETE || event == FileObserver.MOVED_FROM
                        || event == FOLDER_MOVED_FROM) {

                    long startCheckTime = System.currentTimeMillis();
                    long currentTime = 0;
                    while (file.exists()) {
                        currentTime = System.currentTimeMillis();
                        if (currentTime - startCheckTime > MAX_CHECK_TIME) {
                            Log.d(TAG, "onEvent: Timeout, the refresh may have an error!");
                            break;
                        }
                    }
                }
                /* @} */
                if (file.exists()) {
                    synchronized (mFileList) {
                        mAddChangeList.add(new File(mCurrentPath.getAbsolutePath() + File.separator + path)); 
                    }
                } else {
                    synchronized (mFileList) {
                        mDeleteChangeList.add(file);
                    }
                }
                mMainThreadHandler.removeCallbacks(mUpdateChangeListRunnable);
                mMainThreadHandler.postDelayed(mUpdateChangeListRunnable, 500);
            } else {
                Log.d(TAG, "Folder was not exist");
                mCurrentObservePath = null;
                stopObserve();
                setCurrentPath(mTopPath);
                mMainThreadHandler.post(mNotifyRunnable);
            }
        }
        
    }
    
    private Runnable mUpdateChangeListRunnable = new Runnable() {
        @Override
        public void run() {
            updateFileListByObeserver();
        }
    };

    private Runnable mPathChangedRunnable = new Runnable() {
        @Override
        public void run() {
            if (mPathChangeListener != null) {
                mCurrentString = mCurrentPath.getAbsolutePath();
                mPathChangeListener.onPathChanged(mCurrentString);
            }
        }
    };

    private ArrayList<File> mAddChangeList = new ArrayList<File>();
    private ArrayList<File> mDeleteChangeList = new ArrayList<File>();

    public FileListAdapter(Context context, ListView list) {
        super(context);
        mContext = context;
        mDefaultImageIcon = mContext.getResources().getDrawable(R.drawable.file_item_image_ic);
        mDefaultFolderIcon = mContext.getResources().getDrawable(R.drawable.file_item_folder_ic);
        mDefaultApkIcon = mContext.getResources().getDrawable(R.drawable.file_item_apk_default_ic);
        mDefaultVideoIcon = mContext.getResources().getDrawable(R.drawable.file_item_video_ic);
        mLoadCompleteListener = new FileExplorerImageLoadCompleteListener(list);
        mScrollListener = new FileExplorerScrollListener(list);
        list.setOnScrollListener(mScrollListener);
        mExecuteRunnable = new ExecuteRunnable();

    }

    public FileListAdapter attachFragment(BaseFragment fragment) {
        // mAttachedFragment = fragment;
        return this;
    }

    // TODO: In future version, users can set their top path.
    public FileListAdapter initWithPath(File topPath) {
        if (topPath == null) {
            throw new NullPointerException("topPath is null");
        }
        mTopPath = topPath;
        mCurrentPath = topPath;
        mTopString = mTopPath.getAbsolutePath();
        mCurrentString = mCurrentPath.getAbsolutePath();
        refresh();
        return this;
    }

    public FileSort getFileSort() {
        return mSorter;
    }

    public void stopSort() {
        mSorter.stopSort();
    }

    public void setSortType(int type) {
        mSorter.setSortType(type);
    }

    public void startSort() {
        mHandler.post(mSortRunnable);
    }

    private Runnable mSortRunnable = new Runnable() {
        @Override
        public void run() {
            notifySortingStart();
            if (mSorter.isSorting()) {
                mSorter.stopSort();
                notifySortingFinished();
            }
            final List<File> workingList = new ArrayList<File>();
            synchronized (mFileList) {
                for (File f : mFileList) {
                    workingList.add(f);
                }
            }
            mIsSorting = true;
            mSorter.sort(workingList);
            mIsSorting = false;
            mMainThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    updateFileList(workingList, UPDATE_FILES_CLONE);
                }
            });
        
        }

    };

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = super.getView(position, convertView, parent);
        ViewHolder vHolder = (ViewHolder) view.getTag(R.id.action_paste_mode_cancel);
        File file = mFileList.get(position);
        /* SPRD: Add for bug607469. @{ */
        if (file == null) {
            return view;
        }
        /* @} */
        view.setTag(file.getPath());
        Bitmap bitmap = null;
        String filePath = file.getPath();
        Boolean isDirectory = mIsDirectoryList.get(file.getAbsolutePath());
        if(isDirectory == null){
            isDirectory = file.isDirectory();
        }
        if (isDirectory) {
            vHolder.fileIcon.setImageDrawable(mDefaultFolderIcon);
        } else {
            FileType localFileType = FileType.getFileType(mContext);
            String fileSuffix = FileType.getFileType(mContext).getSuffix(file);
            //Local Video file branch
            if (localFileType.getVideoFileType().contains(fileSuffix)) {
                if (mScrolling) {
                    bitmap = ImageCache.get(file.getPath());
                } else {
                    bitmap = ImageCache.get(file.getPath());
                    if (null == bitmap) {
                        ImageCache.loadMediaWithPath(mContext, file.getPath(), mLoadCompleteListener, mMainThreadHandler, true,
                                position);
                    }
                }
                if (null == bitmap) {
                    vHolder.fileIcon.setImageDrawable(mDefaultVideoIcon);
                } else {
                    vHolder.fileIcon.setImageBitmap(bitmap);
                }
            } else {
                int fileType = FileType.getFileType(mContext).getFileType(file);
                view.setTag(R.id.viewtag_filetype, fileType);
                vHolder.fileIcon.setImageResource(fileType);
                /*
                 * //for drm video file set icon
                 * if(isDrmSetIcon(mContext,file,vHolder)){ return view; }
                 */
                if (fileType == FileType.FILE_TYPE_IMAGE) {
                    if (mScrolling) {
                        bitmap = ImageCache.get(file.getPath());
                    } else {
                        bitmap = ImageCache.get(file.getPath());
                        if (null == bitmap) {
                            ImageCache.loadImageBitmap(mContext, file.getPath(), mLoadCompleteListener,
                                    mMainThreadHandler, true, position);
                        }
                    }
                    if (null == bitmap) {
                        vHolder.fileIcon.setImageDrawable(mDefaultImageIcon);
                    } else {
                        vHolder.fileIcon.setImageBitmap(bitmap);
                    }
                } else if (fileType == FileType.FILE_TYPE_PACKAGE) {
                    if (mScrolling) {
                        bitmap = ImageCache.get(file.getPath());
                    } else {
                        bitmap = ImageCache.get(file.getPath());
                        if (null == bitmap) {
                            ImageCache.loadImageBitmap(mContext, file.getPath(), mLoadCompleteListener,
                                    mMainThreadHandler, false, position);
                        }
                    }
                    if (null == bitmap) {
                        vHolder.fileIcon.setImageDrawable(mDefaultApkIcon);
                    } else {
                        vHolder.fileIcon.setImageBitmap(bitmap);
                    }
                }
            }
            // for drm video file set icon
            FileListAdapterUtils.getInstance().DRMFileSetIcon(mContext, file, vHolder);
        }
        return view;
    }

    public void startObserve() {
        if (mCurrentObservePath != null
                && mCurrentObservePath.getAbsolutePath().equals(mCurrentPath.getAbsolutePath())) {
            return;
        } else {
            Log.d(TAG, "startObserver: Start watching "
                    + mCurrentPath.getAbsolutePath());
            mCurrentObservePath = mCurrentPath;
            mFileObserver = new PathObserver(mCurrentPath.getAbsolutePath(), 0);
            mFileObserver.startWatching();
        }
    }

    public void setPathChangeListener(PathChangeListener l) {
        mPathChangeListener = l;
    }

    /**
     * Change filelist should be put into mainthread
     */
    private void updateFileListByObeserver() {
        Log.d(TAG, "apply change of filelist now.");
        synchronized (mFileList) {
            /* SPRD 442908 @{ */
            final SharedPreferences settings = PreferenceManager
                    .getDefaultSharedPreferences(mContext);
            boolean display = settings.getBoolean("display_hide_file", false);
            /* @} */
            if (!mAddChangeList.isEmpty()
                    && checkParentValid(mAddChangeList.get(0))) {
                /* SPRD 442908 @{ */
                for (File f : mAddChangeList) {
                    if (display) {
                        if (!mFileList.contains(f)) {
                            mFileList.add(f);
                        }
                    } else {
                        if (!mFileList.contains(f) && !f.isHidden()) {
                            mFileList.add(f);
                        }
                    }
                }
                /* @} */
            }
            if (!mDeleteChangeList.isEmpty()
                    && checkParentValid(mDeleteChangeList.get(0))) {
                for (File f : mDeleteChangeList) {
                    // SPRD 437777
                    if(f != null && !f.exists())
                    mFileList.remove(f);

                }
            }
            mAddChangeList.clear();
            mDeleteChangeList.clear();
        }
        notifyDataSetChanged();
    }

    protected boolean checkParentValid(File file) {
        Log.d(TAG, "checkParentValid");
        if (file == null) {
            return false;
        } else {
            File parentFile = file.getParentFile();
            if (parentFile == null) {
                return false;
            }
            if (parentFile.getAbsolutePath().equals(
                    mCurrentPath.getAbsolutePath())) {
                return true;
            } else {
                return false;
            }
        }
    }

    public void stopObserve() {
        Log.d(TAG, "stopObserve");
        mFileObserver.stopWatching();
        synchronized (mFileList) {
            mAddChangeList.clear();
            mDeleteChangeList.clear();
        }
    }

    class ExecuteRunnable implements Runnable {
        private int mPosition;
        private int mFirstVisiblePosition;
        
        public void execute(int which, int firstVisiblePosition) {
            mPosition = which;
            mFirstVisiblePosition = firstVisiblePosition;
            Log.d(TAG, "execute " + which);
            mHandler.post(this);
        }

        @Override
        public void run() {
            Log.d(TAG, "execute=" + mPosition);
            File executeFile = null;
            synchronized (mFileList) {
                if (mPosition < mFileList.size()) {
                    executeFile = mFileList.get(mPosition);
                } else {
                    mMainThreadHandler.post(mNotifyRunnable);
                    return;
                }
            }
            
            File newPath = FileUtil.execute(mCurrentPath, executeFile, mContext);
            if (!newPath.equals(mCurrentPath)) {
                Log.d(TAG, "saveCurPositionToBack");
                saveCurPositionToBack(mCurrentPath.getAbsolutePath(), mFirstVisiblePosition);
                mCurrentPath = newPath;
                mCurrentString = mCurrentPath.getAbsolutePath();
                mIsLoading = true;
                notifyLoadingStart();
                mMainThreadHandler.post(mPathChangedRunnable);
                refreshInternal();
            } else {
                int fileType = FileType.getFileType(mContext).getFileType(executeFile);
                final Intent intent = IntentUtil.getIntentByFileType(mContext,
                        fileType, executeFile);
                final String filePath = executeFile.getPath();
                Log.i(TAG, "file type = " + fileType);
                Log.i(TAG, "intent = " + intent);
                //SPRD:585915 file:// uri is not allowed in 7.0, use content uri
                final Uri uri = FileUtil.getFileUri(executeFile, FileType.getTypeByFile(executeFile), mContext);
                final File file = executeFile;
                //drm file item click 
                if(FileListAdapterUtils.getInstance().DRMFileSendIntent(mContext,file,uri,mMainThreadHandler)){
                    return;
                }
                if (intent != null) {
                    mMainThreadHandler.post(new Runnable() {
                        @Override
                        public void run() {
                             mContext.startActivity(intent);
                         }
                    });

                } else {
                    mMainThreadHandler.post(new Runnable() {

                        @Override
                        public void run() {
                            Toast.makeText(mContext, mContext.getString(R.string.msg_invalid_intent), Toast.LENGTH_LONG).show();
                        }
                    });

                }
            }
        }
    };

    public void execute(int position, int firstVisiblePosition) {
        mExecuteRunnable.execute(position, firstVisiblePosition);
    }

    private Runnable mRefreshRunnable = new Runnable() {
        @Override
        public void run() {
            notifyLoadingStart();
            if (refreshInternal()) {
                notifyLoadingFinish();
            }
        }
    };

    public void ensureRefresh() {
        mHandler.post(new Runnable() {

            @Override
            public void run() {
                // SPRD: Add fo bug469870.
                isBackPress = false;
                /* SPRD: Modify for bug612668. @{ */
                if (mCurrentPath.equals(USB_LIST_DIRECTORY)
                        || (!mCurrentPath.equals(USB_LIST_DIRECTORY) && !StorageUtil.isStorageMounted(mCurrentPath))) {
                    if (!mCurrentPath.equals(USB_LIST_DIRECTORY)) {
                        mCurrentPath = USB_LIST_DIRECTORY;
                        mMainThreadHandler.post(mPathChangedRunnable);
                    }
                    /* @} */
                    mIsSorting  = false;
                    mIsLoading  = false;
                    final List<File> usbList = new ArrayList<File>();
                    File[] usbVolume = StorageUtil.getUSBVolume();
                    for (File f : usbVolume) {
                        /* SPRD: Modify for bug612668. @{ */
                        if (f != null && StorageUtil.isStorageMounted(f)) {
                            usbList.add(f);
                        }
                        /* @} */
                    }
                    mMainThreadHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            updateFileList(usbList, UPDATE_FILES_CLONE);
                        }
                    });
                    return;
                }
                //checkCurrentPathValid();
                File[] files = mCurrentPath.listFiles();
                if (files == null) {
                    Log.d("duke", "files == null");
                    mMainThreadHandler.post(new Runnable() {

                        @Override
                        public void run() {
                            synchronized (mFileList) {
                                mFileList.clear();
                                notifyDataSetChanged();
                            }
                        }
                    });
                    return;
                }
                int hiddenCount = 0;
                final SharedPreferences settings = PreferenceManager
                        .getDefaultSharedPreferences(mContext);
                boolean display = settings.getBoolean("display_hide_file", false);
                if (!display) {
                    for (File f : files) {
                        /* SPRD 453003 @{ */
                        if(isBackPress){
                            break;
                        }
                        /* @} */
                        if (f.isHidden()) {
                            hiddenCount++;
                        }
                    }
                }
                if (mFileList.size() != files.length - hiddenCount) {
                    Log.d(TAG, "Size doesn't match");
                    /* SPRD 453003 @{ */
                    //refreshInternal();
                    refreshInternal(isBackPress);
                    /* @} */
                    return;
                }

                File parentFile = null;
                synchronized (mFileList) {
                    /* SPRD 447777 While rename a file ,a call come in leading to failed to show the file in the filelist@{ */
                    if(checkInvalidFile()){
                        /* SPRD 453003 @{ */
                        //refreshInternal();
                        refreshInternal(isBackPress);
                        /* @} */
                        return;
                    }
                    /* @} */
                    if (0 == mFileList.size()) {
                        mMainThreadHandler.post(mNotifyRunnable);
                        return;
                    } else {
                        parentFile = mFileList.get(0);
                    }
                }
                if (parentFile != null
                        && (files.length == 0 || !files[0].getParentFile()
                                .equals(parentFile.getParentFile()))) {
                    Log.d(TAG, "path doesn't match");
                    /* SPRD 453003 @{ */
                    //refreshInternal();
                    refreshInternal(isBackPress);
                    /* @} */
                }
            }
        });
    }
    /* SPRD 445923 While rename a file ,a call come in leading to failed to show the file in the filelist@{ */
    private boolean checkInvalidFile(){
        for(File f : mFileList){
            /* SPRD 453003 @{ */
            if(isBackPress){
                Log.d(TAG, "checkInvalidFile stop check");
                return false;
            }
            /* @} */
            if(f != null && !f.exists()){
            Log.d(TAG, "checkInvalidFile f:"+f.getAbsolutePath());
            return true;
            }
        }
        return false;
    }
    /* @} */
    public void refresh() {
        mHandler.post(mRefreshRunnable);
    }

    private final void checkCurrentPathValid () {
        if (mCurrentPath == null
                || mCurrentPath.isFile()
                || !mCurrentPath.exists()
                || !mCurrentPath.canRead()
                || !mCurrentPath.getAbsolutePath().contains(
                        mTopPath.getAbsolutePath())) {
            mCurrentPath = mTopPath;
            mCurrentString = mCurrentPath.getAbsolutePath();
            mMainThreadHandler.post(mPathChangedRunnable);
        }
    }
    /* SPRD 453003 @{ */
    private boolean refreshInternal(boolean status){
        if(status){
            Log.d(TAG, "stop refreshInternal");
            return true;
        }
        return refreshInternal();
    }
    /* @} */
    public boolean refreshInternal() {
        Log.d(TAG, "refreshInternal, mCurrentPath:"+mCurrentPath);
        mIsLoading = true;
        mCancelLoading = false;
        /* SPRD: Modify for bug612668. @{ */
        if (mCurrentPath.equals(USB_LIST_DIRECTORY)
                || (!mCurrentPath.equals(USB_LIST_DIRECTORY) && !StorageUtil.isStorageMounted(mCurrentPath))) {
            if (!mCurrentPath.equals(USB_LIST_DIRECTORY)) {
                mCurrentPath = USB_LIST_DIRECTORY;
                mMainThreadHandler.post(mPathChangedRunnable);
            }
            /* @} */
            mIsSorting  = false;
            mIsLoading  = false;
            final List<File> usbList = new ArrayList<File>();
            File[] usbVolume = StorageUtil.getUSBVolume();
            for (File f : usbVolume) {
                /* SPRD: Modify for bug612668. @{ */
                if (f != null && StorageUtil.isStorageMounted(f)) {
                    usbList.add(f);
                }
                /* @} */
            }
            mMainThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    updateFileList(usbList, UPDATE_FILES_CLONE);
                }
            });
            return true;
        }
        //checkCurrentPathValid();
        File[] files = mCurrentPath.listFiles();
        if (files == null) {
            Log.d(TAG, " refreshInternal, files is null");
            mMainThreadHandler.post(new Runnable() {

                @Override
                public void run() {
                    synchronized (mFileList) {
                        mFileList.clear();
                        notifyDataSetChanged();
                    }
                }
            });
            mIsLoading = false;
            return true;
        }
        Log.d(TAG, "refreshInternal, files.length = " + files.length);
        synchronized (mIsDirectoryList) {
            mIsDirectoryList.clear();
        }
        final SharedPreferences settings = PreferenceManager
                .getDefaultSharedPreferences(mContext);
        boolean display = settings.getBoolean("display_hide_file", false);

        ArrayList<File> filesList = new ArrayList<File>();
        ArrayList<File> dirsList = new ArrayList<File>();
        Log.d(TAG, "f.exists()---start");
        for (File f : files) {
            if (mCancelLoading) {
                break;
            }
            if (display) {
                if (f.isDirectory()) {
                    dirsList.add(f);
                } else {
                    filesList.add(f);
                }
            }else {
                if (!f.isHidden()) {
                    if (f.isDirectory()) {
                        dirsList.add(f);
                    } else {
                        filesList.add(f);
                    }
                }
            }
        }
        Log.d(TAG, "f.exists()---end");
        if (mCancelLoading) {
            mIsLoading = false;
            Log.d(TAG, "CancelLoading");
            return false;
        }
        mIsSorting = true;
        Log.d(TAG, "f.sort()---start");
        /* SPRD 443730 @{ */
        SharedPreferences mSortPreferences = mContext.getSharedPreferences(FileSort.FILE_SORT_KEY, 0);
        int sortType = mSortPreferences.getInt(FileSort.FILE_SORT_KEY, FileSort.SORT_BY_NAME);
        Log.d(TAG, "f.sort()---sortType:"+sortType);
        setSortType(sortType);
        /* @} */
        mSorter.sortDefault(dirsList);
        mSorter.sortDefault(filesList);
        Log.d(TAG, "f.sort()---end");
        mIsSorting = false;
        startObserve();
        final ArrayList<File> result = new ArrayList<File>();
        for (File file : dirsList) {
            mIsDirectoryList.put(file.getAbsolutePath(), true);
            result.add(file);
        }
        for (File file : filesList) {
            mIsDirectoryList.put(file.getAbsolutePath(), false);
            result.add(file);
        }
        mIsLoading = false;
        mMainThreadHandler.post(new Runnable() {
            @Override
            public void run() {
                updateFileList(result, UPDATE_FILES_CLONE);
            }
        });
        return true;
    }

    class PopupRunnable implements Runnable {
        public void popupFolder() {
            mSorter.stopSort();
            mIsSorting = false;
            notifySortingFinished();
            // SPRD 453003
            isBackPress = true;
            mHandler.post(this);
        }

        @Override
        public void run() {
            if (StorageUtil.isInUSBVolume(mCurrentPath.toString())
                    && mCurrentPath.getParentFile().getAbsolutePath().equals("/storage")) {
                mCurrentPath= USB_LIST_DIRECTORY;
            } else {
                mCurrentPath = mCurrentPath.getParentFile();
            }
            if (mCurrentPath == null) {
                mCurrentPath = mTopPath;
            }
            mCurrentString = mCurrentPath.getAbsolutePath();
            mMainThreadHandler.post(mPathChangedRunnable);
            notifyLoadingStart();
            
            if (refreshInternal()) {
                return;
            }
            // SPRD 453003
            isBackPress = false;
            // notifyLoadingFinish();
            mMainThreadHandler.post(mNotifyRunnable);
        }
    }

    private PopupRunnable mPopupRunnable = new PopupRunnable();

    /**
     * Go up level.
     * 
     * @return Return if finish activity.
     */
    public boolean popupFolder() {
        if (mCurrentPath == null) {
            Log.e(TAG, "currentpath == null");
            mCurrentPath = mTopPath;
            refresh();
            return false;
        }
        if (mTopPath.getAbsolutePath().equals(mCurrentPath.getAbsolutePath())) {
            //notifyDataSetChanged();//SPRD:Bug 276734
            return true;
        }
        mPopupRunnable.popupFolder();
        return false;
    }

    public File getCurrentPath() {
        return mCurrentPath;
    }

    public void setCurrentPath(File currentPath) {
        mCurrentPath = currentPath;
        mCurrentString = mCurrentPath.getAbsolutePath();
        mPathChangedRunnable.run();
        refresh();
    }

    public File getTopPath() {
        return mTopPath;
    }
    @Override
    public void destroy(){
        super.destroy();
        synchronized (mFileList) {
	        if (mAddChangeList != null) {
	            mAddChangeList.clear();
	        }
	        if (mDeleteChangeList !=  null) {
	            mDeleteChangeList.clear();
	        }
        }
        mDefaultImageIcon = null;
        mDefaultFolderIcon = null;
        mDefaultApkIcon = null;
    }
}
