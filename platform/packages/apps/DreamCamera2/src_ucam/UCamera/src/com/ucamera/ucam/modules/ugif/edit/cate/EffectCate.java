/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit.cate;

import java.util.ArrayList;
import com.android.camera2.R;
import com.ucamera.ucam.modules.ugif.edit.callback.ProcessCallback;
import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

public class EffectCate extends GifBasicCate{
    ArrayList<AdapterItem> mArrayList = null;
    public EffectCate(Context context, ProcessCallback callback) {
        super(context, callback);
        mArrayList = EffectCateResource.getInstance(context).getCateResource();
        mSelected = -1;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = null;
        if (convertView == null) {
            view = mInflater.inflate(R.layout.ugif_edit_effect_cate_item, parent, false);
        } else {
            view = convertView;
        }

        ImageView bk = (ImageView)view.findViewById(R.id.ugif_edit_effect_cate_bk);
        TextView tx = (TextView)view.findViewById(R.id.ugif_edit_effect_cate_text);
        if (position < mArrayList.size()) {
            tx.setText(mArrayList.get(position).mTextValue);
            bk.setImageResource(mArrayList.get(position).mTypeValue);
            if (mSelected == position) {
                tx.setSelected(true);
                bk.setSelected(true);
                tx.setTextColor(0xFFFFFFFF);
             } else {
                tx.setSelected(false);
                bk.setSelected(false);
                tx.setTextColor(0x7FFFFFFF);
            }
            tx.setEllipsize(android.text.TextUtils.TruncateAt.MARQUEE);
            tx.setMarqueeRepeatLimit(-1);
            tx.setFocusable(mSelected == position);
            tx.setFocusableInTouchMode(mSelected == position);
        }
        return view;
    }

    @Override
    public int getCount() {
        return mArrayList.size();
    }

    @Override
    public void onItemClick(int position) {
        if (mCallback != null) {
            mCallback.beforeProcess();
        }
        if (mCallback != null ) {
            mCallback.afterProcess(position);
        }
    }

}
