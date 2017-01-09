package com.ucamera.ucam.modules.ugif;

import android.view.View;
import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;
import com.android.camera.debug.Log;
import com.ucamera.ucam.modules.ui.BasicUI;

public class GifUI extends BasicUI{
    public static final String GIF_UI_STRING_ID = "GifUI";

    private static final Log.Tag TAG = new Log.Tag(GIF_UI_STRING_ID);

    public GifUI(int cameraId,PhotoController baseController,CameraActivity activity, View parent) {
        super(cameraId,activity,baseController,parent);
    }
}
