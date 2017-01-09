/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.downloadcenter;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.RelativeLayout;

import com.ucamera.ucomm.downloadcenter.AsyncImageLoader.ImageCallback;

public class ThumbnailBaseAdapter extends BaseAdapter {
    private Context mContext;
    private AsyncImageLoader mAsyncImageLoader;
    // CID 109252 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private GridView mGridView;
    private List<Object> mFileList = new ArrayList<Object>();
    private LayoutInflater mLayoutInflater;
    public Map<Integer, Boolean> mCheckedMap;
    private final int mLayoutId;

    private final ImageCallback mImageCallback = new ImageCallback() {
        public void imageLoaded(Drawable imageDrawable, String imageUrl) {
            notifyDataSetChanged();
        }
    };

    public ThumbnailBaseAdapter(Context c, GridView gridView) {
        this(c,gridView,R.layout.download_center_thumbnail_item);
    }
    public ThumbnailBaseAdapter(Context c, GridView gridView, int itemLayout) {
        this.mContext = c;
        // CID 109252 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // this.mGridView = gridView;
        if (itemLayout <=0 ) {
            this.mLayoutId = R.layout.download_center_thumbnail_item;
        } else {
            this.mLayoutId = itemLayout;
        }
        mCheckedMap = new HashMap<Integer, Boolean>();
        mAsyncImageLoader = new AsyncImageLoader();
        mLayoutInflater = (LayoutInflater)mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    public int getCount() {
        return mFileList.size();
    }

    public Object getItem(int position) {
        if(position < 0 || position >= mFileList.size()){
            return null;
        }
        return mFileList.get(position);
    }

    public long getItemId(int position) {
        return position;
    }

    public List<Object> getItems() {
        return mFileList;
    }

    public Object getItemAt(int i) {
        return mFileList.get(i);
    }

    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder = null;
        if (convertView == null) {
            holder = new ViewHolder();
            convertView = mLayoutInflater.inflate(mLayoutId, parent, false);
            holder.layout = (RelativeLayout) convertView.findViewById(R.id.image_thumbnail_layout);
            holder.thumbnail = (ImageView) convertView.findViewById(R.id.image_thumbnail);
            holder.checkebox = (CheckBox) convertView.findViewById(R.id.checkbox);
            convertView.setTag(holder);
        } else {
            holder = (ViewHolder) convertView.getTag();
        }

        if(position >= mFileList.size()) {
            return null;
        }

        String imagePath = null;
        Object object = mFileList.get(position);
        if(object instanceof String) {
            imagePath = (String)object;
        } else if(object instanceof ThumbnailMode) {
            imagePath = ((ThumbnailMode)object).getThumnailUrl();
        }
        holder.checkebox.setChecked(mCheckedMap.get(position));

//        holder.thumbnail.setTag(imagePath);
        Drawable cachedImage = mAsyncImageLoader.loadDrawable(imagePath, mImageCallback);

        if (cachedImage != null) {
            holder.layout.setVisibility(View.VISIBLE);
            holder.thumbnail.setImageDrawable(cachedImage);
        }

        return convertView;
    }

    public final class ViewHolder {
        public RelativeLayout layout;
        public ImageView thumbnail;
        public CheckBox checkebox;
    }

    public void addItems(List<Object> list) {
        mFileList.addAll(list);
        int size = mFileList.size();
        for (int i = 0; i < size; i++) {
            mCheckedMap.put(i, false);
        }
    }

    public void clear() {
        stopImageLoader();
        mFileList.clear();
    }

    public void stopImageLoader() {
        if (mAsyncImageLoader != null) {
            mAsyncImageLoader.shutdown();
        }
    }
}
