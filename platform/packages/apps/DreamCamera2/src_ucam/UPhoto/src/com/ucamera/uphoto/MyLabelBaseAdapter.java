/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.TextView;
import com.ucamera.ucomm.downloadcenter.DownloadCenter;

import java.util.ArrayList;

public class MyLabelBaseAdapter extends BaseAdapter {
    private Context mContext;
    private ArrayList<ViewAttributes> mAttrList;
    private String[] mFontArray;
    private int mLayout;
    private String mItemTag; //color/font
    private int mIndex; //0:title; 1: balloon; 2: label

    private LayoutInflater mInflater;
    private String mFont;
    private static final String FONT_DISPLAY_CN = ".CN";
    private static final String FONT_DISPLAY_JA = ".JA";

    public MyLabelBaseAdapter(Context context, ArrayList<ViewAttributes> attrList, String[] fontArray, int layout, String itemTag) {
        mContext = context;
        mAttrList = attrList;
        mFontArray = fontArray;
        mLayout = layout;
        mItemTag = itemTag;

        mInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    public MyLabelBaseAdapter(Context context, ArrayList<ViewAttributes> attrList, String[] fontArray, int layout, String itemTag, int index) {
        this(context, attrList, fontArray, layout, itemTag);

        mIndex = index;
    }

    @Override
    public int getCount() {
        if(mAttrList != null && mAttrList.size() > 0) {
            return mAttrList.size();
        } else if(mFontArray != null && mFontArray.length > 0) {
            return mFontArray.length;
        } else {
            return 0;
        }
    }

    @Override
    public Object getItem(int position) {
        if(mAttrList != null && mAttrList.size() > 0) {
            return mAttrList.get(position);
        }  else if(mFontArray != null && mFontArray.length > 0) {
            return mFontArray[position];
        } else {
            return null;
        }
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    public void setFontArray(String[] fonts) {
        mFontArray = fonts;
        notifyDataSetChanged();
    }

    public void setFont(String font) {
        mFont = font;
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
            holder.fontTextView = (TextView) convertView.findViewById(R.id.edit_font_item);
            holder.titleView = (TitleView) convertView.findViewById(R.id.edit_color_title_item);
            holder.balloonView = (BalloonView) convertView.findViewById(R.id.edit_color_balloon_item);
            holder.labelView = (LabelView) convertView.findViewById(R.id.edit_label_item);
            holder.downloadView = convertView.findViewById(R.id.edit_download_item);
            holder.downloadFreeView = (TextView) convertView.findViewById(R.id.edit_font_free);

            convertView.setTag(holder);
        }

        if("Color".equals(mItemTag)) {
            holder.fontTextView.setVisibility(View.GONE);
            holder.labelView.setVisibility(View.GONE);
            ViewAttributes attributes = mAttrList.get(position);
            if(mFont != null) {
                attributes.setFont(mFont);
            }

            if(mIndex == 0/* || mIndex == 2*/) {
                holder.balloonView.setVisibility(View.GONE);
                holder.labelView.setVisibility(View.GONE);
                holder.downloadView.setVisibility(View.GONE);
                holder.titleView.setVisibility(View.VISIBLE);
                holder.titleView.setTitleStyle(attributes, "label", false, mIndex);
            } else if(mIndex == 1) {
                holder.titleView.setVisibility(View.GONE);
                holder.labelView.setVisibility(View.GONE);
                holder.downloadView.setVisibility(View.GONE);
                holder.balloonView.setVisibility(View.VISIBLE);
                holder.balloonView.setBalloonStyleByAdapter(attributes, "Color", "16", ImageEditConstants.BALLOON_STYLE_OVAL);
            }
        } else if("Font".equals(mItemTag)) {
            if (position == 0 && DownloadCenter.RESOURCE_DOWNLOAD_ON) {
                // download btn is the last one
                holder.showDownload();
                holder.downloadFreeView.setVisibility(View.VISIBLE);
            } else {
                holder.downloadFreeView.setVisibility(View.GONE);
                holder.titleView.setVisibility(View.GONE);
                holder.balloonView.setVisibility(View.GONE);
                holder.downloadView.setVisibility(View.GONE);
                holder.fontTextView.setVisibility(View.VISIBLE);
                holder.fontTextView.setTypeface(FontResource.createTypeface(mContext, mFontArray[position]));
                if(mFontArray[position].indexOf(FONT_DISPLAY_CN) > 0) {
                    holder.fontTextView.setText("字体");
                } else if(mFontArray[position].indexOf(FONT_DISPLAY_JA) > 0) {
                    holder.fontTextView.setText("ﾌｫﾝﾄ");
                }else {
                    holder.fontTextView.setText("Font");
                }
            }
        } else if("Label".equals(mItemTag)) {
            ViewAttributes attributes = mAttrList.get(position);
            if(mFont != null) {
                attributes.setFont(mFont);
            }
            holder.fontTextView.setVisibility(View.GONE);
            holder.titleView.setVisibility(View.GONE);
            holder.balloonView.setVisibility(View.GONE);
            holder.downloadView.setVisibility(View.GONE);
            holder.labelView.setVisibility(View.VISIBLE);
            convertView.setLayoutParams(new AbsListView.LayoutParams(GridView.LayoutParams.MATCH_PARENT, GridView.LayoutParams.WRAP_CONTENT));
            holder.labelView.setLabelStyle(attributes, "", 0);
        }

        return convertView;
    }

    private final class ViewHolder {
        TextView fontTextView;
        TitleView titleView;
        BalloonView balloonView;
        LabelView labelView;
        View downloadView;
        TextView downloadFreeView;
        void showDownload() {
            fontTextView.setVisibility(View.GONE);
            titleView.setVisibility(View.GONE);
            balloonView.setVisibility(View.GONE);
            labelView.setVisibility(View.GONE);
            downloadView.setVisibility(View.VISIBLE);
        }
    }
}
