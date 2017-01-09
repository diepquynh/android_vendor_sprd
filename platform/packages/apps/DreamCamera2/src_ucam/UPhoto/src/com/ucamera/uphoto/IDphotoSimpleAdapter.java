/**
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.RelativeLayout.LayoutParams;

import java.util.List;
import java.util.Map;

public class IDphotoSimpleAdapter extends BaseAdapter {
    private int[] mResource;
    private LayoutInflater mInflater;
    private Context mContext;
    private int mSelectedIndex = 0;
    public IDphotoSimpleAdapter(Context context, int[] res) {
        mContext = context;
        mResource = res;
        mInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = null;
        if (convertView == null) {
            view = mInflater.inflate(R.layout.uphoto_decorations_item, parent,
                    false);
        } else {
            view = convertView;
        }
        ImageView resView = (ImageView) view.findViewById(R.id.iv_decorations_item);
        Drawable drawable = mContext.getResources().getDrawable( R.drawable.edit_bottom_bar_bg);
        int sEffectItemHeight = drawable.getIntrinsicHeight();
        if (sEffectItemHeight > 0) {
            LayoutParams params = (LayoutParams) resView.getLayoutParams();
            params.width = sEffectItemHeight;
            params.height = sEffectItemHeight;
            resView.setLayoutParams(params);
            resView.setPadding(0, 0, 0, 0);
        }
        resView.setImageResource(mResource[position]);
        resView.setBackgroundResource(R.drawable.menu_background_bg_selector);
        resView.setSelected(mSelectedIndex == position);
        return view;
    }

    public void setSelected(int index) {
        mSelectedIndex = index;
        notifyDataSetChanged();
    }
    public int getCount() {
        return mResource.length;
    }

    public Object getItem(int position) {
        return mResource[position];
    }

    public long getItemId(int position) {
        return position;
    }


}
