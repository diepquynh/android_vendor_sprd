/*
 * Copyright (C) 2013 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

public class CropRatioAdapter extends BaseAdapter{
    private Context mContext;
    private LayoutInflater mLayoutInflater;
    private int[] mResources;
    private int[][] mRatios;
    public CropRatioAdapter(Context context, int[] resources, int[][] ratios) {
        this.mContext = context;
        this.mResources = resources;
        this.mRatios = ratios;
        mLayoutInflater = LayoutInflater.from(this.mContext);
    }
    @Override
    public int getCount() {
        return mResources.length;
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
        Holder holder;
        if(convertView == null) {
            holder = new Holder();
            convertView = mLayoutInflater.inflate(R.layout.ratio_item, null);
            holder.iv = (ImageView) convertView.findViewById(R.id.iv_cut_ratio);
            holder.tv = (TextView) convertView.findViewById(R.id.tv_cut_ratio);
            convertView.setTag(holder);
        }
        holder = (Holder)convertView.getTag();
        holder.iv.setBackgroundResource(mResources[position]);
        if(position == 7) {
            holder.tv.setText(R.string.cut_ratio_btn);
        } else {
            holder.tv.setText(mRatios[position][0] + ":" + mRatios[position][1]);
        }
        if(position == 6 || position == 8) {
            holder.iv.setVisibility(View.INVISIBLE);
            holder.tv.setVisibility(View.INVISIBLE);
        }

        return convertView;
    }
    static class Holder {
        ImageView iv;
        TextView tv;
    }
}