/*
 * Copyright (C) 2010,2011,2013 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucam.jni;

import android.graphics.Bitmap;
import android.util.Log;
public class ImageProcessJni {
    private static String TAG = "ImageProcessJni ";
    /*public final static int  EFFECT_SKETCH_GRAY = 0;     //SKETCH_GRAY
    public final static int  EFFECT_INFRARED = 1;            //INFRARED
    public final static int  EFFECT_POSER = 2;                //POSER
    public final static int  EFFECT_GHOST = 3;                //GHOST
    public final static int  EFFECT_BLACKWHITE = 4;            // BLACKWIHTE
    public final static int  EFFECT_SEPIA = 5;                // BROWN
    public final static int  EFFECT_AQUA = 6;                // GREEN
    public final static int      EFFECT_NEGATIVE = 7;            // INVERT
    public final static int  EFFECT_SKETCH_COLOR = 9;    // SKETCH_COLOR
    public final static int  EFFECT_BLUE = 10;                // BLUE
    public final static int EFFECT_FISHEYE = 8;            // FISHEYE
    */
    // , byte[] result, int nResultLength
    // funmode jni interface
    public static native byte[] ImageColorProcessBuffer(final byte[] jpegData, int nDataLength, int ntype, String strTime);
    public static native int ImageColorProcessRgbBuffer(final byte[] rgbData, int nWidth, int nHeight, int nType, byte[] result);
    public static native int ImageColorProcessYUVWithBitmap(final byte[] yuvData, int nType,Bitmap bitmap, int rotation, boolean needRevert, int cameraid);
    public static native int[] ImageColorProcessArgbBuffer(final int[] rgbaData, int nWidth, int nHeight, int nType);
    // return -1 if fails or 0 if success
    public static native int ImageColorProcessBitmap(final Bitmap srcBitmap, Bitmap dstBitmap, int nType);
    public static native int ImageColorProcessFile(final String strFileName, int nType, String strResultFileName);
//    public static native void SetMultiBmpEffectState(int state);
//    public static native void SetHardwareEffect(int state);
    public static native void SetEffectCategory(int state);
    public static native void SetMultiBmpEffectWidth(int width);
    public static native int[] GetAllEffectBmps();
    public static native int[] GetEffectBmp(int effect);
    public static native void GenerateThumbnails(final byte[] buffer,int bufferLength);
    public static native void SetEffectSrcBuffer(final byte[] jpegData, int nDataLength, int ntype, String strTime);
    public static native void SetEffectParam();
    public static native byte[] ExecuteEffect();
    // panorama jni interface
    /*public static native void PanoramaInit(int width, int height);
    public static native void PanoramaSetMoveDirection(int direction);
    public static native double PanoramaFeedData(final byte[] yuvData);
    public static native void PanoramaFinish(String panoName,int orientation,String strData);
    public static native void PanoramaRelease();
    public static native int[] PanoramaGetFrameTranslation(final byte[] yuvData);
    public static native int GetPanoramaPictureAngle();*/

 // panorama jni interface
    public static native void PanoramaInit(int width, int height);
    public static native double[] PanoramaFeedData(final byte[] yuvData);
    public static native float PanoramaGetProgress();
    public static native void PanoramaCancel();
    public static native int PanoramaFinish(String panoName,int orientation,String strDate);
    public static native void PanoramaRelease();
    // photogrid jni interface
    public static native int[] PhotoGridInit(int nType,int previewWidth, int prevewHeight,int isFrontCamera);
    public static native int[] PhotoGridFeedData(final byte[] jpegData, int nLength, int direction);
    public static native byte[] PhotoGridFinish(String timeStr);
    public static native void PhotoGridCancel();
    // others
    public static native void SetGlobalJpegQuality(int q);
    public static native byte[] ImageTimeStamp(final byte[] jpegData, int nDataLength, String strDateTime);
    public static native void SetResourcePath(String path);
    public static native void SetJNISOPath(String path);
    public static native byte[] SetJpegOrientation(byte[]jpeg,int angle);
    public static native String ImageOcrProcessBuffer(final byte[] yuvData, int nWidth, int nHeight);
    public static native int GetJpegOrientation(final byte[] jpegData);
    public static native int[] Yuv2RGB888(final byte[] yuvData, int width, int height);
    // photo frame
    public static native int[] AddPhotoFrame4ArgbBuffer(final int[] argbBuffer,int width,int height,String frameImagePath);
    public static native byte[] AddPhotoFrame4JpegBuffer(byte[] jpegBuffer,int srcbufferLength, String frameImagePath);
    //return -1 if fails or 0 will be returned
    //public static native int AddPhotoFrame4Bitmap(final Bitmap srcBitmap,Bitmap dstBitmap,String frameImagePath);

    // Continous shot Memory Manager
    public static native void initCMM(String tmpSavePath, String prefix, int dataSize, int width, int height, int nOrientation, boolean yuvToJpeg);
    // stop jni consumer, release jni memory
    public static native void stopCMM();
    public static native int addToCMM(byte[] data,String strDateTime,int length);
    // decrypt shader
    public static native String decrypt(String src);
    public static native String encrypt(String src);

    public static CallBackJNI mCallBackJNI;
    public static void setBurstListener(CallBackJNI callBackJNI) {
        mCallBackJNI = callBackJNI;
    }

    public static void CMMCallback(int result, int number){
        if(mCallBackJNI != null) {
            mCallBackJNI.updateJniStatus(result, number);
        }
    }

    public static interface CallBackJNI {
        void updateJniStatus(int result, int number);
    }

    static {
        try {
            System.loadLibrary("ImageProcessJni");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "ImageProcessJni library not found!");
        }
    }
}
