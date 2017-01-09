/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.res.Configuration;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.MediaStore;
import android.provider.MediaStore.Images;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.FrameLayout;
import android.widget.Gallery;
import android.widget.GridView;
import android.widget.HorizontalScrollView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.RelativeLayout;
import android.widget.Toast;

import com.android.camera.OnScreenHint;
import com.android.camera.app.CameraApp;
import com.android.camera.util.ToastUtil;
import com.android.camera2.R;
import com.sprd.camera.storagepath.StorageUtil;
import com.ucamera.ucam.ResourceInitializer;
import com.ucamera.ucam.jni.ImageProcessJni;
import com.ucamera.ucam.modules.ugif.GifDecoder;
import com.ucamera.ucam.modules.ugif.GifView;
import com.ucamera.ucam.modules.ugif.edit.BackGroundWorkTask.OnTaskFinishListener;
import com.ucamera.ucam.modules.ugif.edit.MyGallery.GalleryCallback;
import com.ucamera.ucam.modules.ugif.edit.callback.ProcessCallback;
import com.ucamera.ucam.modules.ugif.edit.cate.EffectType;
import com.ucamera.ucam.modules.ugif.edit.cate.GifEditCateAdapter;
import com.ucamera.ucam.modules.ugif.edit.cate.GifEditTypeAdapter;
import com.ucamera.ucam.modules.utils.LogUtils;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.ucamera.ucam.modules.utils.UiUtils;
import com.ucamera.ucam.modules.utils.Utils;
import com.ucamera.ucomm.downloadcenter.DownloadCenter;
import com.ucamera.ucomm.downloadcenter.DownloadCenter.OnLoadCallback;
import com.ucamera.uphoto.EffectCateBaseAdapter;
import com.ucamera.uphoto.EffectResInfo;
import com.ucamera.uphoto.EffectShowed;
import com.ucamera.uphoto.EffectTypeResource;
import com.ucamera.uphoto.EffectTypeResource.EffectItem;
import com.ucamera.uphoto.ImageEditOperationUtil;
import com.ucamera.uphoto.gpuprocess.GpuProcess;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Locale;

public class GifEditActivity extends Activity implements OnTaskFinishListener{
    private final static String TAG = "GifEditActivity";
    private static final int PLAY_FAST = 200;
    private static final int PLAY_NORMAL = 300;
    private static final int PLAY_SLOW = 500;

    private HashMap<Integer, Integer> mSpeedMap = null;

    private static final int PLAY_MODE_ALL = 0;
    private static final int PLAY_MODE_CLOCKWISE = 1;
    private static final int PLAY_MODE_COUNTERCLOCKWISE = 2;

    public static final int ACTION_BASIC_EDIT_FUNCTION = 0;
    public static final int ACTION_EDIT_EFFECT = 1;
    public static final int ACTION_PHOTO_FRAME = 2;
    public static final int ACTION_TEXTURE = 3;
    public static final int ACTION_PLAY_SPEED = 4;
    public static final int ACTION_PLAY_MODE = 5;

    public static final String TAG_SAVE_GIF = "save_gif";
    public static final String TAG_SHARE_GIF = "share_gif";

    private boolean mIsFromOutSide = false;
    // the other application send the gif's url to edit
    private Uri mUrlID;
    // use mDealWithBitmaps in handler means is deal with bitmaps.
    private boolean mDealWithBitmaps = false;
    // mReady4Edit means the bitmaps is ready for edit
    private boolean mReady4Edit = false;
    // whether choose damage picture to compose gif.
    private boolean mChooseDamagePic = false;
    // set default playing speed as normal;
    private int mPlaySpeed = PLAY_NORMAL;
    private int mKeepSpeed = mPlaySpeed;
    // set default playing mode as clockwise;
    private int mPlayMode = PLAY_MODE_CLOCKWISE;
    private int mKeepMode = 0;
    private int mBitmapNums = -1;
    private int mTotalBmpsFromGallery = 0; // the number of pictures chosen from
                                           // gallery include damaged ones.
    private AnimatedGifEncoder mAnimatedGifEncoder;
    private EffectType mGifEditPostEffect;
    private RelativeLayout mEffectTypeLayout;
    private Gallery mEffectTypeGallery;

    private GifView mGifView;
    private ImageView mGifPlayIcon;
    private ImageButton mSaveBtn;
    private ImageButton mClearBtn;
    private GridView mTypeGridView;
    private GridView mCateGridView;
    private GifEditCateAdapter mCateAdapter;
    private ArrayList<Runnable> mCateRunnables = new ArrayList<Runnable>();
    private int mCounter = 0;
    private int[] mAllImageList;
    private MyGallery mMyGallery;
    private MyGalleryAdapter mMyGalleryAdapter;
    private int mCurrentCate = 0;
    private MyHandler mHandler = new MyHandler();
    private BaseAdapter mCurrentTypeAdapter = null;
    private boolean mComposeGIF = false;
    private boolean mSaveCurrentGif = false;
    private Uri mCurrentGifUri = null;
    private Context mContext = null;

    private View mBottomSubMenu;
    private View mMenuView;
    private View mTopSubMenu;
    private Animation mFadeInFromTop = null;
    private Animation mFadeInFromBottom = null;
    private SharedPreferences msharedPrf;
    private int mCateItemMaxSize;
    private ImageView mGifEditBgImageView;
    private OnScreenHint mStorageHint;

    private static boolean IS_ALIVE = false;

    private static GifEditActivity gifActivity = null;
    /*
     * BUG FIX: 4751
     * DATE: 2013-08-28
     */
    private EffectTypeResource mEffectResource = EffectTypeResource.getResInstance(EffectTypeResource.UGIF_EFFECT_RES);
    private EffectShowed effectShowed;// Show Effects int GridView to Select

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        gifActivity = this;

        /*SPRD:fix bug527736 Some Activity about UCamera lacks method of checkpermission@{ */
        if (!UCamUtill.checkPermissions(this)) {
            finish();
            return;
        }
        /* }@ */
        mContext = this;
        com.ucamera.uphoto.UiUtils.initialize(mContext);
        /* SPRD: fix bug465312 photoframe and texture did not work before edit @{*/
        ResourceInitializer.initializeResource(mContext);
        /* @}*/
        setContentView(R.layout.gif_mode_edit_image);

//        Drawable drawable = getResources().getDrawable(R.drawable.magiclens_color);
        ImageProcessJni.SetMultiBmpEffectWidth(48);
        initView();
        initSpeedMap();
        initAnimation();
        Intent intent = getIntent();
        // CID 109119 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        // String action = intent.getAction();
        Bundle bundle = intent.getExtras();
        // mIsFromOutSide is true means GIFEditActivity called by other
        // application or ImageGifGallery
        mIsFromOutSide = intent.getBooleanExtra("fromOutSide", true);
        LogUtils.debug(TAG, "onCreate(): mIsFromOutSide is " + mIsFromOutSide);
        if (mIsFromOutSide) {
            startedFromOutSide(bundle);
        } else {
            startedFromInner();// deal with the bitmaps which taken by camera.
        }
        initLikedEffect();
        Drawable drawable = getResources().getDrawable(R.drawable.edit_bottom_bar_bg);
        mCateItemMaxSize = drawable.getIntrinsicHeight();
        //SPRD:fix bug529680 trying to use a recycled bitmap android.graphics.Bitmape4
        IS_ALIVE = true;
    }

    private void startedFromInner() {
        // get bitmap list by BitMapArrayClass
        Utils.recycleBitmaps(GifEditDatas.getOriBitmaps());
        CameraApp app = (CameraApp) getApplication();
        /* SPRD: Fix bug 542505 NullPointerException @{ */
        if (app.getGifBitmapArray() == null || app.getGifBitmapCount() < 1) finish();
        /* @} */
        mBitmapNums = app.getGifBitmapCount();
        GifEditDatas.initBitmaps(app.getGifBitmapArray(), mBitmapNums);
        if (mBitmapNums > 0) {
            initBitmapData();
        }
        app.clearGifBitmap();
    }

    /*
     * this function does not use
     */
    private void loadfromActionSend() {
        String gifPathTemp = null;
        mUrlID = getIntent().getExtras().getParcelable(Intent.EXTRA_STREAM);
        if (mUrlID != null && mUrlID.getScheme().equals("content")) { // content path
            gifPathTemp = getPhysicalPathFromURI(mUrlID); // convert to file path
        } else if (mUrlID != null) { // file path
            gifPathTemp = mUrlID.toString();
        }
        final String gifPath = gifPathTemp;
        BackGroundWorkTask.processTask(this, getString(R.string.ugif_edit_text_waiting), new Runnable() {
            public void run() {
                mReady4Edit = decodeGif(gifPath);
            }
        }, new LoadListener());
    }

    class LoadListener implements OnTaskFinishListener {
        @Override
        public void afterTaskFinish() {
            if (mDealWithBitmaps) {
                if (mReady4Edit) {  //get bitmaps ok then to show
                    if (mChooseDamagePic) {
                        int numOfDamagePics = mTotalBmpsFromGallery - mBitmapNums;
                        if (numOfDamagePics == 1) {
                            Toast.makeText(GifEditActivity.this, R.string.ugif_edit_choose_error_image_one, Toast.LENGTH_SHORT).show();
                        }else if (numOfDamagePics > 1) {
                            String toastString = getString(R.string.ugif_edit_choose_error_image_many, numOfDamagePics);
                            Toast.makeText(GifEditActivity.this, toastString, Toast.LENGTH_SHORT).show();
                        }
                    }
                    if(mBitmapNums > 1){     // there are more than 2 pics we could compose gif.
                        initBitmapData();
                    } else {                 // otherwise finish current activity.
                        finish();
                    }
                }else {             //get bitmaps failed then toast and finsh.
                    Toast.makeText(mContext, R.string.ugif_edit_open_error_image, Toast.LENGTH_SHORT).show();
                    finish();
                }
                mReady4Edit = false;
                mDealWithBitmaps = false;
            }
        }
    }

    private void startedFromOutSide(Bundle bundle) {
        mDealWithBitmaps = true;
        final ArrayList<String> uriList = bundle.getStringArrayList("uriList");
        final int fitGifSize = bundle.getInt("fitGifSize");
        if (uriList != null && uriList.size() > 0) { // deal with the url list
                                                     // from ImageGifGallery.
            loadFitGifSize(uriList, fitGifSize);
        } else if (getIntent().getAction().equals(Intent.ACTION_SEND)) {
            // deal with the gif from other application.
            loadfromActionSend();
        }
    }

    /**
     * get the physical path from uri(eg:content://) by query DB.
     *
     * @param contentUri
     *            which needed to convert to physical path.
     * @eturn String the physical path.
     */
    public String getPhysicalPathFromURI(Uri contentUri) {
        String searchColumn = MediaStore.Images.Media.DATA;
        String filePath = null;
        String[] project = { searchColumn };
        Cursor cursor = getContentResolver().query(contentUri, project, null, null, null);
        if (cursor != null) {
            try {
                int column_index = cursor.getColumnIndexOrThrow(searchColumn);
                if (cursor.moveToFirst()) {
                    filePath = cursor.getString(column_index);
                }
            } finally {
                cursor.close();
            }
        }
        return filePath;
    }

    private void loadFitGifSize(final ArrayList<String> uriList, final int fitGifSize) {
        mChooseDamagePic = false;
        final int n = uriList.size();
        mReady4Edit = true;
        mBitmapNums = n;
        mTotalBmpsFromGallery = n;
        BackGroundWorkTask.processTask(this, getString(R.string.ugif_edit_text_waiting), new Runnable() {
            public void run() {
                Utils.recycleBitmaps(GifEditDatas.getOriBitmaps());
                mBitmapNums = GifEditDatas.initBitmaps(uriList, fitGifSize);
                if (mBitmapNums != n) {
                    mChooseDamagePic = true;
                }
            }
        }, new LoadListener());
    }

    /**
     * initial some parameters and prepare the bitmaps.
     */
    private void initBitmapData() {
        mCounter = mBitmapNums;
        mAllImageList = new int[mBitmapNums];

        for (int i = 0; i < mBitmapNums; i++) {
            mAllImageList[i] = i;
        }

        GifEditDatas.initEditBitmaps();
        mMyGalleryAdapter = new MyGalleryAdapter(this, null, R.layout.gif_mode_image_item,
                new String[] { "image_list" }, new int[] { R.id.image_list }, GifEditDatas.getResultBitmaps());
        mMyGallery.setAdapter(mMyGalleryAdapter);

        mMyGallery.setOnItemClickListener(mMyGalleryClickedListener);

        initGifView(GifEditDatas.getResultBitmaps());
        mGifView.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                mGifView.setValidIndex(getValidImageList());
                if (!mGifView.isPlaying()) {
                    mGifView.start();
                    mGifPlayIcon.setVisibility(View.INVISIBLE);
                } else {
                    mGifPlayIcon.setVisibility(View.VISIBLE);
                    mGifView.stop();
                }
            }
        });

        mMyGalleryAdapter.setDisabledBg(mAllImageList);

//        onPreviewChanged();
//        initTypeAdapter();
    }

    private void initGifView(Bitmap[] initBitmaps) {
        // CID 109115 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        // float distance = this.getResources().getDimension(R.dimen.gif_mode_edit_preview_width);
        // CID 109115 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        // RectF rc = new RectF(0, 0, distance, distance);

        mGifView.init(initBitmaps, false);
        mGifView.setValidIndex(getValidImageList());
    }

    private void onPreviewChanged(Bitmap[] previewBitmaps){
//        if(previewBitmaps[0] != null){
            if(mMyGalleryAdapter != null) {
                mMyGalleryAdapter.setDrawingState(true);
                mMyGalleryAdapter.updateBitmaps(previewBitmaps);
                mMyGalleryAdapter.setDisabledBg(mAllImageList);
            }
            initGifView(previewBitmaps);
            mGifView.invalidate();
//        }
    }

    private AdapterView.OnItemClickListener mMyGalleryClickedListener = new AdapterView.OnItemClickListener() {
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            FrameLayout linearLayout = (FrameLayout) view;
            if (mAllImageList[position] != -1) {
                if (mCounter <= 2) {
                    // SPRD: Fix bug 580271 Toast long time display
                    ToastUtil.showToast(mContext,getResources()
                            .getString(R.string.gif_compose_edit_tips),Toast.LENGTH_LONG);
                    return;
                } else {
                    mSaveBtn.setEnabled(true);
                    mAllImageList[position] = -1;
                    linearLayout.getChildAt(1).setVisibility(View.VISIBLE);
                    mCounter--;
                }
            } else {
                mSaveBtn.setEnabled(true);
                mCounter++;
                mAllImageList[position] = position;
                linearLayout.getChildAt(1).setVisibility(View.INVISIBLE);
            }
            mGifView.start();
            mGifView.setValidIndex(getValidImageList());
            mGifView.stop();
            mGifPlayIcon.setVisibility(View.VISIBLE);
        }
    };

    /**
     * decode gif to bitmaps
     *
     * @param stringPath
     *            the gif path
     * @return true means decode success or false means failed.
     */
    private boolean decodeGif(String stringPath) {
        /*
         * FIX BUG: 994 BUG CAUSE: in some case stringPath is null FIX COMMENT:
         * if stringPath is empty just ignore DATE: 2012-05-11
         */
        if (TextUtils.isEmpty(stringPath)) {
            return false;
        }
        DisplayMetrics dm = this.getResources().getDisplayMetrics();
        GifDecoder gifDecoder = new GifDecoder(dm.widthPixels, dm.heightPixels);
        gifDecoder.read(stringPath);
        int n = gifDecoder.getFrameCount();
        mBitmapNums = n;
        if (n <= 0) { // get frame error
            return false;
        }

        mPlaySpeed = gifDecoder.getDelay(1);
        mGifView.setSpeed(mPlaySpeed);

        Bitmap tmps[] = new Bitmap[n];
        for (int i = 0; i < n; i++) {
            tmps[i] = gifDecoder.getFrame(i);
        }
        gifDecoder.clear();

        Utils.recycleBitmaps(GifEditDatas.getOriBitmaps());
        Bitmap[] bitmaps = GifUtils.scaleBitmaps(tmps);
        GifEditDatas.initBitmaps(bitmaps, bitmaps.length);
        return GifEditDatas.getOriBitmaps() != null;
    }

    private void initView() {
        mGifEditBgImageView = (ImageView) findViewById(R.id.gif_edit_bg_view);
        mGifView = (GifView) findViewById(R.id.ugif_gifview_id);
        mGifPlayIcon = (ImageView) findViewById(R.id.gif_play_icon);
        mMyGallery = (MyGallery) findViewById(R.id.ugif_mygallery_id);
        mMyGallery.setGalleryCallback(new MyGalleryCallback());
        mSaveBtn = (ImageButton) findViewById(R.id.ugif_topbar_btn_save);
/*        if(!ShareUtils.SNS_SHARE_IS_ON) {
            findViewById(R.id.ugif_topbar_btn_share).setVisibility(View.INVISIBLE);
        }*/
        mClearBtn = (ImageButton) findViewById(R.id.ugif_topbar_btn_clear);
        mClearBtn.setEnabled(false);
        if( !UiUtils.highMemo() ) {
           mClearBtn.setVisibility(View.INVISIBLE);
        }
        mBottomSubMenu = findViewById(R.id.ugif_hs_type);
        mMenuView = findViewById(R.id.ugif_hs_cate);
        mTopSubMenu = findViewById(R.id.top_sub_menu);
        mTopSubMenu.setOnClickListener(null);
        mCateGridView = (GridView) findViewById(R.id.ugif_gv_category);
        mCateAdapter = new GifEditCateAdapter(this);
        setDisplayItemCountsInWindow(mCateGridView, mCateAdapter.getCount(), 5, true);
        mCateGridView.setAdapter(mCateAdapter);
        mCateGridView.setOnItemClickListener(mCateClickListener);
        mTypeGridView = (GridView) findViewById(R.id.ugif_gv_type);
        /*
         * FIX BUG: 4831
         * BUG COMMENT: set selector drawable for gridview
         * DATE: 2013-11-05
         */
        mCateGridView.setSelector(android.R.color.transparent);
        mTypeGridView.setSelector(android.R.color.transparent);
        mEffectTypeLayout = (RelativeLayout) findViewById(R.id.gif_effect_type_layout);
        mEffectTypeGallery = (Gallery) findViewById(R.id.gif_gallery_effect_type);
        mEffectTypeGallery.setCallbackDuringFling(false);
    }

    private void initAnimation() {
        mFadeInFromTop = createTranslateAnimation(this, 0, 0, -200, 0, 500);
        mFadeInFromBottom = createTranslateAnimation(this, 0, 0, 200, 0, 500);
    }

    private void initSpeedMap() {
        mSpeedMap = new HashMap<Integer, Integer>(3);
        mSpeedMap.put(PLAY_SLOW, 0);
        mSpeedMap.put(PLAY_NORMAL, 1);
        mSpeedMap.put(PLAY_FAST, 2);
    }

    private Animation createTranslateAnimation(Context context, float fromX, float toX,
            float fromY, float toY, long duration) {
        Animation anim = null;
        anim = new TranslateAnimation(fromX, toX, fromY, toY);
        anim.setDuration(duration);
        anim.setFillAfter(false);
        return anim;
    }

    private void showSubMenu() {
        if( !mBottomSubMenu.isShown() && !mTopSubMenu.isShown() ) {
            mBottomSubMenu.setVisibility(View.VISIBLE);
            mBottomSubMenu.startAnimation(mFadeInFromBottom);
            mTopSubMenu.setVisibility(View.VISIBLE);
            mTopSubMenu.startAnimation(mFadeInFromTop);
        }
    }

    private void dismissSubMenu() {
        mBottomSubMenu.setVisibility(View.GONE);
        mTopSubMenu.setVisibility(View.GONE);
        mEffectTypeLayout.setVisibility(View.GONE);
    }

    private OnItemClickListener mCateClickListener = new OnItemClickListener() {
        @Override
        public void onItemClick(android.widget.AdapterView<?> parent, View view, int position,
                long id) {
            /*SPRD:fix bug536240 this method is called after onDestroy*/
            if(!IS_ALIVE){
                LogUtils.debug(TAG,"Data has been recycled");
                finish();
                return;
            }
            /*@}*/
            mCurrentSelectPostion = -1;
            LogUtils.debug(TAG, "CateClickListener.onItemClick(): mCurrentCate is " + mCurrentCate + ", position is " + position);
//            if (mCurrentCate == position) {
//                return ;
//            }

            mCateAdapter.setSelected(position);
            /*
             * FIX BUG: 4723
             * BUG COMMENT: remove effect if the devices can not supported it,then get incorrect value of mCurrentCate by position
             * DATE: 2013-11-21
             */
            mCurrentCate = mCateAdapter.getCurrentCate(position);

            mEffectTypeLayout.setVisibility(View.GONE);
            initTypeAdapter();
        }
    };

    private void initLikedEffect() {
        if(msharedPrf == null){
            msharedPrf = this.getSharedPreferences(TAG, Context.MODE_PRIVATE);
        }
        mEffectResource.initLikedList(msharedPrf);
    }

    private void initTypeAdapter() {
        /*
         * FIX BUG: 5363
         * BUG COMMENT: null pointer exception
         * DATE: 2013-11-28
         */
        if(isFinishing()){
            return;
        }
        /*
         * FIX BUG: 5706
         * BUG COMMETN: The mSubHorizontalScrollView will scroll to the Correct position
         * DATE: 2014-01-02
         */
        if(mBottomSubMenu != null) {
            mBottomSubMenu.scrollTo(0, 0);
        }
        if(mCurrentCate == GifEditActivity.ACTION_EDIT_EFFECT) {
            if(mEffectResource.getLikedList() == null || mEffectResource.getLikedList().size() <= 0) {
                initLikedEffect();
            }
            mCurrentTypeAdapter = new EffectCateBaseAdapter(this, mEffectResource.getLikedList());
            ((EffectCateBaseAdapter)mCurrentTypeAdapter).setHighlight(-1);
        }else{
            mCurrentTypeAdapter = new GifEditTypeAdapter(this, mCurrentCate, createCallback());
            ((GifEditTypeAdapter)mCurrentTypeAdapter).setItemMaxSize(mCateItemMaxSize);
        }
        mTypeGridView.setAdapter(mCurrentTypeAdapter);
        /*
         * FIX BUG:5383
         * BUG CAUSE:The Ugif bottom bar have any inconsistent
         * DATE:3012.12.03
         */
        int height = mMenuView.getHeight();
        if(height > 0){
            RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) mBottomSubMenu
                    .getLayoutParams();
            params.height = height;
            mBottomSubMenu.setLayoutParams(params);
        }
        // if the current cate is effect, set subMenu padding is 1,because the effect item of UPhoto size is different from other items
        if(mCurrentCate == GifEditActivity.ACTION_EDIT_EFFECT) {
            int padding = -2;
            mBottomSubMenu.setPadding(padding, padding, padding, padding);
        }
        int countPerScreen = 5;
        /*if(mCurrentCate == ACTION_EDIT_EFFECT) {
            countPerScreen = 4;
        }*/
        if(mCurrentCate == GifEditActivity.ACTION_BASIC_EDIT_FUNCTION) {
            setDisplayItemCountsInWindow(mTypeGridView, mCurrentTypeAdapter.getCount(), countPerScreen, true);
        } else {
            setDisplayItemCountsInWindow(mTypeGridView, mCurrentTypeAdapter.getCount(), countPerScreen, false);
        }
        if(mCurrentCate != GifEditActivity.ACTION_EDIT_EFFECT) {
            mTypeGridView.setOnItemClickListener((GifEditTypeAdapter)mCurrentTypeAdapter);
        }else{
            /**
             *  BUG : 4859
             *  CAUSE: Tips are not positioned correctly.
             *  DATE : 2013-9-11
             */
            if (showEffectTipDlg(msharedPrf) && mScrnNumber > 1) {
               mBottomSubMenu.post(new Runnable() {
                    @Override
                    public void run() {
                        int x = (mScrnNumber - 1) * UiUtils.screenWidth();
                        mBottomSubMenu.scrollTo(x, mBottomSubMenu.getScrollY());
                    }
                });
            }
            if(UiUtils.highMemo()) {
                mClearBtn.setVisibility(View.VISIBLE);
            }
            mTypeGridView.setOnItemClickListener(mEffectItemClickListener);
        }

        if(mGifView.isPlaying()) {
            mGifView.stop();
            mGifPlayIcon.setVisibility(View.VISIBLE);
        }
        LogUtils.debug(TAG, "initTypeAdapter(): mPlaySpeed is " + mPlaySpeed + ", mKeepSpeed is " + mKeepSpeed);
        switch (mCurrentCate) {
            case ACTION_PLAY_SPEED:
                mKeepSpeed = mPlaySpeed;
                break;
            case ACTION_PLAY_MODE:
                mKeepMode = mPlayMode;
                break;
            default:
                GifEditDatas.initPreEditBitmap();
                //SPRD:fix bug527807 GifEditActivity stop when switch language
                if (GifEditDatas.getEditBitmaps() != null) {
                    mMyGalleryAdapter.updateBitmaps(GifEditDatas.getEditBitmaps());
                    mMyGalleryAdapter.notifyDataSetChanged();
                }
                break;
        }

        showSubMenu();
    }

    OnItemClickListener mEffectItemClickListener = new OnItemClickListener() {
        @Override
        public void onItemClick(android.widget.AdapterView<?> parent,
                View view, int position, long id) {
            handleEffect(position);
        }

    };
    private boolean showEffectTipDlg(SharedPreferences sp) {
        String key = "Effect_Liked_Tip_Dlg";
        boolean tipShowed = sp.getBoolean(key, false);
        if(tipShowed)
            return false;
        int width = UiUtils.screenWidth();
        int height = UiUtils.screenHeight();
        com.ucamera.uphoto.EffectShowedTipDlg.createEffectShowedDlg(this, 0, 0, width, height).show();
        Editor editor = sp.edit();
        editor.putBoolean(key, true);
        return editor.commit();
    }
    private void handleEffect(int position) {

        ((EffectCateBaseAdapter) mCurrentTypeAdapter).setHighlight(position);

        if(mCurrentCate == GifEditActivity.ACTION_EDIT_EFFECT && position != -1) {
            if(mEffectResource.getLikedList().size() == position+1){// The last item , open effect showed gridview
                if(effectShowed == null){
                    mEffectResource.initEffectShowedList(msharedPrf);

                    effectShowed = new EffectShowed(this, mHandler, EffectTypeResource.UGIF_EFFECT_RES);
                    effectShowed.showTipDlg(msharedPrf);
                }
                effectShowed.showEffects(mFadeInFromTop);
            }else{
                mCurrentSelectPostion = position;
                handlePostEffect(position);
            }
        }
    }

    private GpuProcess mGpuProcees = null;
    private byte[] effectSync = new byte[0];
    public void handlePostEffect(final int position) {
        final Context context = getApplicationContext();

        ImageEditOperationUtil.startBackgroundJob(this, "loading", getString(R.string.text_waiting),
                new Runnable() {
                    public void run() {

                        synchronized(effectSync) {
//                            int effectValue = mEffectResource.getEffectItem(mCurrentItemIndex).get(position).mTypeValue;
                            EffectItem currentItem = mEffectResource.getLikedList().get(position);
                            Bitmap[] editBitmap = GifEditDatas.getOriginBitmapByJpeg();
                            /*
                             * FIX BUG: 5605
                             * BUG CAUSE: editBitmap is null
                             * DATE: 2013-12-17
                             */
                            if(null == currentItem || null == editBitmap){
                                return;
                            }
                            Bitmap []textureRes=null;

                            int size = editBitmap.length;

                           if(EffectResInfo.isNeedResourceTexture(currentItem.mShaderName)){
                                int drawableId = 0;
                                boolean needRotation = false;
                                drawableId = EffectResInfo.getDrawableID(currentItem.mShaderName);
                                needRotation = EffectResInfo.isResourceDrawableRotated(drawableId);
                                textureRes = new Bitmap[1];
                                BitmapFactory.Options options = new BitmapFactory.Options();
                                options.inScaled = false;
                                options.inPurgeable = true;
                                options.inDensity = editBitmap[0].getDensity();
                                Bitmap bitmap = BitmapFactory.decodeResource(getResources(), drawableId, options);
                                if(needRotation) {
                                    if(bitmap.getWidth() > bitmap.getHeight()) {
                                        Matrix matrix = new Matrix();
                                        matrix.setRotate(90);
                                        Bitmap b2 = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
                                        if(b2 != bitmap) {
                                            Utils.recycleBitmap(bitmap);
                                        }
                                        bitmap = b2;
                                    }
                                }
                                textureRes[0] = bitmap;
                            }

                            mGpuProcees = new GpuProcess(context);
                            for(int j = 0; j < size; j++) {
                                mGpuProcees.setShaderType(currentItem.mShaderName);

                                Bitmap bitmap = Bitmap.createBitmap(editBitmap[j], 0, 0, editBitmap[j].getWidth(), editBitmap[j].getHeight());
                                // To do get the resource texture
                                mGpuProcees.process(editBitmap[j], bitmap, textureRes);

                                Bitmap temp = editBitmap[j];
                                editBitmap[j] = bitmap;
                                if(temp != null && !temp.isRecycled() && temp != bitmap) {
                                    temp.recycle();
                                }
                            }
                            mGpuProcees.finish();
                            if (textureRes != null) {
                                for (int i = 0; i < textureRes.length; i++) {
                                    if (textureRes[i] != null && !textureRes[i].isRecycled()) {
                                        textureRes[i].recycle();
                                    }
                                }
                            }
                            GifEditDatas.updateEditBitmaps(editBitmap);
                        }
                    }
                }, mHandler);
    }


    public void pauseAdapterAndGifView(){
        mGifView.stopDrawing();
        mMyGalleryAdapter.setDrawingState(false);
        mMyGalleryAdapter.notifyDataSetChanged();
    }

//    private void setDisplayItemCountsInWindow(GridView gridview, int totalCount, float countPerScreen) {
//        final int itemWidth = UiUtils.screenWidth() / countPerScreen;
//        int mode = totalCount % countPerScreen;
//        if (mode > 0) {
//            totalCount = totalCount + (countPerScreen - mode);
//        }
//        gridview.setNumColumns(totalCount);
//        final int layout_width = itemWidth * totalCount;
//        gridview.setLayoutParams(new LinearLayout.LayoutParams(layout_width,
//                LayoutParams.MATCH_PARENT));
//    }
    private void setDisplayItemCountsInWindow(GridView gridview, int totalCount,
            float countPerScreen, boolean isCategory) {
        mItemWidth = UiUtils.effectItemWidth();
        cumputeScrnNumber(totalCount , countPerScreen);
        gridview.setNumColumns(totalCount);
        final int layout_width = (int) (mItemWidth * totalCount);
        gridview.setLayoutParams(new LinearLayout.LayoutParams(layout_width,
                LayoutParams.MATCH_PARENT));
    }
    private void cumputeScrnNumber(int totalCount, float countPerScreen){
        mScrnNumber = (int)(totalCount / countPerScreen);
        if(totalCount % countPerScreen > 0) {
            mScrnNumber ++;
        }
    }
    public int[] getValidImageList() {
        final int mode = mPlayMode;
        int[] valid = null;
        if (mode == PLAY_MODE_CLOCKWISE) {
            valid = new int[mCounter];
            for (int i = 0, j = 0; i < mBitmapNums; i++) {
                if (mAllImageList[i] != -1) {
                    valid[j] = mAllImageList[i];
                    j++;
                }
            }
        } else if (mode == PLAY_MODE_ALL) {
            valid = new int[mCounter * 2];
            for (int i = 0, j = 0; i < mBitmapNums; i++) {
                if (mAllImageList[i] != -1) {
                    valid[j] = mAllImageList[i];
                    j++;
                }
            }
            for (int i = 0, j = 0; i < mBitmapNums; i++) {
                if (mAllImageList[mBitmapNums - 1 - i] != -1) {
                    valid[j + mCounter] = mAllImageList[mBitmapNums - 1 - i];
                    j++;
                }
            }
        } else if (mode == PLAY_MODE_COUNTERCLOCKWISE) {
            valid = new int[mCounter];
            for (int i = 0, j = 0; i < mBitmapNums; i++) {
                if (mAllImageList[mBitmapNums - 1 - i] != -1) {
                    valid[j] = mAllImageList[mBitmapNums - 1 - i];
                    j++;
                }
            }
        }
        return valid;
    }

    public void onClick2ClearEffect(View v) {
        BackGroundWorkTask.processTask(mContext, mContext.getString(R.string.ugif_edit_text_waiting), new Runnable() {
            public void run() {
                ArrayList<byte[]> orginList = GifEditDatas.getOriginJpegDataList();
                orginList.clear();
//                Bitmap[] bitmaps = GifEditDatas.getEditBitmaps();
                Bitmap[] originBitmaps = GifEditDatas.getOriBitmaps();
                Bitmap[] resultBitmaps = GifEditDatas.getResultBitmaps();
                byte[] tempBytes;
                for (int i = 0; i < resultBitmaps.length; i++) {
                    tempBytes = Utils.transformBitmapToBuffer(originBitmaps[i]);
                    orginList.add(tempBytes);
//                    if (bitmaps[i] != originBitmaps[i]) {
//                        bitmaps[i] = originBitmaps[i];
//                    }
                    if(resultBitmaps[i] != originBitmaps[i]) {
                        resultBitmaps[i] = originBitmaps[i].copy(Config.ARGB_8888, true);
                    }
                }
                tempBytes = null;
            }
        }, new ClearGifListener());
        clearAllStatus();
    }
    /**
     * FIX BUG:2858
     * BUG CAUSE:The icon is still hignlight after save or share
     * DATE:3012.02.21
     */
    private void clearAllStatus(){
        mClearBtn.setEnabled(false);
        if( mCurrentTypeAdapter != null ) {
            if(mCurrentCate != GifEditActivity.ACTION_EDIT_EFFECT) {
                ((GifEditTypeAdapter)mCurrentTypeAdapter).setHighlight(-1);
            }else {
                ((EffectCateBaseAdapter)mCurrentTypeAdapter).setHighlight(-1);
            }
        }
        if(mEffectTypeLayout != null && mEffectTypeLayout.isShown()){
            mEffectTypeLayout.setVisibility(View.GONE);
        }
    }

    private void cancelCurrentOperate() {
        dismissSubMenu();

        if(mCurrentCate == ACTION_PLAY_SPEED) {
            setPlaySpeed(mSpeedMap.get(mKeepSpeed));
            return;
        }

        if(mCurrentCate == ACTION_PLAY_MODE) {
            setPlayMode(mKeepMode);
            return;
        }

        afterOperationDone(false);
    }

    public void onClickMenuCancel(View v) {
        cancelCurrentOperate();
    }

    public void onClickMenuOk(View v) {
        //SPRD:fix bug530324 java.lang.IndexOutOfBoundsException
        if(!GifEditDatas.isDataReady){
            return;
        }
        dismissSubMenu();

        if(mCurrentCate == ACTION_PLAY_SPEED) {
            mKeepSpeed = mPlaySpeed;
            setTopBtnState();
            return;
        }

        if(mCurrentCate == ACTION_PLAY_MODE) {
            mKeepMode = mPlayMode;
            setTopBtnState();
            return;
        }

        GifEditDatas.pushBitmaps();

        afterOperationDone(true);
    }

    private void afterOperationDone(boolean enableClearBtn) {
        /*
         * FIX BUG: 4236
         * BUG CAUSE: Without the use of a result Bitmaps to update ui,
         *              it has been recycled in the editing Bitmaps,
         *              it will led to try to use a recycled bitmap;
         * FIX COMMENT: At first, to use the result Bitmaps update ui,
         *              and then recycle in the editing Bitmaps;
         * DATE: 2013-06-06
         */
        onPreviewChanged(GifEditDatas.getResultBitmaps());
        GifEditDatas.recycleEditBitmaps(false);
        if(enableClearBtn) {
            setTopBtnState();
        }
    }

    private void setTopBtnState() {
        if( !mSaveBtn.isEnabled() ) {
            mSaveBtn.setEnabled(true);
        }
        if( !mClearBtn.isEnabled() ) {
            mClearBtn.setEnabled(true);
        }
    }

    public void onClick2ShareGIF(View v) {
         saveBitmap2Gif(TAG_SHARE_GIF, false);
         clearAllStatus();
    }

    // make bitmap to gif
    public void onClickSave2GIF(View v) {
         saveBitmap2Gif(TAG_SAVE_GIF, false);
         clearAllStatus();
    }

    /**
     * by click Save or Share button to convert the bitmap into gif, if saveTag is "save", only save,
     * if the saveTag is share, the first save and then share
     *
     * @param saveTag identify the action
     */
    private void saveBitmap2Gif(final String saveTag, final boolean finish) {
        final Bitmap[] resultBitmaps = GifEditDatas.getResultBitmaps();
        if (resultBitmaps == null || resultBitmaps.length < 2) {
            Toast.makeText(this, getResources().getString(R.string.ugif_edit_compose_edit_tips),Toast.LENGTH_SHORT).show();
            return;
        }
        final int speed = mPlaySpeed;
        final long dateTaken = System.currentTimeMillis();
        Date date = new Date(dateTaken);
        SimpleDateFormat dateFormat = new SimpleDateFormat(getString(R.string.ugif_edit_file_name_format), Locale.US);
        final String title = dateFormat.format(date);
        final String filename = dateFormat.format(date) +".gif";
        StorageUtil storageUtil = StorageUtil.getInstance();
        final String directory = storageUtil.getFileDir();
        final String saveString = directory + "/" +filename;
        mComposeGIF = true;

        // bug 538819 directory is null when SDcard is removed
        if (directory == null) {
            String message = null;
            message = getString(R.string.no_storage);
            if (mStorageHint == null) {
                mStorageHint = OnScreenHint.makeText(GifEditActivity.this,
                        message);
            } else {
                mStorageHint.setText(message);
            }
            mStorageHint.show();
            return;
        } else {
            File dir = new File(directory);
            // fix bug 6083 check the path to save file
            /*
             * SPRD: CID 109353 : RV: Bad use of return value
             * (FB.RV_RETURN_VALUE_IGNORED_BAD_PRACTICE) @{
             */
            if (!dir.exists()) {
                if (!dir.mkdirs()) {
                    return;
                }
            }
            // if (!dir.exists()) dir.mkdirs();
            /* @} */
            final Context context = this;
            BackGroundWorkTask.processTask(this, getString(R.string.ugif_edit_text_waiting),
                    new Runnable() {
                        public void run() {
                            if (mSaveBtn.isEnabled() || (TAG_SHARE_GIF.equals(saveTag)
                                    && (false == mSaveCurrentGif || mCurrentGifUri == null))) {
                                mAnimatedGifEncoder = new AnimatedGifEncoder();
                                mAnimatedGifEncoder.start(saveString);
                                mAnimatedGifEncoder.setDelay(speed);
                                mAnimatedGifEncoder.setRepeat(0);
                                int[] list = getValidImageList();
                                int length = list.length;
                                for (int i = 0; i < length; i++) {
                                    if (resultBitmaps[list[i]] != null && !resultBitmaps[list[i]].isRecycled()){
                                        // fix bug 5734
                                        mAnimatedGifEncoder.addFrame(resultBitmaps[list[i]]);
                                    }
                                }
                                // SPRD : we should judge the result of write file
                                if (mAnimatedGifEncoder.finish()) {
                                    mSaveCurrentGif = insertDB(context.getContentResolver(), title,
                                            dateTaken, null, directory, filename);
                                } else {
                                    // when write file error, delete this temp file
                                    File f = new File(saveString);
                                    if (f.exists()) {
                                        f.delete();
                                    }
                                    Log.d(TAG, "create gif file failed, deleted temp file!");
                                    mSaveCurrentGif = false;
                                }

                                if (finish) {
                                    finish();
                                }
                            }
                        }
                    }, new SaveGifListener(saveTag));
            /*
             * FIX BUG: 6100
             * BUG COMMENT: remove last thumbnail of file dir after save image in
             * uphoto DATE: 2014-03-13
             */
            File file = new File(getFilesDir(), "last_image_thumb");
            if (file.exists() && file.isFile()) {
                file.delete();
            }
        }
    }

    class SaveGifListener implements OnTaskFinishListener {
        private String mSaveTag = null;
        public SaveGifListener(String saveTag) {
            mSaveTag = saveTag;
        }
        @Override
        public void afterTaskFinish() {
            if (mComposeGIF) {
                if(mSaveCurrentGif) {
                    try {
                        String imageId = mCurrentGifUri.toString().substring(mCurrentGifUri.toString().lastIndexOf("/")+1);
                        // TODO getThumbnail
//                        BitmapManager.instance().getThumbnail(getContentResolver(), Long.valueOf(imageId),
//                                 Images.Thumbnails.MINI_KIND, Utils.getNativeAllocOptions(), false);  // fix bug 6044 update thumbnail in DB.
                    } catch (Throwable ex) {
                        Log.e(TAG, "microThumbBitmap got exception", ex);
                    }
                    mSaveBtn.setEnabled(false);
                    if(TAG_SAVE_GIF.equals(mSaveTag)) {
                        Toast.makeText(mContext, getResources().getString(R.string.ugif_edit_composing_tips), Toast.LENGTH_SHORT).show();
                    } else if(TAG_SHARE_GIF.equals(mSaveTag)) {
                        shareImage(mCurrentGifUri);
                    }
                } else {
                    Toast.makeText(mContext, getResources().getString(R.string.ugif_edit_composing_failed_tips), Toast.LENGTH_SHORT).show();
                }
                mComposeGIF = false;
                // SPRD: Fix bug 580364, Can't call getPixels() on a recycled bitmap
                if (isDestroyed()) {
                    GifEditDatas.recyleAll();
                }

                if (TAG_SAVE_GIF.equals(mSaveTag)) {
                    finish();
                }
            }
        }
    }

    class ClearGifListener implements OnTaskFinishListener {

        @Override
        public void afterTaskFinish() {
            onPreviewChanged(GifEditDatas.getResultBitmaps());
        }

    }

    private void shareImage(Uri uri) {
//        ShareUtils.shareImage(this, uri);
    }

    private boolean insertDB(ContentResolver cr, String title, long dateTaken,
            Location location, String directory, String filename)
    {
        // Read back the compressed file size.
        long size = new File(directory, filename).length();
        String filePath = directory + "/" + filename;

        ContentValues values = new ContentValues(9);
        values.put(Images.Media.TITLE, title);

        // That filename is what will be handed to Gmail when a user shares a
        // photo. Gmail gets the name of the picture attachment from the
        // "DISPLAY_NAME" field.
        values.put(Images.Media.DISPLAY_NAME, filename);
        values.put(Images.Media.DATE_TAKEN, dateTaken);
        values.put(Images.Media.MIME_TYPE, "image/gif");
        values.put(Images.Media.DATA, filePath);
        values.put(Images.Media.SIZE, size);

        if (location != null) {
            values.put(Images.Media.LATITUDE, location.getLatitude());
            values.put(Images.Media.LONGITUDE, location.getLongitude());
        }
        try {
            if (size != 0) {
//                if(StorageUtils.isExternalStorage(directory)) {
                    mCurrentGifUri = cr.insert(Images.Media.EXTERNAL_CONTENT_URI, values);
//                }else {
//                    mCurrentGifUri = cr.insert(Images.Media.INTERNAL_CONTENT_URI, values);
//                }
            } else {
                new File(filePath).delete();
                return false;
            }
        } catch (Exception e) {
            mCurrentGifUri = null;
            Log.e("save gif error", ""+e.getMessage());
            return false;
        }
        return true;
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        //sprd:fix bug527807 GifEditActivity stop when switch language
        initView();
        super.onConfigurationChanged(newConfig);
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onRestart() {
        super.onRestart();
    }

    @Override
    protected void onResume() {
        super.onResume();
        //SPRD:fix bug529680/534188 trying to use a recycled bitmap android.graphics.Bitmape4
        if(!IS_ALIVE){
            finish();
            return;
        }
        if (mCurrentCate == ACTION_PHOTO_FRAME) { // photoframe
            DownloadCenter.loadPhotoFrame(this, new OnLoadCallback() {
                @Override
                public void onLoadComplete(String[] result) {
                    updateTypeAdapter(result);
                    /*
                     * FIX BUG: 5589
                     * BUG COMMETN: The mSubHorizontalScrollView will scroll to the Correct position
                     * DATE: 2013-12-13
                     */
                    com.ucamera.ucomm.downloadcenter.UiUtils.scrollToCurrentPosition((HorizontalScrollView)mBottomSubMenu, (int)(mItemWidth), mCurrentSelectPostion);
                }
            });
        } else if (mCurrentCate == ACTION_TEXTURE) {// texture
            DownloadCenter.loadTexture(this, new OnLoadCallback() {
                @Override
                public void onLoadComplete(String[] result) {
                    updateTypeAdapter(result);
                    /*
                     * FIX BUG: 5589
                     * BUG COMMETN: The mSubHorizontalScrollView will scroll to the Correct position
                     * DATE: 2013-12-13
                     */
                    com.ucamera.ucomm.downloadcenter.UiUtils.scrollToCurrentPosition((HorizontalScrollView)mBottomSubMenu, (int)(mItemWidth), mCurrentSelectPostion);
                }

            });
        }
    }

    private void updateTypeAdapter(String[] result) {
        setDisplayItemCountsInWindow(mTypeGridView, result.length, 5, false);
        if(mCurrentCate != GifEditActivity.ACTION_EDIT_EFFECT) {
            ((GifEditTypeAdapter)mCurrentTypeAdapter).updateContents(result);
        }
    }

    @Override
    protected void onPause() {
        LogUtils.debug(TAG, "onPause()");
        super.onPause();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        LogUtils.debug(TAG, "onDestroy(): ENTRY");
        //SPRD:fix bug529680 trying to use a recycled bitmap android.graphics.Bitmape4
        IS_ALIVE = false;
        // SPRD: Fix bug 580364, Can't call getPixels() on a recycled bitmap
        if (!mComposeGIF) {
            GifEditDatas.recyleAll();
        }
        //sprd:fix bug527807 GifEditActivity stop when switch language
        finish();
        super.onDestroy();
        LogUtils.debug(TAG, "onDestroy(): LEAVE");
        gifActivity = null;
    }


    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if(keyCode == KeyEvent.KEYCODE_BACK) {
            if(null != effectShowed && effectShowed.onKeyEvent(keyCode, event)){
                // close effect showed gridview
                return true;
            }
            if(mBottomSubMenu.isShown() || mTopSubMenu.isShown()) {
                cancelCurrentOperate();
                return true;
            }
            if (mSaveBtn.isEnabled()){
                new AlertDialog.Builder(this)
                        .setTitle(R.string.text_edit_exit_tip_title)
                        .setMessage(getString(R.string.text_edit_exit_tip_message,
                                getSavePath()))
                        .setPositiveButton(R.string.text_edit_exit_tip_save,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog,
                                            int which) {
                                        saveBitmap2Gif(TAG_SAVE_GIF, true);
                                        clearAllStatus();
                                        dialog.dismiss();
                                        /* FIX BUGF : 4306
                                         * BUG CAUSE : the bitmap has been recycled before compose gif file
                                         * BUG COMMENT : do not finish the current activity at once when the back button has be clicked
                                         * DATE :2013-06-21
                                         */
                                        //finish();
                                    }
                                })
                        .setNeutralButton(R.string.text_edit_exit_tip_exit,
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog,
                                            int which) {
                                        dialog.dismiss();
                                        finish();
                                    }
                                })
                        .setNegativeButton(R.string.text_edit_exit_tip_cancel,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog,
                                            int which) {
                                        dialog.dismiss();
                                    }
                                }).show();
                return true;
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    private String getSavePath() {
        Date date = new Date(System.currentTimeMillis());
        SimpleDateFormat dateFormat = new SimpleDateFormat(getString(R.string.ugif_edit_file_name_format), Locale.US);
        String filename = dateFormat.format(date) +".gif";
        StorageUtil storageUtil = StorageUtil.getInstance();
        String directory = storageUtil.getFileDir();
        return directory + "/" +filename;
    }
    // set gif delay time.
    // fast is 500,normal is 1000, slow is 2000.
    private void setPlaySpeed(int speed){
        switch(speed) {
            case 0: mPlaySpeed = PLAY_SLOW;   break;
            case 1: mPlaySpeed = PLAY_NORMAL; break;
            case 2: mPlaySpeed = PLAY_FAST;   break;
            default: mPlaySpeed = PLAY_NORMAL;
        }
        mGifView.setSpeed(mPlaySpeed);
    }

    private void setPlayMode(int mode){
        mPlayMode = mode;
        if(mGifView.isPlaying()){
            mGifView.stop();
            mGifView.setValidIndex(getValidImageList());
            mGifView.start();
        }
    }

    private ProcessCallback createCallback() {
        switch (mCurrentCate) {
        case ACTION_BASIC_EDIT_FUNCTION:
            return new BasicEditCallback();
        case ACTION_EDIT_EFFECT:
            return new EffectCateCallback(this);
        case ACTION_PHOTO_FRAME:
            return new PhotoFrameCallback(this);
        case ACTION_TEXTURE:
            return new TextureCallback(this);
        case ACTION_PLAY_SPEED:
            return new PlaySpeedCallback();
        case ACTION_PLAY_MODE:
            return new PlayModeCallback();
        default:
            break;
        }

        return null;
    }
    class BasicEditCallback implements ProcessCallback {
        public BasicEditCallback() {
            if(UiUtils.highMemo()) {
                mClearBtn.setVisibility(View.VISIBLE);
            }
        }
        @Override
        public void beforeProcess() {
            pauseAdapterAndGifView();
        }

        @Override
        public void afterProcess(int cur) {
//            mSaveBtn.setEnabled(true);
//            mClearBtn.setEnabled(true);
        }

        @Override
        public void updateAapter(String[] result) {
        }
    }

    class EffectCateCallback implements ProcessCallback {
        Context mContext = null;
        public EffectCateCallback(Context context) {
            mContext = context;
            getOriginDataList(context);
            if(UiUtils.highMemo()) {
                mClearBtn.setVisibility(View.VISIBLE);
            }
        }
        @Override
        public void beforeProcess() {
        }

        @Override
        public void afterProcess(int cur) {
            mGifEditPostEffect = new EffectType(mContext, null, cur);
            mEffectTypeGallery.setAdapter(mGifEditPostEffect);
            mEffectTypeGallery.setLayoutParams(new RelativeLayout.LayoutParams(UiUtils.screenWidth(),
                    LayoutParams.WRAP_CONTENT));
            if (mGifEditPostEffect.updatePostEffectNames(cur)) {
                mEffectTypeLayout.setVisibility(View.VISIBLE);
                mGifEditPostEffect.setHighlight(cur);
                mEffectTypeGallery.setOnItemSelectedListener(mGifEditPostEffect);
                mEffectTypeGallery.setSelection(2);
            }
//            mSaveBtn.setEnabled(true);
//            mClearBtn.setEnabled(true);
        }
        @Override
        public void updateAapter(String[] result) {
        }
    }

    private void getOriginDataList(Context context) {
        ArrayList<byte[]> originList = GifEditDatas.getOriginJpegDataList();
//        Bitmap[] editBitmaps = GifEditDatas.getEditBitmaps();
        Bitmap[] editBitmaps = GifEditDatas.getResultBitmaps();
        if(originList!=null){
            originList.clear();
        }else{
            return;
        }
        byte[] tempBytes;
        for (int i = 0; i < mBitmapNums; i++) {
            if (editBitmaps == null
                    || editBitmaps[i] == null
                    || editBitmaps[i].isRecycled()) {
                Toast.makeText(context, getResources().getString(R.string.ugif_edit_operation_memory_low_warn),
                        Toast.LENGTH_SHORT).show();
                mHandler.sendEmptyMessage(NO_MEMORY_TO_CLOSE_EDIT_ACTIVITY);
                break;
            }
            tempBytes = Utils.transformBitmapToBuffer(editBitmaps[i]);
            originList.add(i, tempBytes);
        }
        tempBytes = null;
    }

    class PhotoFrameCallback implements ProcessCallback {
        public PhotoFrameCallback(Context context) {
            getOriginDataList(context);
            if(UiUtils.highMemo()) {
                mClearBtn.setVisibility(View.VISIBLE);
            }
        }
        @Override
        public void beforeProcess() {
        }

        @Override
        public void afterProcess(int cur) {
//            mSaveBtn.setEnabled(true);
//            mClearBtn.setEnabled(true);
            mCurrentSelectPostion = cur;
        }
        @Override
        public void updateAapter(String[] result) {
            updateTypeAdapter(result);
        }
    }

    class TextureCallback implements ProcessCallback {
        public TextureCallback(Context context) {
            getOriginDataList(context);
            if(UiUtils.highMemo()) {
                mClearBtn.setVisibility(View.VISIBLE);
            }
        }
        @Override
        public void beforeProcess() {
        }

        @Override
        public void afterProcess(int cur) {
//            mSaveBtn.setEnabled(true);
//            mClearBtn.setEnabled(true);
            mCurrentSelectPostion = cur;
        }
        @Override
        public void updateAapter(String[] result) {
            updateTypeAdapter(result);
        }
    }

    class PlaySpeedCallback implements ProcessCallback {
        public PlaySpeedCallback() {
            if (UiUtils.highMemo()) {
                mClearBtn.setVisibility(View.VISIBLE);
            }
        }
        @Override
        public void beforeProcess() {
        }

        @Override
        public void afterProcess(int cur) {
            setPlaySpeed(cur);
//            mSaveBtn.setEnabled(true);
//            mClearBtn.setEnabled(true);
        }
        @Override
        public void updateAapter(String[] result) {
        }
    }

    class PlayModeCallback implements ProcessCallback {
        public PlayModeCallback() {
            if (UiUtils.highMemo()) {
                mClearBtn.setVisibility(View.VISIBLE);
            }
        }
        @Override
        public void beforeProcess() {
        }

        @Override
        public void afterProcess(int cur) {
            setPlayMode(cur);
            if(mCurrentCate != GifEditActivity.ACTION_EDIT_EFFECT) {
                ((GifEditTypeAdapter)mCurrentTypeAdapter).setHighlight(cur);
            }
//            mSaveBtn.setEnabled(true);
//            mClearBtn.setEnabled(true);
        }
        @Override
        public void updateAapter(String[] result) {

        }
    }

    class MyGalleryCallback implements GalleryCallback {
        @Override
        public void gifChangePosition() {
            mSaveBtn.setEnabled(true);
        }

        @Override
        public void itemLongClick() {
            if(mGifView.isPlaying()) {
                mGifView.stop();
                mGifPlayIcon.setVisibility(View.VISIBLE);
            }
        }
    }

    private final int ACTION_PREVIEW = 0;
    private final int NO_MEMORY_TO_CLOSE_EDIT_ACTIVITY = 1;
    public static final int CHANGE_LIKED_EFFECT = -888;
    public static final int EXCHANGE_LIKED_EFFECT = -889;
    public static final int EFFECT_SELECTED_BACK = -890;
    private int mCurrentSelectPostion = -1;
    private int mScrnNumber;
    private float mItemWidth;

    class MyHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
            case NO_MEMORY_TO_CLOSE_EDIT_ACTIVITY:
                finish();
                break;
            case ACTION_PREVIEW:
                onPreviewChanged(GifEditDatas.getEditBitmaps());
                break;
            case CHANGE_LIKED_EFFECT:
                if(mEffectResource.getLikedList().size() <= mCurrentSelectPostion + 1) {
                    mCurrentSelectPostion = 0;
                    mBottomSubMenu.scrollTo(0, mBottomSubMenu.getScrollY());
                }
                mCurrentTypeAdapter = new EffectCateBaseAdapter(mContext, mEffectResource.getLikedList());
                setDisplayItemCountsInWindow(mTypeGridView, mCurrentTypeAdapter.getCount(), 4, false);
                mTypeGridView.setAdapter(mCurrentTypeAdapter);
                ((EffectCateBaseAdapter)mCurrentTypeAdapter).setHighlight(mCurrentSelectPostion);
                handleEffect(mCurrentSelectPostion);
                break;
            case EFFECT_SELECTED_BACK:
                /*
                 * FIX BUG: 5589
                 * BUG COMMETN: The mSubHorizontalScrollView will scroll to the Correct position
                 * DATE: 2013-12-13
                 */
                com.ucamera.ucomm.downloadcenter.UiUtils.scrollToCurrentPosition((HorizontalScrollView)mBottomSubMenu, (int)(mItemWidth), mCurrentSelectPostion);
                ((EffectCateBaseAdapter)mCurrentTypeAdapter).setHighlight(mCurrentSelectPostion);
                break;
            case EXCHANGE_LIKED_EFFECT:
               mBottomSubMenu.scrollTo(mCurrentSelectPostion, mBottomSubMenu.getScrollY());
               ((EffectCateBaseAdapter)mCurrentTypeAdapter).setHighlight(mCurrentSelectPostion);
               handleEffect(mCurrentSelectPostion);
               break;
            }

            super.handleMessage(msg);
        }
    }

    @Override
    public void afterTaskFinish() {
        onPreviewChanged(GifEditDatas.getEditBitmaps());
    }

    public static Activity getInstance(){
        return gifActivity;
    }
}
