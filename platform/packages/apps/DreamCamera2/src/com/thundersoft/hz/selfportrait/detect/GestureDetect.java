package com.thundersoft.hz.selfportrait.detect;
import android.graphics.Rect;
import android.util.Log;
public class GestureDetect implements PreviewDetect{
    private int mHandle = 0;
    @Override
    public void initialize() {
        mHandle = native_create();
    }
    @Override
    public void uninitialize() {
        native_destroy(mHandle);
    }
    public void setFaces(Rect[] faces) {
//        native_set_faces(mHandle, faces);
    }
    public Rect[] detect(byte[] nv21, int width, int height,Rect[] faces) {
        int count = native_detect(mHandle, nv21, width, height, faces);
        if(count<1) {
            return null;
        }
        Rect[] res = new Rect[count];
        for(int i=0; i<count; i++) {
            Rect rect = new Rect();
            native_gesture_info(mHandle, i, rect);
            res[i] = rect;
        }
        return res;
    }
    private static native int native_create();
    private static native void native_destroy(int handle);
    private static native void native_set_faces(int handle, Rect[] faces);
    private static native int native_detect(int handle, byte[] nv21, int width, int height, Rect[] faces);
    private static native int native_count(int handle);
    private static native int native_gesture_info(int handle, int index, Rect rect);
    @Override
    public Rect[] detect(byte[] nv21, int width, int height) {
        // TODO Auto-generated method stub
        return null;
    }
}
