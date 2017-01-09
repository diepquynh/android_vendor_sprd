
package com.dream.camera.modules.slowmotionvideo;

import com.android.camera.CameraActivity;
import com.android.camera.VideoUI;
import com.android.camera.app.AppController;
import com.android.camera.settings.Keys;
import com.dream.camera.dreambasemodules.DreamVideoModule;
import android.widget.Toast;
import com.android.camera2.R;

public class SlowmotionVideoModule extends DreamVideoModule {

    public SlowmotionVideoModule(AppController app) {
        super(app);
    }

    @Override
    public void init(CameraActivity activity, boolean isSecureCamera, boolean isCaptureIntent) {
        super.init(activity, isSecureCamera, isCaptureIntent);
        showSlowmotionHint();
    }
    //dream ui check 226
    public void showSlowmotionHint() {
        boolean shouldSlowmotionHint = mDataModule.getBoolean(Keys.KEY_CAMERA_SLOWMOTION_HINT);
        if (shouldSlowmotionHint == true) {
            Toast.makeText(mActivity, mActivity.getResources().getString(R.string.dream_slowmotion_module_warning),
                    Toast.LENGTH_LONG).show();
            mDataModule.set(Keys.KEY_CAMERA_SLOWMOTION_HINT, false);
        }
    }

    @Override
    public VideoUI createUI(CameraActivity activity) {
        return new SlowmotionVideoUI(activity, this, activity.getModuleLayoutRoot());
    }
}
