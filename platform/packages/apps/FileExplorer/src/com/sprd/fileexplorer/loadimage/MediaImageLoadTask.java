package com.sprd.fileexplorer.loadimage;


import java.io.File;

import android.util.Log;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.Handler;
import android.provider.MediaStore;
import android.database.Cursor;
import android.media.MediaScannerConnection.OnScanCompletedListener;

import com.sprd.fileexplorer.loadimage.ImageCache.OnImageLoadCompleteListener;
import com.sprd.fileexplorer.util.FileType;
import com.sprd.fileexplorer.util.FileUtil;

//SPRD: Add for bug562436
import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;

public class MediaImageLoadTask implements Runnable {
    private final static String TAG = "MediaImageLoadTask";
    Context mContext = null;
    String mFilePath = null;
    Handler mHanler = null;
    long mPriority = -1;
    OnImageLoadCompleteListener mListener = null;
    @SuppressWarnings("unused")
    private MediaImageLoadTask() {
    }

    /* SPRD: Add for bug 526161, the 3gpp audio file has the wrong icon. @{ */
    private OnScanCompletedListener mOnScanCompletedListener = new OnScanCompletedListener() {

        @Override
        public void onScanCompleted(String path, Uri uri) {

            Log.d(TAG, "onScanCompleted() ----- start");
            int iconInt = -1;
            Bitmap mBitmap = null;
            FileType localFileType = FileType.getFileType(mContext);
            File file = new File(path);
            String fileSuffix = FileType.getFileType(mContext).getSuffix(file);

            if (localFileType.recordingType(file)) {
                iconInt = localFileType.getAudioFileIcon(fileSuffix);
            } else {
                iconInt = localFileType.getVideoFileIcon(fileSuffix);
            }
            if (iconInt > 0) {
                mBitmap = ((BitmapDrawable) mContext.getResources().getDrawable(iconInt)).getBitmap();
            } else {
                mBitmap = ((BitmapDrawable) mContext.getResources().getDrawable(localFileType.FILE_TYPE_VIDEO))
                        .getBitmap();
            }
            if (mBitmap != null) {
                ImageCache.put(path, mBitmap);
            }
            mHanler.post(new ImageLoadRunnable(path, mBitmap));
        }
    };
    /* @} */

    public MediaImageLoadTask(Context context, String mFilePath, OnImageLoadCompleteListener listener,
            Handler handler, boolean isImage, long priority) {
        this.mContext = context;
        this.mFilePath = mFilePath;
        this.mListener = listener;
        this.mHanler = handler;
        this.mPriority = priority;
    }

    @Override
    public void run() {
        ImageLoadThreadManager.removeTask(mFilePath);
        FileType localFileType = FileType.getFileType(mContext);
        /* SPRD: Modify for bug 526161, the 3gpp audio file has the wrong icon. @{ */
        File file = new File(mFilePath);
        String fileSuffix = FileType.getFileType(mContext).getSuffix(file);
        Bitmap mBitmap = null;
        int iconInt = -1;
        if (".3GPP".equalsIgnoreCase(fileSuffix) || ".mp4".equalsIgnoreCase(fileSuffix)
                || ".3gp".equalsIgnoreCase(fileSuffix) || ".3g2".equalsIgnoreCase(fileSuffix)) {
            //SPRD: Add for bug562436
            if (!isFileExist(mFilePath) && mContext.checkSelfPermission(WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
                Log.d(TAG, "scan the file: " + mFilePath);
                android.media.MediaScannerConnection.scanFile(mContext, new String[] { mFilePath }, null,
                        mOnScanCompletedListener);
                return;
            } else if (localFileType.recordingType(file)) {
                Log.d(TAG, "This is an audio file!");
                iconInt = localFileType.getAudioFileIcon(fileSuffix);
            } else {
                Log.d(TAG, "This is an video file!");
                iconInt = localFileType.getVideoFileIcon(fileSuffix);
            }
            /* @} */
        } else {
            iconInt = localFileType.getVideoFileIcon(fileSuffix);
        }
        if (iconInt > 0) {
            mBitmap = ((BitmapDrawable) mContext.getResources().getDrawable(iconInt)).getBitmap();
        } else {
            mBitmap = ((BitmapDrawable) mContext.getResources().getDrawable(localFileType.FILE_TYPE_VIDEO)).getBitmap();
        }
        /* SPRD: Modify for bug 526161, the 3gpp audio file has the wrong icon. @{ */
        if (mBitmap != null) {
            ImageCache.put(mFilePath, mBitmap);
        }
        mHanler.post(new ImageLoadRunnable(mFilePath, mBitmap));
        /* @} */
    }

    /* SPRD: Modify for bug 526161, the 3gpp audio file has the wrong icon. @{ */
    private boolean isFileExist(String filepath) {
        Cursor cursor = null;
        try {
            cursor = mContext.getContentResolver().query(MediaStore.Files.getContentUri("external"),
                    new String[] { MediaStore.Files.FileColumns.DATA }, MediaStore.Files.FileColumns.DATA + "=?",
                    new String[] { filepath }, null);
            if (cursor != null && cursor.getCount() > 0) {
                return true;
            }
        } catch (Exception ex) {
            Log.i(TAG, "Query database error!", ex);
        } finally {
            if (cursor != null) {
                cursor.close();
                cursor = null;
            }
        }
        return false;
    }

    public class ImageLoadRunnable implements Runnable {
        String mCurrentFilePath = null;
        Bitmap mImageBitmap = null;

        public ImageLoadRunnable (String filepath, Bitmap bitmap){
            this.mCurrentFilePath = filepath;
            this.mImageBitmap = bitmap;
        }

        @Override
        public void run() {
            if (mListener == null) {
                return;
            }
            if (mImageBitmap != null) {
                mListener.OnImageLoadComplete(mCurrentFilePath, true, mImageBitmap);
            } else{
                mListener.OnImageLoadComplete(mCurrentFilePath, false, mImageBitmap);
            }
        }
    }
    /* @} */
}

