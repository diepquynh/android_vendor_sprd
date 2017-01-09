/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.dream.camera;

import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.TypedArray;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageButton;
import android.widget.LinearLayout;

import com.android.camera.ButtonManager;
import com.android.camera.ButtonManager.ButtonStatusListener;
import com.android.camera.app.AppController;
import com.android.camera.app.CameraAppUI;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.ui.RadioOptions;
import com.android.camera.util.GcamHelper;
import com.android.camera.util.PhotoSphereHelper;
import com.android.camera.widget.ModeOptions;
import com.android.camera2.R;
import com.android.camera.MultiToggleImageButton;
import com.dream.camera.settings.DataModuleBasic;
import com.dream.camera.settings.DataModuleBasic.DreamSettingChangeListener;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.settings.DataModuleManager.ResetListener;
import com.dream.camera.settings.DreamSettingUtil;

/**
 * A class for generating pre-initialized {@link #android.widget.ImageButton}s.
 */
public class ButtonManagerDream extends ButtonManager implements
        SettingsManager.OnSettingChangedListener {
    private static final Log.Tag TAG = new Log.Tag("BMDream");
    public static final int BUTTON_FLASH_DREAM = 20;
    public static final int BUTTON_CAMERA_DREAM = 21;
    public static final int BUTTON_HDR_DREAM = 22;
    public static final int BUTTON_COUNTDOWN_DREAM = 23;
    public static final int BUTTON_METERING_DREAM = 24;
    public static final int BUTTON_TORCH_DREAM = 25;
    public static final int BUTTON_MAKE_UP_DREAM = 26;
    public static final int BUTTON_VIDEO_FLASH_DREAM = 27;
    public static final int BUTTON_VGESTURE_DREAM = 28;
    public static final int BUTTON_GIF_PHOTO_FLASH_DREAM = 29;
    public static final int BUTTON_SETTING_DREAM = 30;
    public static final int BUTTON_MAKE_UP_VIDEO_DREAM = 31;

    /** A reference to the application's settings manager. */
    private final SettingsManager mSettingsManager;

    /** Bottom bar options toggle buttons. */
    private MultiToggleImageButton mButtonCameraDream;
    private MultiToggleImageButton mButtonFlashDream;
    private MultiToggleImageButton mButtonHdrDream;
    private MultiToggleImageButton mButtonMeteringDream;
    private MultiToggleImageButton mButtonCountdownDream;
    private MultiToggleImageButton mButtonMakupDream;
    private MultiToggleImageButton mButtonMakeuiVideoDream;
    private MultiToggleImageButton mButtonVideoFlashDream;
    private MultiToggleImageButton mButtonGifPhotoFlashDreamButton;
    /* SPRD: Fix bug 535110, Photo voice record. 
    private MultiToggleImageButton mButtonVGestureDream;
    @{ */
    private ImageButton mButtonSettingDream;

    /** Intent UI buttons. */
    /*
     * private ImageButton mButtonCancel; private ImageButton mButtonDone;
     * private ImageButton mButtonRetake; // same as review.
     */
    private static final int NO_RESOURCE = -1;

    /**
     * A listener for button enabled and visibility state changes.
     */
    private ButtonStatusListener mListener;

    /** Whether Camera Button can be enabled by generic operations. */
    private boolean mIsCameraButtonBlocked;

    private final AppController mAppController;

    // Add for dream settings.
    private DataModuleBasic mDataModule;

    /**
     * Get a new global ButtonManager.
     */
    public ButtonManagerDream(AppController app) {
        super(app);
        mAppController = app;

        mSettingsManager = app.getSettingsManager();
        mSettingsManager.addListener(this);

        mDataModule = DataModuleManager.getInstance(
                mAppController.getAndroidContext()).getDataModuleCamera();

    }

    /**
     * Gets references to all known buttons.
     */
    protected void getButtonsReferencesDream(View root) {
        mButtonCameraDream = (MultiToggleImageButton) root
                .findViewById(R.id.camera_toggle_button_dream);
        mButtonFlashDream = (MultiToggleImageButton) root
                .findViewById(R.id.flash_toggle_button_dream);
        mButtonHdrDream = (MultiToggleImageButton) root
                .findViewById(R.id.hdr_toggle_button_dream);
        mButtonCountdownDream = (MultiToggleImageButton) root
                .findViewById(R.id.countdown_toggle_button_dream);
        mButtonMeteringDream = (MultiToggleImageButton) root
                .findViewById(R.id.metering_toggle_button_dream);
        mButtonMakupDream = (MultiToggleImageButton) root
                .findViewById(R.id.make_up_toggle_button_dream);
        mButtonMakeuiVideoDream = (MultiToggleImageButton) root
                .findViewById(R.id.make_up_video_toggle_button_dream);
        mButtonVideoFlashDream = (MultiToggleImageButton) root
                .findViewById(R.id.video_flash_toggle_button_dream);
        mButtonGifPhotoFlashDreamButton = (MultiToggleImageButton) root
                .findViewById(R.id.gif_photo_flash_toggle_button_dream);
        /* SPRD: Fix bug 535110, Photo voice record. 
        mButtonVGestureDream = (MultiToggleImageButton) root
                .findViewById(R.id.vgesture_toggle_button_dream);
        @{ */
        mButtonSettingDream = (ImageButton) root
                .findViewById(R.id.settings_button_dream);
    }

    public void load(View root) {
        super.load(mAppController.getCameraAppUI().getModuleRootView());
        Log.d(TAG, "load");
        getButtonsReferencesDream(root);
    }

    @Override
    public void onSettingChanged(SettingsManager settingsManager, String key) {
        // MultiToggleImageButton button = null;
        // int index = 0;
        //
        // if (key.equals(Keys.KEY_FLASH_MODE)) {
        // index = mSettingsManager.getIndexOfCurrentValue(
        // mAppController.getCameraScope(), Keys.KEY_FLASH_MODE);
        // button = getButtonOrError(BUTTON_FLASH_DREAM);
        // /**
        // * SPRD: Change for New Feature Gif original code
        // *
        // * @{ else if (key.equals(Keys.KEY_VIDEOCAMERA_FLASH_MODE)) { index
        // * = mSettingsManager.getIndexOfCurrentValue(mAppController.
        // * getCameraScope(), Keys.KEY_VIDEOCAMERA_FLASH_MODE);
        // */
        // } else if (key.equals(Keys.KEY_VIDEOCAMERA_FLASH_MODE)
        // || key.equals(Keys.KEY_GIF_FLASH_MODE)) {
        // index = DataModuleManager
        // .getInstance(mAppController.getAndroidContext())
        // .getCurrentDataModule()
        // .getIndexOfCurrentValue(
        // mAppController.getCurrentModuleIndex() != mAppController
        // .getAndroidContext().getResources()
        // .getInteger(R.integer.camera_mode_gif) ?
        // Keys.KEY_VIDEOCAMERA_FLASH_MODE
        // : Keys.KEY_GIF_FLASH_MODE);
        // /**
        // * @}
        // */
        // button = getButtonOrError(BUTTON_TORCH_DREAM);
        // } else if (key.equals(Keys.KEY_CAMERA_ID)) {
        // index = mDataModule.getIndexOfCurrentValue(Keys.KEY_CAMERA_ID);
        // button = getButtonOrError(BUTTON_CAMERA_DREAM);
        // } else if (key.equals(Keys.KEY_CAMERA_HDR)) {
        // index = mSettingsManager.getIndexOfCurrentValue(
        // SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_HDR);
        // button = getButtonOrError(BUTTON_HDR_DREAM);
        // } else if (key.equals(Keys.KEY_EXPOSURE)) {
        // updateExposureButtons();
        // } else if (key.equals(Keys.KEY_CAMER_METERING)) {
        // index = mSettingsManager.getIndexOfCurrentValue(
        // SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMER_METERING);
        // button = getButtonOrError(BUTTON_METERING_DREAM);
        // } else if (key.equals(Keys.KEY_CAMERA_BEAUTY_ENTERED)) {
        // index = mSettingsManager.getIndexOfCurrentValue(
        // SettingsManager.SCOPE_GLOBAL,
        // Keys.KEY_CAMERA_BEAUTY_ENTERED);
        // button = getButtonOrError(BUTTON_MAKE_UP_DREAM);
        // } else if (key.equals(Keys.KEY_VIDEO_FLASH_MODE)) {
        // index = mSettingsManager.getIndexOfCurrentValue(
        // SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_FLASH_MODE);
        // button = getButtonOrError(BUTTON_VIDEO_FLASH_DREAM);
        // }
        // if (button != null && button.getState() != index) {
        // button.setState(Math.max(index, 0), false);
        // }
    }

    /**
     * Returns the appropriate {@link com.android.camera.MultiToggleImageButton}
     * based on button id. An IllegalStateException will be throw if the button
     * could not be found in the view hierarchy.
     */
    protected MultiToggleImageButton getButtonOrError(int buttonId) {
        switch (buttonId) {
        case BUTTON_FLASH_DREAM:
            if (mButtonFlashDream == null) {
                throw new IllegalStateException(
                        "Flash button could not be found.");
            }
            return mButtonFlashDream;
        case BUTTON_TORCH_DREAM:
            if (mButtonFlashDream == null) {
                throw new IllegalStateException(
                        "Torch button could not be found.");
            }
            return mButtonFlashDream;
        case BUTTON_CAMERA_DREAM:
            if (mButtonCameraDream == null) {
                throw new IllegalStateException(
                        "Camera button could not be found.");
            }
            return mButtonCameraDream;
        case BUTTON_HDR_DREAM:
            if (mButtonHdrDream == null) {
                throw new IllegalStateException(
                        "Hdr button could not be found.");
            }
            return mButtonHdrDream;
        case BUTTON_COUNTDOWN_DREAM:
            if (mButtonCountdownDream == null) {
                throw new IllegalStateException(
                        "Countdown button could not be found.");
            }
            return mButtonCountdownDream;
        case BUTTON_METERING_DREAM:
            if (mButtonCountdownDream == null) {
                throw new IllegalStateException(
                        "Metering button could not be found.");
            }
            return mButtonMeteringDream;
        case BUTTON_MAKE_UP_DREAM:
            if (mButtonMakupDream == null) {
                throw new IllegalStateException(
                        "Makeup button could not be found.");
            }
            return mButtonMakupDream;
        case BUTTON_MAKE_UP_VIDEO_DREAM:
            if (mButtonMakeuiVideoDream == null) {
                throw new IllegalStateException(
                        "Makeup button could not be found.");
            }
            return mButtonMakeuiVideoDream;
        case BUTTON_VIDEO_FLASH_DREAM:
            if (mButtonVideoFlashDream == null) {
                throw new IllegalStateException(
                        "Video Flash button could not be found.");
            }
            return mButtonVideoFlashDream;
        /* SPRD: Fix bug 535110, Photo voice record. 
        case BUTTON_VGESTURE_DREAM:
            if (mButtonVGestureDream == null) {
                throw new IllegalStateException(
                        "vgesture button could not be found.");
            }
            return mButtonVGestureDream;
        @{ */
        case BUTTON_GIF_PHOTO_FLASH_DREAM:
            if(mButtonGifPhotoFlashDreamButton == null){
                throw new IllegalStateException(
                        "gif photo flash button could not be found.");
            }
            return mButtonGifPhotoFlashDreamButton;
        default:
            return super.getButtonOrError(buttonId);
        }
    }

    /**
     * Returns the appropriate {@link android.widget.ImageButton} based on
     * button id. An IllegalStateException will be throw if the button could not
     * be found in the view hierarchy.
     */
    protected ImageButton getImageButtonOrError(int buttonId) {
        switch (buttonId) {
        case BUTTON_CANCEL:
            if (mButtonCancel == null) {
                throw new IllegalStateException(
                        "Cancel button could not be found.");
            }
            return mButtonCancel;
        case BUTTON_DONE:
            if (mButtonDone == null) {
                throw new IllegalStateException(
                        "Done button could not be found.");
            }
            return mButtonDone;
        case BUTTON_RETAKE:
            if (mButtonRetake == null) {
                throw new IllegalStateException(
                        "Retake button could not be found.");
            }
            return mButtonRetake;
        case BUTTON_REVIEW:
            if (mButtonRetake == null) {
                throw new IllegalStateException(
                        "Review button could not be found.");
            }
            return mButtonRetake;
        case BUTTON_SETTING_DREAM:
            if (mButtonSettingDream == null) {
                throw new IllegalStateException(
                        "gif setting button could not be found.");
            }
            return mButtonSettingDream;

        default:
            return super.getImageButtonOrError(buttonId);
        }
    }

    /**
     * Initialize a known button by id with a state change callback, and then
     * enable the button.
     * 
     * @param buttonId
     *            The id if the button to be initialized.
     * @param cb
     *            The callback to be executed after the button state change.
     */
    public void initializeButton(int buttonId, ButtonCallback cb) {
        initializeButton(buttonId, cb, null);
    }

    public void initializeButton(int buttonId, ButtonCallback cb, int resId) {
        initializeButton(buttonId, cb, null, resId);
    }

    public void initializeButton(int buttonId, ButtonCallback cb,
            ButtonCallback preCb, int resId) {
        MultiToggleImageButton button = getButtonOrError(buttonId);
        switch (buttonId) {
            case BUTTON_FLASH_DREAM:
                initializeFlashButton(button, cb, preCb, resId);
                break;
            // case BUTTON_TORCH_DREAM:
            // initializeTorchButton(button, cb, preCb,
            // R.array.video_flashmode_icons);
            // break;

            case BUTTON_CAMERA_DREAM:
                initializeCameraButton(button, cb, preCb, resId);
                break;

            case BUTTON_HDR_DREAM:
                initializeHdrButton(button, cb, preCb, resId);
                break;

            case BUTTON_COUNTDOWN_DREAM:
                initializeCountdownButton(button, cb, preCb, resId);
                break;

            case BUTTON_METERING_DREAM:
                initializeMeteringButton(button, cb, preCb, resId);
                break;

            case BUTTON_MAKE_UP_DREAM:
                initializeMakeupButton(button, cb, preCb, resId);
                break;
            case BUTTON_MAKE_UP_VIDEO_DREAM:
                initializeVideoMakeupButton(button, cb, preCb, resId);
                break;
            case BUTTON_VIDEO_FLASH_DREAM:
                initializeVideoFlashButton(button, cb, preCb, resId);
                break;

            /* SPRD: Fix bug 535110, Photo voice record.
            case BUTTON_VGESTURE_DREAM:
                initializeVGestureButton(button, cb, preCb, resId);
                break;
            @{ */
            case BUTTON_GIF_PHOTO_FLASH_DREAM:
                initializeGifPhotoFlashButton(button, cb, preCb, resId);

            default:
                super.initializeButton(buttonId, cb, preCb);
        }

        showButton(buttonId);
        enableButton(buttonId);
    }

    /**
     * Initialize a known button by id, with a state change callback and a state
     * pre-change callback, and then enable the button.
     * 
     * @param buttonId
     *            The id if the button to be initialized.
     * @param cb
     *            The callback to be executed after the button state change.
     * @param preCb
     *            The callback to be executed before the button state change.
     */
    public void initializeButton(int buttonId, ButtonCallback cb,
            ButtonCallback preCb) {
        MultiToggleImageButton button = getButtonOrError(buttonId);
        switch (buttonId) {
            case BUTTON_FLASH_DREAM:
                initializeFlashButton(button, cb, preCb,
                        R.array.dream_camera_flashmode_icons);
                break;
            // case BUTTON_TORCH_DREAM:
            // initializeTorchButton(button, cb, preCb,
            // R.array.video_flashmode_icons);
            // break;

            case BUTTON_CAMERA_DREAM:
                initializeCameraButton(button, cb, preCb,
                        R.array.dream_camera_id_icons);
                break;

            case BUTTON_HDR_DREAM:
                initializeHdrButton(button, cb, preCb,
                        R.array.dream_camera_hdr_icons);
                break;

            case BUTTON_COUNTDOWN_DREAM:
                initializeCountdownButton(button, cb, preCb,
                        R.array.dream_countdown_duration_icons);
                break;

            case BUTTON_METERING_DREAM:
                initializeMeteringButton(button, cb, preCb,
                        R.array.dream_metering_icons);
                break;

            case BUTTON_MAKE_UP_DREAM:
                initializeMakeupButton(button, cb, preCb,
                        R.array.dream_make_up_icons);
                break;

            case BUTTON_MAKE_UP_VIDEO_DREAM:
                initializeVideoMakeupButton(button, cb, preCb,
                        R.array.dream_make_up_icons);
                break;
            case BUTTON_VIDEO_FLASH_DREAM:
                initializeVideoFlashButton(button, cb, preCb,
                        R.array.dream_video_flashmode_icons);
                break;

            /* SPRD: Fix bug 535110, Photo voice record.
            case BUTTON_VGESTURE_DREAM:
                initializeVGestureButton(button, cb, preCb, R.array.camera_vgesture_icons);
                break;
            @{ */
            case BUTTON_GIF_PHOTO_FLASH_DREAM:
                initializeGifPhotoFlashButton(button, cb, preCb,
                        R.array.dream_video_flashmode_icons);
                break;

            default:
                super.initializeButton(buttonId, cb, preCb);
        }

        showButton(buttonId);
        enableButton(buttonId);
    }

    /**
     * Initialize a known button with a click listener and a drawable resource
     * id, and a content description resource id. Sets the button visible.
     */
    public void initializePushButton(int buttonId, View.OnClickListener cb,
            int imageId, int contentDescriptionId) {
        ImageButton button = getImageButtonOrError(buttonId);
        button.setOnClickListener(cb);
        if (imageId != NO_RESOURCE) {
            button.setImageResource(imageId);
        }
        if (contentDescriptionId != NO_RESOURCE) {
            button.setContentDescription(mAppController.getAndroidContext()
                    .getResources().getString(contentDescriptionId));
        }

        if (!button.isEnabled()) {
            button.setEnabled(true);
            if (mListener != null) {
                mListener.onButtonEnabledChanged(this, buttonId);
            }
        }
        button.setTag(R.string.tag_enabled_id, buttonId);

        if (button.getVisibility() != View.VISIBLE) {
            button.setVisibility(View.VISIBLE);
            if (mListener != null) {
                mListener.onButtonVisibilityChanged(this, buttonId);
            }
        }
    }

    /**
     * Initialize a known button with a click listener and a resource id. Sets
     * the button visible.
     */
    public void initializePushButton(int buttonId, View.OnClickListener cb,
            int imageId) {
        initializePushButton(buttonId, cb, imageId, NO_RESOURCE);
    }

    /**
     * Initialize a known button with a click listener. Sets the button visible.
     */
    public void initializePushButton(int buttonId, View.OnClickListener cb) {
        initializePushButton(buttonId, cb, NO_RESOURCE, NO_RESOURCE);
    }

    /**
     * Sets the camera button in its disabled (greyed out) state and blocks it
     * so no generic operation can enable it until it's explicitly re-enabled by
     * calling {@link #enableCameraButton()}.
     */
    public void disableCameraButtonAndBlock() {
        // SPRD: Bug 502464
        mAppController.getCameraAppUI().setSwipeEnabled(false);

        mIsCameraButtonBlocked = true;
        disableButton(BUTTON_CAMERA_DREAM);
    }

    /**
     * Sets a button in its disabled (greyed out) state.
     */
    public void disableButton(int buttonId) {
        Log.d(TAG, "disableButton: " + buttonId);
        View button = null;
        if (buttonId == BUTTON_SETTING_DREAM) {
            button = getImageButtonOrError(buttonId);
        } else {
            button = getButtonOrError(buttonId);
        }
        if (buttonId == BUTTON_HDR_DREAM) {
            initializeHdrButtonIcons((MultiToggleImageButton) button,
                    R.array.pref_camera_hdr_icons);
        }

        if (button.isEnabled()) {
            button.setEnabled(false);
            if (mListener != null) {
                mListener.onButtonEnabledChanged(this, buttonId);
            }
        }
        button.setTag(R.string.tag_enabled_id, null);
    }

    public void setButtonVisibility(int buttonId, int visibility) {
        Log.d(TAG,"setButtonVisibility: " + buttonId + ", visibility is " + visibility);
        try {
            View button = getButtonOrError(buttonId);
            ((View)button.getParent()).setVisibility(visibility);
        } catch (IllegalStateException e) {
            Log.e(TAG,"button does not found");
        }
    }

    /**
     * Enables the camera button and removes the block that was set by
     * {@link #disableCameraButtonAndBlock()}.
     */
    public void enableCameraButton() {
        // SPRD: Bug 502464
        mAppController.getCameraAppUI().setSwipeEnabled(true);

        mIsCameraButtonBlocked = false;
        enableButton(BUTTON_CAMERA_DREAM);
    }

    public void updateGifFlashButton(int buttonId, boolean disable) {
        switch (buttonId) {
            case BUTTON_VIDEO_FLASH_DREAM:
                if (disable) {
                    if (mButtonVideoFlashDream != null && mButtonVideoFlashDream.isEnabled()) {
                        mButtonVideoFlashDream.setEnabled(false);
                        if (mListener != null) {
                            mListener.onButtonEnabledChanged(this, buttonId);
                        }
                    }
                } else {
                    if (mButtonVideoFlashDream != null && !mButtonVideoFlashDream.isEnabled()) {
                        mButtonVideoFlashDream.setEnabled(true);
                        if (mListener != null) {
                            mListener.onButtonEnabledChanged(this, buttonId);
                        }
                    }
                }
            case BUTTON_GIF_PHOTO_FLASH_DREAM:
                if (disable) {
                    if (mButtonGifPhotoFlashDreamButton != null && mButtonGifPhotoFlashDreamButton.isEnabled()) {
                        mButtonGifPhotoFlashDreamButton.setEnabled(false);
                        if (mListener != null) {
                            mListener.onButtonEnabledChanged(this, buttonId);
                        }
                    }
                } else {
                    if (mButtonGifPhotoFlashDreamButton != null && !mButtonGifPhotoFlashDreamButton.isEnabled()) {
                        mButtonGifPhotoFlashDreamButton.setEnabled(true);
                        if (mListener != null) {
                            mListener.onButtonEnabledChanged(this, buttonId);
                        }
                    }
                }
        }
    }

    /**
     * Enables a button that has already been initialized.
     */
    public void enableButton(int buttonId) {
        // If Camera Button is blocked, ignore the request.
        if (buttonId == BUTTON_CAMERA_DREAM && mIsCameraButtonBlocked) {
            return;
        }
        View button = null;
        if (buttonId == BUTTON_SETTING_DREAM) {
            button = getImageButtonOrError(buttonId);
        } else {
            button = getButtonOrError(buttonId);
        }

        // SPRD:fix bug528520 flash icon do not show after close hdr
        button.setTag(R.string.tag_enabled_id, buttonId);
        if (!button.isEnabled()) {
            button.setEnabled(true);
            if (mListener != null) {
                mListener.onButtonEnabledChanged(this, buttonId);
            }
        }
        /*
         * SPRD:fix bug528520 flash icon do not show after close hdr android
         * original code button.setTag(R.string.tag_enabled_id, buttonId);
         */
    }

    /**
     * Disable click reactions for a button without affecting visual state. For
     * most cases you'll want to use {@link #disableButton(int)}.
     * 
     * @param buttonId
     *            The id of the button.
     */
    public void disableButtonClick(int buttonId) {
        ImageButton button = getButtonOrError(buttonId);
        if (button instanceof MultiToggleImageButton) {
            ((MultiToggleImageButton) button).setClickEnabled(false);
        }
    }

    /**
     * Enable click reactions for a button without affecting visual state. For
     * most cases you'll want to use {@link #enableButton(int)}.
     * 
     * @param buttonId
     *            The id of the button.
     */
    public void enableButtonClick(int buttonId) {
        ImageButton button = getButtonOrError(buttonId);
        if (button instanceof MultiToggleImageButton) {
            ((MultiToggleImageButton) button).setClickEnabled(true);
        }
    }

    /**
     * Hide a button by id.
     */
    public void hideButton(int buttonId) {
        View button;
        try {
            button = getButtonOrError(buttonId);
        } catch (IllegalArgumentException e) {
            button = getImageButtonOrError(buttonId);
        }
        if (button.getVisibility() == View.VISIBLE) {
            button.setVisibility(View.GONE);
            if (mListener != null) {
                mListener.onButtonVisibilityChanged(this, buttonId);
            }
        }
    }

    /**
     * Show a button by id.
     */
    public void showButton(int buttonId) {
        View button;
        try {
            button = getButtonOrError(buttonId);
        } catch (IllegalArgumentException e) {
            button = getImageButtonOrError(buttonId);
        }
        if (button.getVisibility() != View.VISIBLE) {
            button.setVisibility(View.VISIBLE);
            if (mListener != null) {
                mListener.onButtonVisibilityChanged(this, buttonId);
            }
        }
    }

    /*
     * public void setToInitialState() {
     * mModeOptions.setMainBar(ModeOptions.BAR_STANDARD); } public void
     * setExposureCompensationCallback(final CameraAppUI.BottomBarUISpec
     * .ExposureCompensationSetCallback cb) { if (cb == null) {
     * mModeOptionsExposure.setOnOptionClickListener(null); } else {
     * mModeOptionsExposure .setOnOptionClickListener(new
     * RadioOptions.OnOptionClickListener() {
     * 
     * @Override public void onOptionClicked(View v) { int comp =
     * Integer.parseInt((String)(v.getTag())); if (mExposureCompensationStep !=
     * 0.0f) { int compValue = Math.round(comp / mExposureCompensationStep);
     * cb.setExposure(compValue); } } }); } }
     */
    /**
     * Set the exposure compensation parameters supported by the current camera
     * mode.
     * 
     * @param min
     *            Minimum exposure compensation value.
     * @param max
     *            Maximum exposure compensation value.
     * @param step
     *            Expsoure compensation step value.
     */
    /*
     * public void setExposureCompensationParameters(int min, int max, float
     * step) { mMaxExposureCompensation = max; mMinExposureCompensation = min;
     * mExposureCompensationStep = step; // SPRD Bug:474711 Feature:Exposure
     * Compensation. setVisible(mExposureN3, (Math.round(min * step) <= -3));
     * setVisible(mExposureN2, (Math.round(min * step) <= -2));
     * setVisible(mExposureN1, (Math.round(min * step) <= -1));
     * setVisible(mExposureP1, (Math.round(max * step) >= 1));
     * setVisible(mExposureP2, (Math.round(max * step) >= 2)); // SPRD
     * Bug:474711 Feature:Exposure Compensation. setVisible(mExposureP3,
     * (Math.round(max * step) >= 3)); updateExposureButtons(); }
     */

    private static void setVisible(View v, boolean visible) {
        if (visible) {
            v.setVisibility(View.VISIBLE);
        } else {
            v.setVisibility(View.INVISIBLE);
        }
    }

    /**
     * @return The exposure compensation step value.
     **/
    /*
     * public float getExposureCompensationStep() { return
     * mExposureCompensationStep; }
     */

    /**
     * Check if a button is enabled with the given button id..
     */
    public boolean isEnabled(int buttonId) {
        View button;
        try {
            button = getButtonOrError(buttonId);
        } catch (IllegalArgumentException e) {
            button = getImageButtonOrError(buttonId);
        } catch (IllegalStateException e){
            Log.i(TAG, buttonId + "button not found");
            return false;
        }

        Integer enabledId = (Integer) button.getTag(R.string.tag_enabled_id);
        if (enabledId != null) {
            return (enabledId.intValue() == buttonId) && button.isEnabled();
        } else {
            return false;
        }
    }

    /**
     * Check if a button is visible.
     */
    public boolean isVisible(int buttonId) {
        View button;
        try {
            button = getButtonOrError(buttonId);
        } catch (IllegalArgumentException e) {
            button = getImageButtonOrError(buttonId);
        }
        return (button.getVisibility() == View.VISIBLE);
    }

    /**
     * Initialize a flash button.
     */
    private void initializeFlashButton(MultiToggleImageButton button,
            final ButtonCallback cb, final ButtonCallback preCb, int resIdImages) {

        if (resIdImages > 0) {
            button.overrideImageIds(resIdImages);
        }
        button.overrideContentDescriptions(R.array.camera_flash_descriptions);

        int index = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .getIndexOfCurrentValue(Keys.KEY_FLASH_MODE);
        button.setState(index >= 0 ? index : 0, false);

        setPreChangeCallback(button, preCb);

        button.setOnStateChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {
                DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .changeSettingsByIndex(Keys.KEY_FLASH_MODE, state);
                if (cb != null) {
                    // cb.onStateChanged(state);
                }
            }
        });
    }

    private void initializeVideoFlashButton(MultiToggleImageButton button,
            final ButtonCallback cb, final ButtonCallback preCb, int resIdImages) {

        if (resIdImages > 0) {
            button.overrideImageIds(resIdImages);
        }
        button.overrideContentDescriptions(R.array.video_flash_descriptions);

        int index = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .getIndexOfCurrentValue(Keys.KEY_VIDEO_FLASH_MODE);
        button.setState(index >= 0 ? index : 0, false);

        setPreChangeCallback(button, preCb);

        button.setOnStateChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {
                Log.e(TAG, "Keys.KEY_VIDEO_FLASH_MODE    state = " + state);
                DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .changeSettingsByIndex(Keys.KEY_VIDEO_FLASH_MODE, state);
                if (cb != null) {
                    // cb.onStateChanged(state);
                }
            }
        });
    }

    private void initializeGifPhotoFlashButton(MultiToggleImageButton button,
            final ButtonCallback cb, final ButtonCallback preCb, int resIdImages) {

        if (resIdImages > 0) {
            button.overrideImageIds(resIdImages);
        }
        button.overrideContentDescriptions(R.array.video_flash_descriptions);

        int index = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .getIndexOfCurrentValue(Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE);

        button.setState(index >= 0 ? index : 0, false);

        setPreChangeCallback(button, preCb);

        button.setOnStateChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {
                Log.e(TAG, "Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE    state = " + state);
                DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .changeSettingsByIndex(Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE, state);
                if (cb != null) {
                    // cb.onStateChanged(state);
                }
            }
        });
    }

    /**
     * Initialize video torch button
     */
    private void initializeTorchButton(MultiToggleImageButton button,
            final ButtonCallback cb, final ButtonCallback preCb, int resIdImages) {

        if (resIdImages > 0) {
            button.overrideImageIds(resIdImages);
        }
        button.overrideContentDescriptions(R.array.video_flash_descriptions);

        /**
         * SPRD: Change for New Feature Gif original code
         * 
         * @{ int index =
         *    mSettingsManager.getIndexOfCurrentValue(mAppController.
         *    getCameraScope(), Keys.KEY_VIDEOCAMERA_FLASH_MODE);
         */
        int index = mSettingsManager
                .getIndexOfCurrentValue(
                        mAppController.getCameraScope(),
                        mAppController.getCurrentModuleIndex() != mAppController
                                .getAndroidContext().getResources()
                                .getInteger(R.integer.camera_mode_gif) ? Keys.KEY_VIDEOCAMERA_FLASH_MODE
                                : Keys.KEY_GIF_FLASH_MODE);
        /**
         * @}
         */
        button.setState(index >= 0 ? index : 0, false);

        setPreChangeCallback(button, preCb);

        button.setOnStateChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {
                /**
                 * SPRD: Change for New Feature Gif original code
                 * 
                 * @{ mSettingsManager.setValueByIndex(mAppController.
                 *    getCameraScope(), Keys.KEY_VIDEOCAMERA_FLASH_MODE, state);
                 */
                mSettingsManager.setValueByIndex(
                        mAppController.getCameraScope(),
                        mAppController.getCurrentModuleIndex() != mAppController
                                .getAndroidContext().getResources()
                                .getInteger(R.integer.camera_mode_gif) ? Keys.KEY_VIDEOCAMERA_FLASH_MODE
                                : Keys.KEY_GIF_FLASH_MODE, state);
                /**
                 * @}
                 */
                if (cb != null) {
                    cb.onStateChanged(state);
                }
            }
        });
    }

    /**
     * Initialize hdr plus flash button
     */
    private void initializeHdrPlusFlashButton(MultiToggleImageButton button,
            final ButtonCallback cb, final ButtonCallback preCb, int resIdImages) {

        if (resIdImages > 0) {
            button.overrideImageIds(resIdImages);
        }
        button.overrideContentDescriptions(R.array.hdr_plus_flash_descriptions);

        int index = mSettingsManager.getIndexOfCurrentValue(
                mAppController.getModuleScope(), Keys.KEY_HDR_PLUS_FLASH_MODE);
        button.setState(index >= 0 ? index : 0, false);

        setPreChangeCallback(button, preCb);

        button.setOnStateChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {
                mSettingsManager.setValueByIndex(
                        mAppController.getModuleScope(),
                        Keys.KEY_HDR_PLUS_FLASH_MODE, state);
                if (cb != null) {
                    cb.onStateChanged(state);
                }
            }
        });
    }

    /**
     * Initialize a camera button.
     */
    private void initializeCameraButton(final MultiToggleImageButton button,
            final ButtonCallback cb, final ButtonCallback preCb, int resIdImages) {

        if (resIdImages > 0) {
            button.overrideImageIds(resIdImages);
        }

        int index = mDataModule.getIndexOfCurrentValue(Keys.KEY_CAMERA_ID);

        button.setState(index >= 0 ? index : 0, false);

        setPreChangeCallback(button, preCb);

        button.setOnStateChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {
                /*
                 * SPRD: fix bug 516434,519391 If the state is the same with
                 * current camera ID, the function will not run.
                 */

                int prefCameraId = mDataModule.getInt(Keys.KEY_CAMERA_ID);
                Log.d(TAG, "bbbb initializeCameraButton stateChanged state="
                        + state + "," + prefCameraId);

                if (state != prefCameraId) {
                    mDataModule.set(Keys.KEY_CAMERA_ID, state);
                    int cameraId = mDataModule.getInt(Keys.KEY_CAMERA_ID);
                    Log.d(TAG,
                            "bbbb initializeCameraButton stateChanged state="
                                    + state + "," + cameraId);
                    // This is a quick fix for ISE in Gcam module which can be
                    // found by rapid pressing camera switch button. The
                    // assumption
                    // here is that each time this button is clicked, the
                    // listener
                    // will do something and then enable this button again.
                    button.setEnabled(false);
                    if (cb != null) {
                        cb.onStateChanged(cameraId);
                    }
                    mAppController.getCameraAppUI().onChangeCamera();
                }
            }
        });
    }

    /**
     * Initialize an hdr plus button.
     */
    private void initializeHdrPlusButton(MultiToggleImageButton button,
            final ButtonCallback cb, final ButtonCallback preCb, int resIdImages) {

        initializeHdrPlusButtonIcons(button, resIdImages);

        int index = mSettingsManager.getIndexOfCurrentValue(
                SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_HDR_PLUS);
        button.setState(index >= 0 ? index : 0, false);

        setPreChangeCallback(button, preCb);

        button.setOnStateChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {
                mSettingsManager.setValueByIndex(SettingsManager.SCOPE_GLOBAL,
                        Keys.KEY_CAMERA_HDR_PLUS, state);
                if (cb != null) {
                    cb.onStateChanged(state);
                }
            }
        });
    }

    private void initializeHdrPlusButtonIcons(MultiToggleImageButton button,
            int resIdImages) {
        if (resIdImages > 0) {
            button.overrideImageIds(resIdImages);
        }
        button.overrideContentDescriptions(R.array.hdr_plus_descriptions);
    }

    /**
     * Initialize an hdr button.
     */
    private void initializeHdrButton(MultiToggleImageButton button,
            final ButtonCallback cb, final ButtonCallback preCb, int resIdImages) {

        initializeHdrButtonIcons(button, resIdImages);

        int index = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .getIndexOfCurrentValue(Keys.KEY_CAMERA_HDR);

        button.setState(index >= 0 ? index : 0, false);

        setPreChangeCallback(button, preCb);

        button.setOnStateChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {
                DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .changeSettingsByIndex(Keys.KEY_CAMERA_HDR, state);
                if (cb != null) {
                    // cb.onStateChanged(state);
                }
            }
        });
    }

    private void initializeHdrButtonIcons(MultiToggleImageButton button,
            int resIdImages) {
        if (resIdImages > 0) {
            button.overrideImageIds(resIdImages);
        }
        button.overrideContentDescriptions(R.array.hdr_descriptions);
    }

    /**
     * Initialize a countdown timer button.
     */
    private void initializeCountdownButton(MultiToggleImageButton button,
            final ButtonCallback cb, final ButtonCallback preCb, int resIdImages) {
        if (resIdImages > 0) {
            button.overrideImageIds(resIdImages);
        }

        int index = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .getIndexOfCurrentValue(Keys.KEY_COUNTDOWN_DURATION);
        button.setState(index >= 0 ? index : 0, false);

        setPreChangeCallback(button, preCb);

        button.setOnStateChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {
                DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .changeSettingsByIndex(Keys.KEY_COUNTDOWN_DURATION,
                                state);
                if (cb != null) {
                    // cb.onStateChanged(state);
                }
            }
        });
    }

    /**
     * Update the visual state of the manual exposure buttons
     */
    /*
     * public void updateExposureButtons() { int compValue =
     * mSettingsManager.getInteger(mAppController.getCameraScope(),
     * Keys.KEY_EXPOSURE); if (mExposureCompensationStep != 0.0f) { int comp =
     * Math.round(compValue * mExposureCompensationStep);
     * mModeOptionsExposure.setSelectedOptionByTag(String.valueOf(comp)); } }
     */
    /**
     * Initialize a grid lines button.
     */
    private void initializeGridLinesButton(MultiToggleImageButton button,
            final ButtonCallback cb, final ButtonCallback preCb, int resIdImages) {

        if (resIdImages > 0) {
            button.overrideImageIds(resIdImages);
        }
        button.overrideContentDescriptions(R.array.grid_lines_descriptions);

        int index = mSettingsManager.getIndexOfCurrentValue(
                SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_GRID_LINES);
        button.setState(index >= 0 ? index : 0, true);

        setPreChangeCallback(button, preCb);

        button.setOnStateChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {
                mSettingsManager.setValueByIndex(SettingsManager.SCOPE_GLOBAL,
                        Keys.KEY_CAMERA_GRID_LINES, state);
                if (cb != null) {
                    cb.onStateChanged(state);
                }
            }
        });

    }

    /**
     * Initialize a metering button.
     */
    private void initializeMeteringButton(final MultiToggleImageButton button,
            final ButtonCallback cb, final ButtonCallback preCb, int resIdImages) {

        if (resIdImages > 0) {
            button.overrideImageIds(resIdImages);
        }
        button.overrideContentDescriptions(R.array.metering_descriptions);
        int index = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .getIndexOfCurrentValue(Keys.KEY_CAMER_METERING);
        button.setState(index >= 0 ? index : 0, true);

        setPreChangeCallback(button, preCb);

        button.setOnStateChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {

                int prefstate = DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .getIndexOfCurrentValue(Keys.KEY_CAMER_METERING);

                if (state != prefstate) {
                    DataModuleManager
                            .getInstance(mAppController.getAndroidContext())
                            .getCurrentDataModule()
                            .changeSettingsByIndex(Keys.KEY_CAMER_METERING,
                                    state);
                    if (cb != null) {
                        // cb.onStateChanged(state);
                    }
                }

            }
        });

        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                DreamSettingUtil.showDialog((Activity) mAppController,
                        Keys.KEY_CAMER_METERING,
                        R.string.pref_camera_metering_title,
                        R.array.dream_metering_icons,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                button.setState(which >= 0 ? which : 0, true);
                                dialog.dismiss();
                            }
                        });
            }
        });

    }

    /**
     * Initialize a make up button.
     */
    private void initializeMakeupButton(MultiToggleImageButton button,
            final ButtonCallback cb, final ButtonCallback preCb, int resIdImages) {

        if (resIdImages > 0) {
            button.overrideImageIds(resIdImages);
        }
        button.overrideContentDescriptions(R.array.dream_make_up_descriptions);

        int index = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .getIndexOfCurrentValue(Keys.KEY_CAMERA_BEAUTY_ENTERED);
        button.setState(index >= 0 ? index : 0, true);

        setPreChangeCallback(button, preCb);

        button.setOnStateChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {

                Log.e(TAG, "makeup button state = " + state);

                DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .changeSettingsByIndex(Keys.KEY_CAMERA_BEAUTY_ENTERED,
                                state);
                if (cb != null) {
                    // cb.onStateChanged(state);
                }
            }
        });

    }

    /**
     * Initialize a make up button.
     */
    private void initializeVideoMakeupButton(MultiToggleImageButton button,
            final ButtonCallback cb, final ButtonCallback preCb, int resIdImages) {

        if (resIdImages > 0) {
            button.overrideImageIds(resIdImages);
        }
        button.overrideContentDescriptions(R.array.dream_make_up_descriptions);

        int index = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .getIndexOfCurrentValue(Keys.KEY_VIDEO_BEAUTY_ENTERED);
        button.setState(index >= 0 ? index : 0, true);

        setPreChangeCallback(button, preCb);

        button.setOnStateChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {

                Log.e(TAG, "makeup button state = " + state);

                DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .changeSettingsByIndex(Keys.KEY_VIDEO_BEAUTY_ENTERED,
                                state);
                if (cb != null) {
                    // cb.onStateChanged(state);
                }
            }
        });

    }

    /**
     * Initialize a make up button.
     */
    /* SPRD: Fix bug 535110, Photo voice record. 
    private void initializeVGestureButton(MultiToggleImageButton button,
            final ButtonCallback cb, final ButtonCallback preCb, int resIdImages) {

        if (resIdImages > 0) {
            button.overrideImageIds(resIdImages);
        }
        button.overrideContentDescriptions(R.array.camera_vgesture_descriptions);

        int index = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .getIndexOfCurrentValue(Keys.KEY_CAMERA_VGESTURE);
        button.setState(index >= 0 ? index : 0, true);

        setPreChangeCallback(button, preCb);

        button.setOnStateChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {

                Log.e(TAG, "vgesture button state = " + state);
                DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .changeSettingsByIndex(Keys.KEY_CAMERA_VGESTURE,
                                state);
//                if (cb != null) {
//                     cb.onStateChanged(state);
//                }
            }
        });

    }
@{ */
    private void setPreChangeCallback(MultiToggleImageButton button,
            final ButtonCallback preCb) {
        button.setOnPreChangeListener(new MultiToggleImageButton.OnStateChangeListener() {
            @Override
            public void stateChanged(View view, int state) {
                if (preCb != null) {
                    preCb.onStateChanged(state);
                }
            }
        });
    }

    private DreamSettingChangeListener photoSettingChangeListener = new DreamSettingChangeListener() {

        @Override
        public void onDreamSettingChangeListener(HashMap<String, String> keys) {
            updatePhotoButtonItems(keys.keySet());
        }
    };

    private DreamSettingChangeListener videoSettingChangeListener = new DreamSettingChangeListener() {

        @Override
        public void onDreamSettingChangeListener(HashMap<String, String> keys) {
            updateVideoButtonItems(keys.keySet());
        }

    };

    private ResetListener dataResetListener = new ResetListener() {

        @Override
        public void onSettingReset() {
            Log.e(TAG, "updatePhotoButtonItems onreset................. ");
            Set<String> photoKeys = new HashSet<String>();
            Set<String> VideoKeys = new HashSet<String>();
            Set<String> CameraKeys = new HashSet<String>();

            photoKeys.add(Keys.KEY_CAMERA_BEAUTY_ENTERED);
            photoKeys.add(Keys.KEY_FLASH_MODE);
            photoKeys.add(Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE);
            photoKeys.add(Keys.KEY_COUNTDOWN_DURATION);
            photoKeys.add(Keys.KEY_CAMERA_HDR);
            photoKeys.add(Keys.KEY_CAMER_METERING);
            photoKeys.add(Keys.KEY_CAMERA_VGESTURE);// ui check 182
            VideoKeys.add(Keys.KEY_VIDEO_FLASH_MODE);
            VideoKeys.add(Keys.KEY_VIDEO_BEAUTY_ENTERED);
            CameraKeys.add(Keys.KEY_CAMERA_ID);

            updatePhotoButtonItems(photoKeys);
            updateVideoButtonItems(VideoKeys);
            updateCameraButtonItems(CameraKeys);

        }

    };

    @Override
    public void setListener(ButtonStatusListener listener) {
//        super.setListener(listener);

        DataModuleManager dataModuleManager = DataModuleManager
                .getInstance(mAppController.getAndroidContext());

        dataModuleManager.getDataModulePhoto().addListener(
                photoSettingChangeListener);

        dataModuleManager.getDataModuleVideo().addListener(
                videoSettingChangeListener);

        dataModuleManager.addListener(dataResetListener);

        // hideOrShowButtonAccordingConfig();

    }

    // private void hideOrShowButtonAccordingConfig() {
    // List<String> showItemSet = DataModuleManager
    // .getInstance(mAppController.getAndroidContext())
    // .getCurrentDataModule().getShowItemsSet();
    // List<String> showITems = dataModuleManager.getCurrentDataModule()
    // .getShowItemsSet();
    // if (showITems.contains(Keys.KEY_CAMERA_BEAUTY_ENTERED)) {
    // showButton(BUTTON_MAKE_UP_DREAM);
    // } else {
    // hideButton(BUTTON_MAKE_UP_DREAM);
    // }
    //
    // if (showITems.contains(Keys.KEY_FLASH_MODE)) {
    // showButton(BUTTON_FLASH_DREAM);
    // } else {
    // hideButton(BUTTON_FLASH_DREAM);
    // }
    //
    // if (showITems.contains(Keys.KEY_COUNTDOWN_DURATION)) {
    // showButton(BUTTON_COUNTDOWN_DREAM);
    // } else {
    // hideButton(BUTTON_COUNTDOWN_DREAM);
    // }
    //
    // if (showITems.contains(Keys.KEY_CAMERA_HDR)) {
    // showButton(BUTTON_HDR_DREAM);
    // } else {
    // hideButton(BUTTON_HDR_DREAM);
    // }
    //
    // if (showITems.contains(Keys.KEY_CAMER_METERING)) {
    // showButton(BUTTON_METERING_DREAM);
    // } else {
    // hideButton(BUTTON_METERING_DREAM);
    // }
    //
    // if (showITems.contains(Keys.KEY_VIDEO_FLASH_MODE)) {
    // showButton(BUTTON_VIDEO_FLASH_DREAM);
    // } else {
    // hideButton(BUTTON_VIDEO_FLASH_DREAM);
    // }
    //
    // }

    private void updatePhotoButtonItems(Set<String> keys) {
        for (String key : keys) {
            Log.e(TAG, "updatePhotoButtonItems key = " + key);
            int index = 0;
            MultiToggleImageButton button = null;
            switch (key) {
            case Keys.KEY_CAMERA_BEAUTY_ENTERED:
                index = DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .getIndexOfCurrentValue(Keys.KEY_CAMERA_BEAUTY_ENTERED);
                button = mButtonMakupDream;
                break;
            case Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE:
                index = DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .getIndexOfCurrentValue(Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE);
                if(mButtonGifPhotoFlashDreamButton != null){
                    button = mButtonGifPhotoFlashDreamButton;
                }
                break;
            case Keys.KEY_FLASH_MODE:
                index = DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .getIndexOfCurrentValue(Keys.KEY_FLASH_MODE);
                if(mButtonFlashDream != null){
                    button = mButtonFlashDream;
                }
                break;
            case Keys.KEY_COUNTDOWN_DURATION:
                index = DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .getIndexOfCurrentValue(Keys.KEY_COUNTDOWN_DURATION);
                button = mButtonCountdownDream;
                break;
            case Keys.KEY_CAMERA_HDR:
                index = DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .getIndexOfCurrentValue(Keys.KEY_CAMERA_HDR);
                button = mButtonHdrDream;
                break;
            case Keys.KEY_CAMER_METERING:
                index = DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .getIndexOfCurrentValue(Keys.KEY_CAMER_METERING);
                button = mButtonMeteringDream;
                break;
            /* SPRD: Fix bug 535110, Photo voice record. 
            case Keys.KEY_CAMERA_VGESTURE:
                index = DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .getIndexOfCurrentValue(Keys.KEY_CAMERA_VGESTURE);
                button = mButtonVGestureDream;
                break;
                @{ */
            }
            Log.e(TAG, "updatePhotoButtonItems index = " + index);
            if (button != null) {
                button.setState(index >= 0 ? index : 0, false);
            }
        }

    }

    private void updateVideoButtonItems(Set<String> keys) {

        for (String key : keys) {
            int index = 0;
            MultiToggleImageButton button = null;
            switch (key) {
            case Keys.KEY_VIDEO_FLASH_MODE:
                index = DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .getIndexOfCurrentValue(Keys.KEY_VIDEO_FLASH_MODE);
                button = mButtonVideoFlashDream;
                break;
            case Keys.KEY_VIDEO_BEAUTY_ENTERED:
                index = DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getCurrentDataModule()
                        .getIndexOfCurrentValue(Keys.KEY_VIDEO_BEAUTY_ENTERED);
                button = mButtonMakeuiVideoDream;
                break;
            }
            if (button != null) {
                button.setState(index >= 0 ? index : 0, false);
            }
        }

    }

    private void updateCameraButtonItems(Set<String> keys) {
        for (String key : keys) {
            int index = 0;
            MultiToggleImageButton button = null;
            switch (key) {
            case Keys.KEY_CAMERA_ID:
                index = DataModuleManager
                        .getInstance(mAppController.getAndroidContext())
                        .getDataModuleCamera()
                        .getIndexOfCurrentValue(Keys.KEY_CAMERA_ID);
                button = mButtonCameraDream;
                break;
            }
            if (button != null) {
                button.setState(index >= 0 ? index : 0, false);
            }
        }
    }

    public void refreshButtonState() {
        Set<String> photoKeys = new HashSet<String>();
        Set<String> VideoKeys = new HashSet<String>();
        Set<String> CameraKeys = new HashSet<String>();

        photoKeys.add(Keys.KEY_CAMERA_BEAUTY_ENTERED);
        photoKeys.add(Keys.KEY_FLASH_MODE);
        photoKeys.add(Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE);
        photoKeys.add(Keys.KEY_COUNTDOWN_DURATION);
        photoKeys.add(Keys.KEY_CAMERA_HDR);
        photoKeys.add(Keys.KEY_CAMER_METERING);
        photoKeys.add(Keys.KEY_CAMERA_VGESTURE);

        VideoKeys.add(Keys.KEY_VIDEO_FLASH_MODE);

        CameraKeys.add(Keys.KEY_CAMERA_ID);

        updatePhotoButtonItems(photoKeys);
        updateVideoButtonItems(VideoKeys);
        updateCameraButtonItems(CameraKeys);
    }
}
