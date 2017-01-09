
package com.sprd.engineermode.connectivity.BT;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.util.ArrayList;
import android.util.Log;
import java.util.Collections;
import com.sprd.engineermode.R;

public class BTRXResultAdapter extends BaseAdapter {

    private LayoutInflater inflater = null;
    protected ArrayList<BTRXResult> result = null;
    private ArrayList<BTRXResult> uiResult = null;

    public BTRXResultAdapter(Context context, ArrayList<BTRXResult> result) {
        inflater = LayoutInflater.from(context);
        this.result = result;
    }

    @Override
    public int getCount() {
        return (result != null) ? result.size() : 0;
    }

    @Override
    public Object getItem(int p) {
        if (result != null && p >= 0 && p < result.size()) {
            return result.get(p);
        }
        return null;
    }

    @Override
    public long getItemId(int p) {
        return p;
    }

    class ViewHolder {
        TextView time;
        TextView rssi;
        TextView per;
        TextView ber;
    }

    @Override
    public View getView(int p, View convert, ViewGroup parent) {
        // TODO Auto-generated method stub
        uiResult = (ArrayList<BTRXResult>) result.clone();
        Collections.reverse(uiResult);
        ViewHolder holder;
        if (convert == null) {
            convert = inflater.inflate(R.layout.bt_list_row, null);
            holder = new ViewHolder();
            holder.time = (TextView) convert.findViewById(R.id.time);
            holder.rssi = (TextView) convert.findViewById(R.id.rssi);
            holder.per = (TextView) convert.findViewById(R.id.per);
            holder.ber = (TextView) convert.findViewById(R.id.ber);
            convert.setTag(holder);
        } else {
            holder = (ViewHolder) convert.getTag();
        }

        holder.time.setText(uiResult.get(p).time);
        holder.rssi.setText(uiResult.get(p).rssi);
        holder.per.setText(uiResult.get(p).per);
        holder.ber.setText(uiResult.get(p).ber);
        return convert;
    }
}
