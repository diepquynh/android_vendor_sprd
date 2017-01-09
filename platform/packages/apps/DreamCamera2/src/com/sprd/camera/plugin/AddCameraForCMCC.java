package com.sprd.camera.plugin;

import android.app.AddonManager;
import android.util.Log;

import com.android.camera2.R;

public class AddCameraForCMCC {
    private static final String LOGTAG = "AddCameraForCMCC";
    static AddCameraForCMCC sInstance;

    public static AddCameraForCMCC getInstance() {
        if (sInstance != null) {
            return sInstance;
        } else {
            sInstance = (AddCameraForCMCC) AddonManager.getDefault().getAddon(R.string.camera_cmcc,
                    AddCameraForCMCC.class);
        }
        return sInstance;
    }

    public AddCameraForCMCC() {

    }

    public boolean isCMCCVersion() {
        return false;
    }

    public int getAnimationDurationCmcc() {
        return 0;
    }

    public int getCircleAnimationDurationCmcc() {
        return 0;
    }
}
