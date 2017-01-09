/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit.cate;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import com.android.camera2.R;
import com.ucamera.ucam.jni.ImageProcessJni;
import com.ucamera.ucam.modules.ugif.edit.BackGroundWorkTask;
import com.ucamera.ucam.modules.ugif.edit.GifEditDatas;
import com.ucamera.ucam.modules.ugif.edit.callback.ProcessCallback;
import com.ucamera.ucam.modules.ugif.edit.cate.GifEditTypeAdapter.ListenerId;
import com.ucamera.ucam.modules.utils.UiUtils;
import com.ucamera.ucam.modules.utils.Utils;
import android.content.Context;
import android.graphics.Bitmap;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.AdapterView;
import android.widget.ImageView;

public abstract class GifBasicCate {
    private static final String TAG = "GifBasicCate";
    protected Context mContext = null;
    protected int mSelected = -1;
    protected LayoutInflater mInflater = null;
    protected ArrayList<Integer> mArrayRes= null;
    protected ProcessCallback mCallback = null;
    protected HashMap<Integer, Method> mHashMap = new HashMap<Integer, Method>();
    protected int mItemMaxHeight;
    protected int mItemMaxWidth;

    public GifBasicCate(Context context, ProcessCallback callback) {
        mContext = context;
        mInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mCallback = callback;
    }

    public View getView(int position, View convertView, ViewGroup parent) {
        if (mArrayRes == null) {
            return null;
        }
        View view = null;
        if (convertView == null) {
            view = View.inflate(mContext, R.layout.ugif_edit_menu_item, null);
        } else {
            view = convertView;
        }
        ImageView iv =  (ImageView)view.findViewById(R.id.iv_ugif_edit_item_bk);
        iv.setBackgroundResource(R.drawable.ugif_edit_menu_highlight_bg_selector);
        iv.setImageResource(mArrayRes.get(position));

        if(mItemMaxHeight > 0) {
            LayoutParams layoutparameter = iv.getLayoutParams();
            layoutparameter.height = mItemMaxHeight - UiUtils.dpToPixel(3);
            layoutparameter.width = mItemMaxWidth - UiUtils.dpToPixel(3);
            iv.setLayoutParams(layoutparameter);
        }
        iv.setSelected(mSelected == position);
        return view;
    }

    public void setHighlight(int selected) {
        mSelected = selected;
    }

    public int getCount() {
        return mArrayRes != null ? mArrayRes.size() : 0;
    }

    public void updateContents(String[] strings) {

    }

    public void setItemMaxSize(int size) {
        mItemMaxWidth = mItemMaxHeight = size;
    }

    public abstract void onItemClick(int position);

    protected void handleOverlay(final String strFileName) {
        BackGroundWorkTask.processTask(mContext, mContext.getString(R.string.ugif_edit_text_waiting), new Runnable() {
            public void run() {
                //SPRD:fix bug530324 java.lang.IndexOutOfBoundsException
                GifEditDatas.isDataReady = false;
                byte[] newjpegData;
                byte[] effectOriginJpegData;
                /* SPRD: Fix bug 536577 java.lang.IndexOutOfBoundsException @{ */
                synchronized (GifEditDatas.mLock) {
                    Bitmap[] editedBitmaps = GifEditDatas.getEditBitmaps();
                    ArrayList<byte[]> originJpegDataList = GifEditDatas.getOriginJpegDataList();
                    if (editedBitmaps == null || originJpegDataList == null
                            || editedBitmaps.length != originJpegDataList.size()) {
                        Log.d(TAG, "editedBitmaps = " + editedBitmaps + "; originJpegDataList = " + originJpegDataList);
                        GifEditDatas.isDataReady = true;
                        return;
                    }

                    for(int i = 0;i < editedBitmaps.length; i++){
                        //effectOriginJpegData = GifEditDatas.getOriginJpegDataList().get(i);
                        effectOriginJpegData = originJpegDataList.get(i);
                        if (effectOriginJpegData == null) {
                            Log.d(TAG,"effectOriginJpegData = " + effectOriginJpegData);
                            GifEditDatas.isDataReady = true;
                            return;
                        }
                        newjpegData = ImageProcessJni.AddPhotoFrame4JpegBuffer(effectOriginJpegData, effectOriginJpegData.length, strFileName);
                        if (newjpegData == null) {
                            Log.e(TAG, "ImageProcessJni error");
                            GifEditDatas.isDataReady = true;
                            return ;
                        }
                        editedBitmaps[i] = Utils.transformBufferToBitmap(newjpegData);
                    }
                    GifEditDatas.updateEditBitmaps(editedBitmaps);
                }
                /* @} */
                newjpegData = null;
                effectOriginJpegData = null;
                //SPRD:fix bug530324 java.lang.IndexOutOfBoundsException
                GifEditDatas.isDataReady = true;
            }
        }, null);
    }
}
