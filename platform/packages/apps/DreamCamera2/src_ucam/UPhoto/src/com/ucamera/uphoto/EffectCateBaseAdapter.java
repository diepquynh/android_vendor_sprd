/**
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import java.util.ArrayList;

import com.ucamera.uphoto.R;
import com.ucamera.uphoto.EffectTypeResource.EffectItem;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.util.Log;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.RelativeLayout.LayoutParams;

public class EffectCateBaseAdapter extends BaseAdapter{
    private ArrayList<EffectItem> mArrayList;
    private LayoutInflater mInflater = null;
    private int mSelected = -1;
    private Context mContext;
    int mEffectItemHeight;

    public EffectCateBaseAdapter(Context context, ArrayList<EffectItem> arrayList) {
        mArrayList = arrayList;
        mContext = context;
        mInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        //get height of bottom bar
        Drawable drawable = mContext.getResources().getDrawable(R.drawable.edit_bottom_bar_bg);
        mEffectItemHeight = drawable.getIntrinsicHeight();
    }

    @Override
    public int getCount() {
        return mArrayList.size();
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
        View view = null;
        int size = mArrayList.size();
        if(Build.VERSION.SDK_INT >= 14) {
            if (convertView == null) {
                view = mInflater.inflate(R.layout.edit_effect_cate_item, parent, false);
            } else {
                view = convertView;
            }
        } else {
            view = mInflater.inflate(R.layout.edit_effect_cate_item, parent, false);
        }

        ImageView bk = (ImageView)view.findViewById(R.id.icon_selection);
        TextView tx = (TextView)view.findViewById(R.id.text_desc);

        if (position < size) {
            EffectItem item = mArrayList.get(position);
            int iconId = mContext.getResources().getIdentifier(item.rIconName, "drawable", mContext.getPackageName());
            if(position != size-1) {
                int txtId = mContext.getResources().getIdentifier(item.mTextValue, "string", mContext.getPackageName());
                tx.setText(txtId);
            }else{
                bk.setBackgroundDrawable(null);
                tx.setText(null);
            }
            bk.setImageResource(iconId);
            if(mEffectItemHeight > 0) {
                LayoutParams params = (LayoutParams) bk.getLayoutParams();
                params.height = mEffectItemHeight;
                params.width  = mEffectItemHeight;
                bk.setLayoutParams(params);
            }
            bk.setSelected(mSelected == position);
            tx.setSelected(mSelected == position);
            tx.setEllipsize(android.text.TextUtils.TruncateAt.MARQUEE);
            tx.setMarqueeRepeatLimit(-1);
            tx.setFocusable(mSelected == position);
            tx.setFocusableInTouchMode(mSelected == position);
            tx.setTextColor(mSelected == position ? 0xFFFFFFFF : 0x7FFFFFFF);
        }
        return view;
    }

    public void setHighlight(int position) {
        mSelected = position;
        notifyDataSetChanged();
    }

    public String getType() {
        if (mArrayList == null || mArrayList.size() <= 0) {
            return null;
        }
        return mArrayList.get(mSelected).rIconName;
    }

}
