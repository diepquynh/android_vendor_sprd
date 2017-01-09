/**
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.BaseAdapter;
import android.widget.Gallery;
import android.widget.GridView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.LinearLayout.LayoutParams;

import java.util.ArrayList;

public class HistoryLabelAdapter extends BaseAdapter {
    private ArrayList<Object> mAttrList;
    private String mItemTag;
    private LayoutInflater mInflater;
    private boolean mDeleteViewVisible = false;

    public HistoryLabelAdapter(Context context, ArrayList<Object> attrList, String itemTag, boolean deleteView) {
        mInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mAttrList = attrList;
        mItemTag = itemTag;
        mDeleteViewVisible = deleteView;
    }

    @Override
    public int getCount() {
        if(mAttrList != null) {
            return mAttrList.size();
        }
        return 0;
    }

    @Override
    public Object getItem(int position) {
        if(mAttrList != null && mAttrList.size() > position) {
            return mAttrList.get(position);
        }
        return null;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    public void setDeleteView(boolean value){
        mDeleteViewVisible = value;
        notifyDataSetChanged();
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder = null;
        if(convertView != null) {
            holder = (ViewHolder)convertView.getTag();
        } else {

            holder = new ViewHolder();
            convertView = mInflater.inflate(R.layout.label_history_item, parent, false);
            holder.balloonView = (BalloonView) convertView.findViewById(R.id.edit_balloon_item);
            holder.labelView = (LabelView) convertView.findViewById(R.id.edit_label_item);
            holder.deleteView = convertView.findViewById(R.id.edit_delete_view);
            holder.layout = (RelativeLayout) convertView.findViewById(R.id.edit_label_layout);
            convertView.setTag(holder);
        }

        if(mDeleteViewVisible){
            holder.deleteView.setVisibility(View.VISIBLE);
        }else{
            holder.deleteView.setVisibility(View.GONE);
        }

        if("Balloon".equals(mItemTag)) {
            holder.labelView.setVisibility(View.GONE);
            holder.layout.setPadding(0, 0, 0, 0);
            Object object = mAttrList.get(position);
            if(object instanceof ViewAttributes){
                holder.balloonView.setVisibility(View.VISIBLE);
                ViewAttributes attributes = (ViewAttributes)object;
                int style = attributes.getBalloonStyle() < 0 ? 0 : attributes.getBalloonStyle();
                holder.balloonView.setBalloonStyleByAdapter(attributes, attributes.getDrawText(), "16", style);
            }
        }else if("Label".equals(mItemTag)) {
            holder.balloonView.setVisibility(View.GONE);
            holder.layout.setPadding(0, 18, 0, 18);
            Object object = mAttrList.get(position);
            if(object instanceof ViewAttributes) {
                holder.labelView.setVisibility(View.VISIBLE);
                ViewAttributes attributes = (ViewAttributes)object;
                convertView.setLayoutParams(new Gallery.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.MATCH_PARENT));
//                convertView.setPadding(0, 0, 20, 0);
                holder.labelView.setLabelStyle(attributes, attributes.getDrawText(), 90);
            }
        }
        return convertView;
    }

    private final class ViewHolder {
        BalloonView balloonView;
        LabelView labelView;
        View deleteView;
        RelativeLayout layout;
    }
}
