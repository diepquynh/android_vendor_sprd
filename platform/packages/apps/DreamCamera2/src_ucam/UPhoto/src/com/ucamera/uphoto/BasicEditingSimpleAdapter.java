/**
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.SimpleAdapter;
import android.widget.TextView;

import java.util.List;
import java.util.Map;

public class BasicEditingSimpleAdapter extends SimpleAdapter {
    private int[] mTo;
    private String[] mFrom;
    private List<? extends Map<String, ?>> mData;
    private int mResource;
    private LayoutInflater mInflater;

    private int mSelected = -1;

    public BasicEditingSimpleAdapter(Context context, List<? extends Map<String, ?>> data,
            int resource, String[] from, int[] to) {
        super(context, data, resource, from, to);
        mData = data;
        mResource = resource;
        mFrom = from;
        mTo = to;
        mInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    public int getCount() {
        return mData.size();
    }

    public Object getItem(int position) {
        return mData.get(position);
    }

    public long getItemId(int position) {
        return position;
    }

    public View getView(int position, View convertView, ViewGroup parent) {
        return createViewFromResource(position, convertView, parent, mResource);
    }

    private View createViewFromResource(int position, View convertView, ViewGroup parent,
            int resource) {
        View v;
        if (convertView == null) {
            v = mInflater.inflate(resource, parent, false);
        } else {
            v = convertView;
        }

        bindView(position, v);

        return v;
    }

    private void bindView(int position, View view) {
        final Map dataSet = mData.get(position);
        if (dataSet == null) {
            return;
        }

        final String[] from = mFrom;
        final int[] to = mTo;
        final int count = to.length;

        for (int i = 0; i < count; i++) {
            final View v = view.findViewById(to[i]);
            if (v != null) {
                final Object data = dataSet.get(from[i]);
                String text = data == null ? "" : data.toString();
                if (text == null) {
                    text = "";
                }
                if (v instanceof TextView) {
                    TextView textView = (TextView) v;
                    textView.setText(text);
                } else if (v instanceof ImageView) {
                    ImageView imageView = (ImageView) v;
                    if (data instanceof Integer) {
                        setViewImage(imageView, (Integer) data);
                    } else {
                        setViewImage(imageView, text);
                    }
                    setIconSelected(imageView, position);
                } else {
                    throw new IllegalStateException(v.getClass().getName() + " is not a "
                            + " view that can be bounds by this SimpleAdapter");
                }
            }
        }
    }

    private void setIconSelected(ImageView icon, int position) {
        if (mSelected == position) {
            icon.setSelected(true);
        }
        else {
            icon.setSelected(false);
        }
    }

    public void setHighlight(int position) {
        mSelected = position;
        notifyDataSetChanged();
    }

}
