/**
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucam.modules.uscenery;

import android.content.Context;
import android.graphics.Bitmap;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.ucamera.ucam.modules.utils.Constants;
import com.ucamera.ucam.modules.utils.DownloadCenter;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;

import com.android.camera2.R;

public class SceneryAdapter extends BaseAdapter {
    private String[] mFrameNames;
    private LayoutInflater mInflater;
    private Context mContext;
    private int mSelected = 0;

    public SceneryAdapter(Context context, String[] frameNames) {
        mContext = context;
        mFrameNames = frameNames;
        mInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    public void updateItems(String[] items){
        mFrameNames = items;
        notifyDataSetChanged();
    }

    @Override
    public int getCount() {
        if (mFrameNames != null) {
            return mFrameNames.length;
        }
        return 0;
    }

    @Override
    public Object getItem(int position) {
        if(position < getCount()){
            return mFrameNames[position];
        }
        return null;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = null;
        if (convertView == null) {
            view = mInflater.inflate(R.layout.ucam_menu_scenery_item, parent, false);
        } else {
            view = convertView;
        }
        ImageView sceneryView = (ImageView) view.findViewById(R.id.iv_scenery_item_bk);
        if (position < mFrameNames.length) {
            String frameName = mFrameNames[position];
            Bitmap bitmap = DownloadCenter.assetsThumbNameToThumbBitmap(mContext, frameName,
                    Constants.EXTRA_FRAME_THUMB_VALUE);
            /*if(position == 0 && DownloadCenter.RESOURCE_DOWNLOAD_ON) {
                sceneryView.setImageBitmap(null);
                tvFree.setVisibility(View.VISIBLE);
                sceneryView.setBackgroundResource(R.drawable.bg_download_normal);
            } else*/ {
                sceneryView.setImageBitmap(bitmap);
                sceneryView.setBackgroundResource(R.drawable.ucam_scenery_menu_item_selector);
            }
            if (mSelected == position) {
                sceneryView.setSelected(true);
            } else {
                sceneryView.setSelected(false);
            }
        }
        return view;
    }

    public void setHighlight(int position) {
        mSelected = position;
        notifyDataSetChanged();
    }

    public int getIndexOf(String item) {
        if(mFrameNames == null || item == null) return 0;
        for(int i=0; i < mFrameNames.length; i++) {
            if(item.equals(mFrameNames[i])) {
                return i;
            }
        }
        return 0;
    }

}
