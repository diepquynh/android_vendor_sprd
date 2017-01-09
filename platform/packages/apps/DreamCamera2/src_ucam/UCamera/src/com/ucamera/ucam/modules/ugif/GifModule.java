/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif;

import java.util.List;
import android.app.Activity;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.media.ThumbnailUtils;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Parcelable;
import android.text.TextUtils;
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.view.ViewGroup;
import android.view.SurfaceHolder;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.GridView;
import android.widget.HorizontalScrollView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.LinearLayout.LayoutParams;
import android.widget.FrameLayout;
import android.view.OrientationEventListener;
import com.android.camera2.R;

import com.android.camera.widget.ModeOptionsOverlay;
import com.android.camera.ui.TopRightWeightedLayout;
import com.android.camera.app.AppController;
import com.android.camera.app.CameraApp;
import com.android.camera.app.CameraAppUI;
import com.android.camera.app.OrientationManager;
import com.android.camera.ButtonManager;
import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;
import com.android.camera.Storage;
import com.android.camera.util.CameraUtil;
import com.sprd.camera.storagepath.StorageUtil;
import com.android.camera.stats.SessionStatsCollector;
import com.ucamera.ucam.modules.compatible.Models;
import com.ucamera.ucam.modules.compatible.ResolutionSize;
import com.ucamera.ucam.modules.compatible.PreviewSize;
import com.ucamera.ucam.jni.ImageProcessJni;
import com.ucamera.ucam.modules.ugif.CollageMenuAdapter;
//import com.ucamera.ucam.modules.ugif.GifBrowser;
import com.ucamera.ucam.modules.ugif.GifPlayActivity;
import com.ucamera.ucam.modules.ugif.edit.GifEditActivity;
import com.ucamera.ucam.modules.ugif.thumbnail.ThumbnailController;
import com.ucamera.ucam.modules.ugif.GifUI;
import com.ucamera.ucam.modules.ui.PreviewFrameLayout;
import com.ucamera.ucam.modules.ui.PreviewSurfaceView;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.ui.RotateImageView;
import com.android.camera.util.GservicesHelper;
import com.ucamera.ucam.modules.ui.ShutterButton;
import com.ucamera.ucam.modules.ui.Switcher;
import com.ucamera.ucam.modules.utils.BitmapUtils;
import com.ucamera.ucam.modules.utils.LogUtils;
import com.ucamera.ucam.modules.utils.UiUtils;
import com.ucamera.ucam.modules.utils.Utils;
import com.ucamera.ugallery.ImageGallery;
import com.ucamera.ugallery.ViewImage;
import com.ucamera.ucam.modules.ugif.thumbnail.BaseImageManager;
import com.ucamera.ucam.modules.ugif.thumbnail.BaseImage;
import com.ucamera.ucam.modules.BasicModule;

import com.android.ex.camera2.portability.CameraAgent;
import com.android.ex.camera2.portability.CameraCapabilities;
import com.android.ex.camera2.portability.Size;

public class GifModule extends BasicModule implements
        Switcher.OnSwitchListener, SurfaceHolder.Callback,
        View.OnClickListener, PreviewFrameLayout.OnSizeChangedListener,
        ShutterButton.OnShutterButtonListener,
        OrientationManager.OnOrientationChangeListener {

    private static final String TAG = "GifModule";

    public static final int SET_BITMAP_TO_ADAPTER = 1;
    public static final int SHOW_CONFIRM_BUTTON = 2;
    public static final int THUMBNAIL_CHANGED = 3;
    private static final int UPDATE_THUMBNAIL = 4;
    private static final int HIDE_THUMBNAIL_IMAGE = 5;

    private static final int REQUEST_CODE_PICK_IMAGE = 0xABCDE;

    private static final int PER_SCREEEN_NUM  = 5;

    // SPRD: fix bug539498 shows gap between Video icon area and option button area
    private static final float DEFAULT_ASPECT_RATIO  = 16.0f/9.0f;

    private boolean SWITCH_CONTINUE_MODE = true;    // default is continue mode.

    private View mGifProgressLayout;
    private ProgressBar mGifProgressBar;
    private TextView mProgressTextView;

    private View mGifGridViewScroller;
    private HorizontalScrollView mGridViewScrollView;
    private TextView mPicProgressTextView;
    // CID 109347 : UuF: Unused field
    // private View mThumbnailShadowView;

    private Switcher mSwitcher;
    private ImageView mSwitcherCameraIcon;
    private ImageView mSwitcherVideoIcon;
//    private RotateImageView mGifListButton;
    private RelativeLayout mThumbnailLayout;
    protected RotateImageView mThumbnailView;
    protected ThumbnailController mThumbController;

    private GridView mGridView = null;
    private CollageMenuAdapter mAdapter = null;

    private RotateImageView mGifCancelBtn = null;
    private RotateImageView mGifOkBtn = null;

    // CID 109221 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private int numOfTakenPics = 0;
    private int numOfPics = 0;
    private int mPicWidth = 0;
    private boolean mIsStartEditOrPlayActicity = false; // SPRD: Fix bug 577075
    private Bitmap mBitmap = null;
    private boolean isGifCapturing = false;
    private boolean isGifRecording = false;
    private boolean mIsLoadingImage = false;
    // CID 109221 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private RelativeLayout mPreviewBgLayout;
    protected int mPreviewWidth = 0;
    protected int mPreviewHeight = 0;
    protected RelativeLayout mSufaceRootView; // SPRD: Add for bug 572826
    protected PreviewFrameLayout mPreviewFrameLayout;
    protected PreviewSurfaceView mPreviewSurfaceView;
    protected ViewGroup mRootView;
    protected ShutterButton mShutterButton;
    protected volatile SurfaceHolder mCameraSurfaceHolder;
    protected BaseImageManager mImageManager = new BaseImageManager();
    protected CameraThumbnailThread mCameraThumbnailThread;
    private View mModeOptions;
    // CID 109221 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private LinearLayout mModeOptionsToggle;
    private ModeOptionsOverlay mModeOptionsOverLay;
    private TopRightWeightedLayout mModeOptionsButton;
    // CID 109347 : UuF: Unused field (FB.UUF_UNUSED_FIELD)
    // private int mModeToggleBottomMargin;
    private int mModeBottomMargin;
    private int mGifModuleOrientation = OrientationEventListener.ORIENTATION_UNKNOWN;
    private int mOrientationCompensation = 0;
    private boolean mBottomMarginSaved = false;

    private final Handler mHandler = new MainHandler();
    private class MainHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case SET_BITMAP_TO_ADAPTER: {
                if(SWITCH_CONTINUE_MODE){
                    if(isGifRecording){
                        mAdapter.setThumbnail(mBitmap);
                        moveGifGrid();
                        mGifProgressBar.setProgress(mAdapter.getItemCount());
                        mProgressTextView.setText((mAdapter.getItemCount())+"/" + numOfPics);
                        if(numOfPics - mAdapter.getItemCount() > 0){
                            addPreviewBuffer();
                        }else{
                            isGifRecording = false;
                        }
                    }
                }else{
                    if(isGifCapturing){
                        mAdapter.setThumbnail(mBitmap);
                        moveGifGrid();
                        setCapturingProgressText(mAdapter.getItemCount()+"/" + numOfPics);
                        isGifCapturing = false;
                    }
                }

                mBitmap = null;
                if(mAdapter.getItemCount() == numOfPics){
                    if(SWITCH_CONTINUE_MODE && mShutterSound != null && mShutterSoundEnabled){
                        mShutterSound.playUgifVideoSound();
                    }
                    changeUI(false);
                    setCapturingProgressText("");
                    gotoEditActivity();
                }
                break;
            }
            case SHOW_CONFIRM_BUTTON:{
                changeUI(true);
                break;
            }
            case THUMBNAIL_CHANGED: {
                if(mAdapter != null && mAdapter.getCount() == 0){
                    changeUI(false);
                }
                String progressText = null;
                if(mAdapter != null && mAdapter.getItemCount() > 0){
                    progressText = mAdapter.getItemCount()+"/" + numOfPics;
                }
                setCapturingProgressText(progressText);
                break;
            }
            case UPDATE_THUMBNAIL: {
                Pair<Uri, Bitmap> args = (Pair<Uri, Bitmap>) msg.obj;
                setLastPictureThumb(args.first, args.second);
                break;
            }
            case HIDE_THUMBNAIL_IMAGE: {
                if(mThumbnailView != null){
                    mThumbnailView.setVisibility(View.GONE);
                }
                break;
            }
            default:{
                break;
            }
            }
        }
    }

    public GifModule(AppController app) {
        super(app);
    }

    public String getName() {
        return "Gif";
    }

    @Override
    public boolean isUsingBottomBar() {
        return true;
    }

    @Override
    public void init(CameraActivity activity, boolean isSecureCamera, boolean isCaptureIntent) {
        super.init(activity, isSecureCamera, isCaptureIntent);
        mAdapter = new CollageMenuAdapter(activity);
        mAdapter.setThumbnailListener(new ThumbnailListener());
        mGridView.setAdapter(mAdapter);
    }

    private void setDisplayItemCountsInWindow(GridView gridview, int totalCount, int countPerScreen) {
        final int itemWidth = UiUtils.effectItemWidth();
        int mode = totalCount % countPerScreen;
        if (mode > 0) {
            totalCount = totalCount + (countPerScreen - mode);
        }
        gridview.setNumColumns(totalCount);
        final int layout_width = itemWidth * totalCount;
        gridview.setLayoutParams(new LinearLayout.LayoutParams(layout_width,
                LayoutParams.WRAP_CONTENT));
    }

    @Override
    public void onShutterButtonFocus(boolean pressed) {
    }

    @Override
    public void onShutterButtonClick() {

        /*
         * FIX BUG : 4754
         * BUG COMMENT : do not allow capture when loading images of selected from gallery
         * DATE : 2013-08-26
         */
        if (numOfPics == mAdapter.getItemCount() || !canTakePicture() || mIsLoadingImage
                || mIsStartEditOrPlayActicity) {
            return;
        }

        final long storageSpaceBytes = mActivity.getStorageSpaceBytes();
        if(storageSpaceBytes <= Storage.LOW_STORAGE_THRESHOLD_BYTES) {
            Log.w(TAG, "Low storage warning: " + storageSpaceBytes);
            return;
        }

        if(SWITCH_CONTINUE_MODE){
            /**
             * BUG FIX: 3119
             * BUG CAUSE: Shutter sound not play when taking video in Ugif mode.
             * DATE: 2013-03-11
             */
            if(mShutterSoundEnabled){
                mShutterSound.playUgifVideoSound();
            }
             if(!isGifRecording){
                 mActivity.getCameraAppUI().hideModeOptions();
                 mActivity.getCameraAppUI().setSwipeEnabled(false);
                 isGifRecording = true;
             } else {
                 mActivity.getCameraAppUI().enableModeOptions();
                 mActivity.getCameraAppUI().setSwipeEnabled(true);
                 gifFinish();
                 return;
             }
        }else{
            if(!isGifCapturing){
                mActivity.getCameraAppUI().setSwipeEnabled(false);
                isGifCapturing = true;
                if(mShutterSoundEnabled){
                    mShutterSound.playGif();
                }
            }
        }
        addPreviewBuffer();
        changeUI(true);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
        case R.id.gif_cancel:
            if(SWITCH_CONTINUE_MODE && mShutterSoundEnabled){
                mShutterSound.playUgifVideoSound();
            }
            mAdapter.recyleBitmaps();
            setDisplayItemCountsInWindow(mGridView, mAdapter.getCount(), PER_SCREEEN_NUM);
            setCapturingProgressText("");
            changeUI(false);
            isGifRecording = false;
            //SPRD:fix bug533672 stop record gif, show mode options
            mActivity.getCameraAppUI().showModeOptions();
            mActivity.getCameraAppUI().setSwipeEnabled(true);
            break;
        case R.id.gif_finish:
            if(SWITCH_CONTINUE_MODE && mShutterSoundEnabled){
                mShutterSound.playUgifVideoSound();
            }
            gifFinish();
            break;
//        case R.id.gif_list_button:
        case R.id.review_thumbnail_layout:
        case R.id.review_thumbnail:
            if(!checkStorage())
                return;
//            mActivity.startActivity(new Intent(mActivity, GifBrowser.class));
            //viewLastPicture();
            Intent intent = new Intent(mActivity, GifPlayActivity.class);
            Bundle b = new Bundle();
            b.putBoolean("fromOutSide", false);
            intent.putExtras(b);
            intent.setData(mThumbController.getUri());
            mIsStartEditOrPlayActicity = true; // SPRD: Fix bug 577075
            mActivity.startActivity(intent);
            break;
        }
    }

    private void gifFinish(){
        if(mAdapter.getItemCount() > 1){
            gotoEditActivity();
            changeUI(false);
            setCapturingProgressText("");
            isGifRecording = false;
        } else {
            Toast.makeText(mActivity,mActivity.getResources().getString(R.string.gif_compose_edit_tips),Toast.LENGTH_SHORT).show();
        }
    }

    private boolean canUIPressed() {
        if (mCameraState == SNAPSHOT_IN_PROGRESS) return false;
        if (mCameraState == PREVIEW_STOPPED) return false;
        if (mCameraState == SWITCHING_CAMERA) return false;
        return true;
    }

    @Override
    public void makeModuleUI(PhotoController controller,View parent) {
        //TODO Waiting for merge of WideAngle feature, noted by spread
        //mActivity.getCameraAppUI().setLayoutAspectRation(0f);
        mUI = new GifUI(mCameraId, controller, mActivity, parent);
        initializeModuleControls();
    }

    // Add for dream camera
    // ATTENTION: this function is needed even not in dream camera
    protected void initializeCpatureBottomBar() {
        mShutterButton = (ShutterButton) mRootView.findViewById(R.id.shutter_button);
        mShutterButton.setOnShutterButtonListener(this);

        mSwitcher = (Switcher) mRootView.findViewById(R.id.video_switcher);
        mSwitcher.setOnSwitchListener(this);
        mSwitcher.setSwitch(true);

        mSwitcherCameraIcon = (ImageView) mRootView.findViewById(R.id.camera_icon);

        mSwitcherVideoIcon = (ImageView) mRootView.findViewById(R.id.video_icon);

        // mGifListButton = (RotateImageView) mRootView.findViewById(R.id.gif_list_button);
        // mGifListButton.setOnClickListener(this);
        // mThumbnailShadowView = mRootView.findViewById(R.id.review_thumbnail_shadow);
        mThumbnailView = (RotateImageView) mRootView.findViewById(R.id.review_thumbnail);
        mThumbnailView.setOnClickListener(this);

        mThumbnailLayout = (RelativeLayout) mRootView.findViewById(R.id.review_thumbnail_layout);
        mThumbnailLayout.setOnClickListener(this);
        mThumbController = new ThumbnailController(mActivity.getResources(), null, mContentResolver);
        mThumbController.setLastImageButton(mThumbnailView);
        mThumbController.setLayout(mThumbnailLayout);

        LinearLayout shutterButtonFrame = (LinearLayout) mRootView
                .findViewById(R.id.shutter_button_frame);
        LinearLayout videoSwitcherIndicator = (LinearLayout) mRootView
                .findViewById(R.id.video_switcher_indicator);

        shutterButtonFrame.setVisibility(View.VISIBLE);
        videoSwitcherIndicator.setVisibility(View.VISIBLE);
        mShutterButton.setVisibility(View.VISIBLE);
        mSwitcher.setVisibility(View.VISIBLE);
        mSwitcherCameraIcon.setVisibility(View.VISIBLE);
        mSwitcherVideoIcon.setVisibility(View.VISIBLE);
        mThumbnailView.setVisibility(View.VISIBLE);
        mThumbnailLayout.setVisibility(View.VISIBLE);

    }

    protected void initializeModuleControls() {
        mRootView = mActivity.getCameraAppUI().getModuleView();
        mActivity.getLayoutInflater().inflate(R.layout.gif_module, mRootView);

        // CID 109221 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mPreviewBgLayout = (RelativeLayout) mRootView.findViewById(R.id.gif_preview_bg_layout);
        mSufaceRootView = (RelativeLayout)mActivity.getLayoutInflater().inflate(R.layout.preview_module_frame, null);
        mPreviewFrameLayout = (PreviewFrameLayout) mSufaceRootView.findViewById(R.id.frame);
        mPreviewFrameLayout.setOnSizeChangedListener(this);
        mPreviewFrameLayout.setGifMode(true);

        mPreviewSurfaceView = (PreviewSurfaceView) mSufaceRootView.findViewById(R.id.preview_surface_view);
        mPreviewSurfaceView.setVisibility(View.VISIBLE);
        mPreviewSurfaceView.getHolder().addCallback(this);

        initializeCpatureBottomBar();

        mGifGridViewScroller = (View) mRootView.findViewById(R.id.collage_framelayout_scroller_nav_bottom);
        mGifGridViewScroller.setVisibility(View.GONE);
        mGridView = (GridView) mRootView.findViewById(R.id.collage_gv_nav_bottom);
        mGridView.setOnItemClickListener(mGridViewOnItemClickListener);
        mGridViewScrollView = (HorizontalScrollView) mRootView.findViewById(R.id.collage_scroller_nav_bottom);
        resizeGridViewScrollView();
        mPicProgressTextView = (TextView)mRootView.findViewById(R.id.gif_pic_progress_text);

        mGifCancelBtn = (RotateImageView) mRootView.findViewById(R.id.gif_cancel);
        mGifCancelBtn.setOnClickListener(this);

        mGifOkBtn = (RotateImageView) mRootView.findViewById(R.id.gif_finish);
        mGifOkBtn.setOnClickListener(this);

        mGifProgressLayout = (View) mRootView.findViewById(R.id.gif_progress_layout);
        mGifProgressBar = (ProgressBar) mRootView.findViewById(R.id.gif_progress_bar);
        mProgressTextView = (TextView) mRootView.findViewById(R.id.gif_progress_text);

        mModeOptions = (View) mActivity.getModuleLayoutRoot().findViewById(R.id.mode_options);
        // CID 109221 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mModeOptionsToggle = (LinearLayout) mActivity.getModuleLayoutRoot().findViewById(R.id.mode_options_toggle);
        mModeOptionsButton = (TopRightWeightedLayout) mActivity.getModuleLayoutRoot().findViewById(R.id.mode_options_buttons);
        mModeOptionsOverLay = (ModeOptionsOverlay) mActivity.getModuleLayoutRoot().findViewById(R.id.mode_options_overlay);
        // CID 123743 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        // int bottom = mActivity.getResources().getDimensionPixelSize(R.dimen.mode_option_overlay_gif_height);
        resetLayout();
    }

    private void resizeGridViewScrollView() {
        ViewGroup.LayoutParams layoutParams = mGridViewScrollView.getLayoutParams();
        layoutParams.width = UiUtils.screenWidth() -  UiUtils.effectItemWidth();
        mGridViewScrollView.setLayoutParams(layoutParams);
    }

    @Override
    protected void setModeOptionsLayout() {
        mModeOptionsOverLay.checkOrientation(Configuration.ORIENTATION_PORTRAIT);
        mModeOptionsButton.checkOrientation(Configuration.ORIENTATION_PORTRAIT);
/*        if (mModeOptionsToggle != null) {
            FrameLayout.LayoutParams paramsToggle =
                    (FrameLayout.LayoutParams) mModeOptionsToggle.getLayoutParams();
            if (!mPaused) {
                mModeToggleBottomMargin = paramsToggle.bottomMargin;
                paramsToggle.bottomMargin = 48;
            } else {
                paramsToggle.bottomMargin = mModeToggleBottomMargin;
            }
            mModeOptionsToggle.setLayoutParams(paramsToggle);
        }*/
        if(mModeOptions != null ) {
            FrameLayout.LayoutParams params =
                    (FrameLayout.LayoutParams) mModeOptions.getLayoutParams();
            if (!mPaused) {
                // SPRD:Bug 542268 shows a gap between Video/Photo icon area and option area BEGIN
                if (!mBottomMarginSaved) {
                    mModeBottomMargin = params.bottomMargin;
                    mBottomMarginSaved = true;
                }// SPRD:Bug 542268 shows a gap between Video/Photo icon area and option area END
                params.bottomMargin = 12;
            } else {
                params.bottomMargin = mModeBottomMargin;
            }
            mModeOptions.setLayoutParams(params);
        }
    }

    private void changeUI(boolean capturing) {
        int visible = capturing ? View.VISIBLE : View.GONE;

        mGifCancelBtn.setVisibility(visible);
        mGifOkBtn.setVisibility(visible);
        mSwitcher.setVisibility(View.GONE - visible);
        mSwitcherCameraIcon.setVisibility(View.GONE - visible);
        mSwitcherVideoIcon.setVisibility(View.GONE - visible);
//        mGifListButton.setVisibility(View.GONE - visible);
        mThumbnailLayout.setVisibility(View.GONE - visible);
//        mThumbnailShadowView.setVisibility(View.GONE - visible);

        if(SWITCH_CONTINUE_MODE && capturing){
//            mShutterButton.startAnimation(VideoUtils.createFlickerAnimation(800));
//            mActivity.getSwitcherButton().setEnabled(false);
        }else if(SWITCH_CONTINUE_MODE && !capturing){
            mShutterButton.clearAnimation();
            mGifProgressBar.setProgress(0);
            mProgressTextView.setText("");
//            mActivity.getSwitcherButton().setEnabled(true);
        }
    }

    private OnItemClickListener mGridViewOnItemClickListener = new OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            if (!canUIPressed()) {
                return ;
            }
            if ( position == mAdapter.getItemCount()) {
                mIsStartEditOrPlayActicity = true; // SPRD: Fix bug 580316
                int remains = mAdapter.getMaxItemCount() - mAdapter.getItemCount();
                ImageGallery.showImagePicker(mActivity, REQUEST_CODE_PICK_IMAGE, remains,ViewImage.IMAGE_ENTRY_UGIF_VALUE);
            }
        }
    };

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (data == null) {
            return ;
        }

        if (resultCode == Activity.RESULT_CANCELED) return;
        switch (requestCode) {
        case REQUEST_CODE_PICK_IMAGE:
            Parcelable[] obj = data.getParcelableArrayExtra(ImageGallery.INTENT_EXTRA_IMAGES);
            if (obj != null) {
                mIsLoadingImage = true;
                Uri[] images = new Uri[obj.length];
                System.arraycopy(obj, 0, images, 0, obj.length);
                new AsyncTask<Uri, Pair<Uri, Bitmap>, Void>() {
                    @Override
                    protected Void doInBackground(Uri... params) {
                        if(params != null && params.length > 0){
                            mHandler.sendEmptyMessage(SHOW_CONFIRM_BUTTON);
                        }
                        //fix coverity issue 108963
                        if(params != null) {
                            for (Uri u : params) {
                                if (u == null)
                                    continue;

                                Bitmap bitmap = getScaleBitmap(u);
                                if (bitmap == null)
                                    continue;

                                publishProgress(Pair.create(u, bitmap));
                            }
                        }

                        mHandler.postDelayed(new Runnable(){
                            @Override
                            public void run() {
                                mIsLoadingImage = false;
                            }

                        }, 500);
                        return null;
                    }

                    @Override
                    protected void onProgressUpdate(Pair<Uri, Bitmap>... values) {
                        if (values.length > 0) {
                            mAdapter.setThumbnail(values[0].second);
                            moveGifGrid();
                            String progressText = mAdapter.getItemCount()+"/" + numOfPics;
                            if(mAdapter.getItemCount() == numOfPics) {
                                progressText = "";
                                changeUI(false);
                                gotoEditActivity();
                            }
                            setCapturingProgressText(progressText);
                        }
                    }
                }.execute(images);
            }
            break;
        default: {
            /*
             * No Proper REQUEST CODE, so just close myself. Used in
             * TapnowLicenseCheck Please use RESULT_CLOSE_ME instead.
             */
            mActivity.finish();
        }
        }
    }
/*
    @Override
    protected void addTouchReceivers(PreviewGestures gestures) {
        super.addTouchReceivers(gestures);
        gestures.addTouchReceiver(mRootView.findViewById(R.id.collage_framelayout_scroller_nav_bottom));
        gestures.addTouchReceiver(mRootView.findViewById(R.id.collage_menu_item_layout_id));
        gestures.addTouchReceiver(mActivity.findViewById(R.id.top_bar));
    }
*/

    /**
     * ucamera original code
    @Override
    public void onOrientationChanged(int orientation) {
        super.onOrientationChanged(orientation);
        //TODO Optimized in the future, noted by spread
        mGifModuleOrientation = CameraUtil.roundOrientation(orientation, mGifModuleOrientation);
        //mGifModuleOrientation = orientation;
        // When the screen is unlocked, display rotation may change.
        // Always // calculate the up-to-date orientationCompensation.
        int orientationCompensation =
                (mGifModuleOrientation + CameraUtil.getDisplayRotation()) % 360;
        // Rotate camera mode icons in the switcher
        if (mOrientationCompensation != orientationCompensation) {
            mOrientationCompensation = orientationCompensation;
            updateRotationUI(mGifModuleOrientation);
        }
    }
    */

    /*SPRD:add for Head upside down*/
    @Override
    public void onOrientationChanged(OrientationManager orientationManager,
                                     OrientationManager.DeviceOrientation deviceOrientation) {
        mGifModuleOrientation = deviceOrientation.getDegrees();
        /*SPRD:fix for gif bug, small camera and video icon can not rotate@{*/
        // Always // calculate the up-to-date orientationCompensation.
        int orientationCompensation =
                (mGifModuleOrientation + CameraUtil.getDisplayRotation()) % 360;
        // Rotate camera mode icons in the switcher
        if (mOrientationCompensation != orientationCompensation) {
            mOrientationCompensation = orientationCompensation;
            updateRotationUI(mGifModuleOrientation);
        }
        /*@}*/
    }

    protected void updateRotationUI(int orientation) {
        int rotation = orientation;
        if (orientation % 180 == 90) {
            rotation = orientation + 180;
        }
        if(mSwitcherVideoIcon != null) {
            mSwitcherVideoIcon.setRotation(rotation);
        }
        if(mSwitcherCameraIcon != null) {
            mSwitcherCameraIcon.setRotation(rotation);
        }
        if(mThumbnailLayout != null) {
            mThumbnailLayout.setRotation(rotation);
        }
        if(mRootView.findViewById(R.id.review_thumbnail_layout) != null) {
            mRootView.findViewById(R.id.review_thumbnail_layout).setRotation(rotation);
        }
//        if(mRootView.findViewById(R.id.review_thumbnail_shadow) != null) {
//            mRootView.findViewById(R.id.review_thumbnail_shadow).setRotation(rotation);
//        }

        mShutterButton.setOrientation(orientation, true);
        mGifOkBtn.setOrientation(orientation, true);
        mGifCancelBtn.setOrientation(orientation, true);
//        mGifListButton.setOrientation(orientation, true);
    }

    private void moveGifGrid() {
        setDisplayItemCountsInWindow(mGridView, mAdapter.getCount(), PER_SCREEEN_NUM);
        int itemWidth = mAdapter.getGridItemWidth();
        int count = mAdapter.getCount();
        HorizontalScrollView horSGifView = (HorizontalScrollView) (mRootView
                .findViewById(R.id.collage_scroller_nav_bottom));

        if(horSGifView != null){
            int scrollX = horSGifView.getScrollX();

            if ((count + 1) * itemWidth < scrollX
                    || (count + 1) * itemWidth > (scrollX + 4 * itemWidth)) {
                horSGifView.scrollTo((count - 1) * itemWidth, 0);
            }
        }
    }

    @Override
    public boolean onSwitchChanged(Switcher source, boolean onOff) {
        if (onOff) {
            mShutterButton.setImageResource(R.drawable.ic_control_video_mode);
            mSwitcherCameraIcon.setImageResource(R.drawable.ic_control_camera);
            mSwitcherVideoIcon.setImageResource(R.drawable.ic_control_video_pressed);
            mGifGridViewScroller.setVisibility(View.GONE);
            mGifProgressLayout.setVisibility(View.VISIBLE);
            SWITCH_CONTINUE_MODE = true;
        } else {
            mShutterButton.setImageResource(R.drawable.ic_control_camera_mode);
            mSwitcherCameraIcon.setImageResource(R.drawable.ic_control_camera_pressed);
            mSwitcherVideoIcon.setImageResource(R.drawable.ic_control_video);
            mGifProgressLayout.setVisibility(View.GONE);
            mGifGridViewScroller.setVisibility(View.VISIBLE);
            SWITCH_CONTINUE_MODE = false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////// Override super class function
    //////////////////////////////////////////////////////////////////////////

    @Override
    protected void requestCameraOpen() {
        LogUtils.debug(TAG, "requestCameraOpen mCameraId:" + mCameraId);
        mActivity.getCameraProvider().requestCamera(mCameraId);
        SettingsManager mSettingsManager = mActivity.getSettingsManager();
        mSettingsManager.setValueByIndex(mAppController.getModuleScope(),
                Keys.KEY_CAMERA_ID, mCameraId);//SPRD BUG:379926
    }

    @Override
    protected void setCameraParameters(int updateSet) {
        if ((updateSet & UPDATE_PARAM_INITIALIZE) != 0) {
            updateCameraParametersInitialize();
        }

        if ((updateSet & UPDATE_PARAM_ZOOM) != 0) {
            updateCameraParametersZoom();
        }

        if ((updateSet & UPDATE_PARAM_PREFERENCE) != 0) {
            updateCameraParametersPreference();
        }

        if (mCameraDevice != null) {
            //String message = mParameters.flatten();
            //LogUtils.debug(TAG, " mParameters = " + message);
            mCameraDevice.applySettings(mCameraSettings);
        }
    }

    @Override
    protected void updateCameraParametersPreference() {

        // Set flash mode.
        updateParametersFlashMode();
        // set preview size
        setPreviewSize();

        // Set Anti-Banding
        updateParametersAntibanding();
        /*
         *  FIX BUG: 5100
         *  BUG CAUSE: gif add continuous focus function
         *  Date: 2013-10-30
         */
        // SPRD: Fix Bug 535139,Set white balance.
        updateParametersWhiteBalance() ;

        // SPRD: Fix Bug 535139,Set color effect.
        updateParametersColorEffect();
        List<String> supportedFocusModes = null;
        if (mParameters != null) {
            supportedFocusModes = mParameters.getSupportedFocusModes();
        }
        if(supportedFocusModes != null && supportedFocusModes.contains("continuous-video")){
            CameraCapabilities.Stringifier stringifier = mCameraCapabilities.getStringifier();
            mCameraSettings.setFocusMode(stringifier.focusModeFromString("continuous-video"));
            mCameraSettings.setExposureCompensationIndex(0);
        }
    }

    @Override
    protected void updateSceneMode() {}

    @Override
    protected void updateParametersFlashMode() {
        SettingsManager settingsManager = mActivity.getSettingsManager();
        String flashMode = settingsManager.getString(mAppController.getCameraScope(),
                Keys.KEY_GIF_FLASH_MODE);

        List<String> supportedFlash = null;
        if (mParameters != null) {
            supportedFlash = mParameters.getSupportedFlashModes();
        }

        if (mParameters != null && supportedFlash != null && supportedFlash.indexOf(flashMode) >= 0) {
            CameraCapabilities.Stringifier stringifier = mCameraCapabilities.getStringifier();
            mCameraSettings.setFlashMode(stringifier.flashModeFromString(flashMode));
        }
    }

    private void updateParametersAntibanding() {
        CameraCapabilities.Stringifier stringifier = mCameraCapabilities.getStringifier();
        SettingsManager settingsManager = mActivity.getSettingsManager();

        String mAntibanding = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,Keys.KEY_CAMER_ANTIBANDING);
        List<String> supportedAntibanding = null;
        if (mParameters != null) {
            supportedAntibanding = mParameters.getSupportedAntibanding();
        }
        if (mParameters != null && supportedAntibanding != null && supportedAntibanding.contains(mAntibanding)) {
            mCameraSettings.setAntibanding(stringifier.antibandingModeFromString(mAntibanding));
        }
    }

    @Override
    protected void setPreviewFrameLayoutAspectRatio() {
        /* SPRD:fix bug 462193 Occasionally generate gif error @{*/
        //ResolutionSize optimalSize = PreviewSize.instance(mCameraId).get(mParameters);
        Size optimalSize = mCameraSettings.getCurrentPreviewSize();
        /* @}*/
        if (optimalSize != null) {
            if (mPreviewFrameLayout != null) {
                if( Utils.calcNeedRevert(mCameraId)){
                    mPreviewFrameLayout.setGifAspectRatio((double) optimalSize.height() / optimalSize.width(),true);
                } else {
                    mPreviewFrameLayout.setGifAspectRatio((double) optimalSize.width() / optimalSize.height(),false);
                }
            }
            mPreviewWidth = optimalSize.width();
            mPreviewHeight = optimalSize.height();
        } else {
            LogUtils.debug(TAG, "optimalSize is null, supportedPreviewSizes:");
        }
    }

    protected void setPreviewSize() {
        List<ResolutionSize> sizes = PreviewSize.instance(mCameraId).getSupported(mParameters);
        /*
         * FIX BUG: 5601 5603 5616
         * BUG COMMENT: can not get the correct preview size on some devices
         * DATE: 2013-12-18
         */
        ResolutionSize optimalSize = Utils.getGifPreviewSize(sizes);
        if(optimalSize == null) {
            optimalSize = Utils.getOptimalPreviewSize(mActivity,sizes, 4.0/3.0);
        }
        if (optimalSize != null) {
            mCameraSettings.setPreviewSize(new Size(optimalSize.width, optimalSize.height));
        }
    }

    @Override
    public void onSettingChanged(SettingsManager settingsManager, String key) {
    }

    @Override
    protected void onPreviewCallback(byte[] data) {
        final byte[] tmp = data;
        if(canTakePicture()){
            new Thread(new Runnable() {
                @Override
                public void run() {
                    mBitmap = yuv2SmallBitmap(tmp);
                    mHandler.sendEmptyMessageDelayed(SET_BITMAP_TO_ADAPTER, 200);
                }
            }).start();
        }
    }

    @Override
    protected void startPreview() {
        Log.i(TAG, "startPreview start!");
        if (mCameraDevice == null) {
            Log.i(TAG, "attempted to start preview before camera device");
            // do nothing
            return;
        }

        if (!checkPreviewPreconditions()) {
            return;
        }
        /* SPRD: add for bug 380597: switch camera preview has a frame error @{ */
        mActivity.getCameraAppUI().resetPreview();
        /* @} */
        setDisplayOrientation();

        if (!mSnapshotOnIdle) {
            // If the focus mode is continuous autofocus, call cancelAutoFocus
            // to resume it because it may have been paused by autoFocus call.
            if (mFocusManager.getFocusMode(mCameraSettings.getCurrentFocusMode()) ==
                    CameraCapabilities.FocusMode.CONTINUOUS_PICTURE) {
                mCameraDevice.cancelAutoFocus();
            }
            mFocusManager.setAeAwbLock(false); // Unlock AE and AWB.
        }
        setCameraParameters(UPDATE_PARAM_ALL);

        mCameraDevice.setDisplayOrientation(/*mCameraDisplayOrientation*/mDisplayRotation, false);

        mCameraDevice.setPreviewDisplay(mCameraSurfaceHolder);

        CameraAgent.CameraStartPreviewCallback startPreviewCallback = new CameraAgent.CameraStartPreviewCallback() {
            @Override
            public void onPreviewStarted() {
                GifModule.this.onPreviewStarted();
                SessionStatsCollector.instance().previewActive(true);
                if (mSnapshotOnIdle) {
                    mHandler.post(mDoSnapRunnable);
                }
            }
        };

        mCameraDevice.startPreviewWithCallback(
                new Handler(Looper.getMainLooper()), startPreviewCallback);

        /*SPRD:fix bug 543222 Add callback in onPreviewStarted @{
        newDataBuffer();
        if(mCameraDevice != null){
            mCameraDevice.addCallbackBuffer(mPreviewCallBackBuffer);
        }
        if(mCameraDevice != null){
            mCameraDevice.setPreviewDataCallbackWithBuffer(mHandler,
                    mBufferedPreviewCallback);//Sprd Fix bug460517
        }
        @} */

        Log.i(TAG, "startPreview end!");
    }

    /*SPRD:fix bug 474858 modify for preview call back to reset @{*/
    @Override
    public void closeCamera() {
        if (mCameraDevice != null) {
            mCameraDevice.setPreviewDataCallbackWithBuffer(null, null);
        }
        super.closeCamera();
    }
    /* @} */

    /* SPRD: fix bug460517 switch camera crash @{*/
    @Override
    protected void switchCamera() {
        Log.i(TAG, "switchCamera start");
        if (mPaused) {
            return;
        }

        /*SPRD:add for Head upside down @{*/
        mGifModuleOrientation = mAppController.getOrientationManager().getDeviceOrientation().getDegrees();
        /*@}*/
        SettingsManager settingsManager = mActivity.getSettingsManager();

        closeCamera();
        mCameraId = mPendingSwitchCameraId;
        Log.i(TAG, "Start to switch camera. id=" + mPendingSwitchCameraId+" mCameraId="+mCameraId);
        settingsManager.set(mAppController.getModuleScope(), Keys.KEY_CAMERA_ID, mCameraId);
        requestCameraOpen();
        if (mFocusManager != null) {
            mFocusManager.removeMessages();
        }

        mMirror = isCameraFrontFacing();
        mFocusManager.setMirror(mMirror);
        Log.i(TAG, "switchCamera end");
    }
    /* @}*/

    private void onPreviewStarted() {
        mAppController.onPreviewStarted();
        mHandler.postDelayed(new Runnable(){
            @Override
            public void run() {
                mActivity.getCameraAppUI().onSurfaceTextureUpdated();
                /*SPRD:fix bug 543222 Add callBack in onPreviewStarted instead of startPreview @{*/
                newDataBuffer();
                if(mCameraDevice != null){
                    mCameraDevice.addCallbackBuffer(mPreviewCallBackBuffer);
                }
                if(mCameraDevice != null){
                    mCameraDevice.setPreviewDataCallbackWithBuffer(mHandler,
                            mBufferedPreviewCallback);//Sprd Fix bug460517
                }
                /* @} */
            }
        },800);
        setCameraState(IDLE);
    }

    @Override
    public void resume() {
        // CID 109221 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // numOfTakenPics = 0;
        mIsStartEditOrPlayActicity = false;
        SettingsManager settingsManager = mActivity.getSettingsManager();
        String picSize = settingsManager.getString(
                SettingsManager.SCOPE_GLOBAL, Keys.KEY_GIF_MODE_PIC_SIZE,
                mActivity.getString(R.string.pref_gif_mode_pic_size_default));
        String picNum = settingsManager.getString(
                SettingsManager.SCOPE_GLOBAL, Keys.KEY_GIF_MODE_NUM_SIZE,
                mActivity.getString(R.string.pref_gif_mode_pic_num_default));

        /*SPRD:add for Head upside down @{*/
        OrientationManager orientationManager = mAppController.getOrientationManager();
        orientationManager.addOnOrientationChangeListener(this);
        mGifModuleOrientation = orientationManager.getDeviceOrientation().getDegrees();
        /*@}*/
        //SPRD:fix bug 532741 switch to gifmodule in landscape, the shutterbutton show Vertically
        updateRotationUI(mGifModuleOrientation);
        mPicWidth = Integer.valueOf(picSize);
        numOfPics = Integer.valueOf(picNum);
        isGifRecording = false;
        mAdapter.setMaxItemCount(numOfPics);
        mGifProgressBar.setMax(numOfPics);
        setDisplayItemCountsInWindow(mGridView, mAdapter.getCount(), PER_SCREEEN_NUM);
        //SPRD:fix bug528687 the first selected photos is covered
        if (mAdapter.getCount()-1 > numOfPics) {
            mAdapter.recyleBitmaps();
            setCapturingProgressText("");
            changeUI(false);
        }
        super.resume();
        /* SPRD: Fix bug 572826 @{ */
        mActivity.getModuleLayoutRoot().removeView(mSufaceRootView);
        mActivity.getModuleLayoutRoot().addView(mSufaceRootView, 1,
                new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT,LayoutParams.MATCH_PARENT));
        mPreviewSurfaceView.setVisibility(View.VISIBLE);
        /* @} */
        // SPRD Bug:519334 Refactor Rotation UI of Camera.
        // setScreenOrientation();
        mActivity.getCameraAppUI().setBottomBarVisible(false);
        mCameraThumbnailThread = new CameraThumbnailThread();
        mCameraThumbnailThread.start();

        /* SPRD: Fix bug 560276 reset scenery @{ */
        if (settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL,
                Keys.KEY_GIF_MODE_SWITCHER)) {
            onSwitchChanged(null, true);
            mSwitcher.setSwitch(true);
            settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_GIF_MODE_SWITCHER, false);
        }
        /* @} */
    }

    /* SPRD: fix bug462430 can not generate picture of gif in land @{ */
    @Override
    public void pause() {
        super.pause();
        // SPRD: Fix bug 572826
        if (!mIsStartEditOrPlayActicity) { // SPRD: Fix bug 577075
            // SPRD Bug:519334 Refactor Rotation UI of Camera.
            // setScreenOrientationAuto();
            mActivity.getModuleLayoutRoot().removeView(mSufaceRootView);
        }
        /*SPRD:add for Head upside down @{*/
        mAppController.getOrientationManager().removeOnOrientationChangeListener(this);
        /*@}*/
    }
    /* @}*/

    public Bitmap yuv2SmallBitmap(byte[] yuvData) {
        int bmpWidth = mPicWidth;// square window
        int yuvWidth = mPreviewWidth;//mPreviewWidth;
        int yuvHeight = mPreviewHeight;//mPreviewHeight;

        if (yuvWidth <= 0 || yuvHeight <= 0 ) {
            return null;
        }
        // get bitmap with correct direction
        Bitmap bitmap = null;
        try {
            bitmap = yuv2Bitmap(yuvData,yuvWidth, yuvHeight);
        } catch (OutOfMemoryError e ) {
            LogUtils.error(TAG, "OutOfMemoryError " +e);
            return null;
        }

        // interception
        int x = 0;
        int y = 0;
        int widthCut;
        if (bitmap.getWidth() > bitmap.getHeight()) {
            widthCut = bitmap.getHeight();
            x = (bitmap.getWidth() - widthCut)/2;
        } else {
            widthCut = bitmap.getWidth();
            y = (bitmap.getHeight() - widthCut)/2;
        }

        Matrix matrix = new Matrix();
        matrix.postScale((float)bmpWidth/widthCut,(float)bmpWidth/widthCut);
        Bitmap b = Bitmap.createBitmap(bitmap, x, y, widthCut, widthCut,matrix, true);

        if (b != bitmap) {
            BitmapUtils.recycleSilently(bitmap);
        }
        /*if(Compatible.instance().mIsMeiZuM031 && mCameraId == Const.CAMERA_FRONT){
            return RotationUtil.rotateBmpToDisplay(bitmap, 180, mCameraId);
        }*/

        /*if(Compatible.instance().mIsIS11N && mCameraId == Const.CAMERA_FRONT && (mLastOrientation + 360 ) % 180 == 0){
            return RotationUtil.rotateBmpToDisplay(bitmap, 180, mCameraId);
        }*/

        /* FIX BUG : 4461
         * BUG COMMENT : the data of front camera is mirror in gif portrait mode
         * DATE : 2013-07-16
         */
        if(mCameraId == android.hardware.Camera.CameraInfo.CAMERA_FACING_FRONT && (mGifModuleOrientation + 360) % 180 == 0){//SPRD:fix bug464712/472947
            return BitmapUtils.rotateBmpToDisplay(mActivity, b, mGifModuleOrientation + 180, mCameraId);//SPRD:fix bug472947
        }

        return BitmapUtils.rotateBmpToDisplay(mActivity, b, mGifModuleOrientation, mCameraId);//SPRD:fix bug472947
    }

    public Bitmap yuv2Bitmap(byte[] yuvData, int width, int height) {
        return Bitmap.createBitmap(ImageProcessJni.Yuv2RGB888(yuvData, width, height), width, height, Bitmap.Config.ARGB_8888);
    }

    public void gotoEditActivity(){
        if(mAdapter.getAllBitmap() != null && !mIsStartEditOrPlayActicity){
            mIsStartEditOrPlayActicity = true;
            CameraApp info = (CameraApp)mActivity.getApplication();
            Bitmap[] bitmaps = new Bitmap[mAdapter.getItemCount()];
            for (int i = 0; i < mAdapter.getItemCount(); i++) {
                bitmaps[i] = mAdapter.getItem(i).copy(Config.ARGB_8888, true);

            }
            info.setGifBitmaps(bitmaps, mAdapter.getItemCount());
            Intent intent = new Intent(mActivity, GifEditActivity.class);
            intent.putExtra("fromOutSide", false);
            intent.setFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);// SPRD:Fix bug547258, make sure only the latest activity exist in the current task.
            mActivity.startActivity(intent);
            mAdapter.recyleBitmaps();
        }
    }

    class ThumbnailListener implements CollageMenuAdapter.ThumbnailListener{
        @Override
        public void thumbnailChanged() {
            mHandler.sendEmptyMessage(THUMBNAIL_CHANGED);
        }
    }

    private Bitmap getScaleBitmap(Uri uri) {
        String filePath = Utils.getFilePathByUri(uri, mActivity.getContentResolver());
        if(TextUtils.isEmpty(filePath)) return null;

        int bmpWidth = mPicWidth;
        BitmapFactory.Options options = Utils.getNativeAllocOptions();
        options.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(filePath, options);

        options.inJustDecodeBounds = false;
        int be;
        if (options.outHeight <= options.outWidth) {
            be = (int) (options.outHeight / (float) bmpWidth);
        } else {
            be = (int) (options.outWidth / (float) bmpWidth);
        }
        if (be <= 0)
            be = 1;
        options.inSampleSize = be;
        Bitmap bmp = null;
        try {
            bmp = BitmapFactory.decodeFile(filePath, options);
        } catch (OutOfMemoryError oom) {
            Log.w(TAG, "OOM occured when decode the file.", oom);
        }
        if (bmp != null) {
            int widthCut = Math.min(bmp.getWidth(), bmp.getHeight());
            int x = widthCut < bmp.getWidth() ? (bmp.getWidth() - widthCut) / 2 : 0;
            int y = widthCut < bmp.getHeight() ? (bmp.getHeight() - widthCut) / 2 : 0;
            float scale = (float) bmpWidth / widthCut;
            Matrix matrix = new Matrix();
            matrix.setScale(scale, scale);

            Bitmap tmp = Bitmap.createBitmap(bmp, x, y, widthCut, widthCut, matrix, true);
            if(tmp!= null && !tmp.sameAs(bmp)) {
                Utils.recycleBitmap(bmp);
            }
            bmp = tmp;

            int degree = Utils.getExifOrientation(filePath);
            if (degree != 0) {
                bmp = Utils.rotate(bmp, degree);
            }
        }
        return bmp;
    }

    private void setCapturingProgressText(String text){
        if (mPicProgressTextView != null) {
            mPicProgressTextView.setText(text);
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        LogUtils.debug(TAG, "surfaceChanged:%s, %dx%d", holder, width, height);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        LogUtils.debug(TAG, "surfaceCreated: " + holder);
        mCameraSurfaceHolder = holder;
        // Do not access the camera if camera start up thread is not finished.
        if (mCameraDevice == null) {
            return;
        }

        mCameraDevice.setPreviewDisplay(holder);
        // This happens when onConfigurationChanged arrives, surface has been
        // destroyed, and there is no onFullScreenChanged.
        if (mCameraState == PREVIEW_STOPPED) {
            setupPreview();
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        LogUtils.debug(TAG, "surfaceDestroyed: " + holder);
        mCameraSurfaceHolder = null;
        //stopPreview();
    }

    @Override
    public void onSizeChanged() {
    }

    protected void setLastPictureThumb(Uri uri, Bitmap thumb) {
        if (null == mThumbController)
            return;
        if(uri == null) {
            mThumbController.setData(thumb,false);
        }else {
            mThumbController.setData(uri, thumb,false);
        }
    }

    class CameraThumbnailThread extends Thread{
        @Override
        public void run() {
            StorageUtil storageUtil = StorageUtil.getInstance();
            String filePath = storageUtil.getFileDir();
            /* SPRD: Bug: 532873: filePath null pointer exception @{ */
            BaseImage baseImage = null;

            if(filePath == null){
                if(Environment.MEDIA_MOUNTED.equals(Environment.getExternalStoragePathState())){
                    Toast.makeText(
                            mActivity,
                            mActivity.getResources()
                                    .getString(R.string.externalstorage_removed),
                            Toast.LENGTH_SHORT).show();
                }
            } else {
                String bucketId = StorageUtil.getImageBucketId(filePath);
                /* SPRD: CID 109289: SA: Useless self-operation (FB.SA_LOCAL_DOUBLE_ASSIGNMENT) @{ */
                // BaseImage baseImage = baseImage = mImageManager.getLastGif(mActivity, mContentResolver, bucketId);
                baseImage = mImageManager.getLastGif(mActivity, mContentResolver, bucketId);
                /* @} */
            }
            /* @} */

            if(baseImage != null && baseImage.getUri() != null && baseImage.getBitmap() != null){
                mHandler.obtainMessage(UPDATE_THUMBNAIL,Pair.create(baseImage.getUri(), baseImage.getBitmap())).sendToTarget();
            }else{
                if(mThumbController != null){
                    mThumbController.setData(null, null, false);
                }
                mHandler.sendEmptyMessage(HIDE_THUMBNAIL_IMAGE);
            }
        }
    }

    private final ButtonManager.ButtonCallback mFlashCallback = new ButtonManager.ButtonCallback() {
        @Override
        public void onStateChanged(int state) {
            // Update flash parameters.
            enableTorchMode(true);
        }
    };

    private void enableTorchMode(boolean enable) {
        if (mParameters == null) {
            return;
        }

        if (enable) {
            updateParametersFlashMode();
        } else {
            CameraCapabilities.Stringifier stringifier = mCameraCapabilities.getStringifier();
            stringifier.flashModeFromString("off");
        }

        if (mCameraDevice != null) {
            mCameraDevice.applySettings(mCameraSettings);
        }
    }

    @Override
    public CameraAppUI.BottomBarUISpec getBottomBarSpec() {
        CameraAppUI.BottomBarUISpec bottomBarSpec = new CameraAppUI.BottomBarUISpec();

        bottomBarSpec.enableCamera = true;
        bottomBarSpec.cameraCallback = mCameraCallback;
        bottomBarSpec.enableTorchFlash = true;
        bottomBarSpec.flashCallback = mFlashCallback;
        bottomBarSpec.hideGridLines = true;
        bottomBarSpec.hideHdr = true;
        bottomBarSpec.hideVGesture = true;// SPRD: fix bug 567724

        return bottomBarSpec;
    }

    public void resetLayout() {
        /* SPRD: fix bug539498 shows gap between Video icon area and option button area @{ */
        updatePreviewAspectRatio(DEFAULT_ASPECT_RATIO);
        int bottom = mActivity.getResources().getDimensionPixelSize(R.dimen.mode_option_overlay_gif_height);
        mModeOptionsOverLay.setPadding(0,0,0,bottom);
        /* @} */
    }

    /* SPRD: fix bug549564  CameraProxy uses the wrong API @{ */
    @Override
    public boolean checkCameraProxy() {
        return false ^ !getCameraProvider().isNewApi();
    }
    /* @} */
}
