
package com.sprd.contacts.common.group;

import java.util.ArrayList;
import java.util.HashMap;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.SimpleAdapter;

import com.android.contacts.common.util.ViewUtil;
import com.android.contacts.common.ContactPhotoManager;

public class GroupPhotoAdapter extends SimpleAdapter {
    private static final String TAG = GroupPhotoAdapter.class.getSimpleName();

    private final Context mContext;
    private final int mResource;
    private final int[] mImageViewId;
    private final String[] mGroupMemPhoto;
    private ArrayList<HashMap<String, Object>> mData;

    public GroupPhotoAdapter(Context context, ArrayList<HashMap<String, Object>> data,
            int resource, String[] from, int[] to) {
        super(context, data, resource, from, to);
        mContext = context;
        mResource = resource;
        mGroupMemPhoto = from;
        mImageViewId = to;
        mData = data;
        mPhotoManager = ContactPhotoManager.getInstance(mContext);
}

    @Override
    public int getCount() {
        return mData == null ? 0 : mData.size();
    }

    @Override
    public Object getItem(int position) {
        return null;
    }

    @Override
    public View getView(final int position, View convertView, ViewGroup parent) {
        View view;
        if (convertView == null) {
           view =LayoutInflater.from(mContext).inflate(mResource, parent, false);
        } else {
           view = convertView;
        }
        ImageView imageView = (ImageView) view.findViewById(mImageViewId[0]);
        Object ob = mData.get(position).get(mGroupMemPhoto[0]);
        if (ob instanceof Uri) {
            Uri uri = (Uri) ob;
            mPhotoManager.loadPhoto(imageView, uri,  ViewUtil.getConstantPreLayoutWidth(imageView),
                    false, true, null);
        } else {
            Integer re = (Integer) ob;
            imageView.setImageResource(re);
        }
        return view;
    }

    private ContactPhotoManager mPhotoManager = null;
}
