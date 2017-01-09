/*Created by Spreadst for freeze_display*/
package com.sprd.camera.freeze;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;

import com.android.camera.CameraActivity;
import com.android.camera.Exif;
import com.android.camera2.R;
import com.android.camera.Thumbnail;
import com.android.camera.util.CameraUtil;
import com.sprd.camera.freeze.FreezeRotateLayout;
import com.android.camera.ui.RotateImageView;
import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.provider.MediaStore.Images.ImageColumns;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.util.*;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;

public class FreezeFrameDisplayView extends RelativeLayout {

    private static final String TAG = "CAM_CameraFreezeFrameDisplayView";

    public static interface ProxyFreezeFrameClick {
        // The method runs in main thread
        public void proxyDoneClicked();
        // The method runs in main thread
        public void proxyFinishDeleted(Uri uri);
        // The method runs in main thread
        public void proxyRestartViews();
        // SPRD: bug 251198
        public void proxyRetakeClicked();
    }

    // String resource
    private final int VAL_NOTICE_FAILED  = R.string.notice_progress_text_failed;
    private final int VAL_NOTICE_LOADING = R.string.notice_progress_text_loading;

    private Bitmap mResource;           // hold bitmap object
    private ImageView mImage;           // R.id.image_freeze_frame_display
    private TextView mNoticeView;       // R.id.tv_progress_notice
    private ProgressBar mProgressBar;   // R.id.pb_progress_notice
    private FrameLayout mProgressPanel; // R.id.fl_progress_panel
    private FreezeRotateLayout mRotatableView;// R.id.view_rotate_freeze_frame_display
    private RelativeLayout mRelativeLayoutView;// R.id.view_rotate_freeze_frame_display

    private RotateImageView mDone;      // R.id.btn_freeze_frame_done
    private RotateImageView mCancel;    // R.id.btn_feeze_frame_cancel
    private ControlOnClickListener mClickListener;
    // SPRD: bug 251198
    private View mReviewRetakeButton;

    private ViewHandler sHandler;       // main | work thread communicate
    private AsyncLoadResource mLoadTask;// load resource task in work thread
    private AsyncDeleteResource mDeleteTask; // delete resource taks in work thread
    // notice camera activity
    private ProxyFreezeFrameClick mListener;
    //SPRD: bug 251198
    private byte [] mJpagByteArry;
    private boolean mIsImageCaptureIntent;
    private boolean mIsAutoCapture = false;
    private CameraActivity mActivity;
    // default construct
    public FreezeFrameDisplayView(Context ctx, AttributeSet attrs) {
        super(ctx, attrs);
        // the view default visibility is gone
        setVisibility(View.GONE);
        // initialize objects
        mLoadTask = new AsyncLoadResource();
        mDeleteTask = new AsyncDeleteResource();
        sHandler = new ViewHandler();
    }

    public void initial(CameraActivity activity) {
        mActivity = activity;
        setOnTouchListener(new OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                return true;
            }
        });
    }
    @Override
    protected void onFinishInflate() {
        // initialize control panel
        mClickListener = new ControlOnClickListener();
        mDone = (RotateImageView) findViewById(R.id.btn_freeze_frame_done);
        mCancel = (RotateImageView) findViewById(R.id.btn_feeze_frame_cancel);
        // SPRD: bug 251198
        mReviewRetakeButton = findViewById(R.id.btn_feeze_frame_retake);
        mDone.setOnClickListener(mClickListener); // default just initialize mDone event
        // SPRD: bug 251198
        mReviewRetakeButton.setOnClickListener(mClickListener); // default just initialize mDone event
        // initialize image view
        mImage = (ImageView) findViewById(R.id.image_freeze_frame_display);
        // initialize progress panel and notice text and progress bar
        mProgressPanel = (FrameLayout) findViewById(R.id.fl_progress_panel);
        mProgressBar = (ProgressBar) mProgressPanel.findViewById(R.id.pb_progress_notice);
        mNoticeView = ((TextView) mProgressPanel.findViewById(R.id.tv_progress_notice));
        // initialize rotate view group
        mRotatableView = (FreezeRotateLayout) findViewById(R.id.view_rotate_freeze_frame_display);
        mRelativeLayoutView = (RelativeLayout) findViewById(R.id.control_freeze_frame_display);
        mRotatableView.onFinishInflate();
    }

    public void setListener(ProxyFreezeFrameClick listener) {
        mListener = listener;
    }


    private void updateViews(boolean into, boolean isFreez) {
        // anyway, mCancel is disable & haven't listener
        //@{ SPRD: bug 251198
        if(mIsImageCaptureIntent) {
           FreezeViewUtil.fadeIn(mReviewRetakeButton);
        } //@}
        mCancel.setEnabled(false);                      // mCancel default is disable
        mDone.setEnabled(false);
        mCancel.setOnClickListener(null);               // mCancel default haven't listener
        mDone.setOnClickListener(null);
        Log.d(TAG, "if is freez not dismiss Dialog, isFreez = " + isFreez);
/*        if (mClickListener != null && !isFreez)                     // we must runs "proxyDismissDialog()"
           // mClickListener.proxyDismissDialog();        // Cause dialog visible default value is View.VISIBLE
*/        if (into) {
            mImage.setVisibility(View.GONE);            // image view default visibility is gone
            mNoticeView.setText(VAL_NOTICE_LOADING);    // default string is "loading"
            mProgressBar.setVisibility(View.VISIBLE);   // progress bar default visibility is visible
            mProgressPanel.setVisibility(View.VISIBLE); // progress panel default visibility is visible
        } else {
            boolean failed = mLoadTask.failed();
            mImage.setVisibility(failed ? View.GONE : View.VISIBLE);
            mProgressBar.setVisibility(View.GONE);
            mProgressPanel.setVisibility(failed ? View.VISIBLE : View.GONE);
            // if load resource failed, then we must reset load thread state to UNKNOW or FINISHED
            mLoadTask.proxySyncReset(failed ?
                AsyncLoadResource.VAL_STATE_FINISHED : AsyncLoadResource.VAL_STATE_UNKNOW);
        }
    }

    // Must runs in main thread
    public void proxyFadeIn(boolean isIntent, boolean isFreez) {
        mIsImageCaptureIntent = isIntent;
        updateViews(true, isFreez);
        setVisibility(View.VISIBLE);
    }

    // Must runs in main thread
    public void proxyFadeOut() {
        updateViews(false, false);
        setVisibility(View.GONE);
    }

    // Anyway threads
    public boolean displayed() {
        return (View.VISIBLE == getVisibility());
    }

    // Runs in work thread
    public void runLoadResource(final Uri uri) {
        mLoadTask.run(uri);
    }
    //@{ SPRD: bug 251198
    public void runLoadResource(final byte [] jpagByte, boolean auto) {
        mIsAutoCapture = auto;
        mLoadTask.run(jpagByte);
    } //@}

    private class ControlOnClickListener implements View.OnClickListener {

        private Runnable sRunnableDone;
        // default construct
        private ControlOnClickListener() {
            sRunnableDone = new Runnable() {
                @Override
                public void run() {
                    Log.d(TAG, "execute delete picture work thread ... ...");
                    mListener.proxyRestartViews();
                    mDeleteTask.execute();
                }
            };
        }
        @Override
        public void onClick(View v) {
            switch (v.getId()) {
                case R.id.btn_freeze_frame_done:
                    mListener.proxyDoneClicked();
                    break;
                case R.id.btn_feeze_frame_cancel:
                    Log.d(TAG,"on cancle click");
                    //@{ SPRD: bug 251198
                    if(mIsImageCaptureIntent) {
                        sRunnableDone.run();
                        break;
                    } //@}
                    showAlertDialog();
                    break;
                //@{ SPRD: bug 251198
                case R.id.btn_feeze_frame_retake:
                    if(mIsImageCaptureIntent) {
                        FreezeViewUtil.fadeIn(mReviewRetakeButton);
                        // SPRD: remove the picture when retake a new one
                        mDeleteTask.execute();
                        mListener.proxyRetakeClicked();
                        break;
                    } //@}
            }
        }
        public void showAlertDialog() {
            AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
            final AlertDialog alertDialog = builder.create();
            Context ctx = getContext();
            builder.setMessage(ctx.getString(R.string.dialog_freeze_frame_confirm_delete_text));
            builder.setPositiveButton(ctx.getString(android.R.string.ok), new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                 // TODO Auto-generated method stub
                    sRunnableDone.run();
                }
               });
            builder.setNegativeButton(ctx.getString(android.R.string.cancel), new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                 // TODO Auto-generated method stub
                    alertDialog.dismiss();
                }
               });
            builder.show();
        }
    }

    private class AsyncDeleteResource implements Runnable {

        /*package*/ static final int VAL_STATE_DELETE = 11;
        // mutex lock object
        /*package*/ final Object mMutexLock = new Object();
        // _data column
        private String[] mColumns = new String[] { ImageColumns.DATA };

        // default construct
        private AsyncDeleteResource() { }

        public void execute() {
            Thread t = new Thread(this);
            t.start();
        }

        @Override
        public void run() {
            synchronized (mMutexLock) {
                Uri delUri = mLoadTask.syncGetUri();
                boolean result = false;
                Log.d(TAG, "delete picture uri = " + delUri);
                if (delUri != null) {
                    ContentResolver resolver = getContext().getContentResolver();
                    // search item from database
                    Cursor cursor = null;
                    String path = null;
                    try {
                        cursor = resolver.query(delUri, mColumns, null, null, null);
                        if (cursor != null && cursor.moveToFirst()) {
                            path = cursor.getString(0);
                        }
                    } catch (Exception e) {
                        Log.d(TAG, "serach data from database failed, PLS ignore. URI = " + delUri, e);
                    } finally {
                        if (cursor != null) {
                            cursor.close();
                        }
                    }
                    Log.d(TAG, "delete file path = " + path);

                    // delete database data by delUri
                    if (result = (path != null)) {
                        try {
                            resolver.delete(delUri, null, null);
                        } catch (Exception e) {
                            Log.d(TAG, "delete data from database failed, PLS ignore. URI = " + delUri, e);
                        }
                    }

                    // delete file
                    if (result = (path != null)) {
                        File d = new File(path);
                        if (d.exists() && d.isFile() && !d.isHidden()) {
                            result = d.delete();
                        }
                    }
                    Log.i(TAG, "delete picture success = " + result);
                }
                if (result) {
                    Message msg = sHandler.obtainMessage(VAL_STATE_DELETE, delUri);
                    sHandler.sendMessage(msg);
                }

            }
        }
    }

    private class ViewHandler extends Handler {

        // default construct
        private ViewHandler() { }

        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            int what = msg.what;

            switch (what) {
                case AsyncDeleteResource.VAL_STATE_DELETE:
                    synchronized (mDeleteTask.mMutexLock) {
                        Uri delUri = ((Uri) msg.obj);
                        mListener.proxyFinishDeleted(delUri);
                        break;
                    }
                case AsyncLoadResource.VAL_STATE_FAILED:
                    synchronized (mLoadTask.mStateLock) {
                        if (mImage != null) mImage.setImageBitmap(null);
                        if (mResource != null && !mResource.isRecycled()) {
                            mResource.recycle();
                            mResource = null;
                        }
                        if (mNoticeView != null) mNoticeView.setText(VAL_NOTICE_FAILED);
                        if (mCancel != null) {
                            mCancel.setEnabled(false);
                            mCancel.setOnClickListener(null);
                        }
                        if (mDone != null) {
                            mDone.setEnabled(true);
                            mDone.setOnClickListener(mClickListener);
                        }
                        resetNotice(false); // load failed
                        break;
                    }
                case AsyncLoadResource.VAL_STATE_UNKNOW:
                case AsyncLoadResource.VAL_STATE_FINISHED:
                    synchronized (mLoadTask.mStateLock) {
                        /* SPRD: Fix bug 554334 trying to use a recycled bitmap @{ */
                        Bitmap tmpBitmap = (Bitmap)msg.obj;
                        if (mResource == null) {
                            mResource = tmpBitmap;
                        } else if (mResource != tmpBitmap) {
                            mResource.recycle();
                            mResource = tmpBitmap;
                        }
                        /* @} */

                        if (mResource != null && mImage != null) {
                            mImage.setImageBitmap(mResource);
                            if (mCancel != null) {
                                mCancel.setEnabled(true);
                                mCancel.setOnClickListener(mClickListener);
                            }
                            if (mDone != null) {
                                mDone.setEnabled(true);
                                mDone.setOnClickListener(mClickListener);
                            }
                        }
                        resetNotice(true); // load success
                        break;
                    }
            }
        }

        private void resetNotice(boolean success) {
            if (View.VISIBLE == getVisibility()) {
                if (success) {
                    mProgressPanel.setVisibility(View.GONE);
                    mImage.setVisibility(View.VISIBLE);
                    if (mIsAutoCapture) {
                        mListener.proxyDoneClicked();
                    }
                } else {
                    mImage.setVisibility(View.GONE);
                    mProgressBar.setVisibility(View.GONE);
                    mProgressPanel.setVisibility(View.VISIBLE);
                }
            }
        }
    }

    // use work thread loading URI resource convert to bitmap
    private class AsyncLoadResource implements Runnable {

        // URI from main thread
        private Uri mUri;
        // work thread state
        private int mState;
        // mutex lock object
        /*package*/ final Object mMutexLock = new Object();
        // mState lock
        /*package*/ final Object mStateLock = new Object();
        // default bitmap compress ratio
        private final int mCompressionRatio = 4;
        private final int mBuffSize = 1024;

        /*package*/ static final int VAL_STATE_UNKNOW   = 0;
        /*package*/ static final int VAL_STATE_FAILED   = 1;
        /*package*/ static final int VAL_STATE_RUNNING  = 2;
        /*package*/ static final int VAL_STATE_FINISHED = 3;

        // default construct
        private AsyncLoadResource() { }

        // Must runs in main thread
        /*package*/ void proxySyncReset(int state) {
            syncReset(state);
        }

        /*package*/ Uri syncGetUri() {
            synchronized (mMutexLock) {
                return mUri;
            }
        }

        /*package*/ boolean failed() {
            synchronized (mStateLock) {
                return (VAL_STATE_FAILED == mState);
            }
        }

        /*package*/ boolean finished() {
            synchronized (mStateLock) {
                return (VAL_STATE_FINISHED == mState || VAL_STATE_UNKNOW == mState);
            }
        }

        private void syncReset(int state) {
            synchronized (mStateLock) {
                mState = state;
            }
        }

        private void syncNotice(Bitmap bitmap) {
            synchronized (mStateLock) {
                sHandler.sendMessage(sHandler.obtainMessage(mState, bitmap));
            }
        }

        @Override
        public String toString() {
            String result = null;
            synchronized (mStateLock) {
                switch (mState) {
                    case VAL_STATE_FAILED:
                        result = "failed";
                        break;
                    case VAL_STATE_RUNNING:
                        result = "running";
                        break;
                    case VAL_STATE_FINISHED:
                        result = "finished";
                        break;
                    default: result = "unknow";
                }
            }
            return result;
        }

        // override load run method
        public void run(Uri uri) {
            boolean result = finished();
            Log.d(TAG,
                String.format("we can start thread load resource? [%b] state = %s",
                    new Object[] { result, toString() }));
            if (result) {
                // SPRD: bug 251198
                mJpagByteArry = null;
                mUri = uri;
                Thread t = new Thread(this);
                t.start();
            }
        }
        //@{ SPRD: bug 251198
        public void run(byte [] jpagByte) {
            boolean bResult = finished();
            if(bResult) {
                mUri = null;
                mJpagByteArry = jpagByte;
                Thread t = new Thread(this);
                t.start();
            }
        } //@}

        @Override
        public void run() {
            synchronized (mMutexLock) {
                syncReset(VAL_STATE_RUNNING); // starting load resource
                // convert to byte array from input stream by URI
                //@{ SPRD: bug 251198
                //byte[] dataJpeg = readStream(mUri);
                byte[] dataJpeg = (mJpagByteArry == null)?readStream(mUri):
                    mJpagByteArry; //@}
                boolean success = (dataJpeg != null);
                boolean tryOnce = !success;
                int orientation = (success ? Exif.getOrientation(dataJpeg) : -1);

                Bitmap tmpBitmap = null;
                int compress = mCompressionRatio; // default compression ratio is 16
                // if throw OOM error, we need try to once, changed compression ratio to 32
                while (success) {
                    try {
                        Log.d(TAG, "decode bitmap by byte array stream, compress = " + compress);
                        BitmapFactory.Options options = new BitmapFactory.Options();
                        options.inPreferredConfig = Bitmap.Config.RGB_565;
                        options.inDither = true;
                        options.inSampleSize = compress;
                        // first loading resource by input stream
                        tmpBitmap =
                            BitmapFactory.decodeByteArray(dataJpeg, 0, dataJpeg.length, options);
                        // rotate bitmap
                        tmpBitmap = ThumbnailRotate.proxyRotateImage(tmpBitmap, orientation);
                    } catch (OutOfMemoryError e) {
                        Log.d(TAG, "convert to bitmap has OOM error, PLS ignore", e);
                        compress *= 2; // current compress ratio is compress x 2
                    } finally {
                        success = (mCompressionRatio != compress);
                        if (tryOnce) success = false; // try to once finished
                        if (success) tryOnce = true; // set try to once flag
                    }
                }
                // load resource failed
                if (!success) syncReset(VAL_STATE_FAILED);

                // initialize or recycle bitmap
                if (tmpBitmap != null) {
                    syncReset(VAL_STATE_FINISHED); // load resource success
                }

                syncNotice(tmpBitmap); // notice main thread
            }
        }

        private byte[] readStream(Uri uri) {
            byte[] result = null;
            if (uri != null) {
                InputStream input = null;
                ByteArrayOutputStream output = null;
                try {
                    byte[] buff = new byte[mBuffSize];
                    input = getContext().getContentResolver().openInputStream(uri);
                    output = new ByteArrayOutputStream();
                    int len = 0;
                    while ((len = input.read(buff)) != -1) {
                        output.write(buff, 0, len);
                    }
                    result = output.toByteArray();
                } catch (FileNotFoundException e) {
                    Log.e(TAG, "open input stream by uri failed, uri = " + uri, e);
                } catch (IOException e) {
                    Log.e(TAG, "read input stream failed", e);
                } finally {
                    CameraUtil.closeSilently(output);
                    CameraUtil.closeSilently(input);
                }
            }
            return result;
        }
    }

    // SPRD: change the orientation of freezeframe view's child view
    public void updateFreezeChildUi(int orientation) {
        if(mRotatableView != null && mRelativeLayoutView != null) {
            switch (orientation) {
                case 180:
                    mRotatableView.setRotationY(180);
                    mRelativeLayoutView.setRotationY(180);
                    break;
                case 270:
                    mRotatableView.setRotationX(180);
                    mRelativeLayoutView.setRotationX(180);
            }
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        android.util.Log.i("lingyun","FreezeFrameView.onTouchEvent");
        return true;
    }
}
