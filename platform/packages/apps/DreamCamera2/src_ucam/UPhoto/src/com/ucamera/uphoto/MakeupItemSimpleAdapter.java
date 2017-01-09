/**
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import java.util.List;
import java.util.Map;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.SimpleAdapter;
import android.widget.TextView;

public class MakeupItemSimpleAdapter extends SimpleAdapter {
    private Context mContext;
    private List<? extends Map<String, ?>> mData;
    private LayoutInflater mLayoutInflater;
    private int mResource;
    private String[] mFrom;
    private int[] mTo;
    private int mSelected = -1;

    public MakeupItemSimpleAdapter(Context context,
            List<? extends Map<String, ?>> data, int resource, String[] from,
            int[] to) {
        super(context, data, resource, from, to);
        mContext = context;
        mData = data;
        mLayoutInflater = LayoutInflater.from(mContext);
        mResource = resource;
        mFrom = from;
        mTo = to;
    }

    @Override
    public int getCount() {
        if (mData != null) {
            return mData.size();
        }
        return 0;
    }

    @Override
    public Object getItem(int position) {
        if (mData != null) {
            return mData.get(position);
        }
        return null;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        if (convertView == null) {
            convertView = mLayoutInflater.inflate(mResource, parent, false);
        }
        bindView(convertView, position);
        return convertView;
    }

    private void bindView(View view, int position) {
        Map<String, ?> mapData = mData.get(position);
        if (mapData == null) {
            return;
        }
        final int viewCount = mTo.length;
        for (int i = 0; i < viewCount; i++) {
            View singleView = view.findViewById(mTo[i]);
            if (singleView != null) {
                Object data = mapData.get(mFrom[i]);
                String text = data == null ? " " : data.toString();
                if (singleView instanceof TextView) {
                    TextView tv = (TextView) singleView;
                    setViewText(tv, text);
                    if (mSelected == position) {
                        tv.setSelected(true);
                        tv.setFocusable(true);
                        tv.setFocusableInTouchMode(true);
                    } else {
                        tv.setSelected(false);
                        tv.setFocusable(false);
                        tv.setFocusableInTouchMode(false);
                    }
                } else if (singleView instanceof ImageView) {
                    ImageView iv = (ImageView) singleView;
                    if (data instanceof Integer) {
                        setViewImage(iv, (Integer) data);
                    } else {
                        setViewImage(iv, text);
                    }
                    setImageSelected(iv, position);
                }
            }
        }
    }

    private void setImageSelected(ImageView iv, int position) {
        if (mSelected == position) {
            iv.setSelected(true);
        } else {
            iv.setSelected(false);
        }
    }

    public void setHighLight(int position) {
        mSelected = position;
        notifyDataSetChanged();
    }
}