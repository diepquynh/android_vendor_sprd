package com.ucamera.ucam.modules.utils;

import android.graphics.Bitmap;

public interface MagiclensCallback {
    void captureComplete(Bitmap bmp, int picW, int picH);
}
