package com.dream.camera.modules.qr;

import android.Manifest;

import java.io.IOException;
import java.util.HashMap;
import java.util.Vector;
import android.content.Intent;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.graphics.Bitmap;
import android.hardware.Camera;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.Vibrator;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.app.Service;
import com.google.zxing.BarcodeFormat;
import com.google.zxing.Result;
import com.dream.camera.modules.qr.CameraManager;
import com.android.camera.MultiToggleImageButton.OnStateChangeListener;
import com.dream.camera.modules.qr.QrCaptureActivityHandler;
import com.dream.camera.modules.qr.InactivityTimer;
import com.dream.camera.modules.qr.ViewfinderView;
import com.android.camera.app.AppController;
import com.android.camera.app.CameraController;
import com.android.camera.settings.Keys;
import com.android.camera.ui.RotateImageButton;
import com.android.camera.app.AppController;
import com.android.camera.app.AppController;
import com.android.camera2.R;
import java.util.Hashtable;
import com.google.zxing.BinaryBitmap;
import com.google.zxing.ChecksumException;
import com.google.zxing.DecodeHintType;
import com.google.zxing.FormatException;
import com.google.zxing.LuminanceSource;
import com.google.zxing.MultiFormatReader;
import com.google.zxing.NotFoundException;
import com.google.zxing.common.HybridBinarizer;
import com.google.zxing.qrcode.QRCodeReader;
import com.ucamera.ucam.modules.utils.BitmapUtils;
import com.dream.camera.modules.qr.PlanarYUVLuminanceSource;
import android.graphics.BitmapFactory;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Bitmap.CompressFormat;
import android.text.TextUtils;
import android.provider.MediaStore;
import android.database.Cursor;
import com.dream.camera.modules.qr.Utils;
import com.android.camera.util.ApiHelper;
import com.android.camera.util.CameraUtil;

import android.content.pm.PackageManager;
import com.android.camera.PermissionsActivity;

import java.nio.charset.Charset;

import com.android.camera.MultiToggleImageButton;
import com.android.camera.CameraActivity;
import android.os.Build;
import android.os.Looper;
import android.view.ViewGroup;
import android.app.Activity;
import com.dream.camera.modules.qr.ReusePhotoUI;
import com.dream.camera.modules.qr.ReuseController;
import com.dream.camera.settings.DataModuleBasic;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.settings.DataModuleBasic.DreamSettingChangeListener;
import com.dream.camera.util.DreamUtil;

public class QrCodePhotoModule extends ReuseModule implements Callback,
        DreamSettingChangeListener {

    private QrCaptureActivityHandler handler;
    private ViewfinderView viewfinderView;
    private boolean hasSurface;
    private Vector<BarcodeFormat> decodeFormats;
    private String characterSet;
    private InactivityTimer inactivityTimer;
//    private MediaPlayer mediaPlayer;
//    private boolean playBeep;
    private static final float BEEP_VOLUME = 0.10f;
    private boolean vibrate;
    private boolean isUpdateFlash = false;

    // private CameraActivity mActivity;
    private AppController mAppController;
    private boolean mPaused;
    private static final int REQUEST_CODE = 234;
    private String photo_path;
    private MultiToggleImageButton mFlashButton;
    private RotateImageButton mGalleryButton;
    private SurfaceView surfaceView;
    protected ViewGroup mRootView;
    protected ReusePhotoUI mUI;
    private Boolean mHasCriticalPermissions;
    private Bitmap mFreezeBitmap = null;

    public QrCodePhotoModule(AppController app) {
        super(app);
    }

    @Override
    public void init(CameraActivity activity, boolean isSecureCamera,
            boolean isCaptureIntent) {
        super.init(activity, isSecureCamera, isCaptureIntent);
        CameraManager.init(mActivity.getApplication());
        viewfinderView = (ViewfinderView) mActivity
                .findViewById(R.id.viewfinder_view);

        mFlashButton = (MultiToggleImageButton) mActivity
                .findViewById(R.id.gif_photo_flash_toggle_button_dream);
        mGalleryButton = (RotateImageButton) mActivity
                .findViewById(R.id.qrcode_gallery_toggle_button);

        hasSurface = false;
        // inactivityTimer = new InactivityTimer(mActivity);

    }

    @Override
    public int getMode() {
        return DreamUtil.PHOTO_MODE;
    }

    @Override
    public void makeModuleUI(ReuseController controller, View parent) {
        // TODO Waiting for merge of WideAngle feature, noted by spread
        // mActivity.getCameraAppUI().setLayoutAspectRation(0f);
        mUI = createUI(controller, parent);
        initializeModuleControls();
    }

    public QrCodePhotoUI createUI(ReuseController controller, View parent) {
        return new QrCodePhotoUI(mActivity, controller, parent);
    }

    protected void initializeModuleControls() {
        mRootView = mActivity.getCameraAppUI().getModuleView();
        mActivity.getLayoutInflater().inflate(R.layout.qrcode_capture,
                mRootView);
    }

    public void scanPicture() {
        Intent innerIntent = new Intent();
        if (Build.VERSION.SDK_INT < 19) {
            innerIntent.setAction(Intent.ACTION_GET_CONTENT);
        } else {
            innerIntent.setAction(Intent.ACTION_OPEN_DOCUMENT);
        }
        innerIntent.setType("image/*");
        Intent wrapperIntent = Intent.createChooser(innerIntent,
                "select qrcode picture");
        mActivity.startActivityForResult(wrapperIntent, REQUEST_CODE);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        // super.onActivityResult(requestCode, resultCode, data);
        if (resultCode == Activity.RESULT_OK) {
            switch (requestCode) {
            case REQUEST_CODE:
                /*
                 * SPRD: Fix bug 589142, Two dimensional code to select the
                 * image interface, the camera has been stopped running @{
                 */
                checkPermissions();
                if (!mHasCriticalPermissions) {
                    return;
                }
                if(data == null || data.getData() == null){
                    return;
                }
                /* @} */
                String[] proj = { MediaStore.Images.Media.DATA };
                // Gets the path to the selected image
                Cursor cursor = mActivity.getContentResolver().query(
                        data.getData(), proj, null, null, null);
                if(cursor == null){
                    return;
                }
                if (cursor.moveToFirst()) {
                    int column_index = cursor
                            .getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
                    photo_path = cursor.getString(column_index);
                    if (photo_path == null) {
                        photo_path = Utils.getPath(
                                mActivity.getApplicationContext(),
                                data.getData());
                    }
                }
                cursor.close();
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        isUpdateFlash = true;
                        Result result = scanningImage(photo_path);
                        /*
                         * if (result == null) { Looper.prepare();
                         * Looper.loop(); } else { String recode =
                         * recode(result.toString()); Intent data = new
                         * Intent(); data.putExtra("result", recode);
                         * mActivity.setResult(300, data); mActivity.finish(); }
                         */
                    }
                }).start();
                break;
            }
        }
    }

    // Annlysis part of pictures
    protected Result scanningImage(String path) {
        Bitmap scanBitmap = null;
        MultiFormatReader multiFormatReader = new MultiFormatReader();
        Result result;
        if (TextUtils.isEmpty(path)) {
            return null;
        }
        // DecodeHintType and EncodeHintType
        Hashtable<DecodeHintType, String> hints = new Hashtable<DecodeHintType, String>();
        BitmapFactory.Options options = new BitmapFactory.Options();
        // Get the original size
        options.inJustDecodeBounds = true;
        scanBitmap = BitmapFactory.decodeFile(path, options);
        // Get new size
        options.inJustDecodeBounds = false;

        int sampleSize = (int) (options.outHeight / (float) 200);
        if (sampleSize <= 0)
            sampleSize = 1;
        options.inSampleSize = sampleSize;
        scanBitmap = BitmapFactory.decodeFile(path, options);

        if (scanBitmap == null) {
            return null;
        }
        LuminanceSource source = new PlanarYUVLuminanceSource(
                rgb2YUV(scanBitmap), scanBitmap.getWidth(),
                scanBitmap.getHeight(), 0, 0, scanBitmap.getWidth(),
                scanBitmap.getHeight());
        BinaryBitmap binaryBitmap = new BinaryBitmap(
                new HybridBinarizer(source));

        try {
            result = multiFormatReader.decodeWithState(binaryBitmap);
            String content = result.getText();
            handleDecode(result, scanBitmap);
        } catch (NotFoundException e1) {
            // TODO Auto-generated catch block
            notFoundQrException(e1);
        } catch (ArrayIndexOutOfBoundsException e1) {
            notFoundQrException(e1);
        }
        return null;
    }

    private void notFoundQrException(Exception e1) {
        e1.printStackTrace();
        Intent intent = new Intent();
        Bundle bundle = new Bundle();
        bundle.putString("result", "defaulturi");
        intent.putExtras(bundle);
        intent.setClass(mActivity, QrScanResultActivity.class);
        mActivity.startActivity(intent);
    }

    private String recode(String str) {
        String formart = "";
        try {
            boolean ISO = Charset.forName("ISO-8859-1").newEncoder()
                    .canEncode(str);
            if (ISO) {
                formart = new String(str.getBytes("ISO-8859-1"), "GB2312");
            } else {
                formart = str;
            }
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        return formart;
    }

    public byte[] rgb2YUV(Bitmap bitmap) {
        int width = bitmap.getWidth();
        int height = bitmap.getHeight();
        int[] pixels = new int[width * height];
        bitmap.getPixels(pixels, 0, width, 0, 0, width, height);

        int len = width * height;
        byte[] yuv = new byte[len * 3 / 2];
        int y, u, v;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                int rgb = pixels[i * width + j] & 0x00FFFFFF;

                int r = rgb & 0xFF;
                int g = (rgb >> 8) & 0xFF;
                int b = (rgb >> 16) & 0xFF;

                y = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
                u = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
                v = ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;

                y = y < 16 ? 16 : (y > 255 ? 255 : y);
                u = u < 0 ? 0 : (u > 255 ? 255 : u);
                v = v < 0 ? 0 : (v > 255 ? 255 : v);
                yuv[i * width + j] = (byte) y;
            }
        }
        return yuv;
    }

    @Override
    public void resume() {
        // super.onResume();
        /*
         * SPRD: Fix bug 585415, Close permissions, recent returns to the camera
         * and stop running. @{
         */
        checkPermissions();
        if (!mHasCriticalPermissions) {
            return;
        }
        mRootView.setVisibility(View.VISIBLE);
        mUI.updateGalleryButton();
        // super.resume();
        ((CameraController) mActivity.getCameraProvider()).closeCamera(true);
        surfaceView = (SurfaceView) mActivity.getModuleLayoutRoot()
                .findViewById(R.id.preview_view);
        // mFlashButton = (MultiToggleImageButton)
        // mActivity.findViewById(R.id.video_flash_toggle_button_dream);
        SurfaceHolder surfaceHolder = surfaceView.getHolder();
        if (hasSurface) {
            initCamera(surfaceHolder);
        } else {
            surfaceHolder.addCallback(this);
            surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        }
        decodeFormats = null;
        characterSet = null;

//        playBeep = false;
//        AudioManager audioService = (AudioManager) mActivity
//                .getSystemService(Service.AUDIO_SERVICE);
//        if (audioService.getRingerMode() != AudioManager.RINGER_MODE_NORMAL) {
//            playBeep = false;
//        }
//        initBeepSound();
        vibrate = true;

        mDataModuleCurrent.addListener(this);
    }

    private void onPreviewStarted() {
        mActivity.onPreviewStarted();
        mActivity.getCameraAppUI().onSurfaceTextureUpdated();
        //mActivity.getCameraAppUI().pauseTextureViewRendering();// SPRD:fix bug 474828
    }

    @Override
    public void pause() {
        mCameraAvailable = false;
        if (mDataModuleCurrent != null) {
            mDataModuleCurrent.removeListener(this);
        }

        // super.onPause();
        if (handler != null) {
            handler.quitSynchronously();
            handler = null;
        }

        mRootView.setVisibility(View.INVISIBLE);
        CameraManager.get().closeDriver();
        // super.pause();
    }

    protected void closeCamera() {
        mCameraAvailable = false;
        if (handler != null) {
            handler.quitSynchronously();
            handler = null;
        }
        CameraManager.get().closeDriver();
    }

    @Override
    public void destroy() {
        // inactivityTimer.shutdown();
        // super.onDestroy();
    }

    public void handleDecode(Result result, Bitmap barcode) {
        // inactivityTimer.onActivity();
        playBeepSoundAndVibrate();
        String resultString = result.getText();

        if (resultString.equals("")) {
            resultString = "defaulturi";
        }
        Intent intent = new Intent();
        Bundle bundle = new Bundle();
        bundle.putString("result", resultString);
        intent.putExtras(bundle);
        intent.setClass(mActivity, QrScanResultActivity.class);
        mActivity.startActivity(intent);

    }

    public void enableTorchMode(boolean enable) {

        Camera camera = CameraManager.get().getCamera();
        if (camera == null) {
            return;
        }
        android.hardware.Camera.Parameters parameters = camera.getParameters();
        if (enable) {
            parameters.setFlashMode("torch");
        } else {
            parameters.setFlashMode("off");
        }
        camera.setParameters(parameters);
    }

    public void initCamera(SurfaceHolder surfaceHolder) {
        try {
            CameraManager.get().openDriver(surfaceHolder);
        } catch (IOException ioe) {
            return;
        } catch (RuntimeException e) {
            return;
        }
        qrCameraOpened();
        if (handler == null) {
            handler = new QrCaptureActivityHandler(mActivity, this,
                    decodeFormats, characterSet);
        }

        new Handler().postDelayed(new Runnable() {

            @Override
            public void run() {
                onPreviewStarted();
            }
        },500);

        mCameraAvailable = true;
    }

    @Override
    protected void switchCamera() {

        if (handler != null && handler.getHandler() !=null) {
            if (mFreezeBitmap != null && !mFreezeBitmap.isRecycled()) {
                mFreezeBitmap.recycle();
                mFreezeBitmap = null;
            }

            CameraManager.get().stopPreview();
            Rect previewRect = null;
            RectF previewArea = null;
            if (surfaceView != null) {
                previewRect = surfaceView.getHolder().getSurfaceFrame();
                previewArea = new RectF((float)previewRect.left, (float)previewRect.top, (float)previewRect.right, (float)previewRect.bottom);
            }
            mFreezeBitmap = ((QrDecodeHandler)handler.getHandler()).getPreviewBitmap(/*previewRect*/null);
            mFreezeBitmap = BitmapUtils.rotateBmpToDisplay(mActivity, mFreezeBitmap, 0, 0);

            if (CameraUtil.isFreezeBlurEnable()) {
                mFreezeBitmap = CameraUtil.blurBitmap(CameraUtil.computeScale(mFreezeBitmap, 0.2f), (Context)mActivity);
            }
            mActivity.freezeScreenUntilPreviewReady(mFreezeBitmap, previewArea);
        }

        new Handler().post(new Runnable() {
            @Override
            public void run() {
                if (handler != null) {
                    mActivity.switchFrontAndBackMode();
                    mActivity.getCameraAppUI().updateModeList();
                }
            }
        });
    }

    @Override
    public void onDreamSettingChangeListener(HashMap<String, String> keys) {
        for (String key : keys.keySet()) {
            switch (key) {
            case Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE:

                int state = mFlashButton.getState();
                if (state == 0) {
                    enableTorchMode(true);
                } else {
                    enableTorchMode(false);
                }
                break;
            }
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width,
            int height) {

    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (!hasSurface) {
            hasSurface = true;
            initCamera(holder);
        }

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        hasSurface = false;

    }

    private void qrCameraOpened() {
        updateQrFlash();
    }

    private void updateQrFlash() {
        //SPRD:fix bug 607183 scan picture from gallery, the flash will flash
        if(isUpdateFlash){
            isUpdateFlash = false;
            return;
        }
        if (mFlashButton != null) {
            // mFlashButton.setState(0);
            int state = mFlashButton.getState();
            if (state == 0) {
                enableTorchMode(true);
            } else {
                enableTorchMode(false);
            }
        }
    }

    public ViewfinderView getViewfinderView() {
        return viewfinderView;
    }

    public Handler getHandler() {
        return handler;
    }

    public void drawViewfinder() {
        viewfinderView.drawViewfinder();
    }

//    private void initBeepSound() {
//        if (playBeep && mediaPlayer == null) {
//            // The volume on STREAM_SYSTEM is not adjustable, and users found it
//            // too loud,
//            // so we now play on the music stream.
//            // setVolumeControlStream(AudioManager.STREAM_MUSIC);
//            mediaPlayer = new MediaPlayer();
//            // mediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
//            mediaPlayer.setOnCompletionListener(beepListener);
//            // add volume
//            /*
//             * AssetFileDescriptor file = getResources().openRawResourceFd(
//             * R.raw.beep); try {
//             * mediaPlayer.setDataSource(file.getFileDescriptor(),
//             * file.getStartOffset(), file.getLength()); file.close();
//             * mediaPlayer.setVolume(BEEP_VOLUME, BEEP_VOLUME);
//             * mediaPlayer.prepare(); } catch (IOException e) { mediaPlayer =
//             * null; }
//             */
//        }
//    }

    private static final long VIBRATE_DURATION = 200L;

    private void playBeepSoundAndVibrate() {
//        if (playBeep && mediaPlayer != null) {
//            mediaPlayer.start();
//        }
        if (vibrate) {
            Vibrator vibrator = (Vibrator) mActivity
                    .getSystemService(Service.VIBRATOR_SERVICE);
            vibrator.vibrate(VIBRATE_DURATION);
        }
    }

    /**
     * When the beep has finished playing, rewind to queue up another one.
     */
//    private final OnCompletionListener beepListener = new OnCompletionListener() {
//        public void onCompletion(MediaPlayer mediaPlayer) {
//            mediaPlayer.seekTo(0);
//        }
//    };

    /*
     * SPRD: Fix bug 585415, Close permissions, recent returns to the camera and
     * stop running. @{
     */
    private void checkPermissions() {
        if (!ApiHelper.isMOrHigher()) {
            mHasCriticalPermissions = true;
            return;
        }

        if (mActivity.checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED
                && mActivity
                        .checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
            mHasCriticalPermissions = true;
        } else {
            mHasCriticalPermissions = false;
        }

        if (!mHasCriticalPermissions) {
            // SPRD:fix bug501493,502701,498680,499555 problem caused by
            // permission strategy
            Intent cameraIntent = mActivity.getIntent();
            Bundle data = new Bundle();
            data.putParcelable("cameraIntent", cameraIntent);
            Intent intent = new Intent(mActivity, PermissionsActivity.class);
            /*
             * SPRD:fix bug498680 Open camera from contact,it shows modelistview
             * 
             * @{
             */
            // intent.setAction(getIntent().getAction());
            intent.putExtras(data);
            /* }@ */
            /* SPRD:fix bug519999 Create header photo, pop permission error */
            if (!isCaptureIntent()) {
                mActivity.startActivity(intent);
                mActivity.finish();
            } else {
                mActivity.startActivityForResult(intent, 1);
            }
        }
    }

    private boolean isCaptureIntent() {
        if (MediaStore.ACTION_VIDEO_CAPTURE.equals(mActivity.getIntent()
                .getAction())
                || MediaStore.ACTION_IMAGE_CAPTURE.equals(mActivity.getIntent()
                        .getAction())
                || MediaStore.ACTION_IMAGE_CAPTURE_SECURE.equals(mActivity
                        .getIntent().getAction())) {
            return true;
        } else {
            return false;
        }
    }

    @Override
    public int getModuleTpye() {
        return QR_MODULE;
    }
    /* @} */

    protected boolean mCameraAvailable = false;

    @Override
    public boolean isCameraAvailable(){
        return mCameraAvailable;
    }
}
