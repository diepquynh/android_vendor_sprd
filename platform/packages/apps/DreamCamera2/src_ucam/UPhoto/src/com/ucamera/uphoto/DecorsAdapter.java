/**
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import java.util.Arrays;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.TextView;

import com.ucamera.ucomm.downloadcenter.DownloadCenter;

public class DecorsAdapter extends BaseAdapter {

    private String[] mResNames;
    private LayoutInflater mInflater;
    private Context mContext;
    private String mResType;
    private int mSelectedIndex = -1;

    public DecorsAdapter(Context context, String[] resNames, String resType) {
        mContext = context;
        mResType = resType;
//        if(Const.EXTRA_MOSAIC_VALUE.equals(mResType)) {
//            mResNames = new String[resNames.length -1];
//            System.arraycopy(resNames, 1, mResNames, 0, resNames.length -1);
//        }else {
            mResNames = resNames;
//        }
        mInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    @Override
    public int getCount() {
        if (mResNames != null) {
            return mResNames.length;
        }
        return 0;
    }

    @Override
    public Object getItem(int position) {
        return position;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = null;
        if (convertView == null) {
            view = mInflater.inflate(R.layout.uphoto_decorations_item, parent, false);
        } else {
            view = convertView;
        }
        ImageView resView = (ImageView) view.findViewById(R.id.iv_decorations_item);
        TextView freeResView = (TextView) view.findViewById(R.id.tv_decorations_free);
        if (position < mResNames.length) {
            Drawable drawable = mContext.getResources().getDrawable(R.drawable.edit_bottom_bar_bg);
            int sEffectItemHeight = drawable.getIntrinsicHeight();
            if (sEffectItemHeight > 0) {
                LayoutParams params = (LayoutParams) resView.getLayoutParams();
                params.width = sEffectItemHeight;
                params.height = sEffectItemHeight;
                resView.setLayoutParams(params);
                if(Const.EXTRA_MOSAIC_VALUE.equals(mResType)) {
                    resView.setPadding(0, 0, 0, 0);
                }
            }
            String frameName = mResNames[position];
            Bitmap bitmap = DownloadCenter.thumbNameToThumbBitmap(mContext, frameName, mResType);
            /**
             * FIX BUG: 5157
             * BUG CAUSE: There is not pressed effect on download button;
             * DATE: 2013-10-28
             */
            if(!Const.EXTRA_MOSAIC_VALUE.equals(mResType) && position == 0 && DownloadCenter.RESOURCE_DOWNLOAD_ON) {
                freeResView.setVisibility(View.VISIBLE);
                resView.setImageBitmap(null);
                resView.setBackgroundResource(R.drawable.download_normal);
            } else {
                freeResView.setVisibility(View.GONE);
                resView.setImageBitmap(bitmap);
                if(Const.EXTRA_DECOR_VALUE.equals(mResType)) {
                    resView.setBackgroundResource(R.drawable.decor_menu_item_bg_selector);
                } else if(Const.EXTRA_MOSAIC_VALUE.equals(mResType)) {
                    resView.setBackgroundResource(R.drawable.menu_mosaic_bg_selector);
                }else {
                    resView.setBackgroundResource(R.drawable.menu_item_bg_selector);
                }
            }
            resView.setSelected(mSelectedIndex == position);
        }
        return view;
    }

    public void setSelected(int index) {
        mSelectedIndex = index;
        notifyDataSetChanged();
    }

}
