/*
 *   Copyright (C) 2010,2013 Thundersoft Corporation
 *   All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import com.android.camera2.R;
import com.ucamera.ucam.utils.UiUtils;

public class MyGalleryAdapter extends SimpleAdapter {
    private int mResource;
    private int[] mTo;
    // CID 109250: UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private String[] mFrom;
    private Bitmap[] mBitmaps;
    private int[] mDisabledBg;
    private boolean mDrawState = true;
    // CID 109250: UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private List<? extends Map<String, ?>> mData;
    // CID 109223 : UuF: Unused field (FB.UUF_UNUSED_FIELD)
    // private int mEditSelectedIndex;
    // temp variable, temp storage of original Bitmap exchange
    private Bitmap mTempOrigBitmap = null;
    // temp variable, temp storage of edited Bitmap exchange
    private Bitmap mTempEditBitmap = null;
    private LayoutInflater mInflater;

    public MyGalleryAdapter(Context context, List<? extends Map<String, ?>> data, int resource,
            String[] from, int[] to, Bitmap[] bmp) {
        super(context, data, resource, from, to);
        mResource = resource;
        mTo = to;
        // CID 109250: UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mData = data;
        // CID 109250: UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mFrom = from;
        mBitmaps = bmp;
        mInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    }

    public int getCount() {
        // SPRD: add null judgement
        if (mBitmaps == null) return 0;

        return mBitmaps.length;
    }

    public long getItemId(int position) {
        return position;
    }

    public void updateBitmaps(Bitmap[] bmp){
        mBitmaps = bmp;
    }

    public void setDisabledBg(int[] unabled) {
        mDisabledBg = unabled;
        notifyDataSetChanged();
    }

    public void setDrawingState(boolean draw) {
        mDrawState = draw;
        notifyDataSetChanged();
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view;
        if (convertView == null) {
            view = mInflater.inflate(mResource, parent, false);
        } else {
            view = convertView;
        }
        ImageView bgView =  (ImageView)view.findViewById(R.id.image_list_background);
        if(mDisabledBg != null && mDisabledBg[position] == -1){
            bgView.setVisibility(View.VISIBLE);
        } else {
            bgView.setVisibility(View.GONE);
        }
        ImageView v =  (ImageView)view.findViewById(mTo[0]);
        if (mDrawState && mBitmaps[position] != null
                && !mBitmaps[position].isRecycled()) {
            v.setImageBitmap(mBitmaps[position]);
        } else {
            v.setImageBitmap(null);
        }

        return view;
    }

    /**
     * exchange the position in gallery
     *
     * @param startIndex
     *            start item position
     *
     * @param endIndex
     *            end item position
     *
     * @param isOnDrop
     *            end drag
     *
     * @param currentPressedBitmap
     *            current item pressed
     */
    public void refreshAdapter(int startIndex, int endIndex, boolean isOnDrop,
            Bitmap currentPressedBitmap) {
        // if startIndex more than endIndex, the isMoveToLeft is true, else
        // false;
        boolean isMoveToLeft = false;
        if (startIndex >= endIndex) {
            isMoveToLeft = true;
        }
        Bitmap[] originBitmaps = GifEditDatas.getOriBitmaps();
        Bitmap[] editedBitmaps = GifEditDatas.getEditBitmaps();
        Bitmap[] resultBitmaps = GifEditDatas.getResultBitmaps();
        ArrayList<byte[]> originJpegDataList = GifEditDatas.getOriginJpegDataList();
        Bitmap tempOrigBitmap = null;
        if(UiUtils.highMemo()) {
            tempOrigBitmap = originBitmaps[startIndex];
        }
        /*
         * FIX BUG: 4244
         * BUG CAUSE: In the editing, the result bitmaps will not need to assign it;
         * FIX COMMENT: To separate editing and result ui for drag the item;
         * DATE: 2013-06-07
         */
        Bitmap tempEditBitmap = null;
        if( editedBitmaps[startIndex] != null && !editedBitmaps[startIndex].isRecycled() ) {
            tempEditBitmap = editedBitmaps[startIndex];
        } else {
            tempEditBitmap = resultBitmaps[startIndex];
        }
        if (UiUtils.highMemo() && !tempOrigBitmap.equals(currentPressedBitmap)) {
            mTempOrigBitmap = tempOrigBitmap;
        }
        if (!tempEditBitmap.equals(currentPressedBitmap)) {
            mTempEditBitmap = tempEditBitmap;
        }
        byte[] tempEffectByte = null;
        if (originJpegDataList != null && originJpegDataList.size() > 0) {
            tempEffectByte = originJpegDataList.get(startIndex);
        }
        int tempPosition = mDisabledBg[startIndex];
        int index = startIndex;
        int len = endIndex;
        if (isMoveToLeft) {
            for (; index > len; index--) {
                if(UiUtils.highMemo()) {
                    originBitmaps[index] = originBitmaps[index - 1];
                }
                editedBitmaps[index] = editedBitmaps[index - 1];
                resultBitmaps[index] = resultBitmaps[index - 1];
                if (originJpegDataList != null && originJpegDataList.size() > 0) {
                    originJpegDataList.set(index, originJpegDataList.get(index - 1));
                }
                if (mDisabledBg[index - 1] == -1) {
                    mDisabledBg[index] = -1;
                } else {
                    mDisabledBg[index] = index;
                }
            }
        } else {
            for (; index < len; index++) {
                if(UiUtils.highMemo()) {
                    originBitmaps[index] = originBitmaps[index + 1];
                }
                editedBitmaps[index] = editedBitmaps[index + 1];
                resultBitmaps[index] = resultBitmaps[index + 1];
                if (originJpegDataList != null && originJpegDataList.size() > 0) {
                    originJpegDataList.set(index, originJpegDataList.get(index + 1));
                }
                if (mDisabledBg[index + 1] == -1) {
                    mDisabledBg[index] = -1;
                } else {
                    mDisabledBg[index] = index;
                }
            }
        }
        if (isOnDrop) {
            if(UiUtils.highMemo()) {
                originBitmaps[endIndex] = mTempOrigBitmap;
            }
            if(editedBitmaps[startIndex] != null && !editedBitmaps[startIndex].isRecycled() ) {
                editedBitmaps[endIndex] = mTempEditBitmap;
            } else {
                resultBitmaps[endIndex] = mTempEditBitmap;
            }
        } else {
            if(UiUtils.highMemo()) {
                originBitmaps[endIndex] = currentPressedBitmap;
            }
            if(editedBitmaps[startIndex] != null && !editedBitmaps[startIndex].isRecycled() ) {
                editedBitmaps[endIndex] = currentPressedBitmap;
            } else {
                resultBitmaps[endIndex] = currentPressedBitmap;
            }
        }
        if (originJpegDataList != null && originJpegDataList.size() > 0) {
            originJpegDataList.set(endIndex, tempEffectByte);
        }

        if (tempPosition == -1) {
            mDisabledBg[endIndex] = -1;
        } else {
            mDisabledBg[endIndex] = endIndex;
        }
        notifyDataSetChanged();
    }
}
