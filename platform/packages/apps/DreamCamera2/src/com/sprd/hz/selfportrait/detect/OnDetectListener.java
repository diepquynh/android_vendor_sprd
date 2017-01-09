package com.sprd.hz.selfportrait.detect;
import android.graphics.Rect;
public interface OnDetectListener {
    void onDetectFace(Rect[] faces);
    void onDetectGesture(Rect[] faces);
}
