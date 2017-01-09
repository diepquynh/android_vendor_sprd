package com.sprdroid.note;

import java.util.List;
import android.util.Log;
import com.sprdroid.note.R;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

public class AlarmSettingAdapter extends BaseAdapter {
    private List<String> items;
    private List<String> values;
    private LayoutInflater inflater;
    private static final String TAG = "AlarmSettingAdapter";

    public AlarmSettingAdapter(Context context, List<String> items,
            List<String> values) {
        inflater = LayoutInflater.from(context);
        this.items = items;
        this.values = values;
    }

    @Override
    public int getCount() {
        return items.size();
    }

    @Override
    public Object getItem(int position) {
        return items.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View view, ViewGroup group) {
        ViewHolder holder = new ViewHolder();
        if (view == null) {
            view = inflater.inflate(R.layout.alarm_setting_item_layout, null);
            holder.item = (TextView) view.findViewById(R.id.alarm_item);
            holder.item_value = (TextView) view
                    .findViewById(R.id.alarm_item_value);
            view.setTag(holder);
        } else {
            holder = (ViewHolder) view.getTag();
        }
        Log.e(TAG, "getView: " + "****************************************");
        String title = items.get(position);
        //title = title.length() > 15 ? title.substring(0, 15) + "..." : title;
        holder.item.setText(title);
        holder.item_value.setText(values.get(position));
        return view;
    }

    private class ViewHolder {
        private TextView item;
        private TextView item_value;
    }
}
