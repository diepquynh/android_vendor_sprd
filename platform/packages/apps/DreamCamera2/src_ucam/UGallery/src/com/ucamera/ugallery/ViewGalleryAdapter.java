/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import com.ucamera.ugallery.gallery.IImage;
import com.ucamera.ugallery.gallery.IImageList;
import com.ucamera.ugallery.gallery.LruCache;
import com.ucamera.ugallery.provider.ThumbnailUtils;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.MediaStore.Images;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.AbsListView.OnScrollListener;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageView;

import com.ucamera.ugallery.R;

public class ViewGalleryAdapter extends BaseAdapter{
    private Context mContext;
    private IImageList mList;
    // CID 109363 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private int mIndex = -1;
    private String mCurrentPath = null;
    private Handler mHandler;
    private ExecutorService executorService;
    private static final int IMAGEVIEW_SET_ITMAP = 0;
    private Map<String, ThumbnailView> mViewMap;
    private ArrayList<Bitmap> mBmps;
    // CID 109363 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private GridView mGridView;
    // CID 109363 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private int mMiddleIndex;
    // CID 109363 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private int mFirstIndex;
    // CID 109363 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private int mLashIndex;
    private boolean mIsFirst = true;
    public ViewGalleryAdapter(Context context, IImageList list, int currentIndex, GridView gridview) {
        this.mContext = context;
        this.mList = list;
        // this.mMiddleIndex = currentIndex;
        // CID 109363 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // this.mFirstIndex = mMiddleIndex -10;
        // CID 109363 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // this.mLashIndex = mMiddleIndex + 10;
        // CID 109363 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // this.mGridView = gridview;
        mViewMap = new HashMap<String, ThumbnailView>();
        this.mBmps = new ArrayList<Bitmap>();
        this.mHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                case IMAGEVIEW_SET_ITMAP:
                    ThumbnailView iv = (ThumbnailView)msg.obj;
                    Bundle bundle = msg.getData();
                    if(bundle != null) {
                        // CID 109129 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
                        // int index = bundle.getInt("index");
//                        if(index == mMiddleIndex) {
//                            mIsFirst = false;
//                            notifyDataSetChanged();
//                        }
                        iv.setImageBitmap((Bitmap)bundle.getParcelable("bitmap"));
                    }
                    break;
                default:
                    break;
                }
            }
        };
        executorService = Executors.newFixedThreadPool(3);
    }
    @Override
    public int getCount() {
        return mList.getCount();
    }

    public void onStop() {
        if(executorService != null) {
            executorService.shutdown();
        }
        if(mBmps != null) {
            for(int i = 0; i < mBmps.size(); i++) {
                Bitmap b = mBmps.get(i);
                if(b !=null && !b.isRecycled()) {
                    b.recycle();
                }
            }
            mBmps.clear();
        }
        System.gc();
        if(mViewMap != null) {
            mViewMap.clear();
        }
    }
    @Override
    public Object getItem(int position) {
        return mList.getImageAt(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        if(convertView == null) {
            convertView = LayoutInflater.from(mContext).inflate(R.layout.view_gallery_item, null);
        }
        ThumbnailView view = (ThumbnailView) convertView.findViewById(R.id.iv_gallery_item);
        IImage image = mList.getImageAt(position);
        if(!mViewMap.containsKey(image.getDataPath())) {
            mViewMap.put(image.getDataPath(), view);
        }
        Bitmap bmp = image.getMicroCachedThumbBitmap();
        if(bmp == null) {
//            if(mIsFirst) {
//                if(position > mFirstIndex && position < mLashIndex)
//                    executorService.submit(new LoadBitmapRunnable(image, view, position));
//            } else {
                executorService.submit(new LoadBitmapRunnable(image, view, position));
        } else {
            if(!mBmps.contains(bmp)){
                mBmps.add(bmp);
            }
        }
        view.setImageBitmap(bmp);
        view.setSelected(image.getDataPath().equals(mCurrentPath));
//        view.setSelect();
        return convertView;
    }
//    public void setSelected(int index) {
//        mIndex = index;
//        if(mImageViewMap != null) {
//            for(int i = 0; i < mImageViewMap.size(); i ++) {
//                ThumbnailView view = mImageViewMap.get(i);
//                if(view != null) {
//                    view.setSelected(mIndex == i);
////                    view.setSelect();
//                }
//            }
//        }
////        notifyDataSetChanged();
//    }
    public void setSelected(String index) {
        mCurrentPath = index;
        if(mViewMap != null) {
            Set<String> mSet = mViewMap.keySet();
            Iterator<String> mIter= mSet.iterator();
            while(mIter.hasNext()) {
                String path = mIter.next();
                ThumbnailView view = mViewMap.get(path);
                if(view != null) {
                    view.setSelected(mCurrentPath.equals(path));
                    view.invalidate();
              }
            }
        }
//        notifyDataSetChanged();
    }
    public void deleteThumb(String path) {
        mViewMap.clear();
        notifyDataSetChanged();
    }
    public void reName(String oldPath, String newPath) {
        if(mViewMap != null) {
            mViewMap.put(newPath, mViewMap.get(oldPath));
            mViewMap.remove(oldPath);
            setSelected(newPath);
        }
    }
    public void rotateThumbnail(IImage image) {
        executorService.submit(new LoadBitmapRunnable(image, mViewMap.get(image.getDataPath()), 0));
    }
    class LoadBitmapRunnable implements Runnable {
        private IImage mImage;
        private ThumbnailView mIv;
        private int mIndex;
        public LoadBitmapRunnable(IImage image, ThumbnailView iv, int index) {
            this.mImage = image;
            this.mIv = iv;
            this.mIndex = index;
        }
        public void run() {
            try{
                Bitmap bitmap = (Bitmap) mImage.microBottomThubmBitmap()[1];
//                Bitmap bitmap = (Bitmap)
                if(!mBmps.contains(bitmap)){
                    mBmps.add(bitmap);
                }
                Message msg = new Message();
                msg.what = IMAGEVIEW_SET_ITMAP;
                msg.obj = mIv;
                Bundle bundle = new Bundle();
                bundle.putParcelable("bitmap", bitmap);
                bundle.putInt("index", mIndex);
                msg.setData(bundle);
                mHandler.sendMessage(msg);
            } catch (Exception e) {
                executorService.submit(this);
            }
        }
    }
}
