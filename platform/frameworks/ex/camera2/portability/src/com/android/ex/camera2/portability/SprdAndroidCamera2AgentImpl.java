package com.android.ex.camera2.portability;


import java.util.ArrayList;
import java.util.List;

import com.android.ex.camera2.portability.AndroidCamera2AgentImpl;
import com.android.ex.camera2.portability.AndroidCamera2Capabilities;
import com.android.ex.camera2.portability.CameraActions;
import com.android.ex.camera2.portability.CameraDeviceInfo;
import com.android.ex.camera2.portability.CameraExceptionHandler;
import com.android.ex.camera2.portability.CameraSettings;
import com.android.ex.camera2.portability.DispatchThread;
import com.android.ex.camera2.portability.SprdAndroidCamera2Settings;
import com.android.ex.camera2.portability.AndroidCamera2AgentImpl.CaptureAvailableListener;
import com.android.ex.camera2.portability.debug.Log;

import android.content.Context;
import android.graphics.Rect;
import android.hardware.Camera;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import static android.hardware.camera2.SprdCaptureRequest.SPRD_CAPTURE_MODE;
import android.hardware.camera2.CameraCaptureSessionEx;
import android.hardware.camera2.CaptureResult;
import android.media.MediaActionSound;
import android.os.HandlerThread;
import android.os.Message;
import android.os.Handler;
import android.os.Looper;

public class SprdAndroidCamera2AgentImpl extends AndroidCamera2AgentImpl {

    private static final Log.Tag TAG = new Log.Tag("SprdAndCam2AgntImp");
    protected int mCanceledCaptureCount;//SPRD:fix bug 497854 when cancel 10 burst capture,the count of pics saveing is wrong

    SprdAndroidCamera2AgentImpl(Context context) {
        super(context);
        mSprdAgentImpl = this;
        mCameraHandler = new SprdCamera2Handler(mCameraHandlerThread.getLooper());
        mExceptionHandler = new CameraExceptionHandler(mCameraHandler);
        mDispatchThread.setHandler(mCameraHandler);
    }

    /**
     * SPRD: fix bug 402668/400619 @{
     */
    public void recycle() {
//        if(mNoisemaker != null){
//            mNoisemaker.release();
//            Log.i(TAG,"mNoisemaker release!");
//        }
        closeCamera(null, true);
        mDispatchThread.end();
    }
    /**
     * @}
     */

    @Override
    public void setCameraExceptionHandler(CameraExceptionHandler exceptionHandler) {
        mExceptionHandler = exceptionHandler != null ? exceptionHandler : sDefaultExceptionHandler;
    }

    /* SPRD: fix bug542668 NullPointerException @{ */
    protected static final CameraExceptionHandler sDefaultExceptionHandler =
            new CameraExceptionHandler(null) {
        @Override
        public void onCameraError(int errorCode) {
            Log.w(TAG, "onCameraError called with no handler set: " + errorCode);
        }

        @Override
        public void onCameraException(RuntimeException ex, String commandHistory, int action,
                int state) {
            Log.w(TAG, "onCameraException called with no handler set", ex);
        }

        @Override
        public void onDispatchThreadException(RuntimeException ex) {
            Log.w(TAG, "onDispatchThreadException called with no handler set", ex);
        }
    };
    /* @} */

    public static class FaceDetectionCallbackForward implements CameraFaceDetectionCallback {
        private final Handler mHandler;
        private final CameraFaceDetectionCallback mCallback;
        private final CameraAgent.CameraProxy mCamera;

        public static FaceDetectionCallbackForward getNewInstance(Handler handler,
                CameraProxy camera, CameraFaceDetectionCallback cb) {
            if (handler == null || camera == null || cb == null)
                return null;
            return new FaceDetectionCallbackForward(handler, camera, cb);
        }

        private FaceDetectionCallbackForward(Handler h, CameraAgent.CameraProxy camera,
                CameraFaceDetectionCallback cb) {
            mHandler = h;
            mCamera = camera;
            mCallback = cb;
        }

        @Override
        public void onFaceDetection(final Camera.Face[] faces, CameraAgent.CameraProxy camera) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mCallback.onFaceDetection(faces, mCamera);
                }
            });
        }
    }

    public void monitorControlStatesAIDetect(CaptureResult result, AndroidCamera2ProxyImpl cameraProxy,
            Rect activeArray) {
        Integer faceState = result.get(CaptureResult.STATISTICS_FACE_DETECT_MODE);
        if (faceState != null) {
            int aeState = faceState;
            switch (aeState) {
                case CaptureResult.STATISTICS_FACE_DETECT_MODE_SIMPLE:
                    android.hardware.camera2.params.Face[] faces = result
                            .get(CaptureResult.STATISTICS_FACES);
                    Camera.Face[] cFaces = new Camera.Face[faces.length];
                    for (int i = 0; i < faces.length; i++) {
                        Camera.Face face = new Camera.Face();
                        face.score = faces[i].getScore();
                        face.rect = faceForConvertCoordinate(activeArray, faces[i].getBounds());
                        cFaces[i] = face;
                    }
                    if (mCameraHandler instanceof SprdCamera2Handler) {
                        SprdCamera2Handler handler = (SprdCamera2Handler)mCameraHandler;
                        if (handler.getFaceDetectionListener() != null) {
                            handler.getFaceDetectionListener().onFaceDetection(cFaces, cameraProxy);
                        }
                    }

                    break;
            }
        }
    }

    public Rect faceForConvertCoordinate(Rect activeArray, Rect rect) {
        if (activeArray == null) {
            return null;
        }
        int sensorWidth = activeArray.width();
        int sendorHeight = activeArray.height();
        int left = rect.left * 2000/sensorWidth - 1000;
        int top = rect.top * 2000/sendorHeight - 1000;
        int right = rect.right * 2000/sensorWidth - 1000;
        int bottom = rect.bottom * 2000/sendorHeight -1000;
        return new Rect(left,top,right,bottom);
    }

    public class SprdCamera2Handler extends AndroidCamera2AgentImpl.Camera2Handler {
        private FaceDetectionCallbackForward mFaceDetectionCallback;

        SprdCamera2Handler(Looper looper) {
            super(looper);
        }

        public void setFaceDetectionListener(FaceDetectionCallbackForward listener) {
            mFaceDetectionCallback = listener;
        }

        public FaceDetectionCallbackForward getFaceDetectionListener() {
            return mFaceDetectionCallback;
        }

        public void handleMessage(final Message msg) {
            super.handleMessage(msg);
            int cameraAction = msg.what;
            try {
                switch (cameraAction) {
                    case CameraActions.SET_FACE_DETECTION_LISTENER: {
                        setFaceDetectionListener((FaceDetectionCallbackForward) msg.obj);
                        break;
                    }

                    case CameraActions.START_FACE_DETECTION: {
                        boolean isFace = mPersistentSettings.set(
                                CaptureRequest.STATISTICS_FACE_DETECT_MODE,
                                CaptureRequest.STATISTICS_FACE_DETECT_MODE_SIMPLE);
                        break;
                    }

                    case CameraActions.STOP_FACE_DETECTION: {
                        boolean isFace = mPersistentSettings.set(
                                CaptureRequest.STATISTICS_FACE_DETECT_MODE,
                                CaptureRequest.STATISTICS_FACE_DETECT_MODE_OFF);
                        break;
                    }

                    /* SPRD: fix bug 473462 add burst capture @{ */
                    case CameraActions.CAPTURE_BURST_PHOTO: {
                        int num = mPersistentSettings.get(SPRD_CAPTURE_MODE);
                        Log.i(TAG,"CameraActions.CAPTURE_BURST_PHOTO num="+num);
                        List<CaptureRequest> requests = new ArrayList<CaptureRequest>();
                        final CaptureAvailableListener listener = (CaptureAvailableListener) msg.obj;
                        mCaptureReader.setOnImageAvailableListener(listener, /*handler*/this);

                        for (int i = 0; i < num; i++) {
                            Log.i(TAG,"CameraActions.CAPTURE_BURST_PHOTO i="+i);
                            CaptureRequest request = mPersistentSettings.createRequest(
                                    mCamera, CameraDevice.TEMPLATE_STILL_CAPTURE,
                                    mPreviewSurface,mCaptureReader.getSurface());
                            requests.add(request);
                        }

                        Log.i(TAG,"mSession.captureBurst size " + requests.size());
                        mSession.captureBurst(requests, listener, /*mHandler*/this);

                        break;
                    }

                    case CameraActions.CANCEL_CAPTURE_BURST_PHOTO: {
                        mCaptureCancled = true;
                        Log.i(TAG,"CameraActions.CANCEL_CAPTURE_BURST_PHOTO");
                        //mSession.abortCaptures();
                        /*
                         * SPRD:fix bug 497854 when cancel 10 burst capture,the count of pics
                         * saveing is wrong @{
                         */
                        CancelBurstCaptureCallback cb = (CancelBurstCaptureCallback) msg.obj;
                        /* SPRD:fix bug 644311 cancel picture should not called when cancel burst
                        if (mSession instanceof CameraCaptureSessionEx) {
                            mCanceledCaptureCount = ((CameraCaptureSessionEx)mSession).cancelPicture();
                        }
                        */
                        cb.onCanceled(mContinueNum - 1);
                        /* @} */

                        break;
                    }

                    default: {
                        // TODO: Rephrase once everything has been implemented
                        throw new RuntimeException("Unimplemented CameraProxy message=" + msg.what);
                    }
                }
            } catch (Exception ex) {
                Log.i(TAG, "Unable to run " + cameraAction, ex);
            }
        }

        public CameraSettings buildSettings(AndroidCamera2Capabilities caps) {
            try {
                return new SprdAndroidCamera2Settings(mCamera, CameraDevice.TEMPLATE_PREVIEW,
                        mActiveArray, mPreviewSize, mPhotoSize);
            } catch (CameraAccessException ex) {
                Log.e(TAG, "Unable to query camera device to build settings representation");
                return null;
            }
        }
    }

    protected class SprdAndroidCamera2ProxyImpl extends AndroidCamera2AgentImpl.AndroidCamera2ProxyImpl {

        public SprdAndroidCamera2ProxyImpl(
                AndroidCamera2AgentImpl agent,
                int cameraIndex,
                CameraDevice camera,
                CameraDeviceInfo.Characteristics characteristics,
                CameraCharacteristics properties) {
            super(agent, cameraIndex, camera, characteristics, properties);
        }

        @Override
        public void setFaceDetectionCallback(Handler handler, CameraFaceDetectionCallback callback) {
            mCameraHandler.obtainMessage(
                    CameraActions.SET_FACE_DETECTION_LISTENER,
                    FaceDetectionCallbackForward.getNewInstance(handler,
                            SprdAndroidCamera2ProxyImpl.this, callback)).sendToTarget();
        }

        @Override
        public void startFaceDetection() {
            mCameraHandler.sendEmptyMessage(CameraActions.START_FACE_DETECTION);
        }

        @Override
        public void stopFaceDetection() {
            mCameraHandler.sendEmptyMessage(CameraActions.STOP_FACE_DETECTION);
        }

        /* SPRD:fix bug 497854 when cancel 10 burst capture,the count of pics saveing is wrong {@ */
        @Override
        public int cancelBurstCapture(CancelBurstCaptureCallback cb) {
            Log.i(TAG, "cancelBurstCapture");
            getCameraHandler().sendMessageAtFrontOfQueue(
                    getCameraHandler().obtainMessage(CameraActions.CANCEL_CAPTURE_BURST_PHOTO, cb));
            return mCanceledCaptureCount;
        }
        /* @} */
    }

}
