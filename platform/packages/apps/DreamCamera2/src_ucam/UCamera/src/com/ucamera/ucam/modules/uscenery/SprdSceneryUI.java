package com.ucamera.ucam.modules.ui;

import android.view.View;
import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;

public class SprdSceneryUI extends BasicUI {

    public static final String SCENERY_UI_STRING_ID = "SprdSceneryUI";

    public SprdSceneryUI(int CameraId,PhotoController baseController, CameraActivity activity, View parent) {
        super( CameraId, activity, baseController, parent);
    }
}
