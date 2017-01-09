/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.ucamera.uphoto.preference;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.ucamera.uphoto.R;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class FileListAdapter extends BaseAdapter {
    private Context mContext;
    private List<String> mFileList = new ArrayList<String>();
    private LayoutInflater mLayoutInflater;

    public FileListAdapter(Context context) {
        mContext = context;
        mLayoutInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    public int getCount() {
        return mFileList.size();
    }

    public Object getItem(int position) {
        if(position < 0 || position >= mFileList.size()){
            return null;
        }
        return mFileList.get(position);
    }

    public long getItemId(int position) {
        return position;
    }

    public List<String> getItems() {
        return mFileList;
    }

    public Object getItemAt(int i) {
        return mFileList.get(i);
    }

    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder = null;
        if (convertView == null) {
            holder = new ViewHolder();
            convertView = mLayoutInflater.inflate(R.layout.file_item, parent, false);
            holder.icon = (ImageView) convertView.findViewById(R.id.iv_folder_icon);
            holder.textview = (TextView) convertView.findViewById(R.id.tv_filename);
            convertView.setTag(holder);
        } else {
            holder = (ViewHolder) convertView.getTag();
        }

        if (position >= mFileList.size()) {
            return null;
        }
        String filePath = mFileList.get(position);
        File file = new File(filePath);
        holder.icon.setImageResource(R.drawable.folder_icon);
        holder.textview.setText(file.getName());
        return convertView;
    }

    public final class ViewHolder {
        public ImageView icon;
        public TextView textview;
    }

    /* BUG FIX: 524
     * BUG CASUE: data set changed but not notify
     * FIX COMMENT: when dataset changed notify changed
     * DATE: 2012-06-18
     */
    public void addItems(List<String> list) {
        mFileList.addAll(list);
        notifyDataSetChanged();
    }

    public void clear() {
        mFileList.clear();
        notifyDataSetChanged();
    }
}
