
package com.dream.camera.modules.timelapsevideo;

import com.android.camera.CameraActivity;
import com.android.camera.VideoUI;
import com.android.camera.app.AppController;
import com.dream.camera.dreambasemodules.DreamVideoModule;
import android.widget.Toast;
import com.android.camera2.R;
import com.android.camera.settings.Keys;

public class TimelapseVideoModule extends DreamVideoModule {

    public TimelapseVideoModule(AppController app) {
        super(app);
    }

    @Override
    public void init(CameraActivity activity, boolean isSecureCamera,
                     boolean isCaptureIntent) {
        super.init(activity, isSecureCamera, isCaptureIntent);
        showTimelapseHint();
    }

    //dream ui check 225
    public void showTimelapseHint() {
        boolean shouldTimelapseHint = mDataModule.getBoolean(Keys.KEY_CAMERA_TIMELAPSE_HINT);
        if (shouldTimelapseHint == true) {
            Toast.makeText(mActivity, mActivity.getResources().getString(R.string.dream_timelapse_module_warning),
                    Toast.LENGTH_LONG).show();
            mDataModule.set(Keys.KEY_CAMERA_TIMELAPSE_HINT, false);
        }
    }

    @Override
    public VideoUI createUI(CameraActivity activity) {
        return new TimelapseVideoUI(activity, this, activity.getModuleLayoutRoot());
    }
}
