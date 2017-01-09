package com.sprd.fileexplorer.adapters;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import android.app.Activity;
import android.app.LoaderManager;
import android.app.ProgressDialog;
import android.content.ContentUris;
import android.content.Context;
import android.content.CursorLoader;
import android.content.Loader;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.provider.MediaStore.Audio;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.MediaColumns;
import android.provider.MediaStore.Video;
import android.util.Slog;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.util.Log;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.drmplugin.QuickScanCursorAdapterUtils;
import com.sprd.fileexplorer.loadimage.ImageCache;
import com.sprd.fileexplorer.util.FileSort;
import com.sprd.fileexplorer.util.FileType;
import com.sprd.fileexplorer.util.FileUtil;

public class QuickScanCursorAdapter extends FileAdapter implements View.OnClickListener {
    private static String TAG = "QuickScanCursorAdapter";
    public static final int RESOURCE_VIDEO = 1;

    public static final int RESOURCE_IMAGE = 2;

    public static final int RESOURCE_AUDIO = 3;

    public static final int RESOURCE_DOC = 4;

    public static final int RESOURCE_APK = 5;

    private Activity mContext;

    private int mResourceType;

    private int mPathColumnIndex;

    private ListView mListView;

    private boolean mIsSelectMode;

    private ProgressDialog mWaitDialog;

    private View mEmptyView;

    private boolean mTimeup = false;

    private volatile boolean mLoadFinished = false;

    private int mPosition = 0;

    private Handler mWorkingThread;

    private SharedPreferences mSettings;

    private FileExplorerScrollListener mScrollListener;

    private FileExplorerImageLoadCompleteListener mLoadCompleteListener;

    private Drawable mDefaultImageIcon;

    private Drawable mDefaultApkIcon;

    public QuickScanCursorAdapter(Activity context, int resourceType,
            String orderStr) {
        super(context);
        // SPRD: Modify for bug635086.
        mWorkingThread = new Handler(mHandlerThread.getLooper());
        mResourceType = resourceType;
        mContext = context;
        mDefaultApkIcon = mContext.getResources().getDrawable(R.drawable.file_item_apk_default_ic);
        mDefaultImageIcon = mContext.getResources().getDrawable(R.drawable.file_item_image_ic);

        mSettings = PreferenceManager
                .getDefaultSharedPreferences(mContext);
        refresh();
    }

    public QuickScanCursorAdapter(Activity context, int resourceType,
            View emptyView, ListView listview, boolean isSelectMode) {
        super(context);
        // SPRD: Modify for bug635086.
        mWorkingThread = new Handler(mHandlerThread.getLooper());
        mResourceType = resourceType;
        mContext = context;
        mDefaultApkIcon = mContext.getResources().getDrawable(R.drawable.file_item_apk_default_ic);
        mDefaultImageIcon = mContext.getResources().getDrawable(R.drawable.file_item_image_ic);

        mListView = listview;
        mLoadCompleteListener = new FileExplorerImageLoadCompleteListener(mListView);
        mScrollListener = new FileExplorerScrollListener(mListView);
        mListView.setOnScrollListener(mScrollListener);
        mIsSelectMode =  isSelectMode;
        mWaitDialog = showWaitDialog();
        WaitTimer mTimer = new WaitTimer(500,500);
        mTimer.start();
        mEmptyView = emptyView;
        mSettings = PreferenceManager.getDefaultSharedPreferences(mContext);
        refresh();
    }

    public QuickScanCursorAdapter(Activity context, int resourceType,
            int position, ListView listview, boolean isSelectMode) {
        super(context);
        mResourceType = resourceType;
        mContext = context;
        mDefaultApkIcon = mContext.getResources().getDrawable(R.drawable.file_item_apk_default_ic);
        mDefaultImageIcon = mContext.getResources().getDrawable(R.drawable.file_item_image_ic);

        // SPRD: Modify for bug635086.
        mWorkingThread = new Handler(mHandlerThread.getLooper());
        mListView = listview;
        mLoadCompleteListener = new FileExplorerImageLoadCompleteListener(mListView);
        mScrollListener = new FileExplorerScrollListener(mListView);
        mListView.setOnScrollListener(mScrollListener);
        mIsSelectMode =  isSelectMode;
        mWaitDialog = showWaitDialog();
        WaitTimer mTimer = new WaitTimer(500,500);
        mTimer.start();
        mPosition = position;
        mSettings = PreferenceManager
                .getDefaultSharedPreferences(mContext);
        refresh();
    }

    public interface CursorChangeListener {
        public void onChange(int count);
    }
    private CursorChangeListener mCursorChangeListener;
    public void setCursorChangeListener(CursorChangeListener c){
        mCursorChangeListener = c;
    }

    class LoadFinishedRunnable implements Runnable {
        private ArrayList<File> mAcceptList = new ArrayList<File>();
        /*sprd: modified for bug 403126 {@*/
        /* SPRD 443151 @{*/
        private ArrayList<File> mWorkingList= new ArrayList<File>();
        private List<File> mTempWorkingList;
        /* @} */
        private ArrayList<File> mTempList;
        /*@}*/
        volatile boolean mIsLoading = false;
        volatile boolean mIsDirty = false;

        /*
         * boolean display = mSettings.getBoolean("display_hide_file", false);
         */

        private void reload() {
            synchronized (mAcceptList) {
                mWorkingList = (ArrayList<File>) mAcceptList.clone();
                mIsLoading = true;
                mIsDirty = false;
                mWorkingThread.post(this);
            }

        }

        public void changeCursor(Cursor cursor) {
            Slog.d("MultiSelectActivity", "changeCursor");
            if (cursor != null && cursor.isClosed()) {
                refresh();
                return;
            }
            synchronized (mAcceptList) {
                mAcceptList.clear();
                if (cursor != null && cursor.getCount() > 0) {
                    mPathColumnIndex = cursor.getColumnIndexOrThrow(MediaColumns.DATA);
                    while (cursor.moveToNext()) {
                        String path = cursor.getString(mPathColumnIndex);
                        File file = new File(path);
                        mAcceptList.add(file);
                    }
                }
                /*sprd: modified for bug 403126 {@*/
                mTempList = (ArrayList<File>) mAcceptList.clone();
            }
            Slog.d("MultiSelectActivity", "mIsLoading = " + mIsLoading);
            synchronized (mWorkingList) {
                if (mIsLoading) {
                    mWorkingList = (ArrayList<File>) mTempList.clone();
                    mIsDirty = true;
                } else {
                    mWorkingList = (ArrayList<File>) mTempList.clone();
                    mIsLoading = true;
                    mIsDirty = false;
                    mWorkingThread.removeCallbacks(this);
                    mWorkingThread.postDelayed(this, 500);
                }
            }
            /*@}*/
        }

        /**
         * Time casting action, put it into thread.
         *
         * @param file
         *            , the file you want to check, you can give a null file
         *            into here, but suggest not to do that
         * @param isShowHide
         *            Whether show hidden which set by {@code SharedPreferences}
         * @return a boolean value if need to put into {@code ArrayList}
         */
        private boolean isValid(File file, boolean isShowHide) {
            if (file == null) {
                return false;
            }
            // SPRD: Modify for bug615444.
            return file.exists() && (isShowHide ? true : !FileUtil.isHidden(file.getAbsolutePath()));
        }

        @Override
        public void run() {
            ArrayList<File> invalidFiles = new ArrayList<File>();
            mLoadFinished = false;
            synchronized (mWorkingList) {
                boolean isShowHide = mSettings.getBoolean("display_hide_file", false);
                Slog.d("MultiSelectActivity", "Traverse the mWorkingList and remove the invalid file ");
                for (File f: mWorkingList) {
                    if (!isValid(f, isShowHide)) {
                        if (null != f) {
                            invalidFiles.add(f);
                        }
                    }
                }
                if (!invalidFiles.isEmpty()) {
                    for(File file:invalidFiles) {
                        mWorkingList.remove(file);
                    }
                }
                /* SPRD 443151 @{ */
                //SharedPreferences sortPreference = mContext
                //        .getSharedPreferences(FileSort.FILE_SORT_KEY, 0);
                //FileSort sorter = FileSort.getFileListSort();
                //sorter.setSortType(sortPreference.getInt(
                //        FileSort.FILE_SORT_KEY, FileSort.SORT_BY_NAME));
                //sorter.sortDefault(mWorkingList);
                /* @] */
                mLoadFinished = true;
                mMainThreadHandler.post(new Runnable() {

                    @Override
                    public void run() {
                        if (!mLoadFinished) {
                            return;
                        }
                        QuickScanCursorAdapter.this.mIsLoading = true;
                        synchronized (mWorkingList) {
                            Slog.d("fileexplore","Traverse the mWorkingList in mMainThreadHandler");
                            mFileList.clear();
                            /* SPRD 443151 @{ */
                            mTempWorkingList = (ArrayList<File>) mWorkingList.clone();
                            SharedPreferences sortPreference = mContext
                                    .getSharedPreferences(FileSort.FILE_SORT_KEY, 0);
                            FileSort sorter = FileSort.getFileListSort();
                            sorter.setSortType(sortPreference.getInt(
                                    FileSort.FILE_SORT_KEY, FileSort.SORT_BY_NAME));
                            sorter.sortDefault(mTempWorkingList);
                            for (File f : mTempWorkingList) {
                            //for (File f : mWorkingList) {
                            /* @} */
                                if(f != null){
                                    Slog.d("fileexplore","Traverse the mWorkingList in mMainThreadHandler f:"+f.getAbsolutePath());
                                }else{
                                    Slog.d("fileexplore","Traverse the mWorkingList in mMainThreadHandler f doesn't exsit");
                                }
                                /* SPRD: Modify for bug521802. @{ */
                                if (f.isDirectory()) {
                                    continue;
                                }
                                /* @} */
                                mFileList.add(f);
                            }
                        }
                        mIsLoading = false;
                        QuickScanCursorAdapter.this.mIsLoading = false;
                        notifyDataSetChanged();
                        if (mWaitDialog != null && mWaitDialog.isShowing()
                                && mTimeup) {
                            mWaitDialog.dismiss();
                            mListView.setEmptyView(mEmptyView);
                            mListView.setSelection(mPosition);
                        }
                        int count = -1;
                        count = mFileList.size();
                        if(mCursorChangeListener != null){
                            mCursorChangeListener.onChange(count);
                        }
                    }
                });
                if (mIsDirty) {
                    reload();
                }
            }

        };
    }
    private LoadFinishedRunnable mFinishedRunnable = new LoadFinishedRunnable();

    private LoaderManager.LoaderCallbacks<Cursor> mFileLoaderListener = new LoaderManager.LoaderCallbacks<Cursor>(){
        @Override
        public Loader<Cursor> onCreateLoader(int id, Bundle args) {
            mListView.setEmptyView(null);
            String[] projection = new String[] { MediaColumns._ID,
                    MediaColumns.DATA, MediaColumns.TITLE,
                    MediaColumns.DATE_MODIFIED, MediaColumns.SIZE};
            String selection = null;
            Uri uri = null;
            final SharedPreferences sortPreference =  mContext.getSharedPreferences(
                    FileSort.FILE_SORT_KEY, 0);
            int sort = sortPreference.getInt(FileSort.FILE_SORT_KEY, FileSort.SORT_BY_NAME);

            String sortBy = FileSort.getOrderStr(sort);
            switch (mResourceType) {
            case RESOURCE_VIDEO:
                /* SPRD 425906 query from table files @{ */
                //uri = Video.Media.EXTERNAL_CONTENT_URI;
                uri = MediaStore.Files.getContentUri("external");
                selection = MediaStore.Files.FileColumns.DATA + " like '%.mpeg' or " +  MediaStore.Files.FileColumns.DATA + " like '%.mpg' or "
                        /* SPRD 433748 @{ */
                        + MediaStore.Files.FileColumns.DATA + " like '%.mp4' and "+ MediaStore.Files.FileColumns.MIME_TYPE + " like 'video/%' or "
                        // SPRD 446719
                        + MediaStore.Files.FileColumns.DATA + " like '%.3gp' and "+ MediaStore.Files.FileColumns.MIME_TYPE + " like 'video/%' or "
                        /* @} */
                        /* SPRD 427041 @{ */
                        + MediaStore.Files.FileColumns.DATA + " like '%.3gpp' and " + MediaStore.Files.FileColumns.MIME_TYPE + " like 'video/%' or "
                        // SPRD: Add for bug507035.
                        + MediaStore.Files.FileColumns.DATA + " like '%.3g2' and " + MediaStore.Files.FileColumns.MIME_TYPE + " like 'video/%' or "
                        // SPRD 459132 & 516922.
                        + MediaStore.Files.FileColumns.DATA + " like '%.m4v' or " + MediaStore.Files.FileColumns.DATA + " like '%.vob' or "
                        /* @} */
                        + MediaStore.Files.FileColumns.DATA + " like '%.3gpp2' or " + MediaStore.Files.FileColumns.DATA + " like '%.mkv' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.webm' or " + MediaStore.Files.FileColumns.DATA + " like '%.ts' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.avi' or " + MediaStore.Files.FileColumns.DATA + " like '%.flv' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.f4v' or " + MediaStore.Files.FileColumns.DATA + " like '%.divx' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.amc' or " + MediaStore.Files.FileColumns.DATA + " like '%.k3g' or "
                        // SPRD 467481
                        + MediaStore.Files.FileColumns.DATA + " like '%.dcf' and " + MediaStore.Files.FileColumns.MIME_TYPE + " like 'video/%' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.wmv' or " + MediaStore.Files.FileColumns.DATA + " like '%.asf' or "
                        // SPRD: Add for bug611635.
                        + MediaStore.Files.FileColumns.DATA + " like '%.mov' or "+ MediaStore.Files.FileColumns.DATA + " like '%.m2ts' or "
                        // SPRD: Add for bug562702
                        + MediaStore.Files.FileColumns.DATA + " like '%.rmvb'";
                /* @} */
                break;
            case RESOURCE_IMAGE:
                /* SPRD 425906 query from table files @{ */
                //uri = Images.Media.EXTERNAL_CONTENT_URI;
                uri = MediaStore.Files.getContentUri("external");
                selection = MediaStore.Files.FileColumns.DATA + " like '%.jpg' or " +  MediaStore.Files.FileColumns.DATA + " like '%.jpeg' or "
                        /* SPRD: Add for bug608514. @{ */
                        + MediaStore.Files.FileColumns.DATA + " like '%.cr2' or " + MediaStore.Files.FileColumns.DATA + " like '%.dng' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.nef' or " + MediaStore.Files.FileColumns.DATA + " like '%.orf' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.pef' or " + MediaStore.Files.FileColumns.DATA + " like '%.raf' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.rw2' or " + MediaStore.Files.FileColumns.DATA + " like '%.srw' or "
                        /* @} */
                        + MediaStore.Files.FileColumns.DATA + " like '%.gif' or " + MediaStore.Files.FileColumns.DATA + " like '%.png' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.bmp' or " + MediaStore.Files.FileColumns.DATA + " like '%.drmbmp' or "
                        /* SPRD 432651 Add jpe */
                        + MediaStore.Files.FileColumns.DATA + " like '%.wbmp' or " + MediaStore.Files.FileColumns.DATA + " like '%.webp' or "
                        // SPRD 467481
                        + MediaStore.Files.FileColumns.DATA + " like '%.dcf' and " + MediaStore.Files.FileColumns.MIME_TYPE + " like 'image/%' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.jpe'";
                        /* @} */
                /* @} */
                break;
            case RESOURCE_AUDIO:
                /* SPRD 462989 @{ */
                //uri = Audio.Media.EXTERNAL_CONTENT_URI;
                uri = MediaStore.Files.getContentUri("external");
                selection = MediaStore.Files.FileColumns.DATA + " like '%.mp3' or " +  MediaStore.Files.FileColumns.DATA + " like '%.acc' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.mp4' and "+ MediaStore.Files.FileColumns.MIME_TYPE + " like 'audio/%' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.3gp' and "+ MediaStore.Files.FileColumns.MIME_TYPE + " like 'audio/%' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.3gpp' and " + MediaStore.Files.FileColumns.MIME_TYPE + " like 'audio/%' or "
                        // SPRD 498509
                        + MediaStore.Files.FileColumns.DATA + " like '%.opus' or " + MediaStore.Files.FileColumns.DATA + " like '%.mka' or "
                        // SPRD: Add for bug507035.
                        + MediaStore.Files.FileColumns.DATA + " like '%.3g2' and " + MediaStore.Files.FileColumns.MIME_TYPE + " like 'audio/%' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.aidf' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.av' or " + MediaStore.Files.FileColumns.DATA + " like '%.cd' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.ogg' or " + MediaStore.Files.FileColumns.DATA + " like '%.midi' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.amr' or " + MediaStore.Files.FileColumns.DATA + " like '%.vqf' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.aac' or " + MediaStore.Files.FileColumns.DATA + " like '%.mid' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.m4a' or " + MediaStore.Files.FileColumns.DATA + " like '%.mpga' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.imy' or " + MediaStore.Files.FileColumns.DATA + " like '%.awb' or "
                        // SPRD 467481
                        + MediaStore.Files.FileColumns.DATA + " like '%.dcf' and " + MediaStore.Files.FileColumns.MIME_TYPE + " like 'audio/%' or "
                        // SPRD: Add for bug511015.
                        + MediaStore.Files.FileColumns.DATA + " like '%.m4b' or " + MediaStore.Files.FileColumns.DATA + " like '%.m4r' or "
                        // SPRD: Add for bug493355.
                        + MediaStore.Files.FileColumns.DATA + " like '%.wav' or " + MediaStore.Files.FileColumns.DATA + " like '%.flac' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.oga'" ;
                /* @} */
                break;
            case RESOURCE_DOC:
                uri = MediaStore.Files.getContentUri("external");
                selection = MediaStore.Files.FileColumns.DATA + " like '%.txt' or " +  MediaStore.Files.FileColumns.DATA + " like '%.log' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.doc' or " + MediaStore.Files.FileColumns.DATA + " like '%.pdf' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.ppt' or " + MediaStore.Files.FileColumns.DATA + " like '%.pptx' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.xls' or " + MediaStore.Files.FileColumns.DATA + " like '%.xlsx' or "
                        // SPRD 467481
                        + MediaStore.Files.FileColumns.DATA + " like '%.dcf' and " + MediaStore.Files.FileColumns.MIME_TYPE + " is 'text/plain' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.docx'" ;
                break;
            case RESOURCE_APK:
                uri = MediaStore.Files.getContentUri("external");
                /*  SPRD 467481 @{ */
                selection = MediaStore.Files.FileColumns.DATA + " like '%.apk' or "
                        + MediaStore.Files.FileColumns.DATA + " like '%.dcf' and " + MediaStore.Files.FileColumns.MIME_TYPE + " is 'application/vnd.android.package-archive'";
                /* @} */
                break;
            }
            return new FileLoader(mContext, uri, projection, selection,sortBy);
        }

        @Override
        public void onLoadFinished(Loader<Cursor> loader, final Cursor cursor) {
            if (cursor != null && !cursor.isClosed()) {
                mFinishedRunnable.changeCursor(cursor);
            }
        }
        @Override
        public void onLoaderReset(Loader<Cursor> loader) {
            // TODO Change cursor here?
        }

    };

    public void refresh() {
        final SharedPreferences sortPreference =  mContext.getSharedPreferences(
                FileSort.FILE_SORT_KEY, 0);
        int sort = sortPreference.getInt(FileSort.FILE_SORT_KEY, FileSort.SORT_BY_NAME);
        int id = mResourceType * 10 + sort;
        mContext.getLoaderManager().restartLoader(id, null, mFileLoaderListener);
    }

    /* SPRD: bug535304 destroyLoader when request permissions in onStart() @{ */
    public void end() {
        final SharedPreferences sortPreference =  mContext.getSharedPreferences(FileSort.FILE_SORT_KEY, 0);
        int sort = sortPreference.getInt(FileSort.FILE_SORT_KEY, FileSort.SORT_BY_NAME);
        int id = mResourceType * 10 + sort;
        mContext.getLoaderManager().destroyLoader(id);
    }
    /* @} */

    /* SPRD:433606 the file disappear when rename the file @{*/
    private String mWaitFile = null;
    // SPRD 445518
    //private static final int MAX_CHECK_COUNT = 10;
    private static final int MAX_CHECK_COUNT = 100;
    private int mCheckCount = 0;
    private Runnable mMediaStoreUpdate = new Runnable() {
        @Override
        public void run() {
            Cursor cursor = mContext.getContentResolver().query(MediaStore.Files.getContentUri("external"),//uri,
                    new String[] { MediaStore.Files.FileColumns.DATA },//projection,
                    MediaStore.Files.FileColumns.DATA + "=?",//selection,
                    new String[] {mWaitFile},//selectionArgs,
                    null);
            mCheckCount++;
            if (cursor != null && cursor.getCount() == 1) {
                refresh();
            } else {
                if (mCheckCount < MAX_CHECK_COUNT) {
                    mMainThreadHandler.postDelayed(mMediaStoreUpdate, 100);
                }
            }

            /* SPRD: 465324 close cursor @{ */
            if (cursor != null) {
                cursor.close();
            }
            /* @} */
        }
    };

    public void refreshWaitForFile(String file) {
        if (file != null && (new File(file)).exists()) {
            mWaitFile = file;
            mCheckCount = 0;
            mMainThreadHandler.post(mMediaStoreUpdate);
        }
    }
    /* @}*/

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = super.getView(position, convertView, parent);
        ViewHolder vHolder = (ViewHolder) view.getTag(R.id.action_paste_mode_cancel);
        File file = mFileList.get(position);
        String filePath = file.getPath();
        view.setTag(file.getPath());
        Bitmap bitmap = null;
        switch (mResourceType) {
        case RESOURCE_VIDEO:
            vHolder.fileIcon.setImageResource(FileType.getFileType(mContext).getVideoFileIcon(file));
          //for drm video file set icon
            if(QuickScanCursorAdapterUtils.getInstance().DRMFileVideoType(mContext,filePath,vHolder)){
                break;
            }
            break;
        case RESOURCE_AUDIO:
            vHolder.fileIcon.setImageResource(FileType.getFileType(mContext).getAudioFileIcon(file));
          //for drm audio file set icon
            if(QuickScanCursorAdapterUtils.getInstance().DRMFileAudioType(mContext,filePath,vHolder)){
                break;
            }
            break;
        case RESOURCE_IMAGE:
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
          //for drm image file set icon
            if(QuickScanCursorAdapterUtils.getInstance().DRMFileImageType(mContext,filePath,vHolder)){
                break;
            }
            break;
        case RESOURCE_DOC:
            vHolder.fileIcon.setImageResource(FileType.getFileType(mContext).getDocFileIcon(file));
            /* SPRD: Add for bug640525. @{ */
            if(QuickScanCursorAdapterUtils.getInstance().DRMFileDefaultType(mContext,filePath,vHolder)){
                break;
            }
            /* @} */
            break;
        case RESOURCE_APK:
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
            /* SPRD: Add for bug640525. @{ */
            if(QuickScanCursorAdapterUtils.getInstance().DRMFileDefaultType(mContext,filePath,vHolder)){
                break;
            }
            /* @} */
        }
        vHolder.fileIcon.setOnClickListener(this);
        if(mIsSelectMode){
            vHolder.selectCb.setVisibility(View.VISIBLE);
            if(isChecked(position)){
                vHolder.selectCb.setChecked(true);
            }else{
                vHolder.selectCb.setChecked(false);
            }
            vHolder.fileIcon.setClickable(false);
        }else{
            vHolder.selectCb.setVisibility(View.GONE);
            vHolder.fileIcon.setClickable(true);
        }
        return view;
    }

    public Uri getFileUri(int position){
        Uri uri = MediaStore.Files.getContentUri("external");
        Cursor cursor = mContext.getContentResolver().query(uri, new String[]{MediaStore.Files.FileColumns._ID}, MediaStore.Files.FileColumns.DATA + "=?", new String[]{getFile(position).getPath()}, null);
        if (cursor == null || cursor.getCount() == 0) {
            if (cursor != null) {
                cursor.close();
            }
            notifyDataSetChanged();
            return null;
        }
        cursor.moveToFirst();
        long id = cursor.getLong(0);
        cursor.close();
        return ContentUris.withAppendedId(uri, id);
    }

    public File getFile(int position){
        return mFileList.get(position);
    }

    @Override
    public void onClick(View v) {
        // TODO Auto-generated method stub
        switch (v.getId()) {
        case R.id.file_icon:
            ((View) v).showContextMenu();
            break;
        }
    }

    public List<File> getFileList(){
        return mFileList;
    }
    private ProgressDialog showWaitDialog(){
        /* SPRD 449592 @{ */
        if(mContext instanceof Activity){
            if(mContext == null || (mContext != null && mContext.isFinishing())){
               Log.i(TAG, "showWaitDialog context error");
               return null;
            }
        }
        /* @} */
        ProgressDialog dialog = new ProgressDialog(mContext);
        dialog.setTitle(R.string.dialog_hint_title);
        dialog.setMessage(mContext.getResources().getString(R.string.dialog_hint_msg));
        dialog.setCancelable(false);
        dialog.show();
        return dialog;
    }
    static class FileLoader extends CursorLoader{
        public FileLoader(Context context, Uri uri, String[] projection, String selection, String sort) {
            super(context, uri, projection, selection, null, sort);
            // TODO Auto-generated constructor stub
        }
    }

    @Override
    public void destroyThread() {
        Log.i(TAG, "destroyThread");
        if (mContext instanceof Activity) {
            if (!mContext.isFinishing() && mWaitDialog != null && mWaitDialog.isShowing()) {
                Log.i(TAG, "destroyThread dismiss");
                mWaitDialog.dismiss();
            }
        }
        mWaitDialog = null;
        super.destroyThread();
    }

    class WaitTimer extends CountDownTimer{
        public WaitTimer(long millisInFuture, long countDownInterval) {
            super(millisInFuture, countDownInterval);
            // TODO Auto-generated constructor stub
        }

        @Override
        public void onTick(long millisUntilFinished) {
            // TODO Auto-generated method stub
        }

        @Override
        public void onFinish() {
            mTimeup = true;
            if(mWaitDialog != null && mWaitDialog.isShowing() && mLoadFinished){
                mWaitDialog.dismiss();
                mListView.setEmptyView(mEmptyView);
                mListView.setSelection(mPosition);
            }
        }
    }
}
