/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Gallery;
import android.widget.LinearLayout.LayoutParams;

public class MyBalloonStyleAdapter extends BaseAdapter {
    // CID 109373 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private Context mContext;
    private ViewAttributes mAttributes;
    private int[] mStyles;
    private int mLayout;
    private LayoutInflater mInflater;

    public MyBalloonStyleAdapter(Context context, ViewAttributes attributes, int[] styles, int layout) {
        // CID 109373 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mContext = context;
        mAttributes = attributes;
        mStyles = styles;
        mLayout = layout;

        mInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    @Override
    public int getCount() {
        return mStyles.length;
    }

    @Override
    public Object getItem(int position) {
        return mStyles[position];
    }

    @Override
    public long getItemId(int position) {
        return mStyles[position];
    }

    public void setStyleByAttribute(ViewAttributes attributes) {
        mAttributes = attributes;

        notifyDataSetChanged();
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder = null;
        if(convertView != null) {
            holder = (ViewHolder)convertView.getTag();
        } else {
            holder = new ViewHolder();
            convertView = mInflater.inflate(mLayout, parent, false);
            holder.balloonView = (BalloonView) convertView.findViewById(R.id.edit_balloon_style_item);
            convertView.setTag(holder);
            /**
             * FIX BUG: 5388
             * BUG CAUSE: Layout width too big on mdpi devices
             * Date: 2013-11-27
             */
            int layoutWidth = 0;
            if(UiUtils.screenDensity() > 1.0f){
                layoutWidth = UiUtils.screenWidth() * 3 / 4;
            }else{
                layoutWidth = UiUtils.screenWidth() / 2;
            }
            convertView.setLayoutParams(new Gallery.LayoutParams(layoutWidth, LayoutParams.WRAP_CONTENT));
        }
        holder.balloonView.setBalloonStyleByAdapter(mAttributes, mAttributes.getDrawText(), mAttributes.getTextSize(), mStyles[position]);

        return convertView;
    }

    private final class ViewHolder {
        BalloonView balloonView;
    }

}
