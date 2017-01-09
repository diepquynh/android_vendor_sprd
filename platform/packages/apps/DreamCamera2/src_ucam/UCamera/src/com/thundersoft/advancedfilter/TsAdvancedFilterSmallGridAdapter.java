
package com.thundersoft.advancedfilter;

import java.util.List;

import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

public class TsAdvancedFilterSmallGridAdapter extends BaseAdapter {
    private List<View> attachViews;

    public TsAdvancedFilterSmallGridAdapter(List<View> views) {
        attachViews = views;
    }

    public int getCount() {
        if (attachViews == null) {
            return 0;
        }
        return attachViews.size();
    }

    public Object getItem(int position) {
        return null;
    }

    public long getItemId(int position) {
        if (attachViews == null || position >= attachViews.size()) {
            return -1;
        }
        return attachViews.get(position).getId();
    }

    public View getView(int position, View convertView, ViewGroup parent) {
        // View view = convertView;
        // if (convertView == null) {
        // view = attachViews.get(position);
        // }
        View view = attachViews.get(position);
        return view;
    }

    // public void changeGroup(){
    // if(group==0){
    // group=4;
    // }else{
    // group=0;
    // }
    // notifyDataSetChanged();
    // }
}
