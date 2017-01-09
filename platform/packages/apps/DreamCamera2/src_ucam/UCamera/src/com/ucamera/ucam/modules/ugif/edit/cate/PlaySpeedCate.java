/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit.cate;

import com.ucamera.ucam.modules.ugif.edit.callback.ProcessCallback;
import com.ucamera.ucam.modules.utils.Utils;
import com.android.camera2.R;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

public class PlaySpeedCate extends GifBasicCate{
    private String[] mTitleRes;
    public PlaySpeedCate(Context context, ProcessCallback callback) {
        super(context, callback);
        mArrayRes = Utils.getIconIds(context.getResources(), R.array.ugif_edit_playspeed_icons);
        mTitleRes = Utils.getIconStrings(context.getResources(), R.array.ugif_edit_playspeed_titles);
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
        holder.textView.setText(mTitleRes[position]);
        holder.iconView.setImageResource(mArrayRes.get(position));
        holder.iconView.setSelected(mSelected == position);

        return convertView;
    }

    class ViewHolder {
        ImageView iconView;
        TextView textView;
    }

    @Override
    public void onItemClick(int position) {
        if (mCallback != null) {
            mCallback.beforeProcess();
        }

        if (mCallback != null) {
            mCallback.afterProcess(position);
        }
    }
}
