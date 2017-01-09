package com.sprd.hz.selfportrait.detect;
import java.util.ArrayList;

import com.android.camera.debug.Log;
import com.android.camera.util.CameraUtil;
import com.android.ex.camera2.portability.CameraAgent;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;
import com.ucamera.ucam.modules.utils.LogUtils;
import  com.sprd.hz.selfportrait.util.ContextUtil;
import com.sprd.hz.selfportrait.util.Util;
import android.annotation.TargetApi;
import android.graphics.Matrix;
import android.graphics.Matrix.ScaleToFit;
import android.graphics.Rect;
import android.graphics.RectF;
import android.hardware.Camera;
import android.hardware.Camera.Face;
import android.hardware.Camera.Size;
import android.os.Build;
import android.os.Handler;
import com.thundersoft.hz.selfportrait.detect.NV21Sampler;
import com.thundersoft.hz.selfportrait.detect.GestureDetect;

public class DetectEngine {
    private static String TAG = "DetectEngine";
    public static final int DETECT_TYPE_FACE     = 0x00000001;
    public static final int DETECT_TYPE_GESTURE    = 0x00000004;
    private Object mLock = new Object();
    private CameraProxy mCamera = null;
    private int mDetectType = 0;
    private GestureDetect mGestureDetector = null;
    private ArrayList<NV21Sampler> mSamplersList = new ArrayList<NV21Sampler>();
//    private NV21Sampler mNv21Sampler = null;
    private ArrayList<OnDetectListener> mlstDetectListener = null;
    private int mPreviewWidth,mPreviewHeight;
    private int mDetectWidth, mDetectHeight;
    private int mDetectSample = 0;
    private int mDisplayWidth,mDisplayHeight;
    private int mDisplayOrientation;
    private int mDeviceRotation;
    private int mDetectRotation;
    private boolean mbMirror;
    private Matrix mMatDisplay = null;
    private Matrix mMatHWDetect = null;
    // SPRD: remove callback buffer setting, or continuous focus mode can not work
    //private byte[][] mPreviewBuffer = null;
    //private int mBufIndex = 0;
    private Rect[] mFaces = null;
    private boolean mIsStarted = false;
    private boolean mIsAppendToStart = false;
    private boolean mIsHWDetected = false;
    private boolean mIsFrameExternal = false;
    private PreviewDetectThread mPreviewDetectThread = null;
    private Handler mHandler = null;
    private final RectF mPreviewArea = new RectF();

    private static final boolean DEBUG = false;

    public DetectEngine() {
        try{
            System.loadLibrary("self_portrait_jni");
        }catch(UnsatisfiedLinkError e){
            e.printStackTrace();
        }
        mMatDisplay = new Matrix();
        mMatHWDetect = new Matrix();
        mlstDetectListener = new ArrayList<OnDetectListener>();
//        mNv21Sampler = new NV21Sampler();
    }

    public void setFrameCallbackExternal(boolean bExternal) {
        mIsFrameExternal = bExternal;
    }

    public void setHandler(Handler h){
    	mHandler = h;
    }

    public void destroy() {
        stopDetect();
        if (mGestureDetector!=null) {
            mGestureDetector.uninitialize();
            mGestureDetector = null;
        }
        for (int i = 0; i < mSamplersList.size(); i++) {
            NV21Sampler temp = mSamplersList.get(i);
            if(temp != null){
                temp.destroy();
            }
        }
//        mNv21Sampler.destroy();
        mlstDetectListener.clear();
    }

    public void setDisplaySize(int width, int height) {
        mDisplayWidth = width;
        mDisplayHeight = height;
        refreshTransform();
    }

    public void setPreviewSize(int width, int height){
        mPreviewWidth = width;
        mPreviewHeight = height;
        refreshTransform();
    }
    
    public void setDisplayOritation(int degrees) {
        mDisplayOrientation = degrees;
        refreshTransform();
    }

    public void setDeviceRotation(int degrees) {
        mDeviceRotation = degrees;
        refreshTransform();
    }

    public void setMirror(boolean bMirror) {
        mbMirror = bMirror;
        refreshTransform();
    }

    private void refreshTransform() {
        if (mDisplayWidth==0 || mPreviewHeight==0) {
            return;
        }
        mDetectSample = mPreviewWidth/320;
        mDetectWidth = mPreviewWidth/mDetectSample;
        mDetectHeight = mPreviewHeight/mDetectSample;
        if (mbMirror) {
            mDetectRotation = (720-mDeviceRotation-mDisplayOrientation)%360;
        } else {
            mDetectRotation = (mDeviceRotation+mDisplayOrientation)%360;
        }
        if (mDetectRotation%180!=0) {
            int temp = mDetectWidth;
            mDetectWidth = mDetectHeight;
            mDetectHeight = temp;
        }
        mMatDisplay.reset();
        int maxDetect = Math.max(mDetectWidth,mDetectHeight);
        int minDetect = Math.min(mDetectWidth,mDetectHeight);
        float sx,sy;
        mMatDisplay.postTranslate(-mDetectWidth/2.0f, -mDetectHeight/2.0f);
        mMatDisplay.postRotate(360-mDetectRotation);
        mMatDisplay.postScale(mbMirror ? -1 : 1, 1);
        mMatDisplay.postRotate(mDisplayOrientation);
        sx = (float)mDisplayWidth/minDetect;
        sy = (float)mDisplayHeight/maxDetect;
        mMatDisplay.postScale(sx, sy);
        mMatDisplay.postTranslate(mDisplayWidth / 2f, mDisplayHeight / 2f);
        LogUtils.debug("yyyyy", "Disp(%dx%d)Rotate=%d Prev(%dx%d)Rotate=%d", mDisplayWidth,mDisplayHeight,mDisplayOrientation, mPreviewWidth,mPreviewHeight,mDetectRotation);
        mMatHWDetect.reset();
        mMatHWDetect.setRectToRect(new RectF(-1000,-1000,1000, 1000),
                new RectF(0, 0, maxDetect, minDetect),
                ScaleToFit.FILL);
        mMatHWDetect.postTranslate( -maxDetect/2.0f, -minDetect/2.0f);
        mMatHWDetect.postRotate(mDetectRotation);
        mMatHWDetect.postTranslate(mDetectWidth/2.0f,mDetectHeight/2.0f);
        LogUtils.debug(TAG,"mDetectWidth = "+mDetectWidth+"mDetectHeight = "+mDetectHeight);
    }

    public synchronized Rect[] detectGesture(byte[] nv21, int width, int height, Rect[] faces) {
        if (mGestureDetector==null) {
            mGestureDetector = new GestureDetect();
            mGestureDetector.initialize();
        }
        if (faces==null || faces.length<1 || faces[0]==null) {
            return null;
         }
        //This may be in none-UI thread. (Detect Thread)
        //Avoid that any of mFaces��s elements is null while gesture detecting.
//        synchronized(mLock) {
//            mGestureDetector.setFaces(faces);
//        }
        Rect[] res =  mGestureDetector.detect(nv21, width, height,faces);
        String gest = "null";
        String face = "null";
        if (faces!=null && faces.length>0) face = faces[0].toString();
        if (res!=null && res.length>0) gest = res[0].toString();
        if (DEBUG) {
            LogUtils.debug(TAG, "detectGesture face=" + face + " gest=" + gest);
        }
        mFaces = null;
        return res;
    }

    /**
     * The detect type can be DETECT_TYPE_FACE/DETECT_TYPE_GESTURE,
     * Or combine of them.
     * @param type
     */
    public void setDetectType(int type) {
        if (mDetectType==type) {
            return;
        }
        boolean needRestart = false;
        if (mDetectType==DETECT_TYPE_FACE && mIsHWDetected) {
            needRestart = true;
        }
        if (type==DETECT_TYPE_FACE && mIsHWDetected) {
            needRestart = true;
        }
        mDetectType = type;
        if (mIsStarted && needRestart) {
            stopDetect();
            startDetect(mCamera,mPreviewWidth,mPreviewHeight);
        } else if (!mIsStarted && mIsAppendToStart) {
        	startDetect(mCamera,mPreviewWidth,mPreviewHeight);
        }
    }

    public int getDetectType() {
        return mDetectType;
    }

    public void addOnDetectListener(OnDetectListener l) {
        if (mlstDetectListener.contains(l)) {
            return;
        }
        mlstDetectListener.add(l);
    }

    public void startDetect(CameraAgent.CameraProxy camera,int PreviewWidth,int PreviewHeight) {
        mCamera = camera;
//        mCamera.setPreviewDataCallback(mHandler, new MagiclensFrameCallback());
//        Size previewSize = camera.getParameters().getPreviewSize();
//        mPreviewWidth = previewSize.width;
//        mPreviewHeight = previewSize.height;
        mPreviewWidth = PreviewWidth;
        mPreviewHeight = PreviewHeight;
//        mIsHWDetected = isSupportHWFaceDetect(camera);
        refreshTransform();
        if (mIsStarted) {
            return;
        }
        if (mDetectType==0) {
            mIsAppendToStart = true;
            return;
        }
//        if (mIsHWDetected) {
//            startHWFaceDetect(camera);
//        }
        mIsStarted = true;
//        LogUtils.debug(TAG,"mDetectType&DETECT_TYPE_FACE = "+(mDetectType&DETECT_TYPE_FACE)+"   mDetectType&DETECT_TYPE_GESTURE = "+(mDetectType&DETECT_TYPE_GESTURE));
        if ((mDetectType&DETECT_TYPE_GESTURE)!=0) {
            Util.Assert(mPreviewDetectThread==null);
            mPreviewDetectThread = new PreviewDetectThread();
            mPreviewDetectThread.start();
            if (!mIsFrameExternal) {
                // SPRD: remove callback buffer setting, or continuous focus mode can not work
                //mPreviewBuffer = new byte[][]{
                //    new byte[mPreviewWidth*mPreviewHeight*3/2],
                //    new byte[mPreviewWidth*mPreviewHeight*3/2],
                //    new byte[mPreviewWidth*mPreviewHeight*3/2]
                //};
                //camera.addCallbackBuffer(mPreviewBuffer[mBufIndex]);
                //camera.setPreviewDataCallbackWithBuffer(mHandler, new MagiclensFrameCallback());
                camera.setPreviewDataCallback(mHandler, new MagiclensFrameCallback());
            }
        }
    }

    public void stopDetect() {
//        if (mIsStarted && mIsHWDetected) {
//            stopHWFaceDetect(mCamera);
//        }
        if (!mIsStarted) {
            return;
        }
        mIsStarted = false;
        mIsAppendToStart = false;
        if (mCamera!=null && !mIsFrameExternal) {
            try {
                // SPRD: remove callback buffer setting, or continuous focus mode can not work
                //mCamera.setPreviewDataCallbackWithBuffer(null,null);
                mCamera.setPreviewDataCallback(null,null);
            } catch(Exception e) {
                e.printStackTrace();
            }
        }
        if (mPreviewDetectThread!=null) {
            mPreviewDetectThread.terminate();
            Util.joinThreadSilently(mPreviewDetectThread);
            mPreviewDetectThread = null;
        }
        for (OnDetectListener l: mlstDetectListener) {
            l.onDetectFace(null);
            l.onDetectGesture(null);
        }
        // SPRD: remove callback buffer setting, or continuous focus mode can not work
        //mPreviewBuffer = null;
    }

    @TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
    private boolean isSupportHWFaceDetect(Camera camera) {
//        if (Build.VERSION.SDK_INT<Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
//            return false;
//        }
//        Parameters param = camera.getParameters();
//        int fdNum = param.getMaxNumDetectedFaces();
//        if (fdNum>0) {
//            return true;
//        }
        return false;
    }

//    @TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
//    private void startHWFaceDetect(Camera camera) {
//        camera.setFaceDetectionListener(new Camera.FaceDetectionListener() {
//            @Override
//            public void onFaceDetection(Face[] faces, Camera camera) {
//                //This is in UI thread.
//                //Avoid that any of mFaces��s elements is null while gesture detecting.
//                synchronized(mLock) {
//                    mFaces = new Rect[faces.length];
//                    for(int i=0; i<mFaces.length; i++) {
//                        mFaces[i] = faces[i].rect;
//                        RectF temp = new RectF(faces[i].rect);
//                        mMatHWDetect.mapRect(temp);
//                        mFaces[i] = new Rect((int)temp.left, (int)temp.top, (int)temp.right, (int)temp.bottom);
//                    }
//                }
//                if (mIsStarted) {
//                    for(OnDetectListener l: mlstDetectListener) {
//                        l.onDetectFace(mapRects(mFaces));
//                    }
//                }
//            }
//        });
//        camera.startFaceDetection();
//    }
//    @TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
//    private void stopHWFaceDetect(Camera camera) {
//        camera.stopFaceDetection();
//    }

    private class PreviewDetectThread extends Thread {
        private boolean mRunning = false;
        private byte[] mFrame = ContextUtil.getInstance().data;
        private byte[] mDetectBuffer = null;
        private NV21Sampler mNv21Sampler = null;

        public PreviewDetectThread(){
            mNv21Sampler = new NV21Sampler();
            mSamplersList.add(mNv21Sampler);
        }
        
        public void onPreviewFrame(byte[] data) {
            if (data==null) {
                return;
            }
            mFrame = data;
            interrupt();
        }

        @Override
        public void start() {
        	LogUtils.debug(TAG,"PreviewDetectThread start");
            mRunning = true;
            setName("PreviewDetectThread");
            super.start();
        }

        public void terminate() {
            mSamplersList.remove(mNv21Sampler);
            mNv21Sampler.destroy();
        	LogUtils.debug(TAG,"PreviewDetectThread terminate");
            mRunning = false;
            interrupt();
        }

        private void ensureDetectBufferSize() {
            int requiredSize = mDetectWidth*mDetectHeight*3/2;
            LogUtils.debug("yyyyy", "mDetectWidth = " + mDetectWidth + "mDetectHeight = " + mDetectHeight);
//            if (mDetectBuffer==null || mDetectBuffer.length<requiredSize) {
            	LogUtils.debug("yyyyy","mDetectBuffer new = "+mDetectWidth+"     "+mDetectHeight);
                mDetectBuffer = new byte[requiredSize];
//            }
        }

        @Override
        public void run() {
            while(mRunning) {
                while(mRunning && mFrame==null) {
                    try {
                        Thread.sleep(50);
                    } catch (InterruptedException e) {
                        break;
                    }
                }
                if (!mRunning) break;
                if(!mIsStarted){
                    continue;
                }
                byte[] nv21 = mFrame;
                if (nv21==null || nv21.length==0) {
                    String len = "null";
                    if (nv21!=null) {
                        len = String.valueOf(nv21.length);
                    }
                    LogUtils.debug(TAG, "ERROR: Frame width=%d height=%d nv21 length=%s", mPreviewWidth, mPreviewHeight, len);
                    continue;
                }
//                mDetectWidth = mPreviewWidth;
//                mDetectHeight = mPreviewHeight;
//                Util.dumpNv21ToJpeg(nv21, mPreviewWidth, mPreviewHeight, "/sdcard/zhl.jpg");
                ensureDetectBufferSize();
//                Util.dumpToFile(nv21, "/sdcard/DCIM/D.yuv");
//                Util.dumpNv21ToJpeg(nv21, mPreviewHeight, mPreviewWidth, "/sdcard/DCIM/D.JPG");

                LogUtils.debug("yyyyy", "Frame width=%d height=%d ", mPreviewWidth, mPreviewHeight);
                LogUtils.debug("yyyyy","mDetectRotation = "+ mDetectRotation +"mPreviewWidth = "+mPreviewWidth+"mPreviewHeight = "+mPreviewHeight+"mDetectBuffer = "+mDetectBuffer.length+"mDetectSample = "+mDetectSample);
                mNv21Sampler.downSample(nv21, mPreviewWidth, mPreviewHeight, mDetectBuffer, mDetectSample, mDetectRotation);
                LogUtils.debug("yyyyy", "end............... ");
//                mNv21Sampler.downSample(nv21, mPreviewHeight, mPreviewWidth, mDetectBuffer, mDetectSample, 270);
                nv21 = mDetectBuffer;

//                Util.dumpToFile(nv21, "/sdcard/zhl_"+mDetectWidth+"x"+mDetectHeight+".nv21");
                Rect[] gestures = null;
                Rect[] smiles = null;
//                Util.dumpNv21ToJpeg(nv21, mDetectWidth, mDetectHeight, "/sdcard/zhl.jpg");
                if ((mDetectType&DETECT_TYPE_GESTURE)!=0) {
                    gestures = detectGesture(nv21, mDetectWidth, mDetectHeight, mFaces);
                    if (mRunning) {
                        for(OnDetectListener l: mlstDetectListener) {
                            l.onDetectGesture(mapRects(gestures));
                        }
                    }
                }
                if (mDumpPath!=null) {
                    Util.dumpToFile(nv21, mDumpPath);
                    Util.dumpNv21ToJpeg(nv21, mPreviewWidth, mPreviewHeight, mDumpPath+".jpg");
                    mDumpPath = null;
                    String face = "null";
                    String gesture = null;
                    if (mFaces!=null && mFaces.length>0) face = mFaces[0].toString();
                    if (gestures!=null && gestures.length>0) gesture = gestures[0].toString();
                    LogUtils.debug("dumpPreviewToFile", "face=%s gesture=%s", face, gesture);
                }
                mFrame = null;
                mDetectBuffer = null;
                /* SPRD: remove callback buffer setting, or continuous focus mode can not work @{
                if (mRunning && !mIsFrameExternal) {
                    mBufIndex = (mBufIndex+1)%mPreviewBuffer.length;
                    mCamera.addCallbackBuffer(mPreviewBuffer[mBufIndex]);
                }
                @} */
            }
        }
    }

    public Rect[] changeFaces(Rect[] vrect){
        if (vrect == null||vrect.length == 0){
            return null;
        }
        Rect[] rect = new Rect[vrect.length];
        for(int i = 0; i < vrect.length; i++){
            rect[i] = new Rect();
            rect[i].set(vrect[i].left/mDetectSample,vrect[i].top/mDetectSample,vrect[i].right/mDetectSample,vrect[i].bottom/mDetectSample);
        }
        return rect;
    }

    private Rect[] mapRects(Rect[] src) {
        if (src==null || src.length<1) {
            return null;
        }
   
        Rect[] res = new Rect[src.length];
        RectF temp = new RectF();
        for(int i=0; i<src.length; i++) {
            Rect rect = src[i];
            temp.set(rect);
            mMatDisplay.mapRect(temp);
            temp.offset(mPreviewArea.left, mPreviewArea.top);
            res[i] = new Rect((int)temp.left, (int)temp.top, (int)temp.right, (int)temp.bottom);
        }
        return res;
    }

    private String mDumpPath = null;
    public void dumpPreviewToFile(String path) {
        mDumpPath = path;
    }

    public synchronized void setFaces(Rect[] vRect, Rect rect) {
        if (DEBUG) {
            LogUtils.debug(TAG, "setFaces");
        }
        if (vRect == null ||vRect.length == 0){
            return;
        }
        if (mFaces == null || mFaces.length != vRect.length){
         mFaces = new Rect[vRect.length];
        }
        for (int i = 0; i < vRect.length; i++) {
            int rw, rh;
            rw = (int) rect.width();
            rh = (int) rect.height();

            if (DEBUG) {
                LogUtils.debug(TAG, "face = " + vRect[i].toString());
            }
            vRect[i].set(vRect[i].left*mDetectWidth/rw, vRect[i].top*mDetectHeight/rh, vRect[i].right*mDetectWidth/rw, vRect[i].bottom*mDetectHeight/rh);
            mFaces[i] = new Rect();
            mFaces[i].set(vRect[i]);
        }
//        mFaces = changeFaces(mFaces);
    }


    private final class MagiclensFrameCallback implements CameraAgent.CameraPreviewDataCallback {
        @Override
        public void onPreviewFrame(byte[] data, CameraProxy camera) {
            if (DEBUG) {
                LogUtils.debug(TAG, "onPreviewFrame");
            }
            if (mCamera == null) {
                return;
            }
            if (mPreviewDetectThread!=null) mPreviewDetectThread.onPreviewFrame(data);
        }
    }

    public void onPreviewAreaChanged(RectF previewArea) {
        mPreviewArea.set(previewArea);
    }
}
