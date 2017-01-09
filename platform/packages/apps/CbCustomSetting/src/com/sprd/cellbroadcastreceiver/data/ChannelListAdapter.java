package com.sprd.cellbroadcastreceiver.data;

import java.util.ArrayList;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.sprd.cellbroadcastreceiver.R;
import com.sprd.cellbroadcastreceiver.data.itf.IModify;
import com.sprd.cellbroadcastreceiver.ui.ChannelItemView;

public class ChannelListAdapter extends BaseAdapter {

    private String TAG = "ChannelListAdapter";

    private Context mContext;
    private ArrayList<ChannelItemData> mChannelList;

    public ArrayList<ChannelItemData> getChannelList() {
        return mChannelList;
    }

    private Context getContext() {
        return mContext;
    }

    public ChannelListAdapter(Context context,
            ArrayList<ChannelItemData> channelmgr) {
        mChannelList = channelmgr;
        mContext = context;
    }

    @Override
    public int getCount() {
        return getChannelList().size();
    }

    @Override
    public Object getItem(int position) {
        return getChannelList().get(position);
    }

    @Override
    public long getItemId(int position) {
        return (long) position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {

        ChannelItemView cView = (ChannelItemView) LayoutInflater.from(
                getContext()).inflate(R.layout.cell_broadcast_channel_item,
                null);

        cView.setTag(getItem(position));
        cView.init();

        return cView;
    }

}
