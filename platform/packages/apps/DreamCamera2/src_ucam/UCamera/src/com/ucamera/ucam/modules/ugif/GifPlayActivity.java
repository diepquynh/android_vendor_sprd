/*
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ucam.modules.ugif;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import com.android.camera2.R;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.ucamera.ucam.modules.utils.UiUtils;
import com.ucamera.ucam.modules.utils.Utils;

import android.app.ActivityGroup;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.ContentUris;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.RectF;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.provider.MediaStore;
import android.provider.MediaStore.Images;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;
import com.ucamera.ugallery.gif.GifDecoder;

public class GifPlayActivity extends ActivityGroup implements View.OnClickListener{
    private GifView mShowImageView;
    // private TextView mShowGifSize;
    private ImageView mForwardButton;
    private ImageView mNextButton;
    private ImageView mShareButton;
    private RectF mRectF;
    private int mCurrentIndex = -1;
    private Bitmap[] bmBitmap = null;
    private int mDelayTime;
    private int mTotalGifNum;
    private RelativeLayout mTopbarRelativeLayout;
    private LinearLayout mBottomRelativeLayout;
    // private RelativeLayout mBottombaRelativeLayout;
    private com.ucamera.ugallery.gif.GifDecoder mGifDecoder;
    private Context mContext;
    private boolean mIsFromOutSide = true; // true means current gif played is
                                           // from other application.
    private Handler mHandler = new Handler();
    private List<GifItem> mGifItemList = new ArrayList<GifItem>();
    private GifItem mCurrentItem;
    private TextView mIndexOfCurGifTextView;
    public float mLastXposition = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        /*SPRD:fix bug527736 Some Activity about UCamera lacks method of checkpermission@{ */
        if (!UCamUtill.checkPermissions(this)) {
            finish();
            return;
        }
        /* }@ */
        mContext = this;
        setContentView(R.layout.gif_mode_play_image);
        initView();
        createGifItemList();
        if (mCurrentItem != null && intBitmapData(mCurrentItem.getPath())) {
            playGif();
        } else {
            //finish(); // initial failed ,finish play activity.
            exitGifPlay();
        }

    }

    private void createGifItemList() {
        Bundle bundle = getIntent().getExtras();
        mIsFromOutSide = bundle.getBoolean("fromOutSide", true);
        if(!mIsFromOutSide && getIntent().getData() == null) {
            return;
        }
        Cursor cursor = GifUtils.getGifListFromDB(this);

        /* SPRD: CID 109096 : Resource leak (RESOURCE_LEAK) @{ */
        if(cursor != null) {
            cursor.moveToLast();
            mTotalGifNum = cursor.getCount();
            if(mTotalGifNum <= 0) {
                cursor.close();
                return;
            }
            if(mGifItemList.size() > 0) mGifItemList.clear();
            do{
                GifItem item = new GifItem();
                item.setId(cursor.getLong(0));
                String url = Images.Media.EXTERNAL_CONTENT_URI.toString() + "/" + item.getId();
                item.setUri(Uri.parse(url));
                item.setPath(cursor.getString(1));
                item.setSize(cursor.getLong(2));
                mGifItemList.add(item);
            } while(cursor.moveToPrevious());

            cursor.close();
        }

        /**
        if(cursor != null) {
            cursor.moveToLast();
            mTotalGifNum = cursor.getCount();
            if(mTotalGifNum <= 0) return;
            if(mGifItemList.size() > 0) mGifItemList.clear();
            do{
                GifItem item = new GifItem();
                item.setId(cursor.getLong(0));
                String url = Images.Media.EXTERNAL_CONTENT_URI.toString() + "/" + item.getId();
                item.setUri(Uri.parse(url));
                item.setPath(cursor.getString(1));
                item.setSize(cursor.getLong(2));
                mGifItemList.add(item);
            } while(cursor.moveToPrevious());
        }
         */
        /* @} */

        Uri currentUri = null;
        if (mIsFromOutSide ) {
            currentUri = getIntent().getExtras().getParcelable(Intent.EXTRA_STREAM);
        } else {
            currentUri = getIntent().getData();
        }

        if(currentUri != null) {
            mCurrentItem = getCurrentGifItem(currentUri);
        }else {
            if(mGifItemList.size() > 0) {
                mCurrentItem = mGifItemList.get(0);
            }else {
                return;
            }
        }

        if(mCurrentItem == null) {
            mCurrentItem = new GifItem();
            mCurrentItem.setId(0);
            mCurrentItem.setUri(currentUri);
            //fix coverity issue 108956
            if (currentUri != null) {
                if (currentUri.getScheme().equals("content")){
                    mCurrentItem.setPath(getPhysicalPathFromURI(currentUri));
                }else {
                    mCurrentItem.setPath(currentUri.toString());
                }
            }
        }
        updateIndexOfGif();
    }

    private GifItem getCurrentGifItem(Uri uri) {
        if(uri == null) return null;
        for(int i=0; i < mGifItemList.size(); i++) {
            if(mGifItemList.get(i).getUri().equals(uri) ||
                    mGifItemList.get(i).getPath().equals(uri.toString())) {
                mCurrentIndex = i;
                return mGifItemList.get(i);
            }
        }
        return null;

    }

    private void updateIndexOfGif() {
        mIndexOfCurGifTextView.setText(mCurrentIndex + 1 + " / " + mTotalGifNum);
    }

    /**
     * get the physical path from uri(eg:content://) by query DB.
     *
     * @param contentUri which needed to convert to physical path.
     * @eturn String the physical path.
     */
    public String getPhysicalPathFromURI(Uri contentUri) {
        String searchColumn = MediaStore.Images.Media.DATA;
        String filePath = null;
        String[] project = {
            searchColumn
        };
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

    public void initView() {
        mShowImageView = (GifView) findViewById(R.id.gif_play_view);
        //mShowImageView.setOnClickListener(this);
        mIndexOfCurGifTextView = (TextView) findViewById(R.id.index_of_current_text);
//        findViewById(R.id.btn_gif_share).setOnClickListener(this);
        findViewById(R.id.btn_gif_delete).setOnClickListener(this);
        findViewById(R.id.nav_to_gallery).setOnClickListener(this);
//        if(!ShareUtils.SNS_SHARE_IS_ON) {
//            mShareButton.setVisibility(View.INVISIBLE);
//        }
        mTopbarRelativeLayout = (RelativeLayout) findViewById(R.id.gif_play_top_bar);
        mBottomRelativeLayout = (LinearLayout) findViewById(R.id.gif_play_bottom_bar);
        scheduleControlBar();
    }

    private void playLastGif() {
        if (mCurrentIndex > 0) {
            mCurrentIndex--;
            mCurrentItem = mGifItemList.get(mCurrentIndex);
        } else {
            toast(getResources().getString(R.string.gif_play_first_gif_tips));
            return;
        }
        playCurrentGif();
    }

    private void playNextGif() {
        if (mCurrentIndex < mTotalGifNum - 1) {
            mCurrentIndex++;
            mCurrentItem = mGifItemList.get(mCurrentIndex);
        } else {
            toast(getResources().getString(R.string.gif_play_last_gif_tips));
            return;
        }
        playCurrentGif();
    }

    private void playCurrentGif() {
        showControlBar();
        updateIndexOfGif();
        /*
         * FIX BUG: 6558 BUG CAUSE:use recyled bitmap FIX COMMENT:stop
         * playing and then recyle Date: 2011-12-21
         */
        if (mGifDecoder != null) {
            if (mShowImageView.isPlaying()) {
                mShowImageView.stop();
            }
            mGifDecoder.recycleBitmaps();
        }
        if (intBitmapData(mCurrentItem.getPath())) {
            playGif();
        } else {
            finish();
        }
    }

    public boolean intBitmapData(String stringPath) {
        // CID 109165 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        // DisplayMetrics dm = new DisplayMetrics();
        DisplayMetrics dm = this.getResources().getDisplayMetrics();
        mGifDecoder = null;
        mGifDecoder = new GifDecoder();
        try{
            mGifDecoder.readQPhoneContent(this.getContentResolver().openInputStream(mCurrentItem.mUri));
        } catch(Exception e) {
            return false;
        }
        int n = mGifDecoder.getQPhoneFrameCount();
        if (n <= 0) { // get frame error
            Toast.makeText(this, R.string.edit_open_error_image, Toast.LENGTH_SHORT).show();
            return false;
        }
        mDelayTime = mGifDecoder.getQPhoneDelay(1);
        bmBitmap = new Bitmap[n];
        for (int i = 0; i < n; i++) {
            bmBitmap[i] = mGifDecoder.getQPhoneFrame(i);
        }
        mGifDecoder.clear();
        int width = bmBitmap[0].getWidth();
        int height = bmBitmap[0].getHeight();

        double scale = Math.min(dm.widthPixels / (float)width, dm.heightPixels /(float)height);
        scale = scale > 4 ? 2: 1;
        float top = (dm.heightPixels-(float)(height*scale)) / 2;
        float left = (dm.widthPixels-(float)(width*scale)) / 2;
        mRectF = new RectF(left, top, dm.widthPixels-left, dm.heightPixels-top);
        return true;
    }

    public void playGif() {
        mShowImageView.init(bmBitmap, true, mRectF);
        if (mDelayTime > 0) {
            mShowImageView.setSpeed(mDelayTime);
        }
        mShowImageView.start();
    }

    public void recyleCurrentBitmaps() {
        /**
         * FIX BUG: 2836
         * BUG CAUSE: java.lang.NullPointerException
         * 2013-02-21
         */
        Utils.recycleBitmaps(bmBitmap);
    }

    @Override
    protected void onStop() {
        /**
         * FIX BUG: 1638 BUG CAUSE: Finish the current Activity and recycle the
         * current bitmaps; FIX COMMENT:Do not finish the current Activity and
         * recycle the current bitmaps when running onstop method; Date:
         * 2012-10-08
         */
        // recyleCurrentBitmaps();
        // finish();
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        /**
         * FIX BUG: 1638 BUG CAUSE: Do not finish the current Activity and
         * recycle the current bitmaps when running onstop method; FIX
         * COMMENT:Recycle the current bitmaps when running onDestroy method;
         * Date: 2012-10-08
         */
        recyleCurrentBitmaps();
        if(mShowImageView != null) {
            mShowImageView.stop();
        }
        super.onDestroy();
    }

    private void toast(String message) {
        Toast.makeText(mContext, message, Toast.LENGTH_SHORT).show();
    }

    private void scheduleControlBar() {
        mHandler.removeCallbacks(mHideControlBarRunnable);
        mHandler.postDelayed(mHideControlBarRunnable, 5000);
    }

    private final Runnable mHideControlBarRunnable = new Runnable() {
        @Override
        public void run() {
            hideControlBar();
        }
    };

    private void hideControlBar() {
        mTopbarRelativeLayout.setVisibility(View.INVISIBLE);
        mBottomRelativeLayout.setVisibility(View.INVISIBLE);
    }

    private void showControlBar() {
        mTopbarRelativeLayout.setVisibility(View.VISIBLE);
        mBottomRelativeLayout.setVisibility(View.VISIBLE);
        scheduleControlBar();
    }

    @Override
    protected void onPause() {
        super.onPause();
        // StatApi.onPause(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        showControlBar();
        // StatApi.onResume(this);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_MENU) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        final int action = event.getAction();
        switch(action) {
        case MotionEvent.ACTION_DOWN:
            mLastXposition = event.getRawX();
            break;
        case MotionEvent.ACTION_MOVE:
            break;
        case MotionEvent.ACTION_UP:
        case MotionEvent.ACTION_CANCEL:
            int deltaX = (int)(mLastXposition - event.getRawX());
            if(deltaX > UiUtils.screenWidth() / 4) {
                playNextGif();
            }else if(deltaX < -UiUtils.screenWidth() / 4) {
                playLastGif();
            }else {
                if (mTopbarRelativeLayout.getVisibility() != View.VISIBLE) {
                    showControlBar();
                } else {
                    hideControlBar();
                }
            }
            break;
        }
        return super.onTouchEvent(event);
    }

    @Override
    public void onClick(View v) {
        switch(v.getId()){
        case R.id.nav_to_gallery:
            exitGifPlay();
            break;
//        case R.id.btn_gif_share:
            /**
             * FIX BUG: 6482 BUG CAUSE:share gif to uphoto FIX
             * COMMENT:filter uphoto and gif edit at gif play activity Date:
             * 2011-12-08
             */
//            ShareUtils.shareImage(GifPlayActivity.this, mCurrentItem.getUri());
//            break;
        case R.id.btn_gif_delete:
            android.content.DialogInterface.OnClickListener listener = new android.content.DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    switch (which) {
                        case DialogInterface.BUTTON_POSITIVE:
                            if (mCurrentItem.getUri() != null) {
                                File f = new File(mCurrentItem.getPath());
                                /* CID 109182 : RV: Bad use of return value (FB.RV_RETURN_VALUE_IGNORED_BAD_PRACTICE) @{ */
                                if(!f.delete()){
                                    return;
                                }
                                // f.delete();
                                /* @} */
                                getContentResolver().delete(mCurrentItem.getUri(), null, null);
                                mGifItemList.remove(mCurrentItem);
                                mTotalGifNum = mGifItemList.size();
                                if(mTotalGifNum < 1) {
                                    exitGifPlay();
                                    return;
                                }
                                /**
                                 * FIX BUG: 5693
                                 * COMMENT:the picture number not show out of Ugllery
                                 * 2013-12-25
                                 */
                                if(mCurrentIndex < 0){
                                    mCurrentIndex = 0;
                                }
                                else if(mCurrentIndex >= mTotalGifNum) {
                                    mCurrentIndex = mTotalGifNum - 1;
                                }
                                mCurrentItem = mGifItemList.get(mCurrentIndex);
                                playCurrentGif();
                                updateIndexOfGif();
                            }
                            break;
                        default:
                            break;
                    }
                }
            };
            Dialog dialog = new AlertDialog.Builder(this)
                .setIcon(R.drawable.ic_dialog_alert_gif)
                .setTitle(getString(R.string.text_delete_image_title))
                .setMessage(getString(R.string.text_delete_single_message))
                .setCancelable(false)
                .setPositiveButton(R.string.picture_delete_ok, listener)
                .setNegativeButton(R.string.picture_delete_cancel, listener)
                .create();
            dialog.show();

            break;
        }
    }

    private void exitGifPlay() {
        startActivity(new Intent(this, GifBrowser.class));
        finish();
    }

    class GifItem {
        private long mId;
        private Uri mUri;
        private String mPath;
        private long mSize;

        private long getId() {
            return mId;
        }

        private void setId(long id) {
            mId = id;
        }

        private Uri getUri() {
            return mUri;
        }

        private void setUri(Uri uri) {
            mUri = uri;
        }

        private String getPath() {
            return mPath;
        }

        private void setPath(String path) {
            mPath = path;
        }

        private long getSize() {
            return mSize;
        }

        private void setSize(long size) {
            mSize = size;
        }
    }
}
