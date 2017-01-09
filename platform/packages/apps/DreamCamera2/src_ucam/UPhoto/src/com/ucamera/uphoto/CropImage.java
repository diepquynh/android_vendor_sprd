/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 *
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.media.FaceDetector;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.GridView;
import android.widget.LinearLayout;
import android.widget.Toast;


import java.util.concurrent.CountDownLatch;

import dalvik.system.VMRuntime;

//import com.ucamera.ucam.stat.StatApi;


/**
 * The activity can crop specific region of interest from an image.
 */
public class CropImage extends ImageEditBaseActivity implements OnClickListener, CropImageView.DismissPopUp{
    private static final String TAG = "CropImage";

    public static final int CROP_MSG = 10;
    public static final int CROP_MSG_INTERNAL = 100;

    private int mAspectX, mAspectY; // CR: two definitions per line == sad
                                    // panda.
    private boolean mDoFaceDetection = true;
    private boolean mCircleCrop = false;
    private final Handler mHandler = new Handler();

    // These options specifiy the output image size and whether we should
    // scale the output to fit it (or just crop it).
    private boolean mScale;
    private boolean mScaleUp = true;

    boolean mWaitingToPick; // Whether we are wait the user to pick a face.
    boolean mSaving; // Whether the "save" button is already clicked.

    private CropImageView mImageView;

    public static Bitmap sParamBitmap = null;
    public static Bitmap sConfirmBitmap = null;

    private Bitmap mBitmap;
    HighlightView mCrop;
    private GestureDetector mGestureDetector;
    private ImageEditDesc mImageEditDesc;
    enum CROPMODE {FREE, RATIO};
    public static CROPMODE mMode;
    private Button mBtnRatio;
    private GridView mGridViewRatio;
    private CropRatioAdapter mCropRatioAdapter;
    private int[] mResourcesIds;
    private int[][] mRatios;
    private LinearLayout mLinearRatio;
    private boolean mIsDouble = false;
    public static boolean isNeedRelaout = true;
    private RectF mFaceRect;
    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        Log.d(TAG, "onCreate(): entry...");
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.cropimageview);
        mIDphotoAction = getIntent().getIntExtra(ImageEditControlActivity.EXTRA_ENTRY_MODULE, -1);
        initData();
        initViews();
        mGestureDetector = new GestureDetector(this, new CropImageGestureListener());
        mImageEditDesc= ImageEditDesc.getInstance();
        if (sParamBitmap != null) {
            mBitmap = sParamBitmap;
            sParamBitmap = null;
        } else {
            mBitmap  = mImageEditDesc.getBitmap();
        }
        /* SPRD: Fix bug 565974 trying to use a recycled bitmap @{ */
        if (mBitmap != null && !mBitmap.isRecycled()) {
            mBitmap = mBitmap.copy(mBitmap.getConfig(), true);
        }
        /* @} */

        if(sConfirmBitmap != null) {
            sConfirmBitmap = null;
        }
        if (mBitmap == null) {
              finish();
              return;
        }
        startFaceDetection();
        if(mIDphotoAction == ImageEditControlActivity.ACTION_ID_PHOTO) {
            mMode = CROPMODE.RATIO;
            mAspectX = 3;
            mAspectY = 4;
        } else {
            mMode = CROPMODE.FREE;
        }
        mImageView.setDismissPop(this);
        VMRuntime.getRuntime().setTargetHeapUtilization((float)0.75);
    }

    private void initViews() {
        mImageView = (CropImageView) findViewById(R.id.crop_imageview);
        mBtnRatio = (Button) findViewById(R.id.btn_ratio);
        mBtnRatio.setOnClickListener(this);
        if(mIDphotoAction == ImageEditControlActivity.ACTION_ID_PHOTO) {
            mBtnRatio.setVisibility(View.GONE);
        }
        mLinearRatio = (LinearLayout) findViewById(R.id.linear_ratio);
        mGridViewRatio = (GridView) findViewById(R.id.gv_ratio);
        mCropRatioAdapter = new CropRatioAdapter(this, mResourcesIds ,mRatios);
        mGridViewRatio.setAdapter(mCropRatioAdapter);
        mGridViewRatio.setOnItemClickListener(new RatioItemClickListener());
    }
    private void initData() {
        mResourcesIds = new int[] {R.drawable.cut_ratio_one_selector, R.drawable.cut_ratio_two_selector, R.drawable.cut_ratio_three_selector,
                R.drawable.cut_ratio_four_selector, R.drawable.cut_ratio_five_selector, R.drawable.cut_ratio_six_selector, R.drawable.cut_ratio_free_selector,
                R.drawable.cut_ratio_free_selector, R.drawable.cut_ratio_free_selector};
        mRatios = new int[][]{{1, 1}, {3, 2}, {4, 3}, {16, 9}, {2, 3}, {3, 4}, {0,0}, {0,0}, {0,0}};
    }
    class RatioItemClickListener implements OnItemClickListener {
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            if(position ==6 || position == 8) {
                return;
            }
            if(mRatios[position][0] > mRatios[position][1]) {
                if(mBitmap.getHeight() < 25 || mBitmap.getWidth() / mRatios[position][0] * mRatios[position][1] < 25) {
                    Toast.makeText(CropImage.this, R.string.cut_ratio_message, Toast.LENGTH_SHORT).show();
                    return;
                }
            } else if(mRatios[position][0] < mRatios[position][1]){
                if(mBitmap.getWidth() < 25 || mBitmap.getHeight() / mRatios[position][1] * mRatios[position][0] < 25) {
                    Toast.makeText(CropImage.this, R.string.cut_ratio_message, Toast.LENGTH_SHORT).show();
                    return;
                }
            }
            mMode = CROPMODE.RATIO;
            handleCrop(mRatios[position][0], mRatios[position][1]);
            mLinearRatio.setVisibility(View.INVISIBLE);
            if(position == 7) {
                mMode = CROPMODE.FREE;
                mBtnRatio.setText(R.string.cut_ratio_btn);
            } else {
                mBtnRatio.setText(mRatios[position][0] + ":" +mRatios[position][1]);}
            mBtnRatio.setSelected(false);
        }
    }
    private void startFaceDetection() {
        if (isFinishing()) {
            return;
        }
//        mBitmap = resizeBitmap(b);
        mImageView.setImageBitmapResetBase(mBitmap, true);

        ImageEditOperationUtil.startBackgroundJob(this, null,
                getResources().getString(R.string.text_waiting),
                new Runnable() {
                    public void run() {
                        final CountDownLatch latch = new CountDownLatch(1);
                        final Bitmap b = mBitmap;
                        mHandler.post(new Runnable() {
                            public void run() {
                                if (b != mBitmap && b != null) {
                                    mImageView.setImageBitmapResetBase(b, true);
                                    mBitmap.recycle();
                                    mBitmap = b;
                                }
                                if (mImageView.getScale() == 1.0f) {
                                    mImageView.center(true, true);
                                }
                                latch.countDown();
                            }
                        });
                        try {
                            latch.await();
                        } catch (InterruptedException e) {
                            throw new RuntimeException(e);
                        }
                        mRunFaceDetection.run();
                    }
                }, mHandler);
    }
    private void onSaveClicked(boolean cropAgain) {
        // this code needs to change to use the decode/crop/encode single
        // step api so that we don't require that the whole (possibly large)
        // bitmap doesn't have to be read into memory
        if (mSaving || mCrop == null){
            return;
        }

//        mSaving = true;

        Rect r = mCrop.getCropRect();
        int width = r.width(); // CR: final == happy panda!
        int height = r.height();

        if(width == mBitmap.getWidth() && height == mBitmap.getHeight()) {
            return;
        }
        // If we are circle cropping, we want alpha channel, which is the
        // third param here.
//        Bitmap croppedImage = Bitmap.createBitmap(width, height, mCircleCrop ? Bitmap.Config.ARGB_8888 : Bitmap.Config.RGB_565);
        Bitmap croppedImage = null;
        Log.d(TAG, "onSaveClicked(): width = " + width + ", height = " + height + ",. mCircleCrop = " + mCircleCrop + ", mScale = " + mScale);
        do {
            try {
                croppedImage = Bitmap.createBitmap(width, height, mCircleCrop ? Bitmap.Config.ARGB_8888 : Bitmap.Config.RGB_565);
            } catch(OutOfMemoryError oom) {
                Log.w(TAG, "prepareBitmap(): code has a memory leak is detected...");
            }
            if(croppedImage == null) {
                //index is one, means that any operation can not be Completed
                if(mImageEditDesc.reorganizeQueue() < 2) {
                    ImageEditOperationUtil.showHandlerToast(CropImage.this,  R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                    return;
                }
            }
        } while(croppedImage == null);

        {
            Canvas canvas = new Canvas(croppedImage);
            Rect dstRect = new Rect(0, 0, width, height);
            /*
             * BUG COMMENT:  trying to use a recycled bitmap
             * FIX DATE: 2014-03-26
             */
            if(mBitmap.isRecycled()) {
                return;
            }
            canvas.drawBitmap(mBitmap, r, dstRect, null);
        }

        mBitmap = croppedImage;
        if(cropAgain) {
            if(sConfirmBitmap != mBitmap){
                sConfirmBitmap = mBitmap;
            }
            mImageView.setVisibility(View.INVISIBLE);
            startFaceDetection();
        }
    }

    @Override
    protected void onResume() {
        Log.d(TAG, "onResume(): entry...");
        super.onResume();
//        StatApi.onResume(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
//        StatApi.onPause(this);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    public Bitmap getCropBitmap(boolean perform) {
        if (perform) {
            if(mCrop !=null) {
                if(mBitmap.getWidth() == mCrop.getCropRect().width() && mBitmap.getHeight() == mCrop.getCropRect().height()) {
                    return mBitmap;
                }
            }
            onSaveClicked(false);
        }
        return mBitmap;
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        mGestureDetector.onTouchEvent(ev);
        return super.dispatchTouchEvent(ev);
    }

    Runnable mRunFaceDetection = new Runnable() {
        float mScale = 1F;
        Matrix mImageMatrix;
        FaceDetector.Face[] mFaces = new FaceDetector.Face[3];
        int mNumFaces;

        // For each face, we create a HightlightView for it.
        private void handleFace(FaceDetector.Face f) {
            PointF midPoint = new PointF();

            int r = ((int) (f.eyesDistance() * mScale)) * 2;
            f.getMidPoint(midPoint);
            midPoint.x *= mScale;
            midPoint.y *= mScale;

            int midX = (int) midPoint.x;
            int midY = (int) midPoint.y;

            HighlightView hv = new HighlightView(mImageView);

            int width = mBitmap.getWidth();
            int height = mBitmap.getHeight();

            Rect imageRect = new Rect(0, 0, width, height);

            RectF faceRect = new RectF(midX, midY, midX, midY);
            faceRect.inset(-r, -r);
            if (faceRect.left < 0) {
                faceRect.inset(-faceRect.left, -faceRect.left);
            }

            if (faceRect.top < 0) {
                faceRect.inset(-faceRect.top, -faceRect.top);
            }

            if (faceRect.right > imageRect.right) {
                faceRect.inset(faceRect.right - imageRect.right, faceRect.right - imageRect.right);
            }

            if (faceRect.bottom > imageRect.bottom) {
                faceRect.inset(faceRect.bottom - imageRect.bottom, faceRect.bottom - imageRect.bottom);
            }
            if(mIsDouble == true) {
                mIsDouble = false;
            }
            mAspectX = 0;
            mAspectY = 0;
            mMode = CROPMODE.FREE;
            mBtnRatio.setText(R.string.cut_ratio_btn);
            mFaceRect = faceRect;

            hv.setup(mImageMatrix, imageRect, faceRect, mCircleCrop, mAspectX != 0 && mAspectY != 0);
            // To display face rect
            mImageView.setVisibility(View.VISIBLE);
            mImageView.add(hv);
        }

        // Create a default HightlightView if we found no face in the picture.
        private void makeDefault() {
            mFaceRect = null;
            addCropView();
        }

        // Scale the image down for faster face detection.
        private Bitmap prepareBitmap() {
            if (mBitmap == null) {
                return null;
            }

            // 256 pixels wide is enough.
            if (mBitmap.getWidth() > 256) {
                mScale = 256.0F / mBitmap.getWidth(); // CR: F => f (or change
                                                      // all f to F).
            }
            Matrix matrix = new Matrix();
            matrix.setScale(mScale, mScale);
            Bitmap faceBitmap = null;
            try {
                faceBitmap = Bitmap.createBitmap(mBitmap, 0, 0, mBitmap.getWidth(), mBitmap.getHeight(), matrix, true);
            } catch(OutOfMemoryError oom) {
                Log.w(TAG, "prepareBitmap(): code has a memory leak is detected...");
                ImageEditDesc.getInstance().reorganizeQueue();
            } catch(Exception exception) {
                 Log.w(TAG, exception.getMessage());
            }

            return faceBitmap;
        }

        public void run() {
            mImageMatrix = mImageView.getImageMatrix();
            Bitmap faceBitmap = prepareBitmap();

            mScale = 1.0F / mScale;
            if (faceBitmap != null && mDoFaceDetection) {
                FaceDetector detector = new FaceDetector(faceBitmap.getWidth(), faceBitmap.getHeight(), mFaces.length);
                mNumFaces = detector.findFaces(faceBitmap, mFaces);
            }

            if (faceBitmap != null && faceBitmap != mBitmap) {
                faceBitmap.recycle();
            }

            mHandler.post(new Runnable() {
                public void run() {
                    mWaitingToPick = mNumFaces > 1;
                    if (mNumFaces > 0) {
                        for (int i = 0; i < mNumFaces; i++) {
                            handleFace(mFaces[i]);
                        }
                    } else {
                        makeDefault();
                    }
                    mImageView.invalidate();
                    if (mImageView.mHighlightViews.size() == 1) {
                        mCrop = mImageView.mHighlightViews.get(0);
                        mCrop.setFocus(true);
                    }

                    if (mNumFaces > 1) {
                        // CR: no need for the variable t. just do
                        // Toast.makeText(...).show().
                        Toast t = Toast.makeText(CropImage.this, R.string.multiface_crop_help, Toast.LENGTH_SHORT);
                        t.show();
                    }
                }
            });
        }
    };

    private int mIDphotoAction;

    private void addCropView() {
        HighlightView hv = new HighlightView(mImageView);

        int width = mBitmap.getWidth();
        int height = mBitmap.getHeight();

        Rect imageRect = new Rect(0, 0, width, height);

        int cropWidth = width;
        int cropHeight = height;
        if(mFaceRect != null) {
            width = cropWidth = (int)mFaceRect.width();
            height = cropHeight = (int)mFaceRect.height();
        }
        /*
         * BUG FIX: 4024, 4022
         * BUG COMMENT: after double click,crop size equal picture size
         * FIX DATE: 2013-05-24
         */
        if(mIsDouble) {
            mIsDouble = false;
        } else {
            if(Math.min(cropWidth, cropHeight) <=25) {
                if (mAspectX != 0 && mAspectY != 0) {
                    if (cropWidth > cropHeight) {
                        cropWidth = cropHeight * mAspectX / mAspectY;
                    }else {
                        cropHeight = cropWidth * mAspectY / mAspectX;
                    }
                }else {
                    cropWidth = cropHeight = Math.min(cropWidth, cropHeight);
                }
            }else {
                cropWidth = Math.min(width, height) * 4 / 5;
                cropHeight = cropWidth;

                if (mAspectX != 0 && mAspectY != 0) {
                    if (mAspectX > mAspectY) {
                        cropHeight = cropWidth * mAspectY / mAspectX;
                    } else {
                        cropWidth = cropHeight * mAspectX / mAspectY;
                    }
                }
            }
        }

        int x = (width - cropWidth) / 2;
        int y = (height - cropHeight) / 2;

        RectF cropRect = new RectF(x, y, x + cropWidth, y + cropHeight);
        hv.setup(mImageView.getImageMatrix(), imageRect, cropRect, mCircleCrop, mAspectX != 0 && mAspectY != 0);
        if(mImageView.getVisibility() == View.INVISIBLE) {
            mImageView.setVisibility(View.VISIBLE);
        }
        mImageView.add(hv);
    }
    class CropImageGestureListener extends GestureDetector.SimpleOnGestureListener{
        @Override
        public boolean onDoubleTap(MotionEvent e) {
            onSaveClicked(true);
            mIsDouble = true;
            return true;
        }
    }

    private void handleCrop(int ratioX, int ratioY) {
        mAspectX = ratioX;
        mAspectY = ratioY;
        addCropView();

        mImageView.invalidate();
        if (mImageView.mHighlightViews.size() == 1) {
            mCrop = mImageView.mHighlightViews.get(0);
            mCrop.setFocus(true);
        }
    }
    @Override
    public void onClick(View v) {
        switch(v.getId()) {
        case R.id.btn_ratio:
            isNeedRelaout = false;
            if(mBtnRatio.isSelected()) {
                mBtnRatio.setSelected(false);
                mLinearRatio.setVisibility(View.INVISIBLE);
            }else {
                mBtnRatio.setSelected(true);
                mLinearRatio.setVisibility(View.VISIBLE);
            }
            break;
        }
    }

    @Override
    public void dimiss() {
        if(mLinearRatio.getVisibility() == View.VISIBLE) {
            mLinearRatio.setVisibility(View.INVISIBLE);
        }
        mBtnRatio.setSelected(false);
    }
}
