/*
 * Copyright (C) 2011,2012 Thundersoft Corporation All rights Reserved
 */

package com.ucamera.ucomm.puzzle;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.Matrix.ScaleToFit;
import android.graphics.PixelFormat;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Handler.Callback;
import android.os.Message;
import android.os.Parcelable;
import android.os.Vibrator;
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
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup.MarginLayoutParams;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.GridView;
import android.widget.HorizontalScrollView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.PopupWindow;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.Toast;

import com.sprd.camera.storagepath.StorageUtil;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.ucamera.ucomm.downloadcenter.UiUtils;
import com.ucamera.ucomm.downloadcenter.Constants;
import com.ucamera.ucomm.downloadcenter.DownloadCenter;
import com.ucamera.ucomm.downloadcenter.DownloadCenter.OnLoadCallback;
import com.ucamera.ucomm.puzzle.free.FreePuzzleView;
import com.ucamera.ucomm.puzzle.grid.GridMenuAdapter;
import com.ucamera.ucomm.puzzle.grid.GridPuzzlePiece;
import com.ucamera.ucomm.puzzle.grid.GridPuzzleSelectView;
import com.ucamera.ucomm.puzzle.grid.GridPuzzleView;
import com.ucamera.ucomm.puzzle.stitch.StitchPuzzleView;
import com.ucamera.ucomm.puzzle.util.PuzzleBitmapManager;
import com.ucamera.ugallery.util.Models;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class PuzzleActivity extends Activity implements OnClickListener {
    protected static final String TAG = "PuzzleActivity";
    public static final String INTENT_EXTRA_IMAGES = "ucam.puzzle.IMAGES";
    private Uri[] mImageUris;
    private Uri mLastSaveUri = null;
    private boolean mModified = false;

    //private SensorMonitor mSensorMonitor;
    private GestureDetector mActivityGestureDecDetector;

    private GridGuestureListener mGridGestrueListener;
    private GestureDetector mGridGestureDetector;
    private FreeGuestureListener mFreeGestureListener;
    private GestureDetector mFreeGestureDetector;
    private StitchGuestureListener mStitchGestureListener;
    private GestureDetector mStitchGestureDetector;

    // Can be either mGridPuzzleView or mFreePuzzleView
    private PuzzleView mCurrentPuzzleView;
    private GridPuzzleView mGridPuzzleView;
    private FreePuzzleView mFreePuzzleView;
    private StitchPuzzleView mStitchPuzzleView;
    private ScrollView mStitchPuzzleViewWrapper;

    private GridView mPuzzleGridView = null;
    private PuzzleMenuAdapter mPuzzleAdapter = null;
    private GridMenuAdapter mGridAdapter = null;
    private int mCurrentMenuSelected = 0;
    private boolean mIsBackPressed = false;
    private String[] mPuzzleImages;

    private static final int MODE_DRAG = 2;
    private static final int MODE_CHECK_LONGPRESS = 3;
    private static final int MODE_ZOOM = 4;
    private int mGridTouchMode = 0;
    private WindowManager mWindowManager;
    private WindowManager.LayoutParams mWindowParams;
    private ImageView mDragView;
    private View mDragItem;
    private Bitmap mDragBitmap;
    private int mWindowOffsetX;
    private int mWindowOffsetY;

    private View mGridPuzzleviewTip;
    private View mFreePuzzleviewTip;
    private HorizontalScrollView mScrollViewMenu;

    private static final int PUZZLE_BACK_PICKER = 2;
    private static final int PUZZLE_SINGLE_PICKER = 20;
    private static final int PUZZLE_BACK_UPHOTO = 3;
    private static final int CLICK_DELAY_TIME = 500;
    private long mLastClickTime = 0;

    private boolean mIsPause = false;
    private PopupWindow mGridPopWindow;
    private ArrayList<Integer> mChangedImages = new ArrayList<Integer>();

    //private ArrayList<Puzzle> mPuzzles = new ArrayList<Puzzle>();
    private ArrayList<PuzzleSpec> mPuzzleSpec = new ArrayList<PuzzleSpec>();
    private int mPopMenuWidth;
    private int mPopMenuHeight;
    private SeekBar mGridRoundSeekBar;
    private LinearLayout mGridOperateLayout;
    private LinearLayout mPuzzleMenuLayout;
    private LinearLayout mGridEffectlayout;
    private CheckBox mGridPieceShadowCb;

    //SPRD:fix bug530889 switch language then puzzle crash
    private boolean needInitPuzzleWithUris = true;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        /*SPRD:fix bug514045 Some Activity about UCamera lacks method of checkpermission@{ */
        if (!UCamUtill.checkPermissions(this)) {
            finish();
            return;
        }
        /* }@ */
        /*
         * SPRD: bug 474674 cannot save image when changed display size in setting. @{
         */
        String state = StorageUtil.getInstance().getStorageState();
        if (Environment.MEDIA_UNMOUNTED.equals(state)) {
            finish();
            return;
        }
        /* @} */
        setContentView(R.layout.puzzle_main);
        Parcelable[] data = getIntent().getParcelableArrayExtra(INTENT_EXTRA_IMAGES);
        mImageUris = new Uri[data.length];
        System.arraycopy(data, 0, mImageUris, 0, data.length);
        initPuzzles();
        initialize();
        createPuzzleAdapter(0);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mIsPause = false;
        DownloadCenter.loadPuzzle(this, new OnLoadCallback() {
            @Override
            public void onLoadComplete(String[] result) {
                mPuzzleImages = result;
                if (mCurrentPuzzleView == mFreePuzzleView) {
                    ArrayList<? extends View>  lists = mFreePuzzleView.createAdapterItems(
                                    PuzzleActivity.this,
                                    mPuzzleImages);
                    if (lists != null) {
                        setDisplayItemCountsInWindow(mPuzzleGridView,lists.size(), 5);
                        mPuzzleAdapter.updateAdapterImages(lists);
                        if (mPuzzleImages.length == 0) {
                            return;
                        }
                        if(mPuzzleImages.length <= mCurrentMenuSelected)
                            mCurrentMenuSelected = 0;
                        mFreePuzzleView.setBackgroundFile(PuzzleActivity.this, mPuzzleImages[mCurrentMenuSelected]);
                        /*
                         * FIX BUG: 5598
                         * BUG COMMETN: The HorizontalScrollView will scroll to the Correct position
                         * DATE: 2013-12-13
                         */
                        UiUtils.scrollToCurrentPosition(mScrollViewMenu, mItemWidth, mCurrentMenuSelected);
                        mPuzzleAdapter.setHighlight(mCurrentMenuSelected);
                        setPuzzleAdapterListener();
                    }
                }
            }
        });
    }

    private void initPuzzles() {
        Puzzle puzzle = Puzzle.create(Puzzle.Type.GRID, mImageUris.length);
        //puzzle.initMethods();
        /*if(mPuzzles != null) {
            mPuzzles.clear();
        }*/
        if(mPuzzleSpec != null) {
            mPuzzleSpec.clear();
        }
        for(int i = 0; i < puzzle.getMethodCount(); i++) {
            Puzzle puzzle1 = Puzzle.create(Puzzle.Type.GRID, mImageUris.length);
            //puzzle1.initMethods();
            //mPuzzles.add(puzzle1.getPuzzleIndex(i));
            mPuzzleSpec.add(puzzle1.getPuzzleIndex(i).getSpec());
        }
    }
    /*
     * Do NOT remove, used for Stat (non-Javadoc)
     *
     * @see android.app.Activity#onPause()
     */
    @Override
    protected void onPause() {
        super.onPause();
        mIsPause = true;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_MENU) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    /*@Override
    public void onWindowFocusChanged(boolean hasFocus) {
        if (hasFocus){
            mSensorMonitor.startMonitor();
        } else {
            mSensorMonitor.stopMonitor();
        }
    }*/

    @Override
    public boolean dispatchTouchEvent(MotionEvent m) {
        if (mCurrentPuzzleView == mStitchPuzzleView) {
            processStitchTouchEvent(m);
        }
        if (m.getAction() != MotionEvent.ACTION_DOWN) {
            if (!(mActivityGestureDecDetector != null && mActivityGestureDecDetector
                    .onTouchEvent(m))) {
                super.dispatchTouchEvent(m);
            }
        } else {
            super.dispatchTouchEvent(m);
            if (mActivityGestureDecDetector != null) {
                mActivityGestureDecDetector.onTouchEvent(m);
            }
        }
        /* SPRD:  CID 109349 : UCF: Useless control flow (FB.UCF_USELESS_CONTROL_FLOW) @{
        if(mCurrentPuzzleView == mGridPuzzleView) {
        }
        @} */
        return true;
    }
    private void disableAllButtons() {
        findViewById(R.id.puzzle_tab_free_puzzle).setEnabled(false);
        findViewById(R.id.puzzle_tab_stitch_puzzle).setEnabled(false);
        findViewById(R.id.puzzle_edit).setEnabled(false);
        findViewById(R.id.puzzle_save).setEnabled(false);
        findViewById(R.id.puzzle_share).setEnabled(false);
        findViewById(R.id.puzzle_add).setEnabled(false);
    }
    private void enableAllButtons() {
        findViewById(R.id.puzzle_tab_free_puzzle).setEnabled(true);
        findViewById(R.id.puzzle_tab_stitch_puzzle).setEnabled(true);
        findViewById(R.id.puzzle_edit).setEnabled(true);
        findViewById(R.id.puzzle_save).setEnabled(true);
        findViewById(R.id.puzzle_share).setEnabled(true);
        findViewById(R.id.puzzle_add).setEnabled(true);
    }
    @Override
    public void onClick(View v) {
        if(System.currentTimeMillis() - mLastClickTime < CLICK_DELAY_TIME) {
            return;
        }
        mLastClickTime = System.currentTimeMillis();
        if(v.getId() == R.id.puzzle_grid_piece_rotate) {
            if(mCurrentPuzzleView == mGridPuzzleView && mCurrentPuzzleView.getCurrentPiece() != null) {
                rotatePuzzlePiece(mCurrentPuzzleView.getCurrentPiece());
            }
            return;
        }
        if(v.getId() == R.id.puzzle_grid_piece_single_select) {
            if(mCurrentPuzzleView == mGridPuzzleView) {
                gotoSelectSingleImage();
            }
            return;
        }
        if(v.getId() == R.id.puzzle_grid_piece_flip) {
            if(mCurrentPuzzleView == mGridPuzzleView && mCurrentPuzzleView.getCurrentPiece() != null) {
                flipPuzzlePiece(mCurrentPuzzleView.getCurrentPiece());
            }
            return;
        }
        switch (v.getId()) {
        case R.id.puzzle_tab_photo_grid:
            mCurrentMenuSelected = 0;
            showGridPuzzleView();
            break;
        case R.id.puzzle_tab_free_puzzle:
            mCurrentMenuSelected = 0;
            showFreePuzzleView();
            break;
        case R.id.puzzle_tab_stitch_puzzle:
            mCurrentMenuSelected = 0;
            showStitchPuzzleView();
            break;
        case R.id.puzzle_save:
            doSave(null, null);
            break;
        case R.id.puzzle_edit:
            doEdit();
            break;
        case R.id.puzzle_share:
            doShare();
            break;
        case R.id.puzzle_prev:
        case R.id.puzzle_next:
             nextPuzzle();
             break;
        case R.id.grid_puzzleview_tip:
            mGridPuzzleviewTip.setVisibility(View.GONE);
            break;
        case R.id.free_puzzleview_tip:
            mFreePuzzleviewTip.setVisibility(View.GONE);
            break;
        case R.id.puzzle_add:
            puzzleAdd();
            break;
        case R.id.btn_puzzle_show_type:
            mGridOperateLayout.setVisibility(View.INVISIBLE);
            mPuzzleMenuLayout.setVisibility(View.VISIBLE);
            mScrollViewMenu.setVisibility(View.VISIBLE);
            mGridEffectlayout.setVisibility(View.GONE);
            break;
        case R.id.btn_puzzle_grid_show_effect:
            mGridOperateLayout.setVisibility(View.INVISIBLE);
            mPuzzleMenuLayout.setVisibility(View.VISIBLE);
            mGridEffectlayout.setVisibility(View.VISIBLE);
            mScrollViewMenu.setVisibility(View.GONE);
            break;
        case R.id.btn_puzzle_grid_effect_close:
            mPuzzleMenuLayout.setVisibility(View.GONE);
            mGridOperateLayout.setVisibility(View.VISIBLE);
            break;
        case R.id.btn_puzzle_free_show_type:
            mGridOperateLayout.setVisibility(View.INVISIBLE);
            mPuzzleMenuLayout.setVisibility(View.VISIBLE);
            mScrollViewMenu.setVisibility(View.VISIBLE);
            mGridEffectlayout.setVisibility(View.GONE);
            break;
        /*case R.id.puzzle_back:
            this.finish();
            break;*/
        }
        dismissGridPopupWindow();
    }
    private void puzzleAdd() {
        /*
         * FIX BUG: 4792
         * BUG COMMENT: don't start other activity if current activity is paused
         * DATE: 2013-09-03
         */
        if(mIsPause) {
            return;
        }
        /*
         * FIX BUG: 4313
         * FIX DATE: 2013-06-21
         * */
        selectPuzzlePiece(null);
        mGridPuzzleView.setCurrentPiece(null);
        mFreePuzzleView.setCurrentPiece(null);
        mStitchPuzzleView.setCurrentPiece(null);
        Intent intent = new Intent();
        intent.setClassName(PuzzleActivity.this,
                "com.ucamera.ugallery.CollageImagePickerActivity");
        /*
         * FIX BUG: 6197
         * BUG CAUSE: java.lang.ArrayIndexOutOfBoundsException
         * FIX DATE: 2014-04-10
         */
        mImageUris = new Uri[mCurrentPuzzleView.getChildCount()];
        for(int i = 0; i < mCurrentPuzzleView.getChildCount(); i++) {
            mImageUris[i] = mCurrentPuzzleView.getChildAt(i).getUri();
        }
        intent.putExtra("ucam.puzzle.IMAGES", mImageUris);
        try {
            //SPRD:fix bug530889 switch language then puzzle crash
            needInitPuzzleWithUris = true;
            startActivityForResult(intent, PUZZLE_BACK_PICKER);
        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "Exception : "+e);
            e.printStackTrace();
        }
        //finish();
    }
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if(requestCode == PUZZLE_BACK_PICKER && resultCode == RESULT_OK) {
            Parcelable[] datas = data.getParcelableArrayExtra(INTENT_EXTRA_IMAGES);
            Uri[] uris = new Uri[datas.length];
            System.arraycopy(datas, 0, uris, 0, datas.length);
            updateNewImageUris(mImageUris, uris);
            if(Arrays.equals(uris, mImageUris)) {
            }else {
//                setModified(true);
                mImageUris = new Uri[datas.length];
                System.arraycopy(uris, 0, mImageUris, 0, uris.length);
                getIntent().putExtra("ucam.puzzle.IMAGES", mImageUris);
//                mGridPuzzleView.reset();
//                mFreePuzzleView.reset();
//                mStitchPuzzleView.reset();
                reinitPuzzleData(uris);
            }
        } else if(requestCode == PUZZLE_BACK_UPHOTO){
            if(mCurrentPuzzleView != null /*&& !mCurrentPuzzleView.isInitializeFinish()*/) {
                reinitPuzzleData(mImageUris);
            }
            /* SPRD: add for bug 525014 fix press edit button will save the picture @{ */
            if (mLastSaveUri != null) {
                getApplication().getContentResolver().delete(mLastSaveUri, null, null);
                mLastSaveUri = null;
            }
            /* @} */
        } else if(requestCode == PUZZLE_SINGLE_PICKER && resultCode == RESULT_OK) {
            if(data != null) {
                Parcelable[] datas = data.getParcelableArrayExtra("Extra.Image.Uris");
                PuzzlePiece piece = mCurrentPuzzleView.getCurrentPiece();
                Bitmap bitmap = PuzzleUtils.createPUzzlePieceBitmap(this, mCurrentPuzzleView.getChildCount(), (Uri)datas[0]);
                if(piece != null && bitmap != null) {
                    mImageUris[piece.getNum()] = (Uri)datas[0];
                    PuzzleBitmapManager.getInstance().updateBitmap(piece.getNum(), bitmap);
                    piece.updateUri((Uri) datas[0]);
                    piece.setImageBitmap(bitmap);
                }
            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    private void updateNewImageUris(Uri[] oldUris, Uri[] newUris) {
        mChangedImages.clear();
        int length = oldUris.length > newUris.length ? newUris.length : oldUris.length;
        for(int i = 0; i < length; i++) {
            if(!oldUris[i].equals(newUris[i])) {
                mChangedImages.add(i);
            }
        }
        if(oldUris.length > newUris.length) {
            for(int i = length; i < oldUris.length; i++) {
                mChangedImages.add(i);
            }
        }
    }
    private void reinitPuzzleData(Uri[] uris) {
        setModified(true);
        PuzzleBitmapManager.getInstance().recycleUnusedBms(mChangedImages);
        mGridPuzzleView.reset();
        mFreePuzzleView.reset();
        mStitchPuzzleView.reset();
        initPuzzles();
        //SPRD:fix bug530889 switch language then puzzle crash
        if (needInitPuzzleWithUris) {
            mCurrentPuzzleView.initPuzzleWithUris(uris, new PuzzleView.OnInitListener() {
                @Override
                public void onInit() {
                    if (mCurrentPuzzleView == mGridPuzzleView) {
                        setGridPieceRound(mGridRoundSeekBar.getProgress());
                        setGridPieceShadow(mGridPieceShadowCb.isChecked());
                    }
                }
                @Override
                public void onFail() {
                    finish();
                }
            }, mPuzzleSpec.get(0));
        }
        needInitPuzzleWithUris = false;
        mGridAdapter = null;
        if(mCurrentPuzzleView == mGridPuzzleView) {
            createPuzzleAdapter(0);
        }
    }

    // ////////////////////////////////////////////////////////
    // / Common Methods
    private void initialize() {
        /*Parcelable[] data = getIntent().getParcelableArrayExtra(INTENT_EXTRA_IMAGES);
        mImageUris = new Uri[data.length];
        System.arraycopy(data, 0, mImageUris, 0, data.length);*/

        //mSensorMonitor = new SensorMonitor();

        mGridGestrueListener = new GridGuestureListener();
        mFreeGestureListener = new FreeGuestureListener();
        mStitchGestureListener = new StitchGuestureListener();
        mGridGestureDetector = new GestureDetector(this, mGridGestrueListener);
        mFreeGestureDetector = new GestureDetector(this, mFreeGestureListener);
        mStitchGestureDetector = new GestureDetector(this, mStitchGestureListener);

        mActivityGestureDecDetector = new GestureDetector(this, new ActivityGestureListener());

        findViewById(R.id.puzzle_next).setOnClickListener(this);
        findViewById(R.id.puzzle_prev).setOnClickListener(this);
        findViewById(R.id.puzzle_add).setOnClickListener(this);
        if(Models.AMAZON_KFTT.equals(Models.getModel())){
         findViewById(R.id.puzzle_add).setPadding(-25, 0, 0, 0);
         findViewById(R.id.puzzle_save).setPadding(0, 0, -30, 0);
         findViewById(R.id.puzzle_share).setPadding(0, 0, -15, 0);
         findViewById(R.id.puzzle_edit).setPadding(0, 0, -15, 0);
        }
        mScrollViewMenu = (HorizontalScrollView) findViewById(R.id.puzzle_hs_puzzle_middle_menu);
        mGridPuzzleView = (GridPuzzleView) findViewById(R.id.puzzle_view);
        mGridPuzzleView.setLongClickable(true);
        mGridPuzzleView.setOnTouchListener(new GridTouchListener());

        mFreePuzzleView = (FreePuzzleView) findViewById(R.id.free_puzzle_view);
        mPuzzleGridView = (GridView) findViewById(R.id.puzzle_gv_background);
        mFreePuzzleView.setOnTouchListener(new FreeTouchListener());

        mStitchPuzzleView = (StitchPuzzleView) findViewById(R.id.stitch_puzzle_view);
        mStitchPuzzleView.setLongClickable(true);
        mStitchPuzzleView.setOnTouchListener(new StitchTouchListener());
        mStitchPuzzleViewWrapper = (ScrollView) findViewById(R.id.stitch_puzzle_view_wrap);

        findViewById(R.id.puzzle_tab_photo_grid).setSelected(true);
        mGridPuzzleView.setLayerType(View.LAYER_TYPE_SOFTWARE,null);//Add for Shadow is not work

        //SPRD:fix bug530889 switch language then puzzle crash
        if(needInitPuzzleWithUris){
            mGridPuzzleView.initPuzzleWithUris(mImageUris, new PuzzleView.OnInitListener() {
                @Override
                public void onInit() {
                    findViewById(R.id.puzzle_edit).setOnClickListener(PuzzleActivity.this);
                    findViewById(R.id.puzzle_save).setOnClickListener(PuzzleActivity.this);
                    findViewById(R.id.puzzle_share).setOnClickListener(PuzzleActivity.this);
                    findViewById(R.id.puzzle_tab_free_puzzle).setOnClickListener(PuzzleActivity.this);
                    findViewById(R.id.puzzle_tab_photo_grid).setOnClickListener(PuzzleActivity.this);
                    findViewById(R.id.puzzle_tab_stitch_puzzle).setOnClickListener(PuzzleActivity.this);
                    //findViewById(R.id.puzzle_middle_menu_layout).setVisibility(View.VISIBLE);
                    //findViewById(R.id.puzzle_back).setOnClickListener(PuzzleActivity.this);
                    if(PuzzleUtils.getGridPuzlePre(PuzzleActivity.this)) {
                        mGridPuzzleviewTip.setVisibility(View.VISIBLE);
                        PuzzleUtils.setGridPuzzlePre(PuzzleActivity.this, false);
                    }
                }

                @Override
                public void onFail() {
                    finish();
                }
            }, mPuzzleSpec.get(0));
        }
        needInitPuzzleWithUris = false;

        mCurrentPuzzleView = mGridPuzzleView;
        selectPuzzlePiece(null);

        mGridPuzzleviewTip = (View) findViewById(R.id.grid_puzzleview_tip);
        mFreePuzzleviewTip = (View) findViewById(R.id.free_puzzleview_tip);
        mGridPuzzleviewTip.setOnClickListener(this);
        mFreePuzzleviewTip.setOnClickListener(this);

        mWindowManager = (WindowManager) this.getSystemService("window");
        mGridPopWindow = new PopupWindow(this);
        mGridPopWindow.setBackgroundDrawable(null);
        /*View */mGridPopupView = LayoutInflater.from(this).inflate(R.layout.gridpiece_popupview, null);
        //SPRD:fix bug535435 some icons show not friendly
        ImageButton btn_rotate = (ImageButton) mGridPopupView.findViewById(R.id.puzzle_grid_piece_rotate);
        btn_rotate.setOnClickListener(this);
        ImageButton btn_select = (ImageButton) mGridPopupView.findViewById(R.id.puzzle_grid_piece_single_select);
        btn_select.setOnClickListener(this);
        ImageButton btn_flip = (ImageButton) mGridPopupView.findViewById(R.id.puzzle_grid_piece_flip);
        btn_flip.setOnClickListener(this);
        mGridPopWindow.setWidth(LayoutParams.WRAP_CONTENT);
        mGridPopWindow.setHeight(LayoutParams.WRAP_CONTENT);
        mGridPopWindow.setContentView(mGridPopupView);
        Drawable pop_menu = getResources().getDrawable(R.drawable.bg_grid_menu);
        mPopMenuWidth = pop_menu.getIntrinsicWidth();
        mPopMenuHeight = pop_menu.getIntrinsicHeight();
        findViewById(R.id.btn_puzzle_show_type).setOnClickListener(PuzzleActivity.this);
        findViewById(R.id.btn_puzzle_grid_show_effect).setOnClickListener(PuzzleActivity.this);
        findViewById(R.id.btn_puzzle_free_show_type).setOnClickListener(PuzzleActivity.this);
        findViewById(R.id.btn_puzzle_grid_effect_close).setOnClickListener(PuzzleActivity.this);
        mGridRoundSeekBar = (SeekBar) findViewById(R.id.puzzle_grid_round_degree);
        mGridOperateLayout = (LinearLayout) findViewById(R.id.layout_puzzle_operate);
        mPuzzleMenuLayout = (LinearLayout) findViewById(R.id.layout_puzzle_menu);
        mGridEffectlayout = (LinearLayout) findViewById(R.id.layout_puzzle_grid_effect_operate);
        mGridPieceShadowCb = (CheckBox) findViewById(R.id.cb_puzzle_grid_shadow);
        mGridEffectlayout.setOnTouchListener(new View.OnTouchListener() {
            public boolean onTouch(View v, MotionEvent event) {
                return true;
            }
        });
        mGridRoundSeekBar.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
            public void onStopTrackingTouch(SeekBar seekBar) {
                setModified(true);
            }
            public void onStartTrackingTouch(SeekBar seekBar) {
            }
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if(mCurrentPuzzleView == mGridPuzzleView) {
                    setGridPieceRound(progress);
                }
            }
        });
        mGridPieceShadowCb.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(mCurrentPuzzleView == mGridPuzzleView) {
                    setGridPieceShadow(isChecked);
                    setModified(true);
                }
            }
        });
        findViewById(R.id.puzzle_share).setVisibility(View.INVISIBLE);//SPRD:hide share button 
    }

    private void setGridPieceRound(int progress) {
        for (int i = 0; i < mGridPuzzleView.getChildCount(); i++) {
            GridPuzzlePiece piece = (GridPuzzlePiece) mGridPuzzleView.getChildAt(i);
            piece.setRoundDegree(progress);
            piece.invalidate();
        }
    }
    private void setGridPieceShadow(boolean shadow) {
        for(int i = 0; i < mGridPuzzleView.getChildCount(); i++) {
            GridPuzzlePiece piece = (GridPuzzlePiece) mGridPuzzleView.getChildAt(i);
            piece.setShadow(shadow);
            piece.invalidate();
        }
    }
    private void nextPuzzle() {
        selectPuzzlePiece(null);
        mCurrentPuzzleView.shuffle(mCurrentMenuSelected);
        setModified(true);
        if(mGridTouchMode == MODE_DRAG) {
            /*
             * BUG FIX:4070
             * BUG COMMENT:enable all buttons
             * FIX DATE:2013-05-29
             * */
            enableAllButtons();
            stopDragging();
            selectPuzzlePiece(null);
            PuzzlePiece p1 = mGridPuzzleView.getPieceAt((int) mGridGestrueListener.mCurrentDownEvent.getX(), (int) mGridGestrueListener.mCurrentDownEvent.getY());
            p1.setVisibility(View.VISIBLE);
            mWindowParams = null;
            mGridTouchMode = MODE_CHECK_LONGPRESS;
        }
    }

    private void selectPuzzlePiece(PuzzlePiece p) {
        mCurrentPuzzleView.setCurrentPiece(p);
    }

    protected void showGridPuzzleView() {
        if (mCurrentPuzzleView == mGridPuzzleView) {
            return;
        }

        setModified(true);

        if (!mGridPuzzleView.isInitializeFinish()) {
            mGridPuzzleView.initPuzzleByCopy(mCurrentPuzzleView);
            setGridPieceRound(mGridRoundSeekBar.getProgress());
            setGridPieceShadow(mGridPieceShadowCb.isChecked());
        } else {
            mGridPuzzleView.borrowBitmapsFrom(mCurrentPuzzleView);
        }
        //mGridPuzzleView.borrowBitmapsFrom(mCurrentPuzzleView);

        findViewById(R.id.puzzle_tab_photo_grid).setSelected(true);
        findViewById(R.id.puzzle_tab_free_puzzle).setSelected(false);
        findViewById(R.id.puzzle_tab_stitch_puzzle).setSelected(false);
        mStitchPuzzleViewWrapper.setVisibility(View.INVISIBLE);
//        findViewById(R.id.puzzle_middle_menu_layout).setVisibility(View.VISIBLE);
        findViewById(R.id.puzzle_prev).setVisibility(View.GONE);
        findViewById(R.id.puzzle_next).setVisibility(View.GONE);
        mGridOperateLayout.setVisibility(View.VISIBLE);
        findViewById(R.id.layout_puzzle_grid_operate).setVisibility(View.VISIBLE);
        findViewById(R.id.layout_puzzle_free_operate).setVisibility(View.GONE);

        mCurrentPuzzleView = mGridPuzzleView;
        createPuzzleAdapter(0);
        mGridPuzzleView.setVisibility(View.VISIBLE);
        mFreePuzzleView.setVisibility(View.GONE);
        mStitchPuzzleView.setVisibility(View.GONE);
        //mCurrentPuzzleView.shuff(mPuzzles.get(mCurrentMenuSelected));
        mCurrentPuzzleView.shuff(mPuzzleSpec.get(mCurrentMenuSelected));
        mCurrentPuzzleView.setCurrentPiece(null);
    }

    protected void showFreePuzzleView() {
        if (mCurrentPuzzleView == mFreePuzzleView) {
            return;
        }

        setModified(true);
        if (!mFreePuzzleView.isInitializeFinish()) {
            mFreePuzzleView.initPuzzleByCopy(mCurrentPuzzleView);
        } else {
            mFreePuzzleView.borrowBitmapsFrom(mCurrentPuzzleView);
        }

        findViewById(R.id.puzzle_tab_photo_grid).setSelected(false);
        findViewById(R.id.puzzle_tab_free_puzzle).setSelected(true);
        findViewById(R.id.puzzle_tab_stitch_puzzle).setSelected(false);
        mStitchPuzzleViewWrapper.setVisibility(View.GONE);
        mGridOperateLayout.setVisibility(View.VISIBLE);
        findViewById(R.id.layout_puzzle_grid_operate).setVisibility(View.GONE);
        findViewById(R.id.layout_puzzle_free_operate).setVisibility(View.VISIBLE);
//        findViewById(R.id.puzzle_middle_menu_layout).setVisibility(View.VISIBLE);
//        findViewById(R.id.puzzle_prev).setVisibility(View.VISIBLE);
//        findViewById(R.id.puzzle_next).setVisibility(View.VISIBLE);

        if (mFreePuzzleView.getHeight() == 0) {
            setFreePuzzleWidthHeight();
        }

        mCurrentPuzzleView = mFreePuzzleView;
        createPuzzleAdapter(0);
        mFreePuzzleView.setVisibility(View.VISIBLE);
        mGridPuzzleView.setVisibility(View.GONE);
        mStitchPuzzleView.setVisibility(View.GONE);
        mCurrentPuzzleView.shuffle(0);

//        if(PuzzleUtils.getFreePuzzlePre(PuzzleActivity.this)) {
//            mFreePuzzleviewTip.setVisibility(View.VISIBLE);
//            PuzzleUtils.setFreePuzzlePre(PuzzleActivity.this, false);
//        }
    }
    private void setFreePuzzleWidthHeight() {
        int left = PuzzleUtils.getFreePuzzleLeft(this);
        int width = mGridPuzzleView.getWidth();
        int height = mGridPuzzleView.getHeight();
        if(left != 0) {
            width = PuzzleUtils.getPuzzleWidth(this) - left * 2;
        }
        if((float)width / height < (float)3 / 4) {
            height = width * 4 /3;
        }else {
            width = height * 3 / 4;
        }
        mFreePuzzleView.setWidthAndHeight(width, height);
    }
    protected void showStitchPuzzleView() {
        if (mCurrentPuzzleView == mStitchPuzzleView) {
            return;
        }

        setModified(true);
        if (!mStitchPuzzleView.isInitializeFinish()) {
            mStitchPuzzleView.initPuzzleByCopy(mCurrentPuzzleView);
        } else {
            mStitchPuzzleView.borrowBitmapsFrom(mCurrentPuzzleView);
        }

        findViewById(R.id.puzzle_tab_photo_grid).setSelected(false);
        findViewById(R.id.puzzle_tab_free_puzzle).setSelected(false);
        findViewById(R.id.puzzle_tab_stitch_puzzle).setSelected(true);
//        findViewById(R.id.puzzle_middle_menu_layout).setVisibility(View.GONE);
        mStitchPuzzleViewWrapper.setVisibility(View.VISIBLE);
        findViewById(R.id.puzzle_prev).setVisibility(View.GONE);
        findViewById(R.id.puzzle_next).setVisibility(View.GONE);
        mGridOperateLayout.setVisibility(View.GONE);

        mGridPuzzleView.setVisibility(View.GONE);
        mFreePuzzleView.setVisibility(View.GONE);
        mStitchPuzzleView.setVisibility(View.VISIBLE);
        mCurrentPuzzleView = mStitchPuzzleView;
        mCurrentPuzzleView.shuffle(0);
        mCurrentPuzzleView.setCurrentPiece(null);
    }

    private void setModified(boolean modified) {
        mModified = modified;
        findViewById(R.id.puzzle_save).setEnabled(modified);
    }

    public boolean isModified() {
        return mModified;
    }

    protected void doSave(String filePath, String fileName) {
        if (!mCurrentPuzzleView.isInitializeFinish()) {
            return;
        }
        new SaveTask(new OnSaveListener() {
            @Override
            public void onSave(Uri uri) {
                mLastSaveUri = null;
            }
        }, filePath, fileName).execute();
    }

    private void doShare() {
        if(mIsPause) {
            return;
        }
        if (!mCurrentPuzzleView.isInitializeFinish()) {
            return;
        }

        if (mLastSaveUri != null && !isModified()) {
            shareImage(mLastSaveUri);
            return;
        }

        new SaveTask(new OnSaveListener() {
            @Override
            public void onSave(Uri uri) {
                shareImage(uri);
            }
        }, null, null).execute();
    }

    private void shareImage(Uri uri) {
//        ShareUtils.shareImage(this, uri);
    }

    private void doEdit() {
        if (!mCurrentPuzzleView.isInitializeFinish()) {
            return;
        }
        if (mLastSaveUri != null && !isModified()) {
            gotoEdit(mLastSaveUri);
            return;
        }
        new SaveTask(new OnSaveListener() {
            @Override
            public void onSave(Uri uri) {
                gotoEdit(uri);
            }
        }, null, null).execute();
    }

    private void resetEditBitmap() {
//                if(mGridPuzzleView != null) {
//                    mGridPuzzleView.recycleBm();
//                }
//                if(mFreePuzzleView != null) {
//                    mGridPuzzleView.recycleBm();
//                }
//                if(mStitchPuzzleView != null) {
//                    mGridPuzzleView.recycleBm();
//                }
                PuzzleBitmapManager.getInstance().recycleBm();
    }
    private void gotoEdit(Uri uri) {
        if(mIsPause) {
            return;
        }
        /*
         * FIX BUG : 4760
         * BUG COMMENT : recycle bitmap in onPause and to avoid OOM exception
         * DATE : 2013-08-28
         */
         resetEditBitmap();
         Intent intent = new Intent(this, com.ucamera.uphoto.ImageEditControlActivity.class);
         intent.setDataAndType(uri, "image/*");
         intent.putExtra(com.ucamera.uphoto.ImageEditConstants.EXTRA_FROM_INNER, true);
         //SPRD:fix bug530889 switch language then puzzle crash
         needInitPuzzleWithUris = true;
         startActivityForResult(intent,PUZZLE_BACK_UPHOTO);
         overridePendingTransition(0, 0);
    }

    class SaveTask extends AsyncTask<Void, Void, Uri> {
        private OnSaveListener mOnSaveListener;
        private String mFilePath;
        private String mFileName;
        private ProgressDialog mProgressDialog;
        private Bitmap mBitmap = null;

        SaveTask(OnSaveListener l, String filePath, String fileName) {
            mOnSaveListener = l;
            mFilePath = filePath;
            mFileName = fileName;
        }

        @Override
        protected void onPreExecute() {
            selectPuzzlePiece(null);
            mProgressDialog = new ProgressDialog(PuzzleActivity.this);
            mProgressDialog.setIndeterminate(true);
            mProgressDialog.setMessage(getString(R.string.puzzle_text_waiting));
            mProgressDialog.show();

            if(mBitmap != null && !mBitmap.isRecycled()) {
                mBitmap.recycle();
            }
            try {
                mBitmap = mCurrentPuzzleView.exportBitmap();
            } catch (OutOfMemoryError e) {
                Log.w(TAG, "OOM when exportBitmap!");
                PuzzleUtils.recyleBitmap(mBitmap);
                Toast.makeText(PuzzleActivity.this, R.string.puzzle_toast_insufficient_memory, Toast.LENGTH_SHORT).show();
                cancel(false);
            }
        }

        @Override
        protected void onCancelled() {
            mProgressDialog.dismiss();
        }

        @Override
        protected Uri doInBackground(Void... params) {
            try {
                return PuzzleStorage.getStorage(PuzzleActivity.this).saveBitmap(PuzzleActivity.this, mBitmap, mFilePath, mFileName);
            } catch (Exception e) {
                Log.w(TAG, "Error while saving.", e);
            } finally {
                PuzzleUtils.recyleBitmap(mBitmap);
                mBitmap = null;
            }
            return null;
        }

        @Override
        protected void onPostExecute(Uri result) {
            mLastSaveUri = result;
            findViewById(R.id.puzzle_edit).setEnabled(true);
            findViewById(R.id.puzzle_share).setEnabled(true);
            setModified(false);
            mProgressDialog.dismiss();
            if (mOnSaveListener != null && result != null) {
                mOnSaveListener.onSave(result);
            }
            if (mIsBackPressed)
                finish();
        }
    }

    interface OnSaveListener {
        void onSave(Uri uri);
    }

    class ActivityGestureListener extends GestureDetector.SimpleOnGestureListener {
        // represent this down-move... serial of touch actions should be or not
        // be processed in the aftermath
        boolean canTouchAction = false;

        @Override
        public boolean onDown(MotionEvent e) {
            canTouchAction = true;
            return true;
        }

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
            if (!canTouchAction) {
                return false;
            }
            float y1 = e1.getY();
            float y2 = e2.getY();
            float x1 = e1.getX();
            float x2 = e2.getX();
            boolean isVerticalScroll = (Math.abs(x2 - x1) - Math.abs(y2 - y1)) <= 0;
            if (!isVerticalScroll) {
                return false;
            }
            return false;
        }
    }

    /*class SensorMonitor implements SensorEventListener {
        private SensorManager mSensorManager = null;
        private Sensor mAccelerometer;
        private float mSensorX;
        private float mSensorY;
        private long mLastMillis = 0;

        private static final int TRIGER_INTERVAL = 500; // ms;

        public SensorMonitor() {
            mSensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
            mAccelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        }

        public void startMonitor() {
            mSensorManager.registerListener(this, mAccelerometer, SensorManager.SENSOR_DELAY_UI);
        }

        public void stopMonitor() {
            mSensorManager.unregisterListener(this);
        }

        @Override
        public void onSensorChanged(SensorEvent event) {
            if (event.sensor.getType() != Sensor.TYPE_ACCELEROMETER)
                return;

            float sx = event.values[0];
            float sy = event.values[1];
            if ((Math.abs(sx - mSensorX) > 9.8f || Math.abs(sy - mSensorY) > 9.8f)) {
                if (mLastMillis == 0
                        || (System.currentTimeMillis() - mLastMillis > TRIGER_INTERVAL)) {
                    mLastMillis = System.currentTimeMillis();
                    //nextPuzzle();
                }
            }
            mSensorX = sx;
            mSensorY = sy;
        }

        @Override
        public void onAccuracyChanged(Sensor sensor, int accuracy) {}
    }*/

    static enum ScaleType {
        IN(0.8f), OUT(1.25f);
        final float scaleX, scaleY;

        private ScaleType(float s) {
            this(s, s);
        }

        private ScaleType(float sx, float sy) {
            scaleX = sx;
            scaleY = sy;
        }
    }

    // /////////////////////////////////////////////////////////////////////////////
    // /// Grid Specific

    private static final int SWIPE_MIN_DISTANCE = 250;
    private static final int SWIPE_MAX_OFF_PATH = 180;
    private static final int SWIPE_THRESHOLD_VELOCITY = 250;

    private boolean trySwitchPuzzle(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {

        if (Math.abs(velocityX) < SWIPE_THRESHOLD_VELOCITY) {
            return false;
        }

        float xOffset = e1.getX() - e2.getX();
        float yOffset = e1.getY() - e2.getY();
        if (Math.abs(yOffset) > SWIPE_MAX_OFF_PATH) {
            return false;
        }

        if (Math.abs(xOffset) < SWIPE_MIN_DISTANCE) {
            return false;
        }

        //nextPuzzle();
        return true;
    }
    protected void doZoomPiece(ScaleType op) {
        if (mCurrentPuzzleView != mGridPuzzleView) {
            return;
        }
        PuzzlePiece piece = mCurrentPuzzleView.getCurrentPiece();
        if (piece == null || op == null) {
            return;
        }

        Object zoom = piece.getTag();

        if (op == ScaleType.IN) {
            if (zoom == null || (Integer) zoom <= 0) {
                return;
            }
            piece.setTag((Integer) zoom - 1);
        } else if (op == ScaleType.OUT) {
            if (zoom == null) {
                piece.setTag(1);
            } else {
                piece.setTag((Integer) zoom + 1);
            }
        }

        Matrix matrix = piece.getImageMatrix();
        matrix.postScale(op.scaleX, op.scaleY);
        piece.setImageMatrix(matrix);
        /*
         * FIX BUG:3388
         * COMMENT:after zoom need to adjustscroll
         * DATE:2013-04-08
         */
        piece.adjustScroll();
        piece.invalidate();
        setModified(true);
    }

    private boolean doSwapPuzzleBitmap(PuzzlePiece p1, PuzzlePiece p2) {
        if (p1 != null && p2 != null && p1 != p2) {
            Drawable tmp = p1.getDrawable();
            Uri uri = p1.getUri();
            p1.setImageDrawable(p2.getDrawable());
            p1.updateUri(p2.getUri());
            mImageUris[p1.getNum()] = p2.getUri();
            p2.setImageDrawable(tmp);
            p2.updateUri(uri);
            mImageUris[p2.getNum()] = uri;
            PuzzleBitmapManager.getInstance().swapBmp(p1.getNum(), p2.getNum());
            p1.scrollTo(0, 0);
            p2.scrollTo(0, 0);
            return true;
        }
        return false;
    }
    private View mGridPopupView;
    private void rotatePuzzlePiece(PuzzlePiece puzzle) {
        puzzle.rotateBitmap();
    }
    private void flipPuzzlePiece(PuzzlePiece puzzle) {
        puzzle.oprateHorizonFlipBitmap();
    }
    private void gotoSelectSingleImage() {
        com.ucamera.ugallery.ImageGallery.showImagePicker(this,
                com.ucamera.ugallery.CollageSingleImagePicker.class,
                PUZZLE_SINGLE_PICKER,
                1,
                com.ucamera.ugallery.ViewImage.IMAGE_ENTRY_UPHOTO_VALUE);
    }
    private void showGridPiecePopupView(PuzzlePiece puzzle) {
        if(puzzle == null) return;
        Rect rect = new Rect();
        puzzle.getGlobalVisibleRect(rect);
        int x = (rect.width() - mPopMenuWidth) / 2;
        int y = (rect.height() - mPopMenuHeight) /2;
        if(mGridPopWindow.isShowing()) {
            mGridPopWindow.update(rect.left + x, rect.top + y, LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        }else {
            mGridPopWindow.showAtLocation(puzzle, Gravity.TOP | Gravity.LEFT, rect.left + x, rect.top + y);
        }
    }
    private void dismissGridPopupWindow() {
        if(mGridPopWindow != null && mGridPopWindow.isShowing()) {
            mGridPopWindow.dismiss();
        }
    }
    class GridGuestureListener extends GestureDetector.SimpleOnGestureListener {
        private MotionEvent mCurrentDownEvent;
        private long downTime;

        @Override
        public boolean onSingleTapUp(MotionEvent e) {
            PuzzlePiece current = mGridPuzzleView.getCurrentPiece();
            PuzzlePiece p = mGridPuzzleView.getPieceAt((int) e.getX(), (int) e.getY());
            selectPuzzlePiece((p != current) ? p : null);
            if(p == null) return true;
            if(p != current) {
                showGridPiecePopupView(p);
            } else {
                dismissGridPopupWindow();
            }
            return true;
        }

        @Override
        public boolean onDown(MotionEvent e) {
            downTime = System.currentTimeMillis();
            if (mCurrentDownEvent != null) {
                mCurrentDownEvent.recycle();
            }
            mCurrentDownEvent = MotionEvent.obtain(e);
            mGridTouchMode = MODE_CHECK_LONGPRESS;
            PuzzlePiece p = mGridPuzzleView.getPieceAt((int) e.getX(), (int) e.getY());
            if(p != null) {
                matrix.set(p.getImageMatrix());
                mOldMatrix.set(matrix);
            }
            return true;
        }

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
            PuzzlePiece current = mGridPuzzleView.getCurrentPiece();
            PuzzlePiece p1 = mGridPuzzleView.getPieceAt((int) e1.getX(), (int) e1.getY());
            PuzzlePiece p2 = mGridPuzzleView.getPieceAt((int) e2.getX(), (int) e2.getY());
            if (current != null && p1 == current && p2 == current) {
                p1.scrollBy((int) distanceX, (int) distanceY);
                return true;
            }
            return false;
        }
    }

    private void onGridViewLongPressed(PuzzlePiece puzzle,int x, int y) {
        ImageView imageView = new ImageView(this);
        imageView.setImageBitmap(((BitmapDrawable) puzzle.getDrawable()).getBitmap());
        mDragItem = imageView;
        dragView(x, y);
        PuzzlePiece piece1 = mCurrentPuzzleView.getPieceAt((int)mGridGestrueListener.mCurrentDownEvent.getX(),
                (int)mGridGestrueListener.mCurrentDownEvent.getY());
        piece1.setVisibility(View.INVISIBLE);
    }
    private boolean checkLongPressed(long downtime, long movetime, int x1, int y1, int x2, int y2) {
        if(movetime - downtime > 300 && Math.abs(x2 - x1) < 20 && Math.abs(y2 - y1) < 20) {
            mGridTouchMode = MODE_DRAG;
            PuzzlePiece piece = mCurrentPuzzleView.getPieceAt(x1, y1);
            if(piece != null) {
                selectPuzzlePiece(null);
                onGridViewLongPressed(piece, x1, y1);
                Vibrator vibrator = (Vibrator)getSystemService(Context.VIBRATOR_SERVICE);
                long [] pattern = {100, 50};
                vibrator.vibrate(pattern,-1);
                return true;
            } else {
                return false;
            }

        } else {
            return false;
        }
    }
    private void handleImageMove(MotionEvent event){
        if(mDragItem != null){
            dragView((int)event.getX(), (int)event.getY());
            doExpansion();
        }
    }
    private void stopDragging() {
        if (mDragView != null) {
            WindowManager wm = (WindowManager) this.getSystemService("window");
            wm.removeView(mDragView);
            mDragView.setImageDrawable(null);
            mDragView = null;
        }
    }
    private void doExpansion() {
        if (mDragItem != null) {
            mDragItem.setVisibility(View.INVISIBLE);
        }
    }
    private void dragView(int x, int y) {
        float alpha = 0.5f;
        if(mDragItem == null){
            return;
        }
        Bitmap bitmap = null;
        if(mWindowParams == null){
            mDragItem.setDrawingCacheEnabled(true);
            if(mDragItem instanceof ImageView){
                BitmapDrawable mDrawable =  (BitmapDrawable) ((ImageView) mDragItem).getDrawable();
                bitmap = mDrawable.getBitmap();
            }else{
                bitmap = Bitmap.createBitmap(mDragItem.getDrawingCache());
            }
            startDragging(bitmap, x, y);
        }

        mWindowParams.alpha = alpha;
        mWindowParams.y = y + mWindowOffsetY;
        mWindowParams.x = x + mWindowOffsetX;
        mWindowParams.flags = WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;

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
        mWindowParams.gravity = Gravity.TOP | Gravity.LEFT;

        PuzzlePiece piece1 = mCurrentPuzzleView.getPieceAt((int)mGridGestrueListener.mCurrentDownEvent.getX(),
                (int)mGridGestrueListener.mCurrentDownEvent.getY());
        Rect r = new Rect();
        piece1.getGlobalVisibleRect(r);
        mWindowParams.height = piece1.getHeight();
        mWindowParams.width = piece1.getWidth();
        mWindowParams.x = r.left;
        mWindowParams.y = r.top;
        mWindowOffsetX = r.left - x;
        mWindowOffsetY = r.top - y;
        mWindowParams.flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
        mWindowParams.format = PixelFormat.TRANSLUCENT;
        mWindowParams.windowAnimations = 0;

        ImageView v = new ImageView(this);
        v.setImageBitmap(bm);

        mWindowManager.addView(v, mWindowParams);
        Bitmap bitmap = null;
        if(mDragView != null) {
            bitmap = mDragView.getDrawingCache();
        }
        if(bitmap != null && !bitmap.isRecycled()) {
            bitmap.recycle();
            bitmap = null;
        }
        mDragView = v;
    }
    private float mOldDistance = 1f;
    private float calculateDistance(MotionEvent event) {
        if(event.getPointerCount() > 1) {
            float x = event.getX(0) - event.getX(1);
            float y = event.getY(0) - event.getY(1);
            return FloatMath.sqrt(x * x + y * y);
        }
        return 0;
    }
    private Matrix mZoomPieceMatrix;
    private PuzzlePiece mZoomTargetPiece;
    private void zoomGridPuzzlePiece(float newDistance, float oldDistance, MotionEvent event) {
        if (mCurrentPuzzleView != mGridPuzzleView) {
            return;
        }
        float scale = newDistance / oldDistance;
        //PuzzlePiece piece = mCurrentPuzzleView.getPieceAt((int)event.getX(0), (int)event.getY(0));
        if(mZoomTargetPiece != null) {
            mZoomPieceMatrix = mZoomTargetPiece.getImageMatrix();
            mZoomPieceMatrix.set(mOldMatrix);
            mZoomPieceMatrix.postScale(scale, scale/*, mMidPoint.x, mMidPoint.y*/);
            /*matrix.set(mOldMatrix);
            matrix.postScale(scale, scale, mMidPoint.x, mMidPoint.y);*/
            mZoomTargetPiece.setImageMatrix(mZoomPieceMatrix);
            //piece.adjustScroll();
            mZoomTargetPiece.invalidate();
            setModified(true);
        }
    }
    private PointF mMidPoint = new PointF();
    private void setMidPoint(PointF point, MotionEvent event) {
        float x = event.getX(0) + event.getX(1);
        float y = event.getY(0) + event.getY(1);
        point.set(x / 2, y / 2);
    }
    private Matrix mOldMatrix = new Matrix();
    private Matrix matrix = new Matrix();
    class GridTouchListener implements OnTouchListener {
        private MotionEvent mFirstPointerMotinEvent;
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            boolean processed = mGridGestureDetector.onTouchEvent(event);
            /*
             * BUG FIX:
             * FIX COMMENT: long press:drag
             * FIX DATE: 2013-05-17
             */
            if((event.getAction() & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_POINTER_DOWN) {
                mZoomTargetPiece = mCurrentPuzzleView.getPieceAt((int)mGridGestrueListener.mCurrentDownEvent.getX(), (int)mGridGestrueListener.mCurrentDownEvent.getY());
                mOldDistance = calculateDistance(event);
                if(mOldDistance > 20f && mGridTouchMode != MODE_DRAG && mZoomTargetPiece != null && mZoomTargetPiece.isSelected()) {
                    mGridTouchMode = MODE_ZOOM;
                    mFirstPointerMotinEvent = event;
                    setMidPoint(mMidPoint, event);
                    //piece = mCurrentPuzzleView.getPieceAt((int)mGridGestrueListener.mCurrentDownEvent.getX(), (int)mGridGestrueListener.mCurrentDownEvent.getY());
                    if(mZoomTargetPiece != null) {
                        matrix.set(mZoomTargetPiece.getImageMatrix());
                        mOldMatrix.set(matrix);
                    }
                }
            }
            if((event.getAction() & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_MOVE)
            {
                if(mGridTouchMode == MODE_CHECK_LONGPRESS) {
                    if(checkLongPressed(mGridGestrueListener.downTime, System.currentTimeMillis(), (int)mGridGestrueListener.mCurrentDownEvent.getX(),
                            (int)mGridGestrueListener.mCurrentDownEvent.getY(), (int)event.getX(), (int)event.getY())) {
                        return true;
                    }else {
                        mGridTouchMode = MODE_CHECK_LONGPRESS;
                        return false;
                    }
                }
                if(mGridTouchMode == MODE_DRAG) {
                    if(mGridPopWindow != null && mGridPopWindow.isShowing()) {
                        mGridPopWindow.dismiss();
                    }
                    disableAllButtons();
                    handleImageMove(event);
                    PuzzlePiece piece1 = mCurrentPuzzleView.getPieceAt((int)mGridGestrueListener.mCurrentDownEvent.getX(),
                            (int)mGridGestrueListener.mCurrentDownEvent.getY());
                    piece1.setVisibility(View.INVISIBLE);
                    PuzzlePiece piece2 = mCurrentPuzzleView.getPieceAt((int)event.getX(), (int)event.getY());
                    if(piece1 != piece2) {
                        selectPuzzlePiece(piece2);
                    }
                    return true;
                }
                if(mGridTouchMode == MODE_ZOOM) {
                    float mNewDistance = calculateDistance(event);
                    if(mNewDistance > 20f) {
                        zoomGridPuzzlePiece(mNewDistance, mOldDistance, mFirstPointerMotinEvent);
                    }
                    return true;
                }
            }
            afterGesture(event);
            if (processed) {
                setModified(true);
            }
            return processed;
        }

        private void afterGesture(MotionEvent event) {
            final int action = event.getActionMasked();
            if(action == MotionEvent.ACTION_POINTER_UP) {
                if (mGridTouchMode == MODE_ZOOM) {
                    float[] values = new float[9];
                    /*
                     * FIX BUG: 6036
                     * BUG CAUSE: nullpointer
                     * DATE: 2014-03-07
                     */
                    if(mZoomTargetPiece != null && mZoomPieceMatrix != null) {
                        mZoomPieceMatrix.getValues(values);
                        float scaleWidth = (float) mZoomTargetPiece.getWidth() / mZoomTargetPiece.getDrawable().getBounds().width();
                        float scaleHeight = (float) mZoomTargetPiece.getHeight() / mZoomTargetPiece.getDrawable().getBounds().height();
                        if (values[0] < Math.max(scaleWidth, scaleHeight)) {
                            if (mZoomTargetPiece != null) {
                                Matrix matrix = new Matrix();
                                matrix.setScale(Math.max(scaleWidth, scaleHeight),
                                        Math.max(scaleWidth, scaleHeight));
                                mZoomTargetPiece.setImageMatrix(matrix);
                                mZoomTargetPiece.invalidate();
                            }
                        }
                    }
                }
            }
            if (action == MotionEvent.ACTION_UP) {
                if(mGridTouchMode == MODE_DRAG) {
                    enableAllButtons();
                    if(mGridPuzzleView != null && mGridGestrueListener != null && mGridGestrueListener.mCurrentDownEvent != null) {
                        PuzzlePiece p1 = mGridPuzzleView.getPieceAt((int) mGridGestrueListener.mCurrentDownEvent.getX(), (int) mGridGestrueListener.mCurrentDownEvent.getY());
                        PuzzlePiece p2 = mGridPuzzleView.getPieceAt((int) event.getX(), (int) event.getY());
                        stopDragging();
                        if (p1 != p2) {
                            doSwapPuzzleBitmap(p1, p2);
                        }
                        selectPuzzlePiece(null);
                        if(p1 != null) {
                            p1.setVisibility(View.VISIBLE);
                        }
                    }
                    /*
                     * BUG FIX: 4155
                     */
                    //p1.setTag(0);
                    mWindowParams = null;
                    mGridTouchMode = MODE_CHECK_LONGPRESS;
                } else {
                    if (mGridPuzzleView != null && mGridGestrueListener != null && mGridGestrueListener.mCurrentDownEvent != null) {
                        MotionEvent e = mGridGestrueListener.mCurrentDownEvent;
                        PuzzlePiece p = mGridPuzzleView.getPieceAt((int) e.getX(), (int) e.getY());
                        if (p != null) {
                            p.adjustScroll();
                        }
                    }
                }
            }
            if (/*action == MotionEvent.ACTION_UP ||*/ action == MotionEvent.ACTION_CANCEL) {
                if (mGridGestrueListener != null && mGridGestrueListener.mCurrentDownEvent != null) {

                    MotionEvent e = mGridGestrueListener.mCurrentDownEvent;
                    if(mGridPuzzleView != null) {
                        PuzzlePiece p = mGridPuzzleView.getPieceAt((int) e.getX(), (int) e.getY());
                        if (p != null) {
                            p.adjustScroll();
                        }
                    }
                }
            }
        }
    }

    // /////////////////////////////////////////////////////////////////////////////
    // / Free Puzzle related
    private boolean mViewProcessTouch = false;

    class FreeTouchListener implements OnTouchListener {
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            boolean processed = mFreeGestureDetector.onTouchEvent(event);
            if (mFreePuzzleView.onTouchEvent(event)) {
                setModified(true);
                mViewProcessTouch = true;
            }
            return processed;
        }
    }

    class FreeGuestureListener extends GestureDetector.SimpleOnGestureListener {

        // CID 109357 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // private boolean mSwitchProcessed = false;

        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {

            // if (e2.getY() < -60){
            // doDismissTopbar();
            // }

            return false;
        }

        @Override
        public boolean onDown(MotionEvent e) {
            PuzzlePiece p = mFreePuzzleView.getPieceAt((int) e.getX(), (int) e.getY());
            mFreePuzzleView.setCurrentPiece(p);
            // CID 109357 : UrF: Unread field (FB.URF_UNREAD_FIELD)
            // mSwitchProcessed = false;
            mViewProcessTouch = false;
            return true;
        }

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
            /* SPRD: CID 109157 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE) @{
            PuzzlePiece current = mFreePuzzleView.getCurrentPiece();
            PuzzlePiece p1 = mFreePuzzleView.getPieceAt((int) e1.getX(), (int) e1.getY());
            @} */
            /*if (!mSwitchProcessed
                    && !mViewProcessTouch
                    && (current == null || p1 != current)
                    && trySwitchPuzzle(MotionEvent.obtain(e1), MotionEvent.obtain(e2),
                            SWIPE_THRESHOLD_VELOCITY, SWIPE_THRESHOLD_VELOCITY)) {
                mSwitchProcessed = true;
                return true;
            }*/
            return false;
        }
    }

    @Override
    protected void onRestart() {
        super.onRestart();
    }

    private void setPuzzleAdapterListener() {
        mPuzzleGridView.setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                if (mCurrentMenuSelected == position) {
                    return;
                }
                if (mCurrentPuzzleView == mFreePuzzleView) {
                    freeMenuAdapterItemClick(position);
                }
                else if(mCurrentPuzzleView == mGridPuzzleView){
                    gridMenuAdapterItemClick(position);
                }
            }
        });

        if (mCurrentPuzzleView == mFreePuzzleView) {
            mPuzzleGridView.setSelection(1);
        } else if(mCurrentPuzzleView == mGridPuzzleView){
            mPuzzleGridView.setSelection(0);
        }
    }

    protected void gridMenuAdapterItemClick(int position) {
        mCurrentMenuSelected = position;
        selectPuzzlePiece(null);
        dismissGridPopupWindow();
        setModified(true);
        //mCurrentPuzzleView.shuff(mPuzzles.get(position));
        mCurrentPuzzleView.shuff(mPuzzleSpec.get(position));
        mGridAdapter.setHightLight(position);
        //nextPuzzle();
    }

    protected void freeMenuAdapterItemClick(int position) {
        if (position == 0 && DownloadCenter.RESOURCE_DOWNLOAD_ON) {
            DownloadCenter.openResourceCenter(PuzzleActivity.this, Constants.EXTRA_PUZZLE_VALUE);
        } else {
            /*
             * FIX BUG: 6127
             * BUG COMMETN: java.lang.ArrayIndexOutOfBoundsException: length=5; index=6
             * DATE: 2014-03-18
             */
            if(mCurrentMenuSelected < 0 || mPuzzleImages.length <= mCurrentMenuSelected)
                return;
            mCurrentMenuSelected = position;
            setModified(true);
            mPuzzleAdapter.setHighlight(mCurrentMenuSelected);
            mFreePuzzleView.setBackgroundFile(this, mPuzzleImages[mCurrentMenuSelected]);
        }
    }

    private void setDisplayItemCountsInWindow(final GridView gridView, int length, int count) {
        gridView.setNumColumns(length);
        int default_dp = 54;
        final float scale = getResources().getDisplayMetrics().density;
        mItemWidth = (int) (default_dp * scale + 0.5f);
        //final int itemWidth = (int) (getWindowManager().getDefaultDisplay().getWidth() / 5.5);
        final int layout_width = mItemWidth * length;
        gridView.setLayoutParams(new LinearLayout.LayoutParams(layout_width,
                LayoutParams.WRAP_CONTENT));
    }
    @Override
    protected void onStop() {
        Log.d(TAG,"onStop!");
        super.onStop();
    }
    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        //mSensorMonitor.stopMonitor();
        //mSensorMonitor = null;
        /* SPRD: add for bug 525014 fix press edit button will save the picture @{ */
        if (mLastSaveUri != null) {
            getApplication().getContentResolver().delete(mLastSaveUri, null, null);
            mLastSaveUri = null;
        }
        /* @} */
        PuzzleBitmapManager.getInstance().recycleBm();
        super.onDestroy();
    }
    private void createPuzzleAdapter(final int resId) {
        if (mCurrentPuzzleView == mFreePuzzleView) {
            if (mPuzzleAdapter == null) {
                mPuzzleAdapter = new PuzzleMenuAdapter(null);
                setPuzzleAdapterListener();
            }
            ArrayList<? extends View> lists = mFreePuzzleView.createAdapterItems(PuzzleActivity.this , mPuzzleImages);
            if (lists != null) {
                setDisplayItemCountsInWindow(mPuzzleGridView, lists.size(), 5);
                mPuzzleAdapter.updateAdapterImages(lists);
                mPuzzleGridView.setAdapter(mPuzzleAdapter);
                mCurrentMenuSelected = 1;
                if(mPuzzleImages.length <= mCurrentMenuSelected)
                    return;
                mFreePuzzleView.setBackgroundFile(PuzzleActivity.this, mPuzzleImages[mCurrentMenuSelected]);
                mPuzzleAdapter.setHighlight(mCurrentMenuSelected);
            }
        }
        if(mCurrentPuzzleView == mGridPuzzleView) {
            if (mGridAdapter == null) {
                mGridAdapter = new GridMenuAdapter(PuzzleActivity.this, null, mImageUris.length);
                setPuzzleAdapterListener();
            }
            setDisplayItemCountsInWindow(mPuzzleGridView, mPuzzleSpec.size(), 5);
            //mGridAdapter.updatePuzzles(mPuzzles);
            mGridAdapter.updatePuzzleSpecd(mPuzzleSpec);
            mPuzzleGridView.setAdapter(mGridAdapter);
            mCurrentMenuSelected = 0;
            mPuzzleGridView.setSelection(mCurrentMenuSelected);
            mGridAdapter.setHightLight(mCurrentMenuSelected);
        }
    }
    /*
     * FIX BUG 1188 BUG COMMENT: override onBackPressed function,click back when
     * picture is not saved,we should show dialog to save. DATE: 2012-07-03
     */
    @Override
    public void onBackPressed() {
        if(mPuzzleMenuLayout != null && mPuzzleMenuLayout.isShown()) {
            mGridOperateLayout.setVisibility(View.VISIBLE);
            mPuzzleMenuLayout.setVisibility(View.GONE);
            return;
        }
        boolean isEnable = findViewById(R.id.puzzle_save).isEnabled();
        mIsBackPressed = true;
        if (isEnable) {
            final String filePath = PuzzleStorage.getStorage(PuzzleActivity.this).getStoragePath();
            final String fileName = PuzzleStorage.getStorage(PuzzleActivity.this).makeBaseFilename(System.currentTimeMillis());
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setTitle(getResources().getString(R.string.puzzle_text_edit_exit_tip_title))
                    .setMessage(getString(R.string.puzzle_text_edit_message, filePath + "/" + fileName))
                    .setCancelable(false)
                    .setPositiveButton(getResources().getString(R.string.puzzle_text_edit_exit_tip_save),
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int id) {
                                    dialog.dismiss();
                                    doSave(filePath, fileName);
                                }
                            })
                    .setNeutralButton(getResources().getString(R.string.puzzle_text_edit_exit_tip_exit),
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int id) {
                                    dialog.dismiss();
                                    PuzzleBitmapManager.getInstance().recycleBm();
                                    finish();
                                }
                            })
                    .setNegativeButton(
                            getResources().getString(R.string.puzzle_text_edit_exit_tip_cancel),
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int id) {
                                    dialog.dismiss();
                                    mIsBackPressed = false;
                                }
                            });
            AlertDialog alert = builder.create();
            alert.show();
            return;
        }
        super.onBackPressed();
    }

    // ////////////////////////////////////////////////////////////////////////
    // Stitch Puzzle
    class StitchGuestureListener extends GestureDetector.SimpleOnGestureListener {
        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
            PuzzlePiece current = mStitchPuzzleView.getCurrentPiece();
            PuzzlePiece p1 = mStitchPuzzleView.getPieceAt((int) e1.getX(), (int) e1.getY());
            PuzzlePiece p2 = mStitchPuzzleView.getPieceAt((int) e2.getX(), (int) e2.getY());

            if (p1 == current && p1 != p2) {
                if (doSwapPuzzleBitmap(p1, p2)) {
                    selectPuzzlePiece(p2);
                    return true;
                }
            }

            // if (e2.getY() < -60){
            // doDismissTopbar();
            // }

            return false;
        }

        @Override
        public boolean onSingleTapUp(MotionEvent e) {
            PuzzlePiece current = mStitchPuzzleView.getCurrentPiece();
            PuzzlePiece p = mStitchPuzzleView.getPieceAt((int) e.getX(), (int) e.getY());

            if (p == current || current == null) {
                selectPuzzlePiece((p != current) ? p : null);
            } else if (p != null) {
                if (doSwapPuzzleBitmap(current, p)) {
                    selectPuzzlePiece(p);
                }
            }
            return true;
        }

        /*@Override
        public boolean onDoubleTap(MotionEvent e) {
            PuzzlePiece current = mStitchPuzzleView.getCurrentPiece();
            PuzzlePiece p = mStitchPuzzleView.getPieceAt((int) e.getX(), (int) e.getY());

            if (p == current || current == null) {
                selectPuzzlePiece((p != current) ? p : null);
            } else if (p != null) {
                if (doSwapPuzzleBitmap(current, p)) {
                    selectPuzzlePiece(p);
                }
            }
            return true;
        }*/
    }

    class StitchTouchListener implements OnTouchListener {
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            /**
             * FIX BUG 1641 BUG COMMENT: Finished modifying photos, we need to
             * set save key clickable; DATE: 2012-10-08
             */
            boolean processed = mStitchGestureDetector.onTouchEvent(event);
            if (processed) {
                setModified(true);
            }
            return processed;
            // return mStitchGestureDetector.onTouchEvent(event);
        }
    }

    // Stitch Zoom Related
    private enum Mode {
        NONE, ZOOM, MOVE
    }

    private Mode mMode = Mode.NONE;
    private float mDistance = 0.0f;
    private int mItemWidth;

    private void zoomStitch(float oldValue, float newValue) {
        int maxMargin = calcMaxMargin();
        int delta = (int) Math.ceil((oldValue - newValue) / 2);
        MarginLayoutParams lp = (MarginLayoutParams) mStitchPuzzleView.getLayoutParams();
        lp.leftMargin = Math.min(Math.max(0, lp.leftMargin + delta), maxMargin);
        lp.rightMargin = Math.min(Math.max(0, lp.rightMargin + delta), maxMargin);
        mStitchPuzzleView.setLayoutParams(lp);
    }

    private int calcMaxMargin() {
        float width = mStitchPuzzleViewWrapper.getWidth();
        float height = mStitchPuzzleViewWrapper.getHeight();
        float w = mStitchPuzzleView.getWidth();
        float h = mStitchPuzzleView.getHeight();
        float minW = w / h * height;
        return (int) (width - minW) / 2;
    }

    private float spacing(MotionEvent event) {
        float x = event.getX(0) - event.getX(1);
        float y = event.getY(0) - event.getY(1);
        return FloatMath.sqrt(x * x + y * y);
    }

    private void processStitchTouchEvent(MotionEvent event) {
        if (mStitchPuzzleView == null || mCurrentPuzzleView != mStitchPuzzleView)
            return;
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
        case MotionEvent.ACTION_DOWN:
            mMode = Mode.MOVE;
            break;
        case MotionEvent.ACTION_POINTER_DOWN:
            mDistance = spacing(event);
            mMode = Mode.ZOOM;
            break;
        case MotionEvent.ACTION_UP:
            mMode = Mode.NONE;
            mDistance = 0.0f;
            break;
        case MotionEvent.ACTION_MOVE:
            if (mMode == Mode.ZOOM && event.getPointerCount() > 1) {
                float newDistance = spacing(event);
                if (Math.abs(newDistance - mDistance) > 2) {
                    zoomStitch(mDistance, newDistance);
                    mDistance = newDistance;
                }
            }
        }
    }
}
