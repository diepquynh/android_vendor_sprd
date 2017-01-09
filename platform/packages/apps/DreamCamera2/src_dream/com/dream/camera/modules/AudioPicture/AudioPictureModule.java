
package com.dream.camera.modules.AudioPicture;

import com.android.camera.app.AppController;
import com.android.camera.app.MediaSaver;
import com.android.camera.debug.Log;
import com.android.camera.exif.ExifInterface;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.util.GservicesHelper;
import com.android.camera.util.Size;
import com.android.camera.util.ToastUtil;
import com.android.camera.CameraActivity;
import com.android.camera.PhotoUI;
import com.android.camera2.R;
import android.os.Handler;
import android.os.Looper;
import com.android.ex.camera2.portability.CameraAgent;

import com.dream.camera.DreamModule;
import com.dream.camera.dreambasemodules.DreamPhotoModule;
import com.ucamera.ucam.modules.utils.LogUtils;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.dream.camera.vgesture.VGestureController;
import android.hardware.Camera.Parameters;
import com.android.camera.CameraActivity;
import android.view.View;
import android.view.WindowManager;
import android.view.Gravity;
import android.widget.Toast;
import android.view.Display;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.graphics.RectF;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Canvas;
import com.dream.camera.util.DreamUtil;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.vgesture.VGestureOpenCameraInterface;
import android.location.Location;
import com.sprd.camera.voice.PhotoVoiceMessage;
import com.sprd.camera.voice.PhotoVoiceRecorder;

public class AudioPictureModule extends DreamPhotoModule {
    private static final Log.Tag TAG = new Log.Tag("AudioPictureModule");
    private final Handler mHandler;
    private boolean isAudioRecording = false;
    private PhotoVoiceRecorder mPhotoVoiceRecorder;
    protected AppController mAppController;
    private ImageView mVoicePreview;
    private byte[] mImageData;
    private Bitmap mFreezeScreen;

    public AudioPictureModule(AppController app) {
        super(app);
        mHandler = getHandler();
        mAppController = app;
    }

    @Override
    public PhotoUI createUI(CameraActivity activity) {
        // SPRD: ui check 208
        showRecordVoiceHind();
        return new AudioPictureUI(activity, this, activity.getModuleLayoutRoot());
    }

    public void showRecordVoiceHind() {
        boolean shouldShowRecordVoiceHind = mDataModule.getBoolean(Keys.KEY_CAMERA_RECORD_VOICE_HINT);
        if (shouldShowRecordVoiceHind == true) {
            Toast.makeText(mActivity, R.string.camera_record_voice_tip, Toast.LENGTH_LONG).show();
            mDataModule.set(Keys.KEY_CAMERA_RECORD_VOICE_HINT, false);
        }
    }

    public boolean isSupportTouchAFAE() {
        return true;
    }

    public boolean isSupportManualMetering() {
        return false;
    }

    /* nj dream camera test 24 */
    @Override
    public void destroy() {
        super.destroy();
    }
    /* @} */

    /* SPRD: Fix bug 535110, Photo voice record. @{ */
    public void onStopRecordVoiceClicked() {
        Log.e(TAG, "onStopRecordVoiceClicked isAudioRecording = " + isAudioRecording);
        mHandler.removeMessages(PhotoVoiceMessage.MSG_RECORD_AUDIO);
        if (isAudioRecording) {
            mPhotoVoiceRecorder.stopAudioRecord();
        }
    }

    @Override
    protected void startAudioRecord() {
        if (mFace != null && !mFace.equals(Keys.CAMERA_AI_DATECT_VAL_OFF)) {
            stopFaceDetection();
        }
        boolean result = mPhotoVoiceRecorder.startAudioRecord();
        if (result) {
            isAudioRecording = true;
            ((AudioPictureUI)mUI).showAudioNoteProgress();
            mActivity.getCameraAppUI().updateRecordVoiceUI(View.INVISIBLE);
            freezePreview();
        } else {
            mPhotoVoiceRecorder.savePhoto(null);
            mAppController.getCameraAppUI().setBottomPanelLeftRightClickable(true);
        }
    }

    @Override
    protected void onAudioRecordStopped() {
        Log.i(TAG,"onAudioRecordStopped");
        if (mUI != null) {
            ((AudioPictureUI)mUI).hideAudioNoteProgress();
            mActivity.getCameraAppUI().updateRecordVoiceUI(View.VISIBLE);
            mUI.enablePreviewOverlayHint(true);
            mAppController.getCameraAppUI().setBottomPanelLeftRightClickable(true);
            if (mVoicePreview != null) {
                mVoicePreview.setVisibility(View.GONE);
            }
            if (mFreezeScreen != null && !mFreezeScreen.isRecycled()) {
                mFreezeScreen.recycle();
                mFreezeScreen = null;
            }
        }
        if (mFace != null && !mFace.equals(Keys.CAMERA_AI_DATECT_VAL_OFF)) {
            startFaceDetection();
        }
        isAudioRecording = false;
        mHandler.removeMessages(PhotoVoiceMessage.MSG_RECORD_STOPPED);
    }

    @Override
    public boolean  isAudioRecording() {
        return isAudioRecording;
    }

    @Override
    protected void initData(byte[] jpegData, String title, long date,
            int width, int height, int orientation, ExifInterface exif,
            Location location,
            MediaSaver.OnMediaSavedListener onMediaSavedListener) {
        mImageData = jpegData;
        mPhotoVoiceRecorder.initData(jpegData, title, date, width, height,
                orientation, exif, location, onMediaSavedListener,
                getServices().getMediaSaver());
        mAppController.getCameraAppUI().setBottomPanelLeftRightClickable(false);
        ((AudioPictureUI)mUI).setTopPanelVisible(View.INVISIBLE);
        /* SPRD: Fix bug615557 that photo voice recorded the shutter sound @{ */
        if (mDataModule.getBoolean(
                Keys.KEY_CAMERA_SHUTTER_SOUND)) {
            mHandler.sendEmptyMessageDelayed(PhotoVoiceMessage.MSG_RECORD_AUDIO, 680);
        } else {
            mHandler.sendEmptyMessageDelayed(PhotoVoiceMessage.MSG_RECORD_AUDIO, 300);
        }
        /* @} */
    }
    @Override
    public void resume() {
        super.resume();
        mPhotoVoiceRecorder = new PhotoVoiceRecorder(mAppController);
        mPhotoVoiceRecorder.setHandler(mHandler);
    }

    @Override
    public void pause() {
        super.pause();
        mHandler.removeMessages(PhotoVoiceMessage.MSG_RECORD_AUDIO);
        if (isAudioRecording) {
            mPhotoVoiceRecorder.stopAudioRecord();
            onAudioRecordStopped();
        }
    }

    @Override
    public void onShutterButtonClick() {
        if (isPhoneCalling()) {
            Log.i(TAG, "Audio picture won't start due to telephone is running");
            ToastUtil.showToast(mActivity, R.string.phone_does_not_support_audio_picture,
                    Toast.LENGTH_LONG);
            return;
        }
        /*SPRD Fix Bug #637017 the icon may be mess when audiopicturemodule@{*/
        if (!mAppController.getCameraAppUI().isShutterButtonClickable()) {
            return;
        }
        /*@}*/
        super.onShutterButtonClick();
    }

    @Override
    public boolean onBackPressed() {
        mHandler.removeMessages(PhotoVoiceMessage.MSG_RECORD_AUDIO);
        if (isAudioRecording) {
            mPhotoVoiceRecorder.stopAudioRecord();
            return true;
        } else {
            mHandler.removeMessages(PhotoVoiceMessage.MSG_RECORD_STOPPED);
        }
        return super.onBackPressed();
    }

    private void freezePreview() {
        mVoicePreview = mActivity.getCameraAppUI().getVoicePreview();
        setVoiceReviewParams();
        mVoicePreview.setImageBitmap(makeBitmap(mActivity.getCameraAppUI().getPreviewShotWithoutTransform()));
        mVoicePreview.setVisibility(View.VISIBLE);
    }

    private void setVoiceReviewParams() {
        Size size = new Size(mCameraSettings.getCurrentPreviewSize());
        float aspectRatio= (float)size.getWidth() / size.getHeight();
        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) mVoicePreview.getLayoutParams();
        RectF rect = mActivity.getCameraAppUI().getPreviewArea();
        if (rect == null) {
            return;
        }
        WindowManager wm = mActivity.getWindowManager();
        Display display = wm.getDefaultDisplay();
        int width = display.getWidth();
        int height = display.getHeight();
        if (width > height) {
            height = width;
        }
        if (aspectRatio > 1.5) {
            params.setMargins(0, 0, 0, 0);
        } else {
            params.setMargins(0, (int)rect.top, 0, (int)(height - rect.right * 4 / 3 - rect.top));
        }
        mVoicePreview.setLayoutParams(params);
    }

    public Bitmap makeBitmap(Bitmap bitmap) {
        int width = bitmap.getWidth();
        int height = bitmap.getHeight();
        int previewWidth = (int)mActivity.getCameraAppUI().getPreviewArea().width();
        int previewHeight = (int)mActivity.getCameraAppUI().getPreviewArea().height();
        int scalevalue = previewWidth > previewHeight ? previewWidth : previewHeight;
        float scaleHeight = ((float) scalevalue) / height ;
        Matrix matrix = new Matrix();
        matrix.postScale(1, scaleHeight);
        mFreezeScreen = Bitmap.createBitmap(bitmap, 0, 0, width, height, matrix, true);
        return mFreezeScreen;
    }

    @Override
    public void onSingleTapUp(View view, int x, int y) {
        if (isAudioRecording || !mAppController.getCameraAppUI().isShutterButtonClickable()) {
            return;
        }
        super.onSingleTapUp(view, x, y);
    }
    /* @} */
    @Override
    public int getModuleTpye() {
        return DreamModule.AUDIOPICTURE_MODULE;
    }
}
