/**
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import java.util.ArrayList;
import android.content.Context;
import android.util.Log;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.ucamera.uphoto.R;
import com.ucamera.uphoto.EffectTypeResource.EffectItem;

public class EffectShowedAdapter extends DragGridViewAdapter{
    private ArrayList<EffectItem> mArrayList;
    private LayoutInflater mInflater = null;
    private OnStateChangedListener mListener;
    private Context mContext;
    private int mPressed = -1;
    private float mHorizontalMargins;
    // CID 109338 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private float mVerticalMargins;
    private float mItemWdith;
    private float mItemHeight;
    private int mNumColumns;

    public EffectShowedAdapter(Context context, ArrayList<EffectItem> arrayList) {
        mArrayList = arrayList;
        mContext = context;
        mInflater = LayoutInflater.from(context);
        mNumColumns = 1;
    }

    public void setOnStateChangedListener(OnStateChangedListener l) {
        mListener = l;
    }

    public void setItemMargins(float horizontalPadding,float verticalPadding) {
        mHorizontalMargins = horizontalPadding;
        // CID 109338 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mVerticalMargins = verticalPadding;
    }

    public void setNumColumns(int numColumns) {
        mNumColumns = numColumns;
    }

    public void setItemSize(float itemWidth, float itemHeight){
        mItemWdith = itemWidth;
        mItemHeight = itemHeight;
    }

    @Override
    public int getCount() {
        return mArrayList.size();
    }

    @Override
    public Object getItem(int position) {
        return position;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = null;
        if (convertView == null) {
            view = mInflater.inflate(R.layout.effect_showed_item_layout, parent, false);
        } else {
            view = convertView;
        }
        float leftMargins = mHorizontalMargins;
        float rightMargins = mHorizontalMargins;
        int mode = position % mNumColumns;
        if (0 == mode) {
            leftMargins = (float)Math.ceil(mHorizontalMargins * 2.0);
        }
        if (mode == mNumColumns -1) {
            rightMargins = (float)Math.ceil(mHorizontalMargins * 2.0);
        }
        ViewGroup.LayoutParams layoutParams = view.getLayoutParams();
        layoutParams.width = (int)(leftMargins+mItemWdith+rightMargins);
        layoutParams.height = (int)(mItemHeight);
        view.setLayoutParams(layoutParams);
        view.setPadding((int)leftMargins, 0, (int)rightMargins, 0);
        createInternalView(view, position);
        view.setVisibility(mPressed == position ? View.INVISIBLE : View.VISIBLE);
        return view;
    }

    private void createInternalView(View view, int position) {
        EffectItem item = mArrayList.get(position);
        if(null == item){
            return;
        }
        View stateV = view.findViewById(R.id.effect_showed_state_view);

        if(item.selectedIndex >= 0){
          stateV.setSelected(true);
        }else{
          stateV.setSelected(false);
        }
        ImageView showImgV = (ImageView)view.findViewById(R.id.effect_showed_effect_view);
        adjustImageView(showImgV,position);
        TextView txtV = (TextView)view.findViewById(R.id.effect_showed_tip_txt);
//        showImgV.setImageResource(item.mIcon);
        int iconId = mContext.getResources().getIdentifier(item.rIconName, "drawable", mContext.getPackageName());
        showImgV.setImageResource(iconId);
        /*
         * FIX BUG: 5699
         * FIX COMMENT: remove the clicked effects after long press.
         * DATE: 2014-01-02
         */
        showImgV.setPressed(mPressed == position);
//        txtV.setText(item.mTextValue);
        int txtId = mContext.getResources().getIdentifier(item.mTextValue, "string", mContext.getPackageName());
        txtV.setText(txtId);
    }

    private void adjustImageView(ImageView imgView, int position) {
        ViewGroup.LayoutParams layoutParams = imgView.getLayoutParams();
        layoutParams.width = (int)(mItemWdith);
        layoutParams.height = (int)(mItemHeight);
        imgView.setLayoutParams(layoutParams);
    }

    public interface OnStateChangedListener {
        public boolean checkBeforeDragItem(int pos);
        public boolean checkExchangeItem(int position0, int position1);
        public void onExchangeItem(int position0, int position1, EffectItem item0, EffectItem item1);
    }

    @Override
    public void exchangeItem(int position0, int position1) {
        if(null != mListener && !mListener.checkExchangeItem(position0, position1)){
            return;
        }
        int itemSize = mArrayList.size();
        if(position0 >= itemSize || position1 >= itemSize){
            return;
        }
        EffectItem i0 = mArrayList.get(position0);
        EffectItem i1 = mArrayList.get(position1);
        mArrayList.set(position0, i1);
        mArrayList.set(position1, i0);
        notifyDataSetChanged();

        if(null != mListener){
            mListener.onExchangeItem(position0, position1, i0, i1);
        }
    }

    @Override
    public boolean checkBeforeDragItem(int pos) {
        if(null != mListener){
            return mListener.checkBeforeDragItem(pos);
        }
        return true;
    }

    /*
     * FIX BUG: 5270
     * FIX COMMENT: drag selected item to unselected item of other screen,getView method not be called. so , call notifyDataSetChanged method
     * DATE: 2013-11-08
     */
    public void setPressed(int pressed) {
        mPressed = pressed;
        if(-1 == pressed){
            notifyDataSetChanged();
        }
    }
}
