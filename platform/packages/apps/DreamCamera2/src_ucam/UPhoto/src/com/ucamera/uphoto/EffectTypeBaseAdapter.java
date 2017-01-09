/*
 *   Copyright (C) 2010,2012 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Color;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

import com.ucamera.uphoto.EffectTypeResource.EffectItem;

import java.util.ArrayList;

public class EffectTypeBaseAdapter extends BaseAdapter {
    private int mSelected = 0;
    private Context mContext;
    private ArrayList<EffectItem> mEffectItemList;

    public EffectTypeBaseAdapter(Context context, ArrayList<EffectItem> effectItemList) {
        mContext = context;
        mEffectItemList = effectItemList;
    }

    public int getCount() {
        if(mEffectItemList != null && mEffectItemList.size() > 0) {
            return mEffectItemList.size();
        } else {
            return 0;
        }
    }

    @Override
    public Object getItem(int position) {
        if(mEffectItemList != null && mEffectItemList.size() > 0) {
            return mEffectItemList.get(position);
        } else {
            return null;
        }
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        TextView textView;
        if (convertView == null) {
            textView = new TextView(mContext);
            textView.setTextColor(Color.WHITE);
            textView.setTextSize(24);
            textView.setGravity(Gravity.CENTER);
        } else {
            textView = (TextView) convertView;
        }

        if (position < mEffectItemList.size()) {
            textView.setText(mEffectItemList.get(position).mTextValue);
            if (mSelected == position) {
                textView.setSelected(true);
            } else {
                textView.setTextSize(20);
            }
        }
        return textView;
    }

    public void setHighlight(int position) {
        mSelected = position;
        notifyDataSetChanged();
    }

}
