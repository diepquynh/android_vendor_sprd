/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit.cate;

import java.util.ArrayList;
import com.android.camera2.R;
import com.ucamera.ucam.jni.ImageProcessJni;
import com.ucamera.ucam.modules.ugif.edit.BackGroundWorkTask;
import com.ucamera.ucam.modules.ugif.edit.GifEditDatas;
import com.ucamera.ucam.modules.ugif.edit.BackGroundWorkTask.OnTaskFinishListener;
import com.ucamera.ucam.modules.utils.Utils;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Handler;
import android.os.Message;
import android.renderscript.Type;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.TextView;
import android.widget.AdapterView.OnItemSelectedListener;

public class EffectType extends BaseAdapter implements OnItemSelectedListener {
    private int mSelected = 0;
    private Context mContext;
    private ArrayList<AdapterItem> mEffectItemList;

    // CID 109265 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private OnTaskFinishListener mListener;
    private int mCurrentCate = 0;
    private Handler mHandler = new MyHandler();

    public EffectType(Context context, OnTaskFinishListener listener, int cate) {
        mContext = context;

        // CID 109265 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mListener = listener;
        mCurrentCate = cate;
    }

    public int getCount() {
        if (mEffectItemList != null && mEffectItemList.size() > 0) {
            return mEffectItemList.size();
        } else {
            return 0;
        }
    }

    @Override
    public AdapterItem getItem(int position) {
        if (mEffectItemList != null && mEffectItemList.size() > 0) {
            return mEffectItemList.get(position);
        } else {
            return null;
        }
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    public boolean updatePostEffectNames(int key) {
        ArrayList<AdapterItem> arrayList = EffectTypeResource.getInstance().getEffectItem(key);
        if (arrayList != null && arrayList.size() > 0) {
            mEffectItemList = arrayList;
            notifyDataSetChanged();
            return true;
        }

        return false;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        TextView textView;
        if (convertView == null) {
            textView = new TextView(mContext);
            textView.setTextColor(Color.WHITE);
            textView.setTextSize(28);
            textView.setGravity(Gravity.CENTER);
        } else {
            textView = (TextView) convertView;
        }

        if (position < mEffectItemList.size()) {
            textView.setText(mEffectItemList.get(position).mTextValue);
            if (mSelected == position) {
                textView.setSelected(true);
            } else {
                textView.setTextSize(20);
            }
        }
        return textView;
    }

    public void setHighlight(int position) {
        mSelected = position;
        notifyDataSetChanged();
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        if (mSelected == position) {
            return;
        }
        mSelected = position;
        // some times this will called two times, but the values of two
        // postions
        // are not equal, so here we use the handler to avoiding the
        // situation
        if (mHandler.hasMessages(CHANGE_TYPE)) {
            mHandler.removeMessages(CHANGE_TYPE);
        }
        Message msg = new Message();
        msg.what = CHANGE_TYPE;
        msg.arg1 = position;
        mHandler.sendMessageDelayed(msg, 10);

    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    public void handlePostFunEffect(final int position) {
        BackGroundWorkTask.processTask(mContext,
                mContext.getString(R.string.ugif_edit_text_waiting), new Runnable() {
                    public void run() {
                        AdapterItem adapterItem = EffectTypeResource.getInstance()
                                .getEffectItem(mCurrentCate).get(position);
                        int effectValue = adapterItem.mTypeValue;
                        ImageProcessJni.SetEffectCategory(mCurrentCate);
                        Bitmap[] editBitmap = GifEditDatas.getEditBitmaps();
                        int size = editBitmap.length;
                        byte[] newjpegData;
                        byte[] effectOriginJpegData;
                        for (int i = 0; i < size; i++) {
                            effectOriginJpegData = GifEditDatas.getOriginJpegDataList().get(i);
                            ImageProcessJni.SetEffectSrcBuffer(effectOriginJpegData,
                                    effectOriginJpegData.length,
                                    effectValue, null);
                            newjpegData = ImageProcessJni.ExecuteEffect();
                            editBitmap[i] = Utils.transformBufferToBitmap(newjpegData);
                        }
                        GifEditDatas.updateEditBitmaps(editBitmap);
                        newjpegData = null;
                        effectOriginJpegData = null;
                    }
                }, null);
    }

    private final static int CHANGE_TYPE = 1;

    class MyHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case CHANGE_TYPE:
                handlePostFunEffect(msg.arg1);
                setHighlight(msg.arg1);
                break;
            }
            super.handleMessage(msg);
        }

    }
}
