
package com.sprd.contacts.group;

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

public class GroupPhotoAdapterSprd extends SimpleAdapter {
    private static final String TAG = GroupPhotoAdapterSprd.class.getSimpleName();

    private final Context mContext;
    public final ArrayList<Bitmap> mBitmapList;
    private final int mResource;
    private final int[] mImageViewId;

    public GroupPhotoAdapterSprd(Context context, ArrayList<HashMap<String, Object>> data,
            int resource, String[] from, int[] to) {
        super(context, data, resource, from, to);
        mContext = context;
        mResource = resource;
        mImageViewId = to;
        mBitmapList = new ArrayList<Bitmap>();
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inSampleSize = 2;
        for (HashMap<String, Object> hashMap : data) {
            Object ob = hashMap.get(from[0]);
            if (ob instanceof Uri) {
                Uri uri = (Uri) ob;
                try {
                    Bitmap bitmap = BitmapFactory.decodeStream(mContext.getContentResolver()
                            .openInputStream(uri), null, options);
                    mBitmapList.add(bitmap);
                } catch (Exception e) {
                    Log.d(TAG, "Bitmap decodeStream from Uri failed!");
                }
            } else {
                Integer re = (Integer) ob;
                try {
                    Bitmap bitmap = BitmapFactory.decodeStream(
                            mContext.getResources().openRawResource(re), null, options);
                    mBitmapList.add(bitmap);
                } catch (Exception e) {
                    Log.d(TAG, "Bitmap decodeStream from resource failed!");
                }
            }
        }
    }

    @Override
    public int getCount() {
        return mBitmapList == null ? 0 : mBitmapList.size();
    }

    @Override
    public Object getItem(int position) {
        return null;
    }

    @Override
    public View getView(final int position, View convertView, ViewGroup parent) {
        View view = LayoutInflater.from(mContext).inflate(mResource, parent, false);
        ImageView imageView = (ImageView) view.findViewById(mImageViewId[0]);
        imageView.setImageBitmap(mBitmapList.get(position));
        return view;
    }
}
