package com.dream.camera.settings;

import java.util.Set;

import android.content.Context;
import android.util.AttributeSet;

import com.android.camera.settings.Keys;
import com.android.camera.settings.PictureSizeLoader.PictureSizes;
import com.android.camera.util.CameraUtil;

public class DreamUISettingPartVideo extends DreamUISettingPartBasic {

	private static final String TAG = "DreamUISettingPartVideo";

	public DreamUISettingPartVideo(Context context) {
		super(context);
	}

	public DreamUISettingPartVideo(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	public DreamUISettingPartVideo(Context context, AttributeSet attrs,
			int defStyleAttr) {
		super(context, attrs, defStyleAttr);
	}

	public DreamUISettingPartVideo(Context context, AttributeSet attrs,
			int defStyleAttr, int defStyleRes) {
		super(context, attrs, defStyleAttr, defStyleRes);
	}

	@Override
	public void changContent() {
		mDataModule = DataModuleManager.getInstance(getContext())
				.getCurrentDataModule();
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
		// update visibility of picturesize
		updateVisibilityPictureSizes();

        // update visibility of EIOS
        updateVisibilityEOIS();
	}

    private void updateVisibilityPictureSizes() {
//        PictureSizes mPictureSizes = ((DataModuleInterfacePV) mDataModule)
//                .getPictureSizes();
//        if (mPictureSizes.backCameraSizes.isEmpty()) {
//            recursiveDelete(this, findPreference(Keys.KEY_PICTURE_SIZE_BACK));
//        }
//        if (mPictureSizes.frontCameraSizes.isEmpty()) {
//            recursiveDelete(this, findPreference(Keys.KEY_PICTURE_SIZE_FRONT));
//        }
    }

    private void updateVisibilityEOIS() {
        if (!CameraUtil.isEOISDvBackEnabled()) {
            recursiveDelete(this, findPreference(Keys.KEY_EOIS_DV_BACK));
        }
        if (!CameraUtil.isEOISDvFrontEnabled()) {
            recursiveDelete(this, findPreference(Keys.KEY_EOIS_DV_FRONT));
        }
    }

}
