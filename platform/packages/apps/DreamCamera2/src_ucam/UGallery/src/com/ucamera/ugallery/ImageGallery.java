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

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.OperationApplicationException;
import android.content.SharedPreferences;
import android.database.ContentObserver;
import android.database.Cursor;
import android.database.sqlite.SQLiteException;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.Messenger;
import android.os.Parcelable;
import android.os.RemoteException;
import android.os.StatFs;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Images.Media;
import android.text.SpannableStringBuilder;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.camera.storagepath.StorageUtil;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.ucamera.ugallery.MediaScanner.ScannerFinishCallBack;
import com.ucamera.ugallery.gallery.IImage;
import com.ucamera.ugallery.gallery.IImageList;
import com.ucamera.ugallery.gallery.ImageList;
import com.ucamera.ugallery.gallery.UriImage;
import com.ucamera.ugallery.gallery.privateimage.ImagePassWordDialog;
import com.ucamera.ugallery.gallery.privateimage.ImagePassWordDialog.PrivateAbulmCallback;
import com.ucamera.ugallery.gallery.privateimage.SecretDialog;
import com.ucamera.ugallery.gallery.privateimage.SecretDialog.SecretDialogOnClickListener;
import com.ucamera.ugallery.gallery.privateimage.util.Constants;
import com.ucamera.ugallery.gallery.privateimage.util.LockUtil;
import com.ucamera.ugallery.gallery.privateimage.util.PasswordUtils;
import com.ucamera.ugallery.integration.Build;
import com.ucamera.ugallery.integration.DialogUtils;
import com.ucamera.ugallery.preference.UGalleryPreferenceActivity;
import com.ucamera.ugallery.provider.UCamData;
import com.ucamera.ugallery.util.BitmapManager;
import com.ucamera.ugallery.util.CheckVersion;
import com.ucamera.ugallery.util.Compatible;
import com.ucamera.ugallery.util.ImageManager;
import com.ucamera.ugallery.util.Models;
import com.ucamera.ugallery.util.StorageUtils;
import com.ucamera.ugallery.util.UiUtils;
import com.ucamera.ugallery.util.UpdateService;
import com.ucamera.ugallery.util.Util;

public class ImageGallery extends Activity implements OnClickListener{
    protected static final String TAG = "ImageGallery";

    private Thread mWorkerThread;
    private BroadcastReceiver mReceiver;
    private ContentObserver mDbObserver;
    private MyGridView mGridView;
    private GalleryAdapter mAdapter; // mAdapter is only accessed in main thread.
    private boolean mScanning;
    private boolean mUnmounted;
    private Dialog mMediaScanningDialog;
    private Dialog mLockFinishDialog;
    private ImageListData[] mImageListData = null;
    private Drawable mCellOutline;
    public static final String TELECOM_CLOUD_PATH = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM).toString() + "/Camera";
    public static final String INTENT_EXTRA_IMAGE_GALLERY_CLASS = "Extra.ImageGallery.Class";
    private Class<? extends BaseImagePicker> mImageGalleryImpl;
    public static final String INTENT_EXTRA_WAIT_FOR_RESULT = "Extra.Wait.For.Result";
    private boolean mIsRequestForResult = false;
    private int mImageMaxLimit = -1;
    public static final String INTENT_EXTRA_IMAGES = "Extra.Image.Uris";
    public static final String INTENT_EXTRA_IMAGE_MAX_LIMIT ="Extra.Image.MaxLimit";
    private String mGalleryEntry = null;
    private int mUPhotoModule = -1;
    private boolean mIsLongPressed = false;
    private int mSelectedItemIndex = -1;
    private ImageView mDeleteImageView = null;
    private RelativeLayout mButtonLayout = null;
    private Animation FadeIn,FadeOut;
    private String mFileDirectoryOfDrag= null;
    private ArrayList<ContentProviderOperation> mCoverList;
    private static final int LOCK_FINISH = 100;
    private static final int UNLOCK_FINISH = 101;
    private static final int SHOW_LOCKED_IMAGES = 102;
    private static final int SHOW_SECRET_HINT = 103;
    private static final int UNLOCK_FAILED = 104;
    private static final int LOCK_FAILED = 105;
    public static final int BACK_UP_ALLIMAGES_FINISH = 107;
    public static final int SHOW_BACK_UP_ALLIMAGES_DIALOG = 108;
    private boolean mPausing ;
    private static String mCurrentStoragePos = "external";
    private PopupWindow mPopupWindow;
    private boolean mIsShowLockedImage;
    private ImagePassWordDialog mImagePassWordDialog;
    private int mDistanceToBottom;
    // handler for the main thread
    private Handler mHandler = new Handler(){


        public void handleMessage(Message msg) {
            int what = msg.what;
            switch (what) {
                case UGalleryConst.GOTO_UPDATE:
                    String newVersionTemp = (String) msg.obj; // newVersionTemp is like "1.0.1.120601#xxxxxx"
                    int index = newVersionTemp.indexOf("#");
                    if (index == -1) {
                        Toast.makeText(ImageGallery.this, newVersionTemp, Toast.LENGTH_SHORT).show();
                    } else {
                        final String updateinfo = newVersionTemp.substring(index + 1,
                                newVersionTemp.length());
                        final String newVersion = newVersionTemp.substring(0, index);
                        SpannableStringBuilder mReleaseListContent = new SpannableStringBuilder(
                                updateinfo);
                        new AlertDialog.Builder(ImageGallery.this)
                                .setTitle(
                                        R.string.update_info)
                                .setIcon(android.R.drawable.ic_dialog_info)
                                .setMessage(mReleaseListContent)
                                .setPositiveButton(R.string.update_ok,
                                        new DialogInterface.OnClickListener() {
                                            public void onClick(DialogInterface dialog, int which) {
                                                if (newVersion.startsWith(UpdateService.DIRECTORY)) {
                                                    File apkFile = new File(newVersion);
                                                    Intent intent = new Intent();
                                                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                                                    intent.setAction(android.content.Intent.ACTION_VIEW);
                                                    intent.setDataAndType(
                                                            Uri.fromFile(apkFile),
                                                            "application/vnd.android.package-archive");
                                                    startActivity(intent);
                                                } else {
                                                    startDownloadService(newVersion, updateinfo);
                                                }
                                            }
                                        })
                                    .setNegativeButton(R.string.update_cancel, null)
                                    .show();
                    }
                    break;
                case LOCK_FINISH:
                    if(mIsShowLockedImage) {
                        if(mAdapter == null)
                            return;
                        mAdapter.clear();
                        startWorker();
                    } else {
                        if(mAdapter != null)
                            mAdapter.removeItem(mSelectedItemIndex);
                    }
                    if(mLockFinishDialog !=null) {
                        mLockFinishDialog.cancel();
                        mLockFinishDialog = null;
                    }
                    break;
                case UNLOCK_FINISH:
                    if(mAdapter == null)
                        return;
                    mAdapter.clear();
                    startWorker();
                    if(mLockFinishDialog !=null) {
                        mLockFinishDialog.cancel();
                        mLockFinishDialog = null;
                    }
                    break;
                case SHOW_LOCKED_IMAGES:
                    if(mAdapter == null)
                        return;
                    mAdapter.clear();
                    startWorker();
                    if(mLockFinishDialog !=null) {
                        mLockFinishDialog.cancel();
                        mLockFinishDialog = null;
                    }
                    break;
                case UNLOCK_FAILED:
                    if(mLockFinishDialog !=null) {
                        mLockFinishDialog.cancel();
                        mLockFinishDialog = null;
                    }
                    Toast.makeText(ImageGallery.this, R.string.text_image_unlock_failed, Toast.LENGTH_SHORT).show();
                    break;
                case LOCK_FAILED:
                    if(mLockFinishDialog !=null) {
                        mLockFinishDialog.cancel();
                        mLockFinishDialog = null;
                    }
                    Toast.makeText(ImageGallery.this, R.string.text_image_lock_failed, Toast.LENGTH_SHORT).show();
                    break;
                case SHOW_SECRET_HINT:
                    if (Build.NEED_SECRET_ABLUM && !mPickIntentSelectPhotos && !mSharePref.getBoolean(UGalleryConst.KEY_SECRET_ABLUM_FRIST_SHOW_TIPS_FINISHED, false)) {
                        mSecretStub.setVisibility(View.VISIBLE);
                        mStubViewHint = findViewById(R.id.secret_ablum_hint);
                        mStubViewHint.setOnTouchListener(new View.OnTouchListener( ) {
                            @Override
                            public boolean onTouch(View v, MotionEvent event) {
                                v.setVisibility(View.GONE);
                                return true;
                            }
                        });
                        mSharePref.edit().putBoolean(UGalleryConst.KEY_SECRET_ABLUM_FRIST_SHOW_TIPS_FINISHED, true).commit();
                    }
                    break;
                case SHOW_BACK_UP_ALLIMAGES_DIALOG:
                    new SecretDialog(ImageGallery.this , new SecretDialogOnClickListener() {
                        @Override
                        public void secretDialogOnClick() {
                            mImageRestoreDialog = ProgressDialog.show(ImageGallery.this, null, getResources().getString(
                                    R.string.text_waiting), true, false);
                            ArrayList<String> imageRestorelist = new ArrayList<String>();
                            if(new File(Constants.STORE_DIR_LOCKED).exists()) {
                                imageRestorelist =  LockUtil.restoreAllImages(Constants.STORE_DIR_LOCKED, new File(Constants.STORE_DIR_LOCKED) , imageRestorelist , ImageGallery.this);
                            }
                            if(new File(Constants.STORE_DIR_LOCKED_PHOTOGRAPY).exists()) {
                                imageRestorelist =  LockUtil.restoreAllImages(Constants.STORE_DIR_LOCKED_PHOTOGRAPY, new File(Constants.STORE_DIR_LOCKED_PHOTOGRAPY) , imageRestorelist, ImageGallery.this);
                            }

                            new MediaScanner(ImageGallery.this, new ScannerFinishCallBack() {
                                @Override
                                public void scannerComplete() {
                                    mHandler.sendEmptyMessage(ImageGallery.BACK_UP_ALLIMAGES_FINISH);
                                }
                            }).scanFile(imageRestorelist, null);
                        }
                    }).show();
                    break;
                case BACK_UP_ALLIMAGES_FINISH:
                    if(mAdapter == null)
                        return;
                    mAdapter.clear();
                    startWorker();
                    if(mImageRestoreDialog !=null) {
                        mImageRestoreDialog.cancel();
                        mImageRestoreDialog = null;
                    }
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        /*SPRD:fix bug518361 Some Activity about UCamera lacks method of checkpermission@{ */
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
        setContentView(R.layout.common_image_gallery);
        UiUtils.initialize(this);
        mDistanceToBottom = UiUtils.dpToPixel(160);
        mSharePref = PreferenceManager.getDefaultSharedPreferences(this);
        /**
         * FIX BUG: 5815
         * BUG CAUSE: colse the secret gallery in 4.0.3-4.0.4 version
         * DATE:2014-02-10
         */
        if(android.os.Build.VERSION.SDK_INT == 15 ) {
            Build.NEED_SECRET_ABLUM = false;
            new Thread(new Runnable() {
                @Override
                public void run() {
                    if(LockUtil.isExistHidenImages(Constants.STORE_DIR_LOCKED) || LockUtil.isExistHidenImages(Constants.STORE_DIR_LOCKED_PHOTOGRAPY)) {
                        mHandler.sendEmptyMessage(SHOW_BACK_UP_ALLIMAGES_DIALOG);
                    }
                }
            }).start();
        }
        findViewById(R.id.nav_to_album).setVisibility(View.VISIBLE);
        findViewById(R.id.nav_to_select_photos).setVisibility(View.GONE);
        FadeIn = new TranslateAnimation(Animation.RELATIVE_TO_SELF, 0, Animation.RELATIVE_TO_SELF, 0, Animation.RELATIVE_TO_SELF, 70, Animation.RELATIVE_TO_SELF, 0);
        FadeOut = new TranslateAnimation(Animation.RELATIVE_TO_SELF, 0, Animation.RELATIVE_TO_SELF, 0, Animation.RELATIVE_TO_SELF, 0, Animation.RELATIVE_TO_SELF, 70);
        FadeIn.setDuration(500);
        FadeIn.setFillAfter(true);
        FadeOut.setDuration(1000);
        FadeOut.setFillAfter(true);
        mDeleteTextView = (TextView) findViewById(R.id.btn_multi_sel_operate);
//        mDeleteTextView = (TextView) findViewById(R.id.select_image_tv);
//        mDeleteImageView.setImageResource(R.drawable.gallery_image_action_del_default);
        Util.setTextAndDrawableTop(getApplicationContext(), mDeleteTextView, R.drawable.gallery_image_action_del_default, R.string.text_delete_image);
        mButtonLayout = (RelativeLayout)findViewById(R.id.layout_bottom_normal_action);
        mLockLayout = (LinearLayout)findViewById(R.id.layout_lock_image);
        mLockTextView = (TextView)findViewById(R.id.btn_lock_image);
        mButtonLayout.setVisibility(View.GONE);
        mSecretStub = (ViewStub)findViewById(R.id.secret_ablum_hint_stub);
        mGridView = (MyGridView) findViewById(R.id.albums);
        mGridView.setSelector(android.R.color.transparent);
        mGridView.setOnItemClickListener(new AdapterView.OnItemClickListener() {

            public void onItemClick(AdapterView<?> parent, View view, final int position, long id) {
                if(mAdapter.isLockedIcon(position) && PasswordUtils.isPasswordFileExist()) {
                    mImagePassWordDialog = new ImagePassWordDialog(ImageGallery.this, false, new PrivateAbulmCallback() {
                        @Override
                        public void controlPrivateAbulmOFF() {
                            mImagePassWordDialog.dismiss();
                        }
                        @Override
                        public void controlPrivateAbulmNO() {
                            launchFolderGallery(position);
                        }
                    });
                    mImagePassWordDialog.show();
                } else {
                    launchFolderGallery(position);
                }
            }
        });
        mGridView.setOnItemLongClickListener(new OnItemLongClickListener(){
            @Override
            public boolean onItemLongClick(AdapterView<?> parent, View view,
                    int position, long id) {
                if(ViewImage.IMAGE_ENTRY_NORMAL_VALUE.equals(getGalleryEntry()) || ViewImage.IMAGE_ENTRY_UGALLERY_VALUE.equals(getGalleryEntry())){
                    mIsLongPressed = true;
                    mGridView.setSelectedItem(view);
                    mSelectedItemIndex = position;
                    mHandler.post(new Runnable() {
                        public void run() {
                            if(Build.NEED_SECRET_ABLUM) {
                                if(mAdapter.isLockedIcon(mSelectedItemIndex)) {
                                    Util.setTextAndDrawableTop(getApplicationContext(), mLockTextView, R.drawable.gallery_cancel_private, R.string.secret_ablum_remove);
                                } else {
                                    Util.setTextAndDrawableTop(getApplicationContext(), mLockTextView, R.drawable.gallery_add_private, R.string.secret_ablum_add);
                                }
                                mLockLayout.setVisibility(View.VISIBLE);
                            } else {
                                mLockLayout.setVisibility(View.GONE);
                            }
                            findViewById(R.id.layout_multi_sel_operate).setVisibility(View.VISIBLE);
                            Util.setTextAndDrawableTop(getApplicationContext(), mDeleteTextView, R.drawable.gallery_image_action_del_default, R.string.text_delete_image);
                            mButtonLayout.setVisibility(View.VISIBLE);
                            mButtonLayout.startAnimation(FadeIn);
                            if(Models.getModel().equals(Models.AMAZON_KFTT))
                            mButtonLayout.setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
                        }
                    });
                }
                return false;
            }

        });

        mGridView.setOnTouchListener(new OnTouchListener(){
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch(event.getAction()){
                case MotionEvent.ACTION_DOWN:
                    if(mIsLongPressed){
                        mGridView.downItem(event);
                    }
                    break;
                case MotionEvent.ACTION_MOVE:
                    if(mIsLongPressed){
                        setBottomParams(5, 80);
                        mGridView.moveItem(event);
                        if(Build.NEED_SECRET_ABLUM) {
                            if(event.getY() > getWindowManager().getDefaultDisplay().getHeight() - mDistanceToBottom){
                                if( event.getX() > getWindowManager().getDefaultDisplay().getWidth() /2) {
                                    Util.setTextAndDrawableTop(getApplicationContext(), mDeleteTextView, R.drawable.gallery_image_action_del_pressed, Constants.NO_STRING);
                                    if(mAdapter.isLockedIcon(mSelectedItemIndex)) {
                                        Util.setTextAndDrawableTop(getApplicationContext(), mLockTextView, R.drawable.gallery_cancel_private, Constants.NO_STRING);

                                    } else {
                                        Util.setTextAndDrawableTop(getApplicationContext(), mLockTextView, R.drawable.gallery_add_private, Constants.NO_STRING);
                                    }
                                } else {
                                    mLockLayout.setVisibility(View.VISIBLE);
                                    if(mAdapter.isLockedIcon(mSelectedItemIndex)) {
                                        Util.setTextAndDrawableTop(getApplicationContext(), mLockTextView, R.drawable.gallery_cancel_private_selected, Constants.NO_STRING);
                                    } else {
                                        Util.setTextAndDrawableTop(getApplicationContext(), mLockTextView, R.drawable.gallery_add_private_selected, Constants.NO_STRING);
                                    }
                                    Util.setTextAndDrawableTop(getApplicationContext(), mDeleteTextView, R.drawable.gallery_image_action_del_default,  Constants.NO_STRING);
                                }
                            } else {
                                Util.setTextAndDrawableTop(getApplicationContext(), mDeleteTextView, R.drawable.gallery_image_action_del_default, Constants.NO_STRING);
                                mLockLayout.setVisibility(View.VISIBLE);
                                if(mAdapter == null) break;
                                if(mAdapter.isLockedIcon(mSelectedItemIndex)) {
                                    Util.setTextAndDrawableTop(getApplicationContext(), mLockTextView, R.drawable.gallery_cancel_private, Constants.NO_STRING);
                                } else {
                                    Util.setTextAndDrawableTop(getApplicationContext(), mLockTextView, R.drawable.gallery_add_private, Constants.NO_STRING);
                                }
                            }
                        } else {
                            if(event.getY() > getWindowManager().getDefaultDisplay().getHeight() - mDistanceToBottom){
                                Util.setTextAndDrawableTop(getApplicationContext(), mDeleteTextView, R.drawable.gallery_image_action_del_pressed, Constants.NO_STRING);
                            } else {
                                Util.setTextAndDrawableTop(getApplicationContext(), mDeleteTextView, R.drawable.gallery_image_action_del_default, Constants.NO_STRING);
                            }
                        }
                    }
                    break;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    if (mIsLongPressed) {
                        setBottomParams(0, 0);
                        String bucketId = mAdapter.getItemmBucketId(mSelectedItemIndex);
                        if(Build.NEED_SECRET_ABLUM) {
                            if (event.getY() > getWindowManager().getDefaultDisplay().getHeight() - mDistanceToBottom && mSelectedItemIndex > -1) {
                                if( event.getX() > getWindowManager().getDefaultDisplay().getWidth() /2) {
//                                mDeleteImageView.setImageResource(R.drawable.gallery_image_action_del_default);
                                    Util.setTextAndDrawableTop(getApplicationContext(), mDeleteTextView, R.drawable.gallery_image_action_del_default,  Constants.NO_STRING);
                                    if(ImageManager.isVideoBucketId(bucketId)) {
                                        MenuHelper.deleteVideo(ImageGallery.this, mDeleteImageRunnable, 3);
                                    } else {
                                        MenuHelper.deleteImage(ImageGallery.this, mDeleteImageRunnable, 3);
                                    }
                                } else {
                                    if(mAdapter.isLockedIcon(mSelectedItemIndex)) {
//                                    mLockImageView.setImageResource(R.drawable.gallery_cancel_private);
                                        Util.setTextAndDrawableTop(getApplicationContext(), mLockTextView, R.drawable.gallery_cancel_private, Constants.NO_STRING);
                                        if(PasswordUtils.isPasswordFileExist()) {
                                            mImagePassWordDialog = new ImagePassWordDialog(ImageGallery.this, false, new PrivateAbulmCallback() {
                                                @Override
                                                public void controlPrivateAbulmOFF() {
                                                    mImagePassWordDialog.dismiss();
                                                }
                                                @Override
                                                public void controlPrivateAbulmNO() {
                                                    MenuHelper.unLockImage(ImageGallery.this, mUnLockImageRunnable);
                                                }
                                            });
                                            mImagePassWordDialog.show();
                                        } else {
                                            MenuHelper.unLockImage(ImageGallery.this, mUnLockImageRunnable);
                                        }
                                    } else {
                                        Util.setTextAndDrawableTop(getApplicationContext(), mLockTextView, R.drawable.gallery_add_private, Constants.NO_STRING);
//                                    mLockImageView.setImageResource(R.drawable.gallery_add_private);
                                        MenuHelper.lockImage(ImageGallery.this, mLockImageRunnable);
                                    }
                                }
                            }
                        } else {
                            if (event.getY() > getWindowManager().getDefaultDisplay().getHeight() - mDistanceToBottom && mSelectedItemIndex > -1) {
                                Util.setTextAndDrawableTop(getApplicationContext(), mDeleteTextView, R.drawable.gallery_image_action_del_default,  Constants.NO_STRING);
                                if(ImageManager.isVideoBucketId(bucketId)) {
                                    MenuHelper.deleteVideo(ImageGallery.this, mDeleteImageRunnable, 3);
                                } else {
                                    MenuHelper.deleteImage(ImageGallery.this, mDeleteImageRunnable, 3);
                                }
                            }
                        }
                        mGridView.upItem();
                        mHandler.post(new Runnable() {
                            public void run() {
                                mButtonLayout.startAnimation(FadeOut);
                                mButtonLayout.setVisibility(View.GONE);
                            }
                        });
                    }
                    break;
                }
                /*
                 * FIX BUG: 4592
                 * BUG COMMENT: not dispatch move event in long pressed mode
                 * DATE: 2014-02-11
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

        if (Compatible.instance().mIsMeizuManufacturer) {
            mImageListData = new ImageListData[]{
                    new ImageListData(Item.TYPE_CAMERA_IMAGES,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.UPHOTO_BUCKET_ID,
                            "UPhoto"),
                    new ImageListData(Item.TYPE_CAMERA_IMAGES,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.UPHOTO_BACKUP_BUCKET_ID,
                            "UPhoto"),
                    new ImageListData(Item.TYPE_CAMERA_IMAGES,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.MEIZU_UCAM_BUCKET_ID,
                            "UCam"),
                    // UCam Images backup
                    new ImageListData(Item.TYPE_CAMERA_IMAGES,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.MEIZU_UCAM_BACKUP_BUCKET_ID,
                            "UCam"),
                    // M9 default Camera Images
                    new ImageListData(Item.TYPE_CAMERA_IMAGES,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.IMAGE_BUCKET_NAME_CAMERA_MEIZU_ID,
                            getString(R.string.meizu_camera_default_name)),
                    // UCam Camera images
                    new ImageListData(Item.TYPE_CAMERA_IMAGES,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.getCameraImageBucketId(),
                            ImageManager.getMeizuUCamName()),
                    // UCam Camera Images backup
                    new ImageListData(Item.TYPE_CAMERA_IMAGES,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.CAMERA_IMAGE_BACKUP_BUCKET_ID,
                            ImageManager.getMeizuUCamName()),
                    // All Videos
                    new ImageListData(Item.TYPE_ALL_VIDEOS,
                            ImageManager.INCLUDE_VIDEOS, Constants.ALL_VIDEOS_BUCKET_ID,
                            getString(R.string.all_videos)),
                    // All Videos
                    new ImageListData(Item.TYPE_ALL_VIDEOS,
                            ImageManager.INCLUDE_VIDEOS, ImageManager.ALL_VIDEO_LOCKED_BUCKET_ID,
                            getString(R.string.all_videos)),
            };
        } else {
            mImageListData = new ImageListData[] {
                    new ImageListData(Item.TYPE_CAMERA_IMAGES,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.UPHOTO_BUCKET_ID,
                            "UPhoto"),
                    // Uphoto Images backup
                    new ImageListData(Item.TYPE_CAMERA_IMAGES,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.UPHOTO_BACKUP_BUCKET_ID,
                            "UPhoto"),
                    new ImageListData(Item.TYPE_CAMERA_IMAGES,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.UCAM_BUCKET_ID,
                            "UCam"),
                    // UCam Images backup
                    new ImageListData(Item.TYPE_CAMERA_IMAGES,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.UCAM_BACKUP_BUCKET_ID,
                            "UCam"),
                    // Camera Images
                    new ImageListData(Item.TYPE_CAMERA_IMAGES,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.CAMERA_IMAGE_BUCKET_ID,
                            getString(R.string.text_camera_bucket_name)),
                 // Camera Images backup
                    new ImageListData(Item.TYPE_CAMERA_IMAGES,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.CAMERA_IMAGE_BACKUP_BUCKET_ID,
                            getString(R.string.text_camera_bucket_name)),
                    // All Videos
                    new ImageListData(Item.TYPE_ALL_VIDEOS,
                            ImageManager.INCLUDE_VIDEOS, Constants.ALL_VIDEOS_BUCKET_ID,
                            getString(R.string.all_videos)),
                         // All Videos
                    new ImageListData(Item.TYPE_ALL_VIDEOS,
                            ImageManager.INCLUDE_VIDEOS, ImageManager.ALL_VIDEO_LOCKED_BUCKET_ID,
                            getString(R.string.all_videos)),
            };
        }

        mReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                onReceiveMediaBroadcast(intent);
            }
        };

        mDbObserver = new ContentObserver(mHandler) {
            @Override
            public void onChange(boolean selfChange) {
                if (mPausing) {
                    rebake(false, ImageManager.isMediaScannerScanning(getContentResolver()));
                }
            }
        };

        ImageManager.ensureOSXCompatibleFolder();
        initImageGalleryFromIntent(getIntent());
        if(!Build.HIDE_ADVANCE_SETTINGS) {
            checkVersionUpdate();
        }
    }

    private void setBottomParams(int top, int bottom) {
        if(Models.getModel().equals(Models.AMAZON_KFTT)) {
            RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) mButtonLayout.getLayoutParams();
            params.topMargin = top;
            params.bottomMargin = bottom;
            mButtonLayout.setLayoutParams(params);
        }
    }
    private void checkVersionUpdate() {
        SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
        String buildDisplay = android.os.Build.DISPLAY;
        boolean isCheckUpdate = sharedPreferences.getBoolean(UGalleryConst.KEY_UGALLERY_UPDATE, true);
        Log.d(TAG, "checkVersionUpdate(): build display id :" + buildDisplay + ", isCheckUpdate = " + isCheckUpdate);
        if ((buildDisplay == null || !buildDisplay.contains("smartdroid")) && isCheckUpdate) {
            if (mHandler != null) {
                new CheckVersion().toUpdate(this, mHandler);
            }
        }
    }

    private void startDownloadService(String version, String updateinfo) {
        if (!CheckVersion.isInternalDownload()) {
            try {
                startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("market://search?q=pname:"
                        + getPackageName())));
            } catch (Exception e) {
                new AlertDialog.Builder(this)
                        .setIcon(android.R.drawable.ic_dialog_alert)
                        .setTitle(android.R.string.dialog_alert_title)
                        .setMessage(R.string.market_software_missing)
                        .setPositiveButton(android.R.string.ok, null)
                        .show();
            }
            return;
        }
        long lastUpdateTime = 0;
        String strURL = null;
        if (version.startsWith("http://")) {
            strURL = version;
        } else {
            strURL = CheckVersion.makeUpdatURL(ImageGallery.this, version);
        }

        lastUpdateTime = System.currentTimeMillis();
        SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
        Util.upgradeGlobalPreferences(sharedPreferences);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putLong("lastUpdateTime", lastUpdateTime);
//        editor.putString(CameraSettings.KEY_SHORTCUT_NEVER_REMIND, "0");
        editor.commit();
        Intent intent = new Intent();
        intent.setClass(ImageGallery.this, UpdateService.class);
        intent.putExtra("serverAddress", strURL);
        startService(intent);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mPausing = false;
        if(mDelProgressDialog != null && mDelProgressDialog.isShowing()) {
            mHandler.post(mDeleteNextRunnable);
        }
    }
    @Override
    protected void onPause() {
        super.onPause();
        mPausing = true;
    }

    @Override
    public void onBackPressed() {
        if( mStubViewHint!= null && mStubViewHint.isShown()) {
            mStubViewHint.setVisibility(View.INVISIBLE);
        } else {
            super.onBackPressed();
        }
    }
    protected Runnable mDeleteImageRunnable = new Runnable() {

        public void run() {
            if(mAdapter == null) return;
            String bucketId = mAdapter.getItemmBucketId(mSelectedItemIndex);
            ImageManager.ImageListParam mParam = null;
            if(ImageManager.isVideoBucketId(bucketId)) {
                if(Util.getInternalStorageDir() != null){
                    mParam = ImageManager.getImageListParam(
                            ImageManager.DataLocation.ALL,
                            ImageManager.INCLUDE_VIDEOS,
                            ImageManager.SORT_DESCENDING,
                            bucketId);
                }else{
                    mParam = ImageManager.getImageListParam(
                            ImageManager.DataLocation.EXTERNAL,
                            ImageManager.INCLUDE_VIDEOS,
                            ImageManager.SORT_DESCENDING,
                            bucketId);
                }
            } else {
                if(Util.getInternalStorageDir() != null){
                    mParam = ImageManager.getImageListParam(
                            ImageManager.DataLocation.ALL,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.SORT_DESCENDING,
                            bucketId);
                }else{
                    mParam = ImageManager.getImageListParam(
                            ImageManager.DataLocation.EXTERNAL,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.SORT_DESCENDING,
                            bucketId);
                }
            }
            IImageList mAllImages = ImageManager.makeImageList(getContentResolver(), mParam);
            if(mAllImages == null) return;
            /*Uri[] imageUris = new Uri[mAllImages.getCount()];
            int count = mAllImages.getCount();

            if(count > 0){
                mFileDirectoryOfDrag = mAllImages.getImageAt(0).getDataPath().substring(0, mAllImages.getImageAt(0).getDataPath().lastIndexOf("/"));
            }
            for(int i = count - 1; i >= 0; i--) {
                imageUris[i] = mAllImages.getImageAt(i).fullSizeImageUri();
            }
            new DeleteImageTask(count).execute(imageUris);*/
            int delCount = mAllImages.getCount();
            images = new IImage[delCount];
            /**
             * FIX BUG: 1917
             * BUG CAUSE: StringIndexOutOfBoundsException.
             * DATE:2012-11-16
             */
            if(delCount > 0){
                IImage image = mAllImages.getImageAt(0);
                if (image != null) {
                    String path = image.getDataPath();
                    if (path != null && path.lastIndexOf("/") > 0) {
                        mFileDirectoryOfDrag = path.substring(0, path.lastIndexOf("/"));
                    }
                }
            }
            for(int i = delCount - 1; i >= 0; i--) {
                images[i] = mAllImages.getImageAt(i);
            }
            /**
             * FIX BUG: 1931
             * BUG CAUSE: ProgressDialog changed in child thread will delay.
             * DATE:2012-11-14
             */
            deleteImage(delCount);
        }
    };

    private ProgressDialog mDelProgressDialog;
    private ContentResolver mContentResolver;
    private IImage[] images ;
    private int mIndex = 0;
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
//            new Thread(mDeleteNextRunnable).start();
        }
    }

    private final Runnable mDeleteNextRunnable = new Runnable() {
        public void run() {
            deleteNext();
        }
    };

    private void deleteNext() {
        if (mPausing) return;
        if (mIndex >= images.length) {
            runOnUiThread( new Runnable() {
                public void run() {
                    if (mDelProgressDialog != null) {
                        mDelProgressDialog.dismiss();
                        mDelProgressDialog = null;
                    }
                    mIndex = 0;
                    mAdapter.removeItem(mSelectedItemIndex);
                    if (mAdapter.getCount() == 0) {
                        showNoImagesView();
                    }
                }
            });
            deleteEmptyDirectory(mFileDirectoryOfDrag);
            return;
        }
        IImage image = images[mIndex++];
        if(image != null && mDelProgressDialog != null) {
            runOnUiThread( new Runnable() {
                public void run() {
                    mDelProgressDialog.incrementProgressBy(1);
                }
            });
            mContentResolver.delete(image.fullSizeImageUri(), null, null);
            mContentResolver.delete(UCamData.Thumbnails.CONTENT_URI, UCamData.Thumbnails.THUMB_PATH + "=?", new String[]{image.getDataPath()});
            mContentResolver.delete(UCamData.Albums.CONTENT_URI, UCamData.Albums.ALBUM_BUCKET + "=?", new String[]{image.getBucketId()});
            if(Build.isTelecomCloud()) {
                Util.startCloudService(image , this);
            }
        }
        mHandler.post(mDeleteNextRunnable);
//        deleteNext();
    }

    private void deleteEmptyDirectory(String path) {
        if(path != null){
            try {
                File file = new File(path);
                if (file.isDirectory()) {
                    String[] files = file.list();
                    if (files.length == 1 && new File(files[0]).isHidden()) {
                        new File(file, files[0]).delete();
                    }
                    file.delete();
                }
            } catch (Exception e) {
                e.printStackTrace();
            } finally {
                mFileDirectoryOfDrag = null;
            }
        }
    }

    protected Runnable mLockImageRunnable = new Runnable() {

        public void run() {
            Log.d(TAG, "mLockImageRunnable...");
            if(mAdapter == null) return;
            final String bucketId = mAdapter.getItemmBucketId(mSelectedItemIndex);
            ImageManager.ImageListParam mParam = null;
            if(ImageManager.isVideoBucketId(bucketId)) {
                if(Util.getInternalStorageDir() != null){
                    mParam = ImageManager.getImageListParam(
                            ImageManager.DataLocation.ALL,
                            ImageManager.INCLUDE_VIDEOS,
                            ImageManager.SORT_DESCENDING,
                            bucketId);
                }else{
                    mParam = ImageManager.getImageListParam(
                            ImageManager.DataLocation.EXTERNAL,
                            ImageManager.INCLUDE_VIDEOS,
                            ImageManager.SORT_DESCENDING,
                            bucketId);
                }
            } else {
                if(Util.getInternalStorageDir() != null){
                    mParam = ImageManager.getImageListParam(
                            ImageManager.DataLocation.ALL,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.SORT_DESCENDING,
                            bucketId);
                }else{
                    mParam = ImageManager.getImageListParam(
                            ImageManager.DataLocation.EXTERNAL,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.SORT_DESCENDING,
                            bucketId);
                }
            }
            final IImageList mAllImages = ImageManager.makeImageList(getContentResolver(), mParam);
            if(mAllImages == null) return;
            final int lockCount = mAllImages.getCount();
            images = new IImage[lockCount];
            Log.d(TAG, "lockCount = " +lockCount);
            if (lockCount > 0) {
                mContentResolver = getContentResolver();
                final IImage image = mAllImages.getImageAt(0);
                if (image != null) {
                    final String path = image.getDataPath();
                    /* SPRD: CID 111100 : Dereference before null check (REVERSE_INULL) @{ */
                    if(path == null){
                        return;
                    }
                    /* @} */
                    File file = new File(path);
                    final File dir = file.getParentFile();
                    Log.d(TAG, "dir = " + dir);
                    if (path != null && path.lastIndexOf("/") > 0) {
                        mFileDirectoryOfDrag = path.substring(0, path.lastIndexOf("/"));
                    }
//                    new MediaScanner(ImageGallery.this).scanFile(
//                            dir.toString(), image.getMimeType());

                    mLockFinishDialog = ProgressDialog.show(ImageGallery.this, null, getResources().getString(
                            R.string.text_waiting), true, false);
                    final ArrayList videoList = new ArrayList();
                new Thread(new Runnable() {
                    @Override
                        public void run() {
                            Log.d(TAG, "ALL_VIDEOS_BUCKET_ID.equals(bucketId)"+ Constants.ALL_VIDEOS_BUCKET_ID.equals(bucketId));
                            if (ImageManager.isVideoBucketId(bucketId)) {
                                int count = lockCount - 1;
                                for (int i = count; i >= 0; i--) {
                                   if(ImageManager.isVideo( mAllImages.getImageAt(i))) {
                                       /*
                                        *BUG FIX: 5438
                                        *BUG COMMENT: If the nomedia file exists, the file will not be scanned;
                                        *FIX DATE: 2013-05-27
                                        */
                                       File newFile = LockUtil.backupSingleFile(mAllImages.getImageAt(i).getDataPath(), Constants.STORE_DIR_LOCKED , mIsShowLockedImage);
                                       videoList.add(newFile.getAbsolutePath());
                                       mContentResolver.delete(mAllImages.getImageAt(i).fullSizeImageUri(),
                                               null, null);
                                       mContentResolver.delete( UCamData.Thumbnails.CONTENT_URI,
                                               UCamData.Thumbnails.THUMB_PATH + "=?",
                                               new String[] { mAllImages.getImageAt(i).getDataPath() });
                                       mContentResolver.delete(UCamData.Albums.CONTENT_URI,
                                               UCamData.Albums.ALBUM_BUCKET + "=?",
                                               new String[] { mAllImages.getImageAt(i).getBucketId() });
                                    }
                                }
                                if (mIsShowLockedImage) {
                                    new MediaScanner(ImageGallery.this, new ScannerFinishCallBack() {
                                                @Override
                                                public void scannerComplete() {
                                                    mHandler.sendEmptyMessage(LOCK_FINISH);
                                                }
                                            }).scanFile(videoList, null);
                                } else {
                                    mHandler.sendEmptyMessage(LOCK_FINISH);
                                }
                            } else if(ImageManager.SN_PHOTOGRAPHY_BUCKET_ID.equals(bucketId)) {
                                ArrayList snImageList = new ArrayList();
                                int count = lockCount - 1;
                                for (int i = count; i >= 0; i--) {
                                   if(ImageManager.isImage( mAllImages.getImageAt(i))) {
                                       File newFile = LockUtil.backupSingleFile(mAllImages.getImageAt(i).getDataPath(), Constants.STORE_DIR_LOCKED_PHOTOGRAPY, mIsShowLockedImage);

                                       snImageList.add(newFile.getAbsolutePath());
                                       mContentResolver.delete(mAllImages.getImageAt(i).fullSizeImageUri(),
                                               null, null);
                                       mContentResolver.delete( UCamData.Thumbnails.CONTENT_URI,
                                               UCamData.Thumbnails.THUMB_PATH + "=?",
                                               new String[] { mAllImages.getImageAt(i).getDataPath() });
                                       mContentResolver.delete(UCamData.Albums.CONTENT_URI,
                                               UCamData.Albums.ALBUM_BUCKET + "=?",
                                               new String[] { mAllImages.getImageAt(i).getBucketId() });
                                    }
                                }
                                if (mIsShowLockedImage) {
                                    new MediaScanner(ImageGallery.this, new ScannerFinishCallBack() {
                                                @Override
                                                public void scannerComplete() {
                                                    mHandler.sendEmptyMessage(LOCK_FINISH);
                                                }
                                            }).scanFile(snImageList, null);
                                } else {
                                    mHandler.sendEmptyMessage(LOCK_FINISH);
                                }

                            }else {
                                /*
                                 *BUG FIX: 5472
                                 *BUG COMMENT: the system files can not be locked;
                                 *FIX DATE: 2013-12-02
                                 */
                                if(!new File(mAllImages.getImageAt(0).getDataPath()).canWrite()) {
                                    mHandler.sendEmptyMessage(LOCK_FAILED);
                                } else {
                                    /*
                                     *BUG FIX: 5438
                                     *BUG COMMENT: If the nomedia file exists, the file will not be scanned;
                                     *FIX DATE: 2013-05-27
                                     */
                                    boolean successed = LockUtil.backupAlbum(dir.getAbsolutePath(), mIsShowLockedImage);
                                    if(successed) {
                                        for (int i = lockCount -1 ; i >= 0; i--) {
                                            if(ImageManager.isVideo( mAllImages.getImageAt(i))) {
                                                continue;
                                            }
                                            mContentResolver.delete( UCamData.Thumbnails.CONTENT_URI, UCamData.Thumbnails.THUMB_PATH + "=?",
                                                    new String[] { mAllImages.getImageAt(i).getDataPath() });
                                        }
                                        mContentResolver.delete( UCamData.Albums.CONTENT_URI, UCamData.Albums.ALBUM_BUCKET + "=?",
                                                new String[] { bucketId });
                                        int count =  mContentResolver.delete( Images.Media.EXTERNAL_CONTENT_URI, Media.BUCKET_ID + "= ?",
                                                new String[] { bucketId });
                                        if(count <= 0) {
                                            mContentResolver.delete(Images.Media.INTERNAL_CONTENT_URI, Media.BUCKET_ID + "= ?",
                                                    new String[] { bucketId });
                                        }
                                        if(mIsShowLockedImage) {
                                            final String newPath = Constants.STORE_DIR_LOCKED.concat(dir.getAbsolutePath());

                                            new MediaScanner(ImageGallery.this, new ScannerFinishCallBack() {
                                                @Override
                                                public void scannerComplete() {
                                                    mHandler.sendEmptyMessage(LOCK_FINISH);
                                                }
                                            }).scanFile(newPath, null);
                                        } else {
                                            mHandler.sendEmptyMessage(LOCK_FINISH);
                                        }
                                    } else {
                                        mHandler.sendEmptyMessage(LOCK_FAILED);
                                    }
                                }
                            }
                    }
                }).start();
            }
            }
        }
    };

    protected Runnable mUnLockImageRunnable = new Runnable() {
        public void run() {
            Log.d(TAG, "mUnLockImageRunnable...");
            if(mAdapter == null) return;
            final String bucketId = mAdapter.getItemmBucketId(mSelectedItemIndex);
            ImageManager.ImageListParam mParam = null;
            if(ImageManager.isVideoBucketId(bucketId)) {
                if(Util.getInternalStorageDir() != null){
                    mParam = ImageManager.getImageListParam(
                            ImageManager.DataLocation.ALL,
                            ImageManager.INCLUDE_VIDEOS,
                            ImageManager.SORT_DESCENDING,
                            bucketId);
                }else{
                    mParam = ImageManager.getImageListParam(
                            ImageManager.DataLocation.EXTERNAL,
                            ImageManager.INCLUDE_VIDEOS,
                            ImageManager.SORT_DESCENDING,
                            bucketId);
                }
            } else {
                if(Util.getInternalStorageDir() != null){
                    mParam = ImageManager.getImageListParam(
                            ImageManager.DataLocation.ALL,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.SORT_DESCENDING,
                            bucketId);
                }else{
                    mParam = ImageManager.getImageListParam(
                            ImageManager.DataLocation.EXTERNAL,
                            ImageManager.INCLUDE_IMAGES,
                            ImageManager.SORT_DESCENDING,
                            bucketId);
                }
            }
            final IImageList mAllImages = ImageManager.makeImageList(getContentResolver(), mParam);
            if(mAllImages == null) return;
            final int lockCount = mAllImages.getCount();
            images = new IImage[lockCount];
            if (lockCount > 0) {
                mContentResolver = getContentResolver();
                final IImage image = mAllImages.getImageAt(0);
                if (image != null) {
                    final String path = image.getDataPath();
                    /* SPRD: CID 111101 : Dereference before null check (REVERSE_INULL) @{ */
                    if(path == null){
                        return;
                    }
                    File file = new File(path);
                    final File dir = file.getParentFile();
                    if (path != null && path.lastIndexOf("/") > 0) {
                        mFileDirectoryOfDrag = path.substring(0, path.lastIndexOf("/"));
                    }
                final ArrayList<String> newVideoFilePath = new ArrayList<String>();
                mLockFinishDialog = ProgressDialog.show(ImageGallery.this, null, getResources().getString( R.string.text_waiting), true, false);
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        if(ImageManager.isVideoBucketId(bucketId)) {
                            for (int i = lockCount - 1; i >= 0; i--) {
                                if(ImageManager.isVideo( mAllImages.getImageAt(i))) {
                                    File newFile = LockUtil.restoreSingleFile(mAllImages.getImageAt(i).getDataPath(), Constants.STORE_DIR_LOCKED);
                                    if( !(newFile.getAbsolutePath().equals(mAllImages.getImageAt(i).getDataPath()))) {
                                        newVideoFilePath.add(newFile.getAbsolutePath());
                                        mContentResolver.delete( mAllImages.getImageAt(i).fullSizeImageUri(), null, null);
                                        mContentResolver.delete(UCamData.Thumbnails.CONTENT_URI,
                                                UCamData.Thumbnails.THUMB_PATH + "=?",
                                                new String[] { mAllImages.getImageAt(i)
                                                .getDataPath() });
                                        mContentResolver.delete(UCamData.Albums.CONTENT_URI,
                                                UCamData.Albums.ALBUM_BUCKET + "=?",
                                                new String[] { mAllImages.getImageAt(i)
                                                .getBucketId() });
                                    }
                                }
                            }
                            new MediaScanner(ImageGallery.this, new ScannerFinishCallBack() {
                                @Override
                                public void scannerComplete() {
                                    mHandler.obtainMessage(UNLOCK_FINISH, newVideoFilePath).sendToTarget();
                                }
                            }).scanFile(newVideoFilePath, null);
                            /*
                             * FIX BUG : 6074
                             * BUG COMMENT : when the path is SD_PATH + "/DCIM/UcamSecretBak",wo need restore it
                             * DATE : 2014-03-06
                             */
                        } else if(ImageManager.SN_PHOTOGRAPHY_BUCKET_ID.equals(bucketId) || path.startsWith(Constants.STORE_DIR_LOCKED_DCIM)) {
                            ArrayList snImageList = new ArrayList();
                            for (int i = lockCount - 1; i >= 0; i--) {
                                if(ImageManager.isImage( mAllImages.getImageAt(i))) {
                                    File newFile = LockUtil.restoreSingleFile(mAllImages.getImageAt(i).getDataPath(), Constants.STORE_DIR_LOCKED_PHOTOGRAPY);
                                    snImageList.add(newFile.getAbsolutePath());
                                    mContentResolver.delete( mAllImages.getImageAt(i).fullSizeImageUri(), null, null);
                                    mContentResolver.delete(UCamData.Thumbnails.CONTENT_URI,
                                            UCamData.Thumbnails.THUMB_PATH + "=?",
                                            new String[] { mAllImages.getImageAt(i)
                                            .getDataPath() });
                                    mContentResolver.delete(UCamData.Albums.CONTENT_URI,
                                            UCamData.Albums.ALBUM_BUCKET + "=?",
                                            new String[] { mAllImages.getImageAt(i)
                                            .getBucketId() });
                                }
                            }
                            new MediaScanner(ImageGallery.this, new ScannerFinishCallBack() {
                                @Override
                                public void scannerComplete() {
                                    mHandler.sendEmptyMessage(UNLOCK_FINISH);
                                }
                            }).scanFile(snImageList, null);

                        } else {
                            boolean succesed = LockUtil.restoreAlbum(dir.getAbsolutePath());
                            if(succesed) {
                                for (int i = lockCount -1 ; i >= 0; i--) {
                                    if(ImageManager.isVideo( mAllImages.getImageAt(i))) {
                                        continue;
                                    }
                                    mContentResolver.delete( UCamData.Thumbnails.CONTENT_URI, UCamData.Thumbnails.THUMB_PATH + "=?",
                                            new String[] { mAllImages.getImageAt(i).getDataPath() });
                                }
                                mContentResolver.delete(Images.Media.EXTERNAL_CONTENT_URI,
                                        Media.BUCKET_ID+ "= ?", new String[]{bucketId});
                                final String newPth = dir.getAbsolutePath().substring(Constants.STORE_DIR_LOCKED.length());
                                new MediaScanner(ImageGallery.this, new ScannerFinishCallBack() {
                                    @Override
                                    public void scannerComplete() {
                                        mHandler.obtainMessage(UNLOCK_FINISH, newPth).sendToTarget();
                                    }
                                }).scanFile(newPth, null);
                            } else {
                                mHandler.sendEmptyMessage(UNLOCK_FAILED);
                            }
                        }
                    }
                }).start();
            }
            }
        }
    };

    @SuppressWarnings("unchecked")
    private void initImageGalleryFromIntent(Intent data) {
        if ( data == null ) return;
        Serializable claz = data.getSerializableExtra(INTENT_EXTRA_IMAGE_GALLERY_CLASS);
        if (claz != null) {
            mImageGalleryImpl = (Class<? extends BaseImagePicker>)claz;
        } else {
            mImageGalleryImpl = BaseImagePicker.class;
        }

        mIsRequestForResult = data.getBooleanExtra(INTENT_EXTRA_WAIT_FOR_RESULT, false);
        mImageMaxLimit      = data.getIntExtra(INTENT_EXTRA_IMAGE_MAX_LIMIT, -1);
        mGalleryEntry       = data.getStringExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY);
        mUPhotoModule       = data.getIntExtra(ViewImage.EXTRA_UPHOTO_MODULE_KEY, -1);
        if(isPickIntent()) {
            mGalleryEntry = ViewImage.IMAGE_ENTRY_PICKER_VALUE;
            setViewState(findViewById(R.id.settings_more), false);
            setViewState(findViewById(R.id.photo_collection), false);
            setViewState(findViewById(R.id.gallery_take_photoes), false);
            setViewState(findViewById(R.id.gallery_cloud_sync), false);
            setViewState(findViewById(R.id.gallery_cloud_line), false);
            setViewState(findViewById(R.id.net_print), false);
        } else {
            setViewState(findViewById(R.id.photo_collection), Build.SHOW_DOCOMO_PHOTO_COLLECTION);
            if(Build.isTelecomCloud()) {
                setViewState(findViewById(R.id.gallery_take_photoes), true);
                setViewState(findViewById(R.id.gallery_cloud_sync), true);
                setViewState(findViewById(R.id.gallery_cloud_line), true);
            }
            if(Build.NEED_SECRET_ABLUM) {
                setViewState(findViewById(R.id.settings_more), true);
            } else {
                setViewState(findViewById(R.id.settings_more), false);
            }
        }
        Log.d(TAG, "initImageGalleryFromIntent(): mGalleryEntry is " + mGalleryEntry);
        if(ViewImage.IMAGE_ENTRY_UPHOTO_VALUE.equals(mGalleryEntry) ||
                ViewImage.IMAGE_ENTRY_PICKER_VALUE.equals(mGalleryEntry) ||
                ViewImage.IMAGE_ENTRY_UGIF_VALUE.equals(mGalleryEntry) ||
                ViewImage.IMAGE_ENTRY_PUZZLE_VALUE.equals(mGalleryEntry)){
            findViewById(R.id.nav_to_album).setVisibility(View.GONE);
            findViewById(R.id.nav_to_select_photos).setVisibility(View.VISIBLE);
            setViewState(findViewById(R.id.settings_more), false);
            if(!mPickIntentAll) mPickIntentSelectPhotos = true;
            if(Build.isTelecomCloud()) {
                setViewState(findViewById(R.id.gallery_take_photoes), false);
                setViewState(findViewById(R.id.gallery_cloud_sync), false);
                setViewState(findViewById(R.id.gallery_cloud_line), false);
            }
            setViewState(findViewById(R.id.net_print), false);
            setViewState(findViewById(R.id.photo_collection), false);
        } else {
            mPickIntentSelectPhotos = false;
        }
        if(mGalleryEntry == null) {
            mPickIntentSelectPhotos = false;
            mGalleryEntry = ViewImage.IMAGE_ENTRY_UGALLERY_VALUE;
        }
    }

    private void setViewState(View view, boolean visible) {
        view.setVisibility(visible ? View.VISIBLE : View.GONE);
        view.setOnClickListener(visible ? this : null);
    }

    Class<? extends BaseImagePicker> getImagePickerClass(){return mImageGalleryImpl;}
    boolean isWaitForResult() { return mIsRequestForResult; }
    int     getImageMaxLimit(){return mImageMaxLimit;}
    String  getGalleryEntry() {return mGalleryEntry;}
    int     getUPhotoModule() {return mUPhotoModule;}
    boolean isPickIntent() {
        String action = getIntent().getAction();
        String type = getIntent().getType();
        Log.d(TAG, "isPickIntent(): type is " + type + ", action is " + action);
        if(type != null && "*/*".equals(type)) {
            mPickIntentAll = true;
        } else {
            if(type != null && "video/*".equals(type)) {
                mPickIntentVideoType = true;
            } else {
                mPickIntentVideoType = false;
            }
        }
        return Intent.ACTION_GET_CONTENT.equals(action) || Intent.ACTION_PICK.equals(action);
    }

    public static void showImagePicker(Context context, Class<? extends BaseImagePicker> pickerclz, String galleryEntry) {
        if (pickerclz == null)
            throw new RuntimeException("No Image Picker class provided");

        Intent intent = new Intent()
            .setClass(context, ImageGallery.class)
            .putExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY, galleryEntry)
            .putExtra(ViewImage.EXTRA_IMAGE_STORAGE_KEY, mCurrentStoragePos)
            .putExtra(INTENT_EXTRA_WAIT_FOR_RESULT, true)
            .putExtra(INTENT_EXTRA_IMAGE_GALLERY_CLASS, pickerclz);
        if(ViewImage.IMAGE_ENTRY_UGALLERY_VALUE.equals(galleryEntry) || ViewImage.IMAGE_ENTRY_PICKER_VALUE.equals(galleryEntry)) {
            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        }
        context.startActivity(intent);
    }

    public static void showImagePicker(Context context, Class<? extends BaseImagePicker> pickerclz, String galleryEntry, String storage){
        mCurrentStoragePos = storage;
        showImagePicker(context,pickerclz,galleryEntry);
    }

    public static void showImagePicker(Activity act, Class<? extends BaseImagePicker> pickerclz, int requestCode, int max,
            String galleryEntry, int uphotoModule) {
        Intent intent = new Intent()
            .setClass(act, ImageGallery.class)
            .putExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY, galleryEntry)
            .putExtra(INTENT_EXTRA_WAIT_FOR_RESULT, true)
            .putExtra(INTENT_EXTRA_IMAGE_GALLERY_CLASS, pickerclz)
            .putExtra(INTENT_EXTRA_IMAGE_MAX_LIMIT, max)
            .putExtra(ViewImage.EXTRA_UPHOTO_MODULE_KEY, uphotoModule);
        act.startActivityForResult(intent, requestCode);
    }
    public static void showImagePicker(Activity act, Class<? extends BaseImagePicker> pickerclz, int requestCode, int max, String galleryEntry) {
        Intent intent = new Intent()
            .setClass(act, ImageGallery.class)
            .putExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY, galleryEntry)
            .putExtra(INTENT_EXTRA_WAIT_FOR_RESULT, true)
            .putExtra(INTENT_EXTRA_IMAGE_GALLERY_CLASS, pickerclz)
            .putExtra(INTENT_EXTRA_IMAGE_MAX_LIMIT, max);
        act.startActivityForResult(intent, requestCode);
    }
    static final int REQUEST_CODE_PICK = 0xABCDE;
    static final int REQUEST_CODE_THIRD_PICK = 0xABCDF;
    public static void showImagePicker(Activity act, int requestCode, int max, String galleryEntry) {
        Intent intent = new Intent()
            .setClass(act, ImageGallery.class)
            .putExtra(INTENT_EXTRA_IMAGE_MAX_LIMIT, max)
            .putExtra(INTENT_EXTRA_WAIT_FOR_RESULT, true)
            .putExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY, galleryEntry)
            .putExtra(INTENT_EXTRA_IMAGE_GALLERY_CLASS, DefaultImagePicker.class);
        act.startActivityForResult(intent, requestCode);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK) {
            switch (requestCode) {
                case REQUEST_CODE_PICK:
                    Intent result = new Intent();
                    Parcelable[] obj = data.getParcelableArrayExtra(INTENT_EXTRA_IMAGES);
                    if (obj != null) {
                        Uri[] images = new Uri[obj.length];
                        System.arraycopy(obj, 0, images, 0, obj.length);
                        result.putExtra(INTENT_EXTRA_IMAGES, images);
                    }
                    setResult(RESULT_OK, result);
                    finish();
                    break;
                case REQUEST_CODE_THIRD_PICK:
                    setResult(RESULT_OK, data);
                    finish();
                    break;
            }
        }
    }

    /**
     *
     * @param scanning scanning
     */
    public void updateScanningDialog(boolean scanning) {
        boolean prevScanning = (mMediaScanningDialog != null);
        if(mAdapter == null) {
            return;
        }
        Log.d(TAG, "prevScanning : " + prevScanning + "scanning :" + scanning + "mAdapter.mItems.size() :" +mAdapter.mItems.size());
        if (prevScanning == scanning && mAdapter.mItems.size() == 0) {
            return;
        }
        // Now we are certain the state is changed.
        if (prevScanning) {
            mMediaScanningDialog.cancel();
            mMediaScanningDialog = null;
        } else if (scanning && mAdapter.mItems.size() == 0) {
            mMediaScanningDialog = ProgressDialog.show(this, null, getResources().getString(
                    R.string.text_waiting), true, true);
        }
    }

    private View mNoImagesView;

    // Show/Hide the "no images" icon and text. Load resources on demand.
    private void showNoImagesView() {
        if (mNoImagesView == null) {
            ViewGroup root = (ViewGroup) findViewById(R.id.root);
            getLayoutInflater().inflate(R.layout.warn_no_content, root);
            mNoImagesView = findViewById(R.id.no_images);
        }
        mNoImagesView.setVisibility(View.VISIBLE);
    }

    private void hideNoImagesView() {
        if (mNoImagesView != null) {
            mNoImagesView.setVisibility(View.GONE);
        }
    }

    // The storage status is changed, restart the worker or show "no images".
    private void rebake(boolean unmounted, boolean scanning) {
        Log.d(TAG, "unmounted + "+unmounted +"..."+"scanning"+scanning);
        if (unmounted == mUnmounted && scanning == mScanning) {
            return;
        }
        abortWorker();
        mUnmounted = unmounted;
        mScanning = scanning;
//        updateScanningDialog(mScanning);
        if (mUnmounted) {
            showNoImagesView();
        } else {
            hideNoImagesView();
            startWorker();
        }
    }

    // This is called when we receive media-related broadcast.
    private void onReceiveMediaBroadcast(Intent intent) {
        String action = intent.getAction();
        Log.d(TAG, "onReceiveMediaBroadcast(): action is " + action);
        if (action.equals(Intent.ACTION_MEDIA_MOUNTED)) {
            // SD card available
            Log.d(TAG, "onReceiveMediaBroadcast(): delete the album table record;");
            getContentResolver().delete(UCamData.Albums.CONTENT_URI, null, null);
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

    private void launchFolderGallery(int position) {
        mAdapter.mItems.get(position).launch(this);
        if(mCoverList != null && mCoverList.size() > 0) {
            try {
                getContentResolver().applyBatch(UCamData.AUTHORITY, mCoverList);
            } catch (RemoteException e) {
                e.printStackTrace();
            } catch (OperationApplicationException e) {
                e.printStackTrace();
            }
            mCoverList.clear();
        }
    }

    @Override
    public void onStop() {
        super.onStop();
        Log.d(TAG, "onStop() create");
        abortWorker();
        unregisterReceiver(mReceiver);
        getContentResolver().unregisterContentObserver(mDbObserver);
        // free up some ram
        mAdapter = null;
        mGridView.setAdapter(null);
        if(mIsLongPressed) {
            mGridView.upItem();
            mHandler.post(new Runnable() {
                public void run() {
                    mButtonLayout.startAnimation(FadeOut);
                    mButtonLayout.setVisibility(View.GONE);
                }
            });
            mIsLongPressed = false;
        }
        unloadDrawable();
//        this.finish();
    }

    @Override
    public void onStart() {
        super.onStart();
        Log.d(TAG, "onStart() create");
        mAdapter = new GalleryAdapter(this); //is PhotoView
        mGridView.setAdapter(mAdapter);
        mIsLockedAblum = mSharePref.getBoolean(UGalleryConst.KEY_PASSWORD_SETTING, true);
        mIsShowLockedImage = mSharePref.getBoolean(UGalleryConst.KEY_IS_SHOW_LOCKED_IMAGE, false);
        // install an intent filter to receive SD card related events.
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Intent.ACTION_MEDIA_MOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_SCANNER_STARTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_SCANNER_FINISHED);
        intentFilter.addAction(Intent.ACTION_MEDIA_EJECT);
        intentFilter.addDataScheme("file");

        registerReceiver(mReceiver, intentFilter);
        if("internal".equals(mCurrentStoragePos)){
            getContentResolver().registerContentObserver(MediaStore.Images.Media.INTERNAL_CONTENT_URI,
                    true, mDbObserver);
        }else{
            getContentResolver().registerContentObserver(MediaStore.Images.Media.EXTERNAL_CONTENT_URI,
                    true, mDbObserver);
        }
        checkSDCard();
        // Assume the storage is mounted and not scanning.
        mUnmounted = false;
        mScanning = false;
        startWorker();
    }

    private void checkSDCard() {
        /*
         * FIX BUG:5791
         * BUG COMMENT: if no SDCard so we do nothing
         * DATE: 2014-01-10
         */
//        boolean sdCardExist = Environment.getExternalStorageState()
//                              .equals(android.os.Environment.MEDIA_MOUNTED);
//        if(sdCardExist == false && Util.getInternalStorageDir() == null){
//             Toast.makeText(ImageGallery.this, R.string.text_no_sdcard, Toast.LENGTH_LONG).show();
//        }
    }

    // This is used to stop the worker thread.
    volatile boolean mAbort = false;

    // Create the worker thread.
    private void startWorker() {
        mAbort = false;
        mWorkerThread = new Thread("ImageGifAlbumActivity Worker") {
            @Override
            public void run() {
                workerRun();
            }
        };
        BitmapManager.instance().allowThreadDecoding(mWorkerThread);
        mWorkerThread.start();
    }

    private void abortWorker() {
        if (mWorkerThread != null) {
            BitmapManager.instance().cancelThreadDecoding(mWorkerThread, getContentResolver());
            MediaStore.Images.Thumbnails.cancelThumbnailRequest(getContentResolver(), -1);
            mAbort = true;
            try {
                mWorkerThread.join();
            } catch (InterruptedException ex) {
                Log.e(TAG, "join interrupted");
            }
            mWorkerThread = null;
            mHandler.removeMessages(0);
            mAdapter.clear();
            clearImageLists();
        }
    }

    private boolean lockPrivateAblum() {
        mIsShowLockedImage = false;
//        LockUtil.createNoMediaFile(Constants.STORE_DIR_LOCKED);

//        if(StorageUtils.isContains()) {
//            LockUtil.createNoMediaFile(Constants.STORE_DIR_LOCKED_PHOTOGRAPY);
//            mIsShowLockedImage = false;
//        }
        return hasLockedFile(Constants.STORE_DIR_LOCKED) || hasLockedFile(Constants.STORE_DIR_LOCKED_PHOTOGRAPY);
    }

    private boolean hasLockedFile(String filePath) {
        File file = new File(filePath);
        File[] files = file.listFiles();
        boolean hasLockedFile = false ;
        for (int i = 0; files != null && i < files.length; i++) {
            if (files[i].isFile()) {
                hasLockedFile =  true;
            } else {
                hasLockedFile = hasLockedFile(files[i].getAbsolutePath());
            }
            if(hasLockedFile) {
                break;
            }
        }
        return hasLockedFile;
    }
    private void getBeSacnnerPath(File file) {
        File[] files = file.listFiles();
        for (int i = 0; files != null && i < files.length; i++) {
            if(files[i].isFile()) {
                mToBeScannerPaths.add(files[i].getAbsolutePath());
            } else {
                getBeSacnnerPath(files[i]);
            }
        }
    }
    private void showPrivateAblum() {
        File file = new File(Constants.STORE_DIR_LOCKED);
        // CID 109132 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        // final String lockedPath = file.getAbsolutePath();
        File fileNoMedia = new File(Constants.STORE_DIR_LOCKED + "/" + ".nomedia");
        // CID 109132 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        // File filePhotoGraph = new File(Constants.STORE_DIR_LOCKED_PHOTOGRAPY);
        // CID 109132 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        // final String filePhotoGraphPath = filePhotoGraph.getAbsolutePath();
        File filePhotoGraphNoMedia = new File(Constants.STORE_DIR_LOCKED_PHOTOGRAPY + "/" + ".nomedia");
        mIsShowLockedImage = true;
        mToBeScannerPaths = new ArrayList<String>();
        if (fileNoMedia.exists() || filePhotoGraphNoMedia.exists()) {
            if(fileNoMedia.exists()) {
                fileNoMedia.delete();
                getBeSacnnerPath(new File(Constants.STORE_DIR_LOCKED));
            }
            if(filePhotoGraphNoMedia.exists()) {
                filePhotoGraphNoMedia.delete();
                getBeSacnnerPath(new File(Constants.STORE_DIR_LOCKED_PHOTOGRAPY));
            }
            mLockFinishDialog = ProgressDialog.show(ImageGallery.this, null, getResources().getString(
                    R.string.text_waiting), true, false);
            new MediaScanner(ImageGallery.this, new ScannerFinishCallBack() {
                @Override
                public void scannerComplete() {
                    mHandler.sendEmptyMessage(SHOW_LOCKED_IMAGES);
                }
            }).scanFile(mToBeScannerPaths, null);
        }
    }
    // This is run in the worker thread.
    private void workerRun() {

        // We collect items from checkImageList() and checkBucketIds() and
        // put them in allItems. Later we give allItems to checkThumbBitmap()
        // and generated thumbnail bitmaps for each item. We do this instead of
        // generating thumbnail bitmaps in checkImageList() and checkBucketIds()
        // because we want to show all the folders first, then update them with
        // the thumb bitmaps. (Generating thumbnail bitmaps takes some time.)
        ArrayList<Item> allItems = new ArrayList<Item>();

        if (mAbort) {
            return;
        }

        if(mPickIntentAll) {
            checkVideoList(allItems);
            if (mAbort) return;
            checkBucketIds(allItems);
            if (mAbort) {
                return;
            }
        } else {
            if(!mScanning) {
                if(!mPickIntentSelectPhotos || mPickIntentVideoType) {
                    checkVideoList(allItems);
                    if (mAbort) return;
                }
            }
            if(!mPickIntentVideoType) {
                checkBucketIds(allItems);
                if (mAbort) {
                    return;
                }
            }
        }

        Log.d(TAG, "allItems1 + "+allItems.size());
        checkScanning();

        if (mAbort) {
            return;
        }

        checkThumbBitmap(allItems);
        if (mAbort) {
            return;
        }
        /*
         * BUG FIX: 1973 1919
         * BUG COMMENT:
         *   1973-Muli-thread access problem
         *   1919-catch disk io exception
         * DATE: 2012-11-28
         */
        ArrayList<ContentProviderOperation> coverList = mCoverList;
        if(coverList != null && coverList.size() > 0) {
            try {
                getContentResolver().applyBatch(UCamData.AUTHORITY, coverList);
            } catch (Throwable e) {
                Log.e(TAG,"insert failed", e);
            }
            coverList.clear();
        }
        checkLowStorage();
    }

    // This is run in the worker thread.
    private void checkScanning() {
        ContentResolver cr = getContentResolver();
        final boolean scanning = ImageManager.isMediaScannerScanning(cr);
        mHandler.post(new Runnable() {
            public void run() {
                checkScanningFinished(scanning);
            }
        });
    }

    // This is run in the main thread.
    private void checkScanningFinished(boolean scanning) {
        updateScanningDialog(scanning);
    }

    // This is run in the main thread.
    private void updateItem(Item item) {
        // Hide NoImageView if we are going to add the first item
        if(mAbort)
            return;
        if(mAdapter == null)
            return;
        if (mAdapter.getCount() == 0) {
            hideNoImagesView();
        }
        mAdapter.addItem(item);
    }
    // This is run in the main thread.
    private void updateVideoItem(Item item) {
        // Hide NoImageView if we are going to add the first item
        if(mAdapter == null)
            return;
        if (mAdapter.getCount() == 0) {
            hideNoImagesView();
        }
        mAdapter.addItem(item);
    }
    private void checkVideoList(ArrayList<Item> allItems) {
        int length = mImageListData.length;
        IImageList videoList;
        for (int i = 0; i < length; i++) {
            ImageListData data = mImageListData[i];
            if(data.mType != Item.TYPE_ALL_VIDEOS) {
                continue;
            }
            videoList = createImageList(data.mInclude, data.mBucketId,
                    getContentResolver());
            if (mAbort) return;
            if (videoList.isEmpty()) continue;
            final Item item = new Item(data.mType,
                            data.mBucketId,
                            data.mBucketName,
                            videoList);
            /*if(needLockedAbulm(item)) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        ArrayList<String> toBeSacnnerPaths = new ArrayList<String>();
                        for (int i = item.mCount; item.mCount > 0 && i >= 0; i--) {
                            IImage image = item.mImageList.getImageAt(i);
                            if(image != null && !image.getDataPath().startsWith(Constants.STORE_DIR_LOCKED)) {
                                File newFile = LockUtil.backupUnlockFile(i ,image.getDataPath(), Constants.STORE_DIR_LOCKED, false);
                                toBeSacnnerPaths.add(newFile.getAbsolutePath());
                                getContentResolver().delete(image.fullSizeImageUri(), null, null);
                            }
                        }
                        if (mIsShowLockedImage && toBeSacnnerPaths.size()>0) {
                            new MediaScanner(ImageGallery.this, null).scanFile(toBeSacnnerPaths, null);
                        }
                    }
                }).start();
            }*/
            if(mIsLockedAblum && needLockedAbulm(item)) {
                item.reNameVideoDir();
                for (int j = 0; j < item.mImageList.getCount(); j++) {
                    IImage image = item.mImageList.getImageAt(j);
                    getContentResolver().delete(image.fullSizeImageUri(), null, null);
                    getContentResolver().delete(UCamData.Thumbnails.CONTENT_URI, UCamData.Thumbnails.THUMB_PATH + "=?", new String[]{image.getDataPath()});
                    getContentResolver().delete(UCamData.Albums.CONTENT_URI, UCamData.Albums.ALBUM_BUCKET + "=?", new String[]{image.getBucketId()});
                }
                item.restoreVideoDir();
                /*
                 * FIX BUG:5455
                 * BUG COMMENT: we need create a nomedia file when lock the file;
                 * DATE: 2013-11-28
                 */
                LockUtil.createNoMediaFile(Constants.STORE_DIR_LOCKED);
            } else {
                allItems.add(item);
                final Item finalItem = item;
                mHandler.post(new Runnable() {
                    public void run() {
                        updateVideoItem(finalItem);
                    }
                });
            }
        }
    }

    private boolean isSystemGallery(String path) {
        if(path != null && path.startsWith("/system")) {
            return true;
        }
        return false;
    }
    // This is run in the worker thread.
    private void checkBucketIds(ArrayList<Item> allItems) {
        final IImageList allImages;
        if (!mScanning && !mUnmounted) {
//            if(Util.getInternalStorageDir() != null){
                allImages = ImageManager.makeImageList(getContentResolver(),
                        ImageManager.DataLocation.ALL,
                        ImageManager.INCLUDE_IMAGES,
                        ImageManager.SORT_DESCENDING, null);
//            }else{
//                allImages = ImageManager.makeImageList(getContentResolver(),
//                        ImageManager.DataLocation.EXTERNAL,
//                        ImageManager.INCLUDE_IMAGES,
//                        ImageManager.SORT_DESCENDING, null);
//            }
        } else {
            allImages = ImageManager.makeEmptyImageList();
        }
        /*
         * FIX BUG:5195
         * BUG COMMENT: Avoid null pointer exception
         * DATE: 2013-11-12
         */
        if(allImages.getCount() > 0 && mHandler != null) {
            mHandler.sendEmptyMessage(SHOW_SECRET_HINT);
        }
        if (mAbort) {
            allImages.close();
            return;
        }
        Log.d(TAG, "allImages = " +allImages.getCount());
        HashMap<String, String> hashMap = allImages.getBucketIds();
        // CID 109130 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        // HashMap<String, String> hashMapUri = allImages.getBaseUri();
        allImages.close();
        if (mAbort) {
            return;
        }

        /**
         * FIX BUG: 1090 1132
         * BUG CAUSE: SD card mounted or is scanning, get EmptyImageList object.
         * FIX COMMENT: Did not get the BucketIds, would directly return.
         * DATE:2012-06-26
         */
        if(hashMap == null) {
            return;
        }

        //SPRD Bug:542624 camera2 happens JavaCrash,log:java.lang.ArrayIndexOutOfBoundsException
        ArrayList<ContentProviderOperation> operations = new ArrayList<ContentProviderOperation>();

        LinkedList<String> bucketIds = new LinkedList<String>();
        bucketIds.addAll(hashMap.keySet());

        for (ImageListData data : mImageListData) {
            if(data == null) continue;
            if (bucketIds.remove(data.mBucketId)) {
                bucketIds.addFirst(data.mBucketId);
                hashMap.put(data.mBucketId, data.mBucketName);
            }
        }

        for (String key : bucketIds) {
            if (key == null) {
                continue;
            }
            IImageList list = createImageList(ImageManager.INCLUDE_IMAGES, key, getContentResolver());
            if (mAbort) {
                return;
            }

            final Item item = new Item(Item.TYPE_NORMAL_FOLDERS, key, hashMap.get(key), list);
//                item.mThumbBitmap = makeMiniThumbBitmap(THUMB_SIZE, THUMB_SIZE, item.mImageList);
//                item.loadThumbnailFromFile();
            Cursor cursor = null;
            try {
                String name = null;
                IImage image = item.mImageList.getImageAt(0);
                if(image != null) {
                    name = image.getTitle();
                }
                if(image != null && isSystemGallery(image.getDataPath())){
                    continue;
                }
//                if(hashMapUri != null) {
//                    String baseUri = hashMapUri.get(key);
//                    String photoPath = image.getDataPath();
//                    boolean isRight = isRigntUri(baseUri, photoPath);
//                    if(!isRight) {
//                        continue;
//                    }
//                }
                cursor = getContentResolver().query(UCamData.Albums.CONTENT_URI,
                        new String[] {UCamData.Albums.ALBUM, UCamData.Albums.IMAGE_NAME},
                        UCamData.Albums.ALBUM_BUCKET + "=?",
                        new String[]{item.mBucketId},
                        null);
                Bitmap temp = null;
                String realName = null;
                if(cursor != null) {
                    while (cursor.moveToNext()) {
                        byte[] data = cursor.getBlob(cursor.getColumnIndex(UCamData.Albums.ALBUM));
                        if(data != null) {
                            temp = BitmapFactory.decodeByteArray(data, 0, data.length);
                        }
                        realName = cursor.getString(cursor.getColumnIndex(UCamData.Albums.IMAGE_NAME)) ;
                    }
                }

                if(temp == null || (realName != null && !realName.equals(name))) {
                    temp = makeMiniThumbBitmap(THUMB_SIZE, THUMB_SIZE, item.mImageList);
                    byte[] coverData = transform(temp);
                    if(coverData != null) {
                        operations.add(ContentProviderOperation.newInsert(UCamData.Albums.CONTENT_URI)
                                .withValue(UCamData.Albums.ALBUM_BUCKET, item.mBucketId)
                                .withValue(UCamData.Albums.IMAGE_NAME, name)
                                .withValue(UCamData.Albums.ALBUM, coverData)
                                .build());
                    }
                }

                if(ImageManager.SN_PHOTOGRAPHY_BUCKET_ID.equals(key) && needLockedAbulm(item)) {
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            ArrayList<String> toBeSacnnerPaths = new ArrayList<String>();
                            for (int i = item.mCount; item.mCount > 0 && i >= 0; i--) {
                                IImage image = item.mImageList.getImageAt(i);
                                if(image != null && !image.getDataPath().startsWith(Constants.STORE_DIR_LOCKED_PHOTOGRAPY)) {
                                    File newFile = LockUtil.backupUnlockFile(i ,image.getDataPath(), Constants.STORE_DIR_LOCKED_PHOTOGRAPY, true);
                                    toBeSacnnerPaths.add(newFile.getAbsolutePath());
                                    getContentResolver().delete(image.fullSizeImageUri(), null, null);
                                }
                            }
                            if (mIsShowLockedImage && toBeSacnnerPaths.size()>0) {
                                new MediaScanner(ImageGallery.this, null).scanFile(toBeSacnnerPaths, null);
                            }
                        }
                    }).start();
                }

                item.mThumbBitmap = temp;
                if(mIsLockedAblum && needLockedAbulm(item)) {
                    /*
                     * FIX BUG : 5702
                     * BUG COMMENT : when the path is SD_PATH + "/DCIM/UcamSecretBak",wo need createNoMediaFile in this path
                     * DATE : 2014-02-13
                     */
                    if(ImageManager.SN_PHOTOGRAPHY_BUCKET_ID.equals(key) || image.getDataPath().startsWith(Constants.STORE_DIR_LOCKED_DCIM)) {
                        int count = item.mImageList.getCount();
                        new Thread(new Runnable() {
                            @Override
                            public void run() {
                                item.reNameVideoDir();
                                for (int i = 0; i < item.mImageList.getCount(); i++) {
                                    IImage image = item.mImageList.getImageAt(i);
                                    if(image != null) {
                                        getContentResolver().delete(image.fullSizeImageUri(), null, null);
                                        getContentResolver().delete(UCamData.Thumbnails.CONTENT_URI, UCamData.Thumbnails.THUMB_PATH + "=?", new String[]{image.getDataPath()});
                                        getContentResolver().delete(UCamData.Albums.CONTENT_URI, UCamData.Albums.ALBUM_BUCKET + "=?", new String[]{image.getBucketId()});
                                    }
                                }
                                item.restoreVideoDir();
                                LockUtil.createNoMediaFile(Constants.STORE_DIR_LOCKED_PHOTOGRAPY);
                            }
                        }).start();
                    } else {
                        new Thread(new Runnable() {
                            @Override
                            public void run() {
                                item.reNameDir();
                                getContentResolver().delete(Images.Media.EXTERNAL_CONTENT_URI,
                                        Media.BUCKET_ID+ "= ?", new String[]{item.mBucketId});
                                item.restoreDir();
                                LockUtil.createNoMediaFile(Constants.STORE_DIR_LOCKED);
                            }
                        }).start();
                    }
                }else {
                    allItems.add(item);
                    final Item finalItem = item;
                    mHandler.post(new Runnable() {
                        public void run() {
                            updateItem(finalItem);
                        }
                    });
                }
            }catch(SQLiteException e) {
                Log.e(TAG, "checkBucketIds SQLiteException: "+e);
            } finally {
                if(cursor != null) {
                    cursor.close();
                }
            }
        }
        //SPRD Bug:542624 java.lang.ArrayIndexOutOfBoundsException
        mCoverList = operations;
        mHandler.post(new Runnable() {
            public void run() {
                checkBucketIdsFinished();
            }
        });
    }
    private boolean isRigntUri(String baseUri, String photoPath) {
        if(baseUri != null && photoPath != null) {
            if(baseUri != null && baseUri.contains("external")) {
                if(!photoPath.startsWith(Util.getExternalStorageDir())) {
                    return false;
                }
            }
            if(baseUri != null && baseUri.contains("internal")) {
                /* SPRD: CID 108991 : Dereference null return value (NULL_RETURNS) @{ */
                String internalstorageDir = Util.getInternalStorageDir();
                if(internalstorageDir == null){
                    return false;
                } else {
                    if(!photoPath.startsWith(internalstorageDir)) {
                        return false;
                    }
                }
                /**
                if(!photoPath.startsWith(Util.getInternalStorageDir())) {
                    return false;
                }
                */
                /* @} */
            }
        }
        return true;
    }
    public boolean needLockedAbulm(Item item){
        return needImageLockedAbulm(item) || needVideoLockedAbulm(item);
    }
    public boolean needImageLockedAbulm(Item item) {
        IImageList list  = item.mImageList;
/*        IImage image =  item.mImageList.getImageAt(0);
        String filePath = null;
        if(image != null) {
            filePath = image.getDataPath();
        }
        if (filePath != null) {
            File file = new File(filePath);
            String dir = file.getParentFile().getPath();
            if (dir.startsWith(Constants.STORE_DIR_LOCKED) || dir.startsWith(Constants.STORE_DIR_LOCKED_PHOTOGRAPY)) {
                return true;
            }
        }*/
        for (int i = 0; i < list.getCount(); i++) {
            if(ImageManager.isVideo(list.getImageAt(i)))
                continue;
            String path = list.getImageAt(i).getDataPath();
            if (path != null && (path.startsWith(Constants.STORE_DIR_LOCKED) || path.startsWith(Constants.STORE_DIR_LOCKED_PHOTOGRAPY))) {
                return true;
            }
        }
        return false;
    }
    public boolean needVideoLockedAbulm(Item item){
        IImageList list  = item.mImageList;
        if(ImageManager.isVideoBucketId(item.mBucketId)) {
            for (int i = 0; i < list.getCount(); i++) {
                String path = list.getImageAt(i).getDataPath();
                if (path != null && path.startsWith(Constants.STORE_DIR_LOCKED)) {
                    return true;
                }
            }
        }
        return false;
    }

/*    // This is run in the worker thread.
    private void scanFileItem(String path) {
        Log.d(TAG, "scanFileItem .....path" +path );
        int index = path.lastIndexOf("/")+1;

        String newPath = null;
        if (index > 0) {
            newPath = path.substring(index);
        }
            IImageList list = createImageList(ImageManager.INCLUDE_IMAGES, ImageManager.getBucketId(path), getContentResolver());
            Log.d(TAG, "scan list= "+list);
            Item item = new Item(Item.TYPE_NORMAL_FOLDERS, ImageManager.getBucketId(path), newPath, list);
            Cursor cursor = null;
            try {
                cursor = getContentResolver().query(UCamData.Albums.CONTENT_URI,
                        new String[] {UCamData.Albums.ALBUM},
                        UCamData.Albums.ALBUM_BUCKET + "=?",
                        new String[]{item.mBucketId},
                        null);
                Bitmap temp = null;
                Log.d(TAG, "scan cursor= "+cursor);
                if(cursor != null) {
                    boolean moveToFirst = cursor.moveToFirst();
                    if(moveToFirst) {
                        byte[] data = cursor.getBlob(0);
                        if(data != null) {
                            temp = BitmapFactory.decodeByteArray(data, 0, data.length);
                            Log.d(TAG, "scan temp= "+temp);
                        }
                    }
                }
                if(temp == null) {
                    temp = makeMiniThumbBitmap(THUMB_SIZE, THUMB_SIZE, item.mImageList);
                }

                item.mThumbBitmap = temp;

                final Item finalItem = item;
                Log.d(TAG, "scan finalItem= "+finalItem);
                mHandler.post(new Runnable() {
                    public void run() {
                        updateItem(finalItem);
                    }
                });
            } catch (Exception e) {
                Log.d(TAG, "e :" +e);
            } finally {
                if(cursor != null) {
                    cursor.close();
                }
            }
    }*/
    // This is run in the main thread.
    private void checkBucketIdsFinished() {

        // If we just have one folder, open it.
        // If we have zero folder, show the "no images" icon.
        if (!mScanning && mAdapter!= null) {

            /*
             * FIX BUG : 4622
             * BUG COMMENT : avoid the null point exception
             * DATE : 2013-08-02
             */
            if (mAdapter.mItems != null && mAdapter.mItems.size() == 0) {
                showNoImagesView();
            }
        }
    }
    /*
     * FIX BUG : 5243
     * BUG COMMENT : Make the thumbnail picture quality more clearly;
     * DATE : 2013-10-31
     */
    private static final int THUMB_SIZE = 240;
    // This is run in the worker thread.
    private void checkThumbBitmap(ArrayList<Item> allItems) {
        for (Item item : allItems) {
            if(item.mType != Item.TYPE_ALL_VIDEOS) {
                continue;
            }
            final Bitmap b = makeMiniThumbBitmap(THUMB_SIZE, THUMB_SIZE,
                    item.mImageList);
            if (mAbort) {
                if (b != null) b.recycle();
                return;
            }

            final Item finalItem = item;
            mHandler.post(new Runnable() {
                        public void run() {
                            updateThumbBitmap(finalItem, b);
                        }
                    });
        }
       /* mCoverList = new ArrayList<ContentProviderOperation>();
        Cursor cursor = null;
        try {
            for (Item item : allItems) {
                cursor = getContentResolver().query(UCamData.Albums.CONTENT_URI,
                        new String[] {UCamData.Albums.ALBUM},
                        UCamData.Albums.ALBUM_BUCKET + "=?",
                        new String[]{item.mBucketId},
                        null);
                Bitmap temp = null;
//                Bitmap b = null;
                if(cursor != null) {
                    boolean moveToFirst = cursor.moveToFirst();
                    if(moveToFirst) {
                        byte[] data = cursor.getBlob(0);
                        if(data != null) {
                            temp = BitmapFactory.decodeByteArray(data, 0, data.length);
                        }
                    }
                }
                if(temp == null) {
                    temp = makeMiniThumbBitmap(THUMB_SIZE, THUMB_SIZE, item.mImageList);
                    byte[] coverData = transform(temp);
                    if(coverData != null) {
                        mCoverList.add(ContentProviderOperation.newInsert(UCamData.Albums.CONTENT_URI)
                                .withValue(UCamData.Albums.ALBUM_BUCKET, item.mBucketId)
                                .withValue(UCamData.Albums.ALBUM, coverData)
                                .build());
                    }
                }
                if (mAbort) {
                    if (temp != null) {
                        temp.recycle();
                    }
                    return;
                }
//                item.saveThumbnailToFile(b);
                final Bitmap b = temp;
                final Item finalItem = item;
                mHandler.post(new Runnable() {
                    public void run() {
                        updateThumbBitmap(finalItem, b);
                    }
                });
            }
        } finally {
            if(cursor != null) {
                cursor.close();
            }
        }*/
    }

    // This is run in the main thread.
    private void updateThumbBitmap(Item item, Bitmap b) {
        item.setThumbBitmap(b);
        if(mAdapter != null)
            mAdapter.updateDisplay();
    }

    private static final long LOW_STORAGE_THRESHOLD = 1024 * 1024 * 2;

    // This is run in the worker thread.
    private void checkLowStorage() {
        // Check available space only if we are writable
        if (ImageManager.hasStorage()) {
            String storageDirectory = Environment.getExternalStorageDirectory().toString();
            StatFs stat = new StatFs(storageDirectory);
            long remaining = (long) stat.getAvailableBlocks() * (long) stat.getBlockSize();
            if (remaining < LOW_STORAGE_THRESHOLD) {
                mHandler.post(new Runnable() {
                    public void run() {
                        checkLowStorageFinished();
                    }
                });
            }
        }
    }

    // This is run in the main thread.
    // This is called only if the storage is low.
    private void checkLowStorageFinished() {
        Toast.makeText(ImageGallery.this, R.string.text_sdcar_no_space, 5000).show();
    }

    /**
     * IMAGE_LIST_DATA stores the parameters for the four image lists
     * we are interested in. The order of the IMAGE_LIST_DATA array is
     * significant (See the implementation of GalleryAdapter.init).
     *
     */
    private static final class ImageListData {
        ImageListData(int type, int include, String bucketId, String bucketName) {
            mType = type;
            mInclude = include;
            mBucketId = bucketId;
            mBucketName = bucketName;
        }

        int mType;
        int mInclude;
        String mBucketId;
        String mBucketName;

        public String toString() {
            return " mType : " + mType + " mInclude : " + mInclude
                    + " mBucketId : " + mBucketId + " mBucketName : "
                    + mBucketName;
        }

    }

//    private void loadDrawableIfNeeded() {
//        if (mCellOutline != null) {
//            return; // already loaded
//        }
//        mCellOutline = getResources().getDrawable(R.drawable.photos_bg_88);
//    }
//
    private void unloadDrawable() {
        mCellOutline = null;
    }

    private static void placeImage(Bitmap image, Canvas c, Paint paint, int imageWidth,
            int widthPadding, int imageHeight, int heightPadding, int offsetX, int offsetY,
            int pos) {
        int xPos;
        int yPos ;
        if(UiUtils.screenDensity() == 1) {
            xPos = offsetX + 3;
            yPos = offsetY+ 4 ;
        } else {
            xPos = offsetX + 1;
            yPos = offsetY + 2;
        }
//        if (pos == 0) {
//            xPos += 4;
//            yPos += 6;
//        }
        c.drawBitmap(image, xPos, yPos, paint);
    }

    // This is run in worker thread.
    private Bitmap makeMiniThumbBitmap(int width, int height, IImageList images) {
        int count = images.getCount();
        final int padding = 4;
        int imageWidth = width;
        int imageHeight = height;
        int offsetWidth = 0;
        int offsetHeight = 0;
        final Bitmap b = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        final Canvas c = new Canvas(b);
        final Matrix m = new Matrix();
        unloadDrawable();
//        loadDrawableIfNeeded();
        for (int i = 0; i >= 0; i--) {
            if (mAbort) {
                return null;
            }

            Bitmap temp = null;
            IImage image = i < count ? images.getImageAt(i) : null;
            try {
                if (image != null) {
                    temp = (Bitmap) image.miniThumbBitmap()[1];
                }
            } catch (Exception e) {
                android.util.Log.e(TAG, "makeMiniThumbBitmap(): decode miniThumbBitmap fail", e);
            }
            if (temp != null) {
                temp = Util.transform(m, temp, imageWidth, imageHeight, true, Util.RECYCLE_INPUT);
            }else if(i == 0){
                try {
                    temp = BitmapFactory.decodeResource(getResources(), R.drawable.missing_thumbnail_picture, Util.getNativeAllocOptions());
                } catch (OutOfMemoryError ex) {
                    android.util.Log.w(TAG, "makeMiniThumbBitmap(): decode resource oom", ex);
                    continue;
                }
                if(temp == null) {
                    continue;
                }
                Bitmap bitmap = Util.transform(m, temp.copy(Config.RGB_565, true), imageWidth, imageHeight, true, Util.RECYCLE_INPUT);
                Util.recyleBitmap(temp);
                temp = bitmap;
            }

            Bitmap thumb = Bitmap.createBitmap(imageWidth, imageHeight, Bitmap.Config.ARGB_8888);
            Canvas tempCanvas = new Canvas(thumb);

            Matrix mm = new Matrix();
            Paint pp = new Paint();
            pp.setFlags(Paint.ANTI_ALIAS_FLAG);

            if (temp != null) {
                tempCanvas.drawBitmap(temp, mm, pp);
            }
//            mCellOutline.setBounds(0, 0, imageWidth, imageHeight);
//            mCellOutline.draw(tempCanvas);

            Matrix matrix = new Matrix();
            Paint pPaint = new Paint();
            try {
                Bitmap resizeBitmap = Bitmap.createBitmap(thumb, 0, 0, imageWidth, imageHeight, matrix,true);
                placeImage(resizeBitmap, c, pPaint, imageWidth, padding, imageHeight, padding, offsetWidth, offsetHeight, i);
            } catch(NullPointerException ex) {
                android.util.Log.w("NULL", "Why is null raised? thumb is null? " + (thumb == null), ex);
            } finally{
                Util.recyleBitmap(thumb);
                Util.recyleBitmap(temp);
            }
        }
        return b;
    }


    // image lists created by createImageList() are collected in mAllLists.
    // They will be closed in clearImageList, so they don't hold open files
    // on SD card. We will be killed if we don't close files when the SD card
    // is unmounted.
    ArrayList<IImageList> mAllLists = new ArrayList<IImageList>();

    private SharedPreferences mSharepre;

    private boolean isCloudSyncing = false;

    private boolean mPickIntentSelectPhotos;

    private boolean mPickIntentVideoType;

    private boolean mPickIntentAll; //photo and video

    private LinearLayout mLockLayout;

//    private ImageView mLockImageView;

    private boolean mScanLock;

    private SharedPreferences mSharePref;

    private boolean mIsLockedAblum;

    private ViewStub mSecretStub;

    private View mStubViewHint;

    private TextView mLockTextView;

    private TextView mDeleteTextView;

    private ArrayList<String> mToBeScannerPaths;

    private ProgressDialog mImageRestoreDialog;

    private IImageList createImageList(int mediaTypes, String bucketId, ContentResolver cr) {
        IImageList list = ImageManager.makeImageList(cr, ImageManager.DataLocation.ALL, mediaTypes,
                ImageManager.SORT_DESCENDING, bucketId);
        mAllLists.add(list);
        return list;
    }

    private void clearImageLists() {
        for (IImageList list : mAllLists) {
            list.close();
        }
        mAllLists.clear();
    }

    private byte[] transform(Bitmap bitmap) {
        byte [] data = null;
        if(bitmap != null) {
            ByteArrayOutputStream miniOutStream = new ByteArrayOutputStream();
            bitmap.compress(Bitmap.CompressFormat.PNG, 80, miniOutStream);

            try {
                miniOutStream.close();
                data = miniOutStream.toByteArray();
            } catch (java.io.IOException ex) {
                Log.e(TAG, "got exception ex " + ex);
            }
        }
        return data;
    }

    @Override
    public void onClick(View v) {
        int viewId = v.getId();
        switch (viewId) {
            case R.id.settings_more:
                showPopupWindow();
                break;
            case R.id.photo_collection:
                try {
                    Intent intent = new Intent();
                    intent.setData(Uri.parse("pcs://com.nttdocomo.android.photoviewer/top"));
                    startActivity(intent);
                } catch (ActivityNotFoundException e) {
                    DialogUtils.showDialog(this, false);
                }
                break;
            case R.id.gallery_take_photoes:
                startActivity(new Intent(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA));
                break;
            case R.id.gallery_cloud_sync:
                if(!Environment.getExternalStorageState().equals(android.os.Environment.MEDIA_MOUNTED)) {
                    Toast.makeText(ImageGallery.this, R.string.text_no_sdcard, Toast.LENGTH_LONG).show();
                    return;
                }
                mSharepre = PreferenceManager.getDefaultSharedPreferences(this);
                if(isCloudSyncing) {
                    Toast.makeText(getApplicationContext(), getString(R.string.text_syncing_message), Toast.LENGTH_LONG).show();
                    return;
                }
                final boolean isShowNextTime = mSharepre.getBoolean("showNextTime", true);
                if(Util.isNetworkAvailable(this)) {
                    if(Util.isWiFiNetworkAvilable(this)){
                        if(isShowNextTime) {
                            showDialogSync();
                        } else {
                            cloudSync();
                        }
                    } else {
                        showAlertDialog(getResources().getString(R.string.text_no_wifi_network_message) , new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                if(isShowNextTime) {
                                    showDialogSync();
                                } else {
                                    cloudSync();
                                }
                            }
                        } ,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int id) {
                                dialog.dismiss();
                            }
                        });
                    }
                } else {
                    showAlertDialog(getResources().getString(R.string.telecom_cloud_text_no_network_available),
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    Util.setupNetwork(ImageGallery.this);
                                }
                            },
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int id) {
                                    dialog.dismiss();
                                }
                            });
                }
                break;
            default:
                break;
        }
    }

    private void showPopupWindow() {
        mIsLockedAblum = mSharePref.getBoolean(UGalleryConst.KEY_PASSWORD_SETTING, true);
        View contentView = View.inflate(getApplicationContext(),
                R.layout.popup_settings_item, null);
        TextView popupLock = (TextView) contentView.findViewById(R.id.popup_lock_photoes);
        if (mIsLockedAblum) {
            popupLock.setText(R.string.text_settings_show_hiden_files);
        } else {
            popupLock.setText(R.string.text_settings_hide_hiden_files);
        }

        View.OnClickListener listener =  new View.OnClickListener(){

            @Override
            public void onClick(View v) {
                switch (v.getId()) {
                case R.id.popup_lock_photoes:
                    if(PasswordUtils.isPasswordFileExist()) {
                        ImagePassWordDialog dialog =  new ImagePassWordDialog(ImageGallery.this, false, new PrivateAbulmCallback() {

                            @Override
                            public void controlPrivateAbulmOFF() {
                            }
                            @Override
                            public void controlPrivateAbulmNO() {
                                showOrHideFiles();
                            }
                        });
                        dialog.show();
                    } else {
                        showOrHideFiles();
                    }
                    break;
                case R.id.popup_settings:
                    startActivity(new Intent(ImageGallery.this, UGalleryPreferenceActivity.class));
                    break;
                default:
                    break;
                }
                mPopupWindow.dismiss();
            }
        };
        popupLock.setOnClickListener(listener);
        contentView.findViewById(R.id.popup_settings).setOnClickListener(listener);
        mPopupWindow = new PopupWindow(contentView ,LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT, true);
        mPopupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        mPopupWindow.setFocusable(true);
        mPopupWindow .setOutsideTouchable(true);
        View view = findViewById(R.id.nav_to_album_layout);
        mPopupWindow.showAtLocation(view,Gravity.RIGHT|Gravity.TOP, 0, view.getHeight()-8);
    }

    private void showOrHideFiles() {
        mIsLockedAblum = !mIsLockedAblum;
        if (mIsLockedAblum) {
            if(lockPrivateAblum()) {
                mAdapter.clear();
                startWorker();
            }
        } else {
            showPrivateAblum();
        }
        mSharePref.edit().putBoolean(UGalleryConst.KEY_IS_SHOW_LOCKED_IMAGE, mIsShowLockedImage).commit();
        mSharePref.edit().putBoolean(UGalleryConst.KEY_PASSWORD_SETTING, mIsLockedAblum).commit();
    }

    private void showAlertDialog(String msg , DialogInterface.OnClickListener listenerOk, DialogInterface.OnClickListener listenerCancel ) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(getResources().getString(R.string.text_no_wifi_network_title))
                .setMessage(msg)
                .setCancelable(false)
                .setPositiveButton(getResources().getString(R.string.picture_delete_ok), listenerOk)
                .setNegativeButton(getResources().getString(R.string.picture_delete_cancel),listenerCancel);
        AlertDialog alert = builder.create();
        alert.show();
    }

    private void showDialogSync() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(getResources().getString(R.string.text_no_wifi_network_title))
                .setMessage( mAdapter.isCloudIconExist() ? getString(R.string.text_wifi_network_message) : getString(R.string.text_sync_start_no_syncicon_message))
                .setCancelable(false)
                .setPositiveButton(getResources().getString(R.string.telecom_cloud_text_yes), new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                        cloudSync();
                    }
                })
                .setNegativeButton(getResources().getString(R.string.telecom_cloud_text_no),new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        mSharepre.edit().putBoolean("showNextTime", false).commit();
                        cloudSync();
                    }
                } );
        AlertDialog alert = builder.create();
        alert.show();
    }

    private void cloudSync() {
        Intent intent = new Intent();
        intent.setAction("telecom.mdesk.cloud.sys.ACTION_SYNC_PHOTO_START");
        intent.setPackage("telecom.mdesk.cloud.sys");
        intent.setClassName("telecom.mdesk.cloud.sys", "telecom.mdesk.cloud.sys.SysSyncPhotoService");
        Messenger messenger = new Messenger(new Handler(){
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                case 0:
                    CharSequence[] dirs = msg.getData().getCharSequenceArray("photo.dirs");
                    Log.d(TAG, "dirs = " + Arrays.toString(dirs));
                    break;
                case 1:
                    Bundle data = msg.getData();
                    String code = data.getString("result.code");
                    String message = data.getString("result.message");
                    int upcount = data.getInt("result.up.count");
                    int downcount = data.getInt("result.down.count");
                    int totalCount = upcount + downcount;
                    if("0".equals(code)) {
                        if(totalCount == 0) {
                            Toast.makeText(ImageGallery.this, getString(R.string.text_sync_notneed_message), Toast.LENGTH_LONG).show();
                        } else {
                            Toast.makeText(ImageGallery.this, getString(R.string.text_sync_success_message)+ totalCount, Toast.LENGTH_LONG).show();
                            scanSdCard(TELECOM_CLOUD_PATH);
                        }
                        isCloudSyncing = false;
                    } else {
                        if("cancel_login".equals(message)) {
                            Toast.makeText(ImageGallery.this, getString(R.string.text_login_failed_message), Toast.LENGTH_LONG).show();
                        } else {
                            Toast.makeText(ImageGallery.this, getString(R.string.text_sync_failed_message), Toast.LENGTH_LONG).show();
                        }
                        isCloudSyncing = false;
                    }
                    break;
                case 2:
                    int upCount = msg.arg1;
                    int totalUpCount = msg.arg2;
                    break;
                case 3:
                    int downCount = msg.arg1;
                    int totalDownCount = msg.arg2;
                    break;
                case 4:
                    int delCount = msg.arg1;
                    int totalDelCount = msg.arg2;;
                    if(delCount == 0) {
                        isCloudSyncing = true;
                        Toast.makeText(getApplicationContext(), getString(R.string.text_sync_start_message), Toast.LENGTH_LONG).show();
                    }
                    break;
                default:
                    break;
                }
                super.handleMessage(msg);
            }
        });
        intent.putExtra("messenger", messenger);
        startService(intent);
    }

    private void scanSdCard(String path) {
        Intent i = new Intent();
        i.setAction(Intent.ACTION_MEDIA_MOUNTED);
        Uri uri = Uri.parse("file://"+path);
        i.setData(uri);
        sendBroadcast(i);
    }

   /* public void fileScan(String file) {
        Uri data = Uri.parse("file://" + file);
        sendBroadcast(new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE, data));
    }
*/
    /*public void folderScan(String path) {
        Log.d(TAG, "folderScan path = "+ path);
        File file = new File(path);
        if (file.isDirectory()) {
            File[] array = file.listFiles();
            for (int i = 0; i < array.length; i++) {
                File f = array[i];
                if (f.isFile()) {
                    Log.d(TAG, "folderScan f.getAbsolutePath() = "+ f.getAbsolutePath());
                    fileScan(f.getAbsolutePath());
                } else {
                    folderScan(f.getAbsolutePath());
                }
            }
        }
    }*/
}

/**
 * Item is the underlying data for GalleryAdapter.
 * It is passed from the activity to the adapter.
 *
 */
class Item {
    public static final int TYPE_NONE = -1;
    public static final int TYPE_ALL_IMAGES = 0;
    public static final int TYPE_ALL_VIDEOS = 1;
    public static final int TYPE_CAMERA_IMAGES = 2;
    public static final int TYPE_CAMERA_VIDEOS = 3;
    public static final int TYPE_CAMERA_MEDIAS = 4;
    public static final int TYPE_NORMAL_FOLDERS = 5;

    public final int mType;
    public final String mBucketId;
    public final String mName;
    public final IImageList mImageList;
    public final int mCount;
    private File mOldDir;
    private File mNewDir;
    public final Uri mFirstImageUri; // could be null if the list is empty

    // The thumbnail bitmap is set by setThumbBitmap() later because we want
    // to let the user sees the folder icon as soon as possible (and possibly
    // select them), then present more detailed information when we have it.
    public Bitmap mThumbBitmap; // the thumbnail bitmap for the image list
    private ArrayList<File> mOldDirFiles;

    public Item(int type, String bucketId, String name, IImageList list) {
        mType = type;
        mBucketId = bucketId;
        mName = name;
        mImageList = list;
        mCount = list.getCount();
        if (mCount > 0) {
            mFirstImageUri = list.getImageAt(0).fullSizeImageUri();
        } else {
            mFirstImageUri = null;
        }
    }
    public void reNameDir() {
        IImage image = mImageList.getImageAt(0);
        String filePath = null;
        if(image != null) {
            filePath = image.getDataPath();
        }
        if (filePath != null) {
            mOldDir = new File(filePath).getParentFile();
            mNewDir = new File(mOldDir.getPath()+"backup");
            mOldDir.renameTo(mNewDir);
        }
    }
    public void restoreDir() {
        mNewDir.renameTo(mOldDir);
    }
    public void reNameVideoDir() {
        mOldDirFiles = new ArrayList<File>();
        for (int i = 0; i < mImageList.getCount(); i++) {
            IImage image = mImageList.getImageAt(i);
            String filePath = null;
            if(image != null) {
                filePath = image.getDataPath();
            }
            if (filePath != null) {
                File oldDir = new File(filePath).getParentFile();
                mOldDirFiles.add(oldDir);
                File newDir = new File(oldDir.getPath()+"backup");
                oldDir.renameTo(newDir);
            }
        }
    }
    public void restoreVideoDir() {
        for (File file : mOldDirFiles) {
            File newDir = new File(file.getPath()+"backup");
            newDir.renameTo(file);
        }
    }
    public void setThumbBitmap(Bitmap thumbBitmap) {
        mThumbBitmap = thumbBitmap;
    }

    public boolean needsBucketId() {
        return mType >= TYPE_CAMERA_IMAGES;
    }

    public void launch(ImageGallery activity) {
        Uri uri = Images.Media.INTERNAL_CONTENT_URI;
        if (needsBucketId()) {
            uri = uri.buildUpon().appendQueryParameter("bucketId", mBucketId).build();
        }
        boolean isPick = activity.isPickIntent();
        String galleryEntry = activity.getGalleryEntry();
        try {
            if(isPick) {
                Intent pickIntent = new Intent(activity, BaseImagePicker.class);
                Bundle myExtras = activity.getIntent().getExtras();
                if(myExtras != null) {
                    pickIntent.putExtras(myExtras);
                }
                pickIntent.setData(uri);
                pickIntent.putExtra("mediaTypes", getIncludeMediaTypes());
                pickIntent.putExtra(ViewImage.EXTRACURRENT_BUCKID_KEY, mBucketId);
                pickIntent.putExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY, galleryEntry);
                activity.startActivityForResult(pickIntent, ImageGallery.REQUEST_CODE_THIRD_PICK);
            } else {
                Class<? extends BaseImagePicker> claz = activity.getImagePickerClass();
                if (claz == null) {
                    throw new RuntimeException("No ImageGallery class provided");
                }
                try {
                    Intent intent = new Intent(activity, claz);
                    intent.setData(uri);
                    intent.putExtra("mediaTypes", getIncludeMediaTypes());
                    intent.putExtra(ViewImage.EXTRACURRENT_BUCKID_KEY, mBucketId);
                    intent.putExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY, galleryEntry);
                    intent.putExtra(ViewImage.EXTRA_UPHOTO_MODULE_KEY, activity.getUPhotoModule());
                    if (activity.isWaitForResult()) {
                        intent.putExtra(ImageGallery.INTENT_EXTRA_IMAGE_MAX_LIMIT, activity.getImageMaxLimit());
                        activity.startActivityForResult(intent, ImageGallery.REQUEST_CODE_PICK);
                    } else {
                        activity.startActivity(intent);
                    }
                    if(ViewImage.IMAGE_ENTRY_NORMAL_VALUE.endsWith(galleryEntry)) {
                        activity.finish();
                    }
                } catch (Exception e) {
                    Log.e(ImageGallery.TAG, "Fail start Activity " + claz, e);
                }
            }
        } catch(Exception e) {
            Log.e(ImageGallery.TAG, "Fail pick Activity ", e);
        }

    }

    public int getIncludeMediaTypes() {
        return convertItemTypeToIncludedMediaType(mType);
    }

    public static int convertItemTypeToIncludedMediaType(int itemType) {
        switch (itemType) {
            case TYPE_ALL_IMAGES:
            case TYPE_CAMERA_IMAGES:
                return ImageManager.INCLUDE_IMAGES;
            case TYPE_ALL_VIDEOS:
            case TYPE_CAMERA_VIDEOS:
                return ImageManager.INCLUDE_VIDEOS;
            case TYPE_NORMAL_FOLDERS:
            case TYPE_CAMERA_MEDIAS:
            default:
                return ImageManager.INCLUDE_IMAGES;
        }
    }

    private File makeThumbnailPath() {
        if (mImageList == null
                || mImageList.getImageAt(0) == null) {
            Log.w("Item", "mImageList is empty");
            return null;
        }

        String filepath = mImageList.getImageAt(0).getDataPath();
        if (filepath == null) {
            Log.w("Item","image has no path info, ignore");
            return null;
        }
        File imageFile = new File(filepath);
        if (!imageFile.exists()) {
            return null;
        }
        return new File(imageFile.getParentFile(),".cover");
    }

    public void loadThumbnailFromFile() {
        Log.d("Item","load: " + mBucketId + "," + mName);
        if (mThumbBitmap != null) {
            return;
        }

        File file = makeThumbnailPath();
        if (file != null) {
           mThumbBitmap = BitmapFactory.decodeFile(file.getPath());
        }
    }

    public void saveThumbnailToFile(Bitmap thumbnail) {
        File file = makeThumbnailPath();
        if (file == null) {
            Log.w("Item", "Fail make thumbnail cache path");
            return;
        }

        if (file.exists()) {
            /* SPRD: CID 109343 : RV: Bad use of return value (FB.RV_RETURN_VALUE_IGNORED_BAD_PRACTICE) @{ */
            if(!file.delete()) {
                return;
            }
            // file.delete();
            /* @} */
        }

        OutputStream os = null;
        try {
            os = new FileOutputStream(file);
            thumbnail.compress(CompressFormat.PNG, 90, os);
        }catch (Exception e) {
            Log.w("Item", "Fail cache thumbnail");
        } finally {
            Util.closeSilently(os);
        }
    }
}

class GalleryAdapter extends BaseAdapter {
    ArrayList<Item> mItems = new ArrayList<Item>();
    LayoutInflater mInflater;
    private final String mImagePath;
/*    private File mOldDir;
    private File mNewDir;*/
    GalleryAdapter(Activity act) {
        mInflater = act.getLayoutInflater();
        if (Build.isTelecomCloud()) {
            SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(act);
            mImagePath = pref.getString("sf_pref_ucam_select_path_key", ImageGallery.TELECOM_CLOUD_PATH);
        } else {
            mImagePath = null;
        }
    }

    public void addItem(Item item) {
        mItems.add(item);
        notifyDataSetChanged();
    }

    public void updateDisplay() {
        notifyDataSetChanged();
    }

    public void clear() {
        mItems.clear();
        notifyDataSetChanged();
    }

    public int getCount() {
        return mItems.size();
    }

    public Object getItem(int position) {
        return null;
    }

    public String getItemmBucketId(int position){
        return mItems.get(position).mBucketId;
    }
    public long getItemId(int position) {
        return position;
    }

    public String baseTitleForPosition(int position) {
        return mItems.get(position).mName;
    }

    public int getIncludeMediaTypes(int position) {
        return mItems.get(position).getIncludeMediaTypes();
    }

    public void removeItem(View v){
        mItems.remove(v);
        notifyDataSetChanged();
    }

    public void removeItem(int index){
        /**
         * BUG FIX:3086
         * BUG CAUSE:java.lang.ArrayIndexOutOfBoundsException
         * DATE:2013-03-06
         */
        if(index >= 0 && index < mItems.size()){
            mItems.remove(index);
            notifyDataSetChanged();
        }
    }

    public boolean isCloudIconExist(){
        for (Item item : mItems) {
            String filePath = item.mImageList.getImageAt(0).getDataPath();
            if(filePath != null) {
                File file = new File(filePath);
                String dir = file.getParentFile().getPath();
                if(mImagePath != null && dir.startsWith(mImagePath)) {
                    return true;
                }
            }
        }
        return false;
    }
    public boolean isLockedIcon(int index) {
        return isImageLockedIcon(index) || isVideoLockedIcon(index);
    }
    public boolean isImageLockedIcon(int index) {
        /*        IImage image =  mItems.get(index).mImageList.getImageAt(0);
        String filePath = null;
        if(image != null) {
            filePath = image.getDataPath();
        }
        if (filePath != null) {
            File file = new File(filePath);
            String dir = file.getParentFile().getPath();
            if (dir.startsWith(Constants.STORE_DIR_LOCKED) || dir.startsWith(Constants.STORE_DIR_LOCKED_PHOTOGRAPY)) {
                return true;
            }
        }*/
        IImageList list = null;
        /*
         * FIX BUG: 59
         * BUG COMMENT: Avoid IndexOutOfBoundsException.
         * DATE: 2014-03-04
         */
        if(index >= 0 && index < mItems.size()) {
            list =  mItems.get(index).mImageList;
        }
        if(list == null) {
            return false;
        }
        for (int i = 0; i < list.getCount(); i++) {
            if(ImageManager.isVideo(list.getImageAt(i)))
                continue;
            String path = list.getImageAt(i).getDataPath();
            if (path != null && (path.startsWith(Constants.STORE_DIR_LOCKED) || path.startsWith(Constants.STORE_DIR_LOCKED_PHOTOGRAPY))) {
                return true;
            }
        }
        return false;
    }
    public boolean isVideoLockedIcon(int index){
        IImageList list = null;
        if(index >= 0 && index < mItems.size()) {
            list =  mItems.get(index).mImageList;
        }
        if(list == null) {
            return false;
        }
//        IImage image =  mItems.get(index).mImageList.getImageAt(0);
        if(ImageManager.isVideoBucketId((mItems.get(index).mBucketId))) {
            for (int i = 0; i < list.getCount(); i++) {
                String path = list.getImageAt(i).getDataPath();
                if (path != null && path.startsWith(Constants.STORE_DIR_LOCKED)) {
                    return true;
                }
            }
        }
        return false;
    }

    public View getView(final int position, View convertView, ViewGroup parent) {
        ViewHolder viewHolder = null;
        if (convertView == null) {
            convertView = mInflater.inflate(R.layout.image_album_item, null);
            viewHolder = new ViewHolder();
            viewHolder.titleView = (TextView) convertView.findViewById(R.id.title);
            viewHolder.countView = (TextView) convertView.findViewById(R.id.count);
            viewHolder.thumbView = (ImageView) convertView.findViewById(R.id.thumbnail);
            viewHolder.cloudIcon = (ImageView) convertView.findViewById(R.id.cloud_icon);
            viewHolder.lockIcon = (ImageView) convertView.findViewById(R.id.lock_icon);
            convertView.setTag(viewHolder);
        } else {
            viewHolder = (ViewHolder)convertView.getTag();
        }

        Item item = null;
        /*
         * FIX BUG: 6041
         * BUG COMMENT: Java.lang.IndexOutOfBoundsException
         * DATE: 2014-03-06
         */
        if(mItems != null && position >= 0 && position < mItems.size()) {
            item = mItems.get(position);
            if(isLockedIcon(position)) {
                viewHolder.lockIcon.setVisibility(View.VISIBLE);
            } else {
                viewHolder.lockIcon.setVisibility(View.GONE);
            }
        }
        if(Build.isTelecomCloud() && item != null && item.mImageList != null) {
            String filePath = null;
            IImage image = item.mImageList.getImageAt(0);
            if (image != null) {
                filePath = image.getDataPath();
                /*
                 * FIX BUG: 5721 BUG COMMENT: Avoid null pointer exception
                 * DATE: 2014-01-10
                 */
                if (filePath != null) {
                    File file = new File(filePath).getParentFile();
                    String dir = null;
                    if (file != null) {
                        dir = file.getPath();
                    }
                    if (mImagePath != null && dir != null
                            && dir.startsWith(mImagePath)) {
                        viewHolder.cloudIcon.setVisibility(View.VISIBLE);
                    } else {
                        viewHolder.cloudIcon.setVisibility(View.GONE);
                    }
                }
            }
        }

        /*
         * FIX BUG: 5199 5350
         * BUG COMMENT: Avoid null pointer exception
         * DATE: 2013-11-11 2013-12-10
         */
        if(item != null) {
            viewHolder.thumbView.setImageBitmap(item.mThumbBitmap);
            viewHolder.countView .setText(" (" + item.mCount + ")");
            viewHolder.titleView.setText(item.mName);
        }

        // An workaround due to a bug in TextView. If the length of text is
        // different from the previous in convertView, the layout would be
        // wrong.
        viewHolder.titleView.requestLayout();

        return convertView;
    }

    private final class ViewHolder {
        TextView titleView;
        TextView countView;
        ImageView thumbView;
        ImageView cloudIcon;
        ImageView lockIcon;
    }
}
