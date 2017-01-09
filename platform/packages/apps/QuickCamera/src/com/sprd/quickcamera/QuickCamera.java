package com.sprd.quickcamera;

import android.app.ActivityManager;
import android.app.ActivityManager.RunningTaskInfo;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.Matrix;
import android.graphics.PixelFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.net.Uri;
import android.opengl.GLES20;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Message;
import android.os.Process;
import android.os.Vibrator;
import android.provider.MediaStore;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.OrientationEventListener;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageView;

import com.android.ex.camera2.portability.CameraAgent;
import com.android.ex.camera2.portability.CameraAgentFactory;
import com.android.ex.camera2.portability.CameraCapabilities;
import com.android.ex.camera2.portability.CameraDeviceInfo;
import com.android.ex.camera2.portability.CameraSettings;
import com.android.ex.camera2.portability.Size;
import com.sprd.quickcamera.R;
import com.sprd.quickcamera.exif.ExifInterface;

import java.io.File;
import java.io.FileOutputStream;
import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;


/**
 * TODO: - Performance when over gl surfaces? Ie. Gallery - what do we say in
 * the Toast? Which icon do we get if the user uses another type of gallery?
 */
public class QuickCamera implements SurfaceHolder.Callback, SensorEventListener
        , CameraAgent.CameraOpenCallback {
    private static final String TAG = "QuickCamera";

    private static final int QUICKCAMERA_REMOVE_LAYOUT = 0;
    private static final int QUICKCAMERA_DO_CAPTURE = 1;
    private static final int DO_CAPTURE_PENDING_TIME = 3000;
    private static final int ON_PREVIEW_STARTED_PENDING_TIME= 300;

    private Context mContext;
    private WindowManager mWindowManager;
    private WindowManager.LayoutParams mWindowLayoutParams;
    private Display mDisplay;
    private DisplayMetrics mDisplayMetrics;
    private Matrix mDisplayMatrix;

    // private Bitmap mShotBitmap;
    private View mQuickshotLayout;
    private ImageView mBackgroundView;
    /*
     * private ImageView mQuickshotView;
     * private ImageView mQuickshotFlash;
     */
    private SurfaceHolder holder = null;
    private SurfaceView mSurfaceView = null;
    private RotatableTextView mRotateTextView = null;

    // The value for android.hardware.Camera.Parameters.setRotation.
    private int mJpegRotation;
    private int mDeviceRotation;
    private byte[] mJpegData;
    private int mImageRotation;
    private int mImageWidth;
    private int mImageHeight;
    private Size mBestPictureSize;

    private Long mStartTime;
    private Runnable mFinisher = null;
    private String mStoragePath = null;

    private boolean mCaptureDone = false;
    private int mCameraId = 0;
    //private MediaActionSound mCameraSound;

    private SensorManager mSensorManager;
    private AsyncTask<SaveImageInBackgroundData, Void, SaveImageInBackgroundData> mSaveInBgTask;

    private SurfaceTexture gSurfaceTexture;
    private int gSurfaceTextureId = 0;
    private boolean mSurfaceVisible = false;
    private SurfaceHolder mSurfaceHolder = null;
    private boolean mFirstSensorEvent = true;

    private CameraController mCameraController = null;
    private CameraAgent.CameraProxy mCameraDevice = null;
    private CameraCapabilities mCameraCapabilities = null;
    private long mFocusStartTime;
    private boolean mPreviewStarted;
    private boolean mDeviceRotationDetected;
    private boolean mFocusing;
    private int mFrameCount = 0;
    private final int MAX_FRAME_COUNT = 0;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case QUICKCAMERA_REMOVE_LAYOUT:
                    if (mBackgroundView != null) {
                        mBackgroundView.setVisibility(View.GONE);
                    }
                    if (mWindowManager != null) {
                        mWindowManager.removeView(mQuickshotLayout);
                    }

                    break;
                case QUICKCAMERA_DO_CAPTURE:
                    Log.i(TAG, "doCapture from handler");
                    doCapture(true);
                    break;
            }
        }
    };

    /**
     * @param context everything needs a context
     */
    public void initializeViews(Context context) {
        LayoutInflater layoutInflater = (LayoutInflater) context
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        // Inflate the quick shot layout
        mDisplayMatrix = new Matrix();
        mQuickshotLayout = layoutInflater.inflate(R.layout.quickcamera_snap,
                null);
        mQuickshotLayout.setRotation(0);
        mBackgroundView = (ImageView) mQuickshotLayout
                .findViewById(R.id.quickshot_background);
        /*
         * mQuickshotView = (ImageView)
         * mQuickshotLayout.findViewById(R.id.quickshot); mQuickshotFlash =
         * (ImageView) mQuickshotLayout.findViewById(R.id.quickshot_flash);
         */
        mSurfaceView = (SurfaceView) mQuickshotLayout
                .findViewById(R.id.surface_view);
        mRotateTextView = (RotatableTextView) mQuickshotLayout
                .findViewById(R.id.time_cost);

        // Setup the window that we are going to use
        mWindowLayoutParams = new WindowManager.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 0,
                WindowManager.LayoutParams.TYPE_SECURE_SYSTEM_OVERLAY,
                WindowManager.LayoutParams.FLAG_FULLSCREEN
                        | WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED
                        | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                        | WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD
                        | WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED
                        | WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON,
                PixelFormat.TRANSLUCENT);

        mWindowLayoutParams.screenOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
        mWindowManager = (WindowManager) context
                .getSystemService(Context.WINDOW_SERVICE);
        mDisplay = mWindowManager.getDefaultDisplay();
        mDisplayMetrics = new DisplayMetrics();
        mDisplay.getRealMetrics(mDisplayMetrics);

        // Setup the Camera shutter sound
        //mCameraSound = new MediaActionSound();
        //mCameraSound.load(MediaActionSound.SHUTTER_CLICK);
    }

    /**
     * @param context everything needs a context.
     * @param surfaceVisible whether the preview interface is visible.
     */
    public QuickCamera(Context context, boolean surfaceVisible) {
        mContext = context;
        mSurfaceVisible = surfaceVisible;

        mSensorManager = (SensorManager) context
                .getSystemService(Context.SENSOR_SERVICE);

        if (QuickCameraService.getMode()
                == QuickCameraService.MODE_CAPTURE_WITH_BACK_CAMERA) {
            mCameraId = 0;
        } else if (QuickCameraService.getMode()
                == QuickCameraService.MODE_CAPTURE_WITH_FRONT_CAMERA) {
            mCameraId = 1;
        }

        if (surfaceVisible) {
            initializeViews(context);
        } else {
            int[] textures = new int[1];
            GLES20.glGenTextures(1, textures, 0);
            gSurfaceTextureId = textures[0];
            gSurfaceTexture = new SurfaceTexture(gSurfaceTextureId);
        }

        try {
            mCameraController = new CameraController(this, mHandler,
                    CameraAgentFactory.getAndroidCameraAgent(context,
                            CameraAgentFactory.CameraApi.API_1),
                    CameraAgentFactory.getAndroidCameraAgent(context,
                            CameraAgentFactory.CameraApi.AUTO));
        } catch (AssertionError e) {
            Log.e(TAG, "Creating camera controller failed.", e);
        }
    }

    private boolean checkSpaceAvailable() {
        StorageUtil storageUtil = StorageUtil.newInstance();
        /* SPRD: add fix the bug 521329 quick capture picture won't save with the path
           settings which defined by camera2 app @{ */
        String theDeafultPath = QuickCameraService.getStoragePath();
        mStoragePath = storageUtil.getDefaultSaveDirectory(theDeafultPath);
        /* @} */
        Log.v(TAG, "mStoragePath = " + mStoragePath);
        return (storageUtil.getAvailableSpace(mStoragePath) > StorageUtil.LOW_STORAGE_THRESHOLD_BYTES);

    }

    /**
     * Creates a new worker thread and saves the quick shot to the media store.
     */
    private void saveQuickShotInWorkerThread() {
        SaveImageInBackgroundData data = new SaveImageInBackgroundData();
        data.context = mContext;
        data.jpegData = mJpegData;
        data.width = mImageWidth;
        data.height = mImageHeight;
        data.orientation = mImageRotation;
        data.path = mStoragePath;

        if (mSaveInBgTask != null) {
            mSaveInBgTask.cancel(false);
        }

        //fix coverity issue 108933
        if (data.jpegData == null) {
            android.util.Log.d(TAG, "data.jpegData=null");
            return;
        }

        mSaveInBgTask = new SaveImageInBackgroundTask(mContext, data)
                .execute(data);
    }

    /**
     * Takes a quick shot of the current display and shows an animation.
     */

    void takeQuickShot(Runnable finisher) {
        // We need to orient the quick shot correctly (and the Surface api seems
        // to take quick shot
        // only in the natural orientation of the device :!)
        if (mCameraController == null) {
            Log.d(TAG, "CameraController is null, return");
            return;
        }

        mStartTime = System.currentTimeMillis();
        Log.d(TAG, "call takeQuickShot");
        if (mDisplay != null) {
            mDisplay.getRealMetrics(mDisplayMetrics);
        }
        mFinisher = finisher;

        Sensor gsensor = mSensorManager
                .getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        if (gsensor != null) {
            mSensorManager.registerListener(this, gsensor,
                    SensorManager.SENSOR_DELAY_NORMAL);
        }

        // move addView here for SurfaceView create.
        if (mWindowManager != null && mQuickshotLayout != null
                && mWindowLayoutParams != null && mSurfaceView != null) {
            mWindowManager.addView(mQuickshotLayout, mWindowLayoutParams);
            holder = mSurfaceView.getHolder();
            holder.addCallback(this);
            holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        }

        startCamera();

        // we design to do capture in onSensorChanged(SensorEvent event), but sometimes
        // sensor can't be activated when screen off though we have acquire wakelock,
        // so we make the logic below as substitution.
        // note capture logic is different when mSurfaceVisible is true.
        mHandler.sendEmptyMessageDelayed(QUICKCAMERA_DO_CAPTURE, DO_CAPTURE_PENDING_TIME);
    }

    private synchronized void doCapture() {
        doCapture(false);
    }

    private synchronized void doCapture(boolean forced) {
        if (mCaptureDone) {
            Log.i(TAG, "doCapture return, capture has done");
        }

        if (mCameraDevice == null) {
            Log.i(TAG, "doCapture return, camera device is null");
            return;
        }

        if ((mSurfaceVisible && mSurfaceHolder == null)
                || (mSurfaceVisible && gSurfaceTexture != null)) {
            Log.i(TAG, "doCapture return, surface is null");
            return;
        }

        if (!mPreviewStarted) {
            Log.i(TAG, "doCapture return, preview not started");
            return;
        }

        if (mFocusing) {
            Log.i(TAG, "doCapture return, focusing");
            return;
        }

        if (!forced && !mDeviceRotationDetected) {
            Log.i(TAG, "doCapture return, device rotation not detected");
            return;
        }

        mHandler.removeMessages(QUICKCAMERA_DO_CAPTURE);

        Log.v(TAG, "mDeviceRotation = " + mDeviceRotation);
        setRotation(mDeviceRotation);

        if (mRotateTextView != null) {
            mRotateTextView.setTextViewRotation(mDeviceRotation);
        }
        mCameraDevice.takePicture(mHandler, null, null, null, new JpegPictureCallback());

        mCaptureDone = true;
        Log.i(TAG, "doCapture end");
    }

    private void setRotation(int deviceOrientation) {
        if (deviceOrientation == OrientationEventListener.ORIENTATION_UNKNOWN) return;

        CameraDeviceInfo.Characteristics info = mCameraController.getCharacteristics(mCameraId);
        int sensorOrientation = info.getSensorOrientation();
        boolean isFrontCamera = info.isFacingFront();
        // The sensor of front camera faces in the opposite direction from back camera.
        if (isFrontCamera) {
            deviceOrientation = (360 - deviceOrientation) % 360;
        }
        mJpegRotation = (sensorOrientation + deviceOrientation) % 360;

        if (mCameraDevice != null) {
            mCameraDevice.setJpegOrientation(mJpegRotation);
        }
    }

    private void startCamera() {
        mCameraController.requestCamera(mCameraId, useNewApi());
    }

    public Size getPreviewSize(List<Size> list, double targetRatio) {
        final double aspectRatioTolerance = 0.02;
        for (Size s : list) {
            double ratio = (double) s.width() / s.height();
            if (Math.abs(ratio - targetRatio) < aspectRatioTolerance) {
                return s;
            }
        }

        return list.get(0);
    }

    private String formatSnapCostTime(Long costTime) {
        DecimalFormat df = new DecimalFormat("0.0");
        return df.format((double) costTime / 1000);
    }

    private final class JpegPictureCallback implements CameraAgent.CameraPictureCallback {

        @Override
        public void onPictureTaken(final byte[] jpegData, final CameraAgent.CameraProxy camera) {

            long costTime = System.currentTimeMillis() - mStartTime;
            Log.d(TAG, "JpegPictureCallback cost : " + costTime + "ms.");

            if (mRotateTextView != null) {
                mRotateTextView.setText(mContext.getString(R.string.quick_snap_time_cost, formatSnapCostTime(costTime)));
            }
            Vibrator vibrator = (Vibrator) mContext.getSystemService(Context.VIBRATOR_SERVICE);
            long[] pattern = {0, 50};
            vibrator.vibrate(pattern, -1);

            if (mSurfaceVisible) {
                mHandler.sendEmptyMessage(QUICKCAMERA_REMOVE_LAYOUT);
            }

            mJpegData = jpegData;

            releaseCamera();

            /*Sensor gsensor = mSensorManager
                    .getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            if (gsensor != null) {
                mSensorManager.unregisterListener(QuickCamera.this, gsensor);
            }*/

            if (mJpegData == null) {
                Log.d(TAG, "quickcamera snap error! mJpegData == null");
                return;
            }

            if (!checkSpaceAvailable()) {
                //startCameraActivity(mContext);
                return;
            }

            ExifInterface exif = Exif.getExif(jpegData);
            mImageRotation = Exif.getOrientation(exif);
            // int width, height;
            if ((mJpegRotation + mImageRotation) % 180 == 0) {
                mImageWidth = mBestPictureSize.width();
                mImageHeight = mBestPictureSize.height();
            } else {
                mImageWidth = mBestPictureSize.height();
                mImageHeight = mBestPictureSize.width();
            }

            saveQuickShotInWorkerThread();
        }
    }

    public void surfaceChanged(SurfaceHolder sholder, int format, int width, int height) {
    }

    public void surfaceCreated(SurfaceHolder holder) {
        mSurfaceHolder = holder;
        startPreviewApi_1();
    }

    public void surfaceDestroyed(SurfaceHolder sholder) {
        mSurfaceHolder = null;
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        if (event.values != null) {
            float x = event.values[0];
            float y = event.values[1];
            if (Math.abs(x) < 1) {
                x = 0;
            }
            if (Math.abs(y) < 1) {
                y = 0;
            }
            if (Math.abs(x) <= Math.abs(y)) {
                if (y < 0) {
                    // up is low
                    Log.v(TAG, "up now");
                    mDeviceRotation = 180;
                } else if (y > 0) {
                    // down is low
                    Log.v(TAG, "down now");
                    mDeviceRotation = 0;
                } else if (y == 0) {
                    // do nothing
                }
            } else {
                if (x < 0) {
                    // right is low
                    Log.v(TAG, "right now");
                    mDeviceRotation = 90;
                } else {
                    // left is low
                    Log.v(TAG, "left now");
                    mDeviceRotation = 270;
                }
            }
        }

        // first sensor event is not accurate
        if (!mFirstSensorEvent) {
            Sensor gSensor = mSensorManager
                    .getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            if (gSensor != null) {
                mSensorManager.unregisterListener(QuickCamera.this, gSensor);
            }

            // generally, startCamera() completes before here.
            mDeviceRotationDetected = true;
            Log.d(TAG, "doCapture from onSensorChanged");
            doCapture();
        }

        mFirstSensorEvent = false;
    }

    /**
     * POD used in the AsyncTask which saves an image in the background.
     */
    class SaveImageInBackgroundData {
        Context context;
        Uri imageUri;
        int result;
        byte[] jpegData;
        int width;
        int height;
        int orientation;
        String path;

        void clearImage() {
            imageUri = null;
            jpegData = null;
        }

        void clearContext() {
            context = null;
        }
    }

    /**
     * An AsyncTask that saves an image to the media store in the background.
     */
    private class SaveImageInBackgroundTask extends
            AsyncTask<SaveImageInBackgroundData, Void, SaveImageInBackgroundData> {
        private static final String TAG = "SaveImageInBackgroundTask";

        private static final String QUICKSHOT_FILE_NAME_TEMPLATE = "%s.jpg";

        private final File mQuickshotDir;
        private final String mImageFileName;
        private final String mImageFilePath;
        private final long mSnapTime;
        private final int mImageWidth;
        private final int mImageHeight;
        private final int mOrientation;

        SaveImageInBackgroundTask(Context context, SaveImageInBackgroundData data) {
            // Prepare all the output metadata
            mSnapTime = System.currentTimeMillis();
            String imageDate = new SimpleDateFormat("'IMG'_yyyyMMdd_HHmmss")
                    .format(new Date(mSnapTime));
            mImageFileName = String.format(QUICKSHOT_FILE_NAME_TEMPLATE, imageDate);

            mQuickshotDir = new File(data.path);
            mImageFilePath = new File(mQuickshotDir, mImageFileName)
                    .getAbsolutePath();

            mImageWidth = data.width;
            mImageHeight = data.height;
            mOrientation = data.orientation;

        }

        @Override
        protected SaveImageInBackgroundData doInBackground(
                SaveImageInBackgroundData... params) {
            if (params.length != 1)
                return null;
            if (isCancelled()) {
                params[0].clearImage();
                params[0].clearContext();
                return null;
            }

            // By default, AsyncTask sets the worker thread to have background
            // thread priority, so bump
            // it back up so that we save a little quicker.
            Process.setThreadPriority(Process.THREAD_PRIORITY_FOREGROUND);

            Context context = params[0].context;
            byte[] jpegdata = params[0].jpegData;

            try {
                // Create quick shot directory if it doesn't exist
                mQuickshotDir.mkdirs();

                // media provider uses seconds for DATE_MODIFIED and DATE_ADDED, but
                // milliseconds
                // for DATE_TAKEN
                long dateSeconds = mSnapTime / 1000;

                // Save the quick shot to the MediaStore
                ContentValues values = new ContentValues();
                ContentResolver resolver = context.getContentResolver();
                values.put(MediaStore.Images.ImageColumns.DATA, mImageFilePath);
                values.put(MediaStore.Images.ImageColumns.TITLE, mImageFileName);
                values.put(MediaStore.Images.ImageColumns.DISPLAY_NAME,
                        mImageFileName);
                values.put(MediaStore.Images.ImageColumns.DATE_TAKEN, mSnapTime);
                values.put(MediaStore.Images.ImageColumns.DATE_ADDED, dateSeconds);
                values.put(MediaStore.Images.ImageColumns.DATE_MODIFIED,
                        dateSeconds);
                values.put(MediaStore.Images.ImageColumns.MIME_TYPE, "image/jpeg");
                values.put(MediaStore.Images.ImageColumns.WIDTH, mImageWidth);
                values.put(MediaStore.Images.ImageColumns.HEIGHT, mImageHeight);
                values.put(MediaStore.Images.ImageColumns.ORIENTATION, mOrientation);
                Uri uri = resolver.insert(
                        MediaStore.Images.Media.EXTERNAL_CONTENT_URI, values);

                //startCameraActivity(context);

                writeFile(mImageFilePath, jpegdata);

                // update file size in the database
                values.clear();
                values.put(MediaStore.Images.ImageColumns.SIZE, new File(
                        mImageFilePath).length());
                resolver.update(uri, values, null, null);

                params[0].imageUri = uri;
                params[0].result = 0;
            } catch (Exception e) {
                // IOException/UnsupportedOperationException may be thrown if
                // external storage is not
                // mounted
                Log.d(TAG, "save-Exception:" + e.toString());
                params[0].clearImage();
                params[0].result = 1;
            }

            return params[0];
        }

        @Override
        protected void onPostExecute(SaveImageInBackgroundData params) {
            if (isCancelled()) {
                params.clearImage();
                params.clearContext();
                return;
            }

            if (params.result > 0) {
                // Show a message that we've failed to save the image to disk
                Log.d(TAG, "quickcamera shot error : params.result ="
                        + params.result);
            } else {
                // Show the final notification to indicate quick shot saved

                // Create the intent to show the quick shot in gallery
            }
            params.clearContext();
        }

        public void writeFile(String path, byte[] data) {
            FileOutputStream out = null;
            try {
                out = new FileOutputStream(path);
                out.write(data);
            } catch (Exception e) {
                Log.e(TAG, "Failed to write data", e);
            } finally {
                try {
                    if (out != null) {
                        out.close();
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Failed to close file after write", e);
                }
            }
        }

    }

    private ComponentName getTopActivity(Context context) {
        ActivityManager manager = (ActivityManager) context
                .getSystemService(Context.ACTIVITY_SERVICE);
        List<RunningTaskInfo> runningTaskInfos = manager.getRunningTasks(1);

        if (runningTaskInfos != null) {
            return runningTaskInfos.get(0).topActivity;
        } else {
            return null;
        }
    }

    private void startCameraActivity(Context context) {
        ComponentName topActivity = getTopActivity(context);
        if ((null != topActivity
                && !topActivity.getPackageName().equalsIgnoreCase("com.android.camera2") && !topActivity
                .getPackageName().equalsIgnoreCase("com.huawei.camera"))
                || null == topActivity) {
            Intent intent = new Intent("android.media.action.STILL_IMAGE_CAMERA");
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
        }
    }

    private void releaseCamera() {
        mHandler.removeMessages(QUICKCAMERA_DO_CAPTURE);

        if (mCameraDevice != null) {
            mCameraDevice.stopPreview();
            mCameraDevice = null;
        }

        if (gSurfaceTexture != null) {
            gSurfaceTexture.release();
        }

        mCameraController.closeCamera(true);

        CameraAgentFactory.recycle(CameraAgentFactory.CameraApi.API_1);
        CameraAgentFactory.recycle(CameraAgentFactory.CameraApi.API_2);

        if (mFinisher != null) {
            mFinisher.run();
        }
    }

    private boolean useNewApi() {
        // SurfaceView is only supported by api_1
        return !mSurfaceVisible;
    }

    private boolean isAutoFocusSupport() {
        return false;
        //return mCameraCapabilities != null
        //        && mCameraCapabilities.supports(CameraCapabilities.FocusMode.AUTO);
    }

    private boolean isFlashOffSupport() {
        return mCameraCapabilities != null
                && mCameraCapabilities.supports(CameraCapabilities.FlashMode.OFF);
    }

    private int getDisplayRotation() {
        WindowManager windowManager = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
        int rotation = windowManager.getDefaultDisplay().getRotation();
        switch (rotation) {
            case Surface.ROTATION_0:
                return 0;
            case Surface.ROTATION_90:
                return 90;
            case Surface.ROTATION_180:
                return 180;
            case Surface.ROTATION_270:
                return 270;
        }
        return 0;
    }

    @Override
    public void onCameraOpened(CameraAgent.CameraProxy camera) {
        Log.i(TAG, "onCameraOpened");
        mCameraDevice = camera;
        mCameraCapabilities = mCameraDevice.getCapabilities();
        CameraSettings cameraSettings = mCameraDevice.getSettings();
        cameraSettings.setPhotoFormat(PixelFormat.JPEG);
        mBestPictureSize = mCameraCapabilities.getSupportedPhotoSizes().get(0);
        List<Size> supportPreviewSizes = mCameraCapabilities.getSupportedPreviewSizes();
        Size preSize = getPreviewSize(supportPreviewSizes, (double) mBestPictureSize.width() /
                (double) mBestPictureSize.height());
        cameraSettings.setPreviewSize(preSize);
        cameraSettings.setPhotoSize(mBestPictureSize);
        cameraSettings.setSceneMode(CameraCapabilities.SceneMode.AUTO);
        cameraSettings.setWhiteBalance(CameraCapabilities.WhiteBalance.AUTO);
        cameraSettings.setExposureCompensationIndex(0);
        cameraSettings.setPhotoJpegCompressionQuality(100);
        if (isAutoFocusSupport()) {
            cameraSettings.setFocusMode(CameraCapabilities.FocusMode.AUTO);
        }
        if (isFlashOffSupport()) {
            cameraSettings.setFlashMode(CameraCapabilities.FlashMode.OFF);
        }

        /* SPRD: Fix bug 474860, Add for Dark photos of quick capture. @{ */
        if (mCameraCapabilities.supports(CameraCapabilities.Feature.AUTO_EXPOSURE_LOCK)) {
            cameraSettings.setAutoExposureLock(true);
        }
        /* @} */

        mCameraDevice.setDisplayOrientation(getDisplayRotation());
        mCameraDevice.applySettings(cameraSettings);
        try {
            if (mSurfaceVisible) {
                startPreviewApi_1();
            } else {
                startPreviewApi_2();
            }
        } catch (Exception e) {
            releaseCamera();
        }
    }

    @Override
    public void onCameraDisabled(int cameraId) {

    }

    @Override
    public void onDeviceOpenedAlready(int cameraId, String info) {

    }

    @Override
    public void onReconnectionFailure(CameraAgent mgr, String info) {

    }

    @Override
    public void onDeviceOpenFailure(int cameraId, String info) {

    }

    private void onPreviewStarted() {
        mPreviewStarted = true;
        if (!isAutoFocusSupport()) {
            Log.d(TAG, "doCapture from onPreviewStarted");
            doCapture();
        } else {
            mFocusing = true;
            mFocusStartTime = System.currentTimeMillis();
            mCameraDevice.autoFocus(mHandler, mAutoFocusCallback);
        }
    }

    private void startPreviewApi_1() {
        if (mCameraDevice == null) {
            Log.w(TAG, "startPreview: camera device not ready yet.");
            return;
        }

        if (mSurfaceHolder == null) {
            Log.w(TAG, "startPreview: mSurfaceHolder is not ready.");
            return;
        }

        mCameraDevice.setPreviewDisplay(mSurfaceHolder);
        mCameraDevice.startPreviewWithCallback(mHandler, new CameraAgent.CameraStartPreviewCallback() {
            @Override
            public void onPreviewStarted() {
                mHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        QuickCamera.this.onPreviewStarted();
                    }
                }, ON_PREVIEW_STARTED_PENDING_TIME);
            }
        });
    }

    private void startPreviewApi_2() {
        if (mCameraDevice == null) {
            Log.w(TAG, "startPreview: camera device not ready yet.");
            return;
        }

        if (gSurfaceTexture == null) {
            Log.w(TAG, "startPreview: mSurfaceHolder is not ready.");
            return;
        }

        gSurfaceTexture.setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {
            @Override
            public void onFrameAvailable(SurfaceTexture surfaceTexture) {
                Log.i(TAG, "onFrameAvailable");
                if (mFrameCount < MAX_FRAME_COUNT) {
                    mFrameCount++;
                } else if (mFrameCount == MAX_FRAME_COUNT) {
                    mHandler.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            QuickCamera.this.onPreviewStarted();
                        }
                    }, ON_PREVIEW_STARTED_PENDING_TIME);
                }
            }
        });

        mCameraDevice.setPreviewTexture(gSurfaceTexture);
        mCameraDevice.startPreview();
    }

    private CameraAgent.CameraAFCallback mAutoFocusCallback = new CameraAgent.CameraAFCallback() {
        @Override
        public void onAutoFocus(boolean focused, CameraAgent.CameraProxy camera) {
            mFocusing = false;
            if (mCameraDevice != null) {
                mCameraDevice.cancelAutoFocus();
            }
            long autoFocusTime = System.currentTimeMillis() - mFocusStartTime;
            Log.d(TAG, "doCapture from onAutoFocus " + focused + " " + autoFocusTime + "ms");
            doCapture(true);
        }
    };
}
