package com.sprd.fileexplorer.loadimage;


import android.content.Context;
import android.graphics.Bitmap;
import android.os.Handler;

import com.sprd.fileexplorer.loadimage.ImageCache.OnImageLoadCompleteListener;
import com.sprd.fileexplorer.util.FileUtil;


public class ImageLoadTask implements Runnable {

    Context mContext = null;
    String mImageUrl = null;
    Bitmap mBitmap = null;
    Handler mHanler = null;
    boolean mIsImage = false;
    long mPriority = -1;
    OnImageLoadCompleteListener mListener = null;
    @SuppressWarnings("unused")
    private ImageLoadTask() {
        // TODO Auto-generated constructor stub
    }

    public ImageLoadTask(Context context, String imageUrl, OnImageLoadCompleteListener listener,
            Handler handler, boolean isImage, long priority) {
        this.mContext = context;
        this.mImageUrl = imageUrl;
        this.mListener = listener;
        this.mIsImage = isImage;
        this.mHanler = handler;
        this.mPriority = priority;
    }
    
    @Override
    public void run() {
        // TODO Auto-generated method stub
        mBitmap = FileUtil.readBitMap(mImageUrl, mIsImage, mContext);
        ImageLoadThreadManager.removeTask(mImageUrl);
        if (null != mBitmap) {
            ImageCache.put(mImageUrl, mBitmap);
            mHanler.post(new Runnable() {
                @Override
                public void run() {
                    // TODO Auto-generated method stub
                    mListener.OnImageLoadComplete(mImageUrl, true, mBitmap);
                }                
            });
        } else {
            mHanler.post(new Runnable() {
                @Override
                public void run() {
                    // TODO Auto-generated method stub
                    mListener.OnImageLoadComplete(mImageUrl, false, mBitmap);
                }                
            });
        }
    }

}

