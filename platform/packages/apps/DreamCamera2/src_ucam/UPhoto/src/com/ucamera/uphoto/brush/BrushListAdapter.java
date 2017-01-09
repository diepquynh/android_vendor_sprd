/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Path;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Gallery;
import android.widget.ImageView;
import android.widget.LinearLayout.LayoutParams;

import com.ucamera.uphoto.R;
import com.ucamera.uphoto.UiUtils;

public class BrushListAdapter extends BaseAdapter {
    private Context mContext;
    private int mLayout;
    private LayoutInflater mInflater;
    private int[] mBrushStyleArrays;
    private RandomColorPicker mRandomDarkColorPicker;
    private ImageBrushManager mImageBurshManager;
    private float mDensity = 1.0f;
    private int mBrushWidth;
    private int mBrushHeight;
    private float mBrushSize = -1;
    private int mBrushColor = Color.RED;

    public BrushListAdapter(Context context, int layout, int[] brushStyleArray, RandomColorPicker randomColorPicker, ImageBrushManager imageBurshManager) {
        mContext = context;
        mLayout = layout;
        mBrushStyleArrays = brushStyleArray;
        mRandomDarkColorPicker = randomColorPicker;
        mImageBurshManager = imageBurshManager;
        mInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        getDisplayDensity(context);
    }

    public void getDisplayDensity(Context context) {
        mDensity = context.getApplicationContext().getResources().getDisplayMetrics().density;
    }

    @Override
    public int getCount() {
        if(mBrushStyleArrays != null && mBrushStyleArrays.length > 0) {
            return mBrushStyleArrays.length;
        } else {
            return 0;
        }
    }

    @Override
    public Object getItem(int position) {
        if(mBrushStyleArrays != null && mBrushStyleArrays.length > 0) {
            return mBrushStyleArrays[position];
        } else {
            return null;
        }
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    public View getView(int position, View convertView, ViewGroup viewgroup) {
        ViewHolder viewholder = null;
        if(convertView == null) {
            convertView = mInflater.inflate(mLayout, null);
            viewholder = new ViewHolder();

            viewholder.mImageView = (ImageView)convertView.findViewById(R.id.imageview_brush_icon);
            //mdip: 1.0; hdip: 1.5; pad: 2.0
            mBrushWidth = (int) (180 * mDensity);
            mBrushHeight = (int) (36 * mDensity);
            convertView.setLayoutParams(new Gallery.LayoutParams(UiUtils.screenWidth() * 3 / 4, LayoutParams.WRAP_CONTENT));
            convertView.setTag(viewholder);
        } else {
            viewholder = (ViewHolder)convertView.getTag();
        }

        Bitmap bitmap = Bitmap.createBitmap(mBrushWidth, mBrushHeight, android.graphics.Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        Path path = new Path();
        BaseBrush brush = null;
        int brushStyle = mBrushStyleArrays[position];
        if(brushStyle == BrushConstant.RainbowBrush) {
            brush = BaseBrush.createBrush(brushStyle);
            if(mBrushSize > -1) {
                brush.setSize(mBrushSize);
            }
            if(mBrushColor == Color.RED) {
                brush.setColor(Color.RED);
            } else {
                brush.setColor(mBrushColor);
            }
            mRandomDarkColorPicker.resetPicker();
            brush.setRandomColorPicker(mRandomDarkColorPicker);
            generateRainbowBrush(brush, canvas, mBrushWidth, mBrushHeight);
        } else {
            brush = BaseBrush.createBrush(brushStyle);
            brush.setContext(mContext);
            if(mBrushSize > -1) {
                brush.setSize(mBrushSize);
            }
            if(mBrushColor == Color.RED) {
                brush.setColor(Color.RED);
            } else {
                brush.setColor(mBrushColor);
            }
            if(brushStyle == BrushConstant.GradientBrush) {
                mRandomDarkColorPicker.resetPicker();
                brush.setRandomColorPicker(mRandomDarkColorPicker);
            }
            brush.setImageBurshManager(mImageBurshManager);
            brush.prepareBrush();
            if(brush.mBrushStyle > BrushConstant.DividingLine) {
                MyPoint sPoint = new MyPoint(10F, mBrushHeight / 2);
                MyPoint cPoint = new MyPoint(mBrushWidth / 4, mBrushHeight / 4);
                MyPoint ePoint = new MyPoint(mBrushWidth / 2, mBrushHeight / 2);
                brush.drawStroke(canvas, sPoint, cPoint, ePoint);

                MyPoint sPoint2 = new MyPoint(mBrushWidth / 2, mBrushHeight / 2);
                MyPoint cPoint2 = new MyPoint((mBrushWidth * 3) / 4, (mBrushHeight * 3) / 4);
                MyPoint ePoint2 = new MyPoint(mBrushWidth - 10, mBrushHeight / 2);
                brush.drawStroke(canvas, sPoint2, cPoint2, ePoint2);
            } else {
                path.moveTo(10F, mBrushHeight / 2);
                path.quadTo(mBrushWidth / 4, mBrushHeight / 4, mBrushWidth / 2, mBrushHeight / 2);
                path.quadTo((mBrushWidth * 3) / 4, (mBrushHeight * 3) / 4, mBrushWidth - 10, mBrushHeight / 2);

                brush.drawStroke(canvas, path);
            }
            brush.endStroke();
        }
        viewholder.mImageView.setImageBitmap(bitmap);

        return convertView;
    }

    public void generateRainbowBrush(BaseBrush brush, Canvas canvas, int width, int height) {
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

    public void updateColor(int color) {
        mBrushColor = color;

        notifyDataSetChanged();
    }

    public void updateSize(float brushSize) {
        mBrushSize = brushSize;

        notifyDataSetChanged();
    }

    private final class ViewHolder {
        ImageView mImageView;
    }
}
