/**
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ugallery.panorama;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.util.Timer;
import java.util.TimerTask;


import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.DialogInterface.OnClickListener;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Images.Media;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.Toast;
import android.widget.LinearLayout.LayoutParams;

//import com.ucamera.ucomm.sns.integration.ShareUtils;

import com.ucamera.ugallery.BaseImagePicker;
import com.ucamera.ugallery.ImageListSpecial;
import com.ucamera.ugallery.R;
import com.ucamera.ugallery.UGalleryConst;
import com.ucamera.ugallery.ViewImage;
import com.ucamera.ugallery.gallery.IImage;
import com.ucamera.ugallery.gallery.IImageList;
import com.ucamera.ugallery.gallery.ImageList;
import com.ucamera.ugallery.util.ImageManager;
import com.ucamera.ugallery.util.Models;
import com.ucamera.ugallery.util.Util;
import com.ucamera.ugallery.util.ViewImageCommonUtil;
//import com.ucamera.ucomm.sns.integration.ShareUtils;

public class UgalleryPanoramaActivity extends Activity {
    private final String TAG = "PanoramaActivity";
    private final int FINISH_ACTIVITY = 1;
    /** Called when the activity is first created. */
    private PopupWindow mPopupWindow;
    private PanoramaRender mRender;
    // panoram picture angle
    private int mPictureAngle = -1;
    // review display time
    private int mReviewDisplayTime = -1;
    private Handler mHandler = new MainHandler();
    private boolean mPausing = false;
    private LinearLayout mBottomMenuLayout = null;
    private LinearLayout mTopMenuLayout = null;
    // picture uri
    private Uri mUri = null;
    // picture bitmap
    private Bitmap mBitmap = null;
    // delete dialog
    private AlertDialog mReviewDelDlg = null;
    // file name
    private String mFileName = null;
    private float mRatio = (float) 0.1;
    private Dialog mTipDialog;
    private ImageManager.ImageListParam mParam;
    private IImageList mAllImages;
    private String mCurrentBucketId;
    private String mCurrentStoragePos;
    private int mCurrentPosition;
    private IImage mImage;
    private String mTitle;
    /**
     * This Handler is used to post message back onto the main thread of the
     * application
     */
    private class MainHandler extends Handler {
        // filter the scheduled ModeHintView.hide callback when on pause
        @Override
        public void dispatchMessage(Message msg) {
            if (mPausing && msg.getCallback()!=null){
                return;
            }
            super.dispatchMessage(msg);
        }
        @Override
        public void handleMessage(Message msg) {
            // super class process the holder message
            switch (msg.what) {
                case FINISH_ACTIVITY: {
                    finish();
                    break;
                }
            }
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate");
        /*
         * FIX BUG : 5997
         * FIX COMMENT: action bar overlapps the bottommenu ;
         * DATE: 2014-2-27
         */
        // add menu key support for the devices later than GINGERBREAD_MR1 version
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.GINGERBREAD_MR1 && !Models.getModel().equals(Models.GOOGLE_Nexus_7)){
            getWindow().addFlags(0x08000000);
        }

        GLSurfaceView view = new GLSurfaceView(this);
        Intent intent = getIntent();
        mParam = intent.getParcelableExtra(ViewImage.EXTRA_IMAGE_LIST_KEY);
        mCurrentBucketId = intent.getStringExtra(ViewImage.EXTRACURRENT_BUCKID_KEY);
        mCurrentStoragePos = intent.getStringExtra(ViewImage.EXTRA_IMAGE_STORAGE_KEY);
        mUri = intent.getData();
        /*
         * FIX BUG : 5548
         * FIX COMMENT: Avoid null pointer exception
         * DATE: 2013-12-10
         */
        if (mUri == null || mUri.equals("")) {
            Log.e(TAG, "Uri is null, please transfering a valid uri");
//            Toast.makeText(this, getString(R.string.panorama_review_open_picture_failed), Toast.LENGTH_SHORT).show();
            finish();
        }
        init(mUri);
        try {
            mBitmap = getBitmapAccordingUri(mUri);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            Log.e(TAG, "File is not found, Uri: " + mUri);
            return ;
        }

        if (mBitmap != null) {
            mPictureAngle = intent.getIntExtra("PictureAngle", mPictureAngle);
            if(mPictureAngle <= 0) {
                mPictureAngle = (int)(mBitmap.getWidth() * mRatio);
            }
            if(mPictureAngle > 360) {
                mPictureAngle = 360;
            }
            Log.d(TAG, "Picture angle equals " + mPictureAngle);

            mReviewDisplayTime = intent.getIntExtra("ReviewDisplayTime", mReviewDisplayTime);
            if (mReviewDisplayTime != -1) {
                Log.d(TAG, "Review display time: " + mReviewDisplayTime);
                mHandler.sendEmptyMessageDelayed(FINISH_ACTIVITY, mReviewDisplayTime);
            }
            try {
                mRender = new PanoramaRender(this, mBitmap, mPictureAngle);
            } catch (OutOfMemoryError e) {
//                Toast.makeText(this, getString(R.string.panorama_toast_insufficient_memory), Toast.LENGTH_SHORT).show();
                finish();
                return ;
            }
            view.setRenderer(mRender);
            setContentView(view);

            LayoutInflater inflater = getLayoutInflater();
            getWindow().addContentView(inflater.inflate(R.layout.gallery_panorama_review_menu, null),
                    new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

            mBottomMenuLayout = (LinearLayout)findViewById(R.id.gallery_layout_panorama_review_menu);
            mBottomMenuLayout.getBackground().setAlpha(219);
            mBottomMenuLayout.setOnClickListener(mControlBarLayoutListener);
            mTopMenuLayout = (LinearLayout)findViewById(R.id.gallery_layout_panorama_top_menu);
            mTopMenuLayout.setOnClickListener(mControlBarLayoutListener);
            mFileName = getDefaultPathAccordUri(mUri);
            Log.d(TAG, "file name: " + mFileName);
        }
        else {
            finish();
        }

        Log.d(TAG, "onCreate exit");
    }

    private View.OnClickListener mControlBarLayoutListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            showControlBar();
        }
    };

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_MENU) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    class KeyListener implements DialogInterface.OnKeyListener {
        public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
           switch (keyCode) { // hide back press
               case KeyEvent.KEYCODE_BACK:{
                   return true;
               }
           }

           return true;
       }
   }
    private boolean init(Uri uri) {
        if (uri == null) {
            return false;
        }
        if (mParam != null) {
            mParam.mSort = ImageManager.SORT_DESCENDING;
        }
        if(ViewImage.VIEW_MODE_TIME.equals(getIntent().getStringExtra(ViewImage.EXTRA_VIEW_MODE))) {
            if("internal".equals(mCurrentStoragePos) || uri.toString().contains(Media.INTERNAL_CONTENT_URI.toString())){
                mAllImages = new ImageList(getContentResolver(),Media.INTERNAL_CONTENT_URI, ImageManager.SORT_DESCENDING, mCurrentBucketId);
            }else{
                mAllImages = new ImageList( getContentResolver(),Media.EXTERNAL_CONTENT_URI, ImageManager.SORT_DESCENDING, mCurrentBucketId);
            }
        } else {
            if(mCurrentBucketId != null) {
                Uri tempUri = Images.Media.INTERNAL_CONTENT_URI.buildUpon().appendQueryParameter("bucketId", mCurrentBucketId).build();
                mAllImages = (mParam == null) ? buildImageListFromUri(tempUri) : ImageManager.makeImageList(getContentResolver(), mParam);
            } else {
                mAllImages = (mParam == null) ? buildImageListFromUri(uri) : ImageManager.makeImageList(getContentResolver(), mParam);
            }
        }
        mImage = mAllImages.getImageForUri(uri);
        if (mImage == null) {
            return false;
        }
        mCurrentPosition = mAllImages.getImageIndex(mImage);
        return true;
    }

    private IImageList buildImageListFromUri(Uri uri) {
        return ImageManager.makeImageList(getContentResolver(), uri, ImageManager.SORT_DESCENDING);
    }

    private void deleteFile() {
        File f = new File(mFileName);
        f.delete();
        this.getContentResolver().delete(mUri, null, null);
//        Utils.deleteUri(getContentResolver(), mUri, null, null);
        mUri = null;
        onBackPressed();
    }

    private String getDefaultPathAccordUri(Uri uri) {
        String strPath = null;
        if("file".equals(uri.getScheme())) {
            return uri.getPath();
        }
        final String[] IMAGE_PROJECTION = new String[] {Media.DATA};
        Cursor cr = getContentResolver().query(uri, IMAGE_PROJECTION, null, null, null);
        if(cr != null && cr.getCount() > 0) {
            if(cr.isBeforeFirst()) {
                cr.moveToFirst();
                strPath = cr.getString(cr.getColumnIndex(Media.DATA));
            }
            // CID 109009 : Resource leak (RESOURCE_LEAK)
            // cr.close();
        }

        /* SPRD: CID 109009 (#1 of 1): Resource leak (RESOURCE_LEAK) @{ */
        if(cr != null) {
            cr.close();
        }
        /* @} */

        return strPath;
    }

    private void dismissDlg() {
        if (mReviewDelDlg != null) {
            mReviewDelDlg.dismiss();
            mReviewDelDlg.cancel();
            mReviewDelDlg = null;
        }
    }

    public void onClickGalleryReviewBack(View v) {

        Intent intentImage = new Intent();
        intentImage.setClass(this, BaseImagePicker.class);
        Uri uri = null;
        intentImage.putExtra("mediaTypes", ImageManager.INCLUDE_IMAGES);
        uri = Images.Media.INTERNAL_CONTENT_URI.buildUpon().appendQueryParameter("bucketId", mCurrentBucketId).build();
        intentImage.setData(uri);
        intentImage.putExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY, ViewImage.IMAGE_ENTRY_NORMAL_VALUE);
        intentImage.putExtra(ViewImage.EXTRACURRENT_BUCKID_KEY, mCurrentBucketId);
        intentImage.putExtra(ViewImage.EXTRA_IMAGE_STORAGE_KEY, mCurrentStoragePos);
        startActivity(intentImage);
        finish();
    }
    public void onClickGalleryReviewDel(View v) {
        KeyListener keyListener = new KeyListener();
        OnClickListener buttonListener = new OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                switch (which) {
                    case DialogInterface.BUTTON_POSITIVE:
                        deleteFile();
                        break;
                    case DialogInterface.BUTTON_NEGATIVE :
                        break;
                }
                dismissDlg();
            }
        };
        if (mReviewDelDlg == null) {
            mReviewDelDlg = new AlertDialog.Builder(this)
            .setCancelable(false)
            .setTitle(getString(R.string.text_delete_image_title))
            .setIcon(android.R.drawable.ic_dialog_alert)
            .setMessage(getString(R.string.text_delete_single_message))
            .setPositiveButton(R.string.picture_delete_ok, buttonListener)
            .setNegativeButton(R.string.picture_delete_cancel, buttonListener)
            .create();
            mReviewDelDlg.setOnKeyListener(keyListener);
            mReviewDelDlg.show();
        }
    }

    public void onClickGalleryReviewEdit(View v) {
        try{
            Intent intent = new Intent();
            intent.setAction("android.intent.action.UGALLERY_EDIT");
            //intent.setClassName("com.ucamera.ucam","com.ucamera.uphoto.ImageEditControlActivity");

            intent.setDataAndType(mUri, "image/*");
            intent.putExtra("PictureDegree", 0);
            //Within the camera application, the value of the parameter is true,
            //this means that advanced editing interface, click the Save button
            //several times only to save the final picture.
            intent.putExtra("extra_from_inner", true);
            intent.putExtra("ImageFileName", mFileName);
            // Keep the camera instance for a while.
            // This avoids re-opening the camera and saves time.
            startActivity(intent);
            overridePendingTransition(0, 0);
        }catch(ActivityNotFoundException e) {
            findUCamApp("edit");
        }
    }
    void findUCamApp(String function) {
        if(mTipDialog == null) {
            OnClickListener listener = new OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    mTipDialog.dismiss();
                    mTipDialog = null;
                    switch (which) {
                        case DialogInterface.BUTTON_POSITIVE:
                            if (Util.checkNetworkShowAlert(UgalleryPanoramaActivity.this)) {
                                Intent intent = new Intent();
                                String ucamPkgName = getPackageName().replace("ugallery", "ucam");
                                intent.setData(Uri.parse("market://details?id=" + ucamPkgName));
                                try {
                                    startActivity(intent);
                                } catch (ActivityNotFoundException e) {
                                    Toast.makeText(UgalleryPanoramaActivity.this, R.string.text_not_installed_market_app, Toast.LENGTH_LONG).show();
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

    public void onClickGalleryReviewShare(View v) {
//        ShareUtils.shareImage(this, mUri);
    }

    public void onClickGalleryMore(View v) {
        showPopupWindow(mImage);
    }
    private void showPopupWindow(final IImage image) {
        View contentView = View.inflate(getApplicationContext(),
                R.layout.popup_menu_item, null);
        View.OnClickListener listener =  new View.OnClickListener(){

            @Override
            public void onClick(View v) {
                switch (v.getId()) {
                case R.id.popup_menu_rename:
                    renameFile(mImage);
                    break;
                case R.id.popup_menu_details:
                    ViewImageCommonUtil.showImageDetails(UgalleryPanoramaActivity.this, mImage);
                    break;
                case R.id.popup_menu_setas:
                    Intent intent = Util.createSetAsIntent(mImage);
                    try {
                        startActivity(Intent.createChooser(
                                intent, getText(R.string.setImage)));
                    } catch (android.content.ActivityNotFoundException ex) {
                        Toast.makeText(UgalleryPanoramaActivity.this, R.string.no_way_to_share_video,
                                Toast.LENGTH_SHORT).show();
                    }
                    break;
                default:
                    break;
                }
                mPopupWindow.dismiss();
                hideControlBar();
            }
        };
//        contentView.findViewById(R.id.popup_menu_rotate_left).setOnClickListener(listener);
        contentView.findViewById(R.id.popup_menu_rename).setOnClickListener(listener);
        contentView.findViewById(R.id.popup_menu_details).setOnClickListener(listener);
        contentView.findViewById(R.id.popup_menu_setas).setOnClickListener(listener);
        mPopupWindow = new PopupWindow(contentView ,LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT, true);
        mPopupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        mPopupWindow.setFocusable(true);
        mPopupWindow .setOutsideTouchable(true);
        mPopupWindow.showAtLocation(mBottomMenuLayout, Gravity.RIGHT|Gravity.BOTTOM, 0, mBottomMenuLayout.getHeight()-8);
    }

    private void renameFile(final IImage image) {
        String oriPath = image.getDataPath();
        final File file = new File(oriPath);
        View view = LayoutInflater.from(UgalleryPanoramaActivity.this).inflate(R.layout.gallery_rename_dialog, null);
        final EditText editText = (EditText) view.findViewById(R.id.editText);
        mTitle = image.getTitle();
        /*
         * FIX BUG: 5005
         * FIX COMMENT: Modify the naming panorama mode ;
         * DATE: 2013-10-11
         */
        if(mTitle != null && mTitle.startsWith(ImageListSpecial.PANORAMA_PREFIX)) {
            mTitle = mTitle.substring(ImageListSpecial.PANORAMA_PREFIX.length());
        }
        editText.setText(mTitle);
        final String suffix = Util.getSuffix(oriPath);

        OnClickListener listener = new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                switch (which) {
                case DialogInterface.BUTTON_POSITIVE:
                    String modifyName = ImageListSpecial.PANORAMA_PREFIX.concat(editText.getText().toString());
                    final String fpath = file.getParentFile().getPath();
                    final File newFile = new File(fpath + "/"+ modifyName.concat(suffix));

                    if (newFile.exists()) {
                        Util.displayToast(UgalleryPanoramaActivity.this, getString(R.string.text_image_rename_file_exist));
                        ViewImageCommonUtil.setDialogDissmiss(dialog, false);
                    } else {
                        if (file.renameTo(newFile)) {
                            boolean sucessed = ImageManager.updateRenamedImage(UgalleryPanoramaActivity.this
                                            .getContentResolver(), newFile.getAbsolutePath(),
                                    modifyName, suffix, image ,mCurrentStoragePos);
                            if (sucessed) {
//                                init(image.fullSizeImageUri());
//                                setImage(mCurrentPosition);
                                mImage.rename(modifyName);
                                Util.displayToast(UgalleryPanoramaActivity.this, getString(R.string.text_image_rename_file_success));
                            } else {
                                Util.displayToast(UgalleryPanoramaActivity.this, getString(R.string.text_image_rename_file_failed));
                            }
                        } else {
                            Util.displayToast(UgalleryPanoramaActivity.this, getString(R.string.text_image_rename_file_failed));
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
        final AlertDialog renameDialog = new AlertDialog.Builder(UgalleryPanoramaActivity.this)
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
                if(mTitle.equals(editText.getText().toString()) || editText.getText().length() == 0
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
    private Bitmap getBitmapAccordingUri(Uri uri) throws FileNotFoundException {
        ParcelFileDescriptor pfd = this.getContentResolver().openFileDescriptor(uri, "r");
        FileDescriptor fd = pfd.getFileDescriptor();
        BitmapFactory.Options options = Util.getNativeAllocOptions();
        options.inSampleSize = 1;
        Bitmap bitmap = null;
        try {
             bitmap = BitmapFactory.decodeFileDescriptor(fd, null, options);
        } catch (OutOfMemoryError e) {
            bitmap = null;
            Log.w(TAG, "getBitmapAccordingUri(): code has a memory leak is detected...");
            System.gc();
        }

        return bitmap;
    }

    public void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");
        mPausing = false;
        if (mReviewDisplayTime != -1) {
            mHandler.sendEmptyMessageDelayed(FINISH_ACTIVITY, mReviewDisplayTime);
        }
        if (mRender != null) {
            mRender.onResume();
        }
        //StatApi.onResume(this);
        Log.d(TAG, "onResume exit");
    }

    public void onPause() {
        Log.d(TAG, "onPause");
        mPausing = true;
        if (mRender != null) {
            mRender.onPause();
        }
        if (mHandler != null) {
            mHandler.removeMessages(FINISH_ACTIVITY);
            mHandler.removeCallbacks(mHideControlBarRunnable);
        }
        hideControlBar();
        super.onPause();
        //StatApi.onPause(this);
        Log.d(TAG, "onPause exit");
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestory");
        if (mBitmap != null && !mBitmap.isRecycled()) {
            mBitmap.recycle();
            mBitmap = null;
        }
        super.onDestroy();
        Log.d(TAG, "onDestory exit");
    }

    private float mPrevTouchPosX;
    public boolean onTouchEvent(MotionEvent event) {
        super.onTouchEvent(event);
        /*
         * FIX BUG: 1388
         * BUG CAUSE: supported null pointer
         * Date: 2012-07-30
         */
        if(mRender != null){
            mRender.onTouchEvent(event);
        }
        int pCount = event.getPointerCount();
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
        case MotionEvent.ACTION_DOWN :
            mPrevTouchPosX = (int) event.getX();
            break;
        case MotionEvent.ACTION_UP :
            if(pCount == 1 && Math.abs(event.getX() - mPrevTouchPosX) < 10){
                if (mReviewDisplayTime != -1) {
                    return true;
                }
                /*
                 * FIX BUG: 5836
                 * BUG CAUSE: supported null pointer
                 * Date: 2014-01-20
                 */
                if (mBottomMenuLayout != null && mBottomMenuLayout.isShown()) {
                    hideControlBar();
                }else {
                    showControlBar();
                }
              return true;
            }
            break;
        }
        return true;
    }

    /*
     * FIX BUG : 4733
     * BUG COMMENT : add delay 5s auto hide control bar feature
     * DATE : 2013-08-26
     */
    private void showControlBar() {
        /*
         * FIX BUG: 6134
         * BUG COMMENT: avoid null pointer exception
         * DATE: 2014-03-19
         */
        if(mBottomMenuLayout != null) {
            mBottomMenuLayout.setVisibility(View.VISIBLE);
        }
        if(mTopMenuLayout != null) {
            mTopMenuLayout.setVisibility(View.VISIBLE);
        }
        if (mHandler != null) {
            mHandler.removeCallbacks(mHideControlBarRunnable);
            mHandler.postDelayed(mHideControlBarRunnable, 5000);
        }
    }

    private void hideControlBar() {
        /*
         * FIX BUG: 5433
         * BUG COMMENT: avoid null pointer exception
         * DATE: 2013-11-25
         */
        if(mBottomMenuLayout != null) {
            mBottomMenuLayout.setVisibility(View.GONE);
        }
        if(mTopMenuLayout != null) {
            mTopMenuLayout.setVisibility(View.GONE);
        }
        if(mPopupWindow != null && mPopupWindow.isShowing()) {
            mPopupWindow.dismiss();
        }
    }

    private Runnable mHideControlBarRunnable =  new Runnable() {
        @Override
        public void run() {
            hideControlBar();
        }
    };

}
