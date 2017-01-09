package com.dream.camera.settings;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import com.android.camera2.R;

public class DreamUIAdapterMeter extends BaseAdapter {

    CharSequence[] mEntries;
    int[] mImageResourceIds;
    int mSelectedIndex;

    public void setData(CharSequence[] entries, int[] imageResourceIds,
            int selectedIndex) {
        mEntries = entries;
        mImageResourceIds = imageResourceIds;
        mSelectedIndex = selectedIndex;
    }

    @Override
    public int getCount() {
        return mEntries.length;
    }

    @Override
    public Object getItem(int position) {
        return mEntries[position];
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        if(convertView == null){
            convertView = (DreamUIadapterMeterItemUI) LayoutInflater
                    .from(parent.getContext()).inflate(
                            R.layout.select_dialog_singlechoice_material_meter,
                            null);
        }
        DreamUIadapterMeterItemUI item = (DreamUIadapterMeterItemUI) convertView;
        item.setData(mEntries[position], mImageResourceIds[position]);
        if(mSelectedIndex == position){
            item.setChecked(true);
        }
        return item;
    }

    @Override
    public boolean areAllItemsEnabled() {
        return true;
    }

    @Override
    public boolean isEnabled(int position) {
        return true;
    }
}
