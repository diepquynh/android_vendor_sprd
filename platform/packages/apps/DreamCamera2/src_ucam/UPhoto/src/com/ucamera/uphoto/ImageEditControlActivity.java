/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.app.Activity;
import android.app.ActivityGroup;
import android.app.AlertDialog;
import android.app.LocalActivityManager;
import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ContentUris;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.ActivityInfo;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.media.ExifInterface;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Handler.Callback;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.os.Parcelable;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.provider.MediaStore.Images.Media;
import android.text.Editable;
import android.text.InputFilter;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.Pair;
import android.util.SparseArray;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewStub;
import android.view.Window;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.TranslateAnimation;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.Gallery;
import android.widget.GridView;
import android.widget.HorizontalScrollView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.Toast;

import com.android.camera.stats.profiler.Profile;
import com.android.camera.stats.profiler.Profilers;
import com.ucamera.ucam.ResourceInitializer;
import com.ucamera.ucam.jni.ImageProcessJni;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.ucamera.ucomm.downloadcenter.DownloadCenter;
import com.ucamera.ucomm.downloadcenter.DownloadCenter.OnLoadCallback;
import com.ucamera.uphoto.EffectTypeResource.EffectItem;
import com.ucamera.uphoto.ImageEditViewPreview.ScrollController;
import com.ucamera.uphoto.R;
import com.ucamera.uphoto.brush.BaseBrush;
import com.ucamera.uphoto.brush.BrushColorPickerView;
import com.ucamera.uphoto.brush.BrushColorPickerView.OnColorChangedListener;
import com.ucamera.uphoto.brush.BrushConstant;
import com.ucamera.uphoto.brush.BrushItemInfo;
import com.ucamera.uphoto.brush.BrushListAdapter;
import com.ucamera.uphoto.brush.BrushPainting;
import com.ucamera.uphoto.brush.ImageBrushManager;
import com.ucamera.uphoto.brush.RandomColorPicker;
import com.ucamera.uphoto.gpuprocess.GpuProcess;
import com.ucamera.uphoto.integration.Build;
import com.ucamera.uphoto.mosaic.MosaicConstant;
import com.ucamera.uphoto.util.Models;

import org.xml.sax.SAXException;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Method;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Random;
import java.util.Stack;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import dalvik.system.VMRuntime;

public class ImageEditControlActivity extends ActivityGroup implements View.OnClickListener,
        OnItemClickListener, OnItemLongClickListener, BrushPainting.PaintChangedListener {
    private static final String TAG = "ImageEditControlActivity";
    public static final int EDIT_MSG = 11;
    public static final int EDIT_MSG_INTERNAL = 101;
    private static final int ACTION_BASIC_EDIT_FUNCTION = 0;
    private static final int ACTION_EDIT_EFFECT = 1;
    private static final int ACTION_DECORATION = 2;
    private static final int ACTION_GRAFFITI = 3;
    private static final int ACTION_TEXT_BUBBLE = 4;
    private static final int ACTION_PHOTO_FRAME = 5;
    private static final int ACTION_TEXTURE = 6;
    private static final int ACTION_EDIT_TAB_LABEL = 7;
    private static final int ACTION_EDIT_RESOURCE = 8;
    private static final int ACTION_EDIT_MAKEUP = 9;
    private static final int ACTION_EDIT_TEXT = 10;
    private static final int ACTION_EDIT_TONE = 11;
    public static final int ACTION_ID_PHOTO = 20;
    private static final int ACTION_MOSAIC =12;

    private static final String KEY_FIRST_LAUNCH = "key_first_launch_uphoto";

    private GridView mSubMenuGridView;
    private GridView mMosaicGridView;
    private HorizontalScrollView mSubHorizontalScrollView;
    private HorizontalScrollView mFunctionHorizontalScrollView,mSubMenuScrollView, mMosaicScrollView;
    private ImageView unDoBtn, reDoBtn, saveBtn, shareBtn;
    private ImageView mEditUndoBtn, mEditRedoBtn;
    private LinearLayout mEditUndoRedoLayout;
    private ImageView mOperateOkBtn, mOperateCancelBtn;
    private RelativeLayout mEditMakeUpLayout;
    private View mLayoutUndo, mLayoutRedo, mLayoutSave, mLayoutShare,mLayoutBack, mLayoutOk;
    private ImageView mEditBtn, mEffectBtn, mPhotoFrameBtn, mTextBtn, mGraffitiBtn, mMakeupBtn, mColorBtn,
    mBalloonBtn, mLabelBtn, mDecorationBtn, mTextureBtn, mBackgroundBtn, mMosaicBtn, mIDTypeBtn;
    private RadioGroup btnRadioGroup;
    private AdjustSclControl mAdjustSclControl = null;
    private boolean mAdjustProcess = false;
    private FrameLayout centerLayout;
    private byte[] mEffectOriginJpegData = null;
    private Bitmap mEffectOriginBitmap = null;
    private int[] mArgbOriginData = null;
    private int mBitmapHeight = 0;
    private int mBitmapWidth = 0;
    private String[] mDecorImage = null;
    private String[] mPhotoFrameImage = null;
    private String[] mTextureImage = null;
    private String[] mMosaicImage = null;
    private int mCurrentArgbSelect = -1;
    private int mCurrentItemIndex = -1;
    private Button mMakeupAdjustBtn;
    private ImageButton mMosaicDrawBtn;
    private ImageButton mMosaicEraseBtn;
    private SeekBar mMosaicSeekBar;
    private ImageButton mMosaicGridBtn;
    private ImageView mMosaicMarkView;
    private RelativeLayout mMosaicRel;
    private View mMosaicLineView;
    private GpuProcess mGpuProcees = null;
    private SparseArray<Runnable> actionArrays = new SparseArray<Runnable>();
    private SparseArray<BaseAdapter> adapterArray = new SparseArray<BaseAdapter>();
    private int mCurrentFuncMode = ACTION_EDIT_EFFECT;
    private int preActionCode = ACTION_EDIT_EFFECT;
    private HashMap<String, String> methodNameMap = new HashMap<String, String>();
    private String[] methodNames = {"handleCropAction", "handleRotateRightAction", "handleFlipHorizontalAction",
                                    "handleFlipVerticalAction", "handleSymmetryHorizontalLeftAction",
                                    "handleSymmetryHorizontalRightAction", "handleSymmetryVerticalUpAction"
                                    ,"handleSymmetryVerticalDownAction"};
    public static final int CROP_INDEX = 0;
    public int mCurrentBasicEditIndex = -1;

    private String[] effectMethodNames = {
            "handleLomo",
            "handleHDR",
            "handleSkin",
            "handleVividLight",
            "handleSketch",
            "handleColorFull",
            "handleFunny",
            "handleNostalgia",
            "handleBlackWhite",
            "handleDeform"};
    private String[] mMakeupMethodNames = {ImageEditConstants.MAKEUP_METHOD_SOFTENFACE, ImageEditConstants.MAKEUP_METHOD_WHITEFACE,
            ImageEditConstants.MAKEUP_METHOD_DEBLEMISH, ImageEditConstants.MAKEUP_METHOD_TRIMFACE, ImageEditConstants.MAKEUP_METHOD_BIGEYE};
    private String[] mIDPhotoMakeupMethodNames = {ImageEditConstants.MAKEUP_METHOD_SOFTENFACE, ImageEditConstants.MAKEUP_METHOD_WHITEFACE,
            ImageEditConstants.MAKEUP_METHOD_DEBLEMISH, ImageEditConstants.MAKEUP_METHOD_BIGEYE};

    private LocalActivityManager manager;
    private ImageEditDesc mImageEditDesc;
    private BroadcastReceiver mReceiver;  //main used to received the callback intent of subActivity
    private AlertDialog mAlertDialog = null;
    private boolean mExitAndSaved = false;
    private boolean mIsFromInner = false;
    private AssetManager mAssetManager;
    private float mScreenDensity;
    private boolean mIsOpenPhoto = false;
    private String mCaptureFilePath = null;
    private String tempPath = ImageManager.getCameraImageBucketName();
    private boolean mIsFromLaunch = false;
    private View mLoginView = null;
    //response code about camera
    private static final int REQUEST_CODE_CAMERA = 0xABCDB;
    private static final int REQUEST_CODE_MAKEUP_CAMERA = 0xABCDC;
    private static final int REQUEST_CODE_MAKEUP_PICK = 0xABCDD;
    private static final int REQUEST_CODE_PICK = 0xABCDE;
    private boolean mRefreshDecor = false;
    private boolean mRefreshFrame = false;
    private boolean mRefreshTexture = false;
    private boolean mRefreshMosaic =false;
    private MakeupFaceView mMakeupFaceView;
    private ImageView mTopSettingOpHintViewOut;
    private View mTopSettingBar;

    private GestureDetector mGestureDetector;

    private ViewAttributes mCurrentAttr;
//    private TextView mTextTitle;
    private EditText mEditText;
    private Button mBtnSaveAs;
    private TitleView mTitleView;
    private LabelView mLabelView;
    private TextView mEditLabelHistoryTextView;
    private Gallery mBalloonGallery;
    private Gallery mLabelHistoryGallery;
    private GridView mColorGridView;
    private GridView mFontGridView;
    private String[] mFontArray;
    private ArrayList<ViewAttributes> mAttrList;
//    private ArrayList<ViewAttributes> mLabelPreviewAttrList;
    private MyLabelBaseAdapter mColorAdapter;
    private MyBalloonStyleAdapter mBalloonStyleAdapter;
    private BaseAdapter mLabelHistoryAdapter;
    private SeekBar mBalloonSizeSeekBar;
    private int mCurrentSelectedBalloonStyle = -1;
    private int[] mBalloonStyles;
    private LinearLayout mLabelLayout;
    private LinearLayout mOperateDoneLayout;
    private ImageView mLabelOk;
    private TitleSaveAsDatabase mTitleSaveAsDatabase;
    private boolean mInsertTitleIntoDB = false;
    private String mViewTag = ImageEditConstants.LABEL_TAG_TITLE;
    private boolean isLongClickStatus;
    private View mBalloonSizeLayout;
    private float mCurrentBalloonTextSize = 28f;
    //current selected label shape, e.g: title is -1, label is -2, balloon is from zero to four(total is five)
    private int mLabelShape = 0;
    //if modify the label from long press, this variable is true, else false;
    private boolean mFromLabelModify = false;
    private MyTextBubbleImageView mBubbleImageView = null;
    private int mCurrentLabelIndex = -1;
//    private View mEffectRootView;
//    private GridView mGridViewEffectPost;
//    private Gallery mEffectPostGallery;
//    private EffectTypeBaseAdapter mEffectTypeAdapter;
    private EffectShowed effectShowed;// Show Effects int GridView to Select
    private int effectScrnNumber;
    private int perScrnCount;
    private EffectTypeResource mEffectResource = EffectTypeResource.getResInstance(EffectTypeResource.UPHOTO_EFFECT_RES);
    private int mCurrentFunModePost = -1;
//    private int mEffectThumbnailsWidth;
    private FunModeImageProcess mFunModeImageProcess;
    private int mScreenWidth;
    private int mScreenHeight;

    private MyGraffitiViewBaseAdapter mGraffitiAdapter = null;
    private MyGraffitiViewBaseAdapter mGraffitiHistoryAdapter;
    private LinearLayout mBrushRootLayout;
    private RelativeLayout mEraserLayout;
    private SeekBar mBrushSizeSeekbar;
    private SeekBar mEraserSizeSeekBar;
    private Gallery mBrushGallery;
    private Gallery mBrushHistoryGallery;
    private ImageButton mBtnBrushOK;
    private Button mBtnBrushSaveAs;
    private BrushColorPickerView mBrushColorPickerView;
    private ImageBrushManager mImageBurshManager;
    private ArrayList<Object> mBrushArrayList;
    private BrushListAdapter mBrushListAdapter;
    private BrushPainting mBrushPainting;
    private int mSelectColor;
    private int mBrushStyle;
    private BaseBrush mBrush;
    private float mBrushSize;
    private int mBrushColor;
    private int[] mBrushStyles;
    private int mBrushMode = BrushConstant.BrushModeSelected;
    private RandomColorPicker mRandomColorPicker;
    private boolean mInsertBrushIntoDB = false;
    private boolean mIsFromCamera = false;
    private String mLanguage;
//    private LinearLayout mMenuBottomLayout;
    private boolean mShowCanvas = false;
    private boolean mSaveByGraffiti = false;
    private int mCustomBrushHeight;
//    private View mEventRebakeEmptyView;
//    private View mEventBallomView;
    //Only identify into the UPhoto way, if true, click on the share, you need to save a picture and then share.
    private boolean mFromGraffiti = false;
    private boolean mCancelAnimation = false;
    // for stat usage
    // DO NOT USE OTHERWISE
    //------------->
    private int mStatCurrentLabelIndex = 0;
    private int mStatCurrentTitleIndex = 0;
    private int mStatCurrentColorIndex = 0;
    private int mStatCurrentBalloonIndex = 0;
    private int mStatBrushIndex = 0;
    //<-------------

    //UPhoto home
    private View mEditorContainer;
    private View mMakeupContainer;
    private View mEditorEntryView;
    private View mMakeupEntryView;
    private ViewStub mTipStubView;
    private ViewStub mDeblemishstub;
    private View mTipStubViewHint;
    private View mDeblemishView;
    private boolean mSaving = false;
    private int mEntryModule;
    private String mPhotoPick;
    public static final String EXTRA_ENTRY_MODULE = "entry_uphoto_module";
    private static final String EXTRA_PICKER_IMAGE = "extra_pick";

    private Animation mFadeInFromTop = null;
    private Animation mFadeInFromBottom = null;
    private Animation mToneFadeInFromBottom = null;

    private SharedPreferences msharedPrf;
    private int mSubMenuSelectedIndex = -1;
    private ImageEditViewMosaic mMosaicEditView;

    private ProgressDialog mProgressDialog;
    private int[] mRadioItemsOfUPhoto = {
            R.id.layout_edit_editor,
            R.id.layout_edit_text,
            R.id.layout_edit_photoframe,
            R.id.tab_effect_layout,
            R.id.layout_edit_graffiti,
            R.id.layout_edit_balloon,
            R.id.layout_edit_label,
            R.id.layout_edit_decoration,
            R.id.layout_edit_texture,
            R.id.layout_edit_mosaic
    };

    private int[] mRadioItemOfIDPhoto = {
            R.id.layout_edit_background
//            R.id.layout_edit_photo_type
    };

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        /*SPRD:fix bug514045 Some Activity about UCamera lacks method of checkpermission@{ */
        if (!UCamUtill.checkPermissions(this)) {
            finish();
            return;
        }
        /* }@ */

        // SPRD: For bug 538005 add log
        Profile guard = Profilers.instance().guard("ImageEditControlActivity onCreate()");

        UiUtils.initialize(this);
        // SPRD: For bug 538005 add log
        guard.mark("UiUtils initialize");
        ResourceInitializer.initializeResource(this);
        // SPRD: For bug 538005 add log
        guard.mark("ResourceInitializer initializeResource");
        Window win = getWindow();
        /*
         *BUG FIX: 5428
         *FIX COMMENT: don't set "FLAG_NEEDS_MENU_KEY" to window later than android 4.4,otherwise it will cause some UI issues
         *DATE: 2013-11-26
         */
        // add menu key support for the devices later than GINGERBREAD_MR1 version
        if (android.os.Build.VERSION.SDK_INT > android.os.Build.VERSION_CODES.GINGERBREAD_MR1 &&
                android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            win.addFlags(0x08000000);
        }

//        mGpuProcees = new GpuProcess(getApplicationContext());
        /**
         * FIX BUG: 952
         * BUG CAUSE: not check sdcard state
         * FIX COMMENT: check sdcard state,return if sdcard not exist
         * DATE: 2012-04-27
         */
        if(!Utils.checkSDStatus(this)) {
            finish();
            return;
        }
        // SPRD: For bug 538005 add log
        guard.mark();
        mProgressDialog = ProgressDialog.show(this,"",getResources().getString(R.string.text_waiting));
        this.setContentView(R.layout.edit_image);

        // SPRD: For bug 538005 add log
        guard.mark("setContentView");
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(ImageEditControlActivity.this);
        mIsFromCamera =  getIntent().getBooleanExtra("outCall", false);
        mEntryModule = getIntent().getIntExtra(EXTRA_ENTRY_MODULE, ACTION_EDIT_EFFECT);
        Log.d(TAG, "mEntryModule.." + mEntryModule);;
        mPhotoPick = getIntent().getStringExtra(EXTRA_PICKER_IMAGE);
        mLanguage = Locale.getDefault().getLanguage();

        // update the bucket info according to Sharedpreference
        ImageManager.updateBucketInfo(this);

        /* FIX BUG: 5921 6271
         * BUG CAUSE : some effect will be invalid if start this activity directly from gallery
         * FIX COMMENT : initialize  resource used by special effect;
         *            copy image resource before all possible "return"
         */
        BitmapFactory.Options initEffectOptions = new BitmapFactory.Options();
        initEffectOptions.inJustDecodeBounds = true;
        BitmapFactory.decodeResource(this.getResources(), R.drawable.edit_topbar_bg, initEffectOptions);
        int viewTitleHeight = initEffectOptions.outHeight;

        BitmapFactory.decodeResource(this.getResources(), R.drawable.edit_brush_auto, initEffectOptions);
        mCustomBrushHeight = initEffectOptions.outHeight;
        mScreenWidth = UiUtils.screenWidth();
        mScreenHeight = UiUtils.screenHeight();
        mScreenDensity = UiUtils.screenDensity(); //400x240: 0.75; 320x240: 0.75; 480x320: 1.0; 800x480: 1.5; 854x480: 1.5
        Log.d(TAG, "ImageEditControlActivity.onCreate(): mScreenWidth = " + mScreenWidth + ", mScreenHeight = " + mScreenHeight
                + ", viewTitleHeight is " + viewTitleHeight + ", mCustomBrushHeight is "  + mCustomBrushHeight + ", mScreenDensity is "
                + mScreenDensity);

//        if(Compatible.isHoneyCombo()) {
//            mScreenDensity = 1.5f;
//        }

        mFadeInFromTop = createTranslateAnimation(this, 0, 0, -1, 0, 500);
        mFadeInFromBottom = createTranslateAnimation(this, 0, 0, 1, 0, 500);
        mToneFadeInFromBottom = createTranslateAnimation(this, 0, 0, 1, 0, 500);
        mImageEditDesc = ImageEditDesc.getCountInstance();
        manager = getLocalActivityManager();
        mAssetManager = this.getAssets();
        findWidget();
        initWidgetOnClickListener();
        // SPRD: For bug 538005 add log
        guard.mark();
        // SPRD: fix bug 551209 Camera ANR when initData()
        // initData();
        new Thread(new MyRunnable()).start();
        // SPRD: For bug 538005 add log
        guard.stop("initData");
    }

    /* SPRD: fix bug 551209 Camera ANR when initData() @{ */
    public class MyRunnable implements Runnable {
        public static final int SUCCESS = 0;
        public static final int WARN_MESSAGE_FOR_OPEN_PHOTO = 1;
        public static final int FINISH = 2;
        @Override
        public void run() {
            initData();
        }
    }

    private Handler mUiHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (isDestroyed()) {
                Log.e(TAG, "handleMessage: activity is destroyed");
                return;
            }
            if (mProgressDialog != null) {
                mProgressDialog.dismiss();
                mProgressDialog = null;
            }
            switch (msg.what) {
                case MyRunnable.SUCCESS:
                    Log.d(TAG,"init data success");
                    initUI();
                    break;
                case MyRunnable.WARN_MESSAGE_FOR_OPEN_PHOTO:
                    Log.d(TAG,"on warn_message_for_open_photo");
                    warnMessageForOpenPhoto();
                    mIsFromLaunch = true;
                    break;
                case MyRunnable.FINISH:
                    onFinish();
                    break;
                default:
                    break;
            }
        }
    };
    /* @} */

    private void initUI(){
        // SPRD: fix bug 551209 Camera ANR when initData()
        mAdjustSclControl = new AdjustSclControl(this);
        mAdjustSclControl.initControl(null, this, mHandler, mImageEditDesc);

        mImageEditDesc.setOperationHandler(mHandler);
        getBasicEffectAdapter();
        mSubMenuGridView.setOnItemClickListener(this);
        mSubMenuGridView.setOnItemLongClickListener(this);
        /*
         * FIX BUG: 4831
         * BUG COMMENT: set selector drawable for gridview
         * DATE: 2013-11-05
         */
        mSubMenuGridView.setSelector(android.R.color.transparent);
        dismissSubMenuGridView();
        //if the mIsOpenPhoto is true, the means that entry the advanced edit screen by shortcut,
        //and no photo to show.
        if (!mIsOpenPhoto) {
            //except shortcut, other ways will judge the bitmap,
            if (mImageEditDesc.getBitmap() != null) {
                onPreviewChanged();
                //mEffectOriginJpegData = ImageEditOperationUtil.transformBitmapToBuffer(mImageEditDesc.getBitmap());
                getEffectOriginBmp();
            } else {
                if(TextUtils.isEmpty(mPhotoPick)) {
                    ImageEditOperationUtil.showToast(this, R.string.edit_open_error_image, Toast.LENGTH_SHORT);
                    finish();
                }
            }
        }
        mMakeupAdjustBtn = (Button)findViewById(R.id.makeup_adjust);
        mMakeupAdjustBtn.setOnClickListener(this);
//        findViewById(R.id.imageview_setting_op_hint).setOnClickListener(this);
        mTopSettingOpHintViewOut = (ImageView)findViewById(R.id.imageview_setting_op_hint_out);
        mTopSettingOpHintViewOut.setOnClickListener(this);
        mTopSettingBar = findViewById(R.id.edit_layout_top);
        mGestureDetector = new GestureDetector(this, new MyGestureListener());
        mTipStubView = (ViewStub)findViewById(R.id.makeup_facedetect_hint_stub);
        mDeblemishstub = (ViewStub)findViewById(R.id.makeup_deblemish_hint_stub);
        VMRuntime.getRuntime().setTargetHeapUtilization((float)0.75);

        loadDecor();
        loadPhotoFrame();
        loadTexture();
        loadMosaic();
        if (Build.MAKEUP_ON) {
            MakeupEngine.Init_Lib();
        }
    }

    private void getEffectOriginBmp() {
        if (mEffectOriginBitmap != null && !mEffectOriginBitmap.isRecycled()) {
            mEffectOriginBitmap.recycle();
        }
        if (mImageEditDesc.getBitmap() == null) {
            return ;
        }
//        mEffectOriginBitmap = Bitmap.createBitmap(mImageEditDesc.getBitmap(), 0, 0, mImageEditDesc.getBitmap().getWidth(), mImageEditDesc.getBitmap().getHeight());
        Bitmap bmp = mImageEditDesc.getBitmap();
        do {
            try {
                mEffectOriginBitmap = Bitmap.createBitmap(bmp.getWidth(), bmp.getHeight(), Bitmap.Config.ARGB_8888);
            } catch(OutOfMemoryError oom) {
                LogUtils.error(TAG, "getEffectOriginBmp : OOM");
            }
            if(mEffectOriginBitmap == null) {
                if(mImageEditDesc.reorganizeQueue() < 2) {
                    return;
                }
            }
        } while (mEffectOriginBitmap == null);
        Canvas canvas = new Canvas(mEffectOriginBitmap);
        canvas.drawBitmap(bmp, 0, 0, null);
    }

    private void initData() {
        mIsFromLaunch = false;
        Intent intent = getIntent();
        mIsFromInner = intent.getBooleanExtra(ImageEditConstants.EXTRA_FROM_INNER, false);
        int degree = 0;
        Uri target = null;
        String action = intent.getAction();
        Log.d(TAG, "initData(): action = " + action + ", mIsFromInner = " + mIsFromInner);
        if(mIsFromInner || (action != null && action.equals(Intent.ACTION_EDIT))) {
            //mIsFromInner is true, means that the intent is from UCam;
            //action is Intent.ACTION_EDIT, means that the intent is from Gallery3D
            //Uri is intent.getData();
            target = intent.getData();
            Log.d(TAG, "initData(): out, target = " + target);
            if(target == null) {
                //for moto manufacturer gallery
                target = intent.getExtras().getParcelable(Intent.EXTRA_STREAM);
                Log.d(TAG, "initData(): in, target = " + target);
            }
        } else if(mIsFromInner == false && (action != null && action.equals(Intent.ACTION_SEND))){
            //mIsFromInner is false, means that the intent is from other applications
            //action is Intent_ACTION_SEND, means that the intent is from other application
            target = intent.getExtras().getParcelable(Intent.EXTRA_STREAM);
            /** FIX BUG: 6552
             * BUG CAUSE: do not get the degree
             * FIX COMMENT: get the degree by ExifInterface
             * Date: 2011-12-05
             */
            degree = getImageDegreeByUri(target);
        } else if(mIsFromInner == false && (action != null && action.equals("android.intent.action.UGALLERY_EDIT"))
                && (!TextUtils.isEmpty(mPhotoPick) && mPhotoPick.equals("uphoto_pick"))) {
            /* SPRD:  @{ */
            target = intent.getData();
            // target = target = intent.getData();
            /* @} */
            degree = getImageDegreeByUri(target);
        } else if(mIsFromInner == false) {
            mIsOpenPhoto = true;
            // SPRD: fix bug 551209 Camera ANR when initData()
            mUiHandler.sendMessage(mUiHandler.obtainMessage(MyRunnable.WARN_MESSAGE_FOR_OPEN_PHOTO));
            return;
        }

        if(target == null) {
            mUiHandler.sendMessage(mUiHandler.obtainMessage(MyRunnable.FINISH));
            return;
        }
        String targetScheme = target.getScheme();
        Bitmap bitmap = null;
        Bitmap tempBitmap = null;
        Log.d(TAG, "initData(): target = " + target + ", targetScheme = " + targetScheme + ", mIsFromInner = " + mIsFromInner);
        String photoPath = null;
        if(targetScheme != null) {
            /** FIX BUG: 6305
              * BUG CAUSE: do not get the degree
              * FIX COMMENT: reorganize to get the degree by ExifInterface
              * Date: 2011-11-30
              */
            if ("content".equals(targetScheme)) {
                photoPath = intent.getStringExtra("ImageFileName");
                if(photoPath == null) {
                    photoPath = getDefaultPathAccordUri(target);
                //} else {
                    //degree = intent.getIntExtra("PictureDegree", 0);
                }
                degree = calculateAngle(photoPath);
                Log.i(TAG,"initData() degree = "+ degree);
            } else if("file".equals(targetScheme)) {
                photoPath = target.getPath();
                degree = calculateAngle(photoPath);
                //fixed the bug3 for UCam
                int imageId = findImageIdByPath(photoPath);
                Log.d(TAG, "initData(): imageId = " + imageId + ", photoPath = " + photoPath);
                //fixed the bug6222, if can query photo id by photoPath, it will reset the target value.
                if(imageId > -1) {
                    target = ContentUris.withAppendedId(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, imageId);
                }
            }
            try {
                ParcelFileDescriptor pfd = this.getContentResolver().openFileDescriptor(target, "r");
                FileDescriptor fd = pfd.getFileDescriptor();
                BitmapFactory.Options options = Utils.getNativeAllocOptions();
                options.inSampleSize = 1;
                options.inJustDecodeBounds = true;
                BitmapFactory.decodeFileDescriptor(fd, null, options);
                if(IsDopod) {
                    if (options.outWidth > options.outHeight) {
                        if (getRequestedOrientation() != ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE) {
                            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
                        }
                    }
                }
                int[] targetDimen = getTargetDimen(options);
                int inSampleSize = 1;

                 /*
                  * BUG FIX: 3652 3436
                  * BUG CAUSE: the msm8x30 can not scale bitmap if the width ratio not equals to height
                  * FIX COMMENT: get Max sample size of bitmap and not need to scale it again
                  * DATE: 2013-4-26
                  */
                if(android.os.Build.MODEL.equals("msm8x30")){
                    inSampleSize = ImageEditOperationUtil.computeBitmapSampleSize(options, targetDimen[0], targetDimen[1]);
                }else{
                    inSampleSize = ImageEditOperationUtil.computeSampleSize(options, targetDimen[0], targetDimen[1]);
                }
                Log.d(TAG, "initData(): inSampleSize = " + inSampleSize);
                options.inJustDecodeBounds = false;
                if(inSampleSize > 1){
                    options.inSampleSize = inSampleSize;
                    tempBitmap = BitmapFactory.decodeFileDescriptor(fd, null, options);
                    Log.d(TAG, "initData(): options.outWidth:" + options.outWidth + ",  options.outHeight: " + options.outHeight);
                }else{
                    tempBitmap = BitmapFactory.decodeFileDescriptor(fd, null, options);
                }

                tempBitmap = ImageEditOperationUtil.computeSuitableBitmap(tempBitmap, options, targetDimen[0], targetDimen[1]);
            } catch (Exception e) {
                e.printStackTrace();
                // SPRD: fix bug 551209 Camera ANR when initData()
                mUiHandler.sendMessage(mUiHandler.obtainMessage(MyRunnable.FINISH));
                return;
            }
        }
        /**
         * FIX BUG: bug64
         * BUG CAUSE: In the onCreate/onStart/onResume method executes finish,
         *            and will not immediately destroy Activity, still need to execute the three methods.
         * FIX COMMENT: add to judge null pointer instead finish.
         * Date: 2011-12-01
         */
        /* SPRD: fix bug 551209 Camera ANR when initData() @{ */
        ImageEditDesc imageEditDesc = ImageEditDesc.getCountInstance();
        imageEditDesc.setRotation(degree);
        imageEditDesc.setOriginalUri(target);
        imageEditDesc.setImageEditedPath(photoPath);
        /* @} */
        Log.d(TAG, "initData(): degree = " + degree + ", photoPath = " + photoPath + ", tempBitmap = " + tempBitmap);
        if(tempBitmap != null) {
            if(degree != 0){ // fix the bug28597, 28598, 29212
                bitmap = ImageEditOperationUtil.rotate(tempBitmap, degree);
                if(bitmap != null && bitmap != tempBitmap){
                    tempBitmap.recycle();
                    tempBitmap = null;
                }
            }else{
                bitmap = tempBitmap;
            }
        }

        if (bitmap == null) {
            // SPRD: fix bug 551209 Camera ANR when initData()
            mUiHandler.sendMessage(mUiHandler.obtainMessage(MyRunnable.FINISH));
            return;
        }
        /*
         * BUG FIX: 4289
         * FIX COMMENT: Makeup need ARGB_8888
         * DATE: 2013-06-14
         */
        if (bitmap.getConfig() != Bitmap.Config.ARGB_8888) {
            Bitmap tmp = bitmap.copy(Config.ARGB_8888, true);
            Utils.recyleBitmap(bitmap);
            bitmap = tmp;
        }
        imageEditDesc.setBitmap(bitmap);
        /* SPRD: fix bug 551209 Camera ANR when initData() @{ */
        imageEditDesc.uninitilizeCount();
        mUiHandler.sendMessage(mUiHandler.obtainMessage(MyRunnable.SUCCESS));
        /* @} */
    }

    private void onFinish() {
        ImageEditOperationUtil.showToast(this, R.string.edit_open_error_image, Toast.LENGTH_SHORT);
        finish();
    }

    /**
     * get image degree according uri target
     * @param target: image uri
     * @return Image degree
     */
    private int getImageDegreeByUri(Uri target) {
        if (target == null) {
            return 0;
        }
        String filepath = getDefaultPathAccordUri(target);
        if (filepath != null) {
            return ImageManager.getExifOrientation(filepath);
        }
        else {
            Log.d(TAG, "The path in uri: " + target + " is null");
            return 0;
        }
    }

    private int findImageIdByPath(String imagePath) {
        int imageId = -1;
        Cursor cursor = this.getContentResolver().query(MediaStore.Images.Media.EXTERNAL_CONTENT_URI,
                new String[]{MediaStore.Images.Media._ID},
                MediaStore.Images.Media.DATA + "= '" + imagePath + "'", null, null);
        if (cursor != null && cursor.getCount() > 0) {
            cursor.moveToFirst();
            imageId = cursor.getInt(0);
        /* SPRD: CID 109003 : Resource leak (RESOURCE_LEAK) @{ */
        }

        if (cursor != null) {
            cursor.close();
        }
        //     cursor.close();
        // }
        /* @} */

        return imageId;
    }

    private void warnMessageForOpenPhoto() {
        if(mIsOpenPhoto) {
            mLoginView.setVisibility(View.VISIBLE);
            /** FIX BUG: 6819
             * BUG CAUSE: When exit editor mode, no reset to default tab
             * FIX COMMENT: when exit editor mode, reset to default tab
             * Date: 2011-12-26
             */
            if(btnRadioGroup.getCheckedRadioButtonId() != R.id.btn_edit_effect) {
                btnRadioGroup.check(R.id.btn_edit_effect);
                final Runnable runnable = actionArrays.get(R.id.btn_edit_effect);
                if(runnable != null){
                    runnable.run();
                }
            }
        } else {
            mLoginView.setVisibility(View.GONE);
        }
        //saveBtn.setEnabled(!mIsOpenPhoto);
        shareBtn.setEnabled(!mIsOpenPhoto);
        mLayoutShare.setEnabled(!mIsOpenPhoto);
        mEditBtn.setEnabled(!mIsOpenPhoto);
        mEffectBtn.setEnabled(!mIsOpenPhoto);
        mGraffitiBtn.setEnabled(!mIsOpenPhoto);

        mLabelBtn.setEnabled(!mIsOpenPhoto);
        mMakeupBtn.setEnabled(!mIsOpenPhoto);
        //mSubMenuGridView.setEnabled(!mIsOpenPhoto);
        btnRadioGroup.setEnabled(!mIsOpenPhoto);
        mSubHorizontalScrollView.setEnabled(!mIsOpenPhoto);
//        mEffectRootView.setVisibility(View.GONE);
        mEraserLayout.setVisibility(View.GONE);
    }

    protected void onRestart() {
        super.onRestart();
        Log.d(TAG, "entry onRestart, and mRefreshDecor is " + mRefreshDecor
                + " mRefreshFrame is " + mRefreshFrame +
                " mRefreshTexture is " + mRefreshTexture);
        if(mRefreshDecor) {
            loadDecor();
        }
        if (mRefreshFrame) {
            loadPhotoFrame();
        }
        if (mRefreshTexture) {
            loadTexture();
        }
        if (mRefreshMosaic) {
            loadMosaic();
        }
    }

    @Override
    protected void onStart() {
        Log.d(TAG, "onStart(): entry...");
        registerReceiver();

        mEditText.addTextChangedListener(mTextWatcher);

        super.onStart();
    }

    private void registerReceiver() {
        mReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action=intent.getAction();
                Log.d(TAG, "onReceive(): action: " + action);
                if(action.equals(ImageEditConstants.ACTION_PREVIEW_RECEIVERD)){
                    onPreviewChanged();
                }else if(action.equals(ImageEditConstants.ACTION_GRAFFITI_CAN_BE_SAVE_RECEIVERD)){
                    if(mCurrentFuncMode == ACTION_GRAFFITI) {
                        int queueCount = mImageEditDesc.getqueueSize();
                        int curBpIndex = mImageEditDesc.getCurrentBitmapIndex();

                        int strokeCount = mBrushPainting.getStrokeCount();
                        int stackCount = mBrushPainting.getStackCount();
                        Log.d(TAG, "ImageEditControlActivity.onReceive(): queueCount = " + queueCount + ", curBpIndex = " + curBpIndex
                                + ", strokeCount = " + strokeCount + ", stackCount = " + stackCount);
                        if((strokeCount > 0 && stackCount > 0) || (strokeCount == 0 && stackCount != 0)) {
                            mBrushPainting.clearUndoStrokers();
                            reDoBtn.setEnabled(false);
                            mLayoutRedo.setEnabled(false);
                        }
                        if(curBpIndex >= 0 && curBpIndex < queueCount - 1) {
                            mImageEditDesc.clearReDoQueue();
                        }
                    } else {
                        mImageEditDesc.clearReDoQueue();
                    }
                } else if(action.equals(ImageEditConstants.ACTION_GRAFFITI_CAN_BE_SAVE_CANCEL_RECEIVERD)){
                    if(mCurrentFuncMode == ACTION_GRAFFITI) {
                        int queueCount = mImageEditDesc.getqueueSize();
                        int curBpIndex = mImageEditDesc.getCurrentBitmapIndex();
                        Log.d(TAG, "ImageEditControlActivity.onReceive(): queueCount = " + queueCount + ", curBpIndex = " + curBpIndex);
                        if(curBpIndex > 0 && curBpIndex < queueCount - 1) {
                            unDoBtn.setEnabled(true);
                            mLayoutUndo.setEnabled(true);
                            reDoBtn.setEnabled(true);
                            mLayoutRedo.setEnabled(true);
                        } else if(curBpIndex == 0 && queueCount > 1) {
                            unDoBtn.setEnabled(false);
                            mLayoutUndo.setEnabled(false);
                            reDoBtn.setEnabled(true);
                            mLayoutRedo.setEnabled(true);
                        } else if(queueCount > 1 && curBpIndex == queueCount - 1) {
                            unDoBtn.setEnabled(true);
                            mLayoutUndo.setEnabled(true);
                            reDoBtn.setEnabled(false);
                            mLayoutRedo.setEnabled(false);
                        } else {
                            unDoBtn.setEnabled(false);
                            mLayoutUndo.setEnabled(false);
                            reDoBtn.setEnabled(false);
                            mLayoutRedo.setEnabled(false);
                        }
                    } else {
                        unDoBtn.setEnabled(false);
                        mLayoutUndo.setEnabled(false);
                    }
                }
//                else if(action.equals(ImageEditConstants.ACTION_DECORVIEW_DELETED_RECEIVERD)){
//                    int index = intent.getIntExtra("index", 0);
//                    View view = centerLayout.getChildAt(index);
//                    if(view instanceof ImageEditViewDecorView){
//                        centerLayout.removeViewAt(index);
//                        view = null;
//                    }
//                }
            }
        };
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(ImageEditConstants.ACTION_PREVIEW_RECEIVERD);
        intentFilter.addAction(ImageEditConstants.ACTION_DECORVIEW_DELETED_RECEIVERD);
        intentFilter.addAction(ImageEditConstants.ACTION_GRAFFITI_CAN_BE_SAVE_RECEIVERD);
        intentFilter.addAction(ImageEditConstants.ACTION_GRAFFITI_CAN_BE_SAVE_CANCEL_RECEIVERD);
        registerReceiver(mReceiver, intentFilter);
    }

    private void findWidget() {
        saveBtn = (ImageView) findViewById(R.id.btn_edit_storage);
        unDoBtn = (ImageView) findViewById(R.id.btn_edit_back);
        reDoBtn = (ImageView) findViewById(R.id.btn_edit_next);
        shareBtn = (ImageView) findViewById(R.id.btn_edit_share);
        mLayoutUndo = findViewById(R.id.layout_edit_undo);
        mLayoutRedo = findViewById(R.id.layout_edit_redo);
        mLayoutShare = findViewById(R.id.layout_edit_share);
        mLayoutSave = findViewById(R.id.layout_edit_storage);
        mEditUndoBtn = (ImageView) findViewById(R.id.btn_edit_undo);
        mEditUndoBtn.setEnabled(false);
        mEditUndoBtn.setOnClickListener(this);
        mEditRedoBtn = (ImageView) findViewById(R.id.btn_edit_redo);
        mEditRedoBtn.setEnabled(false);
        mEditRedoBtn.setOnClickListener(this);
        mEditUndoRedoLayout = (LinearLayout) findViewById(R.id.edit_undo_redo_layout);
        mEditUndoRedoLayout.setOnClickListener(null);

        // initialize the bottom btn
        mEditMakeUpLayout = (RelativeLayout ) findViewById(R.id.edit_makeup_layout);

        mEditBtn = (ImageButton) findViewById(R.id.btn_edit_editor);
        /*
         * BUG FIX: 4664
         * FIX DATE: 2013-08-07
         * */
        findViewById(R.id.tv_edit_editor).setSelected(true);
        mEffectBtn = (ImageButton) findViewById(R.id.btn_edit_effect);
        findViewById(R.id.tv_edit_effect).setSelected(true);
        mPhotoFrameBtn = (ImageButton) findViewById(R.id.btn_edit_photoframe);
        findViewById(R.id.tv_edit_photoframe).setSelected(true);
        mTextBtn = (ImageButton) findViewById(R.id.btn_edit_text);
        findViewById(R.id.tv_edit_text).setSelected(true);
        mGraffitiBtn = (ImageView) findViewById(R.id.btn_edit_graffiti);
        findViewById(R.id.tv_edit_graffiti).setSelected(true);
        mMakeupBtn = (ImageView) findViewById(R.id.btn_edit_makeup);
        findViewById(R.id.tv_edit_makeup).setSelected(true);
        mColorBtn = (ImageView) findViewById(R.id.btn_edit_color);
        findViewById(R.id.tv_edit_color).setSelected(true);
        mBalloonBtn = (ImageView) findViewById(R.id.btn_edit_balloon);
        findViewById(R.id.tv_edit_balloon).setSelected(true);
        mLabelBtn = (ImageView) findViewById(R.id.btn_edit_label);
        findViewById(R.id.tv_edit_label).setSelected(true);
        mDecorationBtn = (ImageView) findViewById(R.id.btn_edit_decoration);
        findViewById(R.id.tv_edit_decoration).setSelected(true);
        mTextureBtn = (ImageView) findViewById(R.id.btn_edit_texture);
        findViewById(R.id.tv_edit_texture).setSelected(true);
        mBackgroundBtn = (ImageView) findViewById(R.id.btn_edit_background);
        findViewById(R.id.tv_edit_background).setSelected(true);
        mMosaicBtn = (ImageView) findViewById(R.id.btn_edit_mosaic);
        findViewById(R.id.tv_edit_mosaic).setSelected(true);

        mOperateOkBtn = (ImageView) findViewById(R.id.operate_ok);
        mOperateCancelBtn = (ImageView) findViewById(R.id.operate_cancel);
        mRectView = (RectView) findViewById(R.id.makeup_rectview);
        btnRadioGroup = (RadioGroup) findViewById(R.id.btn_radiogroup);
        mSubMenuGridView = (GridView) this.findViewById(R.id.sub_gallery);
        mMosaicGridView = (GridView) findViewById(R.id.mosaic_sub_gallery);
        mMosaicGridView.setOnItemClickListener(new MosaicOnItemListener());

//        mEffectPostGallery = (Gallery) this.findViewById(R.id.gallery_effect_post);
//        mEffectPostGallery.setLayoutParams(new RelativeLayout.LayoutParams(mScreenWidth, RelativeLayout.LayoutParams.WRAP_CONTENT));
//        mEffectPostGallery.setCallbackDuringFling(false);
//        mEffectRootView = this.findViewById(R.id.effect_root_view);
        mSubHorizontalScrollView = (HorizontalScrollView) this.findViewById(R.id.scroller);
        mFunctionHorizontalScrollView = (HorizontalScrollView) this.findViewById(R.id.function_scroller);
        mSubMenuScrollView = (HorizontalScrollView) this.findViewById(R.id.scroller);
        mMosaicScrollView = (HorizontalScrollView) findViewById(R.id.mosaic_scroller);
        Drawable drawable = getResources().getDrawable(R.drawable.edit_bottom_bar_bg);
        int sEffectItemHeight = drawable.getIntrinsicHeight();
        if (sEffectItemHeight > 0) {
            LinearLayout.LayoutParams params = (LinearLayout.LayoutParams) mFunctionHorizontalScrollView
                    .getLayoutParams();
            LinearLayout.LayoutParams scrollViewParams = (LinearLayout.LayoutParams) mSubMenuScrollView
                    .getLayoutParams();
            params.height = sEffectItemHeight;
            scrollViewParams.height = sEffectItemHeight ;
            mFunctionHorizontalScrollView.setLayoutParams(params);
            mSubMenuScrollView.setLayoutParams(scrollViewParams);
        }
        centerLayout = (FrameLayout) findViewById(R.id.center_layout);
        mLoginView = findViewById(R.id.layout_box);

        mEditText = (EditText) this.findViewById(R.id.edit_preview_input);
        //mBtnSaveAs = (Button) this.findViewById(R.id.edit_preview_save_as);
        mTitleView = (TitleView) this.findViewById(R.id.edit_preview_title);
        mLabelView = (LabelView) this.findViewById(R.id.edit_preview_label);
        mEditLabelHistoryTextView = (TextView) findViewById(R.id.edit_label_history_text);
        mBalloonGallery = (Gallery) this.findViewById(R.id.edit_gallery_balloon);
        mLabelHistoryGallery = (Gallery) this.findViewById(R.id.edit_gallery_history);
        mLabelHistoryGallery.setOnItemClickListener(new LabelHistoryOnClickListener());
        mLabelHistoryGallery.setOnItemLongClickListener(new LabelHistoryOnLongClickListener());
        mColorGridView = (GridView) this.findViewById(R.id.edit_color_gridview);
        mFontGridView = (GridView) this.findViewById(R.id.edit_font_gridview);
        /*
         * FIX BUG: 4831
         * BUG COMMENT: set selector drawable for gridview
         * DATE: 2013-11-05
         */
        mColorGridView.setSelector(android.R.color.transparent);
        mFontGridView.setSelector(android.R.color.transparent);
        mLabelLayout = (LinearLayout) this.findViewById(R.id.edit_layout_label_view);
        mLayoutBack = (LinearLayout) this.findViewById(R.id.layout_view_back);
        mLayoutOk = (LinearLayout) this.findViewById(R.id.layout_title_ok);
        mLabelOk =(ImageView)this.findViewById(R.id.edit_title_ok);//SPRD:Fix bug 463501
        //mBalloonSizeLayout = findViewById(R.id.edit_balloon_size_layout);
        mBalloonSizeSeekBar = (SeekBar) findViewById(R.id.edit_balloon_size_seekbar);
        mBalloonSizeSeekBar.setOnSeekBarChangeListener(new BalloonSizeOnSeekBarChangeListener());

        mBrushRootLayout = (LinearLayout) this.findViewById(R.id.edit_layout_brush_view);
        mEraserLayout = (RelativeLayout) this.findViewById(R.id.layout_brush_eraser_size);
        mBrushSizeSeekbar = (SeekBar) this.findViewById(R.id.seekbar_brush_size);
        mEraserSizeSeekBar = (SeekBar) this.findViewById(R.id.seekbar_brush_eraser_size);
        mBrushGallery = (Gallery) this.findViewById(R.id.brush_gallery);
        mBrushHistoryGallery = (Gallery) this.findViewById(R.id.brush_history_gallery);
        //SPRD:fix bug535435 some icons show not friendly
        mBtnBrushOK = (ImageButton) this.findViewById(R.id.edit_brush_title_ok);
        Button mBtnBack;
        //mBtnBrushSaveAs = (Button) this.findViewById(R.id.edit_brush_save_as);
        mBrushColorPickerView = (BrushColorPickerView)findViewById(R.id.brush_colorpicker);

        mOperateDoneLayout = (LinearLayout) findViewById(R.id.operate_done);
        mOperateDoneLayout.setOnClickListener(null);

        ViewStub viewStub = null;
        if(Build.MAKEUP_ON) {
            mEditMakeUpLayout.setVisibility(View.VISIBLE);
            viewStub = (ViewStub) findViewById(R.id.layout_login_pro);
        } else {
            mEditMakeUpLayout.setVisibility(View.GONE);
            viewStub = (ViewStub) findViewById(R.id.layout_login_normal);
        }

        // CID 109112 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        // final int layout_width = (int)(mScreenWidth / 5.5);
        // some devices can not supported effect, remove effect item from radio
        // group for these
        if (!Models.isSupportedEffect()) {
            btnRadioGroup.removeView(findViewById(R.id.tab_effect_layout));
        }
        for (int j = 0; j < mRadioItemOfIDPhoto.length; j++) {
            btnRadioGroup.removeView(findViewById(mRadioItemOfIDPhoto[j]));
        }
        String action = getIntent().getAction();
        Log.d(TAG, "findWidget(): viewStub is " + viewStub + ", mIsFromLaunch is " + mIsFromLaunch + ", action is " + action) ;
//        if(viewStub != null && mIsFromLaunch) {
        if(viewStub != null && (!TextUtils.isEmpty(action) && Intent.ACTION_MAIN.equals(action))) {
            viewStub.inflate();
        }
        mEditorContainer =  findViewById(R.id.uphoto_editor_layout);
//        if(mEditorContainer != null) {
//            mEditorContainer.setVisibility(View.VISIBLE);
//        }
        mMakeupContainer = findViewById(R.id.uphoto_makeup_layout);
        mEditorEntryView = findViewById(R.id.btn_editor_entry);
        mMakeupEntryView = findViewById(R.id.btn_makeup_entry);

        /*
         * FIX BUG: 1749
         * BUG CAUSE: The Label and Brush view do not intercept the clickable event.
         * FIX COMMENT: Response the clickable event but do nothings.
         * DATE: 2012-10-19
         */
        mLabelLayout.setOnClickListener(this);
        mBrushRootLayout.setOnClickListener(this);

        saveBtn.setEnabled(false);
        mLayoutSave.setEnabled(false);
        unDoBtn.setEnabled(false);
        mLayoutUndo.setEnabled(false);
        reDoBtn.setEnabled(false);
        mLayoutRedo.setEnabled(false);
        //mBtnSaveAs.setEnabled(false);
        mLabelOk.setEnabled(false);
        mLayoutOk.setEnabled(false);

        btnRadioGroup.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                // fix bug 29160,scroll to the first position when click the radio button.
                mSubHorizontalScrollView.scrollTo(0, 0);
            }
        });
        mMakeupFaceView = new MakeupFaceView(ImageEditControlActivity.this);

            mMosaicEraseBtn = (ImageButton) findViewById(R.id.btn_erase_mosaic);
            mMosaicDrawBtn = (ImageButton) findViewById(R.id.btn_draw_mosaic);
            mMosaicSeekBar = (SeekBar) findViewById(R.id.seekbar_mosaic);
            mMosaicMarkView = (ImageView) findViewById(R.id.mosaic_new_mark_view);
            /*SharedPreferences sharedPrf = PreferenceManager.getDefaultSharedPreferences(this);
            String isFirstLaunch = sharedPrf.getString(KEY_FIRST_LAUNCH, "on");
            if(isFirstLaunch.equals("on")) {
                mMosaicMarkView.setVisibility(View.VISIBLE);
                Editor editor = sharedPrf.edit();
                editor.putString(KEY_FIRST_LAUNCH, "off");
                editor.commit();
            }*/
            mMosaicRel = (RelativeLayout) findViewById(R.id.rel_mosaic);
            mMosaicRel.setOnTouchListener(new View.OnTouchListener() {
                @Override
                public boolean onTouch(View v, MotionEvent event) {
                    return true;
                }
            });
            mMosaicGridBtn = (ImageButton) findViewById(R.id.btn_mosaic_grid);
            mMosaicLineView = (View) findViewById(R.id.mosaic_line_view);
            mMosaicEraseBtn.setOnClickListener(this);
            mMosaicDrawBtn.setOnClickListener(this);
            mMosaicGridBtn.setOnClickListener(this);
    }

    private void initWidgetOnClickListener() {
        actionArrays.append(saveBtn.getId(), saveRunnable);
        actionArrays.append(mLayoutSave.getId(), saveRunnable);
        actionArrays.append(unDoBtn.getId(), unDoRunnable);
        actionArrays.append(mLayoutUndo.getId(), unDoRunnable);
        actionArrays.append(reDoBtn.getId(), reDoRunnable);
        actionArrays.append(mLayoutRedo.getId(), reDoRunnable);
        actionArrays.append(shareBtn.getId(), shareRunnable);
        actionArrays.append(mLayoutShare.getId(), shareRunnable);

        actionArrays.append(mEditBtn.getId(), mEditorRunnable);
        actionArrays.append(mEffectBtn.getId(), mEffectRunnable);
        actionArrays.append(mPhotoFrameBtn.getId(), mPhotoFrameRunnable);
        actionArrays.append(mTextBtn.getId(), mTextRunnable);
        actionArrays.append(mGraffitiBtn.getId(), mGraffitiRunnable);
        actionArrays.append(mMakeupBtn.getId(), mMakeupRunnable);
        actionArrays.append(mColorBtn.getId(), mColorRunnable);
        actionArrays.append(mBalloonBtn.getId(), mBubbleRunnable);
        actionArrays.append(mLabelBtn.getId(), mLabelRunnable);
        actionArrays.append(mDecorationBtn.getId(), mDecorationRunnable);
        actionArrays.append(mTextureBtn.getId(), mTextureRunnable);
        actionArrays.append(mMosaicBtn.getId(), mMosaicRunnable);
//        actionArrays.append(mIDTypeBtn.getId(), mIDTypeRunnable);

        saveBtn.setOnClickListener(this);
        unDoBtn.setOnClickListener(this);
        reDoBtn.setOnClickListener(this);
        shareBtn.setOnClickListener(this);
        mLayoutUndo.setOnClickListener(this);
        mLayoutRedo.setOnClickListener(this);
        mLayoutShare.setOnClickListener(this);
        mLayoutSave.setOnClickListener(this);

        mOperateOkBtn.setOnClickListener(this);
        mOperateCancelBtn.setOnClickListener(this);

        mEditBtn.setOnClickListener(this);
        mEffectBtn.setOnClickListener(this);
        mPhotoFrameBtn.setOnClickListener(this);
        mTextBtn.setOnClickListener(this);
        mGraffitiBtn.setOnClickListener(this);
        mMakeupBtn.setOnClickListener(this);
        mColorBtn.setOnClickListener(this);
        mBalloonBtn.setOnClickListener(this);
        mLabelBtn.setOnClickListener(this);
        mDecorationBtn.setOnClickListener(this);
        mTextureBtn.setOnClickListener(this);
        mBackgroundBtn.setOnClickListener(this);
        mMosaicBtn.setOnClickListener(this);

        mLabelOk.setOnClickListener(this);
        mLayoutOk.setOnClickListener(this);
        //mBtnSaveAs.setOnClickListener(this);
        mLayoutBack.setOnClickListener(this);
        findViewById(R.id.edit_view_back).setOnClickListener(this);
        findViewById(R.id.brush_view_back).setOnClickListener(this);

        mBrushSizeSeekbar.setOnSeekBarChangeListener(new BrushSizeSeekBarListener(false));
        mEraserSizeSeekBar.setOnSeekBarChangeListener(new BrushSizeSeekBarListener(true));
        mBrushGallery.setOnItemSelectedListener(new BrushOnItemSelectedListener());
        mBrushHistoryGallery.setOnItemClickListener(new BrushHistoryOnItemClickListener());
        mBrushHistoryGallery.setOnItemLongClickListener(new BrushHistoryOnItemLongClickListener());
        mBtnBrushOK.setOnClickListener(this);
        //mBtnBrushSaveAs.setOnClickListener(this);
        mBrushColorPickerView.setListener(new BrushOnColorChangedListener());

        View captureBtn = findViewById(R.id.btn_camera_capture);
        View choosePicBtn = findViewById(R.id.btn_choose_photo);
        View makeupCaptureView = findViewById(R.id.btn_makeup_capture);
        View makeupPhotoView = findViewById(R.id.btn_makeup_photo);
        View collageBtn = findViewById(R.id.btn_collage_entry);
        View graffitiBtn = findViewById(R.id.btn_uphoto_graffiti);
        View snsBtn = findViewById(R.id.btn_uphoto_share);
        View settingsBtn = findViewById(R.id.btn_uphoto_settings);
        View hideView = findViewById(R.id.label_hide_view);
        View downloadBtn = findViewById(R.id.btn_download_center);
        if(snsBtn != null) snsBtn.setOnClickListener(this);
        if(collageBtn != null) collageBtn.setOnClickListener(this);
        if(captureBtn != null) captureBtn.setOnClickListener(this);
        if(choosePicBtn != null) choosePicBtn.setOnClickListener(this);
        if(makeupCaptureView != null) makeupCaptureView.setOnClickListener(this);
        if(makeupPhotoView != null) makeupPhotoView.setOnClickListener(this);
        if(mMakeupEntryView != null) mMakeupEntryView.setOnClickListener(this);
        if(mEditorEntryView != null) mEditorEntryView.setOnClickListener(this);
        if(graffitiBtn != null) graffitiBtn.setOnClickListener(this);
        if(settingsBtn != null) settingsBtn.setOnClickListener(this);
        if(hideView != null) hideView.setOnClickListener(this);
        if(downloadBtn != null) downloadBtn.setOnClickListener(this);
        findViewById(R.id.brush_hide_view).setOnClickListener(this);
        mBalloonStyles = new int[]{ImageEditConstants.BALLOON_STYLE_OVAL, ImageEditConstants.BALLOON_STYLE_EXPLOSION,
                ImageEditConstants.BALLOON_STYLE_CLOUND, ImageEditConstants.BALLOON_STYLE_RECT, ImageEditConstants.BALLOON_STYLE_CHAMFER};
    }

    private void inVisibleAdjustControlProcess() {
        mAdjustProcess = false;
        mAdjustSclControl.adjustControlVisibility(false);
        updatePreview(mImageEditDesc.getBitmap());
    }

    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume(): entry...");
        /*if (mAdjustProcess) { //fix bug 28789
            updatePreview(getCurrentBitmap());
        }*/

        if (mFontGridView != null && mFontGridView.getAdapter() != null) {
            /**
             * FIX BUG: 1203
             * FIX COMMENT: reset the font adapter
             * DATE: 2012-08-06
             */
            setFontAndLabel();
        }

        /*
         * FIX BUG: 2775
         * FIX COMMENT: When onResume execute completion, to start loading graffiti view;
         * DATE: 2013-03-06
         */
        if(mEntryModule == ACTION_GRAFFITI) {
            mEntryModule = -1;
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    mCancelAnimation = true;
                    gotoGraffiti();
                }
            }, 10);
        } else if(mEntryModule == ACTION_EDIT_MAKEUP) {
            mEntryModule = -1;
            mCancelAnimation = true;
            //mMakeupBtn.setChecked(true);
            mMakeupRunnable.run();
        }
        this.findViewById(R.id.btn_edit_share).setVisibility(View.GONE); //SPRD:Fix bug 461758
//        }
//        if(com.ucamera.ucam.Build.EVENT.isOn()){
////            checkNewEvent();
//        }
    }

    /*
     * Do NOT remove, used for Stat
     * (non-Javadoc)
     * @see android.app.ActivityGroup#onPause()
     */
    @Override
    protected void onPause() {
        super.onPause();
    }
    @Override
    public void onBackPressed() {
        super.onBackPressed();
    }
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if(keyCode == KeyEvent.KEYCODE_BACK) {

            if(mLoginView != null && mLoginView.isShown() && !mIsFromCamera) {
                // click double back in 2s to exit
                Utils.onClickBackToExit(ImageEditControlActivity.this);
                return true;
            }
            if(null != effectShowed && effectShowed.onKeyEvent(keyCode, event)){
                // close effect showed gridview
                return true;
            }

            if(mLabelLayout.isShown() && !mLabelDeleteVisible) {
                mLabelLayout.setVisibility(View.GONE);
                mEditText.setText("");
                mFromLabelModify = false;
                if(true == mInsertTitleIntoDB && mViewTag == ImageEditConstants.LABEL_TAG_TITLE) {
                    refreshTabLabelItem(false);
                }
                dismissSubMenuGridView();
                return true;
            } else if(mBrushRootLayout.isShown() && !isLongClickStatus) {
                mBrushRootLayout.setVisibility(View.GONE);
                if(true == mInsertBrushIntoDB) {
                    refreshBrushItem(false);
                }
                return true;
            } else if(mOperateDoneLayout.isShown() && !isLongClickStatus) {
                if(mTipStubViewHint != null && mTipStubViewHint.isShown()) {
                    mTipStubViewHint.setVisibility(View.GONE);
                    return true;
                }
                if(mDeblemishView != null && mDeblemishView.isShown()) {
                    mDeblemishView.setVisibility(View.GONE);
                    return true;
                }
                mCurrentFunModePost = -1;
                mSubMenuSelectedIndex = -1;
//                mEffectRootView.setVisibility(View.GONE);
                mEraserLayout.setVisibility(View.GONE);
                if(mCurrentFuncMode == ACTION_EDIT_MAKEUP){
                    addMakeupBitmap(false);
                } 
                if(mCurrentBitmap != null){
                    mCurrentBitmap.recycle();
                    mCurrentBitmap = null;
                }
                onPreviewChanged();
                dismissSubMenuGridView();
                return true;
            }

            if((mCurrentFuncMode == ACTION_EDIT_TEXT ||
                    mCurrentFuncMode == ACTION_EDIT_TAB_LABEL ||
                    mCurrentFuncMode == ACTION_TEXT_BUBBLE)
                    && mLabelLayout.isShown() && true == mLabelDeleteVisible) {
                mLabelDeleteVisible = false;
                if(mLabelHistoryAdapter instanceof MyCommonTitleBaseAdapter) {
                    ((MyCommonTitleBaseAdapter)mLabelHistoryAdapter).setDeleteViewVisible(isLongClickStatus);
                }else if(mLabelHistoryAdapter instanceof HistoryLabelAdapter) {
                    ((HistoryLabelAdapter)mLabelHistoryAdapter).setDeleteView(isLongClickStatus);
                }
                return true;
            } else if(mCurrentFuncMode == ACTION_GRAFFITI && mBrushRootLayout.isShown() && true == isLongClickStatus) {
                isLongClickStatus = false;
                //MyGraffitiViewBaseAdapter adapter = (MyGraffitiViewBaseAdapter) adapterArray.get(mGraffitiBtn.getId());
                if(mGraffitiHistoryAdapter != null) {
                    mGraffitiHistoryAdapter.setDeleteViewVisible(isLongClickStatus);
                }
                return true;
            }
            if(mImageEditDesc.getBitmap() != null) {
                exitConfirm();
            }
        } else if (keyCode == KeyEvent.KEYCODE_MENU) {
            return true;
        }
        /*
         * bugfix: 1976
         * fix comment: up to parent to process other key event
         * date: 2012-11-21
         */
        return super.onKeyDown(keyCode, event);
    }

    private void exitConfirm() {
        boolean saveEnabled = saveBtn.isEnabled();
        //if the seekbar value has been changed, exit UPhoto will be tip.
        /**
         * FIX BUG: 986
         * BUG CAUSE: When initialize data, to get the uri is null in the initData method and then in not initially would return;
         * FIX COMMENT: Judgment null pointer.
         * DATE: 2012-07-17
         */
        boolean isAdjustSclControl = (mAdjustSclControl == null) ? false : mAdjustSclControl.getIsChangeSeekbarValue();
        if(saveEnabled || isAdjustSclControl) {
            String filePath = getSaveImagePath();
            String fileName = new File(filePath).getName();
            AlertDialog.Builder builder = new AlertDialog.Builder(this)
            .setTitle(R.string.text_edit_exit_tip_title)
            .setMessage(getString(R.string.text_edit_exit_tip_message, ImageEditOperationUtil.getStoragePath(this, filePath)
                    + "/" + fileName))
            .setPositiveButton(R.string.text_edit_exit_tip_save, new OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    mExitAndSaved = true;
                    final Runnable runnable = actionArrays.get(R.id.btn_edit_storage);
                    if(runnable != null){
                        runnable.run();
                    }
                }
            })
            .setNeutralButton(R.string.text_edit_exit_tip_exit, new OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    exitToLogin();
                    removeHightlight();
                }
            })
            .setNegativeButton(R.string.text_edit_exit_tip_cancel, new OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                }
            })
            ;

            mAlertDialog = builder.create();
            if(mAlertDialog != null && !mAlertDialog.isShowing()) {
                mAlertDialog.show();
            }
        } else if(mIsFromLaunch){
            mImageEditDesc.clearAllQueue();
            /** FIX BUG: 161
             * BUG CAUSE: The status of adjustSclControl was not resetted.
             * FIX COMMENT: reset the status
             * Date: 2011-12-16
             */
            if (mAdjustProcess) {
                mAdjustProcess = false;
                /* SPRD: CID 108959 : Dereference after null check (FORWARD_NULL) @{ */
                if(mAdjustSclControl != null) {
                    mAdjustSclControl.adjustControlVisibility(false);
                }
                // mAdjustSclControl.adjustControlVisibility(false);
                /* @} */
            }
            if(mLoginView != null && !mLoginView.isShown()) {
                mIsOpenPhoto = true;
                warnMessageForOpenPhoto();
                centerLayout.removeAllViews();
                mImageEditDesc.releaseTempBitmap();
                //return true;
            }
        }

    }

    /** FIX BUG: 869
     * BUG CAUSE: not define the contents of the menu
     * FIX COMMENT: add custom menu
     * Date: 2012-04-23
     */
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        return super.onKeyUp(keyCode, event);
    }

    private void getBasicEditAdapter() {
        final Resources resources = getResources();
        String[] editTitles = resources.getStringArray(R.array.edit_function_item_titles);
        int[] editIconIds = getIconIds(resources, R.array.edit_function_item_icons);
        addBasicEditActionName(editTitles, methodNames);
        if(adapterArray.get(mEditBtn.getId()) != null){
            int length = editTitles.length;
            setDisplayItemCountsInWindow(length, 5, false);
            return;
        }
        BasicEditingSimpleAdapter basicEditingAdapter = new BasicEditingSimpleAdapter(
                this,
                getData(editTitles, editIconIds),
                R.layout.sub_gallery_item,
                new String[]{ImageEditConstants.ITEM_ICON, ImageEditConstants.ITEM_TITLE},
                new int[] {R.id.icon_selection, R.id.text_desc});

        adapterArray.put(mEditBtn.getId(), basicEditingAdapter);
    }

    private void getMakeupAdapter() {
        final Resources res = getResources();
        String[] makeupTitles = res.getStringArray(R.array.edit_makeup_item_titles);
        int[] makeupIconIds = getIconIds(res, R.array.edit_makeup_item_icons);
        addBasicEditActionName(makeupTitles, mMakeupMethodNames);
        int len = makeupTitles.length;
        if(adapterArray.get(mMakeupBtn.getId()) != null) {
            setDisplayItemCountsInWindow(len, 5, true);
            return;
        }
        /*
        SimpleAdapter galleryMakeupAdapter = new SimpleAdapter(this, getData(makeupTitles, makeupIconIds), R.layout.makeup_item, new String[]{ImageEditConstants.ITEM_ICON, ImageEditConstants.ITEM_TITLE},
                new int[] {R.id.icon_selection_makeup, R.id.text_desc_makeup});*/
        MakeupItemSimpleAdapter makeupAdapter = new MakeupItemSimpleAdapter(this, getData(makeupTitles, makeupIconIds), R.layout.makeup_item, new String[]{ImageEditConstants.ITEM_ICON, ImageEditConstants.ITEM_TITLE},
                new int[] {R.id.icon_selection_makeup, R.id.text_desc_makeup});
        adapterArray.put(mMakeupBtn.getId(), makeupAdapter);
    }

    private int[] getIconIds(Resources res, int iconsRes) {
        if(iconsRes == 0){
            return null;
        }
        final TypedArray array = res.obtainTypedArray(iconsRes);
        final int n = array.length();
        int ids[] = new int[n];
        int i = 0;
        while(i < n){
            ids[i] = array.getResourceId(i, 0);
            i++;
        }
        array.recycle();
        return ids;
    }

    private  List<? extends Map<String, ?>> getData(String[] editTitles,int[] editIcons) {
        final ArrayList<Map<String, ?>> myData = new ArrayList<Map<String, ?>>();
        Log.d(TAG, "getData(): editTitles.length = " + editTitles.length + ", editIcons.length = " + editIcons.length);
        int length = editTitles.length < editIcons.length ? editTitles.length
                                                          : editIcons.length;
        Log.d(TAG, "getData(): length = " + length);
        for(int i = 0; i < length; i++){
            HashMap<String, Object> map = new HashMap<String, Object>();
            map.put(ImageEditConstants.ITEM_TITLE, editTitles[i]);
            map.put(ImageEditConstants.ITEM_ICON, editIcons[i]);
            myData.add(map);
        }
        int countPerScreen = 5;
        if(mCurrentFuncMode == ACTION_EDIT_EFFECT) {
            countPerScreen = 4;
        }
        Log.d(TAG, "getData(): countPerScreen is " + countPerScreen);
        setDisplayItemCountsInWindow(length, countPerScreen, true);
        return myData;
    }

    private float setDisplayItemCountsInWindowForEffect(int totalCount) {
        float itemWidth = (1.0f + 0.1f) * UiUtils.effectItemWidth();
        perScrnCount = (int) (UiUtils.screenWidth() / itemWidth);
        mSubMenuGridView.setNumColumns(totalCount);
        final int layout_width =(int)( itemWidth * totalCount);
        mSubMenuGridView.setLayoutParams(new LinearLayout.LayoutParams(layout_width,
                LayoutParams.WRAP_CONTENT));
        return itemWidth;
    }

    /*
     * FIX BUG: 4879
     * FIX COMMENT: add args, because some menu need full screen
     * DATE: 2013-09-16
     */
    private int setDisplayItemCountsInWindow(int length, int count, boolean fullScreen) {
        mSubMenuGridView.setNumColumns(length);
        int itemWidth = UiUtils.effectItemWidth();
        if(fullScreen) {
            itemWidth = mScreenWidth / count;
            if(length > count) {
                itemWidth = itemWidth * 9 / 10;
            }
        }
        final int layout_width = itemWidth * length;
        mSubMenuGridView.setLayoutParams(new LinearLayout.LayoutParams(layout_width, LayoutParams.WRAP_CONTENT));
        if(mCurrentFuncMode == ACTION_EDIT_TAB_LABEL) {
            mSubMenuGridView.setHorizontalSpacing(5);
        }

        return itemWidth;
    }

    private int setDisplayItemCountsInWindow(GridView gridview, int length, int count) {
        gridview.setNumColumns(length);
        int itemWidth = mScreenWidth / count;
        if(length > count) {
            itemWidth = itemWidth * 9 / 10;
        }
        final int layout_width = itemWidth * length;

        gridview.setLayoutParams(new LinearLayout.LayoutParams(layout_width, LayoutParams.WRAP_CONTENT));

        return itemWidth;
    }

    private void setDisplayItemCountsInWindow(GridView gridView, int totalCount, int colsPerPage, int rowNum) {
        int totalColumns = totalCount; //font is single
        if(rowNum > 1) {
            int pageSize = colsPerPage * rowNum;
            if ((totalCount + pageSize -1) / pageSize > 1) {
                totalColumns = (totalCount + rowNum - 1) / rowNum;  //rowNum = 2 or 3; //numPerPage = 4;
            } else {
                totalColumns = colsPerPage;
            }
        }
        gridView.setNumColumns(totalColumns);
        int itemWidth = mScreenWidth / colsPerPage;
        if(totalCount > colsPerPage) {
            itemWidth = itemWidth * 9 / 10;
        }
        final int layout_width = itemWidth * totalColumns;
        gridView.setLayoutParams(new LinearLayout.LayoutParams(layout_width, LayoutParams.WRAP_CONTENT));
    }

    private void setGridViewPadding(GridView gridView, int totalCount, int colsPerPage, int itemViewWidth) {
        int itemWidth = mScreenWidth / colsPerPage;
        if(totalCount > colsPerPage) {
            itemWidth = itemWidth * 9 / 10;
        }
        gridView.setPadding((itemWidth - itemViewWidth) / 2, 0, (itemWidth - itemViewWidth) / 2, 0);
    }

    private void addBasicEditActionName(String[] editTitles, String[] methodNames){
        methodNameMap.clear();
        for(int i = 0; i < editTitles.length; i++){
            methodNameMap.put(editTitles[i], methodNames[i]);
        }
    }

    private String currentClassName;
    private int itemPositionSelected = 1;
    public void onItemClick(AdapterView<?> parent, final View view, int position, long id) {

        switch (mCurrentFuncMode) {
        case ACTION_BASIC_EDIT_FUNCTION:
            mCurrentBasicEditIndex = position;
            setBasicEditHightlight(position);
            if(mCurrentBasicEditIndex != CROP_INDEX) {
                if(CropImage.sConfirmBitmap != null) {
                    setCurrentBitmap(CropImage.sConfirmBitmap);
                    CropImage.sConfirmBitmap = null;
                }
            }
            CropImage.isNeedRelaout = false;
            invokeEditMethod(parent, view, position, id);
            break;
        case ACTION_EDIT_EFFECT:
            mCurrentFunModePost = -1;
//            invokeEditMethod(parent, view, position, id);
            handleFunEffect(position);
            break;

        case ACTION_GRAFFITI:
            MyGraffitiViewBaseAdapter graffitiAdapter = (MyGraffitiViewBaseAdapter) adapterArray.get(mGraffitiBtn.getId());
            if(graffitiAdapter != null && true == graffitiAdapter.getDeleteViewVisible()) {
                if(position > BrushConstant.getGraffitiPredefineAllItems(mShowCanvas) - 1 && mBrushArrayList != null && mBrushArrayList.size() > BrushConstant.getGraffitiPredefineAllItems(mShowCanvas)) {
                    deleteCommonBrush(graffitiAdapter, position);
                }
            } else {
                handleGraffitiItemAction(parent, view, position, id);
            }
            break;

        case ACTION_EDIT_TAB_LABEL:
            MyCommonTitleBaseAdapter titleAdapter = (MyCommonTitleBaseAdapter)adapterArray.get(mLabelBtn.getId());
            if(titleAdapter != null && true == titleAdapter.getDeleteViewVisible()) {
                if(position > 2) {
                    deleteCommonTitle(titleAdapter, position);
                }
            } else {
                handleTabLabel(parent, view, position, id);
            }
            break;

        case ACTION_EDIT_MAKEUP:
            if (mMakeupFaceView.isShown()) {
                mMakeupAdjustBtn.setEnabled(true);
                replaceImage();
            }
            setMakeupHightlight(position);
            invokeEditMethod(parent, view, position, id);
            break;

        case ACTION_DECORATION:
            handleDecorAction(parent, view, position, id);
            break;

        case ACTION_PHOTO_FRAME:
            DecorsAdapter frameAdapter = (DecorsAdapter) adapterArray.get(mPhotoFrameBtn.getId());
            if(DownloadCenter.RESOURCE_DOWNLOAD_ON) {
                if(position > 0 && position < frameAdapter.getCount()) {
                    mSubMenuSelectedIndex = position;
                    frameAdapter.setSelected(mSubMenuSelectedIndex);
                }
            } else {
                // CID 109188 : SA: Useless self-operation (FB.SA_FIELD_DOUBLE_ASSIGNMENT)
                // mSubMenuSelectedIndex = position;
                frameAdapter.setSelected(mSubMenuSelectedIndex = position);
            }
            handlePhotoFrame(parent, view, position, id);
            break;

        case ACTION_TEXTURE:
            DecorsAdapter textureAdapter = (DecorsAdapter) adapterArray.get(mTextureBtn.getId());
            if (textureAdapter == null) {
                break;
            }
            if(DownloadCenter.RESOURCE_DOWNLOAD_ON) {
                if(position > 0 && position < textureAdapter.getCount()) {
                    mSubMenuSelectedIndex = position;
                    textureAdapter.setSelected(mSubMenuSelectedIndex);
                }
            } else {
                mSubMenuSelectedIndex = position;
                textureAdapter.setSelected(mSubMenuSelectedIndex);
            }
            handleTexture(parent, view, position, id);
            break;
//        case ACTION_MOSAIC:
//            DecorsAdapter mosaicAdapter = (DecorsAdapter) adapterArray.get(mMosaicBtn.getId());
//            if(DownloadCenter.RESOURCE_DOWNLOAD_ON) {
//                if(position < mosaicAdapter.getCount() -1) {
//                    mSubMenuSelectedIndex = position;
//                    mosaicAdapter.setSelected(mSubMenuSelectedIndex);
//                }
//            } else {
//                mSubMenuSelectedIndex = position;
//                mosaicAdapter.setSelected(mSubMenuSelectedIndex);
//            }
//            handleMosaicRes(parent, view, position, id);
//            break;
        default:
            break;
        }
        isPreviewShow  = false;
        preActionCode = mCurrentFuncMode;
        itemPositionSelected = position;
    }
    private class MosaicOnItemListener implements OnItemClickListener{
        public void onItemClick(AdapterView<?> adapterView, View view, int position, long id) {
            DecorsAdapter mosaicAdapter = (DecorsAdapter) adapterArray.get(mMosaicBtn.getId());
            if(DownloadCenter.RESOURCE_DOWNLOAD_ON) {
                if(position >= 0 && position < mosaicAdapter.getCount()) {
                    mSubMenuSelectedIndex = position;
                    mosaicAdapter.setSelected(mSubMenuSelectedIndex);
                }
            } else {
                mSubMenuSelectedIndex = position;
                mosaicAdapter.setSelected(mSubMenuSelectedIndex);
            }
            handleMosaicRes(adapterView, view, position, id);
        }
    }
    @Override
    public boolean onItemLongClick(AdapterView<?> adapterView, View view, int position, long id) {
        /*
         * FIX BUG: 3585
         * FIX COMMENT: the saved label can delete
         * FIX DATE: 2013-04-18
         */
        if(mCurrentFuncMode == ACTION_EDIT_TAB_LABEL && mLabelList != null && mLabelList.size() > 3 && position > 2) {
            MyCommonTitleBaseAdapter adapter = (MyCommonTitleBaseAdapter)adapterArray.get(mLabelBtn.getId());
            adapter.setDeleteViewVisible(true);
            isLongClickStatus = true;
            return true;
        } else if(mCurrentFuncMode == ACTION_GRAFFITI && mBrushArrayList != null && mBrushArrayList.size() > BrushConstant.getGraffitiPredefineAllItems(mShowCanvas)
                && position > BrushConstant.getGraffitiPredefineAllItems(mShowCanvas) - 1) {
            MyGraffitiViewBaseAdapter adapter = (MyGraffitiViewBaseAdapter) adapterArray.get(mGraffitiBtn.getId());
            adapter.setDeleteViewVisible(true);
            isLongClickStatus = true;
            return true;
        }
        return false;
    }

    private void deleteCommonTitle(BaseAdapter adapter, int position) {
        if(mLabelList != null && mLabelList.size() > 0) {
            Object object = mLabelList.get(position);
            if(object instanceof ViewAttributes) {
                ViewAttributes attributes = (ViewAttributes) object;
                if(mTitleSaveAsDatabase == null || !mTitleSaveAsDatabase.isOpened()) {
                    mTitleSaveAsDatabase = new TitleSaveAsDatabase(this);
                }

                if(true == mTitleSaveAsDatabase.deleteTitle(attributes)) {
                    refreshTabLabelItem(true);
                }

                if(position < mLabelHistoryAdapter.getCount()){
                    mLabelHistoryGallery.setSelection(position);
                }else {
                    mLabelHistoryGallery.setSelection(position - 1);
                }
            }
        }
    }

    private void deleteCommonBrush(MyGraffitiViewBaseAdapter adapter, int position) {
        Object object = mBrushHistoryArrayList.get(position);
        if(object instanceof BrushItemInfo) {
            BrushItemInfo info = (BrushItemInfo) object;
            long brushId = info.brushId;
            if(mTitleSaveAsDatabase == null || !mTitleSaveAsDatabase.isOpened()) {
                mTitleSaveAsDatabase = new TitleSaveAsDatabase(this);
            }
            if(true == mTitleSaveAsDatabase.deleteBrush(info.brushId)) {
                refreshBrushItem(true);
            }

            if(position < mGraffitiHistoryAdapter.getCount()){
                mBrushHistoryGallery.setSelection(position);
            }else {
                mBrushHistoryGallery.setSelection(position - 1);
            }
        }
    }

    private void handleTabLabel(AdapterView<?> parent, View view, int position, long id) {
        if(position < 3 && mCurrentLabelIndex != position) {
            setViewColorByXml(position);
            mCurrentLabelIndex = position;
        }
        if(position == ImageEditConstants.LABEL_TAG_TITLE_INDEX) {
            setCurrentAttrWhenShowTitleLayout();
            String inputText = showTitleText();
            if(mAttrList != null && mAttrList.size() > 0) {
                mCurrentAttr = mAttrList.get(0);
                mTitleView.setTitleStyle(mCurrentAttr, inputText, false, position);
            }
        } else if(position == ImageEditConstants.LABEL_TAG_BALLOON_INDEX) {
            setCurrentAttrWhenShowBalloonLayout();
            String inputText = showTitleText();
            if(mAttrList != null && mAttrList.size() > 0) {
                if(mCurrentAttr != null) {
                    mCurrentAttr = null;
                }
                mCurrentAttr = mAttrList.get(0);
                // CID 109155 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
                // String textSize = mCurrentAttr.getTextSize();
                mCurrentBalloonTextSize = 28f;
                /**
                 * FIX BUG: 5388
                 * BUG CAUSE: Balloon preview text size to big on mdpi devices
                 * Date: 2013-11-27
                 */
                if(UiUtils.screenDensity()<=1.0f){
                    mCurrentBalloonTextSize = 16f;
                }
                mCurrentAttr.setTextSize(String.valueOf(mCurrentBalloonTextSize));
                mCurrentAttr.setDrawText(inputText);
                if(mBalloonStyleAdapter != null) {
                    mBalloonStyleAdapter = null;
                }
                mBalloonStyleAdapter = new MyBalloonStyleAdapter(ImageEditControlActivity.this, mCurrentAttr, mBalloonStyles, R.layout.balloon_style_item);
                mBalloonGallery.setOnItemSelectedListener(new BalloonStyleOnItemSelectedListener());
                mBalloonGallery.setAdapter(mBalloonStyleAdapter);
            }
        } else if(position == ImageEditConstants.LABEL_TAG_LABEL_INDEX) {
            setCurrentAttrWhenShowLabelLayout();
            String inputText = showTitleText();
            if(mAttrList != null && mAttrList.size() > 0) {
                mCurrentAttr = mAttrList.get(0);
                mLabelView.setLabelStyle(mCurrentAttr, inputText, 90);
            }
        }

        if(mLabelHistoryAdapter != null && mLabelHistoryAdapter.getCount() > 0){
            mLabelHistoryAdapter = null;
        }
        mLabelHistoryAdapter = getHistoryLabelAdapter(mLabelDeleteVisible);
        mLabelHistoryGallery.setAdapter(mLabelHistoryAdapter);
    }

    private void setViewColorByXml(int index) {
        try {
            SAXParserFactory factory = SAXParserFactory.newInstance();
            SAXParser parser = factory.newSAXParser();
            InputStream inputStream = null;
//            InputStream labelInputStream = null;
//            String parserLabelXml = "label_preview_attr.xml";
            String parserXml = "title_attr.xml";
            if(index == ImageEditConstants.LABEL_TAG_BALLOON_INDEX) {
                //Balloon
                parserXml = "balloon_attr.xml";
            } else if(index == ImageEditConstants.LABEL_TAG_LABEL_INDEX) {
                //Label
                parserXml = "label_attr.xml";
            }
            inputStream = getAssets().open(parserXml);
//            labelInputStream = getAssets().open(parserLabelXml);
            AttributesDefaultHandler attrHandler = new AttributesDefaultHandler();
//            AttributesDefaultHandler labelAttrHandler = new AttributesDefaultHandler();
            parser.parse(inputStream, attrHandler);
//            parser.parse(labelInputStream, labelAttrHandler);
            if(mAttrList != null) {
                mAttrList.clear();
                mAttrList = null;
            }
            if(mColorAdapter != null) {
                mColorAdapter = null;
            }
            if(mCurrentAttr != null) {
                mCurrentAttr = null;
            }
            mAttrList = attrHandler.getAttributesList();
//            mLabelPreviewAttrList = labelAttrHandler.getAttributesList();
            if(mAttrList != null && mAttrList.size() > 0) {
                Paint paint = new Paint();
                int size = mAttrList.size();
                int colsPerPage = 0;
                int itemViewWidth = 0;
                if(index == ImageEditConstants.LABEL_TAG_BALLOON_INDEX || index == ImageEditConstants.LABEL_TAG_TITLE_INDEX) {
                    //Title and Balloon
                    colsPerPage = 3;
                    if(index == ImageEditConstants.LABEL_TAG_BALLOON_INDEX) {
                        itemViewWidth = UiUtils.dpToPixel(50);
                    }else {
                        paint.setTextSize(UiUtils.dpToPixel(28));
                        itemViewWidth = (int)paint.measureText("Color");
                    }
                    setDisplayItemCountsInWindow(mColorGridView, size, colsPerPage, 1);
                    mColorAdapter = new MyLabelBaseAdapter(this, mAttrList, null, R.layout.label_title_balloon_item, "Color", index);
                } else if(index == ImageEditConstants.LABEL_TAG_LABEL_INDEX) {
                    //Label
                    colsPerPage = 2;
                    paint.setTextSize(UiUtils.dpToPixel(24));
                    itemViewWidth = (int)paint.measureText("Preview");
                    setDisplayItemCountsInWindow(mColorGridView, size, colsPerPage, 1);
                    mColorAdapter = new MyLabelBaseAdapter(this, mAttrList, null, R.layout.label_title_balloon_item, "Label", index);
                }

                mColorGridView.setAdapter(mColorAdapter);
                mColorGridView.setOnItemClickListener(new ColorOnItemListener());
                setGridViewPadding(mColorGridView, size, colsPerPage, itemViewWidth);
                setFontAndLabel();
            }
        } catch (ParserConfigurationException e) {
            e.printStackTrace();
        } catch (SAXException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void setFontAndLabel() {
        mFontArray = listAvailableFonts();
        if (mFontArray != null && mFontArray.length > 0) {
            int len = mFontArray.length;
            Paint paint = new Paint();
            paint.setTextSize(UiUtils.dpToPixel(28));
            int viewWidth = (int)paint.measureText("Font");
            setDisplayItemCountsInWindow(mFontGridView, len, 3, 1);
            mFontGridView.setAdapter(new MyLabelBaseAdapter(this, null, mFontArray, R.layout.label_title_balloon_item, "Font"));
            mFontGridView.setOnItemClickListener(new FontOnItemListener());
            setGridViewPadding(mFontGridView, len, 3, viewWidth);
        }
    }

    private String[] listAvailableFonts() {
        String[] fonts = FontResource.listFonts(this, mLanguage);
        String[] tmp = new String[fonts.length + 1];
        String[] downloadIcon = new String[] { "DOWNLOAD" };
        System.arraycopy(downloadIcon, 0, tmp, 0, 1);
        System.arraycopy(fonts, 0, tmp, 1, fonts.length);
        return tmp;
    }

    private void showLabelOnPreview(ViewAttributes attributes, int position, String viewTag) {
        unDoBtn.setEnabled(true);
        unDoBtn.setPressed(false);//fixed the bug32368
        mLayoutUndo.setEnabled(true);
        saveBtn.setEnabled(true);
        mLayoutSave.setEnabled(true);
        //fixed the bug31990, regardless the background image scaling,
        //to perform redo or undo, and then to add one text bubble,
        //it is not necessary to reinstate the background picture.
        setIsUnDoOrReDo(false);

        mBubbleImageView = null;
        int childCount = centerLayout.getChildCount();
        if(childCount > 1){
            Bitmap bitmap = composeTextBubbleBitmap();
            while(bitmap == null){
                //index is one, means that any operation can not be Completed
                if(mImageEditDesc.reorganizeQueue() < 2) {
                    ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                    return;
                }
                bitmap = composeTextBubbleBitmap();
            }
            updateImageEditBitmap(bitmap);
            ImageEditViewPreview.ScrollController scroller = (ScrollController) centerLayout.getChildAt(0);
            ImageEditViewPreview preview = (ImageEditViewPreview) scroller.getChildAt(0);
            preview.setImageBitmap(mImageEditDesc.getBitmap());
        } else {
            mImageEditDesc.clearReDoQueue();
        }

        RectF rectF = ImageEditOperationUtil.resizeRectF(mImageEditDesc.getBitmap(), centerLayout.getWidth(), centerLayout.getHeight());
        Bitmap bubbleBitmap = null;
        do {
            try {
                bubbleBitmap = Bitmap.createBitmap(centerLayout.getWidth(), centerLayout.getHeight(), Bitmap.Config.ARGB_8888);
            } catch(OutOfMemoryError oom) {
                Log.w(TAG, "handleTextBubble(): code has a memory leak is detected...");
            }
            if(bubbleBitmap == null) {
                //index is one, means that any operation can not be Completed
                if(mImageEditDesc.reorganizeQueue() < 2) {
                    ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                    return;
                }
            }
        } while(bubbleBitmap == null);

        if(childCount > 1){
            View childView = centerLayout.getChildAt(1);
            if(childView instanceof MyTextBubbleImageView){
                mBubbleImageView = (MyTextBubbleImageView) childView;
                mBubbleImageView.setImageBitmap(bubbleBitmap);
                return;
            }
        }

        int labelShape = -1;
        int[] labelDimen = null;
        Matrix labelBodyMatrix = null;
        if(ImageEditConstants.LABEL_TAG_TITLE.equals(viewTag)) {
            labelShape = ImageEditConstants.LABEL_TITLE_SHAPE;
        } else if(ImageEditConstants.LABEL_TAG_BALLOON.equals(viewTag)) {
            labelShape = mCurrentSelectedBalloonStyle;
        } else if(ImageEditConstants.LABEL_TAG_LABEL.equals(viewTag)) {
            labelShape = ImageEditConstants.LABEL_LABEL_SHAPE;
            int labelViewWidth = mLabelView.getLabelWidth();
            int labelViewHeight = mLabelView.getLabelHeight();
            labelDimen = new int[]{labelViewWidth, labelViewHeight};
            labelBodyMatrix = mLabelView.getBodyMatrix();
        }
        /*if(position < 3) {
            attributes.setDrawText(mEditText.getText().toString());
        }*/
        mEditText.setText("");

        mBubbleImageView = new MyTextBubbleImageView(this, mHandler);
        mBubbleImageView.setVisibilityRect(rectF);
        mBubbleImageView.setCurrentLabelShape(labelShape);
        mBubbleImageView.setCurrentViewAttributes(attributes);
        mPreviewBubble = null;
        mPreviewBubble = onPreviewChanged();
        mBubbleImageView.setGroundLayer(centerLayout, mPreviewBubble);
        mBubbleImageView.setImageBitmap(bubbleBitmap);
        centerLayout.addView(mBubbleImageView);
        mBubbleImageView.addTextBubbleBox(labelShape, attributes, labelDimen, labelBodyMatrix);
    }

    private void handleGraffitiItemAction(final AdapterView<?> parent, final View view, int position, final long id) {
        final int index = (mShowCanvas)? position: position + 1;

        final int canvasColor = BrushConstant.getRandomColor();
        if(ImageEditConstants.BRUSH_ITEM_ERASER != index && ImageEditConstants.BRUSH_ITEM_CUSTOM != index) {
            mEraserLayout.setVisibility(View.GONE);
        }
        if(ImageEditConstants.BRUSH_ITEM_CLEAR == index) {
            if(mBrush != null && mBrush.mBrushStyle == BrushConstant.EraserBrush) {
                randomGenerateBrush();
                mBrushMode = BrushConstant.BrushModeRandom;
                mBrushPainting.setBrushStyle(mBrushStyle);
                mBrushPainting.setBrushColor(mBrushColor);
                mBrushPainting.setBrushSize(mBrushSize);
                mBrushPainting.setBrushMode(mBrushMode);
                mBrushPainting.setAlpha(255);
            }
            mBrushPainting.clearAllStrokers();
            handleBrushLabelView(index, canvasColor, false);
        } else if(ImageEditConstants.BRUSH_ITEM_ERASER == index) {
            mEraserLayout.setVisibility(View.VISIBLE);
            /**
             * FIX BUG: 993
             * BUG CAUSE: Use Eraser Brush, not initialize mBrush
             * FIX COMMENT: Judge mBrush value before use it.
             * Date: 2012-05-16
             */
            if(mBrushStyle != BrushConstant.EraserBrush || mBrush == null) {
                mBrushStyle = BrushConstant.EraserBrush;
                mBrushColor = Color.BLACK;
                mBrushMode = BrushConstant.BrushModeSelected;
                mBrush = BaseBrush.createBrush(mBrushStyle);
                setSizeSeekBar(true);
                mBrushSize = mBrush.getSize();
                mBrushPainting.setBrushStyle(mBrushStyle);
                mBrushPainting.setBrushColor(mBrushColor);
                mBrushPainting.setBrushSize(mBrushSize);
                mBrushPainting.setBrushMode(mBrushMode);
                mBrushPainting.setAlpha(255);
            }
        } else if(ImageEditConstants.BRUSH_ITEM_AUTO == index) {
            randomGenerateBrush();
            mBrushMode = BrushConstant.BrushModeRandom;
            mBrushPainting.setBrushStyle(mBrushStyle);
            mBrushPainting.setBrushColor(mBrushColor);
            mBrushPainting.setBrushSize(mBrushSize);
            mBrushPainting.setBrushMode(mBrushMode);
            mBrushPainting.setAlpha(255);
        } else if(ImageEditConstants.BRUSH_ITEM_CUSTOM == index) {
            mBrushStyle = BrushConstant.LineBrush;
            resetBrushColor();
            Utils.showAnimation(mBrushRootLayout, new LayoutAnimListener(mBrushRootLayout));
            mBrushMode = BrushConstant.BrushModeSelected;

            mBrushGallery.setAdapter(mBrushListAdapter);
            mGraffitiHistoryAdapter = getGraffitiHistoryAdapter(true);
            mBrushHistoryGallery.setAdapter(mGraffitiHistoryAdapter);
        } else if(ImageEditConstants.BRUSH_ITEM_CANVAS_COLOR == index) {
            /**
             * FIX BUG: 936 4086
             * BUG CAUSE: New feature
             * FIX COMMENT:
             * Date: 2012-05-15
             */
            if(mBrushPainting.getStrokeCount() > 0 || (mImageEditDesc != null && mImageEditDesc.getqueueSize() > 1)) {
                new AlertDialog.Builder(this)
                .setTitle(R.string.text_edit_graffiti_new_canvas)
                .setMessage(R.string.text_edit_graffiti_message)
                .setPositiveButton(R.string.text_edit_exit_tip_save, new OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        /*
                         * BUG FIX: 4085
                         */
                        addGraffitiBitmap();
                        final Runnable runnable = actionArrays.get(R.id.btn_edit_storage);
                        if(runnable != null){
                            mSaveByGraffiti = true;
                            runnable.run();
                        }
                    }
                })
                .setNeutralButton(R.string.text_edit_graffiti_not_save, new OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        clearAllRecordsWhenChangeCanvas(index, canvasColor);
                    }
                })
                .setNegativeButton(R.string.text_edit_exit_tip_cancel, new OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                    }
                }).show()
                ;
            } else {
                if(mCanvasClickedCount == 0) {
                    int clickedCount  = mSharedPreferences.getInt("clickedCount", 0);
                    if(clickedCount <= 5) {
                        Toast.makeText(ImageEditControlActivity.this, getString(R.string.click_change_canvas) , Toast.LENGTH_SHORT).show();
                        clickedCount ++;
                        mSharedPreferences.edit().putInt("clickedCount", clickedCount).commit();
                    }
                    mCanvasClickedCount = -1;
                }
                clearAllRecordsWhenChangeCanvas(index, canvasColor);
            }
        } else if(position > BrushConstant.getGraffitiActionMaxIndex(mShowCanvas)) {
            if(mBrushArrayList != null && mBrushArrayList.size() > 0) {
                Object object = mBrushArrayList.get(position);
                if(object instanceof BrushItemInfo) {
                    BrushItemInfo info = (BrushItemInfo) object;
                    String brushColor = info.brushColor;
                    mBrushStyle = info.brushStyle;
                    mBrushPainting.setBrushStyle(mBrushStyle);
                    if(brushColor.startsWith("#")) {
                        mBrushPainting.setBrushColor(Color.parseColor(brushColor));
                    } else {
                        mBrushPainting.setBrushColor(Integer.valueOf(brushColor));
                    }
                    mBrushPainting.setBrushSize(info.brushSize);
                    mBrushPainting.setBrushMode(info.brushMode);
                    mBrushPainting.setAlpha(255); //255: default
                }
            }
        }
    }

    private void clearAllRecordsWhenChangeCanvas(int index, int canvasColor) {
        centerLayout.removeAllViews();
        mImageEditDesc.releaseTempBitmap();
        mImageEditDesc.clearAllQueue();
        mBrushPainting.clearAllStrokers();
        handleBrushLabelView(index, canvasColor, true);
    }

    private void reinitBrushPainting() {
        randomGenerateBrush();
        mBrushMode = BrushConstant.BrushModeRandom;
        if(mBrushPainting == null) {
            mBrushPainting = new BrushPainting(ImageEditControlActivity.this);
        }
        mBrushPainting.setBrushStyle(mBrushStyle);
        mBrushPainting.setBrushColor(mBrushColor);
        mBrushPainting.setBrushSize(mBrushSize);
        mBrushPainting.setBrushMode(mBrushMode);
        mBrushPainting.setAlpha(255);
    }

    private void handleBrushLabelView(int index, int canvasColor, boolean createNewPainting) {
        if(createNewPainting) {
            /*
             * FIX BUG: 3089
             * FIX COMMENT: Catch oom, and show toast;
             * DATE: 2013-03-21
             */
            try {
                Bitmap bitmap = Bitmap.createBitmap(centerLayout.getWidth(), centerLayout.getHeight(), Config.ARGB_8888);
                bitmap.eraseColor(canvasColor);
                mImageEditDesc.setBitmap(bitmap);

                ImageEditViewGraffitiView graffitiView = new ImageEditViewGraffitiView(this);
                graffitiView.setBottomLayer(onPreviewChanged());
                graffitiView.setGraffitiDimens(centerLayout.getWidth(), centerLayout.getHeight(), true, mBrushPainting);
                centerLayout.addView(graffitiView);
            } catch(OutOfMemoryError oom) {
                ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
            }
        } else {
            ImageEditViewGraffitiView graffitiView = null;
            if(centerLayout.getChildCount() <= 1){
                return;
            }
            View tempView = centerLayout.getChildAt(1);
            if(tempView instanceof ImageEditViewPreview.ScrollController){
                ImageEditViewPreview.ScrollController scroller = (ImageEditViewPreview.ScrollController)tempView;
                View childView = scroller.getChildAt(0);
                if(childView instanceof ImageEditViewGraffitiView){
                    graffitiView = (ImageEditViewGraffitiView) childView;
                }
            }
            if(graffitiView == null){
                Log.d(TAG, "handleBrushLabelView(): graffitiView is null");
                return;
            }
            graffitiView.chooseGraffitiPattern(index, canvasColor,mBrushPainting);
        }
    }

    private void setPreviewBrush(int brushStyle) {
        mBrushStyle = brushStyle;
        mBrush = BaseBrush.createBrush(mBrushStyle);
        mBrush.setContext(this);
        mBrush.setRandomColorPicker(mRandomColorPicker);
        mBrush.setImageBurshManager(mImageBurshManager);
        mBrush.setColor(Color.RED);
        mBrushSize = mBrush.mBrushSize;
        mBrush.setSize(mBrushSize);
        mBrush.setMode(mBrushMode);
    }

    private void setSizeSeekBar(boolean isEraser) {
        int maxSize = (int)((mBrush.mBrushMaxSize - mBrush.mBrushMinSize) * 10F);
        int currentProgress = (int)((mBrush.mBrushSize - mBrush.mBrushMinSize) * 10F);
        if(isEraser) {
            mEraserSizeSeekBar.setMax(maxSize);
            mEraserSizeSeekBar.setProgress(currentProgress);
        } else {
            mBrushSizeSeekbar.setMax(maxSize);
            mBrushSizeSeekbar.setProgress(currentProgress);
        }
    }

    private MakeupControlView mMakeupControlView;
    private RectView mRectView = null;
    private ProgressDialog mMakeupDlg = null;
    private void adjustBarVisiblility(String strMethodName) {
        if (mAdjustProcess) {
            invisibleAdjustAndSaveBitmap();
            isPreviewShow = true; // fix bug 31917
        }
    }

    @SuppressWarnings("unchecked")
    private void invokeEditMethod(AdapterView<?> parent, View view, int position, long id) {
        final Class clazz = this.getClass();
        HashMap<String, ?> map = (HashMap<String, ?>) parent.getItemAtPosition(position);
        String methodName = methodNameMap.get(map.get(ImageEditConstants.ITEM_TITLE));
        if(methodName == null){
            return;
        }
        adjustBarVisiblility(methodName);
        try {
            final Method method = clazz.getDeclaredMethod(methodName, AdapterView.class, View.class,
                                                          int.class, long.class);
            method.invoke(this, parent, view, position, id);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }


    protected void onStop() {
        Log.d(TAG, "onStop(): entry ...");
        unregisterReceiver(mReceiver);

        /* FIX BUG: 1596
         * FIX CAUSE: use incorrect uri
         * DATE: 2012-09-26
         */
        /*if(mImageEditDesc != null) {
            mImageEditDesc.setEditImageUri(null);
        }*/
        super.onStop();
    }

    /* FIX BUG: 1525
     * FIX CAUSE: use incorrect uri
     * FIX COMMENT: clear edit image uri in onDestroy
     * DATE: 2012-08-30
     */
    protected void onDestroy() {
        Log.d(TAG, "onDestroy(): entry ...");
        if (mImageEditDesc != null) {
            mImageEditDesc.uninitilizeCount();
            // SPRD: fix bug 558999 NullPointException when activity onDestory
            // mImageEditDesc = null;
            mImageEditDesc.setOriginalUri(null); // SPRD: Fix bug 605951
        }
        if(mTitleSaveAsDatabase != null) {
            mTitleSaveAsDatabase.closeDatabase();
        }
//        if (mGpuProcees != null) {
//            mGpuProcees.finish();
//        }
        //mEffectOriginJpegData = null;
        if (mEffectOriginBitmap != null && !mEffectOriginBitmap.isRecycled()) {
            mEffectOriginBitmap.recycle();
        }
        mArgbOriginData = null;
        mBubbleImageView = null;
        mBrushPainting = null;
        if (Build.MAKEUP_ON) {
            MakeupEngine.UnInit_Lib();
        }
        if(effectShowed != null) {
            effectShowed.onDestroy();
        }
        super.onDestroy();
    }

    public void onClick(View v) {
        int viewId = v.getId();
//        if(viewId != mEffectBtn.getId() && mEffectRootView.isShown()) {
        if(viewId != mEffectBtn.getId()) {
            mCurrentFunModePost = -1;
//            mEffectRootView.setVisibility(View.GONE);
        }
        if(viewId != mGraffitiBtn.getId() && mEraserLayout.isShown()) {
            mEraserLayout.setVisibility(View.GONE);
        }
        /*
         * FIX BUG: 5706
         * BUG COMMETN: The mSubHorizontalScrollView will scroll to the Correct position
         * DATE: 2014-01-02
         */
        if(viewId == mEditBtn.getId() || viewId == mEffectBtn.getId() || viewId == mPhotoFrameBtn.getId() || viewId == mTextureBtn.getId()) {
            if(mSubHorizontalScrollView != null) {
                mSubHorizontalScrollView.scrollTo(0, 0);
            }
        }
        if(viewId == mMosaicBtn.getId()) {
            if(mMosaicScrollView != null) {
                mMosaicScrollView.scrollTo(0, 0);
            }
        }

        final Runnable runnable = actionArrays.get(viewId);
        if(runnable != null){
            runnable.run();
        }
        switch(viewId) {
            case R.id.btn_editor_entry:
                new Rotate3dSet().applyRotation(mEditorContainer, mEditorEntryView);
                break;
            case R.id.btn_makeup_entry:
                new Rotate3dSet().applyRotation(mMakeupContainer, mMakeupEntryView);
                break;
            case R.id.btn_camera_capture:
                mShowCanvas = false;
                captureImage(REQUEST_CODE_CAMERA);
                break;
            case R.id.btn_makeup_capture:
                mShowCanvas = false;
                captureImage(REQUEST_CODE_MAKEUP_CAMERA);
                break;
            case R.id.btn_choose_photo:
                mShowCanvas = false;
                pickImage(REQUEST_CODE_PICK);
                break;
            case R.id.btn_makeup_photo:
                mShowCanvas = false;
                pickImage(REQUEST_CODE_MAKEUP_PICK);
                break;
            case R.id.btn_uphoto_settings:
                mShowCanvas = false;
//                Intent intentSettings = new Intent();
//                intentSettings.setClass(this, UPhotoPreferencesActivity.class);
//                startActivity(intentSettings);
                break;
            case R.id.btn_collage_entry:
                mShowCanvas = false;
//                com.ucamera.ugallery.ImageGallery.showImagePicker(this, com.ucamera.ucomm.puzzle.PuzzleImagePicker.class,
//                        com.ucamera.ugallery.ViewImage.IMAGE_ENTRY_PUZZLE_VALUE);
                Intent puzzleIntent = new Intent();
                puzzleIntent.setAction("android.intent.action.SPRD_PUZZLE");
                puzzleIntent .putExtra(com.ucamera.ugallery.ViewImage.EXTRA_IMAGE_ENTRY_KEY, com.ucamera.ugallery.ViewImage.IMAGE_ENTRY_PUZZLE_VALUE);
                puzzleIntent.putExtra(com.ucamera.ugallery.ViewImage.EXTRA_IMAGE_STORAGE_KEY, "external");
                puzzleIntent.putExtra(com.ucamera.ugallery.ImageGallery.INTENT_EXTRA_WAIT_FOR_RESULT, true);
                ;
                try {
                    startActivity(puzzleIntent);
                } catch (ActivityNotFoundException e) {
                }
                break;
            case R.id.btn_uphoto_graffiti:
                mShowCanvas = true;
                gotoGraffiti();
                break;
            case R.id.btn_uphoto_share:
                mShowCanvas = false;
                Intent snsIntent = new Intent();
                snsIntent.setClassName(this, "com.ucamera.ucomm.sns.ShareActivity");
                snsIntent.setAction("android.intent.action.UGALLERY_SHARE");
                snsIntent.setType("image/*");
                try {
                    startActivity(snsIntent);
                } catch(ActivityNotFoundException e) {
                    Log.w(TAG, "onClick(): not found SNS Share app!!!");
                    Toast.makeText(this, R.string.text_activity_is_not_found, Toast.LENGTH_LONG).show();
                }
                break;
            case R.id.btn_download_center:
                if(DownloadCenter.RESOURCE_DOWNLOAD_ON) {
                    Utils.openResourceCenter(this, mScreenDensity);
                }
                break;
            case R.id.edit_title_ok:
            case R.id.layout_title_ok:
                ((InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE)).hideSoftInputFromWindow(mEditText.getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);

                if(mLabelDeleteVisible) {
                    mLabelDeleteVisible = false;
                }
                // save current attr to db
                if(mCurrentAttr != null && mTitleSaveAsDatabase != null && mEditText.getText().toString().length() > 0) {
                    if(!mTitleSaveAsDatabase.isOpened()) {
                        mTitleSaveAsDatabase = new TitleSaveAsDatabase(this);
                    }
                    mCurrentAttr.setDrawText(mEditText.getText().toString());
                    mCurrentAttr.setTitleId(System.currentTimeMillis());
                    mCurrentAttr.setBalloonStyle(mCurrentSelectedBalloonStyle);
                    if(mTitleSaveAsDatabase.insertTitle(mCurrentAttr, mCurrentFuncMode)) {
                        ImageEditOperationUtil.showToast(ImageEditControlActivity.this, R.string.edit_title_save_as_toast, Toast.LENGTH_LONG);
                        mInsertTitleIntoDB = true;
                    }
                }
                // save finish
                if(mLabelLayout != null && mLabelLayout.isShown()) {
                    mLabelLayout.setVisibility(View.GONE);
                }

                if(mCurrentAttr != null && mEditText.getText().toString() != null && mEditText.getText().toString().length() > 0) {
                    if(true == mFromLabelModify) {
                        mFromLabelModify = false;
                        String inputText = mEditText.getText().toString();
                        /**
                         * FIX BUG: 999
                         * BUG CAUSE: When re-edit the content, did not clear the contents of the edit box
                         * FIX COMMENT: When re-edit is complete, remove the contents of the edit box
                         * DATE: 2012-05-16
                         */
                        mEditText.setText("");
                        mCurrentAttr.setDrawText(inputText);
                        if(mLabelShape == ImageEditConstants.LABEL_TITLE_SHAPE) {
                            if(mBubbleImageView != null) {
                                mBubbleImageView.updateTextBox(mLabelShape, mCurrentAttr, null, null);
                            }
                        } else if(mLabelShape == ImageEditConstants.LABEL_LABEL_SHAPE) {
                            if(mLabelView != null && mBubbleImageView != null) {
                                int labelViewWidth = mLabelView.getLabelWidth();
                                int labelViewHeight = mLabelView.getLabelHeight();
                                int[] labelDimen = new int[]{labelViewWidth, labelViewHeight};
                                Matrix labelBodyMatrix = mLabelView.getBodyMatrix();
                                mBubbleImageView.updateTextBox(mLabelShape, mCurrentAttr, labelDimen, labelBodyMatrix);
                            }
                        } else if(mLabelShape >= ImageEditConstants.LABEL_TITLE_SHAPE) {
                            mLabelShape = mCurrentSelectedBalloonStyle;
                            if(mBubbleImageView != null) {
                                mBubbleImageView.updateTextBox(mLabelShape, mCurrentAttr, null, null);
                            }

                        }
                        //anew to select the color or label style by updating the box, need to reset the variable.
                        if(mBubbleImageView != null) {
                            mBubbleImageView.setCurrentLabelShape(mLabelShape);
                            mBubbleImageView.setCurrentViewAttributes(mCurrentAttr);
                        }
                    } else {
                        if(mCurrentAttr.getDrawText() == null){
                            mCurrentAttr.setDrawText(mEditText.getText().toString());
                        }
                        showLabelOnPreview(mCurrentAttr, 0, mViewTag);
                    }
                    if(true == mInsertTitleIntoDB) {
                        refreshTabLabelItem(false);
                    }
                }
                dismissSubMenuGridView();
                break;
            case R.id.edit_view_back:
                if (mEditText != null) {
                    ((InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE))
                            .hideSoftInputFromWindow(mEditText.getWindowToken(),
                                    InputMethodManager.HIDE_NOT_ALWAYS);
                }
            case R.id.label_hide_view:
            case R.id.layout_view_back:
                if(mLabelDeleteVisible) {
                    mLabelDeleteVisible = false;
                }
                if(mLabelLayout != null && mLabelLayout.isShown()) {
                    mLabelLayout.setVisibility(View.GONE);
                    mEditText.setText("");
                    mFromLabelModify = false;
                    if(true == mInsertTitleIntoDB && mViewTag == ImageEditConstants.LABEL_TAG_TITLE) {
                        refreshTabLabelItem(false);
                    }
                }
                dismissSubMenuGridView();
                break;
            case R.id.brush_view_back:
            case R.id.brush_hide_view:
                if(mBrushRootLayout != null && mBrushRootLayout.isShown()) {
                    mBrushRootLayout.setVisibility(View.GONE);
                    if(true == mInsertBrushIntoDB) {
                        refreshBrushItem(false);
                    }
                    if(isLongClickStatus){
                        isLongClickStatus = false;
                    }
                }
                break;
            /*case R.id.edit_preview_save_as:
                if(mCurrentAttr != null && mTitleSaveAsDatabase != null && mEditText.getText().toString().length() > 0) {
                    if(!mTitleSaveAsDatabase.isOpened()) {
                        mTitleSaveAsDatabase = new TitleSaveAsDatabase(this);
                    }
                    mCurrentAttr.setDrawText(mEditText.getText().toString());
                    mCurrentAttr.setTitleId(System.currentTimeMillis());
                    if(true == mTitleSaveAsDatabase.insertTitle(mCurrentAttr)) {
                        ImageEditOperationUtil.showToast(ImageEditControlActivity.this, R.string.edit_title_save_as_toast, Toast.LENGTH_LONG);
                        mInsertTitleIntoDB = true;
                    }
                }
                break;*/
            case R.id.edit_brush_title_ok:
                if(isLongClickStatus){
                    isLongClickStatus = false;
                }
                // save current brush
                if(mTitleSaveAsDatabase != null) {
                    if(!mTitleSaveAsDatabase.isOpened()) {
                        mTitleSaveAsDatabase = new TitleSaveAsDatabase(this);
                    }
                    if(true == mTitleSaveAsDatabase.insertBrush(System.currentTimeMillis(), mBrushStyle, (int)mBrushSize,
                            String.valueOf(mBrushColor), BrushConstant.BrushModeSelected)) {
                        ImageEditOperationUtil.showToast(ImageEditControlActivity.this, R.string.edit_brush_save_as_toast, Toast.LENGTH_LONG);
                        mInsertBrushIntoDB = true;
                    }
                }
                if(mEraserLayout != null && mEraserLayout.isShown()) {
                    mEraserLayout.setVisibility(View.GONE);
                }
                // save finish
                if(mBrushRootLayout != null && mBrushRootLayout.isShown()) {
                    mBrushRootLayout.setVisibility(View.GONE);
                }
                mBrushMode = BrushConstant.BrushModeSelected;
                if(mBrushPainting != null) {
                    mBrushPainting.setBrushStyle(mBrushStyle);
                }
                resetBrushColor();
                if(mBrushPainting != null) {
                    mBrushPainting.setBrushColor(mBrushColor);
                    mBrushPainting.setBrushSize(mBrushSize);
                    mBrushPainting.setBrushMode(mBrushMode);
                    mBrushPainting.setAlpha(255); //255: default
                }


                if(true == mInsertBrushIntoDB) {
                    refreshBrushItem(false);
                }
                break;
            /*case R.id.edit_brush_save_as:
                if(mTitleSaveAsDatabase != null) {
                    if(!mTitleSaveAsDatabase.isOpened()) {
                        mTitleSaveAsDatabase = new TitleSaveAsDatabase(this);
                    }
                    if(true == mTitleSaveAsDatabase.insertBrush(System.currentTimeMillis(), mBrushStyle, (int)mBrushSize,
                            String.valueOf(mBrushColor), BrushConstant.BrushModeSelected)) {
                        ImageEditOperationUtil.showToast(ImageEditControlActivity.this, R.string.edit_brush_save_as_toast, Toast.LENGTH_LONG);
                        mInsertBrushIntoDB = true;
                    }
                }
                break;*/
/*            case R.id.imageview_setting_op_hint:
//                dismissMenuView();
                break;*/
            case R.id.imageview_setting_op_hint_out:
//                showMenuView();
                break;
            case R.id.operate_ok:
                if(mCurrentFuncMode == ACTION_DECORATION){
                    addDecorationBitmap();
                }else if(mCurrentFuncMode == ACTION_GRAFFITI){
                    addGraffitiBitmap();
                }else if(mCurrentFuncMode == ACTION_EDIT_TAB_LABEL){
                    addLabelBitmap();
                }else if(mCurrentFuncMode == ACTION_EDIT_MAKEUP){
                    addMakeupBitmap(false);
                }else if(mCurrentFuncMode == ACTION_EDIT_TONE){
                    addToneBitmap();
                } else if(mCurrentFuncMode == ACTION_BASIC_EDIT_FUNCTION) {
                    if(mCurrentBasicEditIndex == CROP_INDEX ) {
                        Activity act = manager.getActivity(ImageEditConstants.BASIC_EDIT_FUNCTION_ACTION_CROP);
                        if (act != null && act instanceof CropImage) {
                            setCurrentBitmap(((CropImage)act).getCropBitmap(true));

                            CropImage.sConfirmBitmap = null;
                        }
                        mCurrentBasicEditIndex = -1;
                    }
                } else if(mCurrentFuncMode == ACTION_MOSAIC) {
                    addMosaicBitmap();
                    mMosaicEditView.clear();
                }
                dismissSubMenuGridView();
                updateImageEditBitmap(getCurrentBitmap());
                onPreviewChanged();
                mCurrentBitmap = null;
                mSubMenuSelectedIndex = -1;
                break;
            case R.id.operate_cancel:
                if(mCurrentFuncMode == ACTION_EDIT_MAKEUP){
                    addMakeupBitmap(false);
                } else if (mCurrentFuncMode == ACTION_BASIC_EDIT_FUNCTION) {
                    if(mCurrentBasicEditIndex == CROP_INDEX ) {
                        Activity act = manager.getActivity(ImageEditConstants.BASIC_EDIT_FUNCTION_ACTION_CROP);
                        if (act != null && act instanceof CropImage) {
                            Bitmap bitmap = ((CropImage)act).getCropBitmap(false);
                            if (bitmap != getCurrentBitmap()) {
                                Utils.recyleBitmap(bitmap);
                            }
                        }
                        mCurrentBasicEditIndex = -1;
                    }
                } else if(mCurrentFuncMode == ACTION_MOSAIC && mMosaicEditView != null) {
                    mMosaicEditView.clear();
                }
                dismissSubMenuGridView();
                    if(mCurrentBitmap != null){
                        mCurrentBitmap.recycle();
                        mCurrentBitmap = null;
                    }
                    mSubMenuSelectedIndex = -1;
                    onPreviewChanged();

                break;
            case R.id.btn_edit_undo:
                if(mCurrentFuncMode == ACTION_DECORATION){
                    if(mDecorationUndoRunnable != null) {
                        mDecorationUndoRunnable.run();
                    }
                }else{
                    if(unDoRunnable != null) {
                        unDoRunnable.run();
                    }
                }
                break;
            case R.id.btn_edit_redo:
                if(mCurrentFuncMode == ACTION_DECORATION) {
                    if(mDecorationRunnable != null) {
                        mDecorationRedoRunnable.run();
                    }
                }else {
                    if(reDoRunnable != null) {
                        reDoRunnable.run();
                    }
                }
                break;
            case R.id.btn_edit_idphotoback:
                showDialog();
                break;
            case R.id.btn_edit_photo_type:
                break;
            case R.id.makeup_adjust:
                makeUpAdjust();
                break;
            case R.id.btn_draw_mosaic:
                if(mMosaicEditView != null) {
                    mMosaicEditView.setMosaicType(MosaicConstant.MOSAIC_DRAW);
                    mMosaicDrawBtn.setSelected(true);
                    mMosaicEraseBtn.setSelected(false);
                }
                break;
            case R.id.btn_erase_mosaic:
                if(mMosaicEditView != null) {
                    mMosaicEditView.setMosaicType(MosaicConstant.MOSAIC_ERASE);
                    mMosaicDrawBtn.setSelected(false);
                    mMosaicEraseBtn.setSelected(true);
                }
                break;
            case R.id.btn_mosaic_grid:
//                showSubMenuGridView();
                if(mMosaicGridView.getVisibility() == View.VISIBLE) {
                    mMosaicGridView.setVisibility(View.GONE);
                    mMosaicScrollView.setVisibility(View.GONE);
                } else {
                    mMosaicGridView.setVisibility(View.VISIBLE);
                    mMosaicScrollView.setVisibility(View.VISIBLE);
                }
                break;
            case R.id.bt_idphoto_shareandstorage:
                if(shareRunnable != null){
                    shareRunnable.run();
                }
            break;
        }

        if(reDoBtn.isEnabled() || unDoBtn.isEnabled()){
            saveBtn.setEnabled(true);
            mLayoutSave.setEnabled(true);
        }
    }
    private void showDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(ImageEditControlActivity.this)
        .setTitle(R.string.text_edit_exit_tip_title)
        .setMessage(getString(R.string.text_idphoto_exit_tip_message))
        .setPositiveButton(R.string.text_idphoto_exit_yes, new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
                finish();
            }
        })
        .setNegativeButton(R.string.text_idphoto_exit_no, new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        })
        ;
        builder.create().show();
    }
    private void pickImage(int requestCode) {
        com.ucamera.ugallery.ImageGallery.showImagePicker(this, requestCode, 1, com.ucamera.ugallery.ViewImage.IMAGE_ENTRY_UPHOTO_VALUE);
    }

    private void captureImage(int requestCode) {
        if(!new File(tempPath).exists()) {
            new File(tempPath).mkdirs();
        }

        mCaptureFilePath = tempPath + "/" + getPhotoFileName();
        Intent intentCamera = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
        intentCamera.putExtra(MediaStore.EXTRA_OUTPUT, Uri.fromFile(new File(mCaptureFilePath)));
        intentCamera.putExtra("uphoto", true);
        //fixed the bug6348
        try {
            startActivityForResult(intentCamera, requestCode);
        } catch(ActivityNotFoundException exception) {
            exception.printStackTrace();
            Toast.makeText(ImageEditControlActivity.this, R.string.text_activity_is_not_found, Toast.LENGTH_SHORT).show();
        }
    }

    private void gotoGraffiti() {
        String  saveImagePath =
                mSharedPreferences.getString(Const.KEY_UCAM_SELECT_PATH,
                                        ImageManager.getCameraUCamPath());

        mFromGraffiti = true;
        mShowCanvas = true;
        mLoginView.setVisibility(View.GONE);
        mAdjustSclControl = new AdjustSclControl(this);
        mAdjustSclControl.initControl(null, this, mHandler, mImageEditDesc);

        mIsOpenPhoto = false;
        warnMessageForOpenPhoto();

        Bitmap bitmap = createGraffitiBitmap();
        if(bitmap == null) {
            ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
            return;
        }
        bitmap.eraseColor(Color.WHITE);
        mImageEditDesc.setImageEditedPath(saveImagePath + "/" + getPhotoFileName());
        mImageEditDesc.setOriginalUri(null);
        mImageEditDesc.setBitmap(bitmap);
        mImageEditDesc.setOperationHandler(mHandler);

        //mGraffitiBtn.setChecked(true);
        mGraffitiRunnable.run();

//        ImageEditViewGraffitiView graffitiView = new ImageEditViewGraffitiView(this);
//        graffitiView.setBottomLayer(onPreviewChanged());
//        if(mBrushPainting == null) {
//            reinitBrushPainting();
//        }
//        graffitiView.setGraffitiDimens(centerLayout.getWidth(), centerLayout.getHeight(), true, mBrushPainting);
//        centerLayout.addView(graffitiView);
    }

    private Bitmap createGraffitiBitmap() {
        Bitmap bitmap = null;
        int centerWidth = centerLayout.getWidth();
        int centerHeight = centerLayout.getHeight();
        Log.d(TAG, "createGraffitiBitmap(): centerWidth is " + centerWidth + ", centerHeight is " + centerHeight);
        try {
            if(centerWidth > 0 && centerHeight > 0) {
                bitmap = Bitmap.createBitmap(centerWidth, centerHeight, Config.ARGB_8888);
            } else if(TextUtils.isEmpty(mPhotoPick)) {
                Pair<Integer, Integer> pair = getGraffitiHW();
                bitmap = Bitmap.createBitmap(pair.first, pair.second, Config.ARGB_8888);
            }
        } catch(OutOfMemoryError oom) {
            Log.w(TAG, "createGraffitiBitmap(): code has a memory leak is detected...");
        }

        return bitmap;
    }

    private String getPhotoFileName() {
        Date date = new Date(System.currentTimeMillis());
        SimpleDateFormat dateFormat = new SimpleDateFormat("'IMG'_yyyyMMdd_HHmmss",Locale.US);
        return dateFormat.format(date) + ".jpg";
    }

    private Bitmap composeTextBubbleBitmap() {
        Bitmap bitmap = null;
        try {
            int childCount = centerLayout.getChildCount();
            for(int i = 1; i < childCount; i++){
                View view = centerLayout.getChildAt(i);
                if(view instanceof ScrollController) {
                    view = ((ScrollController) view).getChildAt(0);
                }

                if(view instanceof MyTextBubbleImageView){
                    MyTextBubbleImageView textBubbleView = (MyTextBubbleImageView) view;
                    bitmap = textBubbleView.composeTextBubbleBitmap();

                    return bitmap;
                }
            }
        } catch (OutOfMemoryError e) {
            e.printStackTrace();
        }

        return bitmap;
    }

    private Bitmap composeDecorBitmap() {
        /* FIX BUG: 5206
         * BUG CAUSE: NullPointerException of ImageEditControlActivity
         * DATE: 2013-11-04
         */
        if (centerLayout == null || centerLayout.getChildCount() ==0) return null;
        /* SPRD: fix bug540954 have a ClassCastException @{ */
        if (!(centerLayout.getChildAt(0) instanceof ImageEditViewPreview.ScrollController)) {
            return null;
        }
        /* @} */
        ImageEditViewPreview.ScrollController scroller = (ScrollController) centerLayout.getChildAt(0);
        if (scroller.getChildCount() == 0) return null;
        ImageEditViewPreview preview = (ImageEditViewPreview) scroller.getChildAt(0);
        Bitmap previewBitmap = getCurrentBitmap();
        if(previewBitmap == null){
            return null;
        }
        int width = previewBitmap.getWidth();
        int height = previewBitmap.getHeight();
        Bitmap bitmap = null;
        try {
            do {
                try {
                    /* fix bug 6101
                     * use constant config if previewBitmap.getConfig()== null
                     */
                    Config config = previewBitmap.getConfig();
                    bitmap = Bitmap.createBitmap(width, height, (config != null ? config : Config.ARGB_8888));
                } catch(OutOfMemoryError oom) {
                    Log.w(TAG, "composeDecorBitmap(): code has a memory leak is detected...");
                }
                if(bitmap == null) {
                    /* FIX BUG: 4051
                     * BUG CAUSE: can not add decoration when the system memory not sufficient
                     * FIX COMMENT: clear undo stack and recycle history bitmap when system memory is too low
                     * DATE: 2013-05-27
                     */
                    if(reorganizeUndoStack() < 2) {
                        ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                        return null;
                    }
                }
            } while (bitmap == null);

            Canvas c = new Canvas(bitmap);
            Paint p = new Paint();
            c.drawBitmap(previewBitmap, new Matrix(), p);

            RectF rectF = new RectF(centerLayout.getLeft(), centerLayout.getTop(), centerLayout.getRight(), centerLayout.getBottom());
            rectF = ImageEditOperationUtil.resizeRectF(previewBitmap, centerLayout.getWidth(), centerLayout.getHeight());
            int childCount = centerLayout.getChildCount();
            for(int i = 1; i < childCount; i++){
                View view = centerLayout.getChildAt(i);
                if(view instanceof ImageEditViewDecorView){
                    ImageEditViewDecorView decorView = (ImageEditViewDecorView) view;
                    Bitmap decorBitmap = decorView.getBitmap();
                    if(decorBitmap == null){
                        return null;
                    }
                    Matrix decorMatrix = new Matrix();
                    boolean flag = false;
//                    float scaleW = 1f;
//                    float scaleH = 1f;
//                    if(previewBitmap.getWidth() > centerLayout.getWidth() || previewBitmap.getHeight() > centerLayout.getHeight()) {
//                        scaleW = previewBitmap.getWidth() / rectF.width();
//                        scaleH = previewBitmap.getHeight() / rectF.height();
//                    }
                    Matrix m = preview.getImageMatrix();
                    if(m == null){
                       return null;
                    }
                    float[] f = new float[9];
                    m.getValues(f);
                    float decorX = decorView.getOffsetX() - f[2];
                    float decorY = decorView.getOffsetY() - f[5];
                    decorMatrix.setRotate(decorView.getRoateDegree());
                    decorMatrix.postScale(decorView.getScaleX(), decorView.getScaleY());
                    decorMatrix.postTranslate(decorX, decorY);
                    /* FIX BUG: 5814
                     * FIX COMMENT: same to decorMatrix.postScale(1 / preview.getScale(), 1 / preview.getScale());
                     * DATE: 2014-03-12
                     */
//                    decorMatrix.postScale(scaleW, scaleH);
                    decorMatrix.postScale(1 / preview.getScale(), 1 / preview.getScale());
                    c.drawBitmap(decorBitmap, decorMatrix, p);
                }
            }
        } catch (OutOfMemoryError e) {
            e.printStackTrace();
        }
        return bitmap;
    }

    private int[] getTargetDimen(BitmapFactory.Options options) {
        int[] targetDimen = new int[2];
        String targetSize = mSharedPreferences.getString(ImageEditConstants.PREF_UPHOTO_PICTURE_SIZE_KEY, getString(R.string.text_uphoto_picture_size_default_value));
        targetSize = targetSize.trim();
        int dimIndex = targetSize.indexOf("x");
        int newWidth = 0;
        int newHeight = 0;
        if(dimIndex < 0) {
            Log.w(TAG, "dimIndex less than zero, bad target picture size: " +  targetSize);
            newWidth = ImageEditConstants.EDIT_VIEW_SIZE_SHORT;
            newHeight = ImageEditConstants.EDIT_VIEW_SIZE_LONG;
        } else {
            try {
                //1280x720, 800x600, 640x480
                newWidth = Integer.parseInt(targetSize.substring(dimIndex + 1)); //short 720/600/480
                newHeight = Integer.parseInt(targetSize.substring(0, dimIndex)); //long  1280/800/640
            } catch (NumberFormatException nfe) {
                Log.w(TAG, "NumberFormatException, bad target picture size: " + targetSize);
                newWidth = ImageEditConstants.EDIT_VIEW_SIZE_SHORT;
                newHeight = ImageEditConstants.EDIT_VIEW_SIZE_LONG;
            }
        }

        targetDimen[0] = newWidth;
        targetDimen[1] = newHeight;
        if(options.outWidth > options.outHeight) {
            targetDimen[0] = newHeight;
            targetDimen[1] = newWidth;
        }

        return targetDimen;
    }

    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        /** FIX BUG: 6531
          * BUG CAUSE: the uri has problem.
          * FIX COMMENT: reorganize the logic
          * Date: 2011-11-30
          */
        Log.d(TAG, "onActivityResult(): data = " + data + ", requestCode = " + requestCode + ", resultCode = " + resultCode);
        if (resultCode != RESULT_OK){
            return;
        }

        Uri target = null;
        int degree = 0;
        if(requestCode == REQUEST_CODE_CAMERA || requestCode == REQUEST_CODE_MAKEUP_CAMERA){
            if(mCaptureFilePath != null) {
                File file = new File(mCaptureFilePath);
                if(file.exists() && file.length() > 0) {
                    target = Uri.fromFile(file);
                    degree = calculateAngle(mCaptureFilePath);
                    mImageEditDesc.setImageEditedPath(mCaptureFilePath);
                } else {
                    Log.d(TAG,"Fail capture image");
                }
            }
        } else if(requestCode == REQUEST_CODE_PICK || requestCode == REQUEST_CODE_MAKEUP_PICK) {
            Parcelable[] obj = data.getParcelableArrayExtra(com.ucamera.ugallery.ImageGallery.INTENT_EXTRA_IMAGES);
//            Parcelable[] obj = null;
            if (obj != null) {
                Uri[] images = new Uri[obj.length];
                System.arraycopy(obj, 0, images, 0, obj.length);
                target = images[0];
                String scheme = target.getScheme();
                if("file".equals(scheme)) {
                    mCaptureFilePath = target.getPath();
                } else if("content".equals(scheme)) {
                    mCaptureFilePath = getDefaultPathAccordUri(target);
                }
                degree = calculateAngle(mCaptureFilePath);
                mImageEditDesc.setImageEditedPath(mCaptureFilePath);
            }
        }
        Log.d(TAG, "onActivityResult(): target = " + target + ", degree = " + degree);
        /**
         * FIX BUG: 6831
         * BUG CAUSE: re-select the picture, the current fun mode is not restored
         * FIX COMMENT: restore the current fun mode value
         * Date: 2012-01-13
         */
        mCurrentItemIndex = -1;
        mCurrentArgbSelect = -1;
        Bitmap photoBitmap = null;
        Bitmap bitmap = null;
        //if target is null, the means that the source of the picture is generated by taking pictures,
        //else the picture is from Gallery.
        /**
         * FIX BUG: 954
         * BUG CAUSE: when do not get the target picture, use an empty data.
         * FIX COMMENT: judgment the null pointer.
         * DATE: 2012-04-27
         */
        if(target == null && data != null) {
            photoBitmap = data.getParcelableExtra("data");
            Log.d(TAG, "onActivityResult(): photoBitmap = " + photoBitmap);
        }
        Bitmap tempBitmap = null;
        if(target != null) {
            try {
                ParcelFileDescriptor pfd = this.getContentResolver().openFileDescriptor(target, "r");
                FileDescriptor fd = pfd.getFileDescriptor();
                BitmapFactory.Options options = Utils.getNativeAllocOptions();
                options.inSampleSize = 1;
                options.inJustDecodeBounds = true;
                BitmapFactory.decodeFileDescriptor(fd, null, options);

                int[] targetDimen = getTargetDimen(options);
                int inSampleSize = ImageEditOperationUtil.computeSampleSize(options, targetDimen[0], targetDimen[1]);
                Log.d(TAG, "initData(): inSampleSize = " + inSampleSize);
                options.inJustDecodeBounds = false;
                if(inSampleSize > 1){
                    options.inSampleSize = inSampleSize;
                    tempBitmap = BitmapFactory.decodeFileDescriptor(fd, null, options);
                    Log.d(TAG, "initData(): options.outWidth:" + options.outWidth + ",  options.outHeight: " + options.outHeight);
                }else{
                    tempBitmap = BitmapFactory.decodeFileDescriptor(fd, null, options);
                }

                tempBitmap = ImageEditOperationUtil.computeSuitableBitmap(tempBitmap, options, targetDimen[0], targetDimen[1]);
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } catch(OutOfMemoryError oom) {
                oom.printStackTrace();
            } catch (Exception e) {
                e.printStackTrace();
            }
        } else if(photoBitmap != null) {
            tempBitmap = photoBitmap;
            mImageEditDesc.setImageEditedPath(mCaptureFilePath);
        }
        mImageEditDesc.setRotation(degree);
        mImageEditDesc.setOriginalUri(target);
        mAdjustSclControl = new AdjustSclControl(this);
        mAdjustSclControl.initControl(null, this, mHandler, mImageEditDesc);
        if(tempBitmap != null) {
            if(degree != 0){
                bitmap = ImageEditOperationUtil.rotate(tempBitmap, degree);
                if(bitmap != null && !bitmap.equals(tempBitmap)){
                    tempBitmap.recycle();
                    tempBitmap = null;
                }
            }else{
                bitmap = tempBitmap;
            }
            if(bitmap != null) {
                mIsOpenPhoto = false;
                warnMessageForOpenPhoto();
                mImageEditDesc.setBitmap(bitmap);
                mImageEditDesc.setOperationHandler(mHandler);
                ImageEditViewPreview preview = onPreviewChanged();
                //mEffectOriginJpegData = ImageEditOperationUtil.transformBitmapToBuffer(mImageEditDesc.getBitmap());
                getEffectOriginBmp();
                Log.d(TAG, "onActivityResult(): mCurrentFuncMode = " + mCurrentFuncMode);
                if(mCurrentFuncMode == ACTION_GRAFFITI) {
                    final ImageEditViewGraffitiView graffitiView = new ImageEditViewGraffitiView(ImageEditControlActivity.this);
                    centerLayout.addView(graffitiView);
                    graffitiView.setBottomLayer(preview);
                    if(mBrushPainting == null) {
                        reinitBrushPainting();
                    }
                    graffitiView.setGraffitiDimens(centerLayout.getWidth(), centerLayout.getHeight(), true, mBrushPainting);
                }
                if(requestCode == REQUEST_CODE_MAKEUP_CAMERA || requestCode == REQUEST_CODE_MAKEUP_PICK) {
                    //mMakeupBtn.setChecked(true);
                    mMakeupRunnable.run();
                }
            } else {
                ImageEditOperationUtil.showToast(this, R.string.edit_open_error_image, Toast.LENGTH_SHORT);
            }
        } else {
            ImageEditOperationUtil.showToast(this, R.string.edit_open_error_image, Toast.LENGTH_SHORT);
        }
    }

    private boolean isDecorating;
 // NO.1 save runnable
    private Runnable saveRunnable = new Runnable() {
        public void run() {
            ImageEditOperationUtil.startBackgroundJob(ImageEditControlActivity.this, "aaaa",
                    getString(R.string.text_waiting), new Runnable() {
                public void run() {
                    printLog(TAG, "saveRunnable(): ENTRY.");
                    mSaving = true;
                    mCurrentItemIndex = -1;
                    setIsUnDoOrReDo(false);
                    if((mCurrentFuncMode == ACTION_EDIT_TAB_LABEL
                            || mCurrentFuncMode == ACTION_EDIT_TEXT
                            || mCurrentFuncMode == ACTION_TEXT_BUBBLE)
                            && centerLayout.getChildCount() > 1) {
//                        bitmap = composeTextBubbleBitmap();
//                        updateImageEditBitmap(bitmap);
                        addLabelBitmap();
                    }
                    // finish the label
//                    mHandler.sendEmptyMessage(ImageEditConstants.UPDATE_LABEL_BITMAP);
                    if (mAdjustProcess) {
//                        mAdjustSclControl.saveBitmap();
                        mHandler.sendEmptyMessage(ImageEditConstants.INVISIBLE_ADJUST_CONTROL);
                    }

                    Bitmap bitmap = mImageEditDesc.getBitmap();
                    if (bitmap == null) return;
                    /* SPRD: CID 109154 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE) @{
                    int height = bitmap.getHeight();
                    int width = bitmap.getWidth();
                    @} */
                    String imageEditPath =getSaveImagePath();
                    if( TextUtils.isEmpty(imageEditPath) ) return;

                    String fileName = imageEditPath.substring(imageEditPath.lastIndexOf("/") + 1);
                    Pair<Uri,String> paths= ImageEditOperationUtil.saveOutput(
                                                    ImageEditControlActivity.this,
                                                    mImageEditDesc.getBitmap(),
                                                    imageEditPath,
                                                    fileName);
                    if(paths != null){
                        Message msg = new Message();
                        msg.what = ImageEditConstants.ACTION_SAVED_SUCUESS;
                        Log.d(TAG, "saveRunnable(): ACTION_SAVED_SUCUESS...");
                        msg.obj = paths.second;
                        mHandler.sendMessage(msg);
                        mImageEditDesc.setEditImageUri(paths.first);
                    }else{
                        mHandler.sendEmptyMessage(ImageEditConstants.ACTION_SAVED_FAILED);
                    }
                    printLog(TAG, "saveRunnable(): LEAVE.");
                }
            }, mHandler);
        }
    };

    private String getDefaultPathAccordUri(Uri uri) {
        /*
         * FIX BUG: 1454
         * BUG CAUSE: uri may be null;
         * DATE: 2012-08-13
         */
        if(uri == null) {
            return null;
        }
        String imageData = null;
        final String[] IMAGE_PROJECTION = new String[] {Media.DATA};
        Cursor cursor = getContentResolver().query(uri, IMAGE_PROJECTION, null, null, null);
        if(cursor != null) {
            try {
                if(cursor.moveToFirst()) {
                    imageData = cursor.getString(cursor.getColumnIndex(Media.DATA));
                }
            } finally {
                cursor.close();
            }
        }

        return imageData;
    }

    private void previewProcess() {
        if (mAdjustProcess) {
            inVisibleAdjustControlProcess();
        }
        else {
            onPreviewChanged();
        }
    }

 // NO.2 undo runnable
    private Runnable unDoRunnable = new Runnable() {
        public void run() {
            printLog(TAG, "unDoRunnable(): ENTRY.");
            Bitmap bitmap = null;
            setIsUnDoOrReDo(true);
            updateLabelBitmap();
            if(centerLayout.getChildCount() > 1 && mCurrentFuncMode == ACTION_DECORATION){
                bitmap = composeDecorBitmap();
                while(bitmap == null){
                    //index is one, means that any operation can not be Completed
                    if(mImageEditDesc.reorganizeQueue() < 2) {
                        ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                        return;
                    }
                    bitmap = composeDecorBitmap();
                }
                if(bitmap != null){
                    //updateImageEditBitmap(bitmap);
                    setCurrentBitmap(bitmap);
                    ImageEditViewPreview.ScrollController scroller = (ScrollController) centerLayout.getChildAt(0);
                    ImageEditViewPreview preview = (ImageEditViewPreview) scroller.getChildAt(0);
                    preview.setImageBitmap(mImageEditDesc.getBitmap());
                }
            }else if(mCurrentFuncMode == ACTION_GRAFFITI && centerLayout.getChildCount() > 1){
                Log.d(TAG, "ImageEditControlActivity.unDoRunnable.run(): strokeCount = " + mBrushPainting.getStrokeCount()
                        + ", bitmapIndex = " + mImageEditDesc.getCurrentBitmapIndex() + ", stackCount = " + mBrushPainting.getStackCount());
                if(mBrushPainting.getStrokeCount() > 0) {
                    mBrushPainting.undoStroke();
                    if(!mOperateDoneLayout.isShown()){
                        reDoBtn.setEnabled(true);
                        mLayoutRedo.setEnabled(true);
                    }
                    if(mBrushPainting.getStrokeCount() == 0 && mImageEditDesc.getCurrentBitmapIndex() < 1) {
                        unDoBtn.setEnabled(false);
                        mLayoutUndo.setEnabled(false);
                    }
                } else {
                    ImageEditViewPreview.ScrollController scroller = (ScrollController) centerLayout.getChildAt(1);
                    ImageEditViewGraffitiView graffitiView = (ImageEditViewGraffitiView) scroller.getChildAt(0);
                    ImageEditViewPreview.ScrollController privateScroller = (ScrollController) centerLayout.getChildAt(0);
                    ImageEditViewPreview preview = (ImageEditViewPreview) privateScroller.getChildAt(0);
//                    if(graffitiView.isSaved()){
//                        Bitmap bitmap = composeGraffitiBitmap(false);
//                        mImageEditDesc.updateBitmap(bitmap);
//                        preview.setImageBitmap(mImageEditDesc.getBitmap());
//                    }
                    mImageEditDesc.executeUndo();
                    preview.setImageBitmap(mImageEditDesc.getBitmap());
                    if(mBrushPainting == null) {
                        reinitBrushPainting();
                    }
                    graffitiView.setGraffitiDimens(centerLayout.getWidth(), centerLayout.getHeight(), mBrushPainting);
                    if(getUnDoOrReDo()) {
                        preview.mSuppMatrix.reset();
                        graffitiView.mSuppMatrix.reset();
                    }
                }

                return;
            } else if(centerLayout.getChildCount() > 1 && (mCurrentFuncMode == ACTION_EDIT_TAB_LABEL)) {
                bitmap = composeTextBubbleBitmap();
                while(bitmap == null){
                    //index is one, means that any operation can not be Completed
                    if(mImageEditDesc.reorganizeQueue() < 2) {
                        ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                        return;
                    }
                    bitmap = composeTextBubbleBitmap();
                }
                if(bitmap != null){
                    updateImageEditBitmap(bitmap);
                    ImageEditViewPreview.ScrollController scroller = (ScrollController) centerLayout.getChildAt(0);
                    ImageEditViewPreview preview = (ImageEditViewPreview) scroller.getChildAt(0);
                    preview.setImageBitmap(mImageEditDesc.getBitmap());
                }
            } else if(mCurrentFuncMode == ACTION_EDIT_MAKEUP) {
//                int makeupCount = mMakeupControlView.getMakeupCount();
//                if(makeupCount > 0) {
//                    mMakeupControlView.undoMakeup();
//                    reDoBtn.setEnabled(true);
//                    mLayoutRedo.setEnabled(true);
//                    if(mMakeupControlView.getMakeupCount() == 0 && mImageEditDesc.getCurrentBitmapIndex() < 1) {
//                        unDoBtn.setEnabled(false);
//                        mLayoutUndo.setEnabled(false);
//                    }
//                    return;
//                }
            }
            removeHightlight();

            if(mImageEditDesc.executeUndo()){
                itemPositionSelected = -1;
                if (mCurrentFuncMode == ACTION_EDIT_EFFECT) {
                    mCurrentItemIndex = -1;
//                    mEffectOriginJpegData = ImageEditOperationUtil
//                    .transformBitmapToBuffer(mImageEditDesc.getBitmap());
                    getEffectOriginBmp();
                } else if (mCurrentFuncMode == ACTION_PHOTO_FRAME  || mCurrentFuncMode == ACTION_TEXTURE) {
                    mCurrentArgbSelect = -1;
                    getArgbOriginData();
                }
//                else if(mCurrentFuncMode == ACTION_EDIT_MAKEUP) {
//                    resetMakeupParam(false);
//                    return;
//                }
                previewProcess();
            }
            printLog(TAG, "unDoRunnable(): LEAVE.");
        }
    };

 // NO.3 redo runnable
    private Runnable reDoRunnable = new Runnable() {
        public void run() {
            printLog(TAG, "reDoRunnable(): ENTRY.");
            setIsUnDoOrReDo(true);
            if(mImageEditDesc.executeRedo()){
                if (mCurrentFuncMode == ACTION_EDIT_EFFECT) {
                    mCurrentItemIndex = -1;
//                    mEffectOriginJpegData = ImageEditOperationUtil
//                    .transformBitmapToBuffer(mImageEditDesc.getBitmap());
                    getEffectOriginBmp();
                }
                if (mCurrentFuncMode == ACTION_PHOTO_FRAME || mCurrentFuncMode == ACTION_TEXTURE) {
                    mCurrentArgbSelect = -1;
                    getArgbOriginData();
                }

//                if(mCurrentFuncMode == ACTION_EDIT_MAKEUP) {
//                    resetMakeupParam(false);
//                    return;
//                }
                itemPositionSelected = -1;
                previewProcess();
            } else {
                if(mCurrentFuncMode == ACTION_GRAFFITI){
                    if(mBrushPainting.hasStrokesInStack()) {
                        mBrushPainting.redoStroke();
                        unDoBtn.setEnabled(true);
                        mLayoutUndo.setEnabled(true);
                        if(!mBrushPainting.hasStrokesInStack()) {
                            reDoBtn.setEnabled(false);
                            mLayoutRedo.setEnabled(false);
                        }
                        return;
                    }
                } else if(mCurrentFuncMode == ACTION_EDIT_MAKEUP) {
//                    if(mMakeupControlView.hasMakeupItemInStack()) {
//                        mMakeupControlView.redoMakeup();
//                        unDoBtn.setEnabled(true);
//                        mLayoutUndo.setEnabled(true);
//                        if(!mMakeupControlView.hasMakeupItemInStack()) {
//                            reDoBtn.setEnabled(false);
//                            mLayoutRedo.setEnabled(false);
//                        }
//                    }
                }
            }
            printLog(TAG, "reDoRunnable(): LEAVE.");
        }
    };

 // NO.4 share runnable
    private Runnable shareRunnable = new Runnable() {
        public void run() {
            ImageEditOperationUtil.startBackgroundJob(ImageEditControlActivity.this, "loading", getString(R.string.text_waiting), new Runnable() {
                public void run() {
                    printLog(TAG, "shareRunnable(): ENTRY.");
                    mSaving = true;
                    Bitmap bitmap = null;
                    mCurrentItemIndex = -1;
                    setIsUnDoOrReDo(false);
                    if(mCurrentFuncMode == ACTION_DECORATION && centerLayout != null && centerLayout.getChildCount() > 1){
                        bitmap = composeDecorBitmap();
                        updateImageEditBitmap(bitmap);
                    } else if((mCurrentFuncMode == ACTION_EDIT_TAB_LABEL ||
                            mCurrentFuncMode == ACTION_EDIT_TEXT ||
                            mCurrentFuncMode == ACTION_TEXT_BUBBLE) && centerLayout.getChildCount() > 1) {
                        bitmap = composeTextBubbleBitmap();
                        updateImageEditBitmap(bitmap);
                    }
                    // fix bug 29289 when imageEditPath is null,we get the filename from DB
                    String imageEditPath = getSaveImagePath();
                    if( TextUtils.isEmpty(imageEditPath) ) return;
                    Log.d(TAG, "shareRunnable.run(): imageEditPath = " + imageEditPath
                            + ", saveBtn.isEnabled() = " + saveBtn.isEnabled()
                            + ", origUri = " + mImageEditDesc.getOriginalUri()
                            + ", editUri = " + mImageEditDesc.getEditImageUri());
                    /*
                     * FIX BUG: 1167
                     * BUG COMMENT: When save btn can be clicked or by clicking the Graffiti item into editting screenBy,
                     *              need to save image to generate new uri, otherwise only get the edit uri from stack,
                     *              either original uri or edit uri.
                     * DATE: 2012-07-31
                     */
                    Uri imageUri = null;
                    if(saveBtn.isEnabled() || mFromGraffiti ) {
                        String fileName = imageEditPath.substring(imageEditPath.lastIndexOf("/") + 1, imageEditPath.length());
                        Pair<Uri,String> paths = ImageEditOperationUtil.saveOutput(
                                                        ImageEditControlActivity.this,
                                                        mImageEditDesc.getBitmap(),
                                                        imageEditPath,
                                                        fileName);
                        if(paths == null){
                            mHandler.sendEmptyMessage(ImageEditConstants.ACTION_UNLOAD_FAILED);
                            return;
                        }
                        imageUri = paths.first;
                        mImageEditDesc.setEditImageUri(imageUri);
                    } else {
                        imageUri = mImageEditDesc.getEditImageUri();
                        if(imageUri == null) {
                            imageUri = mImageEditDesc.getOriginalUri();
                        }
                    }
                    Message message = new Message();
                    message.what = ImageEditConstants.ACTION_SHARE_DIALOG;
                    message.obj = imageUri;
                    mHandler.sendMessage(message);

                    printLog(TAG, "shareRunnable(): LEAVE.");
                }
            }, mHandler);
        }
    };

    // NO.5 edit runnable
    private Runnable mEditorRunnable = new Runnable() {
        public void run() {
            printLog(TAG, "mEditorRunnable(): ENTRY, and mCurrentFuncMode is " + mCurrentFuncMode);
            setIsUnDoOrReDo(false);
            if (mAdjustProcess) {
                invisibleAdjustAndSaveBitmap();
            }
            mCurrentItemIndex = -1;
            mCurrentArgbSelect = -1;
            /*
             * SPRD: Fix bug 541398 java.lang.RuntimeException: Canvas: trying
             * to use a recycled bitmap @{
             */
            if (mCurrentBitmap != null) {
                mCurrentBitmap.recycle();
                mCurrentBitmap = null;
            }
            /* @} */
            Bitmap bitmap = null;
            updateLabelBitmap();
            if(mCurrentFuncMode == ACTION_EDIT_EFFECT) {
                 setEffectCatHightlight(-1);
             }
             mCurrentFuncMode = ACTION_BASIC_EDIT_FUNCTION;
             getBasicEditAdapter();
             mSubMenuGridView.setAdapter(adapterArray.get(mEditBtn.getId()));
             onPreviewChanged();
             showSubMenuGridView();
             printLog(TAG, "mEditorRunnable(): LEAVE.");
        }
    };

    private Bitmap composeGraffitiBitmap(boolean saveToSdcard) {
        Bitmap bitmap;
        ImageEditViewPreview.ScrollController scroller = (ScrollController) centerLayout.getChildAt(1);
        ImageEditViewGraffitiView graffiti = (ImageEditViewGraffitiView) scroller.getChildAt(0);
        if(!graffiti.isSaved()){
            if(mImageEditDesc.getqueueSize() <= 1){
                unDoBtn.setEnabled(false);
                mLayoutUndo.setEnabled(false);
            }
            return null;
        }
        bitmap = graffiti.composeGraffitiBitmap();
        while(bitmap == null){
            //index is one, means that any operation can not be Completed
            if(mImageEditDesc.reorganizeQueue() < 2) {
                ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                return null;
            }
            bitmap = graffiti.composeGraffitiBitmap();
        }
        if(!saveToSdcard){
            if(mBrushPainting == null) {
                reinitBrushPainting();
            }
            graffiti.setGraffitiDimens(centerLayout.getWidth(), centerLayout.getHeight(), mBrushPainting);
        }
        return bitmap;
    }

    // NO.9 graffitiRunnable runnable
    private Runnable mGraffitiRunnable = new Runnable() {
        public void run() {
            printLog(TAG, "mGraffitiRunnable(): ENTRY and mCurrentFuncMode is " + mCurrentFuncMode);
            setIsUnDoOrReDo(false);
            updateUndoRedoState();
            if (mAdjustProcess) {
                invisibleAdjustAndSaveBitmap();
            }

            updateLabelBitmap();
            removeHightlight();

            mBrushStyles = BrushConstant.mBrushStyles;
            if(mImageBurshManager == null) {
                mImageBurshManager = new ImageBrushManager(ImageEditControlActivity.this);
            }
            if(mRandomColorPicker == null) {
                mRandomColorPicker = new RandomColorPicker(32);
            }
            if(mBrushPainting == null) {
                mBrushPainting = new BrushPainting(ImageEditControlActivity.this);
                mBrushPainting.setListener(ImageEditControlActivity.this);
            }
            if(mBrushColorPickerView != null) {
                mBrushColorPickerView.reset();
            }
            if(mBrushListAdapter != null) {
                mBrushListAdapter.updateColor(Color.RED);
            }

            randomGenerateBrush();
            mBrushMode = BrushConstant.BrushModeRandom;
            mBrushPainting.setBrushStyle(mBrushStyle);
            mBrushPainting.setBrushColor(mBrushColor);
            mBrushPainting.setBrushSize(mBrushSize);
            mBrushPainting.setBrushMode(mBrushMode);
            mBrushPainting.setAlpha(255);

            if(mTitleSaveAsDatabase == null || !mTitleSaveAsDatabase.isOpened()) {
                mTitleSaveAsDatabase = new TitleSaveAsDatabase(ImageEditControlActivity.this);
            }

            mCurrentFuncMode = ACTION_GRAFFITI;
            mGraffitiAdapter = null;
            mGraffitiHistoryAdapter = null;
            mGraffitiAdapter = getGraffitiAdapter(false);
            mSubMenuGridView.setAdapter(mGraffitiAdapter);
            ImageEditViewPreview preview = onPreviewChanged();
            final ImageEditViewGraffitiView graffitiView = new ImageEditViewGraffitiView(ImageEditControlActivity.this);
            centerLayout.addView(graffitiView);
            graffitiView.setBottomLayer(preview);
            if(mBrushPainting == null) {
                reinitBrushPainting();
            }

            int centerWidth = centerLayout.getWidth();
            int centerHeight = centerLayout.getHeight();
            if(centerWidth > 0 && centerHeight > 0) {
                graffitiView.setGraffitiDimens(centerWidth, centerHeight, true, mBrushPainting);
            } else if(TextUtils.isEmpty(mPhotoPick)) {
                Pair<Integer, Integer> pair = getGraffitiHW();
                graffitiView.setGraffitiDimens(pair.first, pair.second, true, mBrushPainting);
            }
            if(unDoBtn.isEnabled() || reDoBtn.isEnabled()){
                saveBtn.setEnabled(true);
                mLayoutSave.setEnabled(true);
            }

            if(mBrushListAdapter == null) {
                mBrushListAdapter = new BrushListAdapter(ImageEditControlActivity.this, R.layout.brush_listview_item, mBrushStyles,
                        mRandomColorPicker, mImageBurshManager);
            }
            showSubMenuGridView();
            mCanvasClickedCount = 0;
            printLog(TAG, "mGraffitiRunnable(): LEAVE.");
        }
    };

    private void randomGenerateBrush() {
        Random random = new Random();
        int brushCount = mBrushStyles.length;
        int index = 0;
        while (true) {
            int randomIndex = random.nextInt(brushCount);
            if(index != randomIndex) {
                mBrushStyle = mBrushStyles[randomIndex];
                break;
            }
            index = randomIndex;
        }
        mBrushSize = random.nextInt(5);
        mBrushColor = mRandomColorPicker.getRandomColor();
    }

    private void resetBrushColor() {
        if(mBrushColorPickerView != null) {
            int color = mBrushColorPickerView.getCurrentColor();
            mBrushColor = (color == Integer.MAX_VALUE) ? Color.RED : color;
        } else {
            mBrushColor = Color.RED;
        }
    }

    private void refreshBrushItem(boolean deleteViewVisiable) {
        mInsertBrushIntoDB = false;
        //getGraffitiAdapter(deleteViewVisiable);
        //mSubMenuGridView.setAdapter(adapterArray.get(mGraffitiBtn.getId()));
        mGraffitiHistoryAdapter = getGraffitiHistoryAdapter(true);
        mBrushHistoryGallery.setAdapter(mGraffitiHistoryAdapter);
    }

    private ImageEditViewPreview mPreviewBubble;

 // NO.10 makeup runnable
    private Runnable mMakeupRunnable = new Runnable() {
        @Override
        public void run() {
            /*
             * FIX BUG: 5185
             * BUG COMMENT: mImageEditDesc.getBitmap() return null
             * DATE: 2013-12-02
             */
            if(mImageEditDesc.getBitmap() == null){
                return;
            }
            printLog(TAG, "mMakeupRunnable(): ENTRY, and mCurrentFuncMode is " + mCurrentFuncMode);
            //fixed the bug31972
            setIsUnDoOrReDo(false);
            if (mAdjustProcess) {
                invisibleAdjustAndSaveBitmap();
            }

            updateLabelBitmap();
            mCurrentItemIndex = -1;
            mCurrentArgbSelect = -1;
            if(mCurrentFuncMode == ACTION_BASIC_EDIT_FUNCTION) {
                setBasicEditHightlight(-1);
            }
            //removeHightlight();

            mCurrentFuncMode = ACTION_EDIT_MAKEUP;
            getMakeupAdapter();
            mSubMenuGridView.setAdapter(adapterArray.get(mMakeupBtn.getId()));
            onPreviewChanged();

            if (mMakeupDlg == null) {
                mMakeupDlg = new ProgressDialog(ImageEditControlActivity.this);
                mMakeupDlg.setProgressStyle(ProgressDialog.STYLE_SPINNER);
                mMakeupDlg.setMessage(ImageEditControlActivity.this
                        .getString(R.string.text_edit_makeup_dialog_message));
                mMakeupDlg.setIndeterminate(false);
                mMakeupDlg.setCancelable(false);
            }

            if (mMakeupControlView == null) {
                mMakeupControlView = new MakeupControlView(ImageEditControlActivity.this);
                mMakeupControlView.initControl(mImageEditDesc.getBitmap(),
                        ImageEditControlActivity.this, mHandler, mImageEditDesc,
                        centerLayout.getWidth(), centerLayout.getHeight());
                // SPRD: add
                mMakeupControlView.initPreviewLocationY(centerLayout.getLocationOnScreen()[1]);
                centerLayout.removeAllViews();
                centerLayout.addView(mMakeupControlView);
                mMakeupControlView.setImageBitmap(mImageEditDesc.getBitmap());
            }
            mMakeupAdjustBtn.setVisibility(View.VISIBLE);
            mMakeupAdjustBtn.setEnabled(true);
            mMakeupControlView.detectFaceRectView(mImageEditDesc.getBitmap(), true);
            showSubMenuGridView();
            printLog(TAG, "mMakeupRunnable(): LEAVE.");
        }
    };

    private boolean isPreviewShow;
    private ImageEditViewPreview onPreviewChanged(){
        ImageEditViewPreview preview = null;
        View view = centerLayout.getChildAt(0);
        /* SPRD: fix bug 527302 NullPointerException @{*/
        if(mImageEditDesc != null && view != null){
            Log.d(TAG, "onPreviewChanged(): bitmap = " + mImageEditDesc.getBitmap() + ", view is " + view);
        }
        /* @}*/
        if(view != null && view instanceof ImageEditViewPreview.ScrollController) {
            ImageEditViewPreview.ScrollController scroller = (ScrollController) view;
            preview = (ImageEditViewPreview) scroller.getChildAt(0);
            if(getUnDoOrReDo()) {
                preview.mSuppMatrix.reset();
            }
            preview.setImageBitmap(getCurrentBitmap());
            isPreviewShow = true;
            int childCount = centerLayout.getChildCount();
            if(childCount > 1){
                for(int i = 1; i < childCount; i++){
                    View childView  = centerLayout.getChildAt(i);
                    centerLayout.removeView(childView);
                    childView = null;
                }
            }
            return preview;
        }
        centerLayout.removeAllViews();
        view = null;
        preview = new ImageEditViewPreview(this);
        centerLayout.addView(preview);
        preview.setImageBitmap(getCurrentBitmap());
        isPreviewShow = true;
        return preview;
    }

    private void invisibleAdjustAndSaveBitmap() {
        mAdjustSclControl.invisibleAdjustAndSaveBitmap();
        mAdjustProcess = false;
    }

 // NO.6 effect runnable
    private Runnable mEffectRunnable = new Runnable() {
        public void run() {
            printLog(TAG, "mEffectRunnable(): ENTRY and mCurrentFuncMode is " + mCurrentFuncMode);
            mCurrentArgbSelect = -1;
            setIsUnDoOrReDo(false);
            if (mAdjustProcess) {
                invisibleAdjustAndSaveBitmap();
            }

            updateLabelBitmap();
            mCurrentFuncMode = ACTION_EDIT_EFFECT;
            mCurrentItemIndex = -1;// fix bug 30981
//            mEffectOriginJpegData = ImageEditOperationUtil.transformBitmapToBuffer(mImageEditDesc.getBitmap());
            getEffectOriginBmp();
            getBasicEffectAdapter();
            mSubMenuGridView.setAdapter(adapterArray.get(mEffectBtn.getId()));
            onPreviewChanged();
            showSubMenuGridView();
            if(true == openEffectFirst){
                openEffectFirst = false;
                if(showEffectTipDlg(msharedPrf) && effectScrnNumber > 1) {
                    /*
                     * FIX BUG: 5466
                     * FIX COMMENT: It will not scrollTo x until scrollView was calculated completely;
                     * DATE: 2013-12-03
                     */
                    mSubHorizontalScrollView.postDelayed((new Runnable(){
                        @Override
                        public void run() {
                            int x = (effectScrnNumber-1) * mScreenWidth;
                            mSubHorizontalScrollView.scrollTo(x, mSubHorizontalScrollView.getScrollY());
                        }
                    }),300);
                }
            }
            printLog(TAG, "mEffectRunnable(): LEAVE.");
        }
    };

    private boolean openEffectFirst = true;
    private boolean showEffectTipDlg(SharedPreferences sp) {
        String key = "Effect_Liked_Tip_Dlg";
        boolean tipShowed = sp.getBoolean(key, false);
        if(tipShowed)
            return false;
        int width = UiUtils.screenWidth();
        int height = UiUtils.screenHeight();
        EffectShowedTipDlg.createEffectShowedDlg(this, 0, 0, width, height).show();
        Editor editor = sp.edit();
        editor.putBoolean(key, true);
        return editor.commit();
    }

    private void getArgbOriginData() {
        Bitmap temp = mImageEditDesc.getBitmap();
        /*
         * FIX BUG: 1397 1468
         * FIX COMMENT: To avoid NullPointerException;
         *              If recycled, will use the original data;
         * DATE: 2012-08-01 2012-08-16
         */
        if(temp != null && !temp.isRecycled()) {
            try{
                mBitmapHeight = temp.getHeight();
                mBitmapWidth = temp.getWidth();
            }catch(OutOfMemoryError e) {
                e.printStackTrace();
                return;
            }
            mArgbOriginData = new int[mBitmapHeight * mBitmapWidth];
            temp.getPixels(mArgbOriginData, 0, mBitmapWidth, 0, 0, mBitmapWidth, mBitmapHeight);
        }
    }

    private void getBasicEffectAdapter() {
        if(null == msharedPrf){
            msharedPrf = this.getSharedPreferences(TAG, Context.MODE_PRIVATE);
            mEffectResource.initLikedList(msharedPrf);
        }

        int totalCount = mEffectResource.getLikedList().size();
        perScrnCount = 4;
        if (adapterArray.get(mEffectBtn.getId()) != null) {
            setDisplayItemCountsInWindowForEffect(totalCount);
            return;
        }
        getScrnNumber(totalCount);
        EffectCateBaseAdapter galleryEffectAdapter = new EffectCateBaseAdapter(this, mEffectResource.getLikedList());
        adapterArray.put(mEffectBtn.getId(), galleryEffectAdapter);
    }

    private void getScrnNumber(int toatleCount) {
        effectScrnNumber = toatleCount / perScrnCount;
        if(toatleCount % perScrnCount > 0) {
            effectScrnNumber ++;
        }
    }

    private void hideResourceGridView() {
        //findViewById(R.id.edit_bottom_tabs).setVisibility(View.VISIBLE);
    }

 // NO.7 photo frame runnable
    private Runnable mPhotoFrameRunnable = new Runnable() {
        public void run() {
            printLog(TAG, "mPhotoFrameRunnable(): ENTRY and mCurrentFuncMode is " + mCurrentFuncMode);
            mCurrentItemIndex = -1;
            setIsUnDoOrReDo(false);
            if (mAdjustProcess) {
                invisibleAdjustAndSaveBitmap();
            }
            updateLabelBitmap();

            mCurrentFuncMode = ACTION_PHOTO_FRAME;
            mCurrentArgbSelect = -1;
            getArgbOriginData();
            if(mPhotoFrameImage == null){
                return;
            }
            BaseAdapter adapter = getResAdapter(mPhotoFrameImage, R.id.btn_edit_photoframe, Const.EXTRA_PHOTO_FRAME_VALUE, false, ImageEditConstants.UPDATE_RES_FRAME);
            mSubMenuGridView.setAdapter(adapter);
            setDisplayItemCountsInWindow(adapter.getCount(), 5, false);
            onPreviewChanged();
            if(unDoBtn.isEnabled() || reDoBtn.isEnabled()){
                saveBtn.setEnabled(true);
                mLayoutSave.setEnabled(true);
            }
            showSubMenuGridView();
            printLog(TAG, "mPhotoFrameRunnable(): LEAVE.");
        }
    };

    // NO.16 mosaic runnable
    private Runnable mMosaicRunnable = new Runnable() {
        public void run() {
            printLog(TAG, "mMosaicRunnable(): ENTRY and mCurrentFuncMode is " + mCurrentFuncMode);
            mCurrentItemIndex = -1;
            setIsUnDoOrReDo(false);
            if (mAdjustProcess) {
                invisibleAdjustAndSaveBitmap();
            }

            updateLabelBitmap();
            mCurrentFuncMode = ACTION_MOSAIC;
            if(mMosaicImage == null){
                return;
            }
            BaseAdapter adapter = getResAdapter(mMosaicImage, R.id.btn_edit_mosaic, Const.EXTRA_MOSAIC_VALUE, false, ImageEditConstants.UPDATE_RES_MOSAIC);
//            mSubMenuGridView.setAdapter(adapter);
            mMosaicGridView.setAdapter(adapter);
//            setDisplayItemCountsInWindow(adapter.getCount(), 5, false);
            setDisplayItemCountsInWindow(mMosaicGridView, adapter.getCount(), 5);

            ImageEditViewPreview preview = onPreviewChanged();
//            final ImageEditViewMosaic mosaicView = new ImageEditViewMosaic(ImageEditControlActivity.this, mImageEditDesc.getBitmap());
            mMosaicEditView = new ImageEditViewMosaic(ImageEditControlActivity.this, mImageEditDesc.getBitmap(), mMosaicSeekBar, mMosaicGridView, mMosaicScrollView);

            if(mMosaicEditView.getMosaicPait() == null) {
                return;
            }
            mMosaicEditView.setBottomLayer(preview);
            centerLayout.addView(mMosaicEditView);
            mMosaicEditView.setMosaicType(MosaicConstant.MOSAIC_DRAW);
            mMosaicDrawBtn.setSelected(true);
            mMosaicEraseBtn.setSelected(false);
            if(unDoBtn.isEnabled() || reDoBtn.isEnabled()){
                saveBtn.setEnabled(true);
                mLayoutSave.setEnabled(true);
            }
            mCurrentArgbSelect = 0;
            setMosaicGridBitmap();
            showOrHideMosaicMenuView(View.VISIBLE);
            ((DecorsAdapter)adapter).setSelected(mCurrentArgbSelect);
            handleChangeMosaicRes();
            printLog(TAG, "mMosaicRunnable(): LEAVE.");
        }
    };
    private void showOrHideMosaicMenuView(int visible) {
        mMosaicRel.setVisibility(visible);
        mMosaicDrawBtn.setVisibility(visible);
        mMosaicEraseBtn.setVisibility(visible);
        mMosaicSeekBar.setVisibility(visible);
        mOperateDoneLayout.setVisibility(visible);
        if(visible == View.GONE) {
            mMosaicGridView.setVisibility(visible);
            mMosaicScrollView.setVisibility(visible);
        }
        mMosaicLineView.setVisibility(visible);
        if(visible == View.VISIBLE) {
          mMosaicRel.startAnimation(mFadeInFromBottom);
          mOperateDoneLayout.startAnimation(mFadeInFromTop);
          mEditUndoRedoLayout.setVisibility(View.GONE);
      }
    }
 // NO.15 label runnable
    private Runnable mTextureRunnable = new Runnable() {
        public void run() {
            printLog(TAG, "mTextureRunnable(): ENTRY and mCurrentFuncMode is " + mCurrentFuncMode);
            mCurrentItemIndex = -1;
            setIsUnDoOrReDo(false);
            if (mAdjustProcess) {
                invisibleAdjustAndSaveBitmap();
            }

            updateLabelBitmap();
            mCurrentFuncMode = ACTION_TEXTURE;
            mCurrentArgbSelect = -1;
            getArgbOriginData();
            if(mTextureImage == null){
                return;
            }
            BaseAdapter adapter = getResAdapter(mTextureImage, R.id.btn_edit_texture, Const.EXTRA_TEXTURE_VALUE, false, ImageEditConstants.UPDATE_RES_TEXTURE);
            mSubMenuGridView.setAdapter(adapter);
            setDisplayItemCountsInWindow(adapter.getCount(), 5, false);

            onPreviewChanged();
            if(unDoBtn.isEnabled() || reDoBtn.isEnabled()){
                saveBtn.setEnabled(true);
                mLayoutSave.setEnabled(true);
            }
            showSubMenuGridView();
            printLog(TAG, "mTextureRunnable(): LEAVE.");
        }
    };

 // NO.14 decoration runnable
    private Runnable mDecorationRunnable = new Runnable() {
        public void run() {
            printLog(TAG, "mDecorationRunnable(): ENTRY and mCurrentFuncMode is " + mCurrentFuncMode);
            mCurrentItemIndex = -1;
            mCurrentArgbSelect = -1;
            setIsUnDoOrReDo(false);
            updateUndoRedoState();
            if (mAdjustProcess) {
                invisibleAdjustAndSaveBitmap();
            }

            updateLabelBitmap();
            mCurrentFuncMode = ACTION_DECORATION;
            if(mDecorImage == null){
                return;
            }
            BaseAdapter adapter = getResAdapter(mDecorImage, R.id.btn_edit_decoration, Const.EXTRA_DECOR_VALUE, false, ImageEditConstants.UPDATE_RES_DECOR);
            mSubMenuGridView.setAdapter(adapter);
            setDisplayItemCountsInWindow(adapter.getCount(), 5, false);

            onPreviewChanged();
            if(unDoBtn.isEnabled() || reDoBtn.isEnabled()){
                saveBtn.setEnabled(true);
                mLayoutSave.setEnabled(true);
            }
            showSubMenuGridView();
            printLog(TAG, "mDecorationRunnable(): LEAVE.");
        }
    };

    private void clearDecorationStack() {
        while(mDecorationUndoStack.size() > 0) {
            mDecorationUndoStack.pop().recycle();
        }

        while(mDecorationRedoStack.size() > 0) {
            mDecorationRedoStack.pop().recycle();
        }
    }

    private int reorganizeUndoStack() {
        if(mDecorationUndoStack.size() > 0){
            Bitmap bitmap = mDecorationUndoStack.get(0);
            mDecorationUndoStack.remove(bitmap);
            if(bitmap != null){
                bitmap.recycle();
                bitmap = null;
            }
        }
        updateUndoRedoState();
        return mDecorationUndoStack.size();
    }

    private Bitmap mTempDecorBitmap = null;
    private Stack<Bitmap> mDecorationUndoStack = new Stack<Bitmap>();
    private Stack<Bitmap> mDecorationRedoStack = new Stack<Bitmap>();

    private void handleDecorAction(AdapterView<?> parent, View view,
            int position, long id) {
        if(position == 0 && DownloadCenter.RESOURCE_DOWNLOAD_ON) {
            mRefreshDecor = true;
            DownloadCenter.openResourceCenter(ImageEditControlActivity.this, Const.EXTRA_DECOR_VALUE);
        } else {
            mLayoutUndo.setEnabled(true);
            saveBtn.setEnabled(true);
            mLayoutSave.setEnabled(true);
            int childCount = centerLayout.getChildCount();
            while(mDecorationRedoStack.size() > 0){
                mDecorationRedoStack.pop().recycle();
            }
            updateUndoRedoState();
            Bitmap bitmap = composeDecorBitmap();
            if(bitmap == null){
                return;
            }
            mDecorationUndoStack.push(bitmap);
            setCurrentBitmap(bitmap);
            if(childCount > 1){
                //updateImageEditBitmap(bitmap);
                ImageEditViewPreview.ScrollController scroller = (ScrollController) centerLayout.getChildAt(0);
                ImageEditViewPreview preview = (ImageEditViewPreview) scroller.getChildAt(0);
                preview.setImageBitmap(getCurrentBitmap());
            } else {
                mImageEditDesc.clearReDoQueue();
            }
            updateUndoRedoState();

            if(mTempDecorBitmap != null && !mTempDecorBitmap.isRecycled()) {
                mTempDecorBitmap.recycle();
                mTempDecorBitmap = null;
            }

            String fileName = mDecorImage[position];
            Bitmap decorationbitmap  = null;
            do {
                try {
                    decorationbitmap = DownloadCenter.thumbNameToBitmap(ImageEditControlActivity.this, fileName, Const.EXTRA_DECOR_VALUE);
                } catch(OutOfMemoryError oom) {
                    Log.w(TAG, "handleDecorAction(): code has a memory leak is detected...");
                } catch (Exception e) {
                    decorationbitmap = null;
                    e.printStackTrace();
                    return;
                }
                if(decorationbitmap == null) {
                    //index is one, means that any operation can not be Completed
                    if(mImageEditDesc.reorganizeQueue() < 2) {
                        ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                        return;
                    }
                }
            } while(decorationbitmap == null);

            mTempDecorBitmap = decorationbitmap;

            RectF rectF = ImageEditOperationUtil.resizeRectF(getCurrentBitmap(), centerLayout.getWidth(), centerLayout.getHeight());
            ImageEditViewDecorView decorView = null;
            if(childCount > 1){
                View childView = centerLayout.getChildAt(1);
                if(childView instanceof ImageEditViewDecorView){
                    decorView = (ImageEditViewDecorView) childView;
                    decorView.setImageBitmap(decorationbitmap);
                    return;
                }
            }

            decorView = new ImageEditViewDecorView(this,rectF);
            decorView.setImageBitmap(decorationbitmap);
            centerLayout.addView(decorView);
            decorView.setIndexInParent(childCount - 1);

        }
    }


    private void handleTexture(AdapterView<?> parent, View view, int position, long id) {
        if(position == 0 && DownloadCenter.RESOURCE_DOWNLOAD_ON) {
            mCurrentArgbSelect = -1;
            mRefreshTexture = true;
            DownloadCenter.openResourceCenter(ImageEditControlActivity.this, Const.EXTRA_TEXTURE_VALUE);
        } else {
            if (mCurrentArgbSelect == position) {
                return ;
            }

            mCurrentArgbSelect = position;
            handleArgb();
        }
    }
    private void handleMosaicRes(AdapterView<?> parent, View view, int position, long id) {
        if(position == -1 && DownloadCenter.RESOURCE_DOWNLOAD_ON) {
            mCurrentArgbSelect = -1;
            mRefreshMosaic = true;
            DownloadCenter.openResourceCenter(ImageEditControlActivity.this, Const.EXTRA_MOSAIC_VALUE);
        } else {
            boolean isDraw = mMosaicDrawBtn.isSelected();
            mMosaicDrawBtn.setSelected(true);
            mMosaicEraseBtn.setSelected(false);
            if (mCurrentArgbSelect == position && isDraw) {
                return ;
            }
            mCurrentArgbSelect = position;
            setMosaicGridBitmap();
            handleChangeMosaicRes();
        }
    }
    private void handleChangeMosaicRes() {
        Bitmap bitmap = null;
        if(mCurrentArgbSelect == 0) {
            bitmap = mMosaicEditView.getMosaicBitmap();
        } else {
            bitmap = DownloadCenter.thumbNameToBitmap(this, mMosaicImage[mCurrentArgbSelect], Const.EXTRA_MOSAIC_VALUE);
        }
        mMosaicEditView.updateMosaicShader(bitmap);
        mMosaicEditView.setMosaicType(MosaicConstant.MOSAIC_DRAW);
    }
    private void setMosaicGridBitmap() {
        Bitmap bitmap = DownloadCenter.thumbNameToThumbBitmap(this, mMosaicImage[mCurrentArgbSelect], Const.EXTRA_MOSAIC_VALUE);
        if(mMosaicGridBtn != null) {
            mMosaicGridBtn.setImageBitmap(bitmap);
        }
    }
    private void handlePhotoFrame(AdapterView<?> parent, View view, int position, long id) {
        if(position == 0 && DownloadCenter.RESOURCE_DOWNLOAD_ON) {
            mCurrentArgbSelect = -1;
            mRefreshFrame = true;
            DownloadCenter.openResourceCenter(ImageEditControlActivity.this, Const.EXTRA_PHOTO_FRAME_VALUE);
        } else {
            if (mCurrentArgbSelect == position) {
                return ;
            }

            mCurrentArgbSelect = position;
            handleArgb();
        }
    }

    private MyGraffitiViewBaseAdapter getGraffitiAdapter(boolean deleteViewVisiable){
        if(mBrushArrayList == null) {
            mBrushArrayList = new ArrayList<Object>();
        } else {
            mBrushArrayList.clear();
        }

        final Resources resources = getResources();
        String[] editTitles = null;
        int[] editIconIds = null;
        String[] tempTitles = resources.getStringArray(R.array.edit_brush_item_titles);
        int[] tempIconIds = getIconIds(resources,R.array.edit_brush_item_icons);
        if(mShowCanvas) {
            editTitles = tempTitles;
            editIconIds = tempIconIds;
        } else {
            int len = tempTitles.length;
            editTitles = new String[len - 1];
            editIconIds = new int[len - 1];
            System.arraycopy(tempTitles, 1, editTitles, 0, len - 1);
            System.arraycopy(tempIconIds, 1, editIconIds, 0, len - 1);
        }
        tempTitles = null;
        tempIconIds = null;

        int defaultIconCount = editTitles.length;
        for(int i = 0; i < defaultIconCount; i++) {
            HashMap<String, Integer> map = new HashMap<String, Integer>();
            map.put(editTitles[i], editIconIds[i]);

            mBrushArrayList.add(map);
        }

        int size = mBrushArrayList.size();

        int itemWidth = 0;
        if(mShowCanvas) {
            itemWidth = setDisplayItemCountsInWindow(size, 5, true);
        }else {
            itemWidth = setDisplayItemCountsInWindow(size, 4, true);
        }

        if(size < BrushConstant.getGraffitiPredefineAllItems(mShowCanvas) + 1) {
            deleteViewVisiable = false;
        }
        MyGraffitiViewBaseAdapter myGraffitiViewBaseAdapter = new MyGraffitiViewBaseAdapter(ImageEditControlActivity.this, mBrushArrayList,
                mRandomColorPicker, mImageBurshManager, itemWidth, deleteViewVisiable, mShowCanvas, mCustomBrushHeight,true);
        adapterArray.put(mGraffitiBtn.getId(), myGraffitiViewBaseAdapter);
        return myGraffitiViewBaseAdapter;

    }

    ArrayList<Object> mBrushHistoryArrayList = null;
    private MyGraffitiViewBaseAdapter getGraffitiHistoryAdapter(boolean deleteViewVisiable) {

        if(mBrushHistoryArrayList == null) {
            mBrushHistoryArrayList = new ArrayList<Object>();
        } else {
            mBrushHistoryArrayList.clear();
        }
        Cursor cursor = mTitleSaveAsDatabase.getAllBrushes();
        try {
            if(cursor != null && cursor.moveToFirst()) {
                int j = 0;
                do {
                    BrushItemInfo brushItemInfo = new BrushItemInfo();
                    brushItemInfo.brushId = cursor.getLong(0);
                    brushItemInfo.brushStyle = cursor.getInt(1);
                    brushItemInfo.brushSize = cursor.getInt(2);
                    brushItemInfo.brushColor = cursor.getString(3);
                    brushItemInfo.brushMode = cursor.getInt(4);

                    mBrushHistoryArrayList.add(brushItemInfo);
                } while (cursor.moveToNext());
            }
        } finally {
            if(cursor != null) {
                cursor.close();
            }
        }

        int size = mBrushHistoryArrayList.size();
        MyGraffitiViewBaseAdapter myGraffitiViewBaseAdapter = new MyGraffitiViewBaseAdapter(ImageEditControlActivity.this, mBrushHistoryArrayList,
                mRandomColorPicker, mImageBurshManager, 200, isLongClickStatus, mShowCanvas, mCustomBrushHeight,false);
        return myGraffitiViewBaseAdapter;
    }

    private BaseAdapter getHistoryLabelAdapter(boolean deleteViewVisiable) {
        if(mLabelList == null) {
            mLabelList = new ArrayList<Object>();
        } else {
            mLabelList.clear();
        }

        Cursor cursor = mTitleSaveAsDatabase.getAllTitles();
        try {
            if(cursor != null && cursor.moveToFirst()) {
                int j = 0;
                int type = 10;
                do {
                    if(cursor.getInt(7) > 0) {
                        type = cursor.getInt(7);
                    }
                    if(type == mCurrentFuncMode){
                        ViewAttributes attributes = new ViewAttributes();
                        attributes.setTitleId(cursor.getLong(0));
                        attributes.setTextColor(cursor.getString(1));
                        attributes.setGradientEnd(cursor.getString(2));
                        attributes.setOutline(cursor.getString(3));
                        attributes.setStrokeWidth(cursor.getString(4));
                        attributes.setFont(cursor.getString(5));
                        attributes.setDrawText(cursor.getString(6));
                        attributes.setTextSize(convertSizeByScreenWidth());
                        attributes.setLabel(cursor.getString(8));
                        attributes.setLabelHead(cursor.getString(9));
                        attributes.setLabelTail(cursor.getString(10));
                        attributes.setPanelFill(cursor.getString(11));
                        attributes.setBalloonStyle(cursor.getString(12));
                        mLabelList.add(j, attributes);
                        j++;
                    }
                } while (cursor.moveToNext());
            }
        } finally {
            if(cursor != null) {
                cursor.close();
            }
        }

        if(mCurrentFuncMode == ACTION_EDIT_TEXT) {
            return new MyCommonTitleBaseAdapter(this, mLabelList, R.layout.title_common_item, deleteViewVisiable);
        }else if(mCurrentFuncMode == ACTION_TEXT_BUBBLE) {
            return new HistoryLabelAdapter(this, mLabelList, "Balloon", deleteViewVisiable);
        } else if(mCurrentFuncMode == ACTION_EDIT_TAB_LABEL) {
            return new HistoryLabelAdapter(this, mLabelList, "Label", deleteViewVisiable);
        }
        return null;
    }

    private String convertSizeByScreenWidth() {
        String textSizeStr = "42";
        int screenWidth = UiUtils.screenWidth();
        int textSize = 39;
        if(screenWidth <= 480) {
            textSize = 42;
        }
        textSize = (int) (textSize * UiUtils.screenWidth() / 480);
        textSizeStr = String.valueOf(textSize);

        Log.d(TAG, "convertSizeByScreenWidth(): textSizeStr is " + textSizeStr + ", screenWidth is " + screenWidth);

        return textSizeStr;

    }

    private ArrayList<Object> mLabelList;
    public class LabelItem {
        public String labelTitle;
        public int labelIconId;
    }

    private void refreshTabLabelItem(boolean deleteViewVisiable) {
        mInsertTitleIntoDB = false;
        mLabelHistoryAdapter =  getHistoryLabelAdapter(deleteViewVisiable);
        mLabelHistoryGallery.setAdapter(mLabelHistoryAdapter);
    }

    private void handleArgb() {
        ImageEditOperationUtil.startBackgroundJob(this, "loading", getString(R.string.text_waiting),
                new Runnable() {
                    public void run() {
                        printLog(TAG, "handleArgb(): ENTRY and mCurrentFuncMode is " + mCurrentFuncMode);
                        /* FIX BUG : 4476
                         * BUG COMMENT : if the mArgbOriginData is null,return and not add frame for it
                         * DATE : 2013-07-16
                         */
                        if(mArgbOriginData == null) {
                            printLog(TAG, "mArgbOriginData is null and return");
                            return;
                        }
                        String strFileName = null;
                        if (mCurrentFuncMode == ACTION_PHOTO_FRAME) {
                            strFileName = DownloadCenter.getFullResourcePath(mPhotoFrameImage[mCurrentArgbSelect], Const.EXTRA_PHOTO_FRAME_VALUE);
                        }
                        else if (mCurrentFuncMode == ACTION_TEXTURE) {
                            strFileName = DownloadCenter.getFullResourcePath(mTextureImage[mCurrentArgbSelect], Const.EXTRA_TEXTURE_VALUE);
                        }
                        int[] pixels = ImageProcessJni.AddPhotoFrame4ArgbBuffer(mArgbOriginData, mBitmapWidth, mBitmapHeight, strFileName);
                        if (pixels == null) {
                            return ;
                        }
                        Bitmap bitmap = null;
                        do {
                            try {
                                bitmap = Bitmap.createBitmap(pixels, mBitmapWidth, mBitmapHeight, Bitmap.Config.ARGB_8888);
                            } catch(OutOfMemoryError oom) {
                                Log.w(TAG, "handleArgb(): code has a memory leak is detected...");
                            }
                            if(bitmap == null){
                                //index is one, means that any operation can not be Completed
                                if(mImageEditDesc.reorganizeQueue() < 2) {
                                    ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                                    return;
                                }
                            }
                        } while(bitmap == null);

                        setCurrentBitmap(bitmap);
                        //updateImageEditBitmap(bitmap);
                        printLog(TAG, "handleArgb(): LEAVE.");
                    }
                }, mHandler);
    }

//    private void handleFunEffect() {
//        setEffectCatHightlight(mCurrentItemIndex);
//
//        if(mCurrentFuncMode == ACTION_EDIT_EFFECT && mCurrentFunModePost < 0 && mCurrentItemIndex != -1) {
//            ArrayList<EffectItem> effectItemList = mEffectResource.getEffectItem(mCurrentItemIndex);
//            if(effectItemList != null && effectItemList.size() > 0) {
//                mEffectTypeAdapter = new EffectTypeBaseAdapter(this, effectItemList);
//                mEffectPostGallery.setAdapter(mEffectTypeAdapter);
//                mEffectPostGallery.setOnItemSelectedListener(new EffectOnItemSelectedListener());
//                mEffectRootView.setVisibility(View.VISIBLE);
//                mEffectPostGallery.setSelection(1);
//                handlePostFunEffect(1);
//            }
//        }
//    }

    private void handleFunEffect(int position) {
        setEffectCatHightlight(position);

        if(mCurrentFuncMode == ACTION_EDIT_EFFECT && position != -1) {
            if(mEffectResource.getLikedList().size() == position+1){// The last item , open effect showed gridview
                if(null == effectShowed){
                    mEffectResource.initEffectShowedList(msharedPrf);

                    effectShowed = new EffectShowed(this, mHandler, EffectTypeResource.UPHOTO_EFFECT_RES);
                    effectShowed.showTipDlg(msharedPrf);
                }
                effectShowed.showEffects(mFadeInFromTop);
            }else{
                mCurrentItemIndex = position;
                handlePostFunEffect(position);
            }
        }
    }

    /*
     * FIX BUG: 4058
     * FIX COMMENT: The JNI method is not thread safety. Add synchronized block
     * DATE: 2013-06-26
     */
    private byte[] effectSync = new byte[0];
    public void handlePostFunEffect(final int position) {
        final Context context = getApplicationContext();

        ImageEditOperationUtil.startBackgroundJob(this, "loading", getString(R.string.text_waiting),
                new Runnable() {
                    public void run() {

                        synchronized (effectSync) {
                            printLog(TAG, "handlePostFunEffect(): ENTRY and mCurrentFuncMode is " + mCurrentFuncMode);
//                            int effectValue = mEffectResource.getEffectItem(mCurrentItemIndex).get(position).mTypeValue;
                            EffectItem currentItem = mEffectResource.getLikedList().get(position);
                            if (null == currentItem) {
                                return;
                            }
                            Bitmap[] textureRes = null;
                            mGpuProcees = new GpuProcess(context);
                            mGpuProcees.setShaderType(currentItem.mShaderName);
//                            if(currentItem.mNeedTexture){
                            if (EffectResInfo.isNeedResourceTexture(currentItem.mShaderName)) {
                                int drawableId = 0;
                                boolean needRotation = false;
                                drawableId = EffectResInfo.getDrawableID(currentItem.mShaderName);
                                needRotation = EffectResInfo.isResourceDrawableRotated(drawableId);
//                                }else{
//                                    if(!"effect_autumn".equals(currentItem.rIconName) &&
//                                            !"effect_fresh".equals(currentItem.rIconName) &&
//                                            !"effect_rainbrown".equals(currentItem.rIconName)) {
//                                        needRotation = true;
//                                    }
//                                    String drawablename = currentItem.rIconName + "_1";
//                                    drawableId = getResources().getIdentifier(drawablename, "drawable", getPackageName());
//                                }
                                textureRes = new Bitmap[1];
                                BitmapFactory.Options options = new BitmapFactory.Options();
                                options.inScaled = false;
                                options.inPurgeable = true;
                                options.inDensity = mEffectOriginBitmap.getDensity();
                                Bitmap bitmap = BitmapFactory.decodeResource(getResources(), drawableId, options);
                                if (bitmap == null) {
                                    return;
                                }
                                if (needRotation) {
                                    if (bitmap.getWidth() > bitmap.getHeight()) {
                                        Matrix matrix = new Matrix();
                                        matrix.setRotate(90);
                                        Bitmap b2 = null;

                                        /*
                                         * FIX BGU: 4789
                                         * FIX COMMENT: fix out of memory exception
                                         * DATE: 2013-09-02
                                         */
                                        try {
                                            b2 = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), matrix, true);
                                        } catch (OutOfMemoryError e) {
                                            b2 = null;
                                        }

                                        while (b2 == null) {
                                            if (mImageEditDesc.reorganizeQueue() < 2) {
                                                ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                                                return;
                                            }
                                        }

                                        if (b2 != bitmap) {
                                            Utils.recyleBitmap(bitmap);
                                            bitmap = b2;
                                        }
                                    }
                                }
                                textureRes[0] = bitmap;
                            }
                            Bitmap bitmap = null;
                            try {
                                /* SPRD:fix bug 521650 The OOM may happened in the function of
                                 * getEffectOriginBmp, which will keep the state of
                                 * mEffectOriginBitmap Recycled, and if that the runtime exception
                                 * will happened here. Add judgement to avoid the exception
                                 * happened.@{*/
                                /*
                                bitmap = Bitmap.createBitmap(mEffectOriginBitmap, 0, 0, mEffectOriginBitmap.getWidth(), mEffectOriginBitmap.getHeight());
                                */
                                if (mEffectOriginBitmap != null && !mEffectOriginBitmap.isRecycled()) {
                                    bitmap = Bitmap.createBitmap(mEffectOriginBitmap, 0, 0,
                                            mEffectOriginBitmap.getWidth(),
                                            mEffectOriginBitmap.getHeight());
                                }
                                /*}@*/
                            } catch (OutOfMemoryError e) {
                                LogUtils.error(TAG, "handlePostFunEffect : OOM");
                            }
                            // To do get the resource texture
                            mGpuProcees.process(mEffectOriginBitmap, bitmap, textureRes);
                            if (textureRes != null) {
                                for (int i = 0; i < textureRes.length; i++) {
                                    if (textureRes[i] != null && !textureRes[i].isRecycled()) {
                                        textureRes[i].recycle();
                                        textureRes[i] = null;
                                    }
                                }
                            }
                            mGpuProcees.finish();
                            while (bitmap == null) {
                                //index is one, means that any operation can not be Completed
                                if (mImageEditDesc.reorganizeQueue() < 2) {
                                    ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                                    return;
                                }
                                //bitmap = BitmapFactory.decodeByteArray(mEffectOriginJpegData, 0, mEffectOriginJpegData.length,Utils.getNativeAllocOptions());
                            }
                            setCurrentBitmap(bitmap);
                            //updateImageEditBitmap(bitmap);

                            printLog(TAG, "handlePostFunEffect(): LEAVE.");
                        }
                    }
                }, mHandler);
    }

    public void handleLomo(AdapterView<?> parent, View view, int position,
            long id) {
        if (mCurrentItemIndex == ImageEditConstants.EFFECT_LOMO) {
            return ;
        }
        mCurrentItemIndex = ImageEditConstants.EFFECT_LOMO;
//        handleFunEffect();
        Log.d(TAG, "handleLomo(): handleLomo...");
    }

    public void handleHDR(AdapterView<?> parent, View view, int position,
            long id) {
        if (mCurrentItemIndex == ImageEditConstants.EFFECT_HDR) {
            return ;
        }
        mCurrentItemIndex = ImageEditConstants.EFFECT_HDR;
//        handleFunEffect();
        Log.d(TAG, "handleHDR(): handleHDR...");
    }

    public void handleSkin(AdapterView<?> parent, View view, int position,
            long id) {
        if (mCurrentItemIndex == ImageEditConstants.EFFECT_SKIN) {
            return ;
        }
        mCurrentItemIndex = ImageEditConstants.EFFECT_SKIN;
//        handleFunEffect();
        Log.d(TAG, "handleSkin(): handleSkin...");
    }

    public void handleVividLight(AdapterView<?> parent, View view, int position,
            long id) {
        if (mCurrentItemIndex == ImageEditConstants.EFFECT_VIVID_LIGHT) {
            return ;
        }
        mCurrentItemIndex = ImageEditConstants.EFFECT_VIVID_LIGHT;
//        handleFunEffect();
        Log.d(TAG, "handleVividLight(): handleVividLight...");
    }

    public void handleSketch(AdapterView<?> parent, View view, int position,
            long id) {
        if (mCurrentItemIndex == ImageEditConstants.EFFECT_SKETCH) {
            return ;
        }
        mCurrentItemIndex = ImageEditConstants.EFFECT_SKETCH;
//        handleFunEffect();
        Log.d(TAG, "handleSketch(): handleSketch...");
    }

    public void handleColorFull(AdapterView<?> parent, View view, int position,
            long id) {
        if (mCurrentItemIndex == ImageEditConstants.EFFECT_COLORFULL) {
            return ;
        }
        mCurrentItemIndex = ImageEditConstants.EFFECT_COLORFULL;
//        handleFunEffect();
        Log.d(TAG, "handleColorFull(): handleColorFull...");
    }

    public void handleFunny(AdapterView<?> parent, View view, int position,
            long id) {
        if (mCurrentItemIndex == ImageEditConstants.EFFECT_FUNNY) {
            return ;
        }
        mCurrentItemIndex = ImageEditConstants.EFFECT_FUNNY;
//        handleFunEffect();
        Log.d(TAG, "handleFunny(): handleFunny...");

    }

    public void handleNostalgia(AdapterView<?> parent, View view, int position,
            long id) {
        if (mCurrentItemIndex == ImageEditConstants.EFFECT_NOSTALGIA) {
            return ;
        }
        mCurrentItemIndex = ImageEditConstants.EFFECT_NOSTALGIA;
//        handleFunEffect();
        Log.d(TAG, "handleNostalgia(): handleNostalgia...");
    }

    public void handleBlackWhite(AdapterView<?> parent, View view,
            int position, long id) {
        if (mCurrentItemIndex == ImageEditConstants.EFFECT_BLACKWHITE) {
            return ;
        }
        mCurrentItemIndex = ImageEditConstants.EFFECT_BLACKWHITE;
//        handleFunEffect();
        Log.d(TAG, "handleBlackWhite(): handleBlackWhite...");

    }

    public void handleDeform(AdapterView<?> parent, View view, int position,
            long id) {
        if (mCurrentItemIndex == ImageEditConstants.EFFECT_DEFORM) {
            return ;
        }
        mCurrentItemIndex = ImageEditConstants.EFFECT_DEFORM;
//        handleFunEffect();
        Log.d(TAG, "handleDeform(): handleDeform...");
    }

    public void handleCropAction(AdapterView<?> parent, View view, int position, long id){
        if(!isPreviewShow && preActionCode == mCurrentFuncMode){
//            ImageEditOperationUtil.showToast(this, R.string.edit_item_running, Toast.LENGTH_SHORT);
            return;
        }
        ImageEditOperationUtil.showToast(this, R.string.edit_double_tap_sure_tip, Toast.LENGTH_SHORT);
        Log.d(TAG, "handleCropAction(): handleCropAction...");
        CropImage.sParamBitmap = getCurrentBitmap();
        final Intent intent = new Intent(ImageEditControlActivity.this, CropImage.class);
       
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        currentClassName = getPackageName() + ".tools.CropImage";
        addActivityCenterView(ImageEditConstants.BASIC_EDIT_FUNCTION_ACTION_CROP, intent);
    }

    public View addActivityCenterView(String id,Intent intent) {
        Log.d(TAG, "addActivityCenterView(): addActivityCenterView...");
        centerLayout.removeAllViews();
        final View cropView = manager.startActivity(id, intent).getDecorView();
        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT);
        params.gravity = Gravity.CENTER;
        centerLayout.addView(cropView, 0, params);
        return cropView;
    }

    @SuppressWarnings("unused")
    private void handleRotateLeftAction(AdapterView<?> parent, View view, int position, long id){
        ImageEditOperationUtil.startBackgroundJob(this, "loading", getString(R.string.text_waiting), new Runnable() {
            public void run() {
                printLog(TAG, "handleRotateLeftAction(): ENTRY.");
                handleRotateAction(ImageEditConstants.ROTATE_CLOCKWISE);
                printLog(TAG, "handleRotateLeftAction(): LEAVE.");
            }
        }, mHandler);
    }

    private void handleRotateRightAction(AdapterView<?> parent, View view, int position, long id) {
        ImageEditOperationUtil.startBackgroundJob(this, "loading", getString(R.string.text_waiting), new Runnable() {
            public void run() {
                printLog(TAG, "handleRotateLeftAction(): ENTRY.");
                handleRotateAction(ImageEditConstants.ROTATE_UNCLOCKWISE);
                printLog(TAG, "handleRotateLeftAction(): LEAVE.");
            }
        }, mHandler);
    }

    /*
     * FIX BUG: 4215
     * FIX COMMENT:restore the bitmap config when rotated
     * DATE: 2013-06-20
     */
    private void handleRotateAction(int rotate) {
        /*
         * FIX BUG: 3794
         * FIX COMMENT:avoid null point
         * DATE: 2013-07-03
         */
        Bitmap bitmap = getCurrentBitmap();
        if(bitmap == null) {
            return;
        }
        Bitmap.Config beforeConfig=bitmap.getConfig();
        bitmap = ImageEditOperationUtil.rotate(bitmap, rotate);
        Log.d(TAG, "handleRotateRightAction(): rotate = " + rotate + ", bitmap = " + bitmap);
        while(bitmap == null) {
            //index is one, means that any operation can not be Completed
            if(mImageEditDesc.reorganizeQueue() < 2) {
                ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                return;
            }
            bitmap = ImageEditOperationUtil.rotate(getCurrentBitmap(), rotate);
        }
        Bitmap.Config afterConfig=bitmap.getConfig();
        if(afterConfig!=beforeConfig){
            Bitmap bmp=bitmap;
            bitmap=ImageEditOperationUtil.createBitmapFromConfig(bmp, beforeConfig);
            bmp.recycle();
        }
        setCurrentBitmap(bitmap);
        //updateImageEditBitmap(bitmap);
    }

    private void handleFlipHorizontalAction(AdapterView<?> parent, View view, int position, long id){
        ImageEditOperationUtil.startBackgroundJob(this, "loading", getString(R.string.text_waiting), new Runnable() {
            public void run() {
                printLog(TAG, "handleFlipHorizontalAction(): ENTRY.");
                handleFlipAction(ImageEditConstants.FLIP_HORIZONTAL);
                printLog(TAG, "handleFlipHorizontalAction(): LEAVE.");
            }
        }, mHandler);
    }

    private void handleFlipVerticalAction(AdapterView<?> parent, View view, int position, long id){
        ImageEditOperationUtil.startBackgroundJob(this, "loading", getString(R.string.text_waiting), new Runnable() {
            public void run() {
                printLog(TAG, "handleFlipVerticalAction(): ENTRY.");
                handleFlipAction(ImageEditConstants.FLIP_VERTICAL);
                printLog(TAG, "handleFlipVerticalAction(): LEAVE.");
            }
        }, mHandler);
    }

    private void handleFlipAction(int flipOrientation) {
        Bitmap bitmap = ImageEditOperationUtil.operateFlipBitmap(getCurrentBitmap(), flipOrientation);
        while(bitmap == null) {
            //index is one, means that any operation can not be Completed
            if(mImageEditDesc != null && mImageEditDesc.reorganizeQueue() < 2) {
                ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                return;
            }
            bitmap = ImageEditOperationUtil.operateFlipBitmap(getCurrentBitmap(), flipOrientation);
        }
        setCurrentBitmap(bitmap);
        //updateImageEditBitmap(bitmap);
    }

    private void handleSymmetryHorizontalLeftAction(AdapterView<?> parent, View view, int position, long id){
        Log.d(TAG, "handleSymmetryHorizontalLeftAction(): handleSymmetryHorizontalLeftAction...");
        ImageEditOperationUtil.startBackgroundJob(this, "loading", getString(R.string.text_waiting), new Runnable() {
            public void run() {
                printLog(TAG, "handleSymmetryHorizontalLeftAction(): ENTRY.");
                Bitmap bitmap = getCurrentBitmap();
                if (bitmap == null) {//SPRD:Fix bug 472666
                    return;
                }
                int nestRectWidth = bitmap.getWidth() / 2;
                int nestRectHeight = bitmap.getHeight();

                handleSymmetryAction(bitmap, 0, 0, nestRectWidth, nestRectHeight, nestRectWidth, 0, ImageEditConstants.FLIP_HORIZONTAL);
                printLog(TAG, "handleSymmetryHorizontalLeftAction(): LEAVE.");
            }
        }, mHandler);
    }

    private void handleSymmetryHorizontalRightAction(AdapterView<?> parent, View view, int position, long id){
        Log.d(TAG, "handleSymmetryHorizontalRightAction(): handleSymmetryHorizontalRightAction...");
        ImageEditOperationUtil.startBackgroundJob(this, "loading", getString(R.string.text_waiting), new Runnable() {
            public void run() {
                printLog(TAG, "handleSymmetryHorizontalRightAction(): ENTRY.");
                Bitmap bitmap = getCurrentBitmap();
                if (bitmap == null) {//SPRD:Fix bug 472666
                    return;
                }
                int nestRectHeight = bitmap.getHeight();
                int nestRectDeltaX = bitmap.getWidth() / 2;

                handleSymmetryAction(bitmap, nestRectDeltaX, 0, nestRectDeltaX, nestRectHeight, 0, 0, ImageEditConstants.FLIP_HORIZONTAL);
                printLog(TAG, "handleSymmetryHorizontalRightAction(): LEAVE.");
            }
        }, mHandler);
    }

    private void handleSymmetryVerticalUpAction(AdapterView<?> parent, View view, int position, long id){
        ImageEditOperationUtil.startBackgroundJob(this, "loading", getString(R.string.text_waiting), new Runnable() {
            public void run() {
                printLog(TAG, "handleSymmetryVerticalUpAction(): ENTRY.");
                Bitmap bitmap = getCurrentBitmap();
                if (bitmap == null) {//SPRD:Fix bug 472666
                    return;
                }
                int nestRectWidth = bitmap.getWidth();
                int nestRectHeight = bitmap.getHeight() / 2;

                handleSymmetryAction(bitmap, 0, 0, nestRectWidth, nestRectHeight, 0, nestRectHeight, ImageEditConstants.FLIP_VERTICAL);
                printLog(TAG, "handleSymmetryVerticalUpAction(): LEAVE.");
            }

        }, mHandler);
    }

    private void handleSymmetryVerticalDownAction(AdapterView<?> parent, View view, int position, long id){
        ImageEditOperationUtil.startBackgroundJob(this, "loading", getString(R.string.text_waiting), new Runnable() {
            public void run() {
                printLog(TAG, "handleSymmetryVerticalDownAction(): ENTRY.");
                Bitmap bitmap = getCurrentBitmap();
                if (bitmap == null) {//SPRD:Fix bug 472666
                    return;
                }
                int nestRectWidth = bitmap.getWidth();
                int nestRectDeltaY = bitmap.getHeight() / 2;

                handleSymmetryAction(bitmap, 0, nestRectDeltaY, nestRectWidth, nestRectDeltaY, 0, 0, ImageEditConstants.FLIP_VERTICAL);
                printLog(TAG, "handleSymmetryVerticalDownAction(): LEAVE.");
            }

        }, mHandler);
    }

    private void handleSymmetryAction(Bitmap bitmap, int nestRectDeltaX, int nestRectDeltaY, int nestRectWidth, int nestRectHeight,
            int nestRectWidth2, int nestRectHeight2, int flipOritation) {
        Bitmap dstBitmap = ImageEditOperationUtil.operateSymmetryBitmap(bitmap, nestRectDeltaX, nestRectDeltaY, nestRectWidth,
                nestRectHeight, nestRectWidth2, nestRectHeight2, flipOritation,new PorterDuffXfermode(PorterDuff.Mode.SRC_OVER));
        while(dstBitmap == null) {
            //index is one, means that any operation can not be Completed
            if(mImageEditDesc.reorganizeQueue() < 2) {
                ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                return;
            }
            dstBitmap = ImageEditOperationUtil.operateSymmetryBitmap(bitmap, nestRectDeltaX, nestRectDeltaY, nestRectWidth,
                    nestRectHeight, nestRectWidth2, nestRectHeight2, flipOritation,new PorterDuffXfermode(PorterDuff.Mode.SRC_OVER));
        }
        setCurrentBitmap(dstBitmap);
        //updateImageEditBitmap(dstBitmap);
    }

    private void processAdjust() {
        mAdjustSclControl.setProcessBitmap(mImageEditDesc.getBitmap());
        mAdjustSclControl.restoreSeekBar();
        updatePreview(null);
        mAdjustProcess = true;
    }

    private synchronized void updateMakeupPreview(Bitmap bitmap) {
//        centerLayout.removeAllViews();
//        centerLayout.addView(mMakeupControlView);
        mMakeupControlView.setImageBitmap(mMakeupControlView.getEffectBitmap());
    }

    private void processMakeupFeature(int makeupMode) {
        mMakeupControlView.resetPopupWindow();
        mMakeupControlView.setMakeupMode(makeupMode);
        updateMakeupPreview(null);
    }

    private void handleDeblemish(AdapterView<?> parent, View view, int position, long id) {
        final String KEY = "pref_deblemish_hint_shown";
        if (!mSharedPreferences.getBoolean(KEY, false)) {
            mDeblemishstub.setVisibility(View.VISIBLE);
            mDeblemishView = findViewById(R.id.makeup_deblemish_hint);
            mDeblemishView.setOnTouchListener(new View.OnTouchListener( ) {
                @Override
                public boolean onTouch(View v, MotionEvent event) {
                    v.setVisibility(View.GONE);
                    return true;
                }
            });
            mSharedPreferences.edit().putBoolean(KEY, true).commit();
        }
        processMakeupFeature(ImageEditConstants.MAKEUP_MODE_DEBLEMISH);
    }

    private void handleWhiteface(AdapterView<?> parent, View view, int position, long id) {
        processMakeupFeature(ImageEditConstants.MAKEUP_MODE_WHITENFACE);
    }

    private void handleSoftenface(AdapterView<?> parent, View view, int position, long id) {
        processMakeupFeature(ImageEditConstants.MAKEUP_MODE_SOFTENFACE);
    }

    private void handleTrimface(AdapterView<?> parent, View view, int position, long id) {
        processMakeupFeature(ImageEditConstants.MAKEUP_MODE_TRIMFACE);
    }

    private void handleBigeye(AdapterView<?> parent, View view, int position, long id) {
        processMakeupFeature(ImageEditConstants.MAKEUP_MODE_BIGEYE);
    }

    private synchronized void updatePreview(Bitmap bitmap) {
        centerLayout.removeAllViews();
        centerLayout.addView(mAdjustSclControl);
        mAdjustSclControl.setImageBitmap(bitmap);
        isPreviewShow = true;
    }

    private Handler mHandler = new Handler(new Callback() {
        private boolean isClearing;
        public boolean handleMessage(Message msg) {
            /* SPRD: Fix bug 559526 mImageEditDesc could be null @{ */
            if (isDestroyed()) {
                Log.e(TAG, "handleMessage: activity is destroyed");
                return true;
            }
            /* @} */

            switch (msg.what) {
            case ImageEditConstants.ACTION_PREVIEW:
                /*
                 * FIX BUG: 1104
                 * BUG CAUSE : array index out of bounds exception
                 * FIX COMMENT: not display effect menu if click the saving or share button
                 * FIX DATE: 2012-06-14
                 */
                if(mCurrentFuncMode != ACTION_EDIT_MAKEUP && mCurrentFuncMode != ACTION_GRAFFITI) {
                    onPreviewChanged();
                }
                if(isClearing){
                    mImageEditDesc.clearQueueWithoutFisrtAndCurrent();
                    isClearing = false;
                }
                //fixed the bug32822
                if(mExitAndSaved) {
                    exitToLogin();
                    mExitAndSaved = false;
                }
                break;
            case ImageEditConstants.ACTION_REDO_ICON_FOCUSED:
                reDoBtn.setEnabled(true);
                mLayoutRedo.setEnabled(true);
                saveBtn.setEnabled(true);
                mLayoutSave.setEnabled(true);
                break;
            case ImageEditConstants.ACTION_REDO_ICON_NOT_FOCUSED:
                reDoBtn.setEnabled(false);
                mLayoutRedo.setEnabled(false);
                if(mBrushPainting != null && mBrushPainting.getStackCount() != 0) {
                    reDoBtn.setEnabled(true);
                    mLayoutRedo.setEnabled(true);
                }
                if(mMakeupControlView != null && mMakeupControlView.getMakeupStackCount() != 0) {
                    reDoBtn.setEnabled(true);
                    mLayoutRedo.setEnabled(true);
                }
                if(!unDoBtn.isEnabled()){
                    saveBtn.setEnabled(false);
                    mLayoutSave.setEnabled(false);
                }
                break;
            case ImageEditConstants.ACTION_UNDO_ICON_FOCUSED:
                unDoBtn.setEnabled(!mSaving);
                unDoBtn.setPressed(mSaving);//fixed the bug32368
                mLayoutUndo.setEnabled(!mSaving);
                saveBtn.setEnabled(true);
                mLayoutSave.setEnabled(true);
                break;
            case ImageEditConstants.ACTION_UNDO_ICON_NOT_FOCUSED:
                unDoBtn.setEnabled(false);
                mLayoutUndo.setEnabled(false);
                if(!reDoBtn.isEnabled()){
                    saveBtn.setEnabled(false);
                    mLayoutSave.setEnabled(false);
                }
                break;
            case ImageEditConstants.ACTION_SAVED_SUCUESS:
                if(mExitAndSaved) {
                    Toast.makeText(ImageEditControlActivity.this, R.string.text_edit_save_success_tip, Toast.LENGTH_LONG).show();
                }
                mSaving = false;
                saveBtn.setEnabled(false);
                mLayoutSave.setEnabled(false);
                reDoBtn.setEnabled(false);
                mLayoutRedo.setEnabled(false);
                unDoBtn.setEnabled(false);
                mLayoutUndo.setEnabled(false);
                /**
                 * BUG: 929
                 * BUG CAUSE: This variable is initialized in graffiti tab
                 * FIX COMMENT: To judge whether a variable is empty
                 * DATE: 2012-04-21
                 */
                if(mBrushPainting != null) {
                    mBrushPainting.clearAllStrokers();
                }
                isClearing = true;
                if (mCurrentFuncMode == ACTION_EDIT_EFFECT) {
//                    mEffectOriginJpegData = ImageEditOperationUtil
//                    .transformBitmapToBuffer(mImageEditDesc.getBitmap());
                    getEffectOriginBmp();
                } else if (mCurrentFuncMode == ACTION_PHOTO_FRAME || mCurrentFuncMode == ACTION_TEXTURE) {
                    getArgbOriginData();
                } else if(mCurrentFuncMode == ACTION_BASIC_EDIT_FUNCTION) {
                    if(mAdjustSclControl != null) {
                        mAdjustSclControl.adjustControlVisibility(false);
                    }
                }
                removeHightlight();
                if(mSaveByGraffiti) {
                    mSaveByGraffiti = false;
                    clearAllRecordsWhenChangeCanvas(0, BrushConstant.getRandomColor());
                }
                break;
            case ImageEditConstants.ACTION_SAVED_FAILED:
                ImageEditOperationUtil.showToast(ImageEditControlActivity.this, R.string.edit_operation_failure_tip, Toast.LENGTH_SHORT);
                saveBtn.setEnabled(false);
                mLayoutSave.setEnabled(false);
                reDoBtn.setEnabled(false);
                mLayoutRedo.setEnabled(false);
                unDoBtn.setEnabled(false);
                mLayoutUndo.setEnabled(false);
                isClearing = true;
                mSaving = false;
                break;
            case ImageEditConstants.ACTION_UNLOAD_FAILED:
                ImageEditOperationUtil.showToast(ImageEditControlActivity.this, R.string.edit_operation_failure_tip, Toast.LENGTH_SHORT);
                reDoBtn.setEnabled(false);
                mLayoutRedo.setEnabled(false);
                unDoBtn.setEnabled(false);
                mLayoutUndo.setEnabled(false);
                isClearing = true;
                mSaving = false;
                break;
            case ImageEditConstants.ADJUST_BITMAP: {
                updatePreview(null);
                break;
            }
            case ImageEditConstants.INVISIBLE_ADJUST_CONTROL: {
                mAdjustProcess = false;
                mAdjustSclControl.adjustControlVisibility(false);
                break;
            }
            case ImageEditConstants.ACTION_SHARE_DIALOG: {
                mSaving = false;
                Uri uri = (Uri)msg.obj;
                /** FIX BUG: 6553
                 * BUG CAUSE:share current pic to gif play and gif edit
                 * FIX COMMENT: filter the gif play and gif edit at pic edit activity
                 * Date: 2011-12-08
                 */
                saveBtn.setEnabled(false);
                mLayoutSave.setEnabled(false);
                reDoBtn.setEnabled(false);
                mLayoutRedo.setEnabled(false);
                unDoBtn.setEnabled(false);
                mLayoutUndo.setEnabled(false);
                mImageEditDesc.clearQueueWithoutFisrtAndCurrent();
                if(mBrushPainting != null) {
                    mBrushPainting.clearAllStrokers();
                }
                if(mCurrentFuncMode == ACTION_BASIC_EDIT_FUNCTION) {
                    if(mAdjustSclControl != null) {
                        mAdjustSclControl.adjustControlVisibility(false);
                    }
                }
                removeHightlight();
               
                break;
            }
            case ImageEditConstants.UPDATE_RES_DECOR:
                mRefreshDecor = false;
                if (mCurrentFuncMode == ACTION_DECORATION) {
                    BaseAdapter adapter = adapterArray.get(R.id.btn_edit_decoration);
                    mSubMenuGridView.setAdapter(adapter);
                    int itemWidth = setDisplayItemCountsInWindow(adapter.getCount(), 5, false);
                    /*
                     * FIX BUG: 5599
                     * BUG COMMETN: The mSubHorizontalScrollView will scroll to the Correct position
                     * DATE: 2013-12-16
                     */
                    com.ucamera.ucomm.downloadcenter.UiUtils.scrollToCurrentPosition(mSubHorizontalScrollView, itemWidth, mSubMenuSelectedIndex);
                }
                break;
            case ImageEditConstants.UPDATE_RES_FRAME:
                mRefreshFrame = false;
                if (mCurrentFuncMode == ACTION_PHOTO_FRAME) {
                    BaseAdapter adapter = adapterArray.get(R.id.btn_edit_photoframe);
                    mSubMenuGridView.setAdapter(adapter);
                    int itemWidth = setDisplayItemCountsInWindow(adapter.getCount(), 5, false);
                    /*
                     * FIX BUG: 5599
                     * BUG COMMETN: The mSubHorizontalScrollView will scroll to the Correct position
                     * DATE: 2013-12-16
                     */
                    com.ucamera.ucomm.downloadcenter.UiUtils.scrollToCurrentPosition(mSubHorizontalScrollView, itemWidth, mSubMenuSelectedIndex);

                }
                break;
            case ImageEditConstants.UPDATE_RES_TEXTURE:
                mRefreshTexture = false;
                if (mCurrentFuncMode == ACTION_TEXTURE) {
                    BaseAdapter adapter = adapterArray.get(R.id.btn_edit_texture);
                    mSubMenuGridView.setAdapter(adapter);
                    int itemWidth = setDisplayItemCountsInWindow(adapter.getCount(), 5, false);
                    /*
                     * FIX BUG: 5599
                     * BUG COMMETN: The mSubHorizontalScrollView will scroll to the Correct position
                     * DATE: 2013-12-16
                     */
                    com.ucamera.ucomm.downloadcenter.UiUtils.scrollToCurrentPosition(mSubHorizontalScrollView, itemWidth, mSubMenuSelectedIndex);
                }
                break;
            case ImageEditConstants.ACTION_TAB_LABEL_UPDATE:
                int labelShape = msg.arg1;
                ViewAttributes attributes = (ViewAttributes)msg.obj;
                boolean needToParserXml = false;
                if((mLabelShape < 0 && labelShape < 0 && mLabelShape != labelShape) ||
                        (mLabelShape < 0 && labelShape >= 0) || (mLabelShape >= 0 && labelShape < 0)) {
                    needToParserXml = true;
                } else if(mCurrentLabelIndex == -1) {
                    needToParserXml = true;
                }

                Log.d(TAG, "ImageEditControlActivity.handleMessage(); mLabelShape = " + mLabelShape + ", mCurrentLabelIndex = "
                        + mCurrentLabelIndex + ", needToParserXml = " + needToParserXml + ", labelShape = " + labelShape);

                if(needToParserXml) {
                    if(labelShape == ImageEditConstants.LABEL_TITLE_SHAPE) {
                        mCurrentLabelIndex = ImageEditConstants.LABEL_TAG_TITLE_INDEX;
                    } else if(labelShape == ImageEditConstants.LABEL_LABEL_SHAPE) {
                        mCurrentLabelIndex = ImageEditConstants.LABEL_TAG_LABEL_INDEX;
                    } else {
                        mCurrentLabelIndex = ImageEditConstants.LABEL_TAG_BALLOON_INDEX;
                    }
                    setViewColorByXml(mCurrentLabelIndex);
                }

                mLabelShape = labelShape;
                mFromLabelModify = true;
                if(labelShape == ImageEditConstants.LABEL_TITLE_SHAPE) {
                    setCurrentAttrWhenShowTitleLayout();

                    mCurrentAttr = attributes;
                    String inputText = attributes.getDrawText();
                    mEditText.setText(inputText);
                    mTitleView.setTitleStyle(attributes, inputText, false, ImageEditConstants.LABEL_TAG_TITLE_INDEX);
                } else if(labelShape == ImageEditConstants.LABEL_LABEL_SHAPE) {
                    setCurrentAttrWhenShowLabelLayout();

                    mCurrentAttr = attributes;
                    String inputText = attributes.getDrawText();
                    mEditText.setText(inputText);
                    mLabelView.setLabelStyle(attributes, inputText, 90);
                } else {
                    setCurrentAttrWhenShowBalloonLayout();

                    mCurrentAttr = attributes;
                    String inputText = attributes.getDrawText();
                    mEditText.setText(inputText);
//                    mGalleryBalloonStyle.setSelection(mCurrentSelectedBalloonStyle);
                    mBalloonGallery.setSelection(mLabelShape);
                    mBalloonStyleAdapter.setStyleByAttribute(attributes);
                }
                break;
            case ImageEditConstants.LABEL_INPUT_TEXT_MESSAGE:
                /**
                 * BUG : 5998
                 * BUG CAUSE : restore the original text if the edittext is empty ;
                 * DATE : 2014-02-28
                 */
                String inputText = showTitleText();
                if (mCurrentAttr == null) {
                    break;
                }
                if(mTitleView.isShown()) {
                    mTitleView.setTitleStyle(mCurrentAttr, inputText, false, 0);
                } else if(mBalloonGallery.isShown()&&mCurrentAttr!=null) {
                    mCurrentAttr.setDrawText(inputText);
                    mBalloonStyleAdapter.setStyleByAttribute(mCurrentAttr);
                } else if(mLabelView.isShown()) {
                    mCurrentAttr.setDrawText(inputText);
                    mLabelView.setLabelStyle(mCurrentAttr, inputText, 90);
                }
                break;
            case ImageEditConstants.MAKEUP_FACE_DETECT:
                Bundle bundle = (Bundle) msg.obj;
                int[] face_num = bundle.getIntArray(ImageEditConstants.MAKEUP_EXTRA_FACE_NUM);
                if(mMakeupDlg != null && mMakeupDlg.isShowing()) {
                    mMakeupDlg.dismiss();
                }
                Point point1 = null;
                Point point2 = null;
                Point point3 = null;
                /**
                 * BUG : 4843 4797
                 * BUG CAUSE : Clicking the adjust button can not recognize the face point;
                 * DATE : 2013-09-09
                 */
                if(mMakeupControlView == null || mImageEditDesc.getBitmap() == null)
                    break;
                int bmWidth = (int)(mImageEditDesc.getBitmap().getWidth() * mMakeupControlView.getScale());
                int bmHeight = (int)(mImageEditDesc.getBitmap().getHeight() * mMakeupControlView.getScale());
                int offsetXBound = (centerLayout.getWidth() - bmWidth) / 2;
                int offsetYBound = (centerLayout.getHeight() - bmHeight) / 2;
                int offsetX = (centerLayout.getWidth() - mImageEditDesc.getBitmap().getWidth()) / 2;
                int offsetY = (centerLayout.getHeight() - mImageEditDesc.getBitmap().getHeight()) / 2;
                mMakeupFaceView.setFaceBound(offsetXBound, offsetYBound, bmWidth, bmHeight);
                // SPRD: makeupengine may return face_num[0] -1, in which case we'll get eye coordinate
                // from null array, then crash happens. Change '== 0' to '<= 0' here.
                if (face_num[0] <= 0) {
                    final String KEY = "pref_facedetect_hint_shown";
                    if (!mSharedPreferences.getBoolean(KEY, false)) {
                        mTipStubView.setVisibility(View.VISIBLE);
                        mTipStubViewHint = findViewById(R.id.makeup_facedetect_hint);
                        mTipStubViewHint.setOnTouchListener(new View.OnTouchListener( ) {
                            @Override
                            public boolean onTouch(View v, MotionEvent event) {
                                v.setVisibility(View.GONE);
                                return true;
                            }
                        });
                    }
                    mSharedPreferences.edit().putBoolean(KEY, true).commit();
                    makeUpAdjust();
                    point1 = new Point(centerLayout.getWidth() / 2 - 80, centerLayout.getHeight() / 2 - 80);
                    point2 = new Point(centerLayout.getWidth() / 2 + 80, centerLayout.getHeight()  / 2-80);
                    point3 = new Point(centerLayout.getWidth() / 2, centerLayout.getHeight()  / 2 + 80);
                    mMakeupFaceView.setFacePosition(point1, point2, point3);
                } else {
                    int[] face_points = bundle.getIntArray(ImageEditConstants.MAKEUP_EXTRA_FACE_RECT);
                    int[] eye_points  = bundle.getIntArray(ImageEditConstants.MAKEUP_EXTRA_EYE_POINTS);
                    int[] mouth_point = bundle.getIntArray(ImageEditConstants.MAKEUP_EXTRA_MOUTH_POINT);

                    point1 = new Point(eye_points[0] + offsetX, eye_points[1] + offsetY);
                    point2 = new Point(eye_points[2] + offsetX, eye_points[3] + offsetY);
                    point3 = new Point(mouth_point[0] + offsetX, mouth_point[1] + offsetY);

                    Rect[] face_rect = new Rect[] {
                        new Rect(face_points[0], face_points[1], face_points[2], face_points[3])
                    };

                    Matrix m = new Matrix();
                    m.postScale(mMakeupControlView.getScale(), mMakeupControlView.getScale(),
                            centerLayout.getWidth() / 2, centerLayout.getHeight() / 2);
                    float pts[] = {
                            point1.x, point1.y, point2.x, point2.y, point3.x, point3.y
                    };
                    m.mapPoints(pts);
                    mMakeupFaceView.setFacePosition(new Point((int)pts[0],(int)pts[1]),
                                                    new Point((int)pts[2],(int)pts[3]),
                                                    new Point((int)pts[4],(int)pts[5]));
                    for ( int i =0; i< face_rect.length; i++) {
                        pts = new float[] {
                            face_rect[i].left,
                            face_rect[i].top,
                            face_rect[i].right,
                            face_rect[i].bottom
                        };
                        m.mapPoints(pts);
                        face_rect[i] = new Rect((int)pts[0],(int)pts[1],(int)pts[2],(int)pts[3]);
                    }
                    mRectView.setVisibility(View.VISIBLE);
                    mRectView.SetRect(face_rect, 1);
                    int[] radiusGroup = new int[5]; //The five deblemish radius
                    int lrclen = face_rect[0].width(); //detect the face's width
                    int lscale = 64;
                    int nReduce = 1;

                    while (lscale <= lrclen) {
                       lscale <<= 1;
                       nReduce += 1;
                    }
                    nReduce = (nReduce + 1) & 0xfffffffe;
                    radiusGroup[0] = (1 * nReduce) * 3 / 2;
                    radiusGroup[1] = (2 * nReduce);
                    radiusGroup[2] = (3 * nReduce) * 4 / 5;
                    radiusGroup[3] = (4 * nReduce) * 2 / 3;
                    radiusGroup[4] = (5 * nReduce) * 3 / 5;
                    mMakeupControlView.setRadius(radiusGroup);
                  }
                break;
            case ImageEditConstants.MAKEUP_PROCESS_RESULT:
                Bitmap bitmap = (Bitmap) msg.obj;
                int result = msg.arg1;
                /* FIX BUG : 4242
                 * BUG COMMENT : avoid null point
                 * DATE : 2013-07-03
                 */
                if(mMakeupControlView != null) {
                    mMakeupControlView.setImageBitmap(bitmap, mImageEditDesc.getBitmap());
                }
                if(mMakeupDlg != null && mMakeupDlg.isShowing()) {
                    mMakeupDlg.dismiss();
                }
                if(result == ImageEditConstants.MAKEUP_MSG_REDO_ENABLE) {
//                    if(!saveBtn.isEnabled()){
//                        saveBtn.setEnabled(true);
//                        mLayoutSave.setEnabled(true);
//                    }
//                    unDoBtn.setEnabled(true);
//                    unDoBtn.setPressed(false);
//                    mLayoutUndo.setEnabled(true);
//
//                    int queueCount = mImageEditDesc.getqueueSize();
//                    int curBpIndex = mImageEditDesc.getCurrentBitmapIndex();
//
//                    int stackCount = mMakeupControlView.getMakeupStackCount();
//                    if(stackCount > 0) {
//                        mMakeupControlView.clearMakeupStack();
//                        reDoBtn.setEnabled(false);
//                        mLayoutRedo.setEnabled(false);
//                    }
//                    if(curBpIndex >= 0 && curBpIndex < queueCount - 1) {
//                        mImageEditDesc.clearReDoQueue();
//                    }
                }
                break;
            case ImageEditConstants.MAKEUP_PROCESS_RESULT_WARN_1:
                if(mMakeupDlg != null && mMakeupDlg.isShowing()) {
                    mMakeupDlg.dismiss();
                }
                String str = (String) msg.obj;
                Toast.makeText(ImageEditControlActivity.this, str, Toast.LENGTH_LONG).show();
                break;
            case ImageEditConstants.MAKEUP_RESET_PARAM:
                boolean toSave = (Boolean) msg.obj;
                mSubMenuGridView.setEnabled(true);
                if (mMakeupControlView != null) {
                    mMakeupControlView.adjustControlVisibility(false);
                    mRectView.setVisibility(View.GONE);
                    mMakeupControlView.reset();
                    mMakeupControlView.resetPopupWindow();
                    boolean undoOrRedo = getUnDoOrReDo();
                    if(undoOrRedo || toSave) {
//                        mMakeupControlView.detectFaceRectView(mImageEditDesc.getBitmap(), !toSave);
                        mMakeupControlView.resetEffectBitmap(mImageEditDesc.getBitmap());
                    } else {
                        mMakeupControlView = null;
                    }
                }
                break;
            case ImageEditConstants.HANDLER_MSG_SHOW_DLG:
                if(mMakeupDlg != null && !mMakeupDlg.isShowing()) {
                    mMakeupDlg.show();
                }
            case ImageEditConstants.MAKEUP_GET_PREVIEW_SIZE:
                if(mMakeupControlView != null)
                    mMakeupControlView.setPreviewDimension(centerLayout.getWidth(),centerLayout.getHeight());
                break;
            case ImageEditConstants.UPDATE_LABEL_BITMAP:
                updateLabelBitmap();
                break;
            case ImageEditConstants.CHANGE_LIKED_EFFECT:
                int length = mEffectResource.getLikedList().size();
                getScrnNumber(length);
                setDisplayItemCountsInWindow(length, perScrnCount, true);
                EffectCateBaseAdapter adapter=new EffectCateBaseAdapter(ImageEditControlActivity.this,
                      mEffectResource.getLikedList());
                adapterArray.put(mEffectBtn.getId(), adapter);
                mSubMenuGridView.setAdapter(adapter);
                if(mCurrentItemIndex >= length-1) {
                    mSubHorizontalScrollView.scrollTo(0, mSubHorizontalScrollView.getScrollY());
                    adapter.setHighlight(0);
                    handleFunEffect(0);
                } else {
                    adapter.setHighlight(mCurrentItemIndex);
                    handleFunEffect(mCurrentItemIndex);
                }
                break;
             case ImageEditConstants.EFFECT_SELECTED_BACK:
                 /*
                  * FIX BUG: 5589
                  * BUG COMMETN: The mSubHorizontalScrollView will scroll to the Correct position
                  * DATE: 2013-12-13
                  */
                 com.ucamera.ucomm.downloadcenter.UiUtils.scrollToCurrentPosition(mSubHorizontalScrollView, (int)((1.0f + 0.1f) * UiUtils.effectItemWidth()), mCurrentItemIndex);
                BaseAdapter bAdapter=adapterArray.get(mEffectBtn.getId());
                if(bAdapter != null)
                    bAdapter.notifyDataSetChanged();
                /*
                 * FIX BUG: 5655
                 * BUG COMMETN: operation back, effect not repeat
                 * DATE: 2013-12-20
                 */
//                handleFunEffect(mCurrentItemIndex);
                setEffectCatHightlight(mCurrentItemIndex);
                break;
             case ImageEditConstants.EXCHANGE_LIKED_EFFECT:
                mSubHorizontalScrollView.scrollTo(mCurrentItemIndex, mSubHorizontalScrollView.getScrollY());
                BaseAdapter baseAdapter=adapterArray.get(mEffectBtn.getId());
                if(baseAdapter != null)
                    baseAdapter.notifyDataSetChanged();
                handleFunEffect(mCurrentItemIndex);
                break;
            default:
                break;
            }
            mHandler.removeMessages(msg.what);
            return true;
        }
    });

    private void updateLabelBitmap() {
        if((mCurrentFuncMode == ACTION_EDIT_TAB_LABEL ||
                mCurrentFuncMode == ACTION_EDIT_TEXT ||
                mCurrentFuncMode == ACTION_TEXT_BUBBLE) && centerLayout.getChildCount() > 1) {
                    addLabelBitmap();
                }
    }

    public void makeUpAdjust() {
        mSubMenuGridView.setEnabled(true);
        setMakeupHightlight(-1);
        mMakeupControlView.adjustControlVisibility(false);
        if (mMakeupFaceView != null) {
            mMakeupAdjustBtn.setEnabled(false);
            /*
             * FIX BUG: 5488
             * BUG COMMETN: java.lang.IllegalStateException: The specified child already has a parent.
             *  You must call removeView() on the child's parent first
             * DATE: 2013-12-09
             */
            centerLayout.removeView(mMakeupFaceView);
            centerLayout.addView(mMakeupFaceView, new FrameLayout.LayoutParams(
                    FrameLayout.LayoutParams.MATCH_PARENT,
                    FrameLayout.LayoutParams.MATCH_PARENT));
            mMakeupFaceView.setVisibility(View.VISIBLE);
        }
    }

    public void replaceImage() {
        float scale = mMakeupControlView.getScale();
        int offsetX = (centerLayout.getWidth() - mImageEditDesc.getBitmap().getWidth())/2;
        int offsetY = (centerLayout.getHeight() - mImageEditDesc.getBitmap().getHeight())/2;
        Point eye1 = mMakeupFaceView.getPoint1();
        Point eye2 = mMakeupFaceView.getPoint2();
        Point mouth = mMakeupFaceView.getPoint3();
        Matrix m = new Matrix();
        m.postScale(1/scale, 1/scale, centerLayout.getWidth()/2, centerLayout.getHeight()/2);
        m.postTranslate(-offsetX, -offsetY);
        float[] pts = {
                eye1.x, eye1.y,
                eye2.x, eye2.y,
                mouth.x, mouth.y
        };
        m.mapPoints(pts);
        eye1 = new Point((int) pts[0], (int)pts[1]);
        eye2 = new Point((int) pts[2], (int)pts[3]);
        mouth = new Point((int) pts[4], (int)pts[5]);

        Rect new_rect = new Rect(
                Math.min(eye1.x, Math.min(eye2.x, mouth.x)),
                Math.min(eye1.y, Math.min(eye2.y, mouth.y)),
                Math.max(eye1.x, Math.max(eye2.x, mouth.x)),
                Math.max(eye1.y, Math.max(eye2.y, mouth.y)));
        Rect[] oldFace_rect = new Rect[] { mMakeupFaceView.getRect() };
        Bitmap bitmap = mMakeupControlView.getEngineLoadBitmap();
        if (bitmap == null) {
            bitmap = mImageEditDesc.getBitmap();
        }
        MakeupEngine.ReplaceImage(
                /*mImageEditDesc.getBitmap()*/bitmap,
                new int[]   {MakeupEngine.Max_FaceNum },
                new Rect[]  {new_rect},
                new Point[] {eye1, eye2},
                new Point[] {mouth}
        );
        mRectView.setVisibility(View.VISIBLE);
        mRectView.SetRect(oldFace_rect, 1);
        int[] radiusGroup = new int[5]; // The five deblemish radius
        int lrclen = oldFace_rect[0].width(); // detect the face's width
        int lscale = 64;
        int nReduce = 1;

        while (lscale <= lrclen) {
            lscale <<= 1;
            nReduce += 1;
        }
        nReduce = (nReduce + 1) & 0xfffffffe;
        radiusGroup[0] = (1 * nReduce) * 3 / 2;
        radiusGroup[1] = (2 * nReduce);
        radiusGroup[2] = (3 * nReduce) * 4 / 5;
        radiusGroup[3] = (4 * nReduce) * 2 / 3;
        radiusGroup[4] = (5 * nReduce) * 3 / 5;
        mMakeupControlView.setRadius(radiusGroup);
        centerLayout.removeView(mMakeupFaceView);
    }

    private FunModeImageProcess getFunModeImageProcess() {
        if(mFunModeImageProcess == null) {
//            mFunModeImageProcess = new FunModeImageProcess(mEffectThumbnailsWidth);
            mFunModeImageProcess = new FunModeImageProcess(1);
        }
        return mFunModeImageProcess;
    }

    private boolean mIsUnDoOrReDo = false;
    private void setIsUnDoOrReDo(boolean isUnDoOrReDo) {
        mIsUnDoOrReDo = isUnDoOrReDo;
    }

    private boolean getUnDoOrReDo() {
        return mIsUnDoOrReDo;
    }

    private void exitToLogin() {
        if(mIsFromLaunch) {
            if(mLoginView != null && !mLoginView.isShown()) {
                mIsOpenPhoto = true;
                hideResourceGridView();
                warnMessageForOpenPhoto();
                centerLayout.removeAllViews();
                mImageEditDesc.releaseTempBitmap();
            }
            mImageEditDesc.clearAllQueue();
            mCurrentFuncMode = -1;
        } else {
            /**
             * FIX BUG: 1190
             * BUG CAUSE: When release the bitmap, the view being used it
             * FIX COMMENT: At First remove all views and then release bitmap.
             * DATE: 2012-07-16
             */
            centerLayout.removeAllViews();
            mImageEditDesc.clearAllQueue();
            ImageEditControlActivity.this.finish();
        }
    }

    /**
     * calculate the degerr by imageUri.
     * @param imageUri current image uri, contains content and file
     * @return current image degree.
     */
    private int calculateAngle(String imagePath) {
        Log.d(TAG, "calculateAngle(): imagePath = " + imagePath);
        if (imagePath == null) {
            return 0;
        }
        int degree = 0;
        try {
            ExifInterface exif = new ExifInterface(imagePath);
            int orientation = exif.getAttributeInt(ExifInterface.TAG_ORIENTATION,
                                                   ExifInterface.ORIENTATION_NORMAL);
            degree = 360 - mImageEditDesc.getDegree(orientation);
            Log.d(TAG, "calculateAngle(): orientation = " + orientation + ", degree = " + degree);
        } catch(IOException ioe) {
            Log.e(TAG, "calculateAngle(): exifInterface parser " + imagePath + " exception.");
        }
        return degree;
    }


    @Override
    public boolean dispatchTouchEvent(MotionEvent m) {
        if (m.getAction() != MotionEvent.ACTION_DOWN){
            if (!(mGestureDetector != null && mGestureDetector.onTouchEvent(m))){
                super.dispatchTouchEvent(m);
            }
        } else{
            super.dispatchTouchEvent(m);
            if (mGestureDetector != null) {
                mGestureDetector.onTouchEvent(m);
            }
        }
        return true;
    }

    private class MyGestureListener extends  GestureDetector.SimpleOnGestureListener {
        // represent this down-move... serial of touch actions should be or not be processed in the aftermath
        boolean canTouchAction = false;

        @Override
        public boolean onDown(MotionEvent e) {
            /* FIX BUG : 4318
             * BUG COMMENT : the requirement has changed,remove useless code
             * DATE : 2013-06-21
             */
            /*if(mLabelLayout.isShown() || mBrushRootLayout.isShown()) {
                if(mLayoutRect != null && !mLayoutRect.contains((int)Math.ceil(e.getRawX()), (int)Math.ceil(e.getRawY()))) {
                    mLabelLayout.setVisibility(View.GONE);
                    mBrushRootLayout.setVisibility(View.GONE);
                }
            } else {
                canTouchAction = true;
            }*/
            canTouchAction = true;
            return true;
        }

        @Override
        public boolean onScroll(MotionEvent e1,
                MotionEvent e2, float distanceX, float distanceY) {
            if (!canTouchAction) {
                return true;
            }
            float y1 = e1.getY();
            float y2 = e2.getY();
            float x1 = e1.getX();
            float x2 = e2.getX();
            boolean isVerticalScroll = (Math.abs(x2-x1) - Math.abs(y2-y1)) <= 0;
            if (!isVerticalScroll){
                return false;
            }
            float topViewHeight = (mTopSettingBar != null) ? mTopSettingBar.getHeight() : 0;
            boolean topSettingShown = mTopSettingBar.isShown();

            topViewHeight = topViewHeight + 70 * UiUtils.screenDensityDPI()/DisplayMetrics.DENSITY_MEDIUM;

            if (y2 < y1 && y1 < topViewHeight && topSettingShown){
                // pull up and the area of touch is top: hide the settings
//                dismissMenuView();
                canTouchAction = false;
                return true;
            }
            else if (y2 > y1 && topViewHeight > y2 && !topSettingShown){
                // pull down and the area of touch is top: display the settings
//                showMenuView();
                canTouchAction = false;
                return true;
            }
            return false;
        }
    }

    private void showMenuView(){
        setTopSettingOpHintSrc(R.drawable.ic_top_packup);
        if (mTopSettingOpHintViewOut != null){
            mTopSettingOpHintViewOut.setVisibility(View.GONE);
        }
        Utils.showViewAnimation(mTopSettingBar);
    }

    protected void setTopSettingOpHintSrc(int resId){
        if (mTopSettingOpHintViewOut != null) {
            mTopSettingOpHintViewOut.setImageResource(resId);
        }
    }

    private void dismissMenuView() {
        setTopSettingOpHintSrc(R.drawable.ic_top_unfold);
        if (mTopSettingBar.isShown()) {
            Utils.dismissViewAnimation(mTopSettingBar, true, new AnimationListener() {
                @Override
                public void onAnimationStart(Animation animation) {
                }
                @Override
                public void onAnimationRepeat(Animation animation) {
                }
                @Override
                public void onAnimationEnd(Animation animation) {
                    mTopSettingBar.setVisibility(View.GONE);
                    mTopSettingOpHintViewOut.setVisibility(View.VISIBLE);
                }
            });
        }
    }

    private class FontOnItemListener implements OnItemClickListener{
        public void onItemClick(AdapterView<?> adapterView, View view, int position, long id) {
            String inputText = showTitleText();
            //download btn is the last one
            if (position == 0 && DownloadCenter.RESOURCE_DOWNLOAD_ON) {
                DownloadCenter.openResourceCenter(ImageEditControlActivity.this, Const.EXTRA_FONT_VALUE);
                return;
            }
            String font = mFontArray[position];
            mCurrentAttr.setFont(font);
            if(mTitleView.isShown()) {
                mTitleView.setTitleStyle(mCurrentAttr, inputText, false, 0);
            } else if(mBalloonGallery.isShown()) {
                //mBalloonView.setBalloonStyle(mCurrentAttr, inputText, true);
                mCurrentAttr.setDrawText(inputText);
                mBalloonStyleAdapter.setStyleByAttribute(mCurrentAttr);
            } else if(mLabelView.isShown()) {
                mLabelView.setLabelStyle(mCurrentAttr, inputText, 90);
            }
            //1.TitleView, color and preview; 2.LabelView, style and preview; 3.BalloonView, color and preview
            mColorAdapter.setFont(font);

        }
    }

    private class ColorOnItemListener implements OnItemClickListener {
        public void onItemClick(AdapterView<?> adapterView, View view, int position, long id) {
            String inputText = showTitleText();
            if(mLabelView.isShown()) {
                ViewAttributes attributes = mAttrList.get(position);
                mCurrentAttr = attributes;
                mLabelView.setLabelStyle(attributes, inputText, 90);
                mStatCurrentLabelIndex = position;
            } else {
                mCurrentAttr = mAttrList.get(position);
                if(mTitleView.isShown()) {
                    mTitleView.setTitleStyle(mCurrentAttr, inputText, false, 0);
                    mStatCurrentTitleIndex = position;
                } else if(mBalloonGallery.isShown()) {
                    mCurrentAttr.setTextSize(String.valueOf(mCurrentBalloonTextSize));
                    mCurrentAttr.setDrawText(inputText);
                    mBalloonStyleAdapter.setStyleByAttribute(mCurrentAttr);

                    mStatCurrentColorIndex = position;
                }
            }
        }
    }

    private class BalloonStyleOnItemSelectedListener implements OnItemSelectedListener {
        public void onItemSelected(AdapterView<?> adapterView, View view, int position, long id) {
            mCurrentSelectedBalloonStyle = position;
            mLabelShape = mCurrentSelectedBalloonStyle;

            mStatCurrentBalloonIndex = position;
        }

        public void onNothingSelected(AdapterView<?> adapterView) {

        }
    }

    private class BalloonSizeOnSeekBarChangeListener implements OnSeekBarChangeListener {
        private int progress;
        private static final int MIN_SIZE = 12;
        private static final int STEP = 2;

        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            this.progress = progress;
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {

        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
            mCurrentBalloonTextSize = getFontSize();
            if (mCurrentAttr != null) {
                mCurrentAttr.setTextSize(String.valueOf(mCurrentBalloonTextSize));
                mBalloonStyleAdapter.setStyleByAttribute(mCurrentAttr);
            }
        }

        private int getFontSize() {
            return MIN_SIZE + progress * STEP;
        }
    }

    private boolean mLabelDeleteVisible = false;
    private class LabelHistoryOnClickListener implements OnItemClickListener {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position,
                long id) {
            if(mLabelDeleteVisible) {
                deleteCommonTitle(mLabelHistoryAdapter, position);
                //refreshTabLabelItem(mLabelDeleteVisible);
            }else {
                if(mLabelList != null && mLabelList.size() > 0) {
                    if(mLabelLayout.isShown()) {
                        mLabelLayout.setVisibility(View.GONE);
                    }
                    Object object = mLabelList.get(position);
                    if(object instanceof ViewAttributes) {
                        mCurrentAttr = (ViewAttributes) object;
                        mCurrentSelectedBalloonStyle = mCurrentAttr.getBalloonStyle();
                        if(mCurrentAttr != null) {
                            showLabelOnPreview(mCurrentAttr, position, mViewTag);
                        }
                        dismissSubMenuGridView();
                    }
                }
            }
        }

    }

    private class LabelHistoryOnLongClickListener implements OnItemLongClickListener {
        @Override
        public boolean onItemLongClick(AdapterView<?> parent, View view,
                int position, long id) {
            if(!mLabelDeleteVisible){
                if(mLabelHistoryAdapter instanceof MyCommonTitleBaseAdapter) {
                    mLabelDeleteVisible = true;
                    ((MyCommonTitleBaseAdapter)mLabelHistoryAdapter).setDeleteViewVisible(true);
                }else if(mLabelHistoryAdapter instanceof HistoryLabelAdapter) {
                    mLabelDeleteVisible = true;
                    ((HistoryLabelAdapter)mLabelHistoryAdapter).setDeleteView(true);
                }
            }
            return false;
        }
    }

    /**
     *
     * @param tabStyle current style contains Title, Balloon, Label
     * @return
     */
    private String showTitleText() {
        String inputText = "Preview";
        Editable editable = mEditText.getText();
        if(editable != null && editable.toString().trim().length() > 0) {
            inputText = editable.toString().trim();
        }

        return inputText;
    }

    private void setCurrentAttrWhenShowTitleLayout() {
        mViewTag = ImageEditConstants.LABEL_TAG_TITLE;
        mLabelShape = ImageEditConstants.LABEL_TITLE_SHAPE;
        mBalloonGallery.setVisibility(View.GONE);
        mLabelView.setVisibility(View.GONE);
        //mBalloonSizeLayout.setVisibility(View.GONE);
        mBalloonSizeSeekBar.setVisibility(View.GONE);
        mTitleView.setVisibility(View.VISIBLE);
        //mBtnSaveAs.setVisibility(View.VISIBLE);

        Utils.showAnimation(mLabelLayout, new LayoutAnimListener(mLabelLayout));
    }

    private void setCurrentAttrWhenShowBalloonLayout() {
        mViewTag = ImageEditConstants.LABEL_TAG_BALLOON;
        mBalloonGallery.setVisibility(View.VISIBLE);
        //mBalloonSizeLayout.setVisibility(View.VISIBLE);
        mBalloonSizeSeekBar.setVisibility(View.VISIBLE);
        mBalloonSizeSeekBar.setProgress(9);
        mTitleView.setVisibility(View.GONE);
        //mBtnSaveAs.setVisibility(View.GONE);
        mLabelView.setVisibility(View.GONE);

        Utils.showAnimation(mLabelLayout, new LayoutAnimListener(mLabelLayout));
    }

    private void setCurrentAttrWhenShowLabelLayout() {
        mViewTag = ImageEditConstants.LABEL_TAG_LABEL;
        mLabelShape = ImageEditConstants.LABEL_LABEL_SHAPE;
        mTitleView.setVisibility(View.GONE);
        mBalloonGallery.setVisibility(View.GONE);
        //mBtnSaveAs.setVisibility(View.GONE);
        //mBalloonSizeLayout.setVisibility(View.GONE);
        mBalloonSizeSeekBar.setVisibility(View.GONE);
        mLabelView.setVisibility(View.VISIBLE);

        Utils.showAnimation(mLabelLayout, new LayoutAnimListener(mLabelLayout));
    }

    private Rect mLayoutRect;
    private class LayoutAnimListener implements AnimationListener {
        private View mAnimListenerView;

        public LayoutAnimListener(View view) {
            mAnimListenerView = view;
        }

        @Override
        public void onAnimationStart(Animation animation) {
            if(mLayoutRect != null) {
                mLayoutRect.setEmpty();
            } else {
                mLayoutRect = new Rect();
            }
        }

        @Override
        public void onAnimationEnd(Animation animation) {
            // SPRD Bug:512895 mLayoutRect may be null when monkey test.
            if (mLayoutRect != null)
                mAnimListenerView.getHitRect(mLayoutRect);
        }

        @Override
        public void onAnimationRepeat(Animation animation) {

        }
    };

    private TextWatcher mTextWatcher = new TextWatcher() {
        public void beforeTextChanged(CharSequence s, int arg1, int arg2,
                int arg3) {
        }

        public void onTextChanged(CharSequence s, int arg1, int arg2,
                int arg3) {
            mHandler.removeMessages(ImageEditConstants.LABEL_INPUT_TEXT_MESSAGE);
            mHandler.sendEmptyMessageDelayed(ImageEditConstants.LABEL_INPUT_TEXT_MESSAGE, 500);
        }

        @Override
        public void afterTextChanged(Editable s) {
            /*
             * FIX BUG:6076
             * BUG CAUSE: set mLabelOk or mLayoutOk disable when text length is 0;
             * DATA :2014-03-05
             */
            int textLength = s.toString().length();
            mLabelOk.setEnabled(textLength != 0);
            mLayoutOk.setEnabled(textLength != 0);
        }
    };

//    private class EffectOnItemSelectedListener implements OnItemSelectedListener {
//
//        @Override
//        public void onItemSelected(AdapterView<?> adapterView, View view, int position, long id) {
//            if(mCurrentFunModePost == position) {
//                return;
//            }
//            mCurrentFunModePost = position;
//            if(mEffectTypeAdapter != null) {
//                mEffectTypeAdapter.setHighlight(position);
//            }
//            handlePostFunEffect(position);
//        }

//        @Override
//        public void onNothingSelected(AdapterView<?> arg0) {
//
//        }
//
//    }

    private class BrushSizeSeekBarListener implements android.widget.SeekBar.OnSeekBarChangeListener {
        private boolean mIsEraser;
        public BrushSizeSeekBarListener(boolean isEraser) {
            mIsEraser = isEraser;
        }
        public void onProgressChanged(SeekBar seekbar, int progress, boolean flag) {
            if(mBrush != null) {
                mBrushSize = (progress * 1F) / 10F + mBrush.mBrushMinSize;
                if(!mIsEraser) {
                    mBrush.setSize(mBrushSize);
                    mBrushListAdapter.updateSize(mBrushSize);
                }
            }
        }

        public void onStartTrackingTouch(SeekBar seekbar) {

        }

        public void onStopTrackingTouch(SeekBar seekbar) {
            if(mIsEraser) {
                mBrushPainting.setBrushSize(mBrushSize);
            }
        }
    }

    private class BrushOnItemSelectedListener implements OnItemSelectedListener {
        @Override
        public void onItemSelected(AdapterView<?> adapterView, View view, int position, long id) {
            mBrushMode = BrushConstant.BrushModeSelected;
            setPreviewBrush(mBrushStyles[position]);
            setSizeSeekBar(false);
            mStatBrushIndex = position;
        }

        @Override
        public void onNothingSelected(AdapterView<?> arg0) {

        }
    }

    private class BrushHistoryOnItemClickListener implements OnItemClickListener {

        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position,
                long id) {
            if(isLongClickStatus){
                if(mBrushHistoryArrayList != null) {
                    deleteCommonBrush(mGraffitiHistoryAdapter, position);
                }
            }else{
                if(mBrushHistoryArrayList != null && mBrushHistoryArrayList.size() > 0) {
                    Object object = mBrushHistoryArrayList.get(position);
                    if(object instanceof BrushItemInfo) {
                        BrushItemInfo info = (BrushItemInfo) object;
                        String brushColor = info.brushColor;
                        mBrushStyle = info.brushStyle;
                        mBrushPainting.setBrushStyle(mBrushStyle);
                        if(brushColor.startsWith("#")) {
                            mBrushPainting.setBrushColor(Color.parseColor(brushColor));
                        } else {
                            mBrushPainting.setBrushColor(Integer.valueOf(brushColor));
                        }
                        mBrushPainting.setBrushSize(info.brushSize);
                        mBrushPainting.setBrushMode(info.brushMode);
                        mBrushPainting.setAlpha(255); //255: default

                        if(mEraserLayout.isShown()) {
                            mEraserLayout.setVisibility(View.GONE);
                        }
                    }

                    if(mBrushRootLayout.isShown()) {
                        mBrushRootLayout.setVisibility(View.GONE);
                    }
                }
            }
        }
    }

    private class BrushHistoryOnItemLongClickListener implements OnItemLongClickListener {
        @Override
        public boolean onItemLongClick(AdapterView<?> parent, View view,
                int position, long id) {
            if(!isLongClickStatus){
                mGraffitiHistoryAdapter.setDeleteViewVisible(true);
                isLongClickStatus = true;
                return true;
            }
            return false;
        }

    }

    private class BrushOnColorChangedListener implements OnColorChangedListener {
        @Override
        public void onColorChanged(int color) {
            mBrushColor = color;
            /*
             * FIX BUG: 6155
             * BUG COMMENT: avoid null pointer exception
             * DATE: 2014-03-25
             */
            if(mBrush != null) {
                mBrush.setColor(color);
            }
            if(mBrushListAdapter != null) {
                mBrushListAdapter.updateColor(color);
            }
        }
    }

    private void printLog(String tag, String message) {
        boolean isDebug = true;
        if(isDebug) {
            Log.d(tag, message);
        }
    }
    private void resetMakeupParam(boolean toSave) {
        compositeMakeup();

        Message msg = new Message();
        msg.what = ImageEditConstants.MAKEUP_RESET_PARAM;
        msg.obj = toSave;
        mHandler.sendMessage(msg);
    }

    private void compositeMakeup() {
        boolean undoRedo = getUnDoOrReDo();
        if(mMakeupControlView != null) {
            if(undoRedo) {
                mMakeupControlView.setImageBitmap(mImageEditDesc.getBitmap());
            } else {
                mMakeupControlView.clearMakeupStack();
                Bitmap effectBitmap = mMakeupControlView.getEffectBitmap();
                if(effectBitmap != null && !effectBitmap.isRecycled()) {
//                    updateImageEditBitmap(effectBitmap);
                    setCurrentBitmap(effectBitmap);
                }
            }
        }
    }
    private Bitmap updateImageEditBitmap(Bitmap bitmap) {
        if(bitmap == mImageEditDesc.getBitmap()){
            return bitmap;
        }
        return mImageEditDesc.updateBitmap(this, bitmap);
    }

    private void loadDecor() {
        DownloadCenter.loadDecor(this, new OnLoadCallback() {
            @Override
            public void onLoadComplete(String[] result) {
                if(result != null && result.length > 0) {
                    mDecorImage = result;
                    getResAdapter(mDecorImage, R.id.btn_edit_decoration, Const.mDecorDirectory, mRefreshDecor, ImageEditConstants.UPDATE_RES_DECOR);
                }
            }
        });
    }

    private void loadPhotoFrame() {
        DownloadCenter.loadPhotoFrame(this, new OnLoadCallback() {
            @Override
            public void onLoadComplete(String[] result) {
                if(result != null && result.length > 0) {
                    mPhotoFrameImage = result;
                    getResAdapter(mPhotoFrameImage, R.id.btn_edit_photoframe, Const.mPhotoFrameDirectory, mRefreshFrame, ImageEditConstants.UPDATE_RES_FRAME);
                }
            }
        });
    }

    private void loadTexture() {
        DownloadCenter.loadTexture(this, new OnLoadCallback() {
            @Override
            public void onLoadComplete(String[] result) {
                if(result != null && result.length > 0) {
                    mTextureImage = result;
                    getResAdapter(mTextureImage, R.id.btn_edit_texture, Const.mTextureDirectory, mRefreshTexture, ImageEditConstants.UPDATE_RES_TEXTURE);
                }
            }
        });
    }

    private void loadMosaic() {
        DownloadCenter.loadMosaic(this, new OnLoadCallback() {
            @Override
            public void onLoadComplete(String[] result) {
                if(result != null && result.length > 0) {
                    mMosaicImage = result;
                    getResAdapter(mMosaicImage, R.id.btn_edit_mosaic, Const.mMosaicDirectory, mRefreshMosaic, ImageEditConstants.UPDATE_RES_MOSAIC);
                }
            }
        });
    }

    private BaseAdapter getResAdapter(final String[] resNames, int adapterId, final String resType, boolean refreshRes, int what) {
        BaseAdapter ba = adapterArray.get(adapterId);
        if (ba != null && resNames != null && !refreshRes) {
//            int length = resNames.length;
//            setDisplayItemCountsInWindow(mSubMenuGridView, length, 5);
            return ba;
        }

//        if(resNames != null) {
//            int length = resNames.length;
//            setDisplayItemCountsInWindow(mSubMenuGridView, length, 5);
//        }

        DecorsAdapter decorsAdapter = new DecorsAdapter(this, resNames, resType);
        /*
         * FIX BUG:4878
         * BUG CAUSE:don't save selected index when come back from other Activity in photo frame mode
         * BUG COMMENT:save current index before launch other Activity
         * DATA :2013.11.08
         */
        decorsAdapter.setSelected(mSubMenuSelectedIndex);
        adapterArray.put(adapterId, decorsAdapter);
        if(refreshRes == true) {
            mHandler.sendEmptyMessage(what);
        }
        return decorsAdapter;
    }

    private void removeHightlight() {
        if(mCurrentFuncMode == ACTION_BASIC_EDIT_FUNCTION) {
            setBasicEditHightlight(-1);
        } else if(mCurrentFuncMode == ACTION_EDIT_EFFECT) {
            setEffectCatHightlight(-1);
        } else if(mCurrentFuncMode == ACTION_EDIT_MAKEUP) {
            setMakeupHightlight(-1);
        }
    }

    private void setBasicEditHightlight(int pos) {
        BasicEditingSimpleAdapter adapter = (BasicEditingSimpleAdapter)adapterArray.get(mEditBtn.getId());
        if(adapter != null) {
            pos = pos > 2 ? -1 : pos;
            adapter.setHighlight(pos);
        }
    }

    private void setMakeupHightlight(int position) {
        MakeupItemSimpleAdapter adapter = (MakeupItemSimpleAdapter)adapterArray.get(mMakeupBtn.getId());
        if(adapter != null) {
            adapter.setHighLight(position);
        }
    }

//        EffectCateSimpleAdapter adapter = (EffectCateSimpleAdapter)adapterArray.get(mEffectBtn.getId());
    private void setEffectCatHightlight(int pos) {
        EffectCateBaseAdapter adapter = (EffectCateBaseAdapter)adapterArray.get(mEffectBtn.getId());
        if(adapter != null) {
            adapter.setHighlight(pos);
            //mGpuProcees.setShaderType(adapter.getType());
//            mGpuProcees.setShaderType("effect_gouache");
        }
    }

    private Pair<Integer, Integer> getGraffitiHW() {
        String targetSize = mSharedPreferences.getString(ImageEditConstants.PREF_UPHOTO_PICTURE_SIZE_KEY, getString(R.string.text_uphoto_picture_size_default_value));
        targetSize = targetSize.trim();
        int dimIndex = targetSize.indexOf("x");
        int newWidth = 0;
        int newHeight = 0;
        if(dimIndex < 0) {
            Log.w(TAG, "dimIndex less than zero, bad target picture size: " +  targetSize);
            newWidth = ImageEditConstants.EDIT_VIEW_SIZE_SHORT;
            newHeight = ImageEditConstants.EDIT_VIEW_SIZE_LONG;
        } else {
            try {
                //1280x720, 800x600, 640x480
                newWidth = Integer.parseInt(targetSize.substring(dimIndex + 1)); //short 720/600/480
                newHeight = Integer.parseInt(targetSize.substring(0, dimIndex)); //long  1280/800/640
            } catch (NumberFormatException nfe) {
                Log.w(TAG, "NumberFormatException, bad target picture size: " + targetSize);
                newWidth = ImageEditConstants.EDIT_VIEW_SIZE_SHORT;
                newHeight = ImageEditConstants.EDIT_VIEW_SIZE_LONG;
            }
        }
        return Pair.create(newWidth, newHeight);
    }

    public static Animation createTranslateAnimation(Context context,float x_from,float x_to,float y_from,float y_to, long duration){
        Animation anim = null;
        anim = new TranslateAnimation(Animation.RELATIVE_TO_SELF, x_from, Animation.RELATIVE_TO_SELF, x_to, Animation.RELATIVE_TO_SELF, y_from, Animation.RELATIVE_TO_SELF, y_to);
        anim.setDuration(duration);
        anim.setFillAfter(false);
        return anim;
    }

    private void showSubMenuGridView() {
        if(mCurrentFuncMode != ACTION_EDIT_TEXT &&
                mCurrentFuncMode != ACTION_EDIT_TAB_LABEL &&
                mCurrentFuncMode != ACTION_TEXT_BUBBLE &&
                mCurrentFuncMode != ACTION_EDIT_TONE){
            mSubHorizontalScrollView.setVisibility(View.VISIBLE);
            mOperateDoneLayout.setVisibility(View.VISIBLE);
            if(!mCancelAnimation) {
                mSubHorizontalScrollView.startAnimation(mFadeInFromBottom);
                mOperateDoneLayout.startAnimation(mFadeInFromTop);
            }else{
                mCancelAnimation = false;
            }
            if(mCurrentFuncMode == ACTION_GRAFFITI ||
                    mCurrentFuncMode == ACTION_DECORATION){
                mEditUndoRedoLayout.setVisibility(View.VISIBLE);
            }else {
                mEditUndoRedoLayout.setVisibility(View.GONE);
            }
        }else if(mCurrentFuncMode == ACTION_EDIT_TONE){
            mFunctionHorizontalScrollView.setVisibility(View.INVISIBLE);
            mSubHorizontalScrollView.setVisibility(View.GONE);
            mOperateDoneLayout.setVisibility(View.VISIBLE);
            mOperateDoneLayout.startAnimation(mFadeInFromTop);
            mEditUndoRedoLayout.setVisibility(View.GONE);
        }
        //mSubMenuGridView.startAnimation(animation);
    }

    private void dismissSubMenuGridView() {
        postFuncHandle();
        if(mCurrentFuncMode == ACTION_MOSAIC) {
            showOrHideMosaicMenuView(View.GONE);
            return;
        }
        mSubHorizontalScrollView.setVisibility(View.GONE);
        mOperateDoneLayout.setVisibility(View.GONE);
        if(mCurrentFuncMode == ACTION_EDIT_TONE){
            mAdjustSclControl.adjustControlVisibility(false);
            mFunctionHorizontalScrollView.setVisibility(View.VISIBLE);
        }

        if(mBrushPainting != null) {
            mBrushPainting.clearAllStrokers();
        }
        if(mMakeupAdjustBtn != null) {
            mMakeupAdjustBtn.setVisibility(View.GONE);
        }
        if(mMakeupControlView != null) {
            mMakeupControlView.adjustControlVisibility(false);
        }
        if(mRectView != null ){
            mRectView.setVisibility(View.GONE);
        }
        if(mMakeupFaceView != null ){
            mMakeupFaceView.setVisibility(View.GONE);
        }
    }

    private Bitmap mCurrentBitmap = null;
    private void setCurrentBitmap(Bitmap bitmap) {
        if(bitmap == null || bitmap.isRecycled())
            return;
        if(mCurrentBitmap != null && mCurrentBitmap != bitmap && mCurrentFuncMode != ACTION_DECORATION) {
            mCurrentBitmap.recycle();
        }
        mCurrentBitmap = bitmap;
    }

    private Bitmap getCurrentBitmap() {
        if(mCurrentBitmap != null && !mCurrentBitmap.isRecycled()){
            return mCurrentBitmap;
        }
        if (mImageEditDesc == null) {
            return null;
        }
        return mImageEditDesc.getBitmap();
    }

    private void addDecorationBitmap() {
        if(centerLayout.getChildCount() > 1){
            Bitmap bitmap = composeDecorBitmap();
            while(bitmap == null){
                //index is one, means that any operation can not be Completed
                if(mImageEditDesc.reorganizeQueue() < 2) {
                    ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                    break;
                }
                bitmap = composeDecorBitmap();
            }
            setCurrentBitmap(bitmap);
            updateImageEditBitmap(bitmap);
            if(mBrushPainting != null) {
                mBrushPainting.clearAllStrokers();
            }
            onPreviewChanged();
        }
    }

    private void addGraffitiBitmap() {
        ImageEditViewPreview.ScrollController scroller = (ScrollController) centerLayout.getChildAt(1);
        if(scroller != null) {
            ImageEditViewGraffitiView graffitiView = (ImageEditViewGraffitiView) scroller.getChildAt(0);
            if(graffitiView.isSaved()){
                Bitmap bitmap = composeGraffitiBitmap(true);
                updateImageEditBitmap(bitmap);
            }
            onPreviewChanged();
        }
    }

    private void addMosaicBitmap() {
        ImageEditViewPreview.ScrollController scroller = (ScrollController) centerLayout.getChildAt(1);
        if(scroller != null) {
            ImageEditViewMosaic mosaicView = (ImageEditViewMosaic) scroller.getChildAt(0);
            if(mosaicView.isDrawMosaic()){
                Bitmap bitmap = composeMosaicBitmap();
                updateImageEditBitmap(bitmap);
            }
            onPreviewChanged();
        }
    }
    private Bitmap composeMosaicBitmap() {
        Bitmap bitmap;
        ImageEditViewPreview.ScrollController scroller = (ScrollController) centerLayout.getChildAt(1);
        ImageEditViewMosaic mosaicView = (ImageEditViewMosaic) scroller.getChildAt(0);
        if(!mosaicView.isDrawMosaic()) {
            if(mImageEditDesc.getqueueSize() <= 1){
                unDoBtn.setEnabled(false);
                mLayoutUndo.setEnabled(false);
            }
            return null;
        }
        bitmap = mosaicView.composeGraffitiBitmap();
        while(bitmap == null){
            //index is one, means that any operation can not be Completed
            if(mImageEditDesc.reorganizeQueue() < 2) {
                ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this, R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                return null;
            }
            bitmap = mosaicView.composeGraffitiBitmap();
        }
        return bitmap;
    }
    private void addLabelBitmap() {
        Bitmap bitmap = composeTextBubbleBitmap();
        while(bitmap == null){
            //index is one, means that any operation can not be Completed
            if(mImageEditDesc.reorganizeQueue() < 2) {
                ImageEditOperationUtil.showHandlerToast(ImageEditControlActivity.this,
                        R.string.edit_operation_memory_low_warn, Toast.LENGTH_SHORT);
                break;
            }
            bitmap = composeTextBubbleBitmap();
        }
        updateImageEditBitmap(bitmap);
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                onPreviewChanged();
            }
        });
    }

    private void addMakeupBitmap(boolean save) {
        resetMakeupParam(save);
    }
    private void addToneBitmap() {
        mAdjustSclControl.addBitmap();
        onPreviewChanged();
    }

    private void postFuncHandle() {
        if(mCurrentFuncMode == ACTION_EDIT_EFFECT) {
            setEffectCatHightlight(-1);
        } else if(mCurrentFuncMode == ACTION_BASIC_EDIT_FUNCTION) {
            setBasicEditHightlight(-1);
        }else if(mCurrentFuncMode == ACTION_EDIT_MAKEUP) {
            setMakeupHightlight(-1);
        } else if(mCurrentFuncMode == ACTION_DECORATION) {
            clearDecorationStack();
        } else if(mCurrentFuncMode == ACTION_EDIT_TEXT){
            mLabelDeleteVisible = false;
        } else if(mCurrentFuncMode == ACTION_PHOTO_FRAME) {
            DecorsAdapter frameAdapter = (DecorsAdapter) adapterArray.get(mPhotoFrameBtn.getId());
            /*
             * FIX BUG: 6133
             * BUG COMMENT: avoid null pointer exception
             * DATE: 2014-03-26
             */
            if(frameAdapter != null) {
                frameAdapter.setSelected(-1);
            }
        } else if(mCurrentFuncMode == ACTION_TEXTURE) {
            DecorsAdapter textureAdapter = (DecorsAdapter) adapterArray.get(mTextureBtn.getId());
            if(textureAdapter != null) {
                textureAdapter.setSelected(-1);
            }
        } else if(mCurrentFuncMode == ACTION_MOSAIC) {
            DecorsAdapter mosaicAdapter = (DecorsAdapter) adapterArray.get(mMosaicBtn.getId());
            if(mosaicAdapter != null) {
                mosaicAdapter.setSelected(-1);
            }
        }
    }

    private void updateUndoRedoState() {
        if(mCurrentFuncMode == ACTION_GRAFFITI){
            if(mBrushPainting.getStrokeCount() > 0){
                mEditUndoBtn.setEnabled(true);
            }else{
                mEditUndoBtn.setEnabled(false);
            }

            if(mBrushPainting.getStackCount() > 0) {
                mEditRedoBtn.setEnabled(true);
            }else {
                mEditRedoBtn.setEnabled(false);
            }
        } else if(mCurrentFuncMode == ACTION_DECORATION) {
            if(mDecorationUndoStack.size() > 0){
                mEditUndoBtn.setEnabled(true);
            }else{
                mEditUndoBtn.setEnabled(false);
            }

            if(mDecorationRedoStack.size() > 0) {
                mEditRedoBtn.setEnabled(true);
            }else{
                mEditRedoBtn.setEnabled(false);
            }
        }
    }

    @Override
    public void onChanged() {
        updateUndoRedoState();
    }

    // NO.8 text runnable
    private Runnable mTextRunnable = new Runnable(){
        @Override
        public void run() {
            mEditText.setFilters(new InputFilter[] {new InputFilter.LengthFilter(17)});
            updateLabelBitmap();
            if(mTitleSaveAsDatabase == null || !mTitleSaveAsDatabase.isOpened()) {
                mTitleSaveAsDatabase = new TitleSaveAsDatabase(ImageEditControlActivity.this);
            }
            mCurrentFuncMode = ACTION_EDIT_TEXT;
            handleTabLabel(null, null, 0, 0);
            onPreviewChanged();
            showSubMenuGridView();
        }

    };

    // NO.11 color runnable
    private Runnable mColorRunnable = new Runnable(){
        @Override
        public void run() {
            mEditText.setFilters(new InputFilter[] {new InputFilter.LengthFilter(22)});
            updateLabelBitmap();
            mCurrentFuncMode = ACTION_EDIT_TONE;
            processAdjust();
            mAdjustSclControl.setToneAnimation(mToneFadeInFromBottom);
            mAdjustSclControl.adjustControlVisibility(true);
            onPreviewChanged();
            showSubMenuGridView();
        }

    };

    // NO.12 bubble runnable
    private Runnable mBubbleRunnable = new Runnable(){
        @Override
        public void run() {
            mEditText.setFilters(new InputFilter[] {new InputFilter.LengthFilter(22)});
            updateLabelBitmap();
            if(mTitleSaveAsDatabase == null || !mTitleSaveAsDatabase.isOpened()) {
                mTitleSaveAsDatabase = new TitleSaveAsDatabase(ImageEditControlActivity.this);
            }
            mCurrentFuncMode = ACTION_TEXT_BUBBLE;
            handleTabLabel(null, null, 1, 0);
            onPreviewChanged();
            showSubMenuGridView();
        }

    };

    // NO.13 label runnable
    private Runnable mLabelRunnable = new Runnable() {
        @Override
        public void run() {
            updateLabelBitmap();
            if(mTitleSaveAsDatabase == null || !mTitleSaveAsDatabase.isOpened()) {
                mTitleSaveAsDatabase = new TitleSaveAsDatabase(ImageEditControlActivity.this);
            }
            mCurrentFuncMode = ACTION_EDIT_TAB_LABEL;
            handleTabLabel(null, null, 2, 0);
            onPreviewChanged();
            showSubMenuGridView();
        }

    };

    // NO.14 decoration undo
    private Runnable mDecorationUndoRunnable = new Runnable() {
        @Override
        public void run() {
            if(centerLayout.getChildCount() > 1){
                setCurrentBitmap(composeDecorBitmap());
            }
            if(mDecorationUndoStack.size() > 0){
                Bitmap bitmap = mDecorationUndoStack.pop();
                mDecorationRedoStack.push(getCurrentBitmap());
                setCurrentBitmap(bitmap);
                onPreviewChanged();
            }
            updateUndoRedoState();
        }

    };

    //NO.15 decoration redo
    private Runnable mDecorationRedoRunnable = new Runnable() {
        @Override
        public void run() {
            if(mDecorationRedoStack.size() > 0){
                Bitmap bitmap = mDecorationRedoStack.pop();
                mDecorationUndoStack.push(getCurrentBitmap());
                setCurrentBitmap(bitmap);
                onPreviewChanged();
            }
            updateUndoRedoState();
        }
    };

    private String getSaveImagePath() {
        String imageEditPath = mImageEditDesc.getImageEditPath();
        Log.d(TAG, "getSaveImagePath(): before imageEditPath = " + imageEditPath);
        if (imageEditPath == null) {
            imageEditPath = getDefaultPathAccordUri(mImageEditDesc.getOriginalUri());
        }
        /*
         * FIX BUG: 5953
         * BUG COMMENT: avoid null pointer exception
         * DATE: 2014-02-24
         */
        if(imageEditPath == null) {
            imageEditPath = mImageEditDesc.getRootEditPath();
        }
        Log.d(TAG, "getSaveImagePath(): after imageEditPath = " + imageEditPath);

        return imageEditPath;
    }
    public static boolean IsDopod = false;
    private int mCanvasClickedCount;
    private SharedPreferences mSharedPreferences;
    public static void setIsDopod(boolean dopod) {
        IsDopod = dopod;
    }
}
