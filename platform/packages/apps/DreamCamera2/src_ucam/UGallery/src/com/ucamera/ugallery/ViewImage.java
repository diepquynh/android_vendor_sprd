/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */

/*
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

package com.ucamera.ugallery;

import com.ucamera.ugallery.util.Models.*;
import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;

import com.ucamera.ugallery.util.ShareVideoUtils;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.ContentUris;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.DialogInterface.OnClickListener;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.Bitmap.Config;
import android.graphics.drawable.ColorDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.MediaStore;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Images.Media;
import android.provider.MediaStore.Video;
import android.text.Editable;
import android.text.TextWatcher;
import android.text.format.DateFormat;
import android.text.format.Formatter;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.FloatMath;
import android.util.Log;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.GridView;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.Scroller;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.VideoView;
import android.widget.AdapterView;

//import com.ucamera.ucomm.sns.integration.ShareUtils;
import com.ucamera.ugallery.R;
import com.ucamera.ugallery.ViewGalleryAdapter;
import com.ucamera.ugallery.gallery.IImage;
import com.ucamera.ugallery.gallery.IImageList;
import com.ucamera.ugallery.gallery.ImageList;
import com.ucamera.ugallery.gallery.ImageLoader;
import com.ucamera.ugallery.gallery.VideoObject;
import com.ucamera.ugallery.integration.Build;
import com.ucamera.ugallery.provider.UCamData;
import com.ucamera.ugallery.util.ImageManager;
import com.ucamera.ugallery.util.Models;
import com.ucamera.ugallery.util.RotationUtil;
import com.ucamera.ugallery.util.UiUtils;
import com.ucamera.ugallery.util.Util;
import com.ucamera.ugallery.util.ViewImageCommonUtil;
import com.ucamera.ugallery.video.MovieView;
import com.ucamera.ugallery.ThumbnailGridView;
import android.graphics.drawable.Drawable;

/**
 * This activity can display a whole picture and navigate them in a specific
 * gallery. It has two modes: normal mode and slide show mode. In normal mode
 * the user view one image at a time, and can click "previous" and "next" button
 * to see the previous or next image. In slide show mode it shows one image
 * after another, with some transition effect.
 */
public class ViewImage extends Activity implements View.OnClickListener, ThumbnailGridView.DrawAdapter, ThumbnailGridView.Listener {
    private static final String TAG = "ViewImage";
    private ImageGetter mGetter;
    private Uri mSavedUri;
    boolean mPaused = true;
    boolean sharedClick = false;
    boolean deleteClick = false;
    // Choices for what adjacents to load.
    private static final int[] sOrderAdjacents = new int[] {
            0, 1, -1
    };
    final GetterHandler mHandler = new GetterHandler();
    int mCurrentPosition = 0;
    public static final String EXTRA_IMAGE_LIST_KEY     = "extra_image_list";
    public static final String EXTRACURRENT_BUCKID_KEY  = "extra_image_bucket_id";
    public static final String EXTRA_IMAGE_ENTRY_KEY    = "extra_image_entry";
    public static final String EXTRA_IMAGE_STORAGE_KEY    = "extra_image_storage";
    public static final String IMAGE_ENTRY_UPHOTO_VALUE = "uphoto"; //uphoto and SNS share
    public static final String IMAGE_ENTRY_UGIF_VALUE   = "ugif";
    public static final String IMAGE_ENTRY_PUZZLE_VALUE = "puzzle";
    public static final String IMAGE_ENTRY_NORMAL_VALUE = "normal";
    public static final String IMAGE_ENTRY_UGALLERY_VALUE = "ugallery";
    public static final String IMAGE_ENTRY_PICKER_VALUE   = "picker";
    public static final String EXTRA_VIEW_MODE = "extra_view_mode";
    public static final String EXTRA_UPHOTO_MODULE_KEY = "entry_uphoto_module";
    public static final String VIEW_MODE_TIME = "mode_time";
    public static final String VIDEO_VIEW = "videoView";
    private static final String STATE_URI = "uri";
    private IImageList mAllImages;
    private String mCurrentBucketId;
    private String mCurrentStoragePos;
    private ImageManager.ImageListParam mParam;
    GestureDetector mGestureDetector;
    private TextView mTitle;
    public ScrollController mScroller;
    private final ImageViewTouch[] mImageViews = new ImageViewTouch[3];
    private static boolean isPepper = false;
    private int mNumPerScreen;
//    private BitmapCache mCache;

    /*private static final int CACHE_CAPACITY = 1024;
    private final LruCache<Integer, Bitmap> mLruCache = new LruCache<Integer, Bitmap>(CACHE_CAPACITY);*/

    private BitmapCache mCache;

    private String mEntry = null;

    private View mTopNumLayout;
    private View mControlActionLayout;
    private LinearLayout mControlAcLinearLayout;
    private ThumbnailGridView mThumbnailGridView;
    private boolean mShowControlBar = false;
    static boolean scrolled;
    public float mLastXposition = 0;
    private Matrix matrix = new Matrix();
    private Matrix savedMatrix = new Matrix();
    private PointF start = new PointF();
    private PointF start2 = new PointF();
    private PointF mid = new PointF();
    private float oldDist;
    private float scale = 0;
    static final int NONE = 0;
    static final int DRAG = 1;
    static final int ZOOM = 2;
    int mode = NONE;
    private float lastStartX;
    private float lastStartY;
    private float lastEndX;
    private float lastEndY;
    private int mCount;
    private Toast mFirstToast = null;
    private Toast mLastToast = null;
    private Dialog mTipDialog = null;
    private int mCurrentIndex = -1;
    private static final int SCROLLVIEW_SCROLL = 100;
    private static boolean mIsNeedThumbnail = true;
    private boolean mIsThumbnailLayoutComplete = false;
    private ImageLoader mLoader;
    private static final float INVALID_POSITION = -1f;
    private float mScrollPosition = INVALID_POSITION;
    private boolean mConfigurationChanged = false;
    private int mRealWidth = 0;

//    private HintLayer mHintLayer;

    public static void setPepper(boolean ispepper) {
        isPepper = ispepper;
    }
    public static void setIsNeedThumbnail(boolean isNeed) {
        mIsNeedThumbnail = isNeed;
    }
    private final Runnable mHideControlBarRunnable = new Runnable() {
        public void run() {
            hideControlBar();
        }
    };

    private void scheduleControlBar() {
        mHandler.removeCallbacks(mHideControlBarRunnable);
        mHandler.postDelayed(mHideControlBarRunnable, 5000);
    }

    private void hideControlBar() {
        mShowControlBar = false;
        if(mIsNeedThumbnail) {
            mThumbnailGridView.setVisibility(View.INVISIBLE);
        }
        mControlActionLayout.setVisibility(View.INVISIBLE);
        mTopNumLayout.setVisibility(View.INVISIBLE);
        /*
         * FIX BUG: 5354
         * BUG COMMENT: java.lang.IllegalArgumentException: View not attached to window manager
         * DATE: 2013-11-18
         */
        if(mPopupWindow != null && !isFinishing()) {
            mPopupWindow.dismiss();
        }
        /*if (mHintLayer != null) {
            mHintLayer.hideHintFor(R.id.nav_to_gallery);
        }*/
    }

    private void showControlBar() {
        mShowControlBar = true;
        if(mIsNeedThumbnail) {
            mThumbnailGridView.setVisibility(View.VISIBLE);
        }
        mControlActionLayout.setVisibility(View.VISIBLE);
        mTopNumLayout.setVisibility(View.VISIBLE);
        scheduleControlBar();
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent m) {
        if (mPaused) {
            return true;
        }

        if (!super.dispatchTouchEvent(m)) {
            return mImageViews[1].handleTouchEvent(m);
        }
        return true;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "entry onDestroy().");
        scrolled = false;
    }

    private class MyGestureListener extends GestureDetector.SimpleOnGestureListener {

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
//            mScroller.scrollTo((int) (mImageViews[1].getLeft() + distanceX), 0);
//            populateMoveEvent((int) distanceX);
            /*if (e2.getAction() == MotionEvent.ACTION_MOVE) {
                populateMoveEvent((int) (e1.getRawX() - e2.getRawX()));
            }*/

            return true;
        }

        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
//            return super.onFling(e1, e2, velocityX, velocityY);
//            stopMoveEvent((int) (e1.getRawX() - e2.getRawX()));

            /*if (Math.abs(e1.getY() - e2.getY()) <= swipe_max_off_path) {
                if (e2.getX() - e1.getX() > swipe_min_distance
                        && Math.abs(velocityX) > swipe_threshold_veloicty) {
                    movePrevious();
                }

                if (e1.getX() - e2.getX() > swipe_min_distance
                        && Math.abs(velocityX) > swipe_threshold_veloicty) {
                    moveNext();
                }
            }*/


            return true;
        }

        @Override
        public boolean onSingleTapConfirmed(MotionEvent e) {
            if(mCount > 0) {
                if (mShowControlBar == true) {
                    hideControlBar();
                } else {
                    showControlBar();
                }
            } else {
                hideControlBar();
            }

            return true;
        }

        @Override
        public boolean onDoubleTap(MotionEvent e) {
            ImageViewTouch imageView = mImageViews[1];

            // Switch between the original scale and 3x scale.
            if (imageView.getScale() > 1F) {
                mImageViews[1].zoomTo(1f);
            } else {
                mImageViews[1].zoomToPoint(3f, e.getX(), e.getY());
            }
            // }
            return true;
        }
    }

    protected Runnable mDeleteImageRunnable = new Runnable() {
        public void run() {
            Log.d(TAG, "entry mDeleteImageRunnable.run().");
            scrolled = false;
            /**
             * FIX BUG: 4696
             * FIX COMMENT: Null pointer exception
             * DATE: 2013-08-13
             */
            if(mAllImages == null) {
                return;
            }
            IImage image = mAllImages.getImageAt(mCurrentPosition);
            if(image == null) {
                return;
            }
            getContentResolver().delete(UCamData.Thumbnails.CONTENT_URI, UCamData.Thumbnails.THUMB_PATH + "=?", new String[]{image.getDataPath()});
            getContentResolver().delete(UCamData.Albums.CONTENT_URI, UCamData.Albums.ALBUM_BUCKET + "=?", new String[]{image.getBucketId()});
            try {
                File file = new File(image.getDataPath());
                file.delete();
            } catch (Exception e) {
                Log.e(TAG, "delete file fail" + e);
            }
            mAllImages.removeImage(image);
            if(mIsNeedThumbnail) {
                mThumbnailGridView.onStop();
                mThumbnailGridView.setImageList(mAllImages);
                mThumbnailGridView.onStart();
            }
            for (ImageViewTouch iv : mImageViews) {
                iv.clear();
            }
            mCache.clear();
            mCount = mAllImages.getCount();
            if (mCount == 0) {
                ImageGallery.showImagePicker(ViewImage.this, BaseImagePicker.class, mEntry, mCurrentStoragePos);
                if(! isUGallery()) {
                    finish();
                }
//                findViewById(R.id.layout_no_images).setVisibility(View.VISIBLE);
//                findViewById(R.id.play_icon).setVisibility(View.INVISIBLE);
                /*
                 * FIX BUG: 1996
                 * FIX COMMENT: When delete all images, only disply no images ui;
                 * DATE: 2012-11-28
                 */
                return;
            }
            if (mCurrentPosition == mCount) {
                mCurrentPosition -= 1;
            }
            setImage(mCurrentPosition);
            Log.d(TAG, "Leave mDeleteImageRunnable.run().");
        }
    };

    void setImage(int pos) {
        mCurrentPosition = pos;
        updateTitle();
        for (int i = 0; i <= 2; i++) {
            int position = pos + i - 1;
           Bitmap b = mCache.getBitmap(position);
            if (b != null && !b.isRecycled() && position>=0) {
                IImage image = null;
                /*
                 * FIX BUG: 5194
                 * BUG COMMENT: Avoid null pointer exception
                 * DATE: 2013-11-12
                 */
                if(mAllImages != null) {
                    image = mAllImages.getImageAt(position);
                }
                if (image != null) {
                    mImageViews[i].setImageRotateBitmapResetBase(
                            new RotateBitmap(b, image.getDegreesRotated()), true);
                }
            }
        }

        ImageViewTouchBase current = mImageViews[1];
        current.mSuppMatrix.reset();
        current.setImageMatrix(current.getImageViewMatrix());

        // move to the right position
        int width = mImageViews[1].getWidth();
        int to = width + 20;
//        int to = width;
        mScroller.scrollTo(to, 0);
        if (mAllImages == null) {
            return;
        }
        IImage image = mAllImages.getImageAt(mCurrentPosition);
        if(mIsNeedThumbnail && mThumbnailGridView != null && mIsThumbnailLayoutComplete) {
            mThumbnailGridView.setSelectIndex(mCurrentPosition);
        }
        if (image != null) {
            if (image.getMimeType() != null && image.getMimeType().equals("image/gif") || ImageManager.isVideo(image)) {
                findViewById(R.id.play_icon).setVisibility(View.VISIBLE);
            } else {
                findViewById(R.id.play_icon).setVisibility(View.INVISIBLE);
            }
            /**
             * FIX BUG: 5111
             * FIX COMMENT:  Remove some wrong features when open the ugallery from the file manager.
             * Date: 2013-10-12
             */
            if ((image.getMimeType() != null && image.getMimeType().equals("image/gif")) ||
                    (image.getTitle() != null && image.getTitle().startsWith(ImageListSpecial.PANORAMA_PREFIX))
                    || isSingleImageMode(mSavedUri)){
                mRotateOrRename.setVisibility(View.GONE);
            } else {
                mRotateOrRename.setVisibility(View.VISIBLE);
            }
        }
        ImageGetterCallback cb = new ImageGetterCallback() {
            public void completed() {
                mImageViews[1].setFocusableInTouchMode(true);
                mImageViews[1].requestFocus();
            }

            public boolean wantsThumbnail(int pos, int offset) {
//                return (mLruCache.get(pos + offset) != null) ? false : true;
                return !mCache.hasBitmap(pos + offset);
            }

            public boolean wantsFullImage(int pos, int offset) {
                return offset == 0;
            }

            public int fullImageSizeToUse(int pos, int offset) {
                // this number should be bigger so that we can zoom. we may
                // need to get fancier and read in the fuller size image as the
                // user starts to zoom.
                // Originally the value is set to 480 in order to avoid OOM.
                // Now we set it to 2048 because of using
                // native memory allocation for Bitmaps.
                final int imageViewSize = Build.USE_LARGE_CAPACITY_IMAGE ? 4096 : 2048;
//                final int imageViewSize = 1024;
                return imageViewSize;
            }

            public int[] loadOrder() {
                return sOrderAdjacents;
            }

            public void imageLoaded(int pos, int offset, RotateBitmap bitmap, boolean isThumb) {
                // We may get a result from a previous request. Ignore it.
                if (pos != mCurrentPosition) {
                    bitmap.recycle();
                    return;
                }

                if (isThumb) {
                    mCache.put(pos + offset, bitmap.getBitmap());
//                    mLruCache.put(pos + offset, bitmap.getBitmap());
                }
                ImageViewTouch ivt = mImageViews[1 + offset];
                if (bitmap == null) {
                    bitmap = new RotateBitmap(BitmapFactory.decodeResource(getResources(),
                            R.drawable.missing_thumbnail_picture));
                }
                ivt.setImageRotateBitmapResetBase(bitmap, isThumb);
            }
        };
        // Could be null if we're stopping a slide show in the course of pausing
        if (mGetter != null) {
            mGetter.setPosition(pos, cb, mAllImages, mHandler);
        }
    }

    private Handler mScrollHandler;
    private int mScrollViewX;
    @Override
    public void onCreate(Bundle instanceState) {
        super.onCreate(instanceState);
        UiUtils.initialize(this);
        mGestureDetector = new GestureDetector(this, new MyGestureListener());
        setContentView(R.layout.common_viewimage);
        Log.d(TAG, "entry onCreate().");

        mScroller = (ScrollController) findViewById(R.id.scroller);
        mTitle = (TextView) findViewById(R.id.title_text);
        mTopNumLayout = findViewById(R.id.layout_top_num);
        mControlActionLayout = findViewById(R.id.layout_control_action);
        mControlAcLinearLayout = (LinearLayout)findViewById(R.id.layout_control_action);
        mControlAcLinearLayout.getBackground().setAlpha(219);
        Drawable drawable = getResources().getDrawable(R.drawable.edit_bottom_bar_bg);
        int sEffectItemHeight = drawable.getIntrinsicHeight();
        if (sEffectItemHeight > 0) {
            RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) mControlAcLinearLayout
                    .getLayoutParams();
            params.height = sEffectItemHeight;
            mControlAcLinearLayout.setLayoutParams(params);
        }
        findViewById(R.id.nav_to_gallery).setOnClickListener(this);
        findViewById(R.id.nav_to_album).setOnClickListener(this);
        findViewById(R.id.btn_image_delete).setOnClickListener(this);
        findViewById(R.id.btn_image_share).setOnClickListener(this);
/*        if(!ShareUtils.SNS_SHARE_IS_ON) {
            findViewById(R.id.btn_image_share).setVisibility(View.INVISIBLE);
        }*/
        findViewById(R.id.btn_image_edit).setOnClickListener(this);
        mRotateOrRename = (TextView)findViewById(R.id.btn_image_rotate);
        mRotateOrRename.setOnClickListener(this);
        TextView moreView = (TextView)findViewById(R.id.btn_image_more);
        moreView.setOnClickListener(this);
        findViewById(R.id.play_icon).setOnClickListener(this);
        mImageViews[0] = (ImageViewTouch) findViewById(R.id.image1);
        mImageViews[1] = (ImageViewTouch) findViewById(R.id.image2);
        mImageViews[2] = (ImageViewTouch) findViewById(R.id.image3);

        mCache = new BitmapCache(3, getApplicationContext());
        for (ImageViewTouch ivt : mImageViews) {
            ivt.setRecycler(mCache);
        }
        mParam = getIntent().getParcelableExtra(EXTRA_IMAGE_LIST_KEY);
        mCurrentBucketId = getIntent().getStringExtra(EXTRACURRENT_BUCKID_KEY);
        mCurrentStoragePos = getIntent().getStringExtra(EXTRA_IMAGE_STORAGE_KEY);
        mIsVideoType = getIntent().getBooleanExtra(VIDEO_VIEW, false);
        mCurrentViewMode = getIntent().getStringExtra(ViewImage.EXTRA_VIEW_MODE);
        if(mIsVideoType) {
            findViewById(R.id.btn_image_edit).setVisibility(View.GONE);
//            findViewById(R.id.btn_image_rotate).setVisibility(View.GONE);
//            moreView.setImageResource(R.drawable.gallery_details_status);
            Util.setTextAndDrawableTop(getApplicationContext(), mRotateOrRename, R.drawable.gallery_rename_status, R.string.text_image_rename);
            Util.setTextAndDrawableTop(getApplicationContext(), moreView, R.drawable.gallery_details_status, R.string.text_image_details);
            ((TextView)findViewById(R.id.nav_to_gallery)).setText(getString(R.string.video));
        }
        if (instanceState != null) {
            mSavedUri = instanceState.getParcelable(STATE_URI);
        } else {
            mSavedUri = getIntent().getData();
        }

        Log.d(TAG, "onCreate(): mSavedUri is " + mSavedUri + ", mCurrentBucketId is " + mCurrentBucketId + ", mParam is " + mParam);
        /*
         * FIX BUG: 5480
         * BUG COMMENT: get bucket id by uri if the scheme of uri is "file"
         * DATE: 2013-11-29
         */
        if(mSavedUri != null && mCurrentBucketId == null) {
            String path = null;
            Log.d(TAG, "onCreate(): mSavedUri is " + mSavedUri);
            if (mSavedUri.getScheme().equals("file")) {
                path = mSavedUri.getPath();
            } else if(mSavedUri.getScheme().equals("content")){
                String uriString = mSavedUri.toString();
                Log.d(TAG, "onCreate(): uriString is " + uriString);
                if(uriString.startsWith("content://media")) {
                    Cursor cursor = getContentResolver().query(mSavedUri, new String[]{MediaStore.Images.Media.DATA}, null, null, null);
                    if (cursor != null) {
                        if(cursor.moveToFirst()) {
                            int columnIndex = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
                            path = cursor.getString(columnIndex);
                        }
                        //String buckutId = cursor.getString(cursor.getColumnIndexOrThrow(MediaStore.Images.Media.BUCKET_ID));
                        //Log.d(TAG, "onCreate(): buckutId is " + buckutId + ", path is " + path);
                        cursor.close();
                    }
                }
            }
            if(path != null){
                path = path.substring(0, path.lastIndexOf("/"));
                mCurrentBucketId = String.valueOf(path.toLowerCase().hashCode());
            }
            Log.d(TAG, "onCreate(): path is " + path + ", mCurrentBucketId is " + mCurrentBucketId);
        }
        mEntry = getIntent().getStringExtra(EXTRA_IMAGE_ENTRY_KEY);
        Log.d(TAG, "onCreate(): mEntry is " + mEntry);
        makeGetter();
        if(mIsNeedThumbnail) {
            mThumbnailGridView = (ThumbnailGridView)findViewById(R.id.thumbnail_grid_view);
            mThumbnailGridView.setOnTouchListener(new View.OnTouchListener() {
                public boolean onTouch(View v, MotionEvent event) {
                    scheduleControlBar();
                    mScrollHandler.sendEmptyMessageDelayed(SCROLLVIEW_SCROLL, 50);
                    return false;
                }
            });
        }
        mScrollHandler = new Handler(){
            public void handleMessage(Message msg) {
                switch(msg.what) {
                    case SCROLLVIEW_SCROLL:
                        int newX = mThumbnailGridView.getScrollX();
                        if(mScrollViewX != newX) {
                            scheduleControlBar();
                            mScrollViewX = newX;
                            mScrollHandler.sendEmptyMessageDelayed(SCROLLVIEW_SCROLL, 50);
                        }
                        break;
                    default:
                        break;
                }
            }
        };
//        mHintLayer = HintLayer.showHintLayer(this,R.layout.hint_ucam_gallery);
    }

    /*
     * FIX BUG: 4684
     * BUG COMMENT: reset timer when the screen has changed
     * DATA: 2013-09-12
     */
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        mConfigurationChanged = true;
        UiUtils.initialize(this);
        mRealWidth = UiUtils.screenWidth();
        scheduleControlBar();
        if(mIsNeedThumbnail && mThumbnailGridView != null) {
             mThumbnailGridView.onStart();
        }
    }

    private boolean isUGallery() {
        return IMAGE_ENTRY_UGALLERY_VALUE.equals(mEntry);
    }

    private void updateTitle() {
        if (mPaused) {
            return;
        }

        int currentCount = mCurrentPosition + 1;
        String title = "";
        if (mAllImages != null) {
            title = currentCount + "/" + mCount;
        }
        mTitle.setText(title);
    }

    private void animateScrollTo(int xNew, int yNew) {
        mScroller.startScrollTo(xNew, yNew);
    }

    void setMode() {
        if (mGetter != null) {
            mGetter.cancelCurrent();
        }
        /* SPRD: CID 109210 : UrF: Unread field (FB.URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD) @{ */
        // ImageViewTouchBase dst = mImageViews[1];
        // dst.mLastXTouchPos = -1;
        // dst.mLastYTouchPos = -1;
        /* @} */

        // mGetter null is a proxy for being paused
        if (mGetter != null) {
            setImage(mCurrentPosition);
        }
    }

    private void makeGetter() {
        mGetter = new ImageGetter(getContentResolver(), this, isSmallMemory());
    }
    public boolean isSmallMemory() {
        mTotalMemory = Util.getSystemTotalMemory(getApplicationContext());
        return ( mTotalMemory >> 20) < 512;
    }
    private IImageList buildImageListFromUri(Uri uri) {
        return ImageManager.makeImageList(getContentResolver(), uri, ImageManager.SORT_DESCENDING);
    }
    private boolean isSingleImageMode(Uri uri) {
        String uriString = (uri != null) ? uri.toString() : "";
//        String bucketId = (uri != null) ?uri.getQueryParameter("bucketId"):null;
        /**
         * FIX BUG: 5617
         * FIX COMMENT: SingleImageMode Do not startwith content://media/external/video/media/ or content://media/internal/video/media/
         * DATE: 2013-12-16
         */
        return !uriString.startsWith(MediaStore.Images.Media.EXTERNAL_CONTENT_URI.toString())
                && !uriString.startsWith(MediaStore.Images.Media.INTERNAL_CONTENT_URI.toString())
                && !uriString.startsWith(MediaStore.Video.Media.EXTERNAL_CONTENT_URI.toString())
                && !uriString.startsWith(MediaStore.Video.Media.INTERNAL_CONTENT_URI.toString()) ;
    }

    private boolean init(Uri uri) {
        if (uri == null) {
            return false;
        }

        if (mParam != null) {
            mParam.mSort = ImageManager.SORT_DESCENDING;
        }
        Log.d(TAG, "init(): mCurrentViewMode = " + mCurrentViewMode + ", mCurrentBucketId = " + mCurrentBucketId + ", mParam is " + mParam);
        if(ViewImage.VIEW_MODE_TIME.equals(mCurrentViewMode)) {
            if(!mIsVideoType) {
                if("internal".equals(mCurrentStoragePos) || uri.toString().contains(Media.INTERNAL_CONTENT_URI.toString())){
                    mAllImages = new ImageList(getContentResolver(),Media.INTERNAL_CONTENT_URI, ImageManager.SORT_DESCENDING, mCurrentBucketId);
                }else{
                    mAllImages = new ImageList( getContentResolver(),Media.EXTERNAL_CONTENT_URI, ImageManager.SORT_DESCENDING, mCurrentBucketId);
                }
            } else {
                mAllImages = (mParam == null) ? buildImageListFromUri(uri) : ImageManager.makeImageList(getContentResolver(), mParam);
            }
        } else {
            if(mCurrentBucketId != null) {
//                Uri tempUri = null;
//                if("internal".equals(mCurrentStoragePos)) {
//                    tempUri = Images.Media.INTERNAL_CONTENT_URI.buildUpon().appendQueryParameter("bucketId", mCurrentBucketId).build();
//                }else {
//                    tempUri = Images.Media.EXTERNAL_CONTENT_URI.buildUpon().appendQueryParameter("bucketId", mCurrentBucketId).build();
//                }
                Uri tempUri  = Images.Media.EXTERNAL_CONTENT_URI.buildUpon().appendQueryParameter("bucketId", mCurrentBucketId).build();
                mAllImages = (mParam == null) ? buildImageListFromUri(tempUri) : ImageManager.makeImageList(getContentResolver(), mParam);
            } else {
                mAllImages = (mParam == null) ? buildImageListFromUri(uri) : ImageManager.makeImageList(getContentResolver(), mParam);
            }
        }
        if(mAllImages == null) {
            return false;
        }
        Log.d(TAG, "init(): mAllImages size is " + mAllImages.getCount());
        IImage image = mAllImages.getImageForUri(uri);
        if (image == null) {
            return false;
        }
        mCurrentPosition = mAllImages.getImageIndex(image);
        return true;
    }
    private Uri getCurrentUri() {
        mCount = mAllImages.getCount();
        if (mCount == 0 || mCurrentPosition >= mCount) {
            return null;
        }
        IImage image = mAllImages.getImageAt(mCurrentPosition);
        /**
         * FIX BUG: 1238
         * BUG CAUSE: Get the Image object is empty;
         * FIX COMMENT: Determine whether the current picture object is empty.
         * DATE: 2012-07-11
         */
        if(image != null) {
            return image.fullSizeImageUri();
        }

        return null;
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        Log.d(TAG, "entry onSaveInstanceState(): mCurrentPosition = " + mCurrentPosition + ", mAllImages = " + mAllImages);
        /**
         * FIX BUG: 1235
         * BUG CAUSE: When no pictures in the current directory, press the HOME key, to get the Image object is empty;
         * FIX COMMENT: Determine whether the current picture object is empty.
         * DATE: 2012-07-11
         */
        IImage image = mAllImages.getImageAt(mCurrentPosition);
        Log.d(TAG, "entry onSaveInstanceState(): image = " + image);
        if(image != null) {
            outState.putParcelable(STATE_URI, image.fullSizeImageUri());
        }
    }

    @Override
    public void onStart() {
        super.onStart();
        mPaused = false;
        Log.d(TAG, "entry onStart().");

        if (!init(mSavedUri)) {
            Log.w(TAG, "init failed: " + mSavedUri);
            finish();
            return;
        }
        // normally this will never be zero but if one "backs" into this
        // activity after removing the sdcard it could be zero. in that
        // case just "finish" since there's nothing useful that can happen.
        mCount = mAllImages.getCount();
        if (mCount == 0 || mCurrentPosition >= mCount) {
            finish();
            return;
        } else if (mCount <= mCurrentPosition) {
            mCurrentPosition = mCount - 1;
        }

        if (mGetter == null) {
            makeGetter();
        }
        scrolled = false;
        updateTitle();
        if(mIsNeedThumbnail) {
            mLoader = new ImageLoader(getContentResolver(), mHandler);
            mThumbnailGridView.onStop();
            mThumbnailGridView.setImageList(mAllImages);
            mThumbnailGridView.setDrawAdapter(this);
            mThumbnailGridView.setListener(this);
            mThumbnailGridView.setImageLoader(mLoader);
            mThumbnailGridView.onStart();
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        Log.d(TAG, "entry onResume().");
        // toast instantiated here is used to get the method of cancel()
        // to prevent the Toast from showing too long when it occurs many times
        scrolled = false;
        mFirstToast = Toast.makeText(this, mIsVideoType ? R.string.text_no_more_videos_first : R.string.text_no_more_pictures_first, Toast.LENGTH_SHORT);
        mLastToast = Toast.makeText(this, mIsVideoType ? R.string.text_no_more_videos_last : R.string.text_no_more_pictures_last, Toast.LENGTH_SHORT);
        setMode();
        showControlBar();
    }

    @Override
    public void onPause() {
        super.onPause();
        Log.d(TAG, "entry onPause().");
         /* FIX BUG : 4753
         * BUG COMMENT : remove "mHideControlBarRunnable" from mHander and avoid view not attached to window manager exception
         * DATE : 2013-08-26
         */
        if(mHandler != null) {
            mHandler.removeCallbacks(mHideControlBarRunnable);
        }
    }

    @Override
    public void onStop() {
        super.onStop();
        Log.d(TAG, "entry onStop().");
        mPaused = true;

        // mGetter could be null if we call finish() and leave early in
        // onStart().
        if (mGetter != null) {
            mGetter.cancelCurrent();
            mGetter.stop();
            mGetter = null;
        }
//        setMode();

        // removing all callback in the message queue
        mHandler.removeAllGetterCallbacks();
        /*
         * FIX BUG: 5711
         * BUG COMMENT:mThumbnailGridView stop before closed cursor
         * DATE: 2013-12-30
         */
        if(mIsNeedThumbnail && mThumbnailGridView != null) {
            mThumbnailGridView.onStop();
        }
        if (mAllImages != null) {
            /*
             * FIX BUG: 5546
             * BUG COMMENT: not need to get current Uri in onStop
             * DATE: 2013-12-10
             */
            mSavedUri = getCurrentUri();
            mAllImages.close();
            mAllImages = null;
        }

        for (ImageViewTouch iv : mImageViews) {
            iv.clear();
        }
        mCache.clear();
        if(mLoader != null) {
            mLoader.stop();
        }

//        mLruCache.clear();

//        final HintLayer hintLayer = mHintLayer;
//        if (hintLayer != null) {
//            hintLayer.hideHintLayer();
//        }
    }
    private long mLastClickTime = 0;
    private boolean mIsVideoType;
    private PopupWindow mPopupWindow;
    private long mTotalMemory;
    private TextView mRotateOrRename;
    private boolean mIsPanorama;
    private boolean mIsBurst;
    private boolean mIsGif;
    private String mOrgTitle;
    public void onClick(View v) {
        int viewId = v.getId();
        /**
         * FIX BUG: 1245
         * BUG CAUSE: When deleted all images, the top and bottom bar has not hidden, and then click delete, share, edit or details, will crash;
         * FIX COMMENT: If current image object is null or the current bucket has no image, display the other message and return;
         * DATE: 2012-07-17
         */
        /*
         * FIX BUG:5353
         * BUG COMMENT: avoid null pointer exception
         * DATE: 2013-11-19
         */
        if(mAllImages == null || mAllImages.getImageAt(mCurrentPosition) == null || mAllImages.getCount() == 0) {
            hideControlBar();
            findViewById(R.id.layout_no_images).setVisibility(View.VISIBLE);
            findViewById(R.id.play_icon).setVisibility(View.INVISIBLE);
            return;
        }
        final IImage image = mAllImages.getImageAt(mCurrentPosition);
        Log.d(TAG, "onClick(): viewId is " + viewId + ", mCurrentPosition is " + mCurrentPosition + ", image is " + image);
        switch (viewId) {
            case R.id.nav_to_album:
//                ImageGallery.showImagePicker(this, BaseImagePicker.class, ViewImage.IMAGE_ENTRY_NORMAL_VALUE);
                ImageGallery.showImagePicker(this, BaseImagePicker.class, mEntry, mCurrentStoragePos);
                if(! isUGallery()) {
                    finish();
                }
                break;
            case R.id.nav_to_gallery:
                Intent intentImage = new Intent();
                intentImage.setClass(this, BaseImagePicker.class);
//                intent.putExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY, ViewImage.IMAGE_ENTRY_NORMAL_VALUE);
                Uri uri = null;
                if(ImageManager.isVideo(image)) {
                    uri = Images.Media.INTERNAL_CONTENT_URI;
                    intentImage.putExtra("mediaTypes", ImageManager.INCLUDE_VIDEOS);
                } else {
                    intentImage.putExtra("mediaTypes", ImageManager.INCLUDE_IMAGES);
                    uri = Images.Media.INTERNAL_CONTENT_URI.buildUpon().appendQueryParameter("bucketId", mCurrentBucketId).build();
                }
                intentImage.setData(uri);
                intentImage.putExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY, mEntry);
                intentImage.putExtra(ViewImage.EXTRACURRENT_BUCKID_KEY, mCurrentBucketId);
                intentImage.putExtra(ViewImage.EXTRA_IMAGE_STORAGE_KEY, mCurrentStoragePos);
                intentImage.putExtra(ViewImage.EXTRA_VIEW_MODE, mCurrentViewMode);
                if(ViewImage.IMAGE_ENTRY_UGALLERY_VALUE.equals(mEntry)) {
                    intentImage.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                }
                startActivity(intentImage);
                if (!isUGallery()) {
                    finish();
                }
                break;
            case R.id.btn_image_delete:
                //hideControlBar();
                scheduleControlBar();
                /**
                 * BUG FIX: 3070
                 * BUG CAUSE: Dialog box pops up several times
                 * DATE: 2013-03-11
                 */
                if (System.currentTimeMillis() - mLastClickTime < 500){
                    return;
                }
                mLastClickTime = System.currentTimeMillis();
                if(ImageManager.isImage(image)) {
                    MenuHelper.deleteImage(ViewImage.this, mDeleteImageRunnable, 1);
                } else {
                    MenuHelper.deleteVideo(ViewImage.this, mDeleteImageRunnable, 1);
                }
                break;
            case R.id.btn_image_share:
                //hideControlBar();
                scheduleControlBar();
                mSavedUri = image.fullSizeImageUri();
                if(ImageManager.isImage(image)) {
//                    ShareUtils.shareImage(this, image.fullSizeImageUri());
                } else {
                    if (!MenuHelper.isWhiteListUri(image.fullSizeImageUri())) {
                        return;
                    }
                    startShareMediaActivity(image);
                }
                break;
            case R.id.btn_image_edit:
                Intent uphotoIntent = new Intent();
                mSavedUri = image.fullSizeImageUri();
                try {
                    uphotoIntent.setAction("android.intent.action.UGALLERY_EDIT");
                    uphotoIntent.setDataAndType(image.fullSizeImageUri(), "image/*");
                    uphotoIntent.putExtra("PictureDegree", image.getDegreesRotated());
                    uphotoIntent.putExtra(UGalleryConst.EXTRA_FROM_INNER, true);
                    uphotoIntent.putExtra("ImageFileName", image.getDataPath());
                    startActivity(uphotoIntent);
                } catch(ActivityNotFoundException e) {
                    findUCamApp("edit");
                }
                break;
            case R.id.btn_image_rotate:
                /*
                 * FIX BUG : 4677
                 * BUG COMMENT : reset delay time of hide control bar when click the button
                 * DATE : 2013-08-06
                 */
                scheduleControlBar();
                if(ImageManager.isVideo(image)) {
                    renameFile(image);
                } else {
                    /*
                     * FIX BUG: 5707
                     * FIX COMMENT: set image rotation when click rotate button to replace get bitmap by file path
                     * DATE: 2014-01-08
                     */
//                    ImageViewTouch ivt = mImageViews[1];
//                    ivt.setImageRotation(90);
                    RotationUtil.startBackgroundJob(ViewImage.this, "loading", getString(R.string.text_waiting), new Runnable() {
                        public void run() {
                            handleRotateAction(image , 90);
                           /*
                            * FIX BUG: 6095
                            * BUG COMMENT: remove last thumbnail of file dir after Rotate image
                            * DATE: 2014-03-13
                            */
                            File file = new File(getFilesDir(),"last_image_thumb");
                            if(file.exists() && file.isFile()) {
                                 file.delete();
                            }
                            if(mCurrentPosition == 0) {
                                getContentResolver().delete(UCamData.Albums.CONTENT_URI, UCamData.Albums.ALBUM_BUCKET + "=?", new String[]{image.getBucketId()});
                            }
                            getContentResolver().delete(UCamData.Thumbnails.CONTENT_URI, UCamData.Thumbnails.THUMB_PATH + "=?", new String[]{image.getDataPath()});
                            runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    if (mAllImages != null && mAllImages.getCount() > 0 && mImageViews[1] != null) {
                                        mImageViews[1].clear();
                                        if(mIsNeedThumbnail && mThumbnailGridView != null) {
                                            mThumbnailGridView.updateSelectedItem();
                                        }
                                        setImage(mCurrentPosition);
                                    }
                                }
                            });
                        }
                    });
                }
                break;
            case R.id.btn_image_more:
                scheduleControlBar();
                if(ImageManager.isVideo(image)) {
                    hideControlBar();
                    ViewImageCommonUtil.showImageDetails(ViewImage.this, image);
                } else {
                    showPopupWindow(image);
                }
                break;
            case R.id.play_icon:
                hideControlBar();
                /*
                 * FIX BUG: 6044
                 * BUG COMMENT: avoid the java.lang.NullPointerExcep
                 * DATE: 2014-03-05
                 */
                if("image/gif".equals(image.getMimeType())){
                    Intent gifIntent = new Intent();
                    gifIntent.setData(image.fullSizeImageUri());
                    gifIntent.setClass(ViewImage.this, GifPlayerActivity.class);
                    startActivity(gifIntent);
                } else if(ImageManager.isVideo(image)){
/*                    Intent videoIntent = new Intent();
                    videoIntent.setData(image.fullSizeImageUri());
                    videoIntent.putExtra(MediaStore.EXTRA_SCREEN_ORIENTATION,
                            ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
                    videoIntent.setClass(ViewImage.this, MovieView.class);*/
                    try {
                        Intent videoIntent = new Intent(Intent.ACTION_VIEW);
                        videoIntent.setDataAndType(image.fullSizeImageUri(), image.getMimeType());
                        //videoIntent.putExtra(MediaStore.EXTRA_SCREEN_ORIENTATION, ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
                        startActivity(videoIntent);
                    } catch (ActivityNotFoundException ex) {
                        Toast.makeText(this, getString(R.string.text_video_no_media_player), Toast.LENGTH_SHORT).show();
                        Log.e(TAG, "Couldn't view video " + image.fullSizeImageUri(), ex);
                    }
                }
                break;
            default:
                break;
        }
    }

    private void showPopupWindow(final IImage image) {
        View contentView = View.inflate(getApplicationContext(),
                R.layout.popup_menu_item, null);
        /**
         * FIX BUG: 5931
         * FIX COMMENT:the menu Does not work
         * Date: 2014-02-10
         */
        if (Models.HTC_DOPOD.equals(Models.getModel())) {
            contentView.findViewById(R.id.popup_menu_setas).setVisibility(View.GONE);
        }
        View.OnClickListener listener =  new View.OnClickListener(){

            @Override
            public void onClick(View v) {
                switch (v.getId()) {
                case R.id.popup_menu_rename:
                    renameFile(image);
                    break;
                case R.id.popup_menu_details:
                    hideControlBar();
                    ViewImageCommonUtil.showImageDetails(ViewImage.this, image);
                    break;
                case R.id.popup_menu_setas:
                    Intent intent = Util.createSetAsIntent(image);
                    try {
                        startActivity(Intent.createChooser(
                                intent, getText(R.string.setImage)));
                    } catch (android.content.ActivityNotFoundException ex) {
                        Toast.makeText(ViewImage.this, R.string.no_way_to_share_video,
                                Toast.LENGTH_SHORT).show();
                    }
                    break;
                default:
                    break;
                }
                //mPopupWindow.dismiss();
                hideControlBar();
            }
        };
//        contentView.findViewById(R.id.popup_menu_rotate_left).setOnClickListener(listener);
        /**
         * FIX BUG: 5111
         * FIX COMMENT:  Remove some wrong features when open the ugallery from the file manager.
         * Date: 2013-10-12
         */
        if(isSingleImageMode(mSavedUri)) {
            contentView.findViewById(R.id.popup_menu_rename).setVisibility(View.GONE);
            /*
             * FIX BUG: 5296
             * FIX COMMENT:Remove the line background of popu memu;
             * DATE: 2013-11-14
             */
            contentView.findViewById(R.id.popup_menu_line2).setVisibility(View.GONE);
        } else {
            contentView.findViewById(R.id.popup_menu_rename).setOnClickListener(listener);
        }
        contentView.findViewById(R.id.popup_menu_details).setOnClickListener(listener);
        /*
         * FIX BUG: 5134
         * FIX COMMENT:Remove the the operation of setting wallpaper in AMAZON_KFTT device
         * DATE: 2013-10-11
         */
        if (Models.AMAZON_KFTT.equals(Models.getModel())) {
            contentView.findViewById(R.id.popup_menu_setas).setVisibility(View.GONE);
        } else {
            contentView.findViewById(R.id.popup_menu_setas).setOnClickListener(listener);
        }
        mPopupWindow = new PopupWindow(contentView ,LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT, true);
        mPopupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        mPopupWindow.setFocusable(true);
        mPopupWindow .setOutsideTouchable(true);
        View view = findViewById(R.id.layout_control_action);
        mPopupWindow.showAtLocation(view,Gravity.RIGHT|Gravity.BOTTOM, 0, view.getHeight()-8);
    }

    private void renameFile(final IImage image) {
        final String oriPath = image.getDataPath();
        final File file = new File(oriPath);
        View view = LayoutInflater.from(ViewImage.this).inflate(R.layout.gallery_rename_dialog, null);
        final EditText editText = (EditText) view.findViewById(R.id.editText);
        mOrgTitle = image.getTitle();
        if(mOrgTitle == null) {
            return;
        }
        /*
         * FIX BUG: 5005
         * FIX COMMENT: Modify the naming panorama mode , burst mode and gif type;
         * DATE: 2013-10-11
         */
        mIsPanorama = false;
        mIsBurst = false;
        mIsGif = false;
        if(mOrgTitle.startsWith(ImageListSpecial.PANORAMA_PREFIX)) {
            mOrgTitle = mOrgTitle.substring(ImageListSpecial.PANORAMA_PREFIX.length());
            mIsPanorama = true;
        } else if(mOrgTitle.startsWith(ImageListSpecial.BURST_PREFIX)) {
            mOrgTitle = mOrgTitle.substring(ImageListSpecial.BURST_PREFIX.length());
            mIsBurst = true;
        } else if (mOrgTitle.startsWith(ImageListSpecial.GIF_PREFIX)) {
            mOrgTitle = mOrgTitle.substring(ImageListSpecial.GIF_PREFIX.length());
            mIsGif = true;
        }
        editText.setText(mOrgTitle);
        final String suffix = Util.getSuffix(oriPath);

        OnClickListener listener = new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                switch (which) {
                case DialogInterface.BUTTON_POSITIVE:
                    String modifyName;
                    String str = editText.getText().toString();
                    if(mIsPanorama) {
                        modifyName = ImageListSpecial.PANORAMA_PREFIX.concat(str);
                    } else if(mIsBurst) {
                        modifyName = ImageListSpecial.BURST_PREFIX.concat(str);
                    } else if(mIsGif) {
                        modifyName = ImageListSpecial.GIF_PREFIX.concat(str);
                    } else {
                        modifyName = str;
                    }
                    final String fpath = file.getParentFile().getPath();
                    final File newFile = new File(fpath + "/"+ modifyName.concat(suffix));
                    if (newFile.exists()) {
                        Util.displayToast(ViewImage.this,getString(R.string.text_image_rename_file_exist));
                        ViewImageCommonUtil.setDialogDissmiss(dialog, false);
                    } else {
                        if (file.renameTo(newFile)) {
                            boolean sucessed = ImageManager.updateRenamedImage(ViewImage.this
                                            .getContentResolver(), newFile.getAbsolutePath(),
                                    modifyName, suffix, image ,mCurrentStoragePos);
                            if (sucessed) {
                                init(image.fullSizeImageUri());
                                /**
                                 * FIX BUG: 5930
                                 * FIX COMMENT:get the incorrect Image if not do this
                                 * Date: 2014-02-10
                                 */
                                if(mIsNeedThumbnail && mThumbnailGridView != null) {
                                    mThumbnailGridView.setImageList(mAllImages);
                                    mThumbnailGridView.onStart();
                                }
                                setImage(mCurrentPosition);
                                Util.displayToast(ViewImage.this, getString(R.string.text_image_rename_file_success));
                            } else {
                                Util.displayToast(ViewImage.this, getString(R.string.text_image_rename_file_failed));
                            }
                        } else {
                            Util.displayToast(ViewImage.this, getString(R.string.text_image_rename_file_failed));
                        }
                        ViewImageCommonUtil.setDialogDissmiss(dialog, true);
                    }
                    break;
                case DialogInterface.BUTTON_NEGATIVE:
                    ViewImageCommonUtil.setDialogDissmiss(dialog, true);
                    break;
                default:
                    break;
                }
            }
        };
        final AlertDialog renameDialog = new AlertDialog.Builder(ViewImage.this)
                .setTitle(R.string.text_image_rename)
                .setView(view)
                .setCancelable(false)
                .setPositiveButton(R.string.picture_select_ok, listener)
                .setNegativeButton(R.string.picture_select_cancel,listener)
                .create();
        renameDialog.show();
        editText.setFocusable(true);
        editText.setFocusableInTouchMode(true);
        editText.requestFocus();
        Timer timer = new Timer();
        timer.schedule(new TimerTask() {
            public void run() {
                InputMethodManager inputManager = (InputMethodManager) editText
                        .getContext().getSystemService(
                                Context.INPUT_METHOD_SERVICE);
                inputManager.showSoftInput(editText, 0);
            }
        }, 300);
        final Button positive = renameDialog.getButton(DialogInterface.BUTTON_POSITIVE);
        positive.setEnabled(false);
        TextWatcher mTextWatcher = new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                if(mOrgTitle.equals(editText.getText().toString()) || editText.getText().length() == 0
                        || (editText.getText().toString()).replace(" ", "").equals("")
                        || (editText.getText().toString()).replace(" ", "").equals(null)) {
                    positive.setEnabled(false);
                } else {
                    positive.setEnabled(true);
                }
            }
        };
        editText.addTextChangedListener(mTextWatcher);
    }

    private void handleRotateAction(IImage image, int rotate) {
        if (image == null || image.isReadonly()) {
            return;
        }
        image.rotateImageBy(rotate);
    }

    private void startShareMediaActivity(IImage image) {
//        Intent intent = new Intent();
//        intent.setAction(Intent.ACTION_SEND);
//        intent.setType(image.getMimeType());
//        intent.putExtra(Intent.EXTRA_STREAM, image.fullSizeImageUri());
//        try {
//            startActivity(Intent.createChooser(intent, getText(R.string.sendVideo)));
//        } catch (android.content.ActivityNotFoundException ex) {
//            Toast.makeText(this, R.string.no_way_to_share_video, Toast.LENGTH_SHORT).show();
//        }
        ShareVideoUtils.shareVideo(this, image);
    }
    void findUCamApp(String function) {
        if(mTipDialog == null) {
            OnClickListener listener = new OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    mTipDialog.dismiss();
                    mTipDialog = null;
                    switch (which) {
                        case DialogInterface.BUTTON_POSITIVE:
                            if (Util.checkNetworkShowAlert(ViewImage.this)) {
                                Intent intent = new Intent();
                                String ucamPkgName = getPackageName().replace("ugallery", "ucam");
                                intent.setData(Uri.parse("market://details?id=" + ucamPkgName));
                                try {
                                    startActivity(intent);
                                } catch (ActivityNotFoundException e) {
                                    Toast.makeText(ViewImage.this, R.string.text_not_installed_market_app, Toast.LENGTH_LONG).show();
                                }
                            }
                            break;
                    default:
                        break;
                }
            }
            };
            mTipDialog = new AlertDialog.Builder(this)
                    .setIcon(android.R.drawable.ic_dialog_alert)
                    .setTitle(R.string.text_prompt_install_ucam_title)
                    .setMessage(function.equals("share") ? R.string.text_function_sns_install_ucam : R.string.text_function_edit_install_ucam)
                    .setCancelable(false)
                    .setPositiveButton(R.string.picture_delete_ok, listener)
                    .setNegativeButton(R.string.picture_delete_cancel, listener)
                    .create();
            mTipDialog.show();
        }
    }

    long getImageFileSize(IImage image) {
        java.io.InputStream data = image.fullSizeImageData();
        if (data == null) {
            return -1;
        }
        try {
            return data.available();
        } catch (java.io.IOException ex) {
            return -1;
        } finally {
            try {
                data.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int deltaX = 0;
        final int action = event.getAction() & MotionEvent.ACTION_MASK;

        float startX = start.x;
        float startY = start.y;
        float endX = event.getX();
        float endY = event.getY();

        float distanceX = endX - startX;
        float distanceY = endY - startY;

        switch(action){
            case MotionEvent.ACTION_DOWN:
                mLastXposition = event.getRawX();
                savedMatrix.set(matrix);
                start.set(event.getX(), event.getY());
                start2.set(event.getX(), event.getY());
                mode = DRAG;
                break;

            case MotionEvent.ACTION_MOVE:
                if (mode == DRAG) {
                    deltaX = (int)(mLastXposition - event.getRawX());
                    if(mImageViews[1].getScale() > 1f){

                        if(startX == lastStartX && startY == lastStartY &&
                             endX == lastEndX && endY == lastEndY){
                            break;
                        }
                        lastStartX = startX;
                        lastStartY = startY;
                        lastEndX = endX;
                        lastEndY = endY;

                        mImageViews[1].postTranslateCenter(distanceX, distanceY);

                        start.set(event.getX(), event.getY());
                    }else{
                        populateMoveEvent(deltaX);
                        scheduleControlBar();
                    }

                } else if (mode == ZOOM) {
                    float newDist = spacing(event);
                    if (newDist > 10f) {
                        if(isPepper) {
                            scale = newDist / oldDist;
                        } else {
                            float distance = FloatMath.sqrt(mImageViews[1].getWidth()
                                * mImageViews[1].getWidth()
                                + mImageViews[1].getHeight()
                                * mImageViews[1].getHeight());
                            scale = 1 + (newDist - oldDist) / (distance * 2 / 3);
                        }

                        if((mImageViews[1].getScale() < 0.25f && scale <= 1) || mImageViews[1].getScale() >= 6f && scale >=1) {
                            break;
                        }

                        mImageViews[1].mSuppMatrix.postScale(scale, scale, mid.x, mid.y);
                        mImageViews[1].setImageMatrix(mImageViews[1].getImageViewMatrix());
                        mImageViews[1].center(true, true);
                    }
                    oldDist = spacing(event);
                }
                break;

            case MotionEvent.ACTION_UP:
                scrolled = true;

            case MotionEvent.ACTION_CANCEL:
                float myScale = mImageViews[1].getScale();
                if(myScale > 1) {
                    return false;
                }
                if(mode == DRAG){
                    deltaX = (int)(mLastXposition - event.getRawX());
                    stopMoveEvent(deltaX);
                }
                mode = NONE;
                break;

            case MotionEvent.ACTION_POINTER_UP:
                mode = NONE;
                break;

            case MotionEvent.ACTION_POINTER_DOWN:
                oldDist = spacing(event);
                if (oldDist > 10f) {
                    savedMatrix.set(matrix);
                    midPoint(mid, event);
                    mode = ZOOM;
                }

                deltaX = (int)(mLastXposition - event.getRawX());
                stopMoveEvent(deltaX);

                break;

            default:
        }
        return false;
    }

    private float spacing(MotionEvent event) {
        float x = event.getX(0) - event.getX(1);
        float y = event.getY(0) - event.getY(1);
        return FloatMath.sqrt(x * x + y * y);
    }

    private void midPoint(PointF point, MotionEvent event) {
        float x = event.getX(0) + event.getX(1);
        float y = event.getY(0) + event.getY(1);
        point.set(x / 2, y / 2);
    }

    /**
     * @param deltaX deltaX
     */
    public void populateMoveEvent(int deltaX) {
        int current = mCurrentPosition;
        int nextImagePos = -2;
        if (deltaX >= 0) {
            nextImagePos = current + 1;
        } else {
            nextImagePos = current - 1;
        }
        if (nextImagePos != -2) {
            if (nextImagePos < 0 || nextImagePos >= mCount) {
                return;
            }
        }
//        mScroller.scrollTo(mImageViews[1].getLeft() + deltaX, 0);
        mScroller.scrollTo(mImageViews[1].getLeft() + deltaX, 0);
    }

    public void stopMoveEvent(int deltaX) {
        scrolled = true;
        if (deltaX == 0) {
            return;
        }

        int width = mImageViews[1].getWidth();

        int current = mCurrentPosition;
        int sPadding = 20;
        int nextImagePosition = -2;

        if (deltaX >= 0) {
            nextImagePosition = current + 1;
        } else {
            nextImagePosition = current - 1;
        }

        if (nextImagePosition != -2 && mCount > 0) {
            if (nextImagePosition < 0 && (Math.abs(deltaX) >= width / 4)) {
//                mFirstToast.cancel();
                mFirstToast.show();
                return;
            }
            if (nextImagePosition >= mCount && (Math.abs(deltaX) >= width / 4)) {
//                mLastToast.cancel();
                mLastToast.show();
                return;
            }
        } else {
            return;
        }

        int from = deltaX <= 0 ? (width + sPadding) + mScroller.getScrollX() : mScroller
                .getScrollX()
                - (width + sPadding);

        if (Math.abs(deltaX) >= width / 4) {
            mScroller.scrollTo(from, 0);
        }
        int to = width + sPadding;
        animateScrollTo(to, 0);

        if (Math.abs(deltaX) >= width / 4) {
            if (deltaX >= width / 4) {
                nextImagePosition = current + 1;
            } else {
                nextImagePosition = current - 1;
            }
            if (nextImagePosition >= 0 && nextImagePosition < mCount) {
                synchronized (this) {
                    setImage(nextImagePosition);
                }
            }
        }
    }
    public void onImageTapped(int index) {
        setImage(index);
        if(mIsNeedThumbnail && mThumbnailGridView != null) {
            mThumbnailGridView.setSelectIndex(index);
        }
    }
    public void onLayoutComplete(boolean changed, int width) {
        /* SPRD: CID 108957 : Dereference after null check (FORWARD_NULL) @{ */
        if(mThumbnailGridView == null){
            return;
        }
        /* @} */
        if(mConfigurationChanged) {
            if(mRealWidth != width && mThumbnailGridView != null) {
                mThumbnailGridView.onStart();
            }else {
                mThumbnailGridView.setSelectIndex(mCurrentPosition);
                mThumbnailGridView.scrollToVisible(mCurrentPosition);
                mIsThumbnailLayoutComplete = true;
                mConfigurationChanged = false;
            }
        }
        mThumbnailGridView.setSelectIndex(mCurrentPosition);
        mThumbnailGridView.scrollBy(0, 0);
        mThumbnailGridView.invalidate();
        mIsThumbnailLayoutComplete = true;
    }
    public void onScroll(float scrollPosition) {
        mScrollPosition = scrollPosition;
    }
    public void drawImage(Canvas canvas, IImage image, Bitmap b, int xPos, int yPos, int w, int h) {
        if(b == null) {
            b = getErrorBitmap(image);
        }
        int width = b.getWidth();
        int height = b.getHeight();
        Bitmap old = b;
        b = b.copy(Config.ARGB_8888, true);
        Rect srcRect = new Rect(0, 0, width, height);
        Rect distRect = new Rect(0, 0, w, h);
        canvas.drawBitmap(b, srcRect, distRect, null);
        if(old != b) {
            Util.recyleBitmap(b);
        }
    }
    private Bitmap mMissingVideoThumbnailBitmap;
    private Bitmap mMissingImageThumbnailBitmap;
    private String mCurrentViewMode;
    public Bitmap getErrorBitmap(IImage image) {
        if (image == null || image.getMimeType() == null || ImageManager.isImage(image)) {
            if (mMissingImageThumbnailBitmap == null) {
                mMissingImageThumbnailBitmap = BitmapFactory.decodeResource(getResources(),
                        R.drawable.missing_thumbnail_picture, Util.getNativeAllocOptions());
            }
            return mMissingImageThumbnailBitmap;
        } else {
            if (mMissingVideoThumbnailBitmap == null) {
                mMissingVideoThumbnailBitmap = BitmapFactory.decodeResource(getResources(),
                        R.drawable.missing_thumbnail_video, Util.getNativeAllocOptions());
            }
            return mMissingVideoThumbnailBitmap;
        }
    }
}

class ScrollController extends LinearLayout {
    private Scroller mScroller;

    private ViewImage mViewImage;

    private int mWidth = -1;

    static final int baseScrollDuration = 1000;

    static final int sPadding = 20;

    public ScrollController(Context context) {
        super(context);

        mScroller = new Scroller(context);
        mViewImage = (ViewImage) context;
    }

    public ScrollController(Context context, AttributeSet attrs) {
        super(context, attrs);

        mScroller = new Scroller(context);
        mViewImage = (ViewImage) context;
    }

    public void startScrollTo(int newX, int newY) {
        int oldX = getScrollX();
        int oldY = getScrollY();

        int deltaX = newX - oldX;
        int deltaY = newY - oldY;

        if (mWidth == -1) {
            mWidth = findViewById(R.id.image2).getWidth();
        }

        int width = mWidth;

        int duration = width > 0 ? baseScrollDuration * Math.abs(deltaX) / width : 0;

        mScroller.startScroll(oldX, oldY, deltaX, deltaY, duration);
        invalidate();
    }

    @Override
    public void computeScroll() {
        boolean offseting = mScroller.computeScrollOffset();
        if (offseting) {
            scrollTo(mScroller.getCurrX(), mScroller.getCurrY());
            postInvalidate();
        }
    }

    @Override
    protected void onLayout(boolean hasChanged, int left, int top, int right, int bottom) {
        int width = right - left;
        int x = 0;
        for (View v : new View[] {
                findViewById(R.id.image1), findViewById(R.id.image2), findViewById(R.id.image3)
        }) {
            v.layout(x, 0, x + width, bottom);
            x += (width + sPadding);
        }

        findViewById(R.id.padding1).layout(width, 0, width + sPadding, bottom);
        findViewById(R.id.padding2).layout(width + sPadding + width, 0,
                width + sPadding + width + sPadding, bottom);

        if (hasChanged) {
            mViewImage.setImage(mViewImage.mCurrentPosition);
        }
    }
}

class ImageViewTouch extends ImageViewTouchBase {
    private final ViewImage mViewImage;

    public ImageViewTouch(Context context) {
        super(context);
        mViewImage = (ViewImage) context;
    }

    public ImageViewTouch(Context context, AttributeSet attrs) {
        super(context, attrs);
        mViewImage = (ViewImage) context;
    }

    protected int postTranslateCenter(float dx, float dy) {
        super.postTranslate(dx, dy);
        return center(true, true);
    }

    protected ScrollController scrollHandler() {
        return mViewImage.mScroller;
    }

    public boolean handleTouchEvent(MotionEvent m) {
        return mViewImage.mGestureDetector.onTouchEvent(m);
    }

    protected void scrollX(int deltaX) {
        scrollHandler().scrollBy(deltaX, 0);
    }

    protected int getScrollOffset() {
        return scrollHandler().getScrollX();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (mViewImage.mPaused) {
            return false;
        }
        return super.onKeyDown(keyCode, event);
    }
}

class BitmapCache implements ImageViewTouchBase.Recycler {
    // CID 109254 : UuF: Unused field (FB.UUF_UNUSED_FIELD)
    // private Bitmap mDefaultBitmap;
    public static class Entry {
        int mPos;

        Bitmap mBitmap;

        public Entry() {
            clear();
        }

        public void clear() {
            mPos = -1;
            mBitmap = null;
        }
    }

    private final Entry[] mCache;

    private Context _mContext;

    public BitmapCache(int size, Context _context) {
        _mContext = _context;
        mCache = new Entry[size];
        for (int i = 0; i < mCache.length; i++) {
            mCache[i] = new Entry();
        }
    }

    // Given the position, find the associated entry. Returns null if there is
    // no such entry.
    private Entry findEntry(int pos) {
        for (Entry e : mCache) {
            if (pos == e.mPos) {
                return e;
            }
        }
        return null;
    }

    // Returns the thumb bitmap if we have it, otherwise return null.
    public synchronized Bitmap getBitmap(int pos) {
        Entry e = findEntry(pos);
        if (e != null) {
            return e.mBitmap;
        } else {
            // return BitmapFactory.decodeResource(_mContext.getResources(),
            // R.drawable.im_default_quick_speed);
//            if (ViewImage.scrolled) {
//                // ViewImage.scrolled = false;
//                if(mDefaultBitmap == null || mDefaultBitmap.isRecycled()) {
//                    return BitmapFactory.decodeStream(_mContext.getResources().openRawResource(R.drawable.im_default_quick_speed));
//                }
//                return mDefaultBitmap;
//            }
        }

         return null;
    }

    public synchronized void put(int pos, Bitmap bitmap) {
        // First see if we already have this entry.
        if (findEntry(pos) != null) {
            return;
        }

        // Find the best entry we should replace.
        // See if there is any empty entry.
        // Otherwise assuming sequential access, kick out the entry with the
        // greatest distance.
        Entry best = null;
        int maxDist = -1;
        for (Entry e : mCache) {
            if (e.mPos == -1) {
                best = e;
                break;
            } else {
                int dist = Math.abs(pos - e.mPos);
                if (dist > maxDist) {
                    maxDist = dist;
                    best = e;
                }
            }
        }

        // Recycle the image being kicked out.
        // This only works because our current usage is sequential, so we
        // do not happen to recycle the image being displayed.
        /* SPRD: CID 108948 : Explicit null dereferenced (FORWARD_NULL) @{ */
        if (best != null && best.mBitmap != null) {
        // if (best.mBitmap != null) {
        /* @} */
//            best.mBitmap.recycle();
            System.gc();
            // CID 108948 : Dereference after null check (FORWARD_NULL)
            best.mPos = pos;
            best.mBitmap = bitmap;
        }
    }

    // Recycle all bitmaps in the cache and clear the cache.
    public synchronized void clear() {
        for (Entry e : mCache) {
            if (e.mBitmap != null) {
                e.mBitmap.recycle();
                System.gc();
            }
            e.clear();
        }
    }

    // Returns whether the bitmap is in the cache.
    public synchronized boolean hasBitmap(int pos) {
        Entry e = findEntry(pos);
        return (e != null);
    }

    // Recycle the bitmap if it's not in the cache.
    // The input must be non-null.
    public synchronized void recycle(Bitmap b) {
        for (Entry e : mCache) {
            if (e.mPos != -1) {
                if (e.mBitmap == b) {
                    return;
                }
            }
        }
        b.recycle();
    }
}
