/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucam.modules.ugif;

import java.util.HashMap;
import java.util.List;

import android.R.integer;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.RectF;
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
import com.dream.camera.ButtonManagerDream;
import com.sprd.camera.storagepath.StorageUtil;
import com.sprd.camera.storagepath.StorageUtilProxy;
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
import com.dream.camera.dreambasemodules.DreamInterface;
import com.dream.camera.settings.DataModuleBasic;

import android.hardware.Camera.Parameters;

import com.android.ex.camera2.portability.CameraAgent.CameraProxy;
import com.android.camera.module.ModuleController;

public abstract class GifModule extends BasicModule implements
        Switcher.OnSwitchListener, SurfaceHolder.Callback,
        View.OnClickListener, PreviewFrameLayout.OnSizeChangedListener,
        ShutterButton.OnShutterButtonListener,
        OrientationManager.OnOrientationChangeListener {

    private static final String TAG = "DGifModule";

    public static final int SET_BITMAP_TO_ADAPTER = 1;
    public static final int SHOW_CONFIRM_BUTTON = 2;
    public static final int THUMBNAIL_CHANGED = 3;
    private static final int UPDATE_THUMBNAIL = 4;
    private static final int HIDE_THUMBNAIL_IMAGE = 5;

    private static final int REQUEST_CODE_PICK_IMAGE = 0xABCDE;

    private static final int PER_SCREEEN_NUM = 5;

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
    // private RotateImageView mGifListButton;
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
    private boolean mIsStartEditOrPlayActicity = false;
    private boolean isStartEditActicity = false;// SPRD: Fix bug 577075
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
    protected FrameLayout mBlackFramePreview;
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
    public boolean mPreviewing = false; // True if preview is started.

    public final Handler mHandler = new MainHandler();

    private class MainHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case SET_BITMAP_TO_ADAPTER: {
                    if (SWITCH_CONTINUE_MODE) {
                        if (isGifRecording) {
                            mAdapter.setThumbnail(mBitmap);
                            moveGifGrid();
                            mGifProgressBar.setProgress(mAdapter.getItemCount());
                            mProgressTextView.setText((mAdapter.getItemCount()) + "/" + numOfPics);
                            if (numOfPics - mAdapter.getItemCount() > 0) {
                                addPreviewBuffer();
                            } else {
                                isGifRecording = false;
                            }
                        }
                    } else {
                        if (isGifCapturing) {
                            mAdapter.setThumbnail(mBitmap);
                            moveGifGrid();
                            setCapturingProgressText(mAdapter.getItemCount() + "/" + numOfPics);
                            isGifCapturing = false;
                        }
                    }

                    mBitmap = null;
                    if (mAdapter.getItemCount() == numOfPics) {
                        if (SWITCH_CONTINUE_MODE && mShutterSound != null && mAppController.isPlaySoundEnable()) {
                            mShutterSound.playUgifVideoSound();
                        }
                        changeUI(false);
                        setCapturingProgressText("");
                        gotoEditActivity();
                    }
                    break;
                }
                case SHOW_CONFIRM_BUTTON: {
                    changeUI(true);
                    break;
                }
                case THUMBNAIL_CHANGED: {
                    if (mAdapter != null && mAdapter.getCount() == 0) {
                        changeUI(false);
                    }
                    String progressText = null;
                    if (mAdapter != null && mAdapter.getItemCount() > 0) {
                        progressText = mAdapter.getItemCount() + "/" + numOfPics;
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
                    if (mThumbnailView != null) {
                        mThumbnailView.setVisibility(View.GONE);
                    }
                    break;
                }
                default: {
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
         * FIX BUG : 4754 BUG COMMENT : do not allow capture when loading images of selected from
         * gallery DATE : 2013-08-26
         */
        if (numOfPics == mAdapter.getItemCount() || !canTakePicture() || mIsLoadingImage
                || isStartEditActicity
                || mAppController.getCameraAppUI().isModeListOpen()) {
            return;
        }

        final long storageSpaceBytes = mActivity.getStorageSpaceBytes();
        if (storageSpaceBytes <= Storage.LOW_STORAGE_THRESHOLD_BYTES) {
            Log.w(TAG, "Low storage warning: " + storageSpaceBytes);
            return;
        }

        if (SWITCH_CONTINUE_MODE) {
            /**
             * BUG FIX: 3119 BUG CAUSE: Shutter sound not play when taking video in Ugif mode. DATE:
             * 2013-03-11
             */
            if(mAppController.isPlaySoundEnable()){
                mShutterSound.playUgifVideoSound();
            }
            if (!isGifRecording) {
                mActivity.getCameraAppUI().hideModeOptions();
                mActivity.getCameraAppUI().setSwipeEnabled(false);
                isGifRecording = true;
            } else {
                mActivity.getCameraAppUI().enableModeOptions();
                mActivity.getCameraAppUI().setSwipeEnabled(true);
                gifFinish();
                return;
            }
        } else {
            if (!isGifCapturing) {
                mActivity.getCameraAppUI().setSwipeEnabled(false);
                isGifCapturing = true;
                if(mAppController.isPlaySoundEnable()){
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
                if (mAppController.isPlaySoundEnable() && SWITCH_CONTINUE_MODE) {
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
                if (mAppController.isPlaySoundEnable() && SWITCH_CONTINUE_MODE) {
                    mShutterSound.playUgifVideoSound();
                }
                gifFinish();
                break;
            // case R.id.gif_list_button:
            case R.id.review_thumbnail_layout:
            case R.id.review_thumbnail:
                if (!checkStorage())
                    return;
                // mActivity.startActivity(new Intent(mActivity, GifBrowser.class));
                // viewLastPicture();
                Intent intent = new Intent(mActivity, GifPlayActivity.class);
                Bundle b = new Bundle();
                b.putBoolean("fromOutSide", false);
                intent.putExtras(b);
                intent.setData(mThumbController.getUri());
                mActivity.startActivity(intent);
                break;
        }
    }

    private void gifFinish() {
        if (mAdapter.getItemCount() > 1) {
            gotoEditActivity();
            changeUI(false);
            setCapturingProgressText("");
            isGifRecording = false;
        } else {
            Toast.makeText(mActivity,
                    mActivity.getResources().getString(R.string.gif_compose_edit_tips),
                    Toast.LENGTH_SHORT).show();
        }
    }

    private boolean canUIPressed() {
        if (mCameraState == SNAPSHOT_IN_PROGRESS)
            return false;
        if (mCameraState == PREVIEW_STOPPED)
            return false;
        if (mCameraState == SWITCHING_CAMERA)
            return false;
        return true;
    }

    @Override
    public void makeModuleUI(PhotoController controller, View parent) {
        // TODO Waiting for merge of WideAngle feature, noted by spread
        // mActivity.getCameraAppUI().setLayoutAspectRation(0f);
        mUI = createUI(controller, parent);
        initializeModuleControls();
        mActivity.setPreviewStatusListener(mUI);
    }

    // Add for dream camera
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
        mGifCancelBtn = (RotateImageView) mRootView.findViewById(R.id.gif_cancel);
        mGifOkBtn = (RotateImageView) mRootView.findViewById(R.id.gif_finish);
        mGifCancelBtn.setOnClickListener(this);
        mGifOkBtn.setOnClickListener(this);
    }

    protected void initializeModuleControls() {
        mRootView = mActivity.getCameraAppUI().getModuleView();
        mActivity.getLayoutInflater().inflate(R.layout.gif_module, mRootView);

        // CID 109221 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mPreviewBgLayout = (RelativeLayout) mRootView.findViewById(R.id.gif_preview_bg_layout);
        mPreviewFrameLayout = (PreviewFrameLayout) mRootView.findViewById(R.id.frame);
        mPreviewFrameLayout.setOnSizeChangedListener(this);
        mPreviewFrameLayout.setGifMode(true);

        mPreviewSurfaceView = (PreviewSurfaceView) mRootView
              .findViewById(R.id.preview_surface_view);
        mPreviewSurfaceView.setVisibility(View.VISIBLE);
        mPreviewSurfaceView.getHolder().addCallback(this);

        mBlackFramePreview = (FrameLayout) mRootView.findViewById(R.id.gif_black_frame);
        initializeCpatureBottomBar();

        mGifGridViewScroller = (View) mRootView
                .findViewById(R.id.collage_framelayout_scroller_nav_bottom);
        if (SWITCH_CONTINUE_MODE) {
            mGifGridViewScroller.setVisibility(View.GONE);
        }
        mGridView = (GridView) mRootView.findViewById(R.id.collage_gv_nav_bottom);
        mGridView.setOnItemClickListener(mGridViewOnItemClickListener);
        mGridViewScrollView = (HorizontalScrollView) mRootView
                .findViewById(R.id.collage_scroller_nav_bottom);
        resizeGridViewScrollView();
        mPicProgressTextView = (TextView) mRootView.findViewById(R.id.gif_pic_progress_text);

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
        layoutParams.width = UiUtils.screenWidth() - UiUtils.effectItemWidth();
        mGridViewScrollView.setLayoutParams(layoutParams);
    }

    @Override
    protected void setModeOptionsLayout() {
        mModeOptionsOverLay.checkOrientation(Configuration.ORIENTATION_PORTRAIT);
        mModeOptionsButton.checkOrientation(Configuration.ORIENTATION_PORTRAIT);
        /*
         * if (mModeOptionsToggle != null) { FrameLayout.LayoutParams paramsToggle =
         * (FrameLayout.LayoutParams) mModeOptionsToggle.getLayoutParams(); if (!mPaused) {
         * mModeToggleBottomMargin = paramsToggle.bottomMargin; paramsToggle.bottomMargin = 48; }
         * else { paramsToggle.bottomMargin = mModeToggleBottomMargin; }
         * mModeOptionsToggle.setLayoutParams(paramsToggle); }
         */
        if (mModeOptions != null) {
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

        changeCaptureBottomBarUI(visible, capturing);
        // mGifListButton.setVisibility(View.GONE - visible);

        // mThumbnailShadowView.setVisibility(View.GONE - visible);

        changeCaptureTopPanelUI(capturing);
        if (SWITCH_CONTINUE_MODE && capturing) {
            // mShutterButton.startAnimation(VideoUtils.createFlickerAnimation(800));
            // mActivity.getSwitcherButton().setEnabled(false);
        } else if (SWITCH_CONTINUE_MODE && !capturing) {

            mGifProgressBar.setProgress(0);
            mProgressTextView.setText("");
            // mActivity.getSwitcherButton().setEnabled(true);
        }

    }

    @Override
    public void onCameraAvailable(CameraProxy cameraProxy) {
        super.onCameraAvailable(cameraProxy);
        if (mAdapter.getCount() > 1) {
            changeCaptureTopPanelUI(true);
        }
    }

    // Add for dream camera
    //@{
    protected void changeCaptureTopPanelUI(boolean disable) {}

    protected void changeCaptureBottomBarUI(int visible, boolean capturing) {
        mSwitcher.setVisibility(View.GONE - visible);
        mSwitcherCameraIcon.setVisibility(View.GONE - visible);
        mSwitcherVideoIcon.setVisibility(View.GONE - visible);
        mThumbnailLayout.setVisibility(View.GONE - visible);
        if (SWITCH_CONTINUE_MODE && !capturing) {
            mShutterButton.clearAnimation();
        }
        mGifCancelBtn.setVisibility(visible);
        mGifOkBtn.setVisibility(visible);
    }
    //@}

    private OnItemClickListener mGridViewOnItemClickListener = new OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            if (!canUIPressed()) {
                return;
            }
            if (position == mAdapter.getItemCount()) {
                int remains = mAdapter.getMaxItemCount() - mAdapter.getItemCount();
                ImageGallery.showImagePicker(mActivity, REQUEST_CODE_PICK_IMAGE, remains,
                        ViewImage.IMAGE_ENTRY_UGIF_VALUE);
            }
        }
    };

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (data == null) {
            return;
        }

        if (resultCode == Activity.RESULT_CANCELED)
            return;
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
                            if (params != null && params.length > 0) {
                                mHandler.sendEmptyMessage(SHOW_CONFIRM_BUTTON);
                            }
                            // fix coverity issue 108963
                            if (params != null) {
                                for (Uri u : params) {
                                    if (u == null)
                                        continue;

                                    Bitmap bitmap = getScaleBitmap(u);
                                    if (bitmap == null)
                                        continue;

                                    publishProgress(Pair.create(u, bitmap));
                                }
                            }

                            mHandler.postDelayed(new Runnable() {
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
                                String progressText = mAdapter.getItemCount() + "/" + numOfPics;
                                if (mAdapter.getItemCount() == numOfPics) {
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
                 * No Proper REQUEST CODE, so just close myself. Used in TapnowLicenseCheck Please
                 * use RESULT_CLOSE_ME instead.
                 */
                mActivity.finish();
            }
        }
    }

    /*
     * @Override protected void addTouchReceivers(PreviewGestures gestures) {
     * super.addTouchReceivers(gestures);
     * gestures.addTouchReceiver(mRootView.findViewById(R.id.collage_framelayout_scroller_nav_bottom
     * )); gestures.addTouchReceiver(mRootView.findViewById(R.id.collage_menu_item_layout_id));
     * gestures.addTouchReceiver(mActivity.findViewById(R.id.top_bar)); }
     */

    /**
     * ucamera original code
     * 
     * @Override public void onOrientationChanged(int orientation) {
     *           super.onOrientationChanged(orientation); //TODO Optimized in the future, noted by
     *           spread mGifModuleOrientation = CameraUtil.roundOrientation(orientation,
     *           mGifModuleOrientation); //mGifModuleOrientation = orientation; // When the screen
     *           is unlocked, display rotation may change. // Always // calculate the up-to-date
     *           orientationCompensation. int orientationCompensation = (mGifModuleOrientation +
     *           CameraUtil.getDisplayRotation()) % 360; // Rotate camera mode icons in the switcher
     *           if (mOrientationCompensation != orientationCompensation) { mOrientationCompensation
     *           = orientationCompensation; updateRotationUI(mGifModuleOrientation); } }
     */

    /* SPRD:add for Head upside down */
    @Override
    public void onOrientationChanged(OrientationManager orientationManager,
            OrientationManager.DeviceOrientation deviceOrientation) {
        mGifModuleOrientation = deviceOrientation.getDegrees();
        /* SPRD:fix for gif bug, small camera and video icon can not rotate@{ */
        // Always // calculate the up-to-date orientationCompensation.
        int orientationCompensation =
                (mGifModuleOrientation + CameraUtil.getDisplayRotation()) % 360;
        // Rotate camera mode icons in the switcher
        if (mOrientationCompensation != orientationCompensation) {
            mOrientationCompensation = orientationCompensation;
            updateRotationUI(mGifModuleOrientation);
        }
        /* @} */
        setBlackFrameFormLandFilmstrip();
    }

    private void setBlackFrameFormLandFilmstrip() {
        if (mActivity != null && mActivity.isFilmstripVisible()) {
            if ((mGifModuleOrientation + 360) % 180 != 0) {
                if (mBlackFramePreview != null && mBlackFramePreview.getVisibility() == View.GONE) {
                    mBlackFramePreview.setVisibility(View.VISIBLE);
                }
            } else {
                if (mBlackFramePreview != null && mBlackFramePreview.getVisibility() == View.VISIBLE) {
                    mBlackFramePreview.setVisibility(View.GONE);
                }
            }
        } else {
            if (mBlackFramePreview != null && mBlackFramePreview.getVisibility() == View.VISIBLE)
                mBlackFramePreview.setVisibility(View.GONE);
        }
    }
    protected void updateRotationUI(int orientation) {
        int rotation = orientation;
        if (orientation % 180 == 90) {
            rotation = orientation + 180;
        }

        if (mRootView.findViewById(R.id.review_thumbnail_layout) != null) {
            mRootView.findViewById(R.id.review_thumbnail_layout).setRotation(rotation);
        }
        // if(mRootView.findViewById(R.id.review_thumbnail_shadow) != null) {
        // mRootView.findViewById(R.id.review_thumbnail_shadow).setRotation(rotation);
        // }

        mGifCancelBtn.setOrientation(orientation, true);
        // mGifListButton.setOrientation(orientation, true);

        updateCaptureBottomBarUI(orientation, rotation);
    }

    // Add for dream camera
    protected void updateCaptureBottomBarUI(int orientation, int rotation) {
        if (mSwitcherVideoIcon != null) {
            mSwitcherVideoIcon.setRotation(rotation);
        }
        if (mSwitcherCameraIcon != null) {
            mSwitcherCameraIcon.setRotation(rotation);
        }
        if (mThumbnailLayout != null) {
            mThumbnailLayout.setRotation(rotation);
        }
        mShutterButton.setOrientation(orientation, true);
        mGifOkBtn.setOrientation(orientation, true);
    }

    private void moveGifGrid() {
        setDisplayItemCountsInWindow(mGridView, mAdapter.getCount(), PER_SCREEEN_NUM);
        int itemWidth = mAdapter.getGridItemWidth();
        int count = mAdapter.getCount();
        HorizontalScrollView horSGifView = (HorizontalScrollView) (mRootView
                .findViewById(R.id.collage_scroller_nav_bottom));

        if (horSGifView != null) {
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

            if (mGifGridViewScroller != null) {
                mGifGridViewScroller.setVisibility(View.GONE);
            }
            if (mGifProgressLayout != null) {
                mGifProgressLayout.setVisibility(View.VISIBLE);
            }
            SWITCH_CONTINUE_MODE = true;
        } else {

            if (mGifProgressLayout != null) {
                mGifProgressLayout.setVisibility(View.GONE);
            }
            if (mGifGridViewScroller != null) {
                mGifGridViewScroller.setVisibility(View.VISIBLE);
            }
            SWITCH_CONTINUE_MODE = false;
        }
        onCaptureBottomBarSwitchChanged(source, onOff);
        return true;
    }

    // Add for dream camera
    protected void onCaptureBottomBarSwitchChanged(Switcher source, boolean onOff) {
        if (onOff) {
            mShutterButton.setImageResource(R.drawable.ic_control_video_mode);
            mSwitcherCameraIcon.setImageResource(R.drawable.ic_control_camera);
            mSwitcherVideoIcon.setImageResource(R.drawable.ic_control_video_pressed);

        } else {
            mShutterButton.setImageResource(R.drawable.ic_control_camera_mode);
            mSwitcherCameraIcon.setImageResource(R.drawable.ic_control_camera_pressed);
            mSwitcherVideoIcon.setImageResource(R.drawable.ic_control_video);

        }

    }

    // ////////////////////////////////////////////////////////////////////////
    // ////////////// Override super class function
    // ////////////////////////////////////////////////////////////////////////

    @Override
    protected void requestCameraOpen() {
        LogUtils.debug(TAG, "requestCameraOpen mCameraId:" + mCameraId);
        mActivity.getCameraProvider().requestCamera(mCameraId);
        SettingsManager mSettingsManager = mActivity.getSettingsManager();
        mDataModule.setValueByIndex(Keys.KEY_CAMERA_ID, mCameraId);// SPRD BUG:379926
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

        //SPRD bug fix 601435, enter into thumbnail and click on share icon,then click the Back button, flash keeps on flash.
        if (mActivity.getCameraAppUI().getFilmstripVisibility() == View.VISIBLE) {
            enableTorchMode(false);
        }

        if (mCameraDevice != null) {
            // String message = mParameters.flatten();
            // LogUtils.debug(TAG, " mParameters = " + message);
            mCameraDevice.applySettings(mCameraSettings);
        }
    }

    @Override
    protected void updateCameraParametersPreference() {

        if (mCameraSettings != null) {
            mCameraSettings.setDefault(mCameraId);//SPRD:fix bug616836 add for photo use api1 or api2 use reconnect
        }
        updateCameraShutterSound();
        updateParametersGifPhotoFlashMode();
        updateParametersFlashMode();
        updateGifModePicSize();
        updateGifModeNumSize();

        // Set flash mode.
        // updateParametersFlashMode();
        // set preview size
        setPreviewSize();

        // Set Anti-Banding
        updateParametersAntibanding();
        /*
         * FIX BUG: 5100 BUG CAUSE: gif add continuous focus function Date: 2013-10-30
         */
        // SPRD: Fix Bug 535139,Set white balance.
        // updateParametersWhiteBalance() ;

        // SPRD: Fix Bug 535139,Set color effect.
        // updateParametersColorEffect();
        List<String> supportedFocusModes = null;
        if (mParameters != null) {
            supportedFocusModes = mParameters.getSupportedFocusModes();
        }
        if (supportedFocusModes != null && supportedFocusModes.contains("continuous-video")) {
            CameraCapabilities.Stringifier stringifier = mCameraCapabilities.getStringifier();
            mCameraSettings.setFocusMode(stringifier.focusModeFromString("continuous-video"));
            mCameraSettings.setExposureCompensationIndex(0);
        }
    }

    @Override
    protected void updateSceneMode() {
    }

    @Override
    protected void updateParametersFlashMode() {
        //SPRD: Fix bug 631061 flash may be in error state when receive lowbattery broadcast
        String flash = getFlashMode(mDataModuleCurrent);
        if (flash == null) {
            return;
        }
        if (!isFlashEnable()) {
            return;
        }
        if (mIsBatteryLow && !"off".equals(flash)) {
            flash = "off";
        }
        CameraCapabilities.FlashMode flashMode = mCameraCapabilities
                .getStringifier().flashModeFromString(flash);
        if (mCameraCapabilities.supports(flashMode)) {
            Log.d(TAG, "mCameraCapabilities.supports(flashMode) = " + flashMode);
            mCameraSettings.setFlashMode(flashMode);
        }

        Log.d(TAG, "updateParametersFlashMode = " + getFlashMode(mDataModuleCurrent));
    }

    protected void updateGifModePicSize() {
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_GIF_MODE_PIC_SIZE)){
            return;
        }
        String picSize = mDataModuleCurrent.getString(Keys.KEY_GIF_MODE_PIC_SIZE);
        if (picSize == null) {
            picSize = mActivity.getString(R.string.pref_gif_mode_pic_size_default);
        }

        mPicWidth = Integer.valueOf(picSize);

        Log.d(TAG, "updateGifModePicSize = " + picSize);
    }

    protected void updateGifModeNumSize() {
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_GIF_MODE_NUM_SIZE)){
            return;
        }
        String picNum = mDataModuleCurrent.getString(Keys.KEY_GIF_MODE_NUM_SIZE);
        if (picNum == null) {
            picNum = mActivity.getString(R.string.pref_gif_mode_pic_num_default);
        }

        numOfPics = Integer.valueOf(picNum);
        mAdapter.setMaxItemCount(numOfPics);
        mGifProgressBar.setMax(numOfPics);

        // SPRD:fix bug528687 the first selected photos is covered
        if (mAdapter.getCount() - 1 > numOfPics) {
            mAdapter.recyleBitmaps();
            setCapturingProgressText("");
            changeUI(false);
        }

        Log.d(TAG, "updateGifModeNumSize = " + picNum);
    }

    protected void updateParametersAntibanding() {
        CameraCapabilities.Stringifier stringifier = mCameraCapabilities.getStringifier();
        SettingsManager settingsManager = mActivity.getSettingsManager();

        String mAntibanding = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                Keys.KEY_CAMER_ANTIBANDING);
        List<String> supportedAntibanding = null;
        if (mParameters != null) {
            supportedAntibanding = mParameters.getSupportedAntibanding();
        }
        if (mParameters != null && supportedAntibanding != null
                && supportedAntibanding.contains(mAntibanding)) {
            mCameraSettings.setAntibanding(stringifier.antibandingModeFromString(mAntibanding));
        }
    }

    @Override
    protected void setPreviewFrameLayoutAspectRatio() {
        /* SPRD:fix bug 462193 Occasionally generate gif error @{ */
        // ResolutionSize optimalSize = PreviewSize.instance(mCameraId).get(mParameters);
        Size optimalSize = mCameraSettings.getCurrentPreviewSize();
        /* @} */
        if (optimalSize != null) {
            if (mPreviewFrameLayout != null) {
                if (Utils.calcNeedRevert(mCameraId)) {
                    mPreviewFrameLayout.setGifAspectRatio((double) optimalSize.height()
                            / optimalSize.width(), true);
                } else {
                    mPreviewFrameLayout.setGifAspectRatio((double) optimalSize.width()
                            / optimalSize.height(), false);
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
         * FIX BUG: 5601 5603 5616 BUG COMMENT: can not get the correct preview size on some devices
         * DATE: 2013-12-18
         */
        ResolutionSize optimalSize = Utils.getGifPreviewSize(sizes);
        if (optimalSize == null) {
            optimalSize = Utils.getOptimalPreviewSize(mActivity, sizes, 4.0 / 3.0);
        }
        if (optimalSize != null) {
            mCameraSettings.setPreviewSize(new Size(optimalSize.width, optimalSize.height));
        }
    }

    @Override
    public void onSettingChanged(SettingsManager settingsManager, String key) {
    }

    @Override
    public void onDreamSettingChangeListener(
            HashMap<String, String> keys) {
        Log.e(TAG, "dream Photo onDreamSettingChangeListener  ");

        for (String key : keys.keySet()) {
            Log.e(TAG, "onSettingChanged key = " + key + " value = " + keys.get(key));
            switch (key) {
                case Keys.KEY_GIF_MODE_PIC_SIZE:
                    updateGifModePicSize();
                    break;
                case Keys.KEY_GIF_MODE_NUM_SIZE:
                    updateGifModeNumSize();
                    break;
                case Keys.KEY_JPEG_QUALITY:
                    break;
                case Keys.KEY_CAMERA_COMPOSITION_LINE:
                    break;
                case Keys.KEY_CAMERA_AI_DATECT:
                    break;
                case Keys.KEY_CAMER_ANTIBANDING:
                    break;
                case Keys.KEY_CAMERA_COLOR_EFFECT:
                    break;
                case Keys.KEY_CAMERA_PHOTOGRAPH_STABILIZATION:
                    break;
                case Keys.KEY_CAMERA_HDR_NORMAL_PIC:
                    break;
                case Keys.KEY_CAMERA_GRADIENTER_KEY:
                    break;
                case Keys.KEY_CAMERA_TOUCHING_PHOTOGRAPH:
                    break;
                case Keys.KEY_CAMERA_TIME_STAMP:
                    break;
                case Keys.KEY_FREEZE_FRAME_DISPLAY:
                    break;
                case Keys.KEY_CAMERA_ZSL_DISPLAY:
                    break;
                case Keys.KEY_CAMERA_BEAUTY_ENTERED:
                    break;
//                case Keys.KEY_FLASH_MODE:
//                    if (mPaused
//                            || mAppController.getCameraProvider().waitingForCamera()) {
//                        return;
//                    }
//                    updateParametersFlashMode();
//                    break;
                case Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE:
                    if (mPaused
                            || mAppController.getCameraProvider()
                                    .waitingForCamera()) {
                        return;
                    }
                    updateParametersGifPhotoFlashMode();
                    break;
                case Keys.KEY_VIDEOCAMERA_FLASH_MODE:
                    if (mPaused) {
                        break;
                    }
                    // Update flash parameters.
                    enableTorchMode(true);
                    break;
                case Keys.KEY_COUNTDOWN_DURATION:
                    break;
                case Keys.KEY_CAMERA_HDR:
                    break;
                case Keys.KEY_CAMER_METERING:
                    break;
            }
        }
        mActivity.getCameraAppUI().initSidePanel();
        if (mCameraDevice != null) {
            Log.d(TAG, "applySettings................ ");
            mCameraDevice.applySettings(mCameraSettings);
        }

    }

    private void updateParametersGifPhotoFlashMode() {
        String flash = mDataModuleCurrent.getString(Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE);
        if (flash == null) {
            return;
        }

        /*
         * the mutex was implemented in datamodulephoto
        if(flash.equals("torch")){
            mDataModuleCurrent.changeSettings(Keys.KEY_FLASH_MODE, "on");
        }else {
            mDataModuleCurrent.changeSettings(Keys.KEY_FLASH_MODE, "off");
        }
         */
        CameraCapabilities.FlashMode flashMode = mCameraCapabilities
                .getStringifier().flashModeFromString(flash);
        if (mCameraCapabilities.supports(flashMode)) {
            mCameraSettings.setFlashMode(flashMode);
        }

        Log.d(TAG, "updateParametersGifFlashMode = " + flashMode);
    }

    @Override
    protected void onPreviewCallback(byte[] data) {
        final byte[] tmp = data;
        if (canTakePicture()) {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    //SPRD:Fix bug 595400
                    if (mNeedFreeze) {
                        mNeedFreeze = false;
                        yuv2FreezeBitmap(tmp);
                    } else {
                        mBitmap = yuv2SmallBitmap(tmp);
                        mHandler.sendEmptyMessageDelayed(SET_BITMAP_TO_ADAPTER, 200);
                    }
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
        setDisplayOrientation();

        if (!mSnapshotOnIdle) {
            // If the focus mode is continuous autofocus, call cancelAutoFocus
            // to resume it because it may have been paused by autoFocus call.
            if (mFocusManager.getFocusMode(mCameraSettings.getCurrentFocusMode()) == CameraCapabilities.FocusMode.CONTINUOUS_PICTURE) {
                mCameraDevice.cancelAutoFocus();
            }
            mFocusManager.setAeAwbLock(false); // Unlock AE and AWB.
        }
        setCameraParameters(UPDATE_PARAM_ALL);

        mCameraDevice.setDisplayOrientation(/* mCameraDisplayOrientation */mDisplayRotation, false);

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

        /*SPRD:fix bug612383 set preview callback before startpreview @ {*/
        newDataBuffer();
        addPreviewBuffer();
        if (mCameraDevice != null) {
            // Sprd Fix bug460517
            mCameraDevice.setPreviewDataCallbackWithBuffer(mHandler, mBufferedPreviewCallback);
        }
        /* @} */
        mCameraDevice.startPreviewWithCallback(
                new Handler(Looper.getMainLooper()), startPreviewCallback);
        mPreviewing = true;
        Log.i(TAG, "startPreview end!");
    }

    /* SPRD:fix bug 595608 modify for flash always flash when back from thumbnail @{ */
    @Override
    public void stopPreview() {
        super.stopPreview();
        mPreviewing = false;
    }
    /* @} */

    /* SPRD:fix bug 474858 modify for preview call back to reset @{ */
    @Override
    public void closeCamera() {
        if (mCameraDevice != null) {
            mCameraDevice.setPreviewDataCallbackWithBuffer(null, null);
        }
        super.closeCamera();
    }

    /* @} */

    /* SPRD: fix bug460517 switch camera crash @{ */
    @Override
    protected void switchCamera() {
        Log.i(TAG, "AAA switchCamera start");
        if (mPaused) {
            return;
        }

        /* SPRD:add for Head upside down @{ */
        mGifModuleOrientation = mAppController.getOrientationManager().getDeviceOrientation()
                .getDegrees();
        /* @} */

        closeCamera();
        mCameraId = mPendingSwitchCameraId;
        Log.i(TAG, "Start to switch camera. id=" + mPendingSwitchCameraId + " mCameraId="
                + mCameraId);
        mDataModule.set(Keys.KEY_CAMERA_ID, mCameraId);
        requestCameraOpen();
        if (mFocusManager != null) {
            mFocusManager.removeMessages();
        }

        mMirror = isCameraFrontFacing();
        mFocusManager.setMirror(mMirror);
        Log.i(TAG, "switchCamera end");
    }

    /* @} */

    private void onPreviewStarted() {
        mAppController.onPreviewStarted();
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                mActivity.getCameraAppUI().onSurfaceTextureUpdated();
            }
        }, 500);
        setCameraState(IDLE);
    }

    public boolean isRefreshThumbnail() {
        if (mAdapter == null) {
            return true;
        }
        return mAdapter.getCount() > 1 ? false : true;
    }

    @Override
    public void resume() {
        // CID 109221 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // numOfTakenPics = 0;
        isStartEditActicity = false;
        String picSize = mDataModuleCurrent.getString(Keys.KEY_GIF_MODE_PIC_SIZE);
        String picNum = mDataModuleCurrent.getString(Keys.KEY_GIF_MODE_NUM_SIZE);

        /* SPRD:add for Head upside down @{ */
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
        // SPRD:fix bug528687 the first selected photos is covered
        if (mAdapter.getCount() - 1 > numOfPics) {
            mAdapter.recyleBitmaps();
            setCapturingProgressText("");
            changeUI(false);
        }
        super.resume();
        /* SPRD: Fix bug 572826 @{ */
        /*mActivity.getModuleLayoutRoot().removeView(mSufaceRootView);
        mActivity.getModuleLayoutRoot().addView(mSufaceRootView, 1,
                new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT,LayoutParams.MATCH_PARENT));
        mPreviewSurfaceView.setVisibility(View.VISIBLE);*/
        /* @} */

        // SPRD Bug:519334 Refactor Rotation UI of Camera.
        // setScreenOrientation();

        // Add for dream camera
        setCaptureBottomBarShow(true);
        mCameraThumbnailThread = new CameraThumbnailThread();
        mCameraThumbnailThread.start();
    }

    /* SPRD: fix bug462430 can not generate picture of gif in land @{ */
    @Override
    public void pause() {
        super.pause();
        /* SPRD:add for Head upside down @{ */
        mAppController.getOrientationManager().removeOnOrientationChangeListener(this);
        /* @} */
        /**
         * Add for dream camera: change module/switch camera when in gifShutter, photo/video button
         * and thumbnail disppear.
         * @{
         */
        changeCaptureBottomBarUI(View.GONE, false);
        changeCaptureTopPanelUI(false);
        mNeedFreeze = false;
        mFreezeforOnModeSelect = false;
        /* @} */
    }

    /* @} */

    // Add for dream camera
    protected void setCaptureBottomBarShow(boolean isVisible) {
        mActivity.getCameraAppUI().setBottomBarVisible(isVisible);
        /* SPRD: ui check 49 gifModule ui @{ */
        if (mAdapter.getCount() > 1) {
            changeUI(true);
        }
        /* @} */
    }

    /* SPRD: Fix bug 595400 the freeze screen for gif @{ */
    private Bitmap mFreezeBitmap = null;
    private boolean mNeedFreeze = false;
    
    private final long FREEZE_TIME_OUT = 100;
    private RectF mPreviewArea = null;
    public int mDelayTime = 500;
    private boolean mFreezeforOnModeSelect = false;
    private boolean mNeedFreezeBlur = false;
    private boolean mFreezeforResetMode = false;
    @Override
    public void freezeScreen(boolean needBlur, boolean needSwitch) {
        Log.i(TAG, "freezeScreen");
        mNeedFreeze = true;
        mNeedFreezeBlur = needBlur;
        addPreviewBuffer();
        mPreviewArea = mPreviewFrameLayout.getPreviewArea();
    }

    /* SPRD:fix bug616685 add freeze for module from gif to other @{ */
    @Override
    public void freezeScreenforGif(boolean needBlur, boolean resetMode) {
        Log.i(TAG, "freezeScreenforGif");
        mNeedFreeze = true;
        mNeedFreezeBlur = needBlur;
        mFreezeforOnModeSelect = true;
        addPreviewBuffer();
        mFreezeforResetMode = resetMode;
        if(mFreezeforResetMode) {
            mFreezeforOnModeSelect = false;
        }
        mPreviewArea = mPreviewFrameLayout.getPreviewArea();
    }
    /* @} */

    private void yuv2FreezeBitmap(byte[] yuvData) {
        Log.i(TAG, "yuv2FreezeBitmap");
        int yuvWidth = mPreviewWidth;
        int yuvHeight = mPreviewHeight;

        if (yuvWidth <= 0 || yuvHeight <= 0) {
            return;
        }

        if (mFreezeBitmap != null && !mFreezeBitmap.isRecycled()) {
            mFreezeBitmap.recycle();
            mFreezeBitmap = null;
        }

        try {
            mFreezeBitmap = yuv2Bitmap(yuvData, yuvWidth, yuvHeight);
        } catch (OutOfMemoryError e) {
            LogUtils.error(TAG, "OutOfMemoryError " + e);
            return;
        }

        if (mCameraId == android.hardware.Camera.CameraInfo.CAMERA_FACING_FRONT) {
            mFreezeBitmap = BitmapUtils.rotateBmpToDisplayandMirror(mActivity, mFreezeBitmap, 0, mCameraId);
        } else {
            mFreezeBitmap = BitmapUtils.rotateBmpToDisplay(mActivity, mFreezeBitmap, 0, mCameraId);
        }

        if (mNeedFreezeBlur) {
            mFreezeBitmap = CameraUtil.blurBitmap(CameraUtil.computeScale(mFreezeBitmap, 0.2f), (Context)mActivity);
        }
        //This must use two post to make sure the freeze screen has drawed.
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (!mPaused && mCameraDevice != null) {
                    mAppController.freezeScreenUntilPreviewReady(mFreezeBitmap, mPreviewArea);
                }
            }
        });

        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if (!mPaused && mCameraDevice != null) {
                    if (mFreezeforOnModeSelect) {
                        mFreezeforOnModeSelect = false;
                        mNeedFreezeBlur = false;
                        mActivity.switchModeSpecial();
                        mActivity.setWaitToChangeMode(false);//Fix bug 649158/628424
                    } else if (mFreezeforResetMode) {
                        mFreezeforResetMode = false;
                        mNeedFreezeBlur = false;
                        mActivity.onModeSelected(isPhotoModule() ? mActivity.getResources().getInteger(R.integer.camera_mode_auto_photo) : mActivity.getResources().getInteger(R.integer.camera_mode_auto_video));
                    } else {
                        mNeedFreezeBlur = false;
                        mActivity.switchFrontAndBackMode();
                        mActivity.getCameraAppUI().updateModeList();
                    }
                }
            }
        }, 50);
        return;
    }
    /* @} */

    public Bitmap yuv2SmallBitmap(byte[] yuvData) {
        int bmpWidth = mPicWidth;// square window
        int yuvWidth = mPreviewWidth;// mPreviewWidth;
        int yuvHeight = mPreviewHeight;// mPreviewHeight;

        if (yuvWidth <= 0 || yuvHeight <= 0) {
            return null;
        }
        // get bitmap with correct direction
        Bitmap bitmap = null;
        try {
            bitmap = yuv2Bitmap(yuvData, yuvWidth, yuvHeight);
        } catch (OutOfMemoryError e) {
            LogUtils.error(TAG, "OutOfMemoryError " + e);
            return null;
        }

        // interception
        int x = 0;
        int y = 0;
        int widthCut;
        if (bitmap.getWidth() > bitmap.getHeight()) {
            widthCut = bitmap.getHeight();
            x = (bitmap.getWidth() - widthCut) / 2;
        } else {
            widthCut = bitmap.getWidth();
            y = (bitmap.getHeight() - widthCut) / 2;
        }

        Matrix matrix = new Matrix();
        matrix.postScale((float) bmpWidth / widthCut, (float) bmpWidth / widthCut);
        Bitmap b = Bitmap.createBitmap(bitmap, x, y, widthCut, widthCut, matrix, true);

        if (b != bitmap) {
            BitmapUtils.recycleSilently(bitmap);
        }
        /*
         * if(Compatible.instance().mIsMeiZuM031 && mCameraId == Const.CAMERA_FRONT){ return
         * RotationUtil.rotateBmpToDisplay(bitmap, 180, mCameraId); }
         */

        /*
         * if(Compatible.instance().mIsIS11N && mCameraId == Const.CAMERA_FRONT && (mLastOrientation
         * + 360 ) % 180 == 0){ return RotationUtil.rotateBmpToDisplay(bitmap, 180, mCameraId); }
         */

        /*
         * FIX BUG : 4461 BUG COMMENT : the data of front camera is mirror in gif portrait mode DATE
         * : 2013-07-16
         */
        if (mCameraId == android.hardware.Camera.CameraInfo.CAMERA_FACING_FRONT
                && (mGifModuleOrientation + 360) % 180 == 0) {// SPRD:fix bug464712/472947
            return BitmapUtils.rotateBmpToDisplay(mActivity, b, mGifModuleOrientation + 180,
                    mCameraId);// SPRD:fix bug472947
        }

        return BitmapUtils.rotateBmpToDisplay(mActivity, b, mGifModuleOrientation, mCameraId);// SPRD:fix
                                                                                              // bug472947
    }

    public Bitmap yuv2Bitmap(byte[] yuvData, int width, int height) {
        return Bitmap.createBitmap(ImageProcessJni.Yuv2RGB888(yuvData, width, height), width,
                height, Bitmap.Config.ARGB_8888);
    }

    public void gotoEditActivity() {
        if (mAdapter.getAllBitmap() != null && !isStartEditActicity) {
            Activity activity = GifEditActivity.getInstance();
            if (activity != null)
                activity.finish();

            isStartEditActicity = true;
            CameraApp info = (CameraApp) mActivity.getApplication();
            Bitmap[] bitmaps = new Bitmap[mAdapter.getItemCount()];
            for (int i = 0; i < mAdapter.getItemCount(); i++) {
                bitmaps[i] = mAdapter.getItem(i).copy(Config.ARGB_8888, true);

            }
            info.setGifBitmaps(bitmaps, mAdapter.getItemCount());
            Intent intent = new Intent(mActivity, GifEditActivity.class);
            intent.putExtra("fromOutSide", false);
//            intent.setFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);// SPRD:Fix bug547258, make sure only the latest activity exist in the current task.
            mActivity.startActivity(intent);
            mAdapter.recyleBitmaps();
        }
    }

    class ThumbnailListener implements CollageMenuAdapter.ThumbnailListener {
        @Override
        public void thumbnailChanged() {
            mHandler.sendEmptyMessage(THUMBNAIL_CHANGED);
        }
    }

    private Bitmap getScaleBitmap(Uri uri) {
        String filePath = Utils.getFilePathByUri(uri, mActivity.getContentResolver());
        if (TextUtils.isEmpty(filePath))
            return null;

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
            if (tmp != null && !tmp.sameAs(bmp)) {
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

    private void setCapturingProgressText(String text) {
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
        // stopPreview();
    }

    @Override
    public void onSizeChanged() {
    }

    protected void setLastPictureThumb(Uri uri, Bitmap thumb) {
        if (null == mThumbController)
            return;
        if (uri == null) {
            mThumbController.setData(thumb, false);
        } else {
            mThumbController.setData(uri, thumb, false);
        }
    }

    class CameraThumbnailThread extends Thread {
        @Override
        public void run() {
            StorageUtil storageUtil = StorageUtil.getInstance();
            String filePath = storageUtil.getFileDir();
            /* SPRD: Bug: 532873: filePath null pointer exception @{ */
            BaseImage baseImage = null;

            if(filePath == null){
                if(Environment.MEDIA_MOUNTED.equals(StorageUtilProxy.getExternalStoragePathState())){
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

    public final ButtonManager.ButtonCallback mFlashCallback = new ButtonManager.ButtonCallback() {
        @Override
        public void onStateChanged(int state) {
            // Update flash parameters.
            Log.d(TAG, "onStateChanged " + state);
            enableTorchMode(true);
        }
    };

    public void enableTorchMode(boolean enable) {
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

    /* SPRD:fix bug 595608 modify for flash always flash when back from thumbnail @{ */
    @Override
    public void onPreviewVisibilityChanged(int visibility) {
        if (mPreviewing) {
            /**
             * SPRD BUG 507795: won't open the flash when enlarge the picture in
             * picture preview UI @{ Original Code enableTorchMode(visibility ==
             * ModuleController.VISIBILITY_VISIBLE );
             */

            // SPRD:fix bug519856 return DV, torch will flash first,then turn on
            // enableTorchMode(visibility == ModuleController.VISIBILITY_VISIBLE
            // &&
            // mActivity.getCameraAppUI().getFilmstripVisibility() !=
            // View.VISIBLE);
            /*CameraCapabilities.FlashMode flashMode;
            flashMode = CameraCapabilities.FlashMode.OFF;
            mCameraSettings.setFlashMode(flashMode);*/

            /*CameraCapabilities.Stringifier stringifier = mCameraCapabilities.getStringifier();
            stringifier.flashModeFromString("off");*/
            enableTorchMode(visibility != ModuleController.VISIBILITY_HIDDEN
                    && mActivity.getCameraAppUI().getFilmstripVisibility() != View.VISIBLE);
            }
            /* @} */
        setBlackFrameFormLandFilmstrip();
    }
    /* @} */

    @Override
    public CameraAppUI.BottomBarUISpec getBottomBarSpec() {
        CameraAppUI.BottomBarUISpec bottomBarSpec = new CameraAppUI.BottomBarUISpec();

        bottomBarSpec.enableCamera = true;
        bottomBarSpec.cameraCallback = mCameraCallback;
        bottomBarSpec.enableTorchFlash = true;
        bottomBarSpec.flashCallback = mFlashCallback;
        bottomBarSpec.hideGridLines = true;
        bottomBarSpec.hideHdr = true;

        return bottomBarSpec;
    }

    @Override
    public void onBeautyValueReset() {

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
        return !getCameraProvider().isNewApi() &&
                (mCameraId == getCameraProvider().getCurrentCameraId().getLegacyValue());
    }
    /* @} */

    /**
     * Add for dream camera
     * 
     * @param controller
     * @param parent
     * @return
     */
    public GifUI createUI(PhotoController controller, View parent) {
        return new GifUI(mCameraId, controller, mActivity, parent);
    }

    /* SPRD: ui check 49 gifModule ui @{ */
    public void onGifCancel() {
        if (isGifCapturing) {
            Log.e(TAG, "can not click cancel button when capturing");
            return;
        }
        if (mAppController.isPlaySoundEnable() && SWITCH_CONTINUE_MODE) {
            mShutterSound.playUgifVideoSound();
        }
        mAdapter.recyleBitmaps();
        setDisplayItemCountsInWindow(mGridView, mAdapter.getCount(), PER_SCREEEN_NUM);
        setCapturingProgressText("");
        changeUI(false);
        isGifRecording = false;
        // SPRD:fix bug533672 stop record gif, show mode options
        mActivity.getCameraAppUI().showModeOptions();
        mActivity.getCameraAppUI().setSwipeEnabled(true);
    }

    public void onGifFinish() {
        if (isGifCapturing) {
            Log.e(TAG, "can not click finish button when capturing");
            return;
        }
        if (mAppController.isPlaySoundEnable() && SWITCH_CONTINUE_MODE) {
            mShutterSound.playUgifVideoSound();
        }
        gifFinish();
    }
    /* @} */

    @Override
    public boolean isGifCapture() {
        if (mAdapter != null && mAdapter.getCount() > 1) {
            return true;
        }
        return false;
    }

    @Override
    protected void newDataBuffer() {
        Size previewSize = mCameraSettings.getCurrentPreviewSize();

        int previewPixels = 800 * 480; // max size of 800*480 screen
        if (previewSize != null) {
            previewPixels = previewSize.width() * previewSize.height();
        }

        calcYuvSize();
        int bufferSize = previewPixels * mBitPerPixels / 8;
        if (mPreviewCallBackBuffer == null
                || mPreviewCallBackBuffer.length != bufferSize) {
            mPreviewCallBackBuffer = new byte[bufferSize];
        }
        Log.i(TAG, "new callback Buffer " + bufferSize + " bytes.");
    }
}
