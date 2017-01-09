/**
 * Copyright (C) 2010,2013 Thundersoft Corporation
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

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.TreeMap;


import android.app.Activity;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.FontMetrics;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.text.format.DateFormat;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.camera.storagepath.StorageUtil;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.ucamera.ugallery.R;
import com.ucamera.ugallery.gallery.IImage;
import com.ucamera.ugallery.gallery.IImageList;
import com.ucamera.ugallery.gallery.ImageLoader;
import com.ucamera.ugallery.gallery.VideoObject;
import com.ucamera.ugallery.gallery.privateimage.util.Constants;
import com.ucamera.ugallery.integration.Build;
import com.ucamera.ugallery.provider.UCamData;
import com.ucamera.ugallery.util.ImageManager;
import com.ucamera.ugallery.util.Models;
import com.ucamera.ugallery.util.UiUtils;
import com.ucamera.ugallery.util.Util;
import com.ucamera.ugallery.util.ViewImageCommonUtil;
import com.ucamera.ugallery.video.MovieView;
import com.ucamera.ugallery.panorama.UgalleryPanoramaActivity;
import com.android.camera.util.ToastUtil;

public class BaseImagePicker extends Activity
        implements GridViewSpecial.Listener,
                   GridViewSpecial.DrawAdapter,
                   View.OnClickListener {
    private static final String STATE_SCROLL_POSITION = "scroll_position";
    private static final String STATE_SELECTED_INDEX = "first_index";
    private static final String TAG = "BaseImagePicker";
    private static final float INVALID_POSITION = -1f;
    private ImageManager.ImageListParam mParam;
    private ImageListSpecial mImageListSpecial;
    private int mInclusion;
    private View mNoImagesView;
    private Dialog mMediaScanningDialog;
    private SharedPreferences mPrefs;
    private BroadcastReceiver mReceiver = null;
    private final Handler mHandler = new Handler();

    private boolean mLayoutComplete;
    private boolean mPausing = true;
    private ImageLoader mLoader;
    GridViewSpecial mGvs;
    private Uri mCropResultUri;
    // The index of the first picture in GridViewSpecial.
    private int mSelectedIndex = GridViewSpecial.INDEX_NONE;
    private float mScrollPosition = INVALID_POSITION;
    private boolean mConfigurationChanged = false;
    private List<IImage> mMultiSelected = null;
    //the number of the selected
    private int mSelectCount;
    private int mAllCount;
    private Paint mCheckPaint = null;
    //select panel object
    private Drawable mMultiSelectTrue;
    private Drawable mMultiSelectFalse;
    private Drawable mTitleMultiSelectTrue;
    private Drawable mTitleMultiSelectFalse;
    //  UShare: ushare;
    //    UGif: ugif;
    //  Puzzle: puzzle
    //  UPhoto: uphoto
    //    UCam: normal
    //UGallery: ugallery
    private String mEntry;
    protected int mUPhotoModule;
    private String mCurrentBucketId;
    private View mControlLayout;
    private View mControlNormalLayout;
    protected TextView mBtnSelect;
    private TextView mBtnCancel;
    private TextView mBtnAction; //OK or Delete
    // This is used to stop the worker thread.
    volatile boolean mAbort = false;
    private int mCurrentMode = 0; //0: grid, 1: time
    private TextView mDeleteTextView = null;
    private ArrayList<TimeModeItem> mTimeModeItems = null;
    public static boolean mIsLongPressed = false;
    public static final int CROP_MSG = 2;
    private long mVideoSizeLimit = Long.MAX_VALUE;
    private Bitmap mBurstIconBitmap = null;
    private Bitmap mPanoramaIconBitmap = null;
    private Bitmap mGifIconBitmap = null;
    private int mDistanceToBottom;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        /*SPRD:fix bug514045 Some Activity about UCamera lacks method of checkpermission@{ */
        if (!UCamUtill.checkPermissions(this)) {
            finish();
            return;
        }
        /* }@ */
        /*
         * SPRD: Bug 613037 image gallery display not file when changed setting with size. @{
         */
        String state = StorageUtil.getInstance().getStorageState();
        if (Environment.MEDIA_UNMOUNTED.equals(state)) {
            finish();
            return;
        }
        /* @} */
        UiUtils.initialize(this);
        Log.d(TAG, "ENTRY onCreate function.");
        mPrefs = PreferenceManager.getDefaultSharedPreferences(this);
        setContentView(R.layout.common_image_picker);
        mDistanceToBottom = UiUtils.dpToPixel(160);
        mWindowManager = (WindowManager) this.getSystemService("window");
        mNoImagesView = findViewById(R.id.no_images);
        mGvs = (GridViewSpecial) findViewById(R.id.grid);
        mGvs.setListener(this);
        mGvs.setOnTouchListener(new OnTouchListener(){

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch(event.getAction()){
                case MotionEvent.ACTION_DOWN:
                    if(mIsLongPressed){
                        handlerImageDown(event);
                    }
                    break;
                case MotionEvent.ACTION_MOVE:
                    if(mIsLongPressed){
                        setBottomLyoutParams(5, 80);
                        handleImageMove(event);
                    }
                    break;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                     if (mIsLongPressed) {
                         setBottomLyoutParams(0, 0);
                         handleImageUp(event);
                     }
                    break;
                }
                /*
                 * FIX BUG: 5936
                 * BUG COMMENT: not dispatch move event in long pressed mode
                 * DATE: 2014-02-28
                 */
                if(mIsLongPressed) {
                    if(event.getAction() == MotionEvent.ACTION_UP || event.getAction() == MotionEvent.ACTION_CANCEL) {
                        mIsLongPressed = false;
                    }
                    return true;
                }else {
                    return false;
                }
            }

        });
        mControlLayout = findViewById(R.id.layout_bottom_action);
        mControlNormalLayout = findViewById(R.id.layout_bottom_normal_action);
        mControlNormalLayout.setOnClickListener(this);
        mBtnAction = (TextView)findViewById(R.id.btn_multi_confirm);
        mBtnAction.setOnClickListener(this);
        findViewById(R.id.layout_multi_confirm).setOnClickListener(this);
        mBtnCancel = (TextView)findViewById(R.id.btn_multi_cancel);
        mBtnCancel.setOnClickListener(this);
        findViewById(R.id.layout_multi_cancel).setOnClickListener(this);
        mBtnSelect = (TextView) findViewById(R.id.btn_multi_all_select);
        mBtnSelect.setOnClickListener(this);
        findViewById(R.id.layout_multi_all_select).setOnClickListener(this);
        findViewById(R.id.btn_multi_collage).setOnClickListener(this);
        findViewById(R.id.gallery_gridview).setOnClickListener(this);
        RadioButton rb = (RadioButton)findViewById(R.id.gallery_timeview);
        rb.setOnClickListener(this);
        mDeleteTextView = (TextView) findViewById(R.id.btn_multi_sel_operate);
        mDeleteTextView.setOnClickListener(this);
//        mDeleteTextView = (TextView) findViewById(R.id.select_image_tv);
        mEntry = getIntent().getStringExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY);
        mUPhotoModule = getIntent().getIntExtra(ViewImage.EXTRA_UPHOTO_MODULE_KEY, 1);
        setViewStatusByEntry();
        mCurrentBucketId = getIntent().getStringExtra(ViewImage.EXTRACURRENT_BUCKID_KEY);
        String currentMode = getIntent().getStringExtra(ViewImage.EXTRA_VIEW_MODE);
        if(ViewImage.VIEW_MODE_TIME.equals(currentMode)) {
            mCurrentMode = 1;
            rb.setChecked(true);
        }
        if (isPickIntent()) {
            mVideoSizeLimit = getIntent().getLongExtra(
                    MediaStore.EXTRA_SIZE_LIMIT, Long.MAX_VALUE);
        } else {
            mVideoSizeLimit = Long.MAX_VALUE;
        }

        setupInclusion();
        if(mInclusion == ImageManager.INCLUDE_VIDEOS) {
            ((TextView)findViewById(R.id.nav_to_gallery)).setText(getString(R.string.video));
            findViewById(R.id.layout_multi_collage).setVisibility(View.GONE);
        }
        mLoader = new ImageLoader(getContentResolver(), mHandler);
        Log.d(TAG, "LEAVE onCreate function.");
    }
    private void setBottomLyoutParams(int top, int bottom) {
        if(Models.getModel().equals(Models.AMAZON_KFTT)) {
            RelativeLayout layout = (RelativeLayout) findViewById(R.id.layout_bottom_action_root);
            RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams)layout.getLayoutParams();
            params.topMargin = top;
            params.bottomMargin = bottom;
            layout.setLayoutParams(params);
        }
    }
    private Runnable mDeleteGridImageRunnable = new Runnable(){
        @Override
        public void run() {
            if(mImageListSpecial != null){
                new DeleteImageTask(mImageListSpecial.getCountAtPosition(mSelectedItemIndex)).execute(mImageListSpecial.getAllImageAtIndex(mSelectedItemIndex));
            }
        }
    };

    String getGalleryEntry() { return mEntry; }
    int getUphotoModule()    {return mUPhotoModule;};

    private void setViewStatusByEntry() {
        if(ViewImage.IMAGE_ENTRY_NORMAL_VALUE.equals(mEntry) || isUGallery()) {
            findViewById(R.id.nav_to_album).setVisibility(View.VISIBLE);
            findViewById(R.id.nav_to_album).setOnClickListener(this);
            mControlLayout.setVisibility(View.GONE);
            mControlNormalLayout.setVisibility(View.VISIBLE);
            Util.setViewStatus(this, mMultiSelected);
        } else if(ViewImage.IMAGE_ENTRY_PUZZLE_VALUE.equals(mEntry) || ViewImage.IMAGE_ENTRY_UGIF_VALUE.equals(mEntry)) {
            showSelectPhotosTag();
            mControlLayout.setVisibility(View.VISIBLE);
            mControlNormalLayout.setVisibility(View.GONE);
            findViewById(R.id.layout_gallery_mode).setVisibility(View.GONE);
            findViewById(R.id.layout_multi_all_select).setVisibility(View.GONE);
            findViewById(R.id.layout_multi_collage).setVisibility(View.GONE);
            Util.setTextAndDrawableTop(getApplicationContext(), mBtnAction, R.drawable.gallery_ok_status, R.string.picture_select_ok);
            Util.setTextAndDrawableTop(getApplicationContext(), mBtnCancel, R.drawable.gallery_cancel_status, R.string.picture_select_cancel);
            Util.setViewStatus(this, mMultiSelected);
        } else if(ViewImage.IMAGE_ENTRY_UPHOTO_VALUE.equals(mEntry)) {
            showSelectPhotosTag();
            mControlLayout.setVisibility(View.GONE);
            mControlNormalLayout.setVisibility(View.GONE);
            findViewById(R.id.layout_gallery_mode).setVisibility(View.GONE);
        } else  if(ViewImage.IMAGE_ENTRY_PICKER_VALUE.equals(mEntry)) {
            showSelectPhotosTag();
            mControlLayout.setVisibility(View.GONE);
            mControlNormalLayout.setVisibility(View.GONE);
        }else{
            /*
             * FIX BUG: 1851
             * BUG CAUSE: The third-part app pick image from UGallery, the control bar is not gone;
             * FIX COMMENT: gone the control bar;
             * DATE: 2012-11-07
             */
            mControlLayout.setVisibility(View.GONE);
            mControlNormalLayout.setVisibility(View.GONE);
            /*
             * FIX BUG: 5490
             * BUG CAUSE: The third-part app pick image from UGallery, nav_to_album View has no click listener;
             * FIX COMMENT: gone the control bar;
             * DATE: 2013-12-04
             */
            findViewById(R.id.nav_to_album).setVisibility(View.VISIBLE);
            findViewById(R.id.nav_to_album).setOnClickListener(this);
        }
    }

    private void showSelectPhotosTag(){
        findViewById(R.id.nav_to_album).setVisibility(View.GONE);
        findViewById(R.id.nav_to_gallery).setVisibility(View.GONE);
        findViewById(R.id.nav_to_gallery_arrow).setVisibility(View.GONE);
        findViewById(R.id.nav_to_select_photos).setVisibility(View.VISIBLE);
    }
    /**
     * clear the current state
     */
    @Override
    protected void onRestart() {
        super.onRestart();
        Log.d(TAG, "ENTRY onRestart function.");
//        mBtnAction.setEnabled(false);
//        mBtnCancel.setEnabled(false);
//        mSelectCount = 0;
        Log.d(TAG, "LEAVE onRestart function.");
    }

    public void onClick(View v) {
        int viewId = v.getId();
        switch (viewId) {
            case R.id.btn_multi_sel_operate:
                openMultiSelectMode();
                rebake(false, ImageManager.isMediaScannerScanning(getContentResolver()), false);
                break;
            case R.id.nav_to_album:
                    gotoAlbum();
                break;
            case R.id.btn_multi_confirm:
                if(ViewImage.IMAGE_ENTRY_NORMAL_VALUE.equals(mEntry) || ViewImage.IMAGE_ENTRY_UGALLERY_VALUE.equals(mEntry)) {
                    onDeleteMultipleClicked();
                } else if(ViewImage.IMAGE_ENTRY_PUZZLE_VALUE.equals(mEntry) || ViewImage.IMAGE_ENTRY_UGIF_VALUE.equals(mEntry)) {
                    selectDone(mMultiSelected);
                } else {
                    onSelectDone(mMultiSelected);
                }
                break;
            case R.id.btn_multi_cancel:
                if(ViewImage.IMAGE_ENTRY_PUZZLE_VALUE.equals(mEntry) || ViewImage.IMAGE_ENTRY_UGIF_VALUE.equals(mEntry)) {
                    if(mSelectCount > 0) {
                        for(int i = mSelectCount - 1; i > -1; i--) {
                            IImage image = mMultiSelected.get(i);
                            toggleMultiSelected(image);
                        }
                    }
                } else {
                    if(mBtnSelect.isEnabled() == false) {
                        mBtnSelect.setEnabled(true);
                    }
                    if(isGridMode()) {
                        selectAllItem(true);
                    } else {
                        toggleCheckboxStatus(false);
                    }
                }
                break;
            case R.id.btn_multi_all_select:
                //false;
                if(isGridMode()) {
                    selectAllItem(!mBtnSelect.isEnabled());
                } else {
                    toggleCheckboxStatus(true);
                }
                break;
            case R.id.btn_multi_collage:
                goToCollageActivity();
                break;
            case R.id.gallery_gridview:
                if(isGridMode()) {
                    return;
                }
                mCurrentMode = 0;
                mScrollPosition = 0 ;
                mGvs.resetPosition();
                if(isInMultiSelectMode()) {
                    closeMultiSelectMode();
                }
                rebake(false, ImageManager.isMediaScannerScanning(getContentResolver()));
                break;
            case R.id.gallery_timeview:
                if(!isGridMode()) {
                    return;
                }
                mScrollPosition = 0 ;
                mGvs.resetPosition();
                mCurrentMode = 1;
                if(isInMultiSelectMode()) {
                    closeMultiSelectMode();
                }
                mSelectCount = 0;
                rebake(false, ImageManager.isMediaScannerScanning(getContentResolver()));
                break;
            default:
                break;
        }
    }

    boolean isGridMode() { return mCurrentMode == 0; }

    boolean isUGallery() {return ViewImage.IMAGE_ENTRY_UGALLERY_VALUE.equals(mEntry);}
    boolean isThirdPicker() {return ViewImage.IMAGE_ENTRY_PICKER_VALUE.equals(mEntry);}
    private void  goToCollageActivity() {
       if (mMultiSelected.size() < 2) {
           Toast.makeText(this,
                   R.string.text_collage_min_select_notice,
                   Toast.LENGTH_SHORT).show();
           return;
       } else if (mMultiSelected.size() > 9) {
           // SPRD: Fix bug 580271 Toast long time display
           ToastUtil.showToast(this, R.string.text_collage_max_select_notice, Toast.LENGTH_LONG);
           return;
       } else {
           try {
               Uri mUriList[] = new Uri[mMultiSelected.size()];
               Intent intent = new Intent();
               intent.setClassName(this,
                       "com.ucamera.ucomm.puzzle.PuzzleActivity");
               for (int i = 0; i < mMultiSelected.size(); i++) {
                   mUriList[i] = mMultiSelected.get(i).fullSizeImageUri();
               }
               intent.putExtra("ucam.puzzle.IMAGES", mUriList);
               startActivity(intent);
               finish();
           } catch (Exception e) {
           }
       }
    }
    /*
     * FIX BUG: 1389
     * FIX COMMENT: To determine all of the current album pictures are deleted.
     * DATE: 2012-07-31
     */
    private void gotoAlbum() {
        /*SPRD:fix bug533979 unmount SD card, imageGallery can not select photo
        ImageGallery.showImagePicker(this, BaseImagePicker.class, getGalleryEntry());
        */
        if(! isUGallery()) {
            finish();
        }
        //SPRD:fix bug533979 unmount SD card, imageGallery can not select photo
        ImageGallery.showImagePicker(this, BaseImagePicker.class, getGalleryEntry());
    }

    private void selectAllItem(boolean isDeselectAll) {
        /*
         * FIX BUG: 6199
         * FIX COMMENT: avoid null pointer exception
         * DATE: 2014-04-11
         */
        if(mImageListSpecial == null) return;
        int count = mImageListSpecial.getCount();
        Log.d(TAG, "selectAllItem(): count = " + count);
        for (int index = 0; index < count; index++) {
            if (isInMultiSelectMode()) {
                mGvs.setSelectedIndex(GridViewSpecial.INDEX_NONE);
                toggleMultiSelected(mImageListSpecial.getImageAtIndex(index), true, isDeselectAll);
            }
        }
    }

    boolean canHandleEvent() {
        // Don't process event in pause state.
        return (!mPausing) && (mLayoutComplete);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (!canHandleEvent()) {
            return false;
        }
        if(keyCode == KeyEvent.KEYCODE_BACK) {
            if((ViewImage.IMAGE_ENTRY_NORMAL_VALUE.equals(mEntry) || ViewImage.IMAGE_ENTRY_UGALLERY_VALUE.equals(mEntry)) && isInMultiSelectMode()) {
                if(isGridMode()) {
                    selectAllItem(true);
                } else {
                    toggleCheckboxStatus(false);
                }
                closeMultiSelectMode();
                rebake(false, ImageManager.isMediaScannerScanning(getContentResolver()), false);
                return true;
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onPause() {
        super.onPause();
        Log.d(TAG, "ENTRY onPause function.");
        mPausing = true;
        mLoader.stop();
        mGvs.stop();

        if (mReceiver != null) {
            unregisterReceiver(mReceiver);
            mReceiver = null;
        }

        // Now that we've paused the threads that are using the cursor it is
        // safe to close it.
        ImageListSpecial imagelist = mImageListSpecial;
        if(imagelist != null) {
            imagelist.close();
            mImageListSpecial = null;
        }
        if(mBurstIconBitmap != null) {
            mBurstIconBitmap.recycle();
            mBurstIconBitmap = null;
        }
        Log.d(TAG, "LEAVE onPause function.");
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "ENTRY onDestroy function.");
        if(mMultiSelected != null) {
            mMultiSelected.clear();
        }
        mSelectCount = 0;
        Log.d(TAG, "LEAVE onDestroy function.");
    }

    @Override
    protected void onStop() {
        Log.d(TAG, "ENTRY onStop function.");
        super.onStop();
        /*if(mMultiSelected != null) {
            mMultiSelected.clear();
        }
        mSelectCount = 0;*/
        Log.d(TAG, "LEAVE onStop function.");
    }
    private void rebake(boolean unmounted, boolean scanning) {
        rebake(unmounted, scanning, true);
    }

    private void rebake(boolean unmounted, boolean scanning, boolean needScan) {
        mGvs.stop();
        if (needScan && mImageListSpecial != null) {
            mImageListSpecial.close();
            mImageListSpecial = null;
        }

        if (mMediaScanningDialog != null) {
            mMediaScanningDialog.cancel();
            mMediaScanningDialog = null;
        }

        if (scanning) {
            mMediaScanningDialog = ProgressDialog.show(this, null, getResources().getString(
                    R.string.text_waiting), true, true);
        }

        Log.d(TAG,"Do actually rebake");

        // for some device(such as LA-M1), when set internal storage is master,
        // then the scanning status is always true
        // Just ignore the scanning status
        mParam = allImages(!unmounted /*&& !scanning*/);
        boolean filter = false;
        if(!Util.isSelectPicMode(mEntry)) {
            filter = true;
        }
        if(needScan) {
            mImageListSpecial = new ImageListSpecial(ImageManager.makeImageList(getContentResolver(), mParam), filter);
        }else {
            if(mImageListSpecial == null) {
                mImageListSpecial = new ImageListSpecial(ImageManager.makeImageList(getContentResolver(), mParam), filter);
            }
            mImageListSpecial.setFilter(!isInMultiSelectMode());
        }

        if (mImageListSpecial != null) {
            mAllCount = mImageListSpecial.getCount();
        }
        if (mMediaScanningDialog != null) {
            mMediaScanningDialog.cancel();
            mMediaScanningDialog = null;
        }

        mGvs.setImageList(mImageListSpecial);
        mGvs.setDrawAdapter(this);
        mGvs.setLoader(mLoader);
        mGvs.setDisplayMode(mCurrentMode);
        if(isGridMode()) {
            mGvs.resetTimeModeItems();
        } else {
            mGvs.resetTimeModeItems();
            regroupImages(mImageListSpecial , this);
            mGvs.setTimeModeItems(mTimeModeItems);
        }
        mGvs.start();
        if(scanning) {
            mNoImagesView.setVisibility(View.GONE);
        } else {
            if(mAllCount > 0) {
                mNoImagesView.setVisibility(View.GONE);
            } else {
                gotoAlbum();
            }
        }
    }

    private void regroupImages(ImageListSpecial imageList, Activity activity) {
        int count = imageList.getCount();
        TreeMap<String, TimeModeItem> treeMap = new TreeMap<String, TimeModeItem>();
        final SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd");
        int pos = 0;
        int picCount = 0;
        ArrayList<TimeModeItem> items = new ArrayList<TimeModeItem>();
        for(int index = 0; index < count; index++) {
            IImage image = imageList.getImageAtIndex(index);
            picCount = imageList.getCountAtPosition(index);
            String date = ViewImageCommonUtil.getImageDateTime(activity, image, true);
            TimeModeItem timeModeItem = null;
            if(treeMap.containsKey(date)) {
                timeModeItem = treeMap.get(date);
                timeModeItem.setCount(1 + pos++);
                timeModeItem.addPictureCount(picCount);
            } else {
                pos = 1;
                String week = null;
                try {
                    week = new SimpleDateFormat("EEEE").format(sdf.parse(date));
                } catch (ParseException e) {
                    e.printStackTrace();
                    week = "";
                }
                timeModeItem = new TimeModeItem(date, week, pos, picCount);
                treeMap.put(date, timeModeItem);
                items.add(timeModeItem);
            }

            /*
             * BUG FIX: 2022
             * FIX COMMENT: update the selected item when rebake
             * DATE: 2012-11-28
             */
            if (mMultiSelected!= null && mMultiSelected.contains(image)){
                timeModeItem.addItemToDateList(image);
            }
        }

        treeMap.clear();
        mTimeModeItems = items;
    }

    @Override
    protected void onSaveInstanceState(Bundle state) {
        super.onSaveInstanceState(state);
        state.putFloat(STATE_SCROLL_POSITION, mScrollPosition);
        state.putInt(STATE_SELECTED_INDEX, mSelectedIndex);
    }

    @Override
    protected void onRestoreInstanceState(Bundle state) {
        super.onRestoreInstanceState(state);
        mScrollPosition = state.getFloat(STATE_SCROLL_POSITION, INVALID_POSITION);
        mSelectedIndex = state.getInt(STATE_SELECTED_INDEX, 0);
    }

    @Override
    public void onResume() {
        super.onResume();
        Log.d(TAG, "ENTRY onResume function.");
        mGvs.setSizeChoice(Integer.parseInt(mPrefs.getString("pref_gallery_size_key", "1")));
        mGvs.requestFocus();

        mPausing = false;

        // install an intent filter to receive SD card related events.
        IntentFilter intentFilter = new IntentFilter(Intent.ACTION_MEDIA_MOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_SCANNER_STARTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_SCANNER_FINISHED);
        intentFilter.addAction(Intent.ACTION_MEDIA_EJECT);
        intentFilter.addDataScheme("file");

        mReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (action.equals(Intent.ACTION_MEDIA_MOUNTED)) {
                    // SD card available
                    // put up a "please wait" message
                    // also listen for the media scanner finished message
                } else if (action.equals(Intent.ACTION_MEDIA_UNMOUNTED)) {
                    // SD card unavailable
                    rebake(true, false);
                } else if (action.equals(Intent.ACTION_MEDIA_SCANNER_STARTED)) {
                    rebake(false, true);
                } else if (action.equals(Intent.ACTION_MEDIA_SCANNER_FINISHED)) {
                    rebake(false, false);
                } else if (action.equals(Intent.ACTION_MEDIA_EJECT)) {
                    rebake(true, false);
                }
            }
        };
        registerReceiver(mReceiver, intentFilter);
        rebake(false, ImageManager.isMediaScannerScanning(getContentResolver()));
        if(mEntry != null && (mEntry.equals(ViewImage.IMAGE_ENTRY_PUZZLE_VALUE) || mEntry.equals(ViewImage.IMAGE_ENTRY_UGIF_VALUE))) {
            openMultiSelectMode();
        }
        initCellSize();

        /* SPRD:Fix bug 522400 @{ */
        if (mMultiSelected != null && mMultiSelected.size() > 0) {
            List<IImage> list = new ArrayList<IImage>();
            if (mImageListSpecial != null) {
                int count = mImageListSpecial.getCount();
                for (int index = 0; index < count; index++) {
                    list.add(mImageListSpecial.getImageAtIndex(index));
                }
            }
            List delList = new ArrayList();
            for (IImage tmp : mMultiSelected) {
                if(!list.contains(tmp)) {
                    delList.add(tmp);
                }
            }
            mMultiSelected.removeAll(delList);

            mSelectCount = mMultiSelected.size();
            //SPRD : bug fix 535329,getminlimit on resume
            if (mSelectCount < getMinLimit()) {
                mBtnAction.setEnabled(false);
            }
            if (mSelectCount <= 0) {
                mBtnCancel.setEnabled(false);
            }
        }
        /* @} */

        Log.d(TAG, "LEAVE onResume function.");
    }

    private boolean isImageType(String type) {
        return type.equals("vnd.android.cursor.dir/image") || type.equals("image/*");
    }

    private boolean isVideoType(String type) {
        return type.equals("vnd.android.cursor.dir/video")
                || type.equals("video/*");
    }
    // According to the intent, setup what we include (image/video) in the
    // gallery and the title of the gallery.
    private void setupInclusion() {
        mInclusion = ImageManager.INCLUDE_IMAGES | ImageManager.INCLUDE_VIDEOS;

        Intent intent = getIntent();
        if (intent != null) {
            String type = intent.resolveType(this);
            Bundle extras = intent.getExtras();
            if (type != null) {
                if (isImageType(type)) {
                    mInclusion = ImageManager.INCLUDE_IMAGES;
                }
                if (isVideoType(type)) {
                    mInclusion = ImageManager.INCLUDE_VIDEOS;
                }
            }

            if (extras != null) {
                mInclusion = (ImageManager.INCLUDE_IMAGES
                        | ImageManager.INCLUDE_VIDEOS)
                        & extras.getInt("mediaTypes", mInclusion);
            }
        }
    }

    // Returns the image list parameter which contains the subset of image/video
    // we want.
    private ImageManager.ImageListParam allImages(boolean storageAvailable) {
        if (!storageAvailable) {
            return ImageManager.getEmptyImageListParam();
        } else {
            Uri uri = getIntent().getData();
            return ImageManager.getImageListParam(ImageManager.DataLocation.ALL, mInclusion,
                    ImageManager.SORT_DESCENDING,
                    mInclusion == ImageManager.INCLUDE_VIDEOS ? mCurrentBucketId :
                        ((uri != null) ? uri.getQueryParameter("bucketId") : null));
        }
    }

    /**
     * add or remove current IImage object and then refresh the screen
     * @param image current image object
     */
    private void toggleMultiSelected(IImage image) {
        if(mMultiSelected.contains(image)){
            mMultiSelected.remove(image);
        } else {
            final int maxLimit = getMaxLimit();
            if( maxLimit < 0 || mSelectCount < maxLimit) {
                /*
                 * FIX BUG: 5456
                 * FIX COMMENT: Select the damage photos, should have the prompt Message
                 * Date: 2013-12-03
                 */
                Object[] Obj = image.miniThumbBitmap();
                if (Obj[1] == null) {
                    Toast.makeText(getApplicationContext(), R.string.text_image_damage, Toast.LENGTH_LONG).show();
                    return;
                } else {
                    mMultiSelected.add(image);
                }
            } else {
                // SPRD: Fix bug 580271 Toast long time display
                ToastUtil.showToast(this, getString(R.string.text_gif_max_select_count, maxLimit), Toast.LENGTH_LONG);
                return;
            }
        }

        mSelectCount = mMultiSelected.size();
        final int minLimit = getMinLimit();
        if(mSelectCount > 0) {
            mBtnCancel.setEnabled(true);
            if(minLimit < 0 || mSelectCount >= minLimit) {
                mBtnAction.setEnabled(true);
            } else {
                mBtnAction.setEnabled(false);
            }
        } else {
            mBtnCancel.setEnabled(false);
            mBtnAction.setEnabled(false);
        }
        mGvs.invalidate();
    }

    private void toggleMultiSelected(IImage image, boolean isAllSelect, boolean isDeselectAll) {
        if(isAllSelect) {
            if(isDeselectAll) {
                mBtnSelect.setEnabled(true);
                if(mMultiSelected.contains(image)){
                    mMultiSelected.remove(image);
                }
            } else {
                mBtnSelect.setEnabled(false);
                if(!mMultiSelected.contains(image)) {
                    mMultiSelected.add(image);
                }
            }
        } else {
            mBtnSelect.setEnabled(true);
            if(mMultiSelected.contains(image)){
                mMultiSelected.remove(image);
            }else{
                mMultiSelected.add(image);
            }
        }
        Util.setViewStatus(this, mMultiSelected);
        if (mMultiSelected != null && mMultiSelected.size() == mImageListSpecial.getCount()) {
            mBtnSelect.setEnabled(false);
        }
        mGvs.invalidate();
    }

    @Override
    public void onTitleTapped(int row) {
        if(isInMultiSelectMode()) {
            TreeMap<Integer, Integer> timeMap = mGvs.getImageIndexMap();
            TreeMap<Integer, TimeModeItem> titleMap = mGvs.getTitleRowMap();
            TimeModeItem item = titleMap.get(row);
            int count = item.getCount();
            int pos = findPosByRow(timeMap, row + 1);
            int temp = 0;
            for(int index = 0; index < count; index++) {
                IImage image = mImageListSpecial.getImageAtIndex(pos + index);
                if(mMultiSelected.contains(image)) {
                    temp++;
                }
            }

            for(int index = 0; index < count; index++) {
                IImage image = mImageListSpecial.getImageAtIndex(pos + index);
                if(temp == count) {
                    //remove
                    if(mMultiSelected.contains(image)) {
                        mMultiSelected.remove(image);
                        item.removeItemFromDateList(image);
                    }
                } else {
                    //add
                    if(!mMultiSelected.contains(image)) {
                        mMultiSelected.add(image);
                        item.addItemToDateList(image);
                    }
                }
            }

            mBtnSelect.setEnabled(mMultiSelected.size() < mAllCount);
            Util.setViewStatus(BaseImagePicker.this, mMultiSelected);

            mGvs.invalidate();
        }
    }

    private int findPosByRow(TreeMap<Integer, Integer> timeMap, int row) {
        int pos = 0;
        for(Integer index : timeMap.keySet()) {
            if(index == row) {
                break;
            }
            pos += timeMap.get(index);
        }
        return pos;
    }

    public int findRowByPos(TreeMap<Integer, Integer> timeMap, int pos) {
        int row = 0;
        int curCount = 0;
        for(Integer rowIndex: timeMap.keySet()) {
            curCount += timeMap.get(rowIndex);
            if(pos < curCount) {
                row = rowIndex;
                break;
            }
        }
        return row;
    }

    public int findTitleRowByPos(TreeMap<Integer, TimeModeItem> titleMap, int index) {
        TreeMap<Integer, Integer> timeMap = mGvs.getImageIndexMap();
        int row = findRowByPos(timeMap, index);
        int targetRow = 0;
        for(int best = row - 1; best >= 0; best--) {
            if(titleMap.containsKey(best)) {
                targetRow = best;
                break;
            }
        }

        return targetRow;
    }

    /**
     * @param index index
     */
    public void onImageTapped(int index) {
        Log.d(TAG, "onImageTapped(): index = " + index);
        if (isInMultiSelectMode()) {
            mGvs.setSelectedIndex(GridViewSpecial.INDEX_NONE);
            if(needOnOffCheckbox()) {
                if(isGridMode()) {
                    toggleMultiSelected(mImageListSpecial.getImageAtIndex(index), false, false);
                } else {
                    onImageClick(mImageListSpecial.getImageAtIndex(index), index);
                }
            } else {
                toggleMultiSelected(mImageListSpecial.getImageAtIndex(index));
            }
        } else {
            onImageClicked(index);
        }
    }

    private void onImageClick(IImage image, int index) {
        TreeMap<Integer, TimeModeItem> titleMap = mGvs.getTitleRowMap();
        int targetRow = findTitleRowByPos(titleMap, index);

        TimeModeItem item = titleMap.get(targetRow);
        if(mMultiSelected.contains(image)){
            mMultiSelected.remove(image);
        }else{
            mMultiSelected.add(image);
        }
        ArrayList<IImage> curDateSelectedList = item.getCurDateSelectedList();
        if(curDateSelectedList.contains(image)) {
            item.removeItemFromDateList(image);
        } else {
            item.addItemToDateList(image);
        }

        mBtnSelect.setEnabled(mMultiSelected.size() < mAllCount);
        Util.setViewStatus(this, mMultiSelected);
    }

    public void onImageClicked(int index) {
        if (index < 0 || index >= mImageListSpecial.getCount()) {
            return;
        }
        mSelectedIndex = index;
        mGvs.setSelectedIndex(index);

        IImage image = mImageListSpecial.getImageAtIndex(index);

        if (isInMultiSelectMode()) {
            toggleMultiSelected(image);
            return;
        }

        if(ViewImage.IMAGE_ENTRY_PICKER_VALUE.equals(mEntry)) {
            launchCropperOrFinish(image);
        } else if(ViewImage.IMAGE_ENTRY_UPHOTO_VALUE.equals(mEntry)) {
            onSelected(image);
        } else {
           if(isGridMode()) {
                Intent intent = null;
                if (ImageManager.isVideo(image)) {
                    intent = new Intent(this, ViewImage.class);
                    intent.setData(image.fullSizeImageUri());
//                    intent.putExtra(MediaStore.EXTRA_SCREEN_ORIENTATION, ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
                    /*
                     * FIX BUG: 5614
                     * FIX COMMENT: Pass the mCurrentBucketId to the viewImage
                     * Date: 2013-11-16
                     */
                    intent.putExtra(ViewImage.EXTRACURRENT_BUCKID_KEY, mCurrentBucketId);
                    intent.putExtra(ViewImage.VIDEO_VIEW, true);
                } else {
                    if(isPanorama(image.getDataPath())){
                        intent = new Intent(this, UgalleryPanoramaActivity.class);
                    } else {
                        intent = new Intent(this, ViewImage.class);
                    }
                    intent.setData(image.fullSizeImageUri());
                    intent.putExtra(ViewImage.EXTRACURRENT_BUCKID_KEY, mCurrentBucketId);
                }
                intent.putExtra(ViewImage.EXTRA_IMAGE_LIST_KEY, mParam);
                if (isUGallery()) {
                    intent.putExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY, ViewImage.IMAGE_ENTRY_UGALLERY_VALUE);
                } else {
                    intent.putExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY, mEntry);
                }
                startActivity(intent);
                overridePendingTransition(0, 0);
                if (!isUGallery()) {
                    finish();
                }
            } else {
                Intent intent = null;
                if (ImageManager.isVideo(image)) {
                    intent = new Intent(this, ViewImage.class);
                    intent.setData(image.fullSizeImageUri());
//                    intent.putExtra(MediaStore.EXTRA_SCREEN_ORIENTATION, ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
                    /*
                     * FIX BUG: 5614
                     * FIX COMMENT: Pass the mCurrentBucketId to the viewImage
                     * Date: 2013-11-16
                     */
                    intent.putExtra(ViewImage.EXTRACURRENT_BUCKID_KEY, mCurrentBucketId);
                    intent.putExtra(ViewImage.VIDEO_VIEW, true);
                    intent.putExtra(ViewImage.EXTRA_IMAGE_LIST_KEY, mParam);

                } else {
                    TreeMap<Integer, TimeModeItem> titleMap = mGvs.getTitleRowMap();
                    int targetRow = findTitleRowByPos(titleMap, index);
                    String date = titleMap.get(targetRow).getDate();
                    if(isPanorama(image.getDataPath())){
                        intent = new Intent(BaseImagePicker.this, UgalleryPanoramaActivity.class);
                    } else {
                        intent = new Intent(BaseImagePicker.this, ViewImage.class);
                    }
                    intent.putExtra(ViewImage.EXTRA_IMAGE_LIST_KEY, ImageManager.getImageListParam(date, mCurrentBucketId));
                    intent.setData(image.fullSizeImageUri());
                    intent.putExtra(ViewImage.EXTRACURRENT_BUCKID_KEY, mCurrentBucketId);
                }
                /*
                 * FIX BUG: 5667
                 * FIX COMMENT: Pass the ViewImage.EXTRA_IMAGE_ENTRY_KEY and ViewImage.EXTRA_VIEW_MODE to the viewImage
                 * Date: 2013-12-20
                 */
                intent.putExtra(ViewImage.EXTRA_VIEW_MODE, ViewImage.VIEW_MODE_TIME);
                if (isUGallery()) {
                    intent.putExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY, ViewImage.IMAGE_ENTRY_UGALLERY_VALUE);
                } else {
                    intent.putExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY, mEntry);
                }
                startActivity(intent);
                overridePendingTransition(0, 0);
                if (!isUGallery()) {
                    this.finish();
                }

            }
        }
    }

    private boolean isPanorama(String path){
        String namePic = path.substring(path.lastIndexOf("/") + 1);
        if(namePic != null && namePic.startsWith("PANORAMA")){
            return true;
        }
        return false;
    }

    private boolean isPickIntent() {
        return ViewImage.IMAGE_ENTRY_UPHOTO_VALUE.equals(mEntry);
    }

    private String pickImageEntry() {
        if(ViewImage.IMAGE_ENTRY_UPHOTO_VALUE.equals(mEntry)) {
            return mEntry;
        } else {
            return getIntent().getAction();
        }
    }

    private void launchCropperOrFinish(IImage img) {
        Bundle myExtras = getIntent().getExtras();

        String cropValue = myExtras != null ? myExtras.getString("crop") : null;
        if (cropValue != null) {
            Bundle newExtras = new Bundle();
            if (cropValue.equals("circle")) {
                newExtras.putString("circleCrop", "true");
            }

            Intent cropIntent = new Intent();
            cropIntent.setData(img.fullSizeImageUri());
            cropIntent.setClass(this, CropImage.class);
            cropIntent.putExtras(newExtras);

            /* pass through any extras that were passed in */
            cropIntent.putExtras(myExtras);
            startActivityForResult(cropIntent, CROP_MSG);
        } else {
            Intent result = new Intent(null, img.fullSizeImageUri());
            if (myExtras != null && myExtras.getBoolean("return-data")) {
                // The size of a transaction should be below 100K.
                Bitmap bitmap = img.fullSizeBitmap(IImage.UNCONSTRAINED, 100 * 1024);
                if (bitmap != null) {
                    result.putExtra("data", bitmap);
                }
            }
            setResult(RESULT_OK, result);
            finish();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case MenuHelper.RESULT_COMMON_MENU_CROP:
                if (resultCode == RESULT_OK) {

                    // The CropImage activity passes back the Uri of the cropped
                    // image as the Action rather than the Data.
                    // We store this URI so we can move the selection box to it
                    // later.
                    mCropResultUri = Uri.parse(data.getAction());
                }
                break;
            case CROP_MSG:
                if (resultCode == RESULT_OK) {
                    setResult(resultCode, data);
                    finish();
                }
                break;
            default :
                break;
        }
    }

    /**
     * @param changed changed
     */
    public void onLayoutComplete(boolean changed) {
        mLayoutComplete = true;
        /*
        * FIX BUG: 3675
        * FIX COMMENT: Select the deleted photos, select the frame will move to another photo
        * Date: 2013-05-08
        */
        mSelectedIndex= GridViewSpecial.INDEX_NONE;
        if (mCropResultUri != null) {
            IImage image = mImageListSpecial.getImageForUri(mCropResultUri);
            mCropResultUri = null;
            if (image != null) {
                mSelectedIndex = mImageListSpecial.getImageIndex(image);
            }
        }
        mGvs.setSelectedIndex(mSelectedIndex);
        if (mScrollPosition == INVALID_POSITION) {
            mGvs.scrollToImage(0);
        } else if (mConfigurationChanged) {
            mConfigurationChanged = false;
            mGvs.scrollTo(mScrollPosition);
            if (mGvs.getCurrentSelection() != GridViewSpecial.INDEX_NONE) {
                mGvs.scrollToVisible(mSelectedIndex);
            }
        } else {
            mGvs.scrollTo(mScrollPosition);
        }
    }

    /**
     * @param scrollPosition scrollPosition
     */
    public void onScroll(float scrollPosition) {
        mScrollPosition = scrollPosition;
    }

    private void handleImageLongPress(int position,View view){
        mIsLongPressed = true;
        mSelectedItemIndex = position;
        setSelectedItem(view);
        Util.setTextAndDrawableTop(getApplicationContext(), mDeleteTextView, R.drawable.gallery_image_action_del_default ,R.string.text_delete_image);
        mDeleteTextView.setClickable(false);
    }

    private void handlerImageDown(MotionEvent event){
        /*
         * FIX BUG: 6129
         * FIX COMMENT: java.lang.NullPointerException
         * DATE: 2014-03-20
         */
        if(mDragItem != null && mDragItem.getDrawingCache() != null){
            Bitmap bitmap = Bitmap.createBitmap(mDragItem.getDrawingCache());
            startDragging(bitmap, (int)event.getX(),(int)event.getY());
        }
    }

    private void handleImageMove(MotionEvent event){
        if(mDragItem != null){
            dragView((int)event.getX(), (int)event.getY());
            doExpansion();
        }
//        mControlNormalLayout.setBackgroundResource(R.drawable.gallery_image_action_bar);
        if(event.getY() > getWindowManager().getDefaultDisplay().getHeight() - mDistanceToBottom){
            Util.setTextAndDrawableTop(getApplicationContext(), mDeleteTextView, R.drawable.gallery_image_action_del_pressed, Constants.NO_STRING);
        }else{
            Util.setTextAndDrawableTop(getApplicationContext(), mDeleteTextView, R.drawable.gallery_image_action_del_default, Constants.NO_STRING);
        }
    }

    private void handleImageUp(MotionEvent event){
         mGvs.resetLayoutCompleteStatus();
        if (event.getY() > getWindowManager().getDefaultDisplay().getHeight() - mDistanceToBottom && mSelectedItemIndex > -1) {
            if(!mPausing){
                if(mImageListSpecial != null){
                    IImage image = mImageListSpecial.getImageAtIndex(mSelectedItemIndex);
                    if(ImageManager.isImage(image)){
                        MenuHelper.deleteImage(BaseImagePicker.this, mDeleteGridImageRunnable, mImageListSpecial.getCountAtPosition(mSelectedItemIndex));
                    } else {
                        MenuHelper.deleteVideo(BaseImagePicker.this, mDeleteGridImageRunnable, 1);
                    }

                }
            }
        }
//        mControlNormalLayout.setBackgroundResource(R.drawable.gallery_image_action_bar);
/*        mDeleteImageView.setImageResource(R.drawable.gallery_entry_select_status);
        mDeleteTextView.setText(R.string.text_select_image);*/
        Util.setTextAndDrawableTop(getApplicationContext(), mDeleteTextView, R.drawable.gallery_entry_select_status ,R.string.text_select_image);
        mDeleteTextView.setClickable(true);
        stopDragging();
        unExpandViews();
        mWindowParams = null;
        mIsLongPressed = false;
    }

    @Override
    public void onImageLongPressed(int index) {
        if(mImageListSpecial == null){
            return;
        }
        if(!isInMultiSelectMode() && (ViewImage.IMAGE_ENTRY_NORMAL_VALUE.equals(getGalleryEntry())
                || ViewImage.IMAGE_ENTRY_UGALLERY_VALUE.equals(getGalleryEntry()))){
            ImageView imageView = new ImageView(this);

            imageView.setImageBitmap((Bitmap)mImageListSpecial.getImageAtIndex(index).miniThumbBitmap()[1]);
            handleImageLongPress(index,imageView);
            mGvs.invalidateImage(index);
        }
    }
    private Drawable mVideoOverlay;
    private Drawable mVideoMmsErrorOverlay;

    // mSrcRect and mDstRect are only used in drawImage, but we put them as
    // instance variables to reduce the memory allocation overhead because
    // drawImage() is called a lot.
    private final Rect mSrcRect = new Rect();

    private final Rect mDstRect = new Rect();

    private final Paint mPaint = new Paint(Paint.FILTER_BITMAP_FLAG);

    /**
     * @param canvas canvas
     * @param image image
     * @param b b
     * @param xPos xPos
     * @param yPos yPos
     * @param w w
     * @param h h
     */
    public void drawImage(Canvas canvas, IImage image, Bitmap b, int xPos, int yPos, int w, int h) {
        if(b == null){
            b = getErrorBitmap(image);
        }
//        if (b != null) {
            // if the image is close to the target size then crop,
            // otherwise scale both the bitmap and the view should be
            // square but I suppose that could change in the future.

            Paint paint = new Paint();

            int bw = b.getWidth();
            int bh = b.getHeight();

            int deltaW = bw - w;
            int deltaH = bh - h;
            Bitmap old = b;
            b = b.copy(Config.ARGB_8888, true);
            if (deltaW >= 0 && deltaW < 10 && deltaH >= 0 && deltaH < 10) {
                int halfDeltaW = deltaW / 2;
                int halfDeltaH = deltaH / 2;
                mSrcRect.set(0 + halfDeltaW, 0 + halfDeltaH, bw - halfDeltaW, bh - halfDeltaH);
                mDstRect.set(xPos, yPos, xPos + w, yPos + h);
                /*
                 * FIX BUG: 5224
                 * FIX COMMENT: Drag the photos, the photos are still in place
                 * Date: 2013-11-04
                 */
                if(!mIsLongPressed || image != mImageListSpecial.getImageAtIndex(mSelectedItemIndex)){
                    canvas.drawBitmap(b, mSrcRect, mDstRect, null);
                    drawImageDecor(canvas, image, paint);
                }
//                if(!Util.isSelectPicMode(mEntry) && Util.isBurstPicture(image.getTitle())) {
//                    if(mBurstIconBitmap == null) {
//                        mBurstIconBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.ic_burst_mode);
//                    }
//                    canvas.drawBitmap(mBurstIconBitmap, mDstRect.left + 20, mDstRect.bottom - 10 - mBurstIconBitmap.getHeight(), paint);
//                }else if(!Util.isSelectPicMode(mEntry) && Util.isPanoramaPicture(image.getTitle())){
//                    if(mPanoramaIconBitmap == null) {
//                        mPanoramaIconBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.ic_panorama_mode);
//                    }
//                    canvas.drawBitmap(mPanoramaIconBitmap, mDstRect.left + 20, mDstRect.bottom - 10 - mPanoramaIconBitmap.getHeight(), paint);
//                }else if(!Util.isSelectPicMode(mEntry) && ImageListSpecial.GIF_TYPE.equals(image.getMimeType())) {
//                    if(mGifIconBitmap == null) {
//                        mGifIconBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.ic_gif_mode);
//                    }
//                    canvas.drawBitmap(mGifIconBitmap, mDstRect.left + 20, mDstRect.bottom - 10 - mGifIconBitmap.getHeight(), paint);
//                }
            } else {
                mSrcRect.set(0, 0, bw, bh);
                mDstRect.set(xPos, yPos, xPos + w, yPos + h);
                if(!mIsLongPressed || image != mImageListSpecial.getImageAtIndex(mSelectedItemIndex)){
                    canvas.drawBitmap(b, mSrcRect, mDstRect, mPaint);
                    drawImageDecor(canvas, image, paint);
//                    if(!Util.isSelectPicMode(mEntry) && Util.isBurstPicture(image.getTitle())) {
//                        if(mBurstIconBitmap == null) {
//                            mBurstIconBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.ic_burst_mode);
//                        }
//                        canvas.drawBitmap(mBurstIconBitmap, mDstRect.left + 20, mDstRect.bottom - 10 - mBurstIconBitmap.getHeight(), paint);
//                    }else if(!Util.isSelectPicMode(mEntry) && Util.isPanoramaPicture(image.getTitle())) {
//                        if(mPanoramaIconBitmap == null) {
//                            mPanoramaIconBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.ic_panorama_mode);
//                        }
//                        canvas.drawBitmap(mPanoramaIconBitmap, mDstRect.left + 20, mDstRect.bottom - 10 - mPanoramaIconBitmap.getHeight(), paint);
//                    }else if(!Util.isSelectPicMode(mEntry) && ImageListSpecial.GIF_TYPE.equals(image.getMimeType())) {
//                        if(mGifIconBitmap == null) {
//                            mGifIconBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.ic_gif_mode);
//                        }
//                        canvas.drawBitmap(mGifIconBitmap, mDstRect.left + 20, mDstRect.bottom - 10 - mGifIconBitmap.getHeight(), paint);
//                    }
                }
            }
            if (old != b) {
                Util.recyleBitmap(b);
            }
    }
    private void drawImageDecor(Canvas canvas, IImage image, Paint paint) {
        if(!Util.isSelectPicMode(mEntry) && Util.isBurstPicture(image.getTitle()) && !isInMultiSelectMode()) {
            if(mBurstIconBitmap == null) {
                mBurstIconBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.ic_burst_mode);
            }
            canvas.drawBitmap(mBurstIconBitmap, mDstRect.left + 20, mDstRect.bottom - 10 - mBurstIconBitmap.getHeight(), paint);
        }else if(!Util.isSelectPicMode(mEntry) && Util.isPanoramaPicture(image.getTitle())){
            if(mPanoramaIconBitmap == null) {
                mPanoramaIconBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.ic_panorama_mode);
            }
            canvas.drawBitmap(mPanoramaIconBitmap, mDstRect.left + 20, mDstRect.bottom - 10 - mPanoramaIconBitmap.getHeight(), paint);
        }else if(!Util.isSelectPicMode(mEntry) && ImageListSpecial.GIF_TYPE.equals(image.getMimeType())) {
            if(mGifIconBitmap == null) {
                mGifIconBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.ic_gif_mode);
            }
            canvas.drawBitmap(mGifIconBitmap, mDstRect.left + 20, mDstRect.bottom - 10 - mGifIconBitmap.getHeight(), paint);
        }
    }
    public void drawPlayIcon(Canvas canvas, IImage image, Bitmap b, int xPos, int yPos, int w, int h) {
        /* SPRD:  CID 109103 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE) @{ */
        if(b == null){
            b = getErrorBitmap(image);
        }
        /* @} */

        if (ImageManager.isVideo(image)) {
            Drawable overlay = null;
            long size = MenuHelper.getImageFileSize(image);
            if (size >= 0 && size <= mVideoSizeLimit) {
                if (mVideoOverlay == null) {
                    mVideoOverlay = getResources().getDrawable(
                            R.drawable.ic_gallery_video_overlay);
                }
                overlay = mVideoOverlay;
            } else {
                if (mVideoMmsErrorOverlay == null) {
                    mVideoMmsErrorOverlay = getResources().getDrawable(
                            R.drawable.ic_error_mms_video_overlay);
                }
                overlay = mVideoMmsErrorOverlay;
                Paint paint = new Paint();
                paint.setARGB(0x80, 0x00, 0x00, 0x00);
                canvas.drawRect(xPos, yPos, xPos + w, yPos + h, paint);
            }
            int width = overlay.getIntrinsicWidth();
            int height = overlay.getIntrinsicHeight();
            int left = (w - width) / 2 + xPos;
            int top = (h - height) / 2 + yPos;
            mSrcRect.set(left, top, left + width, top + height);
            overlay.setBounds(mSrcRect);
            overlay.draw(canvas);
        }
    }

    /**
     * @return boolean
     */
    public boolean needsDecoration() {
        return (mMultiSelected != null);
    }

    /**
     * draw the view by position
     * @param canvas draw view panel
     * @param image current view object
     * @param xPos the x coordinate to start drawing
     * @param yPos the y coordinate to start drawing
     * @param w current view width
     * @param h current view height
     */
    public void drawDecoration(Canvas canvas, IImage image, int xPos, int yPos, int w, int h) {
        if (mMultiSelected != null) {
            initMultiSelectDrawables();
            if(needOnOffCheckbox()) {
                Drawable checkBox = mMultiSelected.contains(image) ? mMultiSelectTrue : mMultiSelectFalse;
                if (checkBox != null) {
                    int width = checkBox.getIntrinsicWidth();
                    int height = checkBox.getIntrinsicHeight();

                    Paint paint = new Paint();
                    paint.setARGB(0x00, 0x00, 0x00, 0x00);
                    canvas.drawRect(new Rect(xPos, yPos, xPos + w, yPos+h), paint);
                    int left = xPos + w - width;
                    int top = yPos;
                    /*
                     * FIX BUG : 5314
                     * BUG COMMENT : Adjust the UI of mdpi screen phones;
                     * DATE : 2013-11-14
                     */
  /*                  if(UiUtils.screenDensity() == 1) {
                        left = xPos + w - width - 3;
                        top = yPos + 3;
                    } else {
                        left = xPos + w - width - Util.dip2px(this, 0);
                        top = yPos + Util.dip2px(this, -10);
                    }*/
                    mSrcRect.set(left, top, left + width, top + height);
                    checkBox.setBounds(mSrcRect);
                    checkBox.draw(canvas);
                }
            } else {
                Drawable checkBox = null;
                if(mMultiSelected.contains(image)) {
                    checkBox = mMultiSelectTrue;
                } else {
                    return;
                }
                int width = checkBox.getIntrinsicWidth();
                int height = checkBox.getIntrinsicHeight();

                //calculate selection panel position
                int left = xPos + w - width;
                int top = yPos;
                /*
                 * FIX BUG : 5312
                 * BUG COMMENT : Adjust the UI of mdpi screen phones;
                 * DATE : 2013-11-14
                 */
                /*if(UiUtils.screenDensity() == 1) {
                    left = xPos + w - width - 3;
                    top = yPos + 3;
                } else {
                    left = xPos + w - width - Util.dip2px(this, 20);
                    top = yPos + Util.dip2px(this, 16);
                }*/
                mSrcRect.set(left, top, left + width, top + height);
                checkBox.setBounds(mSrcRect);
                checkBox.draw(canvas);

                //get current position in the mMultiSelected list.
                int position = mMultiSelected.indexOf(image);
                String drawText = position + 1 + "";
                //instance the FontMetrics object
                FontMetrics fontMetrics = mCheckPaint.getFontMetrics();
                //calculate text width
                float halfWidth = mCheckPaint.measureText(drawText) / 2;
                //calculate text height
                float descent = fontMetrics.descent;
                float ascent = fontMetrics.ascent;
                float halfHeight = (descent + ascent) / 2;
                //calculate the coordinates of the text to start drawing
                float x , y;
                if(Integer.valueOf(drawText)<10) {
                    x = left + width/2 + halfWidth;
                    y = top  +height/2  + halfHeight/2;
                } else {
                    x = left + width / 2 ;
                    y = top + height / 2 ;
                }
                //draw text
                canvas.drawText(drawText, x, y, mCheckPaint);
            }
        }
    }

    @Override
    public void drawTitleDecoration(Canvas canvas, TimeModeItem item, int titleRow, int xPos, int yPos, int w, int h) {
        if (mMultiSelected != null) {
            initTitleMultiSelectDrawables();
            if(needOnOffCheckbox()) {
                Drawable checkBox = null;
                if(mMultiSelected.size() == mAllCount || item.getCurDateSelectedNum() == item.getCount()) {
                    checkBox = mTitleMultiSelectTrue;
                } else if(item.getCurDateSelectedNum() < item.getCount() || mMultiSelected.size() < mAllCount) {
                    checkBox = mTitleMultiSelectFalse;
                }
                int width = checkBox.getIntrinsicWidth();
                int height = checkBox.getIntrinsicHeight();

                int left = xPos + (w - width);
                int top = yPos + (h - height) / 2;
                mSrcRect.set(left, top, left + width, top + height);
                checkBox.setBounds(mSrcRect);
                checkBox.draw(canvas);
            }
        }
    }

    /**
     * initialize component
     */
    private void initMultiSelectDrawables() {
        if(needOnOffCheckbox()) {
            if(mMultiSelectTrue == null && mMultiSelectFalse == null) {
                mMultiSelectTrue = getResources().getDrawable(R.drawable.gallery_check_ok);
                mMultiSelectFalse = getResources().getDrawable(R.drawable.gallery_check_off);
            }
        } else {
            if(mMultiSelectTrue == null) {
                mMultiSelectTrue = getResources().getDrawable(R.drawable.gallery_item_select_panel);
                if(mCheckPaint == null) {
                    //initialize mCheckPaint object
                    mCheckPaint = new Paint();
                    //remove alias
                    mCheckPaint.setAntiAlias(true);
                    //set typeface to DEFAULT_BOLD
                    mCheckPaint.setTypeface(Typeface.DEFAULT_BOLD);
                    //set the text size to 16 pixel
                    mCheckPaint.setTextSize(getResources().getDimension(R.dimen.size_gif_select_panel_text));
                    //set the text color to white
                    mCheckPaint.setColor(Color.WHITE);
                }
            }
        }
    }

    private void initTitleMultiSelectDrawables() {
        if(needOnOffCheckbox()) {
            if(mTitleMultiSelectTrue == null && mTitleMultiSelectFalse == null) {
                mTitleMultiSelectTrue = getResources().getDrawable(R.drawable.gallery_date_sel_selected);
                mTitleMultiSelectFalse = getResources().getDrawable(R.drawable.gallery_date_sel_normal);
            }
        }
    }

    private boolean needOnOffCheckbox() {
        return (ViewImage.IMAGE_ENTRY_NORMAL_VALUE.equals(mEntry) || ViewImage.IMAGE_ENTRY_UGALLERY_VALUE.equals(mEntry)) ? true : false;
    }

    private Bitmap mMissingImageThumbnailBitmap;

    private Bitmap mMissingVideoThumbnailBitmap;

    /**
     * Create this bitmap lazily, and only once for all the ImageBlocks to use
     * @param image image
     * @return Bitmap
     */
    public Bitmap getErrorBitmap(IImage image) {
        /*
         * FIX BUG: 5013
         * FIX COMMENT: avoid null pointer exception
         * DATE: 2013-10-11
         */
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

    boolean isInMultiSelectMode() {
        return mMultiSelected != null;
    }

    private void closeMultiSelectMode() {
        if (mMultiSelected == null) {
            return;
        }
        mMultiSelected.clear();
        mMultiSelected = null;
        mControlLayout.setVisibility(View.GONE);
        mControlNormalLayout.setVisibility(View.VISIBLE);
        mGvs.invalidate();
        mBtnSelect.setEnabled(true);
        Util.setViewStatus(this, mMultiSelected);
    }

    private void openMultiSelectMode() {
        if (mMultiSelected != null) {
            return;
        }
        mMultiSelected = new ArrayList<IImage>();
        mGvs.invalidate();
        mControlLayout.setVisibility(View.VISIBLE);
        mControlNormalLayout.setVisibility(View.GONE);
    }

    private void onDeleteMultipleClicked() {
        Runnable action = new Runnable() {
            public void run() {
                /*
                 * FIX: 2008
                 * FIX COMMENT: Null Point
                 * DATE: 2012-11-28
                 */
                if (mMultiSelected != null && mImageListSpecial != null) {
                    int selectedSize = mImageListSpecial.getCountAtList(mMultiSelected);
                    deleteImage(selectedSize);
                }
            }
        };
        /*
         * FIX: 5851
         * FIX COMMENT: Null Point
         * DATE: 2014-01-21
         */
        if(mMultiSelected != null && mImageListSpecial != null) {
            if(ImageManager.isImage(mMultiSelected.get(0))) {
                MenuHelper.deleteImage(this, action, mImageListSpecial.getCountAtList(mMultiSelected));
            } else {
                MenuHelper.deleteVideo(this, action, mMultiSelected.size());
            }
        }


    }

    private ProgressDialog mDelProgressDialog;
    private ContentResolver mContentResolver;
    int mIndex = 0;
    private void deleteImage(int deleteCount) {
        mContentResolver = getContentResolver();
        if(mDelProgressDialog == null) {
            mDelProgressDialog = new ProgressDialog(this);
            mDelProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
            mDelProgressDialog.setMax(deleteCount);
            mDelProgressDialog.setTitle(R.string.text_waiting);
            mDelProgressDialog.setCancelable(false);
            mDelProgressDialog.show();
            mHandler.post(mDeleteNextRunnable);
        }
    }

    private final Runnable mDeleteNextRunnable = new Runnable() {
        public void run() {
            deleteNext();
        }
    };

    private void deleteNext() {
        if (mImageListSpecial == null || mIndex >= mImageListSpecial.getCountAtList(mMultiSelected)) {
            /**
             * FIX BUG: 1920
             * BUG CAUSE: View not attached to window manager;
             * DATE: 2012-11-15
             */
            if(isFinishing()) {
                return;
            }
            mDelProgressDialog.dismiss();
            mDelProgressDialog = null;
            mIndex = 0;
            closeMultiSelectMode();
            rebake(false, false);
            return;
        }

        IImage image = null;
        if(mImageListSpecial != null) {
            image = mImageListSpecial.getImageAtSelectedList(mMultiSelected, mIndex++);
        }
        if(image != null) {
            mDelProgressDialog.incrementProgressBy(1);
            mContentResolver.delete(image.fullSizeImageUri(), null, null);
            mContentResolver.delete(UCamData.Thumbnails.CONTENT_URI, UCamData.Thumbnails.THUMB_PATH + "=?", new String[]{image.getDataPath()});
            mContentResolver.delete(UCamData.Albums.CONTENT_URI, UCamData.Albums.ALBUM_BUCKET + "=?", new String[]{image.getBucketId()});
            if(Build.isTelecomCloud()) {
                Util.startCloudService(image , this);
            }
        }
        mHandler.post(mDeleteNextRunnable);
    }

    private class DeleteImageTask extends AsyncTask<List<IImage>, Integer, Long> {
        private ContentResolver mContentResolver = null;
        private ProgressDialog mProgressDialog = null;
        private int mCount;

        public DeleteImageTask(int count) {
            mCount = count;
        }

        protected void onPreExecute() {
            mContentResolver = BaseImagePicker.this.getContentResolver();
            mProgressDialog = new ProgressDialog(BaseImagePicker.this);
            mProgressDialog.setMax(mCount);
            mProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
            mProgressDialog.setTitle(R.string.text_waiting);
            mProgressDialog.setCancelable(false);
            mProgressDialog.show();
        }
        protected Long doInBackground(List<IImage>... list) {
            int count = list.length;
            long totalSize = 0;
            for (int i = 0; i < count; i++) {
                if (isCancelled()) {
                    break;
                }
                ArrayList<IImage> imageList = (ArrayList<IImage>) list[i];
                int j = 0;
                for(IImage image : imageList) {
                    mContentResolver.delete(image.fullSizeImageUri(), null, null);
                    mContentResolver.delete(UCamData.Thumbnails.CONTENT_URI, UCamData.Thumbnails.THUMB_PATH + "=?", new String[]{image.getDataPath()});
                    mContentResolver.delete(UCamData.Albums.CONTENT_URI, UCamData.Albums.ALBUM_BUCKET + "=?", new String[]{image.getBucketId()});
                    if(Build.isTelecomCloud()) {
                        Util.startCloudService(image , BaseImagePicker.this);
                    }
                    publishProgress(++j);
                }
            }
            return totalSize;
        }

        protected void onProgressUpdate(Integer... progress) {
            setProgressPercent(progress[0]);
        }

        protected void onPostExecute(Long result) {
            if(!BaseImagePicker.this.isFinishing()){
                mProgressDialog.dismiss();
            }
            mProgressDialog = null;
//            if(isGridMode()) {
                rebake(false, ImageManager.isMediaScannerScanning(getContentResolver()));
            /*} else {
                startWorker();
            }*/
        }

        @Override
        protected void onCancelled() {
            if(!BaseImagePicker.this.isFinishing()){
                mProgressDialog.dismiss();
            }
            mProgressDialog = null;
            rebake(false, ImageManager.isMediaScannerScanning(getContentResolver()));
            /*if(isGridMode()) {
                rebake(false, ImageManager.isMediaScannerScanning(getContentResolver()));
            }*/
        }

        private void setProgressPercent(int progress) {
            if(mProgressDialog != null) {
                /*
                 * FIX BUG: 4698
                 * BUG COMMENT: invoke correct function to update dialog progress
                 * DATE: 2013-08-26
                 */
                //mProgressDialog.incrementProgressBy(progress);
                mProgressDialog.setProgress(progress);
            }
        }
    }

    private LayoutSpec mSpec;
    private void initCellSize() {
        mSpec = mGvs.getLayoutSpec();
    }

    private void toggleCheckboxStatus(boolean isSelectAll) {
        int startIndex = 0;
        for(TimeModeItem item : mTimeModeItems) {
            int count = item.getCount();
            for(int i = 0; i < count; i++) {
                IImage image = mImageListSpecial.getImageAtIndex(startIndex + i);
                if(isSelectAll) {
                    //selected all
                    if(!mMultiSelected.contains(image)) {
                        mMultiSelected.add(image);
                    }
                    item.addItemToDateList(image);
                } else {
                    //deselected all
                    if(mMultiSelected.contains(image)) {
                        mMultiSelected.remove(image);
                    }
                    item.removeItemFromDateList(image);
                }
            }
            startIndex += count;
        }

        mBtnSelect.setEnabled(!isSelectAll);
        Util.setViewStatus(this, mMultiSelected);

        mGvs.invalidate();
    }

///////////////////////////////////////////////////////////////////////////////
    protected static final int NO_LIMIT = -1;
    private void selectDone(List<IImage> selected) {
        final int minLimit = getMinLimit();
        if ( minLimit < 0 || mSelectCount >= minLimit) {
            onSelectDone(selected);
        } else {
            // Make Toast
        }
    }
    protected int getMaxLimit() {return NO_LIMIT;}
    protected int getMinLimit() {return NO_LIMIT;}

//    protected abstract void onSelectDone(List<IImage> selected);
    protected void onSelectDone(List<IImage> selected) {

    }

    protected void onSelected(IImage image) {

    }

    private WindowManager mWindowManager;
    private WindowManager.LayoutParams mWindowParams;
    private View mDragItem;
    private ImageView mDragView;
    private Bitmap mDragBitmap;
    private int mItemOffset_X;
    private int mItemOffset_Y;
    private int mSelectedItemIndex = -1;
    public void setSelectedItem(View v){
        mDragItem = v;
        if(isGridMode()){
            mItemOffset_X = (int) (mSpec.mCellWidth*0.45);
            mItemOffset_Y = (int) (mSpec.mCellHeight*0.12);
        }else{
            mItemOffset_X = (int) (mDragItem.getWidth()*0.5);
            mItemOffset_Y = (int) (mDragItem.getHeight()*0.25);
        }
        mDragItem.getHeight();
    }

    private void dragView(int x, int y) {
        float alpha = 1.0f;
        if(mDragItem == null){
            return;
        }
        Bitmap bitmap = null;
        if(mWindowParams == null){
            mDragItem.setDrawingCacheEnabled(true);
            if(mDragItem instanceof ImageView){
                BitmapDrawable drawable =  (BitmapDrawable) ((ImageView) mDragItem).getDrawable();
                bitmap = drawable.getBitmap();
            }else{
                bitmap = Bitmap.createBitmap(mDragItem.getDrawingCache());
            }
            startDragging(bitmap, x, y);
        }
        /* SPRD: CID 110294 : Dereference after null check (FORWARD_NULL) @{ */
        else {
            mWindowParams.alpha = alpha;
            mWindowParams.y = y - mItemOffset_Y;
            mWindowParams.x = x - mItemOffset_X;
            mWindowParams.flags = WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;
        }
        /**
        mWindowParams.alpha = alpha;
        mWindowParams.y = y - mItemOffset_Y;
        mWindowParams.x = x - mItemOffset_X;
        mWindowParams.flags = WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;
        */
        /* @} */

        /**
         * FIX BUG: 1508
         * BUG CAUSE: View not attached to window manager;
         * DATE: 2012-08-24
         */
        if(mDragView == null){
            mDragView = new ImageView(this);
            mDragView.setImageBitmap(bitmap);
        }
        if(mDragView != null){
            mWindowManager.updateViewLayout(mDragView, mWindowParams);
        }
    }

    private void startDragging(Bitmap bm, int x,int y) {
        stopDragging();

        mWindowParams = new WindowManager.LayoutParams();
        mWindowParams.gravity = Gravity.TOP|Gravity.LEFT;
        mWindowParams.x = x - mItemOffset_X;
        mWindowParams.y = y - mItemOffset_Y;

        if(mSpec != null && mSpec.mCellHeight > 12 && mSpec.mCellWidth > 12) {
            mWindowParams.height = mSpec.mCellHeight - 12;
            mWindowParams.width =  mSpec.mCellWidth - 12;
        } else{
            mWindowParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
            mWindowParams.width = WindowManager.LayoutParams.WRAP_CONTENT;
        }
        mWindowParams.flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
        mWindowParams.format = PixelFormat.TRANSLUCENT;
        mWindowParams.windowAnimations = 0;

        ImageView v = new ImageView(this);
        v.setImageBitmap(bm);
        mDragBitmap = bm;

        mWindowManager.addView(v, mWindowParams);
        mDragView = v;
    }

    private void stopDragging() {
        if (mDragView != null) {
            WindowManager wm = (WindowManager) this.getSystemService("window");
            wm.removeView(mDragView);
            mDragView.setImageDrawable(null);
            mDragView = null;
        }
        if (mDragBitmap != null) {
            mDragBitmap.recycle();
            mDragBitmap = null;
        }
    }

    private void doExpansion() {
        if (mDragItem != null) {
            mDragItem.setVisibility(View.INVISIBLE);
        }
    }

    private void unExpandViews() {
        if (mDragItem != null) {
            mDragItem.setVisibility(View.VISIBLE);
        }
    }
}
