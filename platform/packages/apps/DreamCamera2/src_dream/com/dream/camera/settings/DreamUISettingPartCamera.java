
package com.dream.camera.settings;

import java.util.Set;

import android.content.Context;
import android.util.AttributeSet;

import com.android.camera.settings.Keys;
import com.android.camera.util.CameraUtil;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.android.camera2.R;
import android.os.SystemProperties;
import android.preference.ListPreference;


public class DreamUISettingPartCamera extends DreamUISettingPartBasic {

    public DreamUISettingPartCamera(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
    }

    public DreamUISettingPartCamera(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
    }

    public DreamUISettingPartCamera(Context context, AttributeSet attrs,
            int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        // TODO Auto-generated constructor stub
    }

    public DreamUISettingPartCamera(Context context, AttributeSet attrs,
            int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        // TODO Auto-generated constructor stub
    }

    private static final String TAG = "DreamUISettingPartCamera";

    // SPRD: fix for bug 499642 delete location save function
    private boolean isSupportGps =  CameraUtil.isRecordLocationEnable();

    @Override
    public void changContent() {
        mDataModule = DataModuleManager.getInstance(getContext())
                .getDataModuleCamera();
        super.changContent();
    }

    /*SPRD: fix bug 606536 not add ui change listener when back from secure camera @*/
    @Override
    public void addListener() {
        super.addListener();
    }
    /* @ */

    @Override
    protected void updatePreItemsAccordingProperties() {

        // record location according if support gps
        updateVisibilityRecordLocation();

        // quick capture
        updateQuickCaptureShow();

    }

    private void updateVisibilityRecordLocation() {
        if (!isSupportGps) {
            recursiveDelete(this, findPreference(Keys.KEY_RECORD_LOCATION));
        }
    }

    private void updateQuickCaptureShow() {
        if (UCamUtill.isQuickCaptureEnabled()) {
            ListPreference speedCapturePref = (ListPreference) findPreference(Keys.KEY_QUICK_CAPTURE);
            if (SystemProperties.getBoolean("persist.sys.cam.hascamkey", false)) {
                speedCapturePref.setTitle(R.string.pref_camera_quick_capture_title_camkey);
                speedCapturePref.setDialogTitle(R.string.pref_camera_quick_capture_title_camkey);
            }
        } else {
            recursiveDelete(this, findPreference(Keys.KEY_QUICK_CAPTURE));
        }
    }

}
