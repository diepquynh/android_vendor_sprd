package com.ucamera.ucam.modules.ufilter;

import android.view.View;
import com.android.camera.PhotoController;
import com.android.camera.app.AppController;
import com.ucamera.ucam.modules.BasicModule;
import android.graphics.Bitmap;

public class UcamFilterPhotoModule extends BasicModule{

    public UcamFilterPhotoModule(AppController app) {
        super(app);
    }

    public Bitmap getPreviewBitmap() {
        return null;
    }
    public void delayUiPause() {
    }
}