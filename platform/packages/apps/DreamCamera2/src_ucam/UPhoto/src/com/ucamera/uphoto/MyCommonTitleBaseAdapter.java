/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import com.ucamera.uphoto.ImageEditControlActivity.LabelItem;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Gallery;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.LinearLayout.LayoutParams;

import java.util.ArrayList;

public class MyCommonTitleBaseAdapter extends BaseAdapter {
    private ArrayList<Object> mLabelList;
    private int mLayout;

    private LayoutInflater mLayoutInflater;
    private boolean mDeleteViewVisiable;

    public MyCommonTitleBaseAdapter(Context context, ArrayList<Object> labelList, int layout, boolean deleteViewVisiable) {
        mLabelList = labelList;
        mLayout = layout;
        mDeleteViewVisiable = deleteViewVisiable;

        mLayoutInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    @Override
    public int getCount() {
        if(mLabelList != null && mLabelList.size() > 0) {
            return mLabelList.size();
        }
        return 0;
    }

    @Override
    public Object getItem(int position) {
        if(mLabelList != null && mLabelList.size() > 0) {
            return mLabelList.get(position);
        }

        return null;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    public void setDeleteViewVisible(boolean visible) {
        mDeleteViewVisiable = visible;
        notifyDataSetChanged();
    }

    public boolean getDeleteViewVisible() {
        return mDeleteViewVisiable;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder;
        if(convertView != null) {
            holder = (ViewHolder) convertView.getTag();
        } else {
            holder = new ViewHolder();
            convertView = mLayoutInflater.inflate(mLayout, parent, false);
            holder.imageView = (ImageView) convertView.findViewById(R.id.icon_selection);
            holder.titleView = (TitleView) convertView.findViewById(R.id.edit_title_common_item);
            holder.textView = (TextView) convertView.findViewById(R.id.text_desc);
            holder.deleteView = (ImageView) convertView.findViewById(R.id.edit_title_common_item_delete);

            convertView.setTag(holder);
            convertView.setLayoutParams(new Gallery.LayoutParams(UiUtils.screenWidth() * 3 / 4, LayoutParams.WRAP_CONTENT));
        }

        Object object = mLabelList.get(position);
        if(object instanceof ViewAttributes) {
            holder.imageView.setVisibility(View.GONE);
            holder.titleView.setVisibility(View.VISIBLE);
            ViewAttributes attributes = (ViewAttributes) object;
            attributes.setTextSize("32");
            holder.titleView.setTitleStyle(attributes, attributes.getDrawText(), true, 0);
            holder.textView.setText((position - 2) + "");
            if(true == mDeleteViewVisiable) {
                holder.deleteView.setVisibility(View.VISIBLE);
            } else {
                holder.deleteView.setVisibility(View.GONE);
            }
        } else if(object instanceof LabelItem){
            holder.titleView.setVisibility(View.GONE);
            holder.imageView.setVisibility(View.VISIBLE);
            LabelItem labelItem = (LabelItem) object;
            holder.imageView.setImageResource(labelItem.labelIconId);
            holder.textView.setText(labelItem.labelTitle);
            holder.deleteView.setVisibility(View.GONE);
        }

        return convertView;
    }

    private final class ViewHolder {
        ImageView imageView;
        TitleView titleView;
        TextView textView;
        ImageView deleteView;
    }
}
