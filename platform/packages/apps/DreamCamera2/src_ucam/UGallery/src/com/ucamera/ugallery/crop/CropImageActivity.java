/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery.crop;

import com.ucamera.ugallery.util.Util;

import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;
import android.provider.MediaStore.Images.Media;
import android.app.Activity;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Toast;

import com.ucamera.uphoto.idphoto.IDPhotoType;
import com.ucamera.uphoto.idphoto.IDPhotoTypeFactory;
import com.ucamera.ugallery.R;
import com.ucamera.ugallery.util.BitmapUtil;

public class CropImageActivity extends Activity implements OnClickListener{
    private OperateCropImageView mView;
    private static int sPixelHeight  = -1;
    private static int sPixelWidth   = -1;
    private int mTopBarHeight;
    private Bitmap mBitmap;
    private HighlightView mHighlightView;
    private Uri mUri;
    private Matrix matrix;
    private int mIdPhotoType;
    private IDPhotoType mType;
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.crop_main);
        mUri = getIntent().getData();
        if(mUri == null) {
            finish();
            return;
        }
//        mBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.sample1);
        initData(mUri);
        if(mBitmap == null) {
            finish();
            return;
        }
        mIdPhotoType = getIntent().getIntExtra("idphoto_type", 1);
        mType = IDPhotoTypeFactory.getIdPhotoType(mIdPhotoType);
        initParams();
        initViews();
        initPos();
        setUpHighView();
    }
    private void initViews() {
        mView = (OperateCropImageView) findViewById(R.id.crop_image);
        findViewById(R.id.btn_crop_ok).setOnClickListener(this);
        findViewById(R.id.btn_crop_back).setOnClickListener(this);
        mView.setImageBitmap(mBitmap);
    }
    private void initParams() {
        Drawable drawable = getResources().getDrawable(R.drawable.bg_topbar);
        mTopBarHeight = drawable.getIntrinsicHeight();
        DisplayMetrics metrics = new DisplayMetrics();
        WindowManager wm = (WindowManager) this.getSystemService(Context.WINDOW_SERVICE);
        wm.getDefaultDisplay().getMetrics(metrics);
        sPixelHeight  = metrics.heightPixels - mTopBarHeight;
        sPixelWidth   = metrics.widthPixels;
    }
    @Override
    public void onClick(View v) {
        if(mView != null && mView.isTouch()) {
            return;
        }
        switch (v.getId()) {
        case R.id.btn_crop_ok:
            save();
            break;
        case R.id.btn_crop_back:
            finish();
            break;
        default:
            break;
        }
    }
    private void initPos() {
        int widthB = mBitmap.getWidth();
        int heightB = mBitmap.getHeight();
        float scalex = (float)sPixelWidth / widthB;
        float scaley = (float)sPixelHeight / heightB;
        float scale = Math.min(scalex, scaley);
        matrix = new Matrix();
        matrix.postScale(scale, scale);
        int x = (int)(sPixelWidth - widthB * scale) / 2;
        int y = (int)(sPixelHeight - heightB * scale) / 2;
        matrix.postTranslate(x, y);
        mView.setImageMatrix(matrix);
    }
    private void setUpHighView() {
        mHighlightView = new HighlightView(mView);
        int widthB = mBitmap.getWidth();
        int heightB = mBitmap.getHeight();
        int width = widthB * 3 / 4;
        int height = width * mType.getmHeight() / mType.getmWidth();
        if(widthB > heightB) {
            height = heightB * 3 / 4;
            width = height * mType.getmWidth() / mType.getmHeight();
        }
        int x = (widthB - width) / 2;
        int y = (heightB - height) / 2;
        if(x < 0 || y < 0 || width * getScale(matrix) < mType.getmWidth() || height * getScale(matrix) < mType.getmHeight()) {
            Toast.makeText(this, R.string.text_picture_wrong, Toast.LENGTH_SHORT).show();
            finish();
            return;
        }
        mHighlightView.setup(matrix, new RectF(x, y, x + width, y + height), true);
        mView.addHighLightView(mHighlightView);
    }
    private void save() {
        Bitmap bitmap = mView.save();
        Intent intent = new Intent();
        intent.setClassName(this, "com.ucamera.ucomm.smartcut.SmartCutActivity");
        intent.putExtra("idphoto_type", mIdPhotoType);
        CropImageBitmapConstant.setCropBitmap(bitmap);
        startActivity(intent);
//        finish();
    }
    private void initData(Uri uri) {
        mBitmap = BitmapUtil.getBitmap(uri,getApplicationContext(),1080,1920);
    }
    @Override
    public void onBackPressed() {
        CropImageBitmapConstant.clear();
        finish();
    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if(mHighlightView != null) {
            mHighlightView.clearCornerBitmap();
        }
    }
    private float getScale(Matrix matrix) {
        float[] values = new float[9];
        matrix.getValues(values);
        return values[4];
    }
}
