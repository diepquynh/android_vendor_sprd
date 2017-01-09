package com.thundersoft.hz.selfportrait.detect;
import android.graphics.Rect;
public interface PreviewDetect {
    void initialize();
    void uninitialize();
    Rect[] detect(byte[] nv21, int width, int height);
}
