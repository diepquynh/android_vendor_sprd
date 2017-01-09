/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import com.ucamera.uphoto.brush.BaseBrush;
import com.ucamera.uphoto.brush.BrushConstant;
import com.ucamera.uphoto.brush.BrushItemInfo;
import com.ucamera.uphoto.brush.ImageBrushManager;
import com.ucamera.uphoto.brush.MyPoint;
import com.ucamera.uphoto.brush.RandomColorPicker;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Path;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Gallery;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.LinearLayout.LayoutParams;

import java.util.ArrayList;
import java.util.HashMap;

public class MyGraffitiViewBaseAdapter extends BaseAdapter {
    private Context mContext;
    private ArrayList<Object> mBrushList;
    private RandomColorPicker mRandomColorPicker;
    private ImageBrushManager mImageBurshManager;
    private boolean mDeleteViewVisiable;
    private boolean mTextViewVisiable;
    private int mItemWidth;
    private int mItemHeight = 48;
    private boolean mShowCanvas;
    private float mDensity = 1.0f;

    private LayoutInflater mLayoutInflater;

    public MyGraffitiViewBaseAdapter(Context context, ArrayList<Object> brushList, RandomColorPicker randomColorPicker,
            ImageBrushManager imageBurshManager, int itemWidth, boolean deleteViewVisiable, boolean showCanvas, int itemHeight, boolean textVisible) {
        mContext = context;
        mBrushList = brushList;
        mRandomColorPicker = randomColorPicker;
        mImageBurshManager = imageBurshManager;
        mDeleteViewVisiable = deleteViewVisiable;
        mTextViewVisiable = textVisible;
        mItemWidth = itemWidth;
//        mItemHeight = 48;
        mItemHeight = itemHeight;
        mShowCanvas = showCanvas;

        mLayoutInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        getDisplayDensity(context);
    }

    public void getDisplayDensity(Context context) {
        mDensity = context.getApplicationContext().getResources().getDisplayMetrics().density;
    }

    @Override
    public int getCount() {
        if(mBrushList != null && mBrushList.size() > 0) {
            return mBrushList.size();
        } else {
            return 0;
        }
    }

    @Override
    public Object getItem(int position) {
        if(mBrushList != null && mBrushList.size() > 0) {
            return mBrushList.get(position);
        } else {
            return null;
        }
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    public void setDeleteViewVisible(boolean visible) {
        mDeleteViewVisiable = visible;
        notifyDataSetChanged();
    }

    public boolean getDeleteViewVisible() {
        return mDeleteViewVisiable;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder;
        Object object = mBrushList.get(position);
        if(convertView != null) {
            holder = (ViewHolder) convertView.getTag();
        } else {
            holder = new ViewHolder();
            convertView = mLayoutInflater.inflate(R.layout.brush_common_item, parent, false);
            holder.imageView = (ImageView) convertView.findViewById(R.id.imageview_brush_icon);
            holder.textView = (TextView) convertView.findViewById(R.id.textview_desc);
            holder.deleteView = (ImageView) convertView.findViewById(R.id.imageview_brush_delete_icon);
            mItemWidth = (int) (180 * mDensity);
            mItemHeight = (int) (36 * mDensity);
            if(object instanceof BrushItemInfo) {
                convertView.setLayoutParams(new Gallery.LayoutParams(UiUtils.screenWidth() * 3 / 4, LayoutParams.WRAP_CONTENT));
            }
            convertView.setTag(holder);
        }

        if(object instanceof HashMap) {
            @SuppressWarnings("unchecked")
            HashMap<String, Integer> map = (HashMap<String, Integer>) object;
            holder.deleteView.setVisibility(View.GONE);
            String key = map.keySet().iterator().next();
            int iconId = map.get(key).intValue();
            holder.textView.setText(key);
            holder.imageView.setImageResource(iconId);
        } else if(object instanceof BrushItemInfo) {
            BrushItemInfo info = (BrushItemInfo) object;
            if(true == mDeleteViewVisiable) {
                holder.deleteView.setVisibility(View.VISIBLE);
            } else {
                holder.deleteView.setVisibility(View.GONE);
            }

            if(mTextViewVisiable){
                holder.textView.setVisibility(View.VISIBLE);
            }else{
                holder.textView.setVisibility(View.GONE);
            }

            int brushStyle = info.brushStyle;
            Bitmap bitmap = Bitmap.createBitmap(mItemWidth, mItemHeight, android.graphics.Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            Path path = new Path();
            BaseBrush brush = null;
            brush = BaseBrush.createBrush(brushStyle);
            String brushColor = info.brushColor;
            if(brushColor.startsWith("#")) {
                brush.setColor(Color.parseColor(brushColor));
            } else {
                brush.setColor(Integer.valueOf(brushColor));
            }
            brush.mBrushSize = info.brushSize;
            if(brushStyle == BrushConstant.RainbowBrush) {
                mRandomColorPicker.resetPicker();
                brush.setRandomColorPicker(mRandomColorPicker);
                generateRainbowBrush(brush, canvas, mItemWidth, mItemHeight);
            } else {
                brush.setContext(mContext);
                if(brushStyle == BrushConstant.GradientBrush) {
                    mRandomColorPicker.resetPicker();
                    brush.setRandomColorPicker(mRandomColorPicker);
                }
                brush.setImageBurshManager(mImageBurshManager);
                brush.prepareBrush();
                if(brush.mBrushStyle > BrushConstant.DividingLine) {
                    MyPoint sPoint = new MyPoint(10F, mItemHeight / 2);
                    MyPoint cPoint = new MyPoint(mItemWidth / 4, mItemHeight / 4);
                    MyPoint ePoint = new MyPoint(mItemWidth / 2, mItemHeight / 2);
                    brush.drawStroke(canvas, sPoint, cPoint, ePoint);

                    MyPoint sPoint2 = new MyPoint(mItemWidth / 2, mItemHeight / 2);
                    MyPoint cPoint2 = new MyPoint((mItemWidth * 3) / 4, (mItemHeight * 3) / 4);
                    MyPoint ePoint2 = new MyPoint(mItemWidth - 10, mItemHeight / 2);
                    brush.drawStroke(canvas, sPoint2, cPoint2, ePoint2);
                } else {
                    path.moveTo(10F, mItemHeight / 2);
                    path.quadTo(mItemWidth / 4, mItemHeight / 4, mItemWidth / 2, mItemHeight / 2);
                    path.quadTo((mItemWidth * 3) / 4, (mItemHeight * 3) / 4, mItemWidth - 10, mItemHeight / 2);

                    brush.drawStroke(canvas, path);
                }
                brush.endStroke();
            }
            holder.imageView.setImageBitmap(bitmap);
            holder.textView.setText(mShowCanvas ? String.valueOf(position - BrushConstant.getGraffitiFunctionItems())
                    : String.valueOf(position - BrushConstant.getGraffitiFunctionItems() + 1));
        }

        return convertView;
    }

    private static void generateRainbowBrush(BaseBrush brush, Canvas canvas, int width, int height) {
        Path path = new Path();
        float drawWidth = width - 10;
        float space = drawWidth / 32;
        float index = 5F;
        do {
            path.reset();
            path.moveTo(index, height / 2);
            path.lineTo(index + space, height / 2);
            brush.updateBrush();
            brush.drawStroke(canvas, path);
            index += space;
        } while(index < drawWidth);
    }

    private final class ViewHolder {
        ImageView imageView;
        TextView textView;
        ImageView deleteView;
    }
}
