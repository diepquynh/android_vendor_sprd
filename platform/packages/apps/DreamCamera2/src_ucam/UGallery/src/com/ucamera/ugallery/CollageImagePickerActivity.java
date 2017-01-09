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

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.Serializable;
import java.lang.ref.WeakReference;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

import com.sprd.camera.storagepath.StorageUtil;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.ucamera.ugallery.MediaScanner.ScannerFinishCallBack;
import com.ucamera.ugallery.gallery.UriImage;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.LauncherActivity;
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
import android.content.res.Configuration;
import android.database.ContentObserver;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
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
import android.provider.MediaStore.Images.ImageColumns;
import android.provider.MediaStore.Images.Media;
import android.provider.MediaStore.Images.Thumbnails;
import android.text.SpannableStringBuilder;
import android.text.format.DateFormat;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.LinearLayout.LayoutParams;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.GridView;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListAdapter;
import android.widget.RelativeLayout;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;

import com.ucamera.ugallery.gallery.IImage;
import com.ucamera.ugallery.gallery.IImageList;
import com.ucamera.ugallery.gallery.ImageList;
import com.ucamera.ugallery.gallery.ImageLoader;
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
import com.ucamera.ugallery.provider.ThumbnailUtils;
import com.ucamera.ugallery.provider.UCamData;
import com.ucamera.ugallery.util.BitmapManager;
import com.ucamera.ugallery.util.CheckVersion;
import com.ucamera.ugallery.util.Compatible;
import com.ucamera.ugallery.util.ImageManager;
import com.ucamera.ugallery.util.UiUtils;
import com.ucamera.ugallery.util.ImageManager.ImageListParam;
import com.ucamera.ugallery.util.UpdateService;
import com.ucamera.ugallery.util.Util;

public class CollageImagePickerActivity extends Activity implements GridViewSpecial.Listener,
GridViewSpecial.DrawAdapter,View.OnClickListener{
    protected static final String TAG = "CollageImagePickerActivity";
    private Thread mWorkerThread;
    private MyGridView mFolderGridView;
    private GalleryCollageAdapter mAdapter; // mAdapter is only accessed in main
                                            // thread.
    private boolean mScanning;
    private boolean mUnmounted;
    private Dialog mMediaScanningDialog;
    private ImageListData[] mImageListData = null;
    public Drawable mCellOutline;
    public static final String TELECOM_CLOUD_PATH = Environment
            .getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)
            .toString()
            + "/Camera";
    private static final String INTENT_EXTRA_IMAGE_GALLERY_CLASS = "Extra.ImageGallery.Class";
    private Class<? extends BaseImagePicker> mImageGalleryCollageImpl;
    private static final String INTENT_EXTRA_WAIT_FOR_RESULT = "Extra.Wait.For.Result";
    private boolean mIsRequestForResult = false;
    private int mImageMaxLimit = -1;
    public static final String INTENT_EXTRA_IMAGES = "Extra.Image.Uris";
    public static final String INTENT_EXTRA_IMAGE_MAX_LIMIT = "Extra.Image.MaxLimit";
    private static String mGalleryEntry = null;
    private int mUPhotoModule = -1;
    private ArrayList<ContentProviderOperation> mCoverList;
    private ArrayList<IImage> mThumbImageList;
    private GridViewSpecial mGvs;
    private AsyncImageLoader mImageLoader;
    private GridView mThumbGridView;
    private boolean mPausing;
    private TextView mPhotoNum, mPuzzPhoto;
    private int MAX_NUMB = 9;
    // private ImageView mItemImageView;
    private static String mCurrentStoragePos = "external";
    private boolean mIsReSelect = false;
    // handler for the main thread
    IImageList mAllImageList;
    private ImagePassWordDialog mImagePassWordDialog;
    private HorizontalScrollView mHorSPuzzleView;
    private ProgressDialog mImageRestoreDialog;
    private Handler mHandler = new Handler(){
        public void handleMessage(Message msg) {
            int what = msg.what;
            switch (what) {
                case ImageGallery.SHOW_BACK_UP_ALLIMAGES_DIALOG:
                    new SecretDialog(CollageImagePickerActivity.this , new SecretDialogOnClickListener() {

                        @Override
                        public void secretDialogOnClick() {
                            mImageRestoreDialog = ProgressDialog.show(CollageImagePickerActivity.this, null, getResources().getString(
                                    R.string.text_waiting), true, false);
                            ArrayList<String> imageRestorelist = new ArrayList<String>();
                            if(new File(Constants.STORE_DIR_LOCKED).exists()) {
                                imageRestorelist =  LockUtil.restoreAllImages(Constants.STORE_DIR_LOCKED, new File(Constants.STORE_DIR_LOCKED) , imageRestorelist ,CollageImagePickerActivity.this);
                            }
                            if(new File(Constants.STORE_DIR_LOCKED_PHOTOGRAPY).exists()) {
                                imageRestorelist =  LockUtil.restoreAllImages(Constants.STORE_DIR_LOCKED_PHOTOGRAPY, new File(Constants.STORE_DIR_LOCKED_PHOTOGRAPY) , imageRestorelist,CollageImagePickerActivity.this);
                            }

                            new MediaScanner(CollageImagePickerActivity.this, new ScannerFinishCallBack() {
                                @Override
                                public void scannerComplete() {
                                    mHandler.sendEmptyMessage(ImageGallery.BACK_UP_ALLIMAGES_FINISH);
                                }
                            }).scanFile(imageRestorelist, null);
                        }
                    }).show();
                    break;
                case ImageGallery.BACK_UP_ALLIMAGES_FINISH:
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
        UiUtils.initialize(this);
        setContentView(R.layout.collage_image_picker);
        mLoader= new ImageLoader(getContentResolver(), mHandler);
        init();
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
        initData();
        if(android.os.Build.VERSION.SDK_INT == 15 ) {
            Build.NEED_SECRET_ABLUM = false;
            new Thread(new Runnable() {
                @Override
                public void run() {
                    if(LockUtil.isExistHidenImages(Constants.STORE_DIR_LOCKED) || LockUtil.isExistHidenImages(Constants.STORE_DIR_LOCKED_PHOTOGRAPY)) {
                        mHandler.sendEmptyMessage(ImageGallery.SHOW_BACK_UP_ALLIMAGES_DIALOG);
                    }
                }
            }).start();
        }
        mPuzzPhoto.setOnClickListener(puzzleOnclcik);
        mFolderGridView
                .setOnItemClickListener(new AdapterView.OnItemClickListener() {

                    public void onItemClick(AdapterView<?> parent, View view,
                            final int position, long id) {
                        if(mAdapter.isImageLockedIcon(position) && PasswordUtils.isPasswordFileExist()) {
                            mImagePassWordDialog = new ImagePassWordDialog(CollageImagePickerActivity.this, false, new PrivateAbulmCallback() {
                                @Override
                                public void controlPrivateAbulmOFF() {
                                    mImagePassWordDialog.dismiss();
                                }
                                @Override
                                public void controlPrivateAbulmNO() {
                                    launchFolderGallery(position);
                                    mGvs.setVisibility(View.VISIBLE);
                                    mFolderGridView.setVisibility(View.GONE);
                                }
                            });
                            mImagePassWordDialog.show();
                        } else {
                            launchFolderGallery(position);
                            mGvs.setVisibility(View.VISIBLE);
                            mFolderGridView.setVisibility(View.GONE);
                        }
                    }
                });
        mThumbGridView.setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view,
                    int position, long id) {
                mThumbImageList.remove(position);
                showPhotoNmber();
                updateThumbGridviewUI(mThumbGridView, mThumbAdapter.getCount(), 5);
                mThumbAdapter.notifyDataSetChanged();
            }
        });
        if (mThumbImageList.size() == 0) {
            mPhotoNum.setText(String.format(
                    getResources().getString(
                            R.string.text_collage_selected_pictures), 0));
        }

        mImageListData = new ImageListData[] {
                new ImageListData(Item.TYPE_CAMERA_IMAGES,
                        ImageManager.INCLUDE_IMAGES,
                        ImageManager.UPHOTO_BUCKET_ID,
                        "UPhoto"),
                new ImageListData(Item.TYPE_CAMERA_IMAGES,
                        ImageManager.INCLUDE_IMAGES,
                        ImageManager.UCAM_BUCKET_ID,
                        "UCam"),
                // Camera Images
                new ImageListData(Item.TYPE_CAMERA_IMAGES,
                        ImageManager.INCLUDE_IMAGES,
                        ImageManager.CAMERA_IMAGE_BUCKET_ID,
                        getString(R.string.text_camera_bucket_name)),

        };

        ImageManager.ensureOSXCompatibleFolder();
        initImageGalleryFromIntent(getIntent());
    }

    private void initData() {
        Parcelable[] data = getIntent().getParcelableArrayExtra("ucam.puzzle.IMAGES");
        if(data != null) {
            findViewById(R.id.collage_back).setVisibility(View.VISIBLE);
            findViewById(R.id.collage_back).setOnClickListener(this);
            mIsReSelect = true;
            for(int i = 0; i < data.length; i++) {
             mThumbImageList.add(new UriImage(null, this.getContentResolver(), (Uri)data[i]));
            };
            updateThumbGridviewUI(mThumbGridView, mThumbAdapter.getCount(), 5);
            showPhotoNmber();
             mHorSPuzzleView.post(new Runnable() {
                @Override
                public void run() {
                 mHorSPuzzleView.scrollTo(0, 0);
                }
            });
        }
    }
    private void updateThumbGridviewUI(GridView gridview, int totalCount, int countPerScreen) {
        mPuzzPhoto.setEnabled(totalCount >1);
        DisplayMetrics metrics = new DisplayMetrics();
        WindowManager wm = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
        wm.getDefaultDisplay().getMetrics(metrics);
        int itemWidth=(int)getResources().getDimension(R.dimen.collage_thumbnail_perScreen_item);
        int padding=(int)getResources().getDimension(R.dimen.collage_thumbgridview_edge);
        countPerScreen = Math.max(metrics.widthPixels / (itemWidth + padding), countPerScreen);

//        gridview.setNumColumns(totalCount);
//        final int itemWidth = metrics.widthPixels / countPerScreen;
//        int mode = totalCount % countPerScreen;
//        if (mode > 0) {
//            totalCount = totalCount + (countPerScreen - mode);
//        }
        gridview.setNumColumns(totalCount);
        final int layout_width = itemWidth * totalCount + padding * (totalCount-1);
        gridview.setLayoutParams(new LinearLayout.LayoutParams(layout_width,
                LayoutParams.WRAP_CONTENT));
        if (totalCount > countPerScreen) {
          final int scrollToX= (int) ((itemWidth +padding) * (totalCount - countPerScreen));
          mHorSPuzzleView.post(new Runnable() {
                @Override
                public void run() {
                  mHorSPuzzleView.scrollTo(scrollToX, 0);
                }
            });
        } else {
                mHorSPuzzleView.scrollTo(0, 0);
        }
    }

    private void init() {
        mAdapter = new GalleryCollageAdapter(this); // is PhotoView
        mThumbAdapter = new ThumbAdapter(CollageImagePickerActivity.this);
        mPuzzPhoto = (TextView) findViewById(R.id.btn_puzz_operate);
        mPhotoNum = (TextView) findViewById(R.id.text_most_photo_number);

        mGvs = (GridViewSpecial) this.findViewById(R.id.collage_grid);
        mGvs.setListener(this);

        mFolderGridView = (MyGridView) findViewById(R.id.albums);
        mFolderGridView.setSelector(android.R.color.transparent);
        mFolderGridView.setAdapter(mAdapter);
        mThumbGridView = (GridView) findViewById(R.id.collage_thumb_gv);
        mThumbGridView.setAdapter(mThumbAdapter);
        mThumbImageList = new ArrayList<IImage>();
        mHorSPuzzleView = (HorizontalScrollView) findViewById(R.id.layout_horscro_type);
    }

    private void showPhotoNmber() {
        String sNumber = getResources().getString(
                R.string.text_collage_selected_pictures);
        String sFinal1 = String.format(sNumber, mThumbImageList.size());
        mPhotoNum.setText(sFinal1);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if(mGvs.getVisibility() == View.VISIBLE) {
                mLoader.stop();
                mGvs.stop();
                IImageList iImageList = mAllImageList;
                if(iImageList != null) {
                    iImageList.close();
                    mAllImageList = null;
                }
                mGvs.setVisibility(View.GONE);
                mFolderGridView.setVisibility(View.VISIBLE);
                return true;
            }
        }
           return super.onKeyDown(keyCode, event);

    }
    @Override
    protected void onResume() {
        super.onResume();
        mGvs.requestFocus();
        mPausing = false;
        if (mImageLoader == null) {
            mImageLoader = new AsyncImageLoader();
            mImageLoader.start();
        }
        if(mGvs.getVisibility()==View.VISIBLE){
            mGvs.start();
        }
    }

/*    @Override
    public void onBackPressed() {
        if(mImagePassWordDialog != null && mImagePassWordDialog.isShowing()) {
            mImagePassWordDialog.dismiss();
        } else {
            super.onBackPressed();
        }
    }*/
    @Override
    protected void onPause() {
        super.onPause();
        mPausing = true;
        if (mImageLoader != null) {
            mImageLoader.interrupt();
        }
        mLoader.stop();
        mGvs.stop();

    }

    @SuppressWarnings("unchecked")
    private void initImageGalleryFromIntent(Intent data) {
        if (data == null)
            return;
        Serializable claz = data
                .getSerializableExtra(INTENT_EXTRA_IMAGE_GALLERY_CLASS);
        if (claz != null) {
            mImageGalleryCollageImpl = (Class<? extends BaseImagePicker>) claz;
        } else {
            mImageGalleryCollageImpl = BaseImagePicker.class;
        }
        mIsRequestForResult = data.getBooleanExtra(
                INTENT_EXTRA_WAIT_FOR_RESULT, false);
        mImageMaxLimit = data.getIntExtra(INTENT_EXTRA_IMAGE_MAX_LIMIT, -1);
        mGalleryEntry = data.getStringExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY);
        mUPhotoModule = data.getIntExtra(ViewImage.EXTRA_UPHOTO_MODULE_KEY, -1);
        if (ViewImage.IMAGE_ENTRY_PUZZLE_VALUE.equals(mGalleryEntry)) {
            findViewById(R.id.collage_thumb_gv).setVisibility(View.VISIBLE);
            findViewById(R.id.text_most_photo_number).setVisibility(View.VISIBLE);
            mPickIntentSelectPhotos = true;
        } else {
            mPickIntentSelectPhotos = false;
        }
    }
    OnClickListener puzzleOnclcik = new OnClickListener() {
        public void onClick(View v) {
            // CID 109114 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
            // Uri uri = Images.Media.INTERNAL_CONTENT_URI;

            if (mThumbImageList.size() < 2) {
                Toast.makeText(mContext,
                        R.string.text_collage_min_select_notice,
                        Toast.LENGTH_SHORT).show();
                return;
            } else {
                try {
                    String mBucketId = mThumbImageList.get(1).getDataPath();
                    Uri mUriList[] = new Uri[mThumbImageList.size()];
                    Intent intent = new Intent();
                    intent.setClassName(CollageImagePickerActivity.this,
                            "com.ucamera.ucomm.puzzle.PuzzleActivity");
                    for (int i = 0; i < mThumbImageList.size(); i++) {
                        mUriList[i] = mThumbImageList.get(i).fullSizeImageUri();
                    }
                    intent.putExtra("ucam.puzzle.IMAGES", mUriList);
                    if(mIsReSelect) {
                        setResult(RESULT_OK, intent);
                        mIsReSelect = false;
                    }else {
                        startActivity(intent);
                    }
                    finish();
                } catch (Exception e) {
                }
            }
        }
    };

    Class<? extends BaseImagePicker> getImagePickerClass() {
        return mImageGalleryCollageImpl;
    }

    boolean isWaitForResult() {
        return mIsRequestForResult;
    }

    int getImageMaxLimit() {
        return mImageMaxLimit;
    }

    static String getGalleryEntry() {
        return mGalleryEntry;
    }

    int getUPhotoModule() {
        return mUPhotoModule;
    }

    public boolean isPickIntent() {
        String action = getIntent().getAction();
        String type = getIntent().getType();
        if (type != null && "video/*".equals(type)) {
            mPickIntentVideoType = true;
        } else {
            mPickIntentVideoType = false;
        }
        return Intent.ACTION_GET_CONTENT.equals(action)
                || Intent.ACTION_PICK.equals(action);
    }

    public static void showImagePicker(Context context,
            Class<? extends BaseImagePicker> pickerclz, String galleryEntry) {
        if (pickerclz == null)
            throw new RuntimeException("No Image Picker class provided");
        Intent intent = new Intent()
                .setClass(context, CollageImagePickerActivity.class)
                .putExtra(ViewImage.EXTRA_IMAGE_ENTRY_KEY, galleryEntry)
                .putExtra(ViewImage.EXTRA_IMAGE_STORAGE_KEY, mCurrentStoragePos)
                .putExtra(INTENT_EXTRA_WAIT_FOR_RESULT, true)
                .putExtra(INTENT_EXTRA_IMAGE_GALLERY_CLASS, pickerclz);
        if (ViewImage.IMAGE_ENTRY_UGALLERY_VALUE.equals(galleryEntry)
                || ViewImage.IMAGE_ENTRY_PICKER_VALUE.equals(galleryEntry)) {
            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        }
        context.startActivity(intent);
    }

    /**
     * @param scanning scanning
     */
    public void updateScanningDialog(boolean scanning) {
        boolean prevScanning = (mMediaScanningDialog != null);
        Log.d(TAG, "prevScanning : " + prevScanning + "scanning :" + scanning
                + "mAdapter.mItems.size() :" + mAdapter.mItems.size());
        if (prevScanning == scanning && mAdapter.mItems.size() == 0) {
            return;
        }
        // Now we are certain the state is changed.
        if (prevScanning) {
            mMediaScanningDialog.cancel();
            mMediaScanningDialog = null;
        } else if (scanning && mAdapter.mItems.size() == 0) {
            mMediaScanningDialog = ProgressDialog
                    .show(this, null,
                            getResources().getString(R.string.text_waiting),
                            true, true);
        }
    }

    private View mNoImagesView;
    private ImageLoader mLoader;
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
    private void launchFolderGallery(int position) {
        ImageListParam lmageParam = ImageManager.getImageListParam(
                ImageManager.DataLocation.ALL, ImageManager.INCLUDE_IMAGES,
                ImageManager.SORT_DESCENDING,
                mAdapter.mItems.get(position).mBucketId);
         mAllImageList = ImageManager.makeImageList(getContentResolver(),  lmageParam);
         /**
          * FIX BUG:6212
          * BUG COMMENT: If the the RunningStat true,mGvs.stop() is nedded;
          * DATE: 2014-04-17
          */
         if(mGvs.getRunningStat()){
             mGvs.stop();
         }
         mGvs.setImageList(mAllImageList);
         mGvs.setDrawAdapter(this);
         mGvs.setLoader(mLoader);
         mGvs.start();
    }
    @Override
    public void onStop() {
        super.onStop();
        abortWorker();
        unloadDrawable();
    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        mAdapter = null;
        mThumbAdapter=null;
        mFolderGridView.setAdapter(null);
        mThumbGridView.setAdapter(null);
    }
    @Override
    public void onStart() {
        super.onStart();
        // install an intent filter to receive SD card related events.
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Intent.ACTION_MEDIA_MOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_SCANNER_STARTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_SCANNER_FINISHED);
        intentFilter.addAction(Intent.ACTION_MEDIA_EJECT);
        intentFilter.addDataScheme("file");
        mIsLockedAblum = PreferenceManager.getDefaultSharedPreferences(this).getBoolean(UGalleryConst.KEY_PASSWORD_SETTING, true);
        // Assume the storage is mounted and not scanning.
        if(!checkSDCard()){
            mHorSPuzzleView.setVisibility(View.GONE);
            findViewById(R.id.layout_bottom_normal_action).setVisibility(View.GONE);
        }
        mUnmounted = false;
        mScanning = false;
        startWorker();
    }

    private boolean checkSDCard() {
        boolean sdCardExist = Environment.getExternalStorageState().equals(
                android.os.Environment.MEDIA_MOUNTED);
        if (sdCardExist == false && Util.getInternalStorageDir() == null) {
            /*
             * FIX BUG:5811
             * BUG COMMENT: if no SDCard so we do nothing
             * DATE: 2014-01-16
             */
//            Toast.makeText(CollageImagePickerActivity.this, R.string.text_no_sdcard,
//                     Toast.LENGTH_LONG).show();
            return false;
        }
        return true;
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
            BitmapManager.instance().cancelThreadDecoding(mWorkerThread,
                    getContentResolver());
            MediaStore.Images.Thumbnails.cancelThumbnailRequest(
                    getContentResolver(), -1);
            mAbort = true;
            try {
                mWorkerThread.join();
            } catch (InterruptedException ex) {
                Log.e(TAG, "join interrupted");
            }
            mWorkerThread = null;
            mHandler.removeMessages(0);
            mAdapter.clear();
            mAdapter.updateDisplay();
            clearImageLists();
        }
    }

    private void checkThumbBitmap(ArrayList<Item> allItems) {
        for (Item item : allItems) {
            if (item.mType != Item.TYPE_ALL_VIDEOS) {
                continue;
            }
            final Bitmap b = makeMiniThumbBitmap(THUMB_SIZE, THUMB_SIZE,
                    item.mImageList);
            if (mAbort) {
                if (b != null)
                    b.recycle();
                return;
            }

            final Item finalItem = item;
            mHandler.post(new Runnable() {
                public void run() {
                    updateThumbBitmap(finalItem, b);
                }
            });
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
        if (!mPickIntentVideoType) {
            checkBucketIds(allItems);
            if (mAbort) {
                return;
            }
        }
        checkScanning();
        checkThumbBitmap(allItems);
        if (mAbort) {
            return;
        }

        /*
         * BUG FIX: 1973 1919 BUG COMMENT: 1973-Muli-thread access problem
         * 1919-catch disk io exception DATE: 2012-11-28
         */
        ArrayList<ContentProviderOperation> coverList = mCoverList;
        if (coverList != null && coverList.size() > 0) {
            try {
                getContentResolver().applyBatch(UCamData.AUTHORITY, coverList);
            } catch (Throwable e) {
                Log.e(TAG, "insert failed", e);
            }
            coverList.clear();
        }

        checkLowStorage();
    }

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
        if (mAdapter.getCount() == 0) {
            hideNoImagesView();
        }
        mAdapter.addItem(item);
        mAdapter.updateDisplay();
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
//            if (Util.getInternalStorageDir() != null) {
                allImages = ImageManager.makeImageList(getContentResolver(),
                        ImageManager.DataLocation.ALL,
                        ImageManager.INCLUDE_IMAGES,
                        ImageManager.SORT_DESCENDING, null);
//            } else {
//                allImages = ImageManager.makeImageList(getContentResolver(),
//                        ImageManager.DataLocation.EXTERNAL,
//                        ImageManager.INCLUDE_IMAGES,
//                        ImageManager.SORT_DESCENDING, null);
//            }
        } else {
            allImages = ImageManager.makeEmptyImageList();
        }

        if (mAbort) {
            allImages.close();
            return;
        }

        HashMap<String, String> hashMap = allImages.getBucketIds();
        allImages.close();
        if (mAbort) {
            return;
        }
        /**
         * FIX BUG: 1090 1132 BUG CAUSE: SD card mounted or is scanning, get
         * EmptyImageList object. FIX COMMENT: Did not get the BucketIds, would
         * directly return. DATE:2012-06-26
         */
        if (hashMap == null) {
            return;
        }

        mCoverList = new ArrayList<ContentProviderOperation>();

        LinkedList<String> bucketIds = new LinkedList<String>();
        bucketIds.addAll(hashMap.keySet());

        for (ImageListData data : mImageListData) {
            if (bucketIds.remove(data.mBucketId)) {
                bucketIds.addFirst(data.mBucketId);
                hashMap.put(data.mBucketId, data.mBucketName);
            }
        }

        for (String key : bucketIds) {
            if (key == null) {
                continue;
            }
            IImageList list = createImageList(ImageManager.INCLUDE_IMAGES, key,
                    getContentResolver());
            if (mAbort) {
                return;
            }
            final Item item = new Item(Item.TYPE_NORMAL_FOLDERS, key, hashMap.get(key), list);
            // item.mThumbBitmap = makeMiniThumbBitmap(THUMB_SIZE, THUMB_SIZE,
            // item.mImageList);
            // item.loadThumbnailFromFile();
            Cursor cursor = null;
            try {
                String name = item.mImageList.getImageAt(0).getTitle();
                IImage image = item.mImageList.getImageAt(0);
                if(image != null && isSystemGallery(image.getDataPath())){
                    continue;
                }
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
                        mCoverList.add(ContentProviderOperation.newInsert(UCamData.Albums.CONTENT_URI)
                                .withValue(UCamData.Albums.ALBUM_BUCKET, item.mBucketId)
                                .withValue(UCamData.Albums.IMAGE_NAME, name)
                                .withValue(UCamData.Albums.ALBUM, coverData)
                                .build());
                    }
                }

                item.mThumbBitmap = temp;

                if(mIsLockedAblum && needImageLockedAbulm(item)) {
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            item.reNameDir();
                            getContentResolver().delete(Images.Media.EXTERNAL_CONTENT_URI,
                                    Media.BUCKET_ID+ "= ?", new String[]{item.mBucketId});
                            item.restoreDir();
                        }
                    }).start();
                }else {
                    allItems.add(item);
                    final Item finalItem = item;
                    mHandler.post(new Runnable() {
                        public void run() {
                            updateItem(finalItem);
                        }
                    });
                }

            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }
        }

        mHandler.post(new Runnable() {
            public void run() {
                checkBucketIdsFinished();
            }
        });
    }

    // This is run in the main thread.
    private void checkBucketIdsFinished() {

        // If we just have one folder, open it.
        // If we have zero folder, show the "no images" icon.
        if (!mScanning) {
            int numItems = mAdapter.mItems.size();
            if (numItems == 0) {
                showNoImagesView();
            }
        }
    }
    public boolean needImageLockedAbulm(Item item) {
        if(item == null) return false;
        if(item.mImageList == null) return false;
        IImage image =  item.mImageList.getImageAt(0);
        String filePath = null;
        if(image != null) {
            filePath = image.getDataPath();
        }
        /*
         * FIX BUG : 4736 4973
         * BUG COMMENT : avoid null point exception
         * DATE : 2013-08-23 2013-10-11
         */
        if (filePath != null) {
            File file = new File(filePath);
            String dir = file.getParentFile() != null ? file.getParentFile().getPath() : null;
            if (dir != null && (dir.startsWith(Constants.STORE_DIR_LOCKED) || dir.startsWith(Constants.STORE_DIR_LOCKED_PHOTOGRAPY))) {
                return true;
            }
        }
        return false;
    }
    private static final int THUMB_SIZE = 240;

    // This is run in the main thread.
    private void updateThumbBitmap(Item item, Bitmap b) {
        item.setThumbBitmap(b);
        mAdapter.updateDisplay();
    }

    private static final long LOW_STORAGE_THRESHOLD = 1024 * 1024 * 2;

    // This is run in the worker thread.
    private void checkLowStorage() {
        // Check available space only if we are writable
        if (ImageManager.hasStorage()) {
            String storageDirectory = Environment.getExternalStorageDirectory()
                    .toString();
            StatFs stat = new StatFs(storageDirectory);
            long remaining = (long) stat.getAvailableBlocks()
                    * (long) stat.getBlockSize();
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
        Toast.makeText(CollageImagePickerActivity.this, R.string.text_sdcar_no_space,
                5000).show();
    }

    /**
     * IMAGE_LIST_DATA stores the parameters for the four image lists we are
     * interested in. The order of the IMAGE_LIST_DATA array is significant (See
     * the implementation of GalleryCollageAdapter.init).
     */
    private static final class ImageListData {
        @SuppressWarnings("unused")
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

    private void unloadDrawable() {
        mCellOutline = null;
    }

    private static void placeImage(Bitmap image, Canvas c, Paint paint,
            int imageWidth, int widthPadding, int imageHeight,
            int heightPadding, int offsetX, int offsetY, int pos) {
        int xPos;
        int yPos ;
        if(UiUtils.screenDensity() == 1) {
            xPos = offsetX + 3;
            yPos = offsetY+ 4 ;
        } else {
            xPos = offsetX + 1;
            yPos = offsetY + 2;
        }
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
        final Bitmap b = Bitmap.createBitmap(width, height,
                Bitmap.Config.ARGB_8888);
        final Canvas c = new Canvas(b);
        final Matrix m = new Matrix();
        unloadDrawable();
        // loadDrawableIfNeeded();
        for (int i = 0; i >= 0; i--) {
            if (mAbort) {
                return null;
            }
            Bitmap temp = null;
            IImage image = i < count ? images.getImageAt(i) : null;
            if (image != null) {
                temp = (Bitmap) image.miniThumbBitmap()[1];
            }
            if (temp != null) {
                temp = Util.transform(m, temp, imageWidth, imageHeight, true,
                        Util.RECYCLE_INPUT);
            } else if (i == 0) {
                try {
                    temp = BitmapFactory.decodeResource(getResources(),
                            R.drawable.missing_thumbnail_picture,
                            Util.getNativeAllocOptions());
                } catch (OutOfMemoryError ex) {
                    android.util.Log.w(TAG,
                            "makeMiniThumbBitmap(): decode resource oom", ex);
                    continue;
                }
                if (temp == null) {
                    continue;
                }
                temp = Util.transform(m, temp.copy(Config.RGB_565, true),
                        imageWidth, imageHeight, true, Util.RECYCLE_INPUT);
            }

            Bitmap thumb = Bitmap.createBitmap(imageWidth, imageHeight,
                    Bitmap.Config.ARGB_8888);
            Canvas tempCanvas = new Canvas(thumb);

            Matrix mm = new Matrix();
            Paint pp = new Paint();
            pp.setFlags(Paint.ANTI_ALIAS_FLAG);

            if (temp != null) {
                tempCanvas.drawBitmap(temp, mm, pp);
            }
            Matrix matrix = new Matrix();
            Paint pPaint = new Paint();
            try {
                Bitmap resizeBitmap = Bitmap.createBitmap(thumb, 0, 0,
                        imageWidth, imageHeight, matrix, true);
                placeImage(resizeBitmap, c, pPaint, imageWidth, padding,
                        imageHeight, padding, offsetWidth, offsetHeight, i);
            } catch (NullPointerException ex) {
                android.util.Log
                        .w("NULL", "Why is null raised? thumb is null? "
                                + (thumb == null), ex);
            } finally {
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
    private boolean mPickIntentSelectPhotos;

    private boolean mPickIntentVideoType;

    private IImageList createImageList(int mediaTypes, String bucketId,
            ContentResolver cr) {
        IImageList list = ImageManager.makeImageList(cr,
                ImageManager.DataLocation.ALL, mediaTypes,
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
        byte[] data = null;
        if (bitmap != null) {
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
        switch(v.getId()) {
        case R.id.collage_back:
            this.finish();
            break;
        }
    }
    /**
     * Item is the underlying data for GalleryAdapter. It is passed from the
     * activity to the adapter.
     */
    class Item {
        public static final int TYPE_NONE = -1;
        public static final int TYPE_ALL_IMAGES = 0;
        public static final int TYPE_ALL_VIDEOS = 1;
        public static final int TYPE_CAMERA_IMAGES = 2;
        public static final int TYPE_CAMERA_VIDEOS = 3;
        public static final int TYPE_CAMERA_MEDIAS = 4;
        public static final int TYPE_NORMAL_FOLDERS = 5;
        private File mOldDir;
        private File mNewDir;
        public final int mType;
        public final String mBucketId;
        public final String mName;
        public final IImageList mImageList;
        public final int mCount;
        public final Uri mFirstImageUri; // could be null if the list is empty

        // The thumbnail bitmap is set by setThumbBitmap() later because we want
        // to let the user sees the folder icon as soon as possible (and
        // possibly
        // select them), then present more detailed information when we have it.
        public Bitmap mThumbBitmap; // the thumbnail bitmap for the image list

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

        public void setThumbBitmap(Bitmap thumbBitmap) {
            mThumbBitmap = thumbBitmap;
        }

        public boolean needsBucketId() {
            return mType >= TYPE_CAMERA_IMAGES;
        }

        public void launch(CollageImagePickerActivity activity) {

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

        public int getIncludeMediaTypes() {
            return convertItemTypeToIncludedMediaType(mType);
        }

        public int convertItemTypeToIncludedMediaType(int itemType) {
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
            if (mImageList == null || mImageList.getImageAt(0) == null) {
                Log.w("Item", "mImageList is empty");
                return null;
            }

            String filepath = mImageList.getImageAt(0).getDataPath();
            if (filepath == null) {
                Log.w("Item", "image has no path info, ignore");
                return null;
            }
            File imageFile = new File(filepath);
            if (!imageFile.exists()) {
                return null;
            }
            return new File(imageFile.getParentFile(), ".cover");
        }

        public void loadThumbnailFromFile() {
            Log.d("Item", "load: " + mBucketId + "," + mName);
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
                /* SPRD:  CID 109326 : RV: Bad use of return value (FB.RV_RETURN_VALUE_IGNORED_BAD_PRACTICE) @{ */
                if(!file.delete()){
                    return;
                }
                //file.delete();
                /* @} */
            }

            OutputStream os = null;
            try {
                os = new FileOutputStream(file);
                thumbnail.compress(CompressFormat.PNG, 90, os);
            } catch (Exception e) {
                Log.w("Item", "Fail cache thumbnail");
            } finally {
                Util.closeSilently(os);
            }
        }
    }

    class GalleryCollageAdapter extends BaseAdapter {
        ArrayList<Item> mItems = new ArrayList<Item>();
        LayoutInflater mInflater;
        private final String mImagePath;
        GalleryCollageAdapter(Activity act) {
            mInflater = act.getLayoutInflater();
            if (Build.isTelecomCloud()) {
                SharedPreferences pref = PreferenceManager
                        .getDefaultSharedPreferences(act);
                mImagePath = pref.getString("sf_pref_ucam_select_path_key",
                        CollageImagePickerActivity.TELECOM_CLOUD_PATH);
            } else {
                mImagePath = null;
            }
        }

        public void addItem(Item item) {
            mItems.add(item);
        }

        public void updateDisplay() {
            notifyDataSetChanged();
        }

        public void clear() {
            mItems.clear();
        }

        public int getCount() {
            return mItems.size();
        }

        public Object getItem(int position) {
            return null;
        }

        public String getItemmBucketId(int position) {
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

        public void removeItem(View v) {
            mItems.remove(v);
            notifyDataSetChanged();
        }

        public void removeItem(int index) {
            /**
             * BUG FIX:3086 BUG CAUSE:java.lang.ArrayIndexOutOfBoundsException
             * DATE:2013-03-06
             */
            if (index < mItems.size()) {
                mItems.remove(index);
            }
            notifyDataSetChanged();
        }

        public boolean isImageLockedIcon(int index) {
            IImageList list =  mItems.get(index).mImageList;
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

        public boolean isCloudIconExist() {
            for (Item item : mItems) {
                String filePath = item.mImageList.getImageAt(0).getDataPath();
                if (filePath != null) {
                    File file = new File(filePath);
                    String dir = file.getParentFile().getPath();
                    if (mImagePath != null && dir.startsWith(mImagePath)) {
                        return true;
                    }
                }
            }
            return false;
        }

        public View getView(final int position, View convertView,
                ViewGroup parent) {
            ViewHolder viewHolder = null;
            if (convertView == null) {
                convertView = mInflater
                        .inflate(R.layout.image_album_item, null);
                viewHolder = new ViewHolder();
                viewHolder.titleView = (TextView) convertView
                        .findViewById(R.id.title);
                viewHolder.countView = (TextView) convertView
                        .findViewById(R.id.count);
                viewHolder.thumbView = (ImageView) convertView
                        .findViewById(R.id.thumbnail);
                viewHolder.cloudIcon = (ImageView) convertView
                        .findViewById(R.id.cloud_icon);
                viewHolder.lockIcon = (ImageView) convertView.findViewById(R.id.lock_icon);
                convertView.setTag(viewHolder);
            } else {
                viewHolder = (ViewHolder) convertView.getTag();
            }
            Item item = mItems.get(position);
            if (Build.isTelecomCloud()) {
                String filePath = item.mImageList.getImageAt(0).getDataPath();
                if (filePath != null) {
                    File file = new File(filePath);
                    String dir = file.getParentFile().getPath();
                    if (mImagePath != null && dir.startsWith(mImagePath)) {
                        viewHolder.cloudIcon.setVisibility(View.VISIBLE);
                    } else {
                        viewHolder.cloudIcon.setVisibility(View.GONE);
                    }
                }
            }
            if(isImageLockedIcon(position)) {
                viewHolder.lockIcon.setVisibility(View.VISIBLE);
            } else {
                viewHolder.lockIcon.setVisibility(View.GONE);
            }
            viewHolder.thumbView.setImageBitmap(item.mThumbBitmap);
            viewHolder.countView.setText(" (" + item.mCount + ")");
            viewHolder.titleView.setText(item.mName);

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


    private class AsyncImageLoader extends Thread {

        AsyncImageLoader() {
            super("AsyncImageLoader");
            setDaemon(true);
        }

        java.util.Queue<WeakReference<ImageView>> mImages = new LinkedList<WeakReference<ImageView>>();

        public synchronized void addImageView(ImageView img) {
            if (!mImages.contains(img)) {
                mImages.offer(new WeakReference<ImageView>(img));
                notify();
            }
        }

        private synchronized ImageView nextImageView() {
            WeakReference<ImageView> img = mImages.poll();
            while (img == null && !mPausing) {
                try {
                    wait();
                } catch (Exception e) {
                }
                img = mImages.poll();
            }
            return img != null ? img.get() : null;
        }

        public void run() {
            while (!mPausing) {
                final ImageView imageview = nextImageView();
                if (imageview != null) {
                    final IImage img = (IImage) imageview.getTag();
                    if (img == null)
                        continue;
                    Object[] objects = img.miniThumbBitmap();
                    Bitmap bitmap = null;
                    if(objects != null && objects.length > 1) {
                        bitmap = (Bitmap)objects[1];
                    }
                    final Bitmap bm = bitmap;
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            imageview.setImageBitmap(bm);
                        }
                    });
                }
            }
            Log.d(TAG, "Exit AsyncImageLoader");
            mImages.clear();
            mImageLoader = null;
        }
    }
    private Context mContext;
    private ThumbAdapter mThumbAdapter;

    class ThumbAdapter extends BaseAdapter {
        public ThumbAdapter(Context context) {
            mContext = context;
        }

        public int getCount() {
            return (mThumbImageList != null) ? mThumbImageList.size() : 0;
        }

        public IImage getItem(int position) {
            return mThumbImageList != null ? mThumbImageList.get(position) : null;
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            View view;
            if (convertView != null) {
                view = (View) convertView;
            } else {
                view = View.inflate(mContext,
                        R.layout.collage_seleced_thumbnail, null);
            }
            if(mThumbImageList == null|| mThumbImageList.size()==0){
                return null;
            }
            ImageView image = (ImageView) view.findViewById(R.id.thumbnail_puzz_item);
            IImage item = getItem(position);
            /* SPRD: CID 108988 : Dereference null return value (NULL_RETURNS) @{ */
            Bitmap b = null;
            if(item != null) {
                b = item.getCachedThumbBitmap();
            }
            // Bitmap b = item.getCachedThumbBitmap();
            /* @} */
            /* FIX BUG : 4462
             * BUG COMMENT : get bitmap from file again if the current bitmap is null
             * DATE : 2013-07-16
             */
            // CID 111179 : Dereference after null check (FORWARD_NULL)
            if(b == null && item != null) {
                Object[] object = item.miniThumbBitmap();
                if(object != null && object.length > 1) {
                    b = (Bitmap) object[1];
                }
            }
            if (b != null) {
                image.setImageBitmap(b);
            } else {
                image.setTag(item);
                if (mImageLoader == null) {
                    mImageLoader = new AsyncImageLoader();
                    mImageLoader.start();
                }
                mImageLoader.addImageView(image);
            }
            return view;
        }
    }
    private Bitmap mMissingImageThumbnailBitmap;
    private Bitmap mMissingVideoThumbnailBitmap;
    private final Rect mSrcRect = new Rect();
    private final Rect mDstRect = new Rect();
    private final Paint mPaint = new Paint(Paint.FILTER_BITMAP_FLAG);;
    public Bitmap getErrorBitmap(IImage image) {
        if (ImageManager.isImage(image)) {
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

    @Override
    public void drawImage(Canvas canvas, IImage image, Bitmap b, int xPos, int yPos, int w, int h) {
        if(b == null){
            b = getErrorBitmap(image);
        }
//        if (b != null) {
            // if the image is close to the target size then crop,
            // otherwise scale both the bitmap and the view should be
            // square but I suppose that could change in the future.
            int bw = b.getWidth();
            int bh = b.getHeight();

            int deltaW = bw - w;
            int deltaH = bh - h;
            Bitmap old = b;
            Log.d("bitmap  ",deltaW+","+deltaH);
            b = b.copy(Config.ARGB_8888, true);
            if (deltaW >= 0 && deltaW < 10 && deltaH >= 0 && deltaH < 10) {
                int halfDeltaW = deltaW / 2;
                int halfDeltaH = deltaH / 2;
                mSrcRect.set(0 + halfDeltaW, 0 + halfDeltaH, bw - halfDeltaW, bh - halfDeltaH);
                mDstRect.set(xPos, yPos, xPos + w, yPos + h);
                canvas.drawBitmap(b, mSrcRect, mDstRect, null);
            } else {
                mSrcRect.set(0, 0, bw, bh);
                mDstRect.set(xPos, yPos, xPos + w, yPos + h);
                canvas.drawBitmap(b, mSrcRect, mDstRect, mPaint);
            }
            if (old != b) {
                Util.recyleBitmap(b);
            }


    }
    @Override
    public void onImageTapped(int index) {
        if (mThumbImageList.size() >= MAX_NUMB) {
            Toast.makeText(mContext,
                    R.string.text_collage_max_select_notice,
                    Toast.LENGTH_SHORT).show();
        } else {
            /*
             * BUG FIX: 5325 BUG
             * COMMENT: if exist Damaged image ,so we judged
             * it by this BUg DATE:2013-11-26
             */
            String path = null;
            IImage image = mAllImageList.getImageAt(index);
            if(image != null){
                path= image.getDataPath();
            }
            if(path == null){
                Toast.makeText(mContext, R.string.text_image_damage, Toast.LENGTH_LONG).show();
                return;
            }
//            Bitmap bitmap = ThumbnailUtils.createImageThumbnail(path, Images.Thumbnails.MINI_KIND);
            Bitmap bitmap = (Bitmap)image.miniThumbBitmap()[1];
            if (bitmap == null) {
                Toast.makeText(mContext, R.string.text_image_damage, Toast.LENGTH_LONG).show();
                return;
            } else if (!bitmap.isRecycled()) {
                bitmap.recycle();
                bitmap = null;
            }
            mThumbImageList.add(mAllImageList.getImageAt(index));
            showPhotoNmber();
            updateThumbGridviewUI(mThumbGridView, mThumbAdapter.getCount(), 5);
            mThumbAdapter.notifyDataSetChanged();
        }
    }
    @Override
    public void drawPlayIcon(Canvas canvas, IImage image, Bitmap b, int xPos, int yPos, int w, int h) {}
    @Override
    public void drawDecoration(Canvas canvas, IImage image, int xPos, int yPos, int w, int h) {}
    @Override
    public void drawTitleDecoration(Canvas canvas, TimeModeItem item, int titleRow, int xPos,
            int yPos, int w, int h) {
    }
    @Override
    public boolean needsDecoration() {
        return false;
    }
    @Override
    public void onTitleTapped(int row) {
    }

    private static final float INVALID_POSITION = -1f;
    private float mScrollPosition = INVALID_POSITION;
    // The index of the first picture in GridViewSpecial.
    private int mSelectedIndex = GridViewSpecial.INDEX_NONE;
    private boolean mIsLockedAblum;
    @Override
    public void onLayoutComplete(boolean changed) {
        /*
         * BUG FIX: 5778
         * BUG COMMENT:GridViewSpecial should scroll to selected position after long Pressed
         * BUg DATE:2014-01-08
         */
//        mSelectedIndex= GridViewSpecial.INDEX_NONE;
//        mGvs.setSelectedIndex(mSelectedIndex);
/*        if (mScrollPosition == INVALID_POSITION) {
            mGvs.scrollToImage(0);
        } else {
            mGvs.scrollTo(mScrollPosition);
        }*/
//       mGvs.scrollToImage(0);
    }
    @Override
    public void onScroll(float scrollPosition) {
        mScrollPosition = scrollPosition;
    }
    @Override
    public void onImageLongPressed(int index) {
    }
}
