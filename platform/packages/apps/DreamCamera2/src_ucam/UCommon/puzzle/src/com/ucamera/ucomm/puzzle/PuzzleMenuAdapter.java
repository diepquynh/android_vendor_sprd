/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle;

import java.util.ArrayList;

import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import com.ucamera.ucomm.puzzle.R;
import com.ucamera.uphoto.LogUtils;

import android.widget.BaseAdapter;
import android.widget.ImageView;

public class PuzzleMenuAdapter extends BaseAdapter{
    private int mSelected = 0;
    private ArrayList<? extends View> mArrayImageViews = null;
    public PuzzleMenuAdapter(ArrayList<? extends View> imageViews) {
        mArrayImageViews = imageViews;
    }

    public void updateAdapterImages(ArrayList<? extends View> lists){
        mArrayImageViews = lists;
        notifyDataSetChanged();
    }
    @Override
    public int getCount() {
        return mArrayImageViews.size();
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

        convertView = mArrayImageViews.get(position);
        if(convertView instanceof ImageView){
             ImageView imageView = (ImageView)convertView ;
             imageView.setSelected(mSelected == position);
        } else {
            ImageView iv = (ImageView)convertView.findViewById(R.id.iv_free_puzzle_item_bk);
            iv.setSelected(mSelected == position);
        }
        return convertView;
    }

    public void setHighlight(int position) {
        mSelected = position;
        notifyDataSetChanged();
    }

}
