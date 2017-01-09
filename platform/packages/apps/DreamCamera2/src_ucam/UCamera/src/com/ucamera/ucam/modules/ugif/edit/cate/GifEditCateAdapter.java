/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit.cate;

import com.ucamera.ucam.modules.compatible.Models;
import com.ucamera.ucam.modules.utils.Utils;
import android.content.Context;
import android.graphics.Color;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import com.android.camera2.R;
import java.util.ArrayList;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout.LayoutParams;
import android.widget.TextView;

public class GifEditCateAdapter extends BaseAdapter {
    private ArrayList<Integer> mArrayIcons;
    private String[] mCateStrings = null;
    private ArrayList<Integer> mCateValues = null;
    // CID 109294 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private int mSelected = 0;
    private LayoutInflater mInflater = null;

    public GifEditCateAdapter(Context context) {
        mArrayIcons = Utils.getIconIds(context.getResources(), R.array.ugif_edit_tab_icons);
        mCateStrings = Utils.getIconStrings(context.getResources(), R.array.ugif_edit_tab_string);
        mCateValues = Utils.getIconValues(context.getResources(), R.array.ugif_edit_tab_value);
        mInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    @Override
    public int getCount() {
        return mCateStrings.length;
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
        ViewHolder holder = null;
        if(convertView != null) {
            holder = (ViewHolder) convertView.getTag();
        } else {
            holder = new ViewHolder();
            convertView = mInflater.inflate(R.layout.ugif_edit_cate_item, parent, false);
            holder.iconView = (ImageView) convertView.findViewById(R.id.ugif_edit_cate_icon_id);
            holder.textView = (TextView) convertView.findViewById(R.id.ugif_edit_cate_text_id);

            convertView.setTag(holder);
        }
        holder.textView.setText(mCateStrings[position]);
        holder.iconView.setImageResource(mArrayIcons.get(position));
//        if (mSelected == position) {
//            tv.setTextColor(Color.WHITE);
//            tv.setSelected(true);
//        } else {
//            tv.setTextColor(0xff909090);
//            tv.setSelected(false);
//        }
        return convertView;
    }

    public void setSelected(int index) {
//        mSelected = index;
//        notifyDataSetChanged();
    }

    public int getCurrentCate(int position) {
        if(mCateValues == null || position < 0 || position >= mCateValues.size())
            return -1;
        return mCateValues.get(position);
    }

    class ViewHolder {
        ImageView iconView;
        TextView textView;
    }
}
