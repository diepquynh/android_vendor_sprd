
package com.android.camera.qr;

import java.io.IOException;
import java.util.Vector;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.hardware.Camera;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.Vibrator;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;

import com.android.camera.PermissionsActivity;
import com.android.camera.util.ApiHelper;
import com.google.zxing.BarcodeFormat;
import com.google.zxing.Result;
import com.android.camera.MultiToggleImageButton.OnStateChangeListener;
import com.android.camera.app.AppController;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.ui.RotateImageButton;

import com.android.ex.camera2.portability.CameraCapabilities;
import com.android.ex.camera2.portability.CameraSettings;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;

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

import android.graphics.BitmapFactory;
import android.text.TextUtils;
import android.provider.MediaStore;
import android.database.Cursor;

import java.nio.charset.Charset;
import com.android.camera.MultiToggleImageButton;
import com.android.camera.CameraActivity;
import com.android.camera.ButtonManager;

import android.os.Build;
import android.os.Looper;

public class QrScanActivityCapture extends Activity implements Callback {

    private QrCaptureActivityHandler handler;
    private ViewfinderView viewfinderView;
    private boolean hasSurface;
    private Vector<BarcodeFormat> decodeFormats;
    private String characterSet;
    private InactivityTimer inactivityTimer;
    private MediaPlayer mediaPlayer;
    private boolean playBeep;
    private static final float BEEP_VOLUME = 0.10f;
    private boolean vibrate;

    private CameraCapabilities mCameraCapabilities;
    private CameraSettings mCameraSettings;
    private CameraActivity mActivity;
    private AppController mAppController;
    private CameraProxy mCameraDevice;
    private boolean mPaused;

    private static final int REQUEST_CODE = 234;
    private String photo_path;
    private Bitmap scanBitmap;
    // SPRD: Fix bug 584710, When the screen is unlocked, the flash of the state should be turned on
    private MultiToggleImageButton mFlashButton;
    private SettingsManager mSettingsManager;
    private Boolean mHasCriticalPermissions;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_qrcode_capture);
        CameraManager.init(getApplication());
        viewfinderView = (ViewfinderView) findViewById(R.id.viewfinder_view);
        // SPRD: Fix bug 584710, When the screen is unlocked, the flash of the state should be turned on
        mSettingsManager = new SettingsManager(getApplicationContext());
        MultiToggleImageButton mFlashButton = (MultiToggleImageButton) findViewById(R.id.qrcode_flash_toggle_button);
        MultiToggleImageButton mGalleryButton = (MultiToggleImageButton) findViewById(R.id.qrcode_gallery_toggle_button);

        mFlashButton.setOnStateChangeListener(new OnStateChangeListener() {

            @Override
            public void stateChanged(View view, int state) {
                // SPRD: Fix bug 584710, When the screen is unlocked, the flash of the state should be turned on
                mSettingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_QRSCANCAMERA_FLASH_MODE, state);
                // Update flash parameters.
                if (state > 0) {
                    enableTorchMode(true);
                } else {
                    enableTorchMode(false);
                }

            }
        });
        mGalleryButton.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                scanPicture();

            }
        });
        RotateImageButton mReturnButton = (RotateImageButton) findViewById(R.id.qrcode_back_button);
        mReturnButton.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                QrScanActivityCapture.this.finish();

            }
        });
        hasSurface = false;
        inactivityTimer = new InactivityTimer(this);
    }

    public void scanPicture() {
        Intent innerIntent = new Intent();
        if (Build.VERSION.SDK_INT < 19) {
            innerIntent.setAction(Intent.ACTION_GET_CONTENT);
        } else {
            innerIntent.setAction(Intent.ACTION_OPEN_DOCUMENT);
        }
        innerIntent.setType("image/*");
        Intent wrapperIntent = Intent.createChooser(innerIntent, "select qrcode picture");
        QrScanActivityCapture.this
                .startActivityForResult(wrapperIntent, REQUEST_CODE);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {

        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode == RESULT_OK) {
            switch (requestCode) {
                case REQUEST_CODE:
                     /* SPRD: Fix bug 589142, Two dimensional code to select the image interface, the camera has been stopped running @{ */
                    checkPermissions();
                    if (!mHasCriticalPermissions) {
                        return;
                    }
                    /* @} */
                    String[] proj = {
                            MediaStore.Images.Media.DATA
                    };
                    // Gets the path to the selected image
                    Cursor cursor = getContentResolver().query(data.getData(),
                            proj, null, null, null);
                    if (cursor.moveToFirst()) {
                        int column_index = cursor
                                .getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
                        photo_path = cursor.getString(column_index);
                        if (photo_path == null) {
                            photo_path = Utils.getPath(getApplicationContext(),
                                    data.getData());
                        }
                    }
                    cursor.close();
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            Result result = scanningImage(photo_path);
                            if (result == null) {
                                Looper.prepare();
                                Looper.loop();
                            } else {
                                String recode = recode(result.toString());
                                Intent data = new Intent();
                                data.putExtra("result", recode);
                                setResult(300, data);
                                finish();
                            }
                        }
                    }).start();
                    break;
            }
        }
    }

    // Annlysis part of pictures
    protected Result scanningImage(String path) {
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
        BinaryBitmap binaryBitmap = new BinaryBitmap(new HybridBinarizer(
                source));

        try {
            result = multiFormatReader.decodeWithState(binaryBitmap);
            String content = result.getText();
            handleDecode(result, scanBitmap);
        } catch (NotFoundException e1) {
            // TODO Auto-generated catch block
            e1.printStackTrace();
            Intent intent = new Intent();
            Bundle bundle = new Bundle();
            bundle.putString("result", "defaulturi");
            intent.putExtras(bundle);
            intent.setClass(QrScanActivityCapture.this, QrScanResultActivity.class);
            QrScanActivityCapture.this.startActivity(intent);
        }
        return null;
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
    protected void onResume() {
        super.onResume();
        /* SPRD: Fix bug 585415, Close permissions, recent returns to the camera and stop running. @{ */
        checkPermissions();
        if (!mHasCriticalPermissions) {
            return;
        }
        /* @} */
        SurfaceView surfaceView = (SurfaceView) findViewById(R.id.preview_view);
        mFlashButton = (MultiToggleImageButton) findViewById(R.id.qrcode_flash_toggle_button);
        SurfaceHolder surfaceHolder = surfaceView.getHolder();
        if (hasSurface) {
            initCamera(surfaceHolder);
        } else {
            surfaceHolder.addCallback(this);
            surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        }
        decodeFormats = null;
        characterSet = null;

        playBeep = true;
        AudioManager audioService = (AudioManager) getSystemService(AUDIO_SERVICE);
        if (audioService.getRingerMode() != AudioManager.RINGER_MODE_NORMAL) {
            playBeep = false;
        }
        initBeepSound();
        vibrate = true;
        /* SPRD: Fix bug 584710, When the screen is unlocked, the flash of the state should be turned on @{*/
        if (mFlashButton != null) {
            int state = mSettingsManager.getInteger(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_QRSCANCAMERA_FLASH_MODE, 0);
            mFlashButton.setState(state);
        }
        /* @} */

    }

    @Override
    protected void onPause() {
        super.onPause();
        if (handler != null) {
            handler.quitSynchronously();
            handler = null;
        }
        CameraManager.get().closeDriver();
    }

    @Override
    protected void onDestroy() {
        inactivityTimer.shutdown();
        super.onDestroy();
    }

    public void handleDecode(Result result, Bitmap barcode) {
        inactivityTimer.onActivity();
        playBeepSoundAndVibrate();
        String resultString = result.getText();

        if (resultString.equals("")) {
            resultString = "defaulturi";
        }
        Intent intent = new Intent();
        Bundle bundle = new Bundle();
        bundle.putString("result", resultString);
        intent.putExtras(bundle);
        intent.setClass(QrScanActivityCapture.this, QrScanResultActivity.class);
        QrScanActivityCapture.this.startActivity(intent);

    }

    private final ButtonManager.ButtonCallback mFlashCallback =
            new ButtonManager.ButtonCallback() {
                @Override
                public void onStateChanged(int state) {
                    if (mPaused) {
                        return;
                    }
                    // Update flash parameters.
                    if (state > 0) {
                        enableTorchMode(true);
                    } else {
                        enableTorchMode(false);
                    }
                }
            };

    private void enableTorchMode(boolean enable) {

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

    private void initCamera(SurfaceHolder surfaceHolder) {
        try {
            CameraManager.get().openDriver(surfaceHolder);
        } catch (IOException ioe) {
            return;
        } catch (RuntimeException e) {
            return;
        }
        if (handler == null) {
            handler = new QrCaptureActivityHandler(this, decodeFormats,
                    characterSet);
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
            /* SPRD: Fix bug 584710, When the screen is unlocked, the flash of the state should be turned on @{ */
            if (mSettingsManager.getInteger(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_QRSCANCAMERA_FLASH_MODE, 0) > 0) {
                enableTorchMode(true);
            } else {
                enableTorchMode(false);
            }
            /* @} */
        }

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        hasSurface = false;

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

    private void initBeepSound() {
        if (playBeep && mediaPlayer == null) {
            // The volume on STREAM_SYSTEM is not adjustable, and users found it
            // too loud,
            // so we now play on the music stream.
            setVolumeControlStream(AudioManager.STREAM_MUSIC);
            mediaPlayer = new MediaPlayer();
            mediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
            mediaPlayer.setOnCompletionListener(beepListener);
            //add volume
/*            AssetFileDescriptor file = getResources().openRawResourceFd(
                    R.raw.beep);
            try {
                mediaPlayer.setDataSource(file.getFileDescriptor(),
                        file.getStartOffset(), file.getLength());
                file.close();
                mediaPlayer.setVolume(BEEP_VOLUME, BEEP_VOLUME);
                mediaPlayer.prepare();
            } catch (IOException e) {
                mediaPlayer = null;
            }*/
        }
    }

    private static final long VIBRATE_DURATION = 200L;

    private void playBeepSoundAndVibrate() {
        if (playBeep && mediaPlayer != null) {
            mediaPlayer.start();
        }
        if (vibrate) {
            Vibrator vibrator = (Vibrator) getSystemService(VIBRATOR_SERVICE);
            vibrator.vibrate(VIBRATE_DURATION);
        }
    }

    /**
     * When the beep has finished playing, rewind to queue up another one.
     */
    private final OnCompletionListener beepListener = new OnCompletionListener() {
        public void onCompletion(MediaPlayer mediaPlayer) {
            mediaPlayer.seekTo(0);
        }
    };

    /* SPRD: Fix bug 585415, Close permissions, recent returns to the camera and stop running. @{ */
    private void checkPermissions() {
        if (!ApiHelper.isMOrHigher()) {
            mHasCriticalPermissions = true;
            return;
        }

        if (checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED &&
                checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
            mHasCriticalPermissions = true;
        } else {
            mHasCriticalPermissions = false;
        }

        if (!mHasCriticalPermissions) {
            //SPRD:fix bug501493,502701,498680,499555 problem caused by permission strategy
            Intent cameraIntent = getIntent();
            Bundle data = new Bundle();
            data.putParcelable("cameraIntent", cameraIntent);
            Intent intent = new Intent(this, PermissionsActivity.class);
               /* SPRD:fix bug498680 Open camera from contact,it shows modelistview @{ */
            //intent.setAction(getIntent().getAction());
            intent.putExtras(data);
            /* }@ */
            /*SPRD:fix bug519999 Create header photo, pop permission error*/
            if (!isCaptureIntent()) {
                startActivity(intent);
                finish();
            } else {
                startActivityForResult(intent, 1);
            }
        }
    }

    private boolean isCaptureIntent() {
        if (MediaStore.INTENT_ACTION_VIDEO_CAMERA.equals(getIntent().getAction())
                || MediaStore.ACTION_VIDEO_CAPTURE.equals(getIntent().getAction())
                || MediaStore.ACTION_IMAGE_CAPTURE.equals(getIntent().getAction())
                || MediaStore.ACTION_IMAGE_CAPTURE_SECURE.equals(getIntent().getAction())) {
            return true;
        } else {
            return false;
        }
    }
    /* @} */
}
