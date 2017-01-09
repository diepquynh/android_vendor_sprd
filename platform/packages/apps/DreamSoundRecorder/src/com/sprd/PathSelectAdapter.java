/*
 * Copyright (c) 2012 Thunder Software Technology Co.,Ltd.
 * All Rights Reserved.
 * Thundersoft Confidential and Proprietary.
 * Developed by Thundersoft Engineering team.
 */

package com.sprd.soundrecorder;

import java.io.File;
import java.util.List;

import com.android.soundrecorder.R;
import com.android.soundrecorder.R.drawable;
import com.android.soundrecorder.R.id;
import com.android.soundrecorder.R.layout;
import com.android.soundrecorder.R.string;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Environment;
import android.os.EnvironmentEx;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;
import android.content.res.Resources;

public class PathSelectAdapter extends BaseAdapter{
    private LayoutInflater mInflater;
    private List<String> items;
    private List<String> paths;
    private Bitmap backIcon;
    private Bitmap dirIcon;
    private Resources mResources;

    public PathSelectAdapter(Context context,
            List<String> itemsIn,List<String> pathsIn){
        backIcon = BitmapFactory.decodeResource(context.getResources(),
                R.drawable.back);
        dirIcon = BitmapFactory.decodeResource(context.getResources(),
                R.drawable.folder);
        mInflater = LayoutInflater.from(context);
        items = itemsIn;
        paths = pathsIn;
        mResources = context.getResources();
    }

    public View getView(int position,View convertView,ViewGroup parent){
        ViewHolder holder;
        if(convertView == null){
            convertView = mInflater.inflate(R.layout.pathselect_row, null);
            holder = new ViewHolder();
            holder.text = (TextView) convertView.findViewById(R.id.text);
            holder.icon = (ImageView) convertView.findViewById(R.id.icon);
            convertView.setTag(holder);
        }else{
            holder = (ViewHolder) convertView.getTag();
        }
        File f=new File(paths.get(position).toString());
        if(items.get(position).toString().equals("back")){
            holder.text.setText(R.string.back);
            holder.icon.setImageBitmap(backIcon);
        }else{
            /* SPRD: fix bug 546490 @{ */
            if(f.getPath().equals(EnvironmentEx.getInternalStoragePath().getPath())){
                holder.text.setText(mResources.getString(R.string.internal_storage));
                //SPRD: fix bug512688 Add setContentDescription for path text.
                holder.text.setContentDescription(mResources.getString(R.string.internal_sdcard));
            } else if (f.getPath().equals(EnvironmentEx.getExternalStoragePath().getPath())){
                holder.text.setText(mResources.getString(R.string.external_storage));
                //SPRD: fix bug512688 Add setContentDescription for path text.
                holder.text.setContentDescription(mResources.getString(R.string.external_sdcard));
            } else {
                holder.text.setText(f.getName());
            }
            /* @} */
            holder.icon.setImageBitmap(dirIcon);
        }
        return convertView;
    }

    private class ViewHolder{
        TextView text;
        ImageView icon;
    }

    public long getItemId(int position){
        return position;
    }

    public Object getItem(int position){
        return items.get(position);
    }

    public int getCount(){
        return items.size();
    }

}
