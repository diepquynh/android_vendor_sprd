/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.Rect;
import android.util.Log;

public class MakeupEngine {
    private static final String TAG = "MakeupEngine";
    public final static int Max_FaceNum = 1;
    public final static int MAX_DEBLEMISH_AREA = 30;

    static boolean bInitial = false;

    public MakeupEngine() {
    }

    public static void Init_Lib() {
        if (bInitial == false) {
            Init();
            bInitial = true;
        }

    }

    public static void UnInit_Lib() {
        if (bInitial == true) {
            UnInit();
            bInitial = false;
        }
    }

    private static native void Init();

    private static native void UnInit();

    public static native void ResetParameter();

//    public static native void SetImageParam(MakeupItem makeupItem);
    public static native void SetImageParam(FeatureInfo info);

    // input BGRA8888 bitmap, output face info
    public static native void LoadImage(Bitmap bitmap, int[] face_num, Rect[] face_rect, Point[] eyePoint, Point[] mouthPoint);

    public static native void ReplaceImage(Bitmap bitmap, int[] face_num, Rect[] face_rect, Point[] eyePoint, Point[] mouthPoint);

    public static native int ManageImgae(Bitmap bitmap, FeatureInfo info);

    public static int ManageImgae2(Bitmap bitmap, FeatureInfo info) {
        Log.d(TAG, "Into ManageImgae");
        int res = ManageImgae(bitmap, info);
        Log.e(TAG, "Leave ManageImgae");

        return res;
    }

    static {
        try {
            System.loadLibrary("makeupengine");
        } catch (UnsatisfiedLinkError error) {
            Log.w(TAG, "Load the library makeupengine is error!!!");
        }
    }

}
