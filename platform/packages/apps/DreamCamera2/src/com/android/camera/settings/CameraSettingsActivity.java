/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.camera.settings;

import android.app.ActionBar;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.media.CamcorderProfile;
import android.os.Bundle;
import android.os.SystemProperties;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceFragment;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;
import android.provider.Settings;
import android.support.v4.app.FragmentActivity;
import android.text.TextUtils;
import android.util.StringBuilderPrinter;
import android.view.MenuItem;
import android.widget.Toast;

import com.android.camera.FatalErrorHandler;
import com.android.camera.FatalErrorHandlerImpl;
import com.android.camera.PermissionsActivity;
import com.android.camera.debug.Log;
import com.android.camera.one.OneCameraException;
import com.android.camera.one.OneCameraManager;
import com.android.camera.one.OneCameraModule;
import com.android.camera.settings.PictureSizeLoader.PictureSizes;
import com.android.camera.settings.SettingsUtil.SelectedVideoQualities;
import com.android.camera.util.CameraSettingsActivityHelper;
import com.android.camera.util.CameraUtil;
import com.android.camera.util.Size;
import com.android.camera2.R;
import com.android.ex.camera2.portability.CameraAgentFactory;
import com.android.ex.camera2.portability.CameraDeviceInfo;
import com.android.camera.qr.QrScanResultActivity;
import com.android.camera.qr.QrScanActivityCapture;
import com.sprd.camera.storagepath.MultiStorage;
import com.sprd.camera.storagepath.StorageUtil;
import com.ucamera.ucam.modules.utils.UCamUtill;

import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;

/**
 * Provides the settings UI for the Camera app.
 */
public class CameraSettingsActivity extends FragmentActivity {

    /**
     * Used to denote a subsection of the preference tree to display in the
     * Fragment. For instance, if 'Advanced' key is provided, the advanced
     * preference section will be treated as the root for display. This is used
     * to enable activity transitions between preference sections, and allows
     * back/up stack to operate correctly.
     */
    public static final String PREF_SCREEN_EXTRA = "pref_screen_extra";
    public static final String HIDE_ADVANCED_SCREEN = "hide_advanced";
    public static final String PERSIST_CAMERA_SMILE = "persist.sys.cam.smile";//SPRD：Add for ai detect
    private OneCameraManager mOneCameraManager;
    public static ArrayList<Context> contexts = new ArrayList<Context>();
    public static String scopeName;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // SPRD Bug:474694 Feature:Reset Settings.
        contexts.add(this);

        FatalErrorHandler fatalErrorHandler = new FatalErrorHandlerImpl(this);
        boolean hideAdvancedScreen = false;

        try {
            mOneCameraManager = OneCameraModule.provideOneCameraManager();
        } catch (OneCameraException e) {
            // Log error and continue. Modules requiring OneCamera should check
            // and handle if null by showing error dialog or other treatment.
            fatalErrorHandler.onGenericCameraAccessFailure();
        }

        // Check if manual exposure is available, so we can decide whether to
        // display Advanced screen.
        /**
         * SPRD BUG 506348: clicking "advanced setting" under setting UI make error.
         * according to current advanced setting logic, the advanced screen always be shown@{
         * Original Code

        try {
            CameraId frontCameraId = mOneCameraManager.findFirstCameraFacing(Facing.FRONT);
            CameraId backCameraId = mOneCameraManager.findFirstCameraFacing(Facing.BACK);

            // The exposure compensation is supported when both of the following conditions meet
            //   - we have the valid camera, and
            //   - the valid camera supports the exposure compensation
            boolean isExposureCompensationSupportedByFrontCamera = (frontCameraId != null) &&
                    (mOneCameraManager.getOneCameraCharacteristics(frontCameraId)
                            .isExposureCompensationSupported());
            boolean isExposureCompensationSupportedByBackCamera = (backCameraId != null) &&
                    (mOneCameraManager.getOneCameraCharacteristics(backCameraId)
                            .isExposureCompensationSupported());

            // Hides the option if neither front and back camera support exposure compensation.
            if (!isExposureCompensationSupportedByFrontCamera &&
                    !isExposureCompensationSupportedByBackCamera) {
                hideAdvancedScreen = true;
            }
        } catch (OneCameraAccessException e) {
            fatalErrorHandler.onGenericCameraAccessFailure();
        }
         */

        hideAdvancedScreen = false;
        /* @} */

        ActionBar actionBar = getActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setTitle(R.string.mode_settings);

        String prefKey = getIntent().getStringExtra(PREF_SCREEN_EXTRA);

        // SPRD: Fix bug 474843, New feature of Filter.
        this.mCurrentModeIndex = getIntent().getIntExtra(CURRENT_MODULE, 0);
        /*
         * SPRD Bug:474694 Feature:Reset Settings. @{
         * Original Android code:

        CameraSettingsFragment dialog = new CameraSettingsFragment();

         */
        scopeName = getIntent().getStringExtra(CAMERA_MODULE_SCOPENAME);
        String intentCameraScope = getIntent().getStringExtra(CAMERA_SCOPE);
        if ((prefKey == null && intentCameraScope != null)
                || (prefKey != null && CameraSettingsFragment.PREF_CATEGORY_ADVANCED
                        .equals(prefKey))) {
            this.mCameraScope = intentCameraScope;
        }

        CameraSettingsFragment dialog = new CameraSettingsFragment(mCameraScope, mCurrentModeIndex, this);
        /* @} */
        //SPRD:fix bug537963 pull sd card when lock screen
        dialog.installIntentFilter();

        Bundle bundle = new Bundle(1);
        bundle.putString(PREF_SCREEN_EXTRA, prefKey);
        bundle.putBoolean(HIDE_ADVANCED_SCREEN, hideAdvancedScreen);
        dialog.setArguments(bundle);
        getFragmentManager().beginTransaction().replace(android.R.id.content, dialog).commit();
    }

    @Override
    public boolean onMenuItemSelected(int featureId, MenuItem item) {
        int itemId = item.getItemId();
        if (itemId == android.R.id.home) {
            finish();
            return true;
        }
        return true;
    }

    public static class CameraSettingsFragment extends PreferenceFragment implements
            OnSharedPreferenceChangeListener {

        public static final String PREF_CATEGORY_RESOLUTION = "pref_category_resolution";
        public static final String PREF_CATEGORY_ADVANCED = "pref_category_advanced";
        public static final String PREF_LAUNCH_HELP = "pref_launch_help";
        public static final String PREF_CAMERA_STORAGE_PATH = "pref_camera_storage_path";
        private static final Log.Tag TAG = new Log.Tag("SettingsFragment");
        private static DecimalFormat sMegaPixelFormat = new DecimalFormat("##0.0");
        private static final String INTENT_START_ZXING_ACTIVITY = "com.android.camera.START_ZXING_ACTIVITY";
        // SPRD: Fix bug 548643 that AlertDialog.show() throws WindowManager$BadTokenException
        // because smContext (actually activity) have been destroyed
        private Context mContext;

        private String[] mCamcorderProfileNames;
        //SPRD:add for smile capture Bug548832
        private CameraDeviceInfo mInfos;
        private CameraDeviceInfo mInfos2;
        private String mPrefKey;
        private boolean mHideAdvancedScreen;
        private boolean mGetSubPrefAsRoot = true;
        private List<String> mSupportedStorage;
        // SPRD: Fix bug 572309 camera GPS function
        private boolean isSupportGps = CameraUtil.isRecordLocationEnable();// SPRD: fix for bug 499642 delete location save  function
        /*
         * SPRD: mutex - Premise: Exposure =3, HDR = on; Action: Set sceneMode = action; Result:
         * HDR off. And Exposure = 0. Expected: Exposure = 3.
         */
        public static boolean mNeedCheckMutex = false;

        // Selected resolutions for the different cameras and sizes.
        private PictureSizes mPictureSizes;
        //Bug 474696 slow motion with 720p
        private final int VALUE_720p = 5;
        private final int VALUE_2160P = 8;
        private String entryValue_720p = null;
        private String entryValue_2160P = null;

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            Bundle arguments = getArguments();
            if (arguments != null) {
                mPrefKey = arguments.getString(PREF_SCREEN_EXTRA);
                mHideAdvancedScreen = arguments.getBoolean(HIDE_ADVANCED_SCREEN);
            }
            Context context = this.getActivity().getApplicationContext();
            addPreferencesFromResource(R.xml.camera_preferences);
            PreferenceScreen advancedScreen =
                    (PreferenceScreen) findPreference(PREF_CATEGORY_ADVANCED);

            // If manual exposure not enabled, hide the Advanced screen.
            if (mHideAdvancedScreen) {
                PreferenceScreen root = (PreferenceScreen) findPreference("prefscreen_top");
                root.removePreference(advancedScreen);
            }

            // Allow the Helper to edit the full preference hierarchy, not the
            // sub tree we may show as root. See {@link #getPreferenceScreen()}.
            mGetSubPrefAsRoot = false;
            CameraSettingsActivityHelper.addAdditionalPreferences(this, context);
            mGetSubPrefAsRoot = true;

            mCamcorderProfileNames = getResources().getStringArray(R.array.camcorder_profile_names);
            mInfos = CameraAgentFactory
                    .getAndroidCameraAgent(context, CameraAgentFactory.CameraApi.API_1)
                    .getCameraDeviceInfo();
            //SPRD:add for smile capture Bug548832
            mInfos2 = CameraAgentFactory
                    .getAndroidCameraAgent(context, CameraAgentFactory.CameraApi.API_2)
                    .getCameraDeviceInfo();
            // SPRD: Fix bug 545710 The sAndroidCameraAgentClientCount is
            // keeping increase.
            CameraAgentFactory.recycle(CameraAgentFactory.CameraApi.API_1);
            CameraAgentFactory.recycle(CameraAgentFactory.CameraApi.API_2);
        }

        @Override
        public void onResume() {
            super.onResume();
            /*
             * SPRD modify for Coverity 109125@{
             * Original Android code:
            final Activity activity = this.getActivity();
            @}*/

            // Load the camera sizes.
            loadSizes();
            loadStorageDirectories();

            // Send loaded sizes to additional preferences.
            CameraSettingsActivityHelper.onSizesLoaded(this, mPictureSizes.backCameraSizes,
                    new ListPreferenceFiller() {
                        @Override
                        public void fill(List<Size> sizes, ListPreference preference) {
                            setEntriesForSelection(sizes, preference);
                        }
                    });

            // Make sure to hide settings for cameras that don't exist on this
            // device.
            setVisibilities();

            // Put in the summaries for the currently set values.
            final PreferenceScreen resolutionScreen =
                    (PreferenceScreen) findPreference(PREF_CATEGORY_RESOLUTION);
            fillEntriesAndSummaries(resolutionScreen);
            setPreferenceScreenIntent(resolutionScreen);

            final ListPreference cameraStoragePath = (ListPreference) findPreference(PREF_CAMERA_STORAGE_PATH);
            /*
             * SPRD: fix bug 537963 pull sd card when lock screen @{
             * SPRD: Fix bug 572473 add for usb storage support
             */
            SettingsManager settingsManager = new SettingsManager(mContext);
            String currentStoragePath = settingsManager
                    .getString(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_STORAGE_PATH);
            if (!StorageUtil.getStoragePathState(currentStoragePath)) {
                updatePreferredStorage();
            }

            // if we set usb storage as preferred storage, then reenter the CameraSettingsActivity,
            // we'll see no summary showing, because 'cameraStoragePath.getEntry()' return null,
            // no idea for why, just do workaround here
            if (!TextUtils.isEmpty(cameraStoragePath.getEntry())) {
                cameraStoragePath.setSummary(cameraStoragePath.getEntry());
            } else {
                cameraStoragePath.setSummary(currentStoragePath);
            }
            /* @} */
            setEntries(cameraStoragePath);

            /* SPRD:Bug 535058 New feature: volume @{ */
            final ListPreference mVolume =
                    (ListPreference) findPreference(Keys.KEY_CAMERA_VOLUME);
            mVolume.setSummary(mVolume.getEntry());
            setEntries(mVolume);
            /* @} */

            /* SPRD: fix bug 562263 loadSlowMotion in onResume instead of onCreat @{ */
            loadSlowMotion();
            /* @} */

            final PreferenceScreen advancedScreen =
                    (PreferenceScreen) findPreference(PREF_CATEGORY_ADVANCED);

            if (!mHideAdvancedScreen) {
                fillEntriesAndSummaries(advancedScreen);
                setPreferenceScreenIntent(advancedScreen);
            }

            final PreferenceScreen debounceScreen =
                    (PreferenceScreen) findPreference("pref_eois_resolution");
            if (debounceScreen != null) {
                fillEntriesAndSummaries(debounceScreen);
                setPreferenceScreenIntent(debounceScreen);
            }

            /*
             * SPRD Bug:474694 Feature:Reset Settings. @{
             */
            Preference resetCamera = findPreference(Keys.KEY_CAMER_RESET);
            resetCamera.setOnPreferenceClickListener(new OnPreferenceClickListener() {
                @Override
                public boolean onPreferenceClick(Preference preference) {
                    showAlertDialog(true);
                    return true;
                }
            });
            /* @} */

            /*
             * fix bug 578797 New feature:QR code. @{
             */
          PreferenceScreen qrCode = (PreferenceScreen)findPreference(Keys.PREF_KEY_QRCODE);
          Log.i(TAG, "qrCode = " + qrCode);
          if(qrCode != null){
          qrCode.setOnPreferenceClickListener(new OnPreferenceClickListener() {
             @Override
             public boolean onPreferenceClick(Preference preference) {
                // Intent intent = new Intent(INTENT_START_ZXING_ACTIVITY);
                Intent intent = new Intent();
                intent.setClassName(mContext,
                       "com.android.camera.qr.QrScanActivityCapture");
                mContext.startActivity(intent);
                CameraSettingsActivity cameraSettingsActivity = (CameraSettingsActivity) mContext;
                cameraSettingsActivity.finish();
                return true;
                }
            });
          }
            /* @} */
            /*
             * SPRD Bug:488399 Remove Google Help and Feedback. @{
             * Original Android code:

            Preference helpPref = findPreference(PREF_LAUNCH_HELP);
            helpPref.setOnPreferenceClickListener(
                    new OnPreferenceClickListener() {
                        @Override
                        public boolean onPreferenceClick(Preference preference) {
                            new GoogleHelpHelper(activity).launchGoogleHelp();
                            return true;
                        }
                    });

             */

            /* SPRD: add Feature of Quick Capture @{ */
            if (UCamUtill.isQuickCaptureEnabled()) {
                ListPreference speedCapturePref = (ListPreference)findPreference(Keys.KEY_QUICK_CAPTURE);
                if (SystemProperties.getBoolean("persist.sys.cam.hascamkey", false)) {
                    speedCapturePref.setTitle(R.string.pref_camera_quick_capture_title_camkey);
                    speedCapturePref.setDialogTitle(R.string.pref_camera_quick_capture_title_camkey);
                }
                speedCapturePref.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
                    @Override
                    public boolean onPreferenceChange(Preference preference, Object newValue) {
                        String mode = (String)newValue;
                        Log.d(TAG,"Quick Capture mode id = " + mode);
                        /* SPRD: Fix bug 567394 when switch user the state of the quickcamera is wrong @{ */
                        Settings.Global.putString(getActivity().getContentResolver()
                                , "camera_quick_capture_userid_" + ActivityManager.getCurrentUser(), mode);
                        /* @} */
                        return true;
                    }
                });

            ListPreference storagePathPref = (ListPreference)findPreference(Keys.KEY_CAMERA_STORAGE_PATH);
            storagePathPref.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
                @Override
                public boolean onPreferenceChange(Preference preference, Object newValue) {
                    Log.d(TAG,"the storage value is" + (String)newValue);
                    Settings.Global.putString(getActivity().getContentResolver(),"camera_quick_capture_storage_path", (String)newValue);
                    return true;
                }
            });

            }
            /* @} */

            getPreferenceScreen().getSharedPreferences()
                    .registerOnSharedPreferenceChangeListener(this);
        }

        /**
         * Configure home-as-up for sub-screens.
         */
        private void setPreferenceScreenIntent(final PreferenceScreen preferenceScreen) {
            Intent intent = new Intent(getActivity(), CameraSettingsActivity.class);
            intent.putExtra(PREF_SCREEN_EXTRA, preferenceScreen.getKey());

            // SPRD Bug:474694 Feature:Reset Settings.
            if (PREF_CATEGORY_ADVANCED.equals(preferenceScreen.getKey())) {
                if (mCameraScope != null) {
                    intent.putExtra(CAMERA_SCOPE, mCameraScope);
                }
                intent.putExtra(CURRENT_MODULE, mCurrentModeIndex);
            }

            preferenceScreen.setIntent(intent);
        }

        /**
         * This override allows the CameraSettingsFragment to be reused for
         * different nested PreferenceScreens within the single camera
         * preferences XML resource. If the fragment is constructed with a
         * desired preference key (delivered via an extra in the creation
         * intent), it is used to look up the nested PreferenceScreen and
         * returned here.
         */
        @Override
        public PreferenceScreen getPreferenceScreen() {
            PreferenceScreen root = super.getPreferenceScreen();
            if (!mGetSubPrefAsRoot || mPrefKey == null || root == null) {
                return root;
            } else {
                PreferenceScreen match = findByKey(root, mPrefKey);
                if (match != null) {
                    return match;
                } else {
                    throw new RuntimeException("key " + mPrefKey + " not found");
                }
            }
        }

        private PreferenceScreen findByKey(PreferenceScreen parent, String key) {
            if (key.equals(parent.getKey())) {
                return parent;
            } else {
                for (int i = 0; i < parent.getPreferenceCount(); i++) {
                    Preference child = parent.getPreference(i);
                    if (child instanceof PreferenceScreen) {
                        PreferenceScreen match = findByKey((PreferenceScreen) child, key);
                        if (match != null) {
                            return match;
                        }
                    }
                }
                return null;
            }
        }

        /**
         * Depending on camera availability on the device, this removes settings
         * for cameras the device doesn't have.
         */
        private void setVisibilities() {
            /* PRD: fix for bug 499642 delete location save function @{ */
            PreferenceGroup resolutions_gategory =
                    (PreferenceScreen) findPreference("prefscreen_top");

            PreferenceGroup resolutions_advanced =
                    (PreferenceGroup) findPreference(PREF_CATEGORY_ADVANCED);

            PreferenceGroup eois_resolultions =
                    (PreferenceScreen) findPreference(Keys.KEY_EOIS_RESOLUTION);

            if (!isSupportGps) {
                recursiveDelete(resolutions_gategory, findPreference(Keys.KEY_RECORD_LOCATION));
            }
            /* @} */
            PreferenceGroup resolutions =
                    (PreferenceGroup) findPreference(PREF_CATEGORY_RESOLUTION);

            if (mPictureSizes.backCameraSizes.isEmpty()) {
                recursiveDelete(resolutions,
                        findPreference(Keys.KEY_PICTURE_SIZE_BACK));
                recursiveDelete(resolutions,
                        findPreference(Keys.KEY_VIDEO_QUALITY_BACK));
            }
            if (mPictureSizes.frontCameraSizes.isEmpty()) {
                recursiveDelete(resolutions,
                        findPreference(Keys.KEY_PICTURE_SIZE_FRONT));
                recursiveDelete(resolutions,
                        findPreference(Keys.KEY_VIDEO_QUALITY_FRONT));
            }
            /* SPRD:Fix bug 447953,548832 @{ */
            if (mInfos2 != null && !mInfos2.getSmileEnable()) {
                ListPreference keyPreference =
                        (ListPreference) findPreference(Keys.KEY_CAMERA_AI_DATECT);
                keyPreference.setEntries(R.array.pref_camera_ai_detect_entries_removesmile);
            }
            /* @} */

            /* SPRD:add for antiband auto Bug549740 @{ */
            ListPreference keyPreference_camera =
                    (ListPreference) findPreference(Keys.KEY_CAMER_ANTIBANDING);
            ListPreference keyPreference_video =
                    (ListPreference) findPreference(Keys.KEY_VIDEO_ANTIBANDING);
            Log.d(TAG, "mInfos2.getAntibandAutoEnable() = " + mInfos2.getAntibandAutoEnable());
            if (keyPreference_camera != null && keyPreference_video != null) {
                if (mInfos2 != null && mInfos2.getAntibandAutoEnable()) {
                    keyPreference_camera.setEntries(R.array.pref_camera_antibanding_entries_addauto);
                    keyPreference_video.setEntries(R.array.pref_camera_antibanding_entries_addauto);
                    if (keyPreference_camera.getValue() == null) {
                        keyPreference_camera.setValue(mContext
                                .getString(R.string.pref_camera_antibanding_entryvalue_auto));
                    }
                    if (keyPreference_video.getValue() == null) {
                        keyPreference_video.setValue(mContext
                                .getString(R.string.pref_camera_antibanding_entryvalue_auto));
                    }
                } else {
                    if (keyPreference_camera.getValue() == null) {
                        keyPreference_camera.setValue(mContext
                                .getString(R.string.pref_camera_antibanding_entryvalue_50));
                    }
                    if (keyPreference_video.getValue() == null) {
                        keyPreference_video.setValue(mContext
                                .getString(R.string.pref_camera_antibanding_entryvalue_50));
                    }
                }
            }
            /* @} */

            if (!UCamUtill.isTimeStampEnable()) {
                recursiveDelete(resolutions_advanced,
                        findPreference(Keys.KEY_CAMERA_TIME_STAMP));
            }

            if(!CameraUtil.isZslEnable()) {
                recursiveDelete(resolutions_advanced,
                        findPreference(Keys.KEY_CAMERA_ZSL_DISPLAY));
            }

            /* SPRD: Fix 474843 Add for Filter Feature @{ */
            if ((mCurrentModeIndex == mContext.getResources()
                    .getInteger(R.integer.camera_mode_filter))) {
                recursiveDelete(resolutions_advanced,
                        findPreference(Keys.KEY_CAMERA_TIME_STAMP));
                recursiveDelete(resolutions_advanced,
                        findPreference(Keys.KEY_CAMERA_ZSL_DISPLAY));
                recursiveDelete(resolutions_advanced,
                        findPreference(Keys.KEY_FREEZE_FRAME_DISPLAY));
                recursiveDelete(resolutions_advanced
                        , findPreference(Keys.KEY_CAMERA_COLOR_EFFECT));
                recursiveDelete(resolutions_advanced
                        , findPreference(Keys.KEY_CAMERA_CONTINUE_CAPTURE));
                /* SPRD: Fix bug 569058 filter do mutex @{ */
                recursiveDelete(resolutions_advanced
                        , findPreference(Keys.KEY_WHITE_BALANCE));
                recursiveDelete(resolutions_advanced
                        , findPreference(Keys.KEY_CAMERA_CONTRAST));
                recursiveDelete(resolutions_advanced
                        , findPreference(Keys.KEY_CAMERA_BRIGHTNESS));
                recursiveDelete(resolutions_advanced
                        , findPreference(Keys.KEY_CAMERA_SATURATION));
                /* @} */

                // SPRD : Add FILTER-HIGHISO mutex
                recursiveDelete(resolutions_advanced, findPreference(Keys.KEY_HIGH_ISO));
            }
            /* @} */

            if (!UCamUtill.isGifEnable()) {
                recursiveDelete(resolutions_advanced, findPreference(Keys.KEY_GIF_ADVANCED_SETTINGS));
            }

            /* SPRD: Fix bug 531780 and 474843 @{ */
            SettingsManager settingsManager = new SettingsManager(mContext);
            if (settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_BEAUTY_ENTERED)) {
                recursiveDelete(resolutions_advanced, findPreference(Keys.KEY_CAMERA_CONTINUE_CAPTURE));
                // SPRD : Add BEAUTY-HIGHISO mutex
                recursiveDelete(resolutions_advanced, findPreference(Keys.KEY_HIGH_ISO));
            }
            /* @} */

            /* SPRD:Fix bug 500099 front camera need mirror function @{ */
            if(!CameraUtil.isFrontCameraMirrorEnable()) {
                recursiveDelete(resolutions_advanced,
                findPreference(Keys.KEY_FRONT_CAMERA_MIRROR));
                }
            /* @} */

            /* SPRD:add for new encoder @{ */
            if (CameraUtil.isSupportH265()) {
                ListPreference keyPreference =
                        (ListPreference) findPreference(Keys.KEY_VIDEO_ENCODE_TYPE);
                keyPreference.setEntries(R.array.pref_video_encode_type_entries_add_H256);
                keyPreference.setEntryValues(R.array.pref_video_encode_type_entry_values); // SPRD: Fix bug 580489
                /* Bug 474696 set the encode type is default H265 when opened slow motion @{ */
                ListPreference slowMotionPreference = (ListPreference) findPreference(Keys.KEY_VIDEO_SLOW_MOTION);

                String videoQuality = settingsManager.getString(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_QUALITY_BACK);
                int quality = SettingsUtil.getVideoQuality(videoQuality, 0);
                Log.d(TAG, "Selected video quality for '" + videoQuality + "' is " + quality);
                if ((slowMotionPreference != null
                        && mContext.getString(R.string.pref_entry_value_one) != null
                        && !mContext.getString(R.string.pref_entry_value_one).equals(slowMotionPreference.getValue()))
                        || (quality == CamcorderProfile.QUALITY_2160P)) {
                    keyPreference.setEntries(R.array.pref_video_encode_type_entries_H265);
                    keyPreference.setEntryValues(R.array.pref_video_encode_type_entry_values_H265);
                }
                /* @} */
            }
            /* @} */

            /* SPRD: Add for quick capture @{ */
            if (!UCamUtill.isQuickCaptureEnabled()) {
                recursiveDelete(resolutions_advanced, findPreference(Keys.KEY_QUICK_CAPTURE));
            }

            /* SPRD: New feature vgesture detect @{ */
            if (!UCamUtill.isVgestureEnnable()) {
                recursiveDelete(resolutions_advanced,
                        findPreference(Keys.KEY_CAMERA_GRID_LINES));
            }

            /* SPRD: Fix Bug 534257 New Feature EIS&OIS @{ */
            if (!(CameraUtil.isEOISDcBackEnabled()
                    || CameraUtil.isEOISDcFrontEnabled()
                    || CameraUtil.isEOISDvBackEnabled()
                    || CameraUtil.isEOISDvFrontEnabled())) {
                recursiveDelete(resolutions_gategory,
                        findPreference(Keys.KEY_EOIS_RESOLUTION));
            } else {
                if (!CameraUtil.isEOISDcBackEnabled()) {
                    recursiveDelete(eois_resolultions,
                            findPreference(Keys.KEY_EOIS_DC_BACK));
                }
                if (!CameraUtil.isEOISDcFrontEnabled()) {
                    recursiveDelete(eois_resolultions,
                            findPreference(Keys.KEY_EOIS_DC_FRONT));
                }
                if (!CameraUtil.isEOISDvBackEnabled()) {
                    recursiveDelete(eois_resolultions,
                            findPreference(Keys.KEY_EOIS_DV_BACK));
                }
                if (!CameraUtil.isEOISDvFrontEnabled()) {
                    recursiveDelete(eois_resolultions,
                            findPreference(Keys.KEY_EOIS_DV_FRONT));
                }
            }
            /* @} */

            /* SPRD: Add for color effect @{ */
            if (!CameraUtil.isColorEffectEnabled()) {
                recursiveDelete(resolutions_advanced, findPreference(Keys.KEY_CAMERA_COLOR_EFFECT));
                recursiveDelete(resolutions_advanced, findPreference(Keys.KEY_VIDEO_COLOR_EFFECT));
            }
            /* @} */

            /* SPRD: Add for slow motion @{ */
            if (!CameraUtil.isSlowMotionEnabled()) {
                recursiveDelete(resolutions_advanced, findPreference(Keys.KEY_VIDEO_SLOW_MOTION));
            }
            /* @} */

            /* SPRD: Fix bug 569496 front camera and slowmotion do mutex @{ */
            int cameraId = settingsManager.getInteger(SettingsManager
                    .getModuleSettingScope(SettingsScopeNamespaces.VIDEO), Keys.KEY_CAMERA_ID);
            if (CameraUtil.isSlowMotionEnabled()
                    && mCurrentModeIndex == mContext.getResources().getInteger(R.integer.camera_mode_video)
                    && CameraUtil.isFrontCameraIntent(cameraId)) {
                recursiveDelete(resolutions_advanced, findPreference(Keys.KEY_VIDEO_SLOW_MOTION));
            }
            /* @} */

            /*
             * fix bug 578797 New feature:QR code. @{
             */
            if(CameraUtil.isQrCodeEnabled()){
                int cameraid = settingsManager.getInteger(SettingsManager
                        .getModuleSettingScope(scopeName), Keys.KEY_CAMERA_ID);
                if (cameraid == 1) {
                    recursiveDelete(resolutions_gategory, findPreference(Keys.PREF_KEY_QRCODE));
                }
            }else{
                recursiveDelete(resolutions_gategory, findPreference(Keys.PREF_KEY_QRCODE));
            }

            /* @} */

            //SPRD Add for highiso
            if (!CameraUtil.isHighISOEnable()) {
                recursiveDelete(resolutions_advanced, findPreference(Keys.KEY_HIGH_ISO));
            }

            /* SPRD: Fix bug 535110, Photo voice record. @{ */
            if (!CameraUtil.isVoicePhotoEnable()) {
                recursiveDelete(resolutions_advanced,
                        findPreference(Keys.KEY_CAMERA_RECORD_VOICE));
            }
            /* @} */
        }

        /**
         * SPRD:Bug 557798 dynamic display the values of slow motion.
         * load the slow motion data into ListPreference.
         */
        private void loadSlowMotion() {
            ListPreference keyPreference_slowMotion =
                    (ListPreference) findPreference(Keys.KEY_VIDEO_SLOW_MOTION);
            SettingsManager sm = new SettingsManager(mContext);
            if (keyPreference_slowMotion != null && sm != null) {
                String slowMotion = sm.getString(SettingsManager.SCOPE_GLOBAL,
                        Keys.KEY_VIDEO_SLOW_MOTION_ALL);
                if (slowMotion != null) {
                    String[] slowMotionValues = slowMotion.split(",");
                    int k = 0;
                    if (slowMotionValues != null && slowMotionValues.length > 1
                            && "0".equals(slowMotionValues[0])
                            && "1".equals(slowMotionValues[1])) {
                        k = 1;
                    }
                    if (slowMotionValues != null) {
                        String[] slowMotionEntries = new String[slowMotionValues.length - k];
                        String[] slowMotionEntryValues = new String[slowMotionValues.length - k];
                        int i = 0;
                        for (String entryValue : slowMotionValues) {
                            if ("0".equals(entryValue))
                                continue;
                            if ("1".equals(entryValue)) {
                                slowMotionEntries[i] = mContext
                                        .getString(R.string.pref_entry_title_off);
                            } else {
                                slowMotionEntries[i] = entryValue;
                            }
                            slowMotionEntryValues[i] = entryValue;
                            ++i;
                        }
                        keyPreference_slowMotion.setEntries(slowMotionEntries);
                        keyPreference_slowMotion.setEntryValues(slowMotionEntryValues);
                    }
                    sm.setDefaults(Keys.KEY_VIDEO_SLOW_MOTION,
                            mContext.getString(R.string.pref_entry_value_one),
                            slowMotionValues);
                }
            }
        }

        /**
         * Recursively go through settings and fill entries and summaries of our
         * preferences.
         */
        private void fillEntriesAndSummaries(PreferenceGroup group) {
            for (int i = 0; i < group.getPreferenceCount(); ++i) {
                Preference pref = group.getPreference(i);
                if (pref instanceof PreferenceGroup) {
                    fillEntriesAndSummaries((PreferenceGroup) pref);
                }
                setSummary(pref);
                setEntries(pref);
            }
        }

        /**
         * Recursively traverses the tree from the given group as the route and
         * tries to delete the preference. Traversal stops once the preference
         * was found and removed.
         */
        private boolean recursiveDelete(PreferenceGroup group, Preference preference) {
            if (group == null) {
                Log.d(TAG, "attempting to delete from null preference group");
                return false;
            }
            if (preference == null) {
                Log.d(TAG, "attempting to delete null preference");
                return false;
            }
            if (group.removePreference(preference)) {
                // Removal was successful.
                return true;
            }

            for (int i = 0; i < group.getPreferenceCount(); ++i) {
                Preference pref = group.getPreference(i);
                if (pref instanceof PreferenceGroup) {
                    if (recursiveDelete((PreferenceGroup) pref, preference)) {
                        return true;
                    }
                }
            }
            return false;
        }

        @Override
        public void onPause() {
            super.onPause();

            // SPRD Bug:474694 Feature:Reset Settings.
            mResetCamera = false;

            ListPreference storagePreference = (ListPreference) findPreference(Keys.KEY_CAMERA_STORAGE_PATH);
            /*SPRD:fix bug537963 pull sd card when lock screen@*/
            if (storagePreference.getDialog() != null) {
                storagePreference.getDialog().dismiss();
            }
            /*@}*/
            getPreferenceScreen().getSharedPreferences()
                    .unregisterOnSharedPreferenceChangeListener(this);
        }

        @Override
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
            setSummary(findPreference(key));
            setMutexPreference(key);
        }

        /**
         * Set the entries for the given preference. The given preference needs
         * to be a {@link ListPreference}
         */
        private void setEntries(Preference preference) {
            if (!(preference instanceof ListPreference)) {
                return;
            }

            ListPreference listPreference = (ListPreference) preference;
            /* SPRD: Fix bug 567399 VGesture and AiDetect is mutex @{ */
            SettingsManager sm = new SettingsManager(mContext);
            if (listPreference.getKey().equals(Keys.KEY_CAMERA_AI_DATECT) && Keys.isVGestureOn(sm)) {
                String [] aiDetectProfileNames = getResources().getStringArray(R.array.pref_camera_ai_detect_entries);
                ArrayList<String> entries = new ArrayList<String>();
                entries.add(aiDetectProfileNames[1]);
                listPreference.setEntries(entries.toArray(new String[0]));

                String[] entryValues = {
                    "face"
                };
                listPreference.setEntryValues(entryValues);
            } else if (listPreference.getKey().equals(Keys.KEY_PICTURE_SIZE_BACK)) {
            /* @} */
                /*
                 * SPRD Bug:474694 Feature:Reset Settings. @{
                 * Original Android code:

                setEntriesForSelection(mPictureSizes.backCameraSizes, listPreference);

                 */
                setEntriesForSelection(mPictureSizes.backCameraSizes, listPreference,
                        Keys.KEY_PICTURE_SIZE_BACK);
                /* @} */
            } else if (listPreference.getKey().equals(Keys.KEY_PICTURE_SIZE_FRONT)) {
                /*
                 * SPRD Bug:474694 Feature:Reset Settings. @{
                 * Original Android code:

                setEntriesForSelection(mPictureSizes.frontCameraSizes, listPreference);

                 */
                setEntriesForSelection(mPictureSizes.frontCameraSizes, listPreference,
                        Keys.KEY_PICTURE_SIZE_FRONT);
                /* @} */
            } else if (listPreference.getKey().equals(Keys.KEY_VIDEO_QUALITY_BACK)) {
                setEntriesForSelection(mPictureSizes.videoQualitiesBack.orNull(), listPreference);
            } else if (listPreference.getKey().equals(Keys.KEY_VIDEO_QUALITY_FRONT)) {
                setEntriesForSelection(mPictureSizes.videoQualitiesFront.orNull(), listPreference);
            } else if (listPreference.getKey().equals(Keys.KEY_CAMERA_STORAGE_PATH)) {
                setEntriesForSelectionStorage(mSupportedStorage, listPreference);
            /**
             * SPRD:fix bug 473462 add for burst capture @{
            }
            */
            } else if (listPreference.getKey().equals(Keys.KEY_CAMERA_CONTINUE_CAPTURE)) {
                SettingsManager settingsManager = new SettingsManager(mContext);//SPRD:fix bug474672
                if (!CameraUtil.isNinetyNineBurstEnabled()
                        || settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL,
                                Keys.KEY_CAMERA_BEAUTY_ENTERED)) {
                    setEntriesForBurst(listPreference);
                }
            }
            /**
             * @}
             */
        }

        /**
         * Set the summary for the given preference. The given preference needs
         * to be a {@link ListPreference}.
         */
        private void setSummary(Preference preference) {
            if (!(preference instanceof ListPreference)) {
                return;
            }

            ListPreference listPreference = (ListPreference) preference;
            if (listPreference.getKey().equals(Keys.KEY_PICTURE_SIZE_BACK)) {
                setSummaryForSelection(mPictureSizes.backCameraSizes,
                        listPreference);
            } else if (listPreference.getKey().equals(Keys.KEY_PICTURE_SIZE_FRONT)) {
                setSummaryForSelection(mPictureSizes.frontCameraSizes,
                        listPreference);
            } else if (listPreference.getKey().equals(Keys.KEY_VIDEO_QUALITY_BACK)) {
                setSummaryForSelection(mPictureSizes.videoQualitiesBack.orNull(), listPreference);
            } else if (listPreference.getKey().equals(Keys.KEY_VIDEO_QUALITY_FRONT)) {
                setSummaryForSelection(mPictureSizes.videoQualitiesFront.orNull(), listPreference);
            } else {
                listPreference.setSummary(listPreference.getEntry());
            }
        }

        /**
         * Sets the entries for the given list preference.
         *
         * @param selectedSizes The possible S,M,L entries the user can choose
         *            from.
         * @param preference The preference to set the entries for.
         */
        private void setEntriesForSelection(List<Size> selectedSizes,
                ListPreference preference) {
            if (selectedSizes == null) {
                return;
            }

            String[] entries = new String[selectedSizes.size()];
            String[] entryValues = new String[selectedSizes.size()];
            for (int i = 0; i < selectedSizes.size(); i++) {
                Size size = selectedSizes.get(i);
                entries[i] = getSizeSummaryString(size);
                entryValues[i] = SettingsUtil.sizeToSettingString(size);
            }
            preference.setEntries(entries);
            preference.setEntryValues(entryValues);
        }

        /**
         * Sets the entries for the given list preference.
         *
         * @param selectedQualities The possible S,M,L entries the user can
         *            choose from.
         * @param preference The preference to set the entries for.
         */
        private void setEntriesForSelection(SelectedVideoQualities selectedQualities,
                ListPreference preference) {
            if (selectedQualities == null) {
                return;
            }
            /* Bug 474696 set the back camera video is default 720p when opened slow motion @{ */
            if (selectedQualities.large == VALUE_720p) {
                entryValue_720p = "large";
            } else if (selectedQualities.medium == VALUE_720p) {
                entryValue_720p = "medium";
            } else if (selectedQualities.small == VALUE_720p) {
                entryValue_720p = "small";
            }
            if (selectedQualities.large == VALUE_2160P) {
                entryValue_2160P= "large";
            } else if (selectedQualities.medium == VALUE_2160P) {
                entryValue_2160P = "medium";
            } else if (selectedQualities.small == VALUE_2160P) {
                entryValue_2160P = "small";
            }
            ArrayList<String> entries = new ArrayList<String>();
            if (isSlowMotionOn() && entryValue_720p != null && preference.getKey() != null
                    && preference.getKey().equals(Keys.KEY_VIDEO_QUALITY_BACK)) {
                entries.add(mCamcorderProfileNames[VALUE_720p]);
                preference.setEntries(entries.toArray(new String[0]));
                String[] entryValues = {
                    entryValue_720p
                };
                preference.setEntryValues(entryValues);
            } else {
                // Avoid adding double entries at the bottom of the list which
                // indicates that not at least 3 qualities are supported.
                entries.add(mCamcorderProfileNames[selectedQualities.large]);
                if (selectedQualities.medium != selectedQualities.large) {
                    entries.add(mCamcorderProfileNames[selectedQualities.medium]);
                }
                if (selectedQualities.small != selectedQualities.medium) {
                    entries.add(mCamcorderProfileNames[selectedQualities.small]);
                }
                preference.setEntries(entries.toArray(new String[0]));
                if (preference.getKey() != null
                        && preference.getKey().equals(Keys.KEY_VIDEO_QUALITY_BACK)) {
                    SettingsManager sm = new SettingsManager(mContext);
                    String videoQualityBackLastValue = sm.getString(SettingsManager.SCOPE_GLOBAL,
                            Keys.KEY_VIDEO_QUALITY_BACK_LAST);
                    if (videoQualityBackLastValue != null && !videoQualityBackLastValue.equals(entryValue_720p)) {
                        preference.setValue(videoQualityBackLastValue);
                        setSummaryForSelection(selectedQualities,preference);
                        sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_QUALITY_BACK,
                                videoQualityBackLastValue);
                        sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_QUALITY_BACK_LAST,
                                null);
                    }
                }
            } /* @} */
        }
        // Bug 474696 slow motion
        private boolean isSlowMotionOn() {
            SettingsManager sm = new SettingsManager(mContext);
            String slowMotion = sm.getString(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_VIDEO_SLOW_MOTION, Keys.SLOWMOTION_DEFAULT_VALUE);
            if (Integer.parseInt(slowMotion) > 1) {
                return true;
            } else {
                return false;
            }
        }


        /**
         * Sets the summary for the given list preference.
         *
         * @param displayableSizes The human readable preferred sizes
         * @param preference The preference for which to set the summary.
         */
        private void setSummaryForSelection(List<Size> displayableSizes,
                                            ListPreference preference) {
            String setting = preference.getValue();
            if (setting == null || !setting.contains("x")) {
                return;
            }
            Size settingSize = SettingsUtil.sizeFromSettingString(setting);
            if (settingSize == null || settingSize.area() == 0) {
                return;
            }
            preference.setSummary(getSizeSummaryString(settingSize));
        }

        /**
         * Sets the summary for the given list preference.
         *
         * @param selectedQualities The selected video qualities.
         * @param preference The preference for which to set the summary.
         */
        private void setSummaryForSelection(SelectedVideoQualities selectedQualities,
                ListPreference preference) {
            if (selectedQualities == null) {
                return;
            }

            int selectedQuality = selectedQualities.getFromSetting(preference.getValue());
            preference.setSummary(mCamcorderProfileNames[selectedQuality]);
        }

        /**
         * This method gets the selected picture sizes for S,M,L and populates
         * {@link #mPictureSizes} accordingly.
         */
        private void loadSizes() {
            if (mInfos == null) {
                Log.w(TAG, "null deviceInfo, cannot display resolution sizes");
                return;
            }
            PictureSizeLoader loader = new PictureSizeLoader(getActivity().getApplicationContext());
            mPictureSizes = loader.computePictureSizes();
        }

        /**
         * @param size The photo resolution.
         * @return A human readable and translated string for labeling the
         *         picture size in megapixels.
         */
        private String getSizeSummaryString(Size size) {
            Size approximateSize = ResolutionUtil.getApproximateSize(size);
            String megaPixels = sMegaPixelFormat.format((size.width() * size.height()) / 1e6);
            int numerator = ResolutionUtil.aspectRatioNumerator(approximateSize);
            int denominator = ResolutionUtil.aspectRatioDenominator(approximateSize);
            String result = getResources().getString(
                    R.string.setting_summary_aspect_ratio_and_megapixels, numerator, denominator,
                    megaPixels);
            return result;
        }

        /*SPRD: Add Storage check API for supportedStorage */
        public void loadStorageDirectories() {
            List<String> supportedStorage = MultiStorage.getSupportedStorage();
            if (supportedStorage != null) {
                mSupportedStorage = supportedStorage;
            }
        }

        /* SPRD: Add Storage Entries&EntrayValues API for storage setting list */
        public void setEntriesForSelectionStorage(List<String> supportedValue,
                ListPreference preference) {
            if (supportedValue == null) {
                return;
            }
            String[] entries = new String[supportedValue.size()];
            String[] entryValues = new String[supportedValue.size()];
            for (int i = 0; i < supportedValue.size(); i++) {
                String value = supportedValue.get(i);
                entries[i] = getStorageSummeryString(value);
                entryValues[i] = value;
            }
            preference.setEntries(entries);
            preference.setEntryValues(entryValues);
        }

        public String getStorageSummeryString(String value) {
            String entry = null;
            if (MultiStorage.KEY_DEFAULT_INTERNAL.equals(value)) {
                entry = getResources().getString(R.string.storage_path_internal);
            } else if (MultiStorage.KEY_DEFAULT_EXTERNAL.equals(value)) {
                entry = getResources().getString(R.string.storage_path_external);
            } else {
                // SPRD: Fix bug 572473 add for usb storage support
                entry = value;
            }
            return entry;
	}
        /**
         * SPRD: fix bug 473462 add for burst capture
         */
        private void setEntriesForBurst(ListPreference preference) {
            SettingsManager sm = new SettingsManager(mContext);

            String[] burstEntries = mContext.getResources().getStringArray(R.array.pref_camera_burst_entries);
            if (burstEntries != null) {
                String[] entries = new String[burstEntries.length - 1];
                int i = 0;
                for (String entry : burstEntries) {
                    if (!"99".equals(entry)) {
                        entries[i] = entry;
                        i++;
                    }
                }
                preference.setEntries(entries);
            }

            String[] burstCount = mContext.getResources().getStringArray(R.array.pref_camera_burst_entryvalues);
            if (burstCount != null) {
                String[] entryValues = new String[burstCount.length - 1];
                int i = 0;
                for (String entryValue : burstCount) {
                    if (!"ninetynine".equals(entryValue)) {
                        entryValues[i] = entryValue;
                        i++;
                    }
                }
                preference.setEntryValues(entryValues);
                sm.setDefaults(Keys.KEY_CAMERA_CONTINUE_CAPTURE,
                        mContext.getString(R.string.pref_camera_burst_entry_defaultvalue), entryValues);
            }
        }

        /* SPRD: setMutexPreference method is for Setting function Mutex in setting List @{ */
        public void setMutexPreference(String key) {
            Preference preference = findPreference(key);
            Log.d(TAG, "setMutexPreference key=" + key + ",preference=" + preference);
            if (preference instanceof ListPreference) {
                ListPreference listPreference = (ListPreference) preference;
            }
            if (preference instanceof ManagedSwitchPreference) {
                ManagedSwitchPreference switchPreference = (ManagedSwitchPreference) preference;
            }
            SettingsManager settingsManager = new SettingsManager(mContext);
            if (preference == null || key == null) {
                return;
            }

            String toastString = null;

            // SCENE MODE MUTEX WITH HDR, WHITE BALANCE, and FLASH MODE
            if (key.equals(Keys.KEY_SCENE_MODE) && preference instanceof ListPreference) {
                ListPreference listPreference = (ListPreference) preference;

                if (!mContext.getString(R.string.pref_camera_scenemode_default)
                        .equals(listPreference.getValue())) {
                    // SCENE MODE - HDR
                    if (Keys.isHdrOn(settingsManager)) {
                        //settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_SCENE_MODE,listPreference.getValue());
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_CAMERA_HDR, false);
                        toastString = mContext.getString(R.string.scene_mutex);
                        mNeedCheckMutex = true;
                    }

                    // SCENE MODE - FLASH MODE
                    //SPRD: Fix bug 542565
                    String cameraScope = "_preferences_camera_0";
                    if (!"off".equals(settingsManager.getString(cameraScope,
                            Keys.KEY_FLASH_MODE))) {
                        settingsManager.set(cameraScope, Keys.KEY_FLASH_MODE, "off");
                        toastString = mContext.getString(R.string.scene_mutex);
                    }

                    // SCENE MODE - WHITE BALANCE
                    ListPreference whiteBalancePreference = (ListPreference) findPreference(Keys.KEY_WHITE_BALANCE);
                    /* SPRD:fix bug576603 Add a judge to solve NullPointerException @{ */
                    if (whiteBalancePreference != null) {
                        whiteBalancePreference.setValue(mContext
                                .getString(R.string.pref_camera_whitebalance_default));
                    }
                    /* @} */
                    /* SPRD:fix bug534665 add some mutex about scene mode@{ */
                    // SCENE MODE - EXPOSURE
                    if (0 != settingsManager.getInteger(mCameraScope,
                            Keys.KEY_EXPOSURE)) {
                        settingsManager.set(mCameraScope, Keys.KEY_EXPOSURE, "0");
                        toastString = mContext.getString(R.string.scene_mutex);
                    }

                    //SPRD:fix bug534665 add some mutex about scene mode
                    // SCENE MODE - ISO
                    ListPreference ISOPreference = (ListPreference) findPreference(Keys.KEY_CAMERA_ISO);
                    ISOPreference.setValue(mContext.getString(R.string.pref_entry_value_auto));
                    // SCENE MODE - METER
                    String sceneMode = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,Keys.KEY_SCENE_MODE);
                    ListPreference meteringPreference = (ListPreference) findPreference(Keys.KEY_CAMER_METERING);
                    if(sceneMode.equals("landscape")){
                        meteringPreference.setValue(mContext
                                .getString(R.string.pref_camera_metering_entry_value_frame_average));
                    }
                    if(sceneMode.equals("portrait")){
                        meteringPreference.setValue(mContext
                                .getString(R.string.pref_camera_metering_entry_value_center_weighted));
                    }
                    /*@}*/
                    if (toastString != null) {
                        showMutexToast(toastString);
                    }
                }
                return;
            }

            //SPRD:fix bug534665 add some mutex about scene mode
            // METER - SCENE MODE
            if (key.equals(Keys.KEY_CAMER_METERING) && preference instanceof ListPreference) {
                String sceneMode = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                        Keys.KEY_SCENE_MODE);
                String meterMode = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                        Keys.KEY_CAMER_METERING);
                if ((sceneMode.equals("landscape") && !meterMode.equals("frameaverage"))
                        || (sceneMode.equals("portrait") && !meterMode.equals("centerweighted"))) {
                    ListPreference sceneModePreference = (ListPreference) findPreference(Keys.KEY_SCENE_MODE);
                    sceneModePreference.setValue(mContext
                            .getString(R.string.pref_camera_scenemode_default));
                }
                return;
            }

            //SPRD:fix bug534665 add some mutex about scene mode
            // ISO - SCENE MODE, HDR
            if (key.equals(Keys.KEY_CAMERA_ISO) && preference instanceof ListPreference) {
                ListPreference listPreference = (ListPreference) preference;

                if (!mContext.getString(R.string.pref_entry_value_auto)
                        .equals(listPreference.getValue())) {
                    ListPreference sceneModePreference = (ListPreference) findPreference(Keys.KEY_SCENE_MODE);
                    sceneModePreference.setValue(mContext
                            .getString(R.string.pref_camera_scenemode_default));
                    /* SPRD: Add for bug 549909, mutex ISO - HDR @{ */
                    // ISO - HDR
                    if (Keys.isHdrOn(settingsManager)) {
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_CAMERA_HDR, false);
                        toastString = mContext.getString(R.string.mutex_hdr);
                        mNeedCheckMutex = true;
                    }
                    if (toastString != null) {
                        showMutexToast(toastString);
                    }
                    /* @} */
                }
                return;
            }

            // WHITE_BALANCE - HDR, SCENE MODE
            if (key.equals(Keys.KEY_WHITE_BALANCE) && preference instanceof ListPreference) {
                ListPreference listPreference = (ListPreference) preference;
                if (!mContext.getString(R.string.pref_camera_whitebalance_default)
                        .equals(listPreference.getValue())) {
                    // WHITE_BALANCE - HDR
                    if (Keys.isHdrOn(settingsManager)) {
                        // settingsManager.set(mCameraScope,
                        // Keys.KEY_SCENE_MODE,listPreference.getValue());
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_CAMERA_HDR, false);
                        toastString = mContext.getString(R.string.mutex_hdr);
                        mNeedCheckMutex = true;
                    }
                    // WHITE_BALANCE - SCENE MODE
                    ListPreference sceneModePreference = (ListPreference) findPreference(Keys.KEY_SCENE_MODE);
                    sceneModePreference.setValue(mContext
                            .getString(R.string.pref_camera_scenemode_default));

                    if (toastString != null) {
                        showMutexToast(toastString);
                    }
                }
                return;
            }

            // CONTINUE_CAPTURE MUTEX WITH HDR, KEY_FREEZE_FRAME_DISPLAY, and FLASH MODE, SMILE, TIMESTAMPS, VGESTURE
            if (key.equals(Keys.KEY_CAMERA_CONTINUE_CAPTURE)
                    && preference instanceof ListPreference) {
                ListPreference listPreference = (ListPreference) preference;
                if (!mContext.getString(R.string.pref_camera_burst_entry_defaultvalue)
                        .equals(listPreference.getValue())) {

                    // CONTINUE_CAPTURE - HDR
                    String cameraScope = "_preferences_camera_0";
                    if (Keys.isHdrOn(settingsManager)) {
                        // settingsManager.set(mCameraScope,
                        // Keys.KEY_SCENE_MODE,listPreference.getValue());
                        //SPRD: Fix bug 572156
                        String PREF_BEFORE="PREF_BEFORE";
                        settingsManager.set(cameraScope, Keys.KEY_FLASH_MODE+ PREF_BEFORE, "off");
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_CAMERA_HDR, false);
                        toastString = mContext.getString(R.string.countine_mutex);
                        mNeedCheckMutex = true;
                    }

                    // CONTINUE_CAPTURE - KEY_FREEZE_FRAME_DISPLAY
                    ManagedSwitchPreference freezeFrameDisplayPreference =
                            (ManagedSwitchPreference) findPreference(Keys.KEY_FREEZE_FRAME_DISPLAY);
                    if (freezeFrameDisplayPreference != null) {
                        freezeFrameDisplayPreference.setChecked(false);
                    }

                    // CONTINUE_CAPTURE - FLASH MODE
                    //SPRD: Fix bug 542565
                    if (!"off".equals(settingsManager.getString(cameraScope, Keys.KEY_FLASH_MODE))) {
                        // flash auto or on
                        settingsManager.set(cameraScope, Keys.KEY_FLASH_MODE, "off");
                        toastString = mContext.getString(R.string.countine_mutex);
                    }

                    // CONTINUE_CAPTURE - SMILE
                    if ("smile".equals(settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                            Keys.KEY_CAMERA_AI_DATECT))) {
                        ListPreference aiPreference = (ListPreference) findPreference(Keys.KEY_CAMERA_AI_DATECT);
                        aiPreference.setValue(Keys.CAMERA_AI_DATECT_VAL_OFF);
                        toastString = mContext.getString(R.string.countine_mutex);
                    }

                    // CONTINUE_CAPTURE - TIMESTAMPS
                    if (settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_TIME_STAMP)) {
                        ManagedSwitchPreference tsPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_CAMERA_TIME_STAMP);
                        if (tsPreference != null) {
                            tsPreference.setChecked(false);
                            toastString = mContext.getString(R.string.countine_mutex);
                        }
                    }

                    // CONTINUE_CAPTURE - MIRROR - Bug 545447
                    if (settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL,
                            Keys.KEY_FRONT_CAMERA_MIRROR)) {
                        ManagedSwitchPreference mirrPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_FRONT_CAMERA_MIRROR);
                        if (mirrPreference != null) {
                            mirrPreference.setChecked(false);
                            toastString = mContext.getString(R.string.countine_mutex);
                        }
                    }

                    // CONTINUE_CAPTURE - VGESTURE
                    if(UCamUtill.isVgestureEnnable()){
                    if (Keys.isVGestureOn(settingsManager)) {
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_CAMERA_VGESTURE, false);
                        toastString = mContext.getString(R.string.countine_mutex);
                        }
                    }

                    // CONTINUE_CAPTURE - CAMERA_RECORD_VOICE
                    if (settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL,
                            Keys.KEY_CAMERA_RECORD_VOICE)) {
                        ManagedSwitchPreference cameraRecordVoicePreference =
                                (ManagedSwitchPreference) findPreference(Keys.KEY_CAMERA_RECORD_VOICE);
                        if (cameraRecordVoicePreference != null) {
                            cameraRecordVoicePreference.setChecked(false);
                            toastString = mContext.getString(R.string.countine_mutex);
                        }
                    }

                    // CONTINUE_CAPTURE - HIGHISO
                    if (Keys.isHighISOOn(settingsManager)) {
                        ManagedSwitchPreference highisoPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_HIGH_ISO);
                        if (highisoPreference != null) {
                            highisoPreference.setChecked(false);
                            toastString = mContext.getString(R.string.countine_mutex);
                        }
                    }

                    if (toastString != null) {
                        showMutexToast(toastString);
                    }
                }
                return;
            }

            // KEY_FREEZE_FRAME_DISPLAY - CONTINUE_CAPTURE
            if (key.equals(Keys.KEY_FREEZE_FRAME_DISPLAY)
                    && preference instanceof ManagedSwitchPreference) {
                ManagedSwitchPreference switchPreference = (ManagedSwitchPreference) preference;
                if (switchPreference.isChecked()) {
                    ListPreference burstPreference = (ListPreference) findPreference(Keys.KEY_CAMERA_CONTINUE_CAPTURE);
                    /* SPRD add for bug 533661 @{ */
                    if (burstPreference == null) {
                        Log.e(TAG, "burstPreference is null !");
                        return;
                    }
                    /* @} */
                    burstPreference.setValue(mContext
                            .getString(R.string.pref_camera_burst_entry_defaultvalue));

                    /* SPRD: Fix bug 535110, Photo voice record. @{ */
                    if (CameraUtil.isVoicePhotoEnable()) {
                        ManagedSwitchPreference cameraRecordVoicePreference = (ManagedSwitchPreference) findPreference(Keys.KEY_CAMERA_RECORD_VOICE);
                        cameraRecordVoicePreference.setChecked(false);
                    }
                    /* @} */
                }
                return;
            }

            //ZSL MUTEX COUNTDOWN FLASH BEGIN
            if (key.equals(Keys.KEY_CAMERA_ZSL_DISPLAY) && preference instanceof ManagedSwitchPreference) {
                ManagedSwitchPreference switchPreference = (ManagedSwitchPreference) preference;
                if(!switchPreference.isChecked()){
                    return;
                }
                //SPRD:fix bug545327 flash is not restored when zsl on and reset photomodule 
                if(1 == settingsManager.getInteger(SettingsManager.SCOPE_GLOBAL,Keys.KEY_CAMERA_ZSL_DISPLAY)){
                    int countDownDuration = settingsManager.getInteger(SettingsManager.SCOPE_GLOBAL,
                            Keys.KEY_COUNTDOWN_DURATION);
                    //SPRD: Fix bug 542565
                    String cameraScope = "_preferences_camera_0";
                    String flash = settingsManager.getString(cameraScope, Keys.KEY_FLASH_MODE);
                    //SPRD Bug:542367 HDR MUTEX WITH ZSL BEGIN
                    // SPRD: Fix bug 564279, remove mutex of zsl and flash
                    /*
                    if (countDownDuration > 0 || !"off".equals(flash)||Keys.isHdrOn(settingsManager)) {
                    */
                    if (countDownDuration > 0 || Keys.isHdrOn(settingsManager)) {
                        settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                                Keys.KEY_COUNTDOWN_DURATION);
                        /*
                        settingsManager.set(cameraScope, Keys.KEY_FLASH_MODE, "off");
                        */
                        settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                                Keys.KEY_CAMERA_HDR);
                        toastString = mContext.getString(R.string.zsl_mutex);
                    }
                    // SPRD: fix bug 569430 VGESTURE MUTEX WITH ZSL BEGIN
                    if (Keys.isVGestureOn(settingsManager)) {
                        settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_VGESTURE);
                        toastString = mContext.getString(R.string.zsl_mutex);
                    }

                    // SPRD Add ZSL-HIGHISO
                    if (Keys.isHighISOOn(settingsManager)) {
                        ManagedSwitchPreference highisoPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_HIGH_ISO);
                        if (highisoPreference != null) {
                            highisoPreference.setChecked(false);
                            toastString = mContext.getString(R.string.zsl_mutex);
                        }
                    }
                    if (toastString != null) {
                        showMutexToast(toastString);
                    }
                }
                return;
            }
            //ZSL MUTEX COUNTDOWN FLASH END

            //SMILE MUTEX COUNTDOWN , CONTINUE_CAPTURE , COLOR_EFFECT BEGIN
            if (key.equals(Keys.KEY_CAMERA_AI_DATECT) && preference instanceof ListPreference){
                String mface = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                        Keys.KEY_CAMERA_AI_DATECT);

                /* SPRD: Fix bug 577777, ai_detect with color effect. @{ */
                if (!"off".equals(mface)) {
                    ListPreference colorEffectPreference = (ListPreference) findPreference(Keys.KEY_CAMERA_COLOR_EFFECT);
                    if (colorEffectPreference != null) {
                        colorEffectPreference.setValue(mContext.getString(R.string.pref_camera_color_effect_entry_value_none));
                        toastString = mContext.getString(R.string.ai_detect_mutex);
                    }
                }
                /* @} */

                if ("smile".equals(mface)) {
                    // SMILE - COUNTDOWN
                    if (!"0".equals(settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                            Keys.KEY_COUNTDOWN_DURATION))) {
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,
                                Keys.KEY_COUNTDOWN_DURATION, "0");
                        toastString = mContext.getString(R.string.ai_detect_mutex);
                    }

                    //SMILE - VGESTURE
                    if (Keys.isVGestureOn(settingsManager)) {
                        settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_VGESTURE);
                        toastString = mContext.getString(R.string.ai_detect_mutex);
                    }

                    // SMILE - CAMERA_RECORD_VOICE
                    if (Keys.isCameraRecordVoiceOn(settingsManager)) {
                        ManagedSwitchPreference cameraRecordVoicePreference =
                                (ManagedSwitchPreference) findPreference(Keys.KEY_CAMERA_RECORD_VOICE);
                        cameraRecordVoicePreference.setChecked(false);
                        toastString = mContext.getString(R.string.ai_detect_mutex);
                    }

                    // SMILE - CONTINUE_CAPTURE
                    ListPreference burstPreference = (ListPreference) findPreference(Keys.KEY_CAMERA_CONTINUE_CAPTURE);
                    /* SPRD add for bug 533661 @{ */
                    if (burstPreference == null) {
                        Log.e(TAG, "burstPreference is null !");
                        return;
                    }
                    /* @} */
                    if (!mContext.getString(R.string.pref_camera_burst_entry_defaultvalue)
                            .equals(burstPreference.getValue())) {
                        burstPreference.setValue(mContext
                                .getString(R.string.pref_camera_burst_entry_defaultvalue));
                        toastString = mContext.getString(R.string.ai_detect_mutex);
                    }
                }
                if (toastString != null) {
                    showMutexToast(toastString);
                }
                return;
            }
            // SMILE MUTEX COUNTDOWN , CONTINUE_CAPTURE END

            // COLOR EFFECT - HDR , VGESTURE , AI_DETECT
            if (CameraUtil.isColorEffectEnabled()) {
                if (key.equals(Keys.KEY_CAMERA_COLOR_EFFECT) && preference instanceof ListPreference) {
                    ListPreference listPreference = (ListPreference) preference;
                    if (!mContext.getString(R.string.pref_camera_color_effect_entry_value_none)
                            .equals(listPreference.getValue())) {
                        // COLOR EFFECT - HDR
                        if (Keys.isHdrOn(settingsManager)) {
                            // settingsManager.set(mCameraScope,
                            // Keys.KEY_SCENE_MODE,listPreference.getValue());
                            settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_HDR, false);
                            toastString = mContext.getString(R.string.coloreffect_mutex);
                            mNeedCheckMutex = true;
                        }
                        // COLOR EFFECT - VGESTURE
                        if (Keys.isVGestureOn(settingsManager)) {
                            settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_VGESTURE, false);
                            toastString = mContext.getString(R.string.coloreffect_mutex);
                        }

                        /* SPRD: Fix bug 577777, ai_detect with color effect. @{ */
                        // COLOR EFFECT - AI DETECT
                        if (!"off".equals(settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                                Keys.KEY_CAMERA_AI_DATECT))) {
                            ListPreference aiPreference = (ListPreference) findPreference(Keys.KEY_CAMERA_AI_DATECT);
                            aiPreference.setValue(Keys.CAMERA_AI_DATECT_VAL_OFF);
                            toastString = mContext.getString(R.string.coloreffect_mutex);
                        }
                        /* @} */

                        if (toastString != null) {
                            showMutexToast(toastString);
                        }
                    }
                    return;
                }
            }

            // ISO - HDR
            if (key.equals(Keys.KEY_CAMERA_ISO) && preference instanceof ListPreference) {
                ListPreference listPreference = (ListPreference) preference;
                if (!mContext.getString(R.string.pref_entry_value_auto)
                        .equals(listPreference.getValue())) {
                    if (Keys.isHdrOn(settingsManager)) {
                        // settingsManager.set(mCameraScope,
                        // Keys.KEY_SCENE_MODE,listPreference.getValue());
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_CAMERA_HDR, false);
                        toastString = mContext.getString(R.string.mutex_hdr);
                        mNeedCheckMutex = true;
                    }
                    if (toastString != null) {
                        showMutexToast(toastString);
                    }
                }
                return;
            }

            // CONTRAST - HDR
            if (key.equals(Keys.KEY_CAMERA_CONTRAST) && preference instanceof ListPreference) {
                ListPreference listPreference = (ListPreference) preference;
                if (!mContext.getString(R.string.pref_contrast_entry_defaultvalue)
                        .equals(listPreference.getValue())) {
                    if (Keys.isHdrOn(settingsManager)) {
                        // settingsManager.set(mCameraScope,
                        // Keys.KEY_SCENE_MODE,listPreference.getValue());
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_CAMERA_HDR, false);
                        toastString = mContext.getString(R.string.mutex_hdr);
                        mNeedCheckMutex = true;
                    }
                    if (toastString != null) {
                        showMutexToast(toastString);
                    }
                }
                return;
            }

            // SATURATION - HDR
            if (key.equals(Keys.KEY_CAMERA_SATURATION) && preference instanceof ListPreference) {
                ListPreference listPreference = (ListPreference) preference;
                if (!mContext.getString(R.string.pref_saturation_entry_defaultvalue)
                        .equals(listPreference.getValue())) {
                    if (Keys.isHdrOn(settingsManager)) {
                        // settingsManager.set(mCameraScope,
                        // Keys.KEY_SCENE_MODE,listPreference.getValue());
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_CAMERA_HDR, false);
                        toastString = mContext.getString(R.string.mutex_hdr);
                        mNeedCheckMutex = true;
                    }
                    if (toastString != null) {
                        showMutexToast(toastString);
                    }
                }
                return;
            }

            // BRIGHTNESS - HDR
            if (key.equals(Keys.KEY_CAMERA_BRIGHTNESS) && preference instanceof ListPreference) {
                ListPreference listPreference = (ListPreference) preference;
                if (!mContext.getString(R.string.pref_brightness_entry_defaultvalue)
                        .equals(listPreference.getValue())) {
                    if (Keys.isHdrOn(settingsManager)) {
                        // settingsManager.set(mCameraScope,
                        // Keys.KEY_SCENE_MODE,listPreference.getValue());
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_CAMERA_HDR, false);
                        toastString = mContext.getString(R.string.mutex_hdr);
                        mNeedCheckMutex = true;
                    }
                    if (toastString != null) {
                        showMutexToast(toastString);
                    }
                }
                return;
            }

            // SLOW MOTION - TIME LAPSE and EOIS
            if (CameraUtil.isSlowMotionEnabled()) {
                if (key.equals(Keys.KEY_VIDEO_SLOW_MOTION)
                        && preference instanceof ListPreference) {
                    SettingsManager sm = new SettingsManager(mContext);
                    ListPreference listPreference = (ListPreference) preference;
                    if (!mContext.getString(R.string.pref_entry_value_one).equals(
                            listPreference.getValue())) {
                        ListPreference timeLapsePreference = (ListPreference) findPreference(Keys.KEY_VIDEO_TIME_LAPSE_FRAME_INTERVAL);
                        timeLapsePreference
                                .setValue(mContext.getString(R.string.pref_timelapse_entry_value_default));
                        /* Bug 474696 set the back camera video is default 720p when opened slow motion @{ */
                        if (entryValue_720p != null) {
                            ListPreference videoQualityBackPreference = (ListPreference) findPreference(Keys.KEY_VIDEO_QUALITY_BACK);
                            String videoQualityBackValue = videoQualityBackPreference.getValue();
                            if (!entryValue_720p.equals(videoQualityBackValue)) {
                                sm.set(SettingsManager.SCOPE_GLOBAL,
                                        Keys.KEY_VIDEO_QUALITY_BACK_LAST,
                                        videoQualityBackValue);
                            }
                            sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_QUALITY_BACK,
                                    entryValue_720p);
                        }/* @} */

                        /* Bug 474696 set the encode type is default H265 when opened slow motion @{ */
                        if (CameraUtil.isSupportH265()) {
                            ListPreference encodePreference =
                                    (ListPreference) findPreference(Keys.KEY_VIDEO_ENCODE_TYPE);
                            if (encodePreference != null && sm != null) {
                                String encodeValue = encodePreference.getValue();
                                String videoEncodeTypeValue = mContext
                                        .getString(R.string.pref_video_encode_type_entry_value_h265);
                                encodePreference
                                        .setEntries(R.array.pref_video_encode_type_entries_H265);
                                encodePreference
                                        .setEntryValues(R.array.pref_video_encode_type_entry_values_H265);

                                sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_ENCODE_TYPE,
                                        videoEncodeTypeValue);
                                sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_ENCODE_TYPE_LAST, // SPRD: Fix bug 580489
                                        encodeValue);
                                encodePreference.setValue(mContext
                                        .getString(R.string.pref_video_encode_type_entry_value_h265));
                                encodePreference
                                        .setSummary(R.string.pref_video_encode_type_entry_h265);
                            }
                        } /* @} */

                        ManagedSwitchPreference eoisBackDvPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_EOIS_DV_BACK);
                        ManagedSwitchPreference eoisFrontDvPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_EOIS_DV_FRONT);
                        boolean eoisBackDvOn = settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DV_BACK);
                        boolean eoisFrontDvOn = settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DV_FRONT);
                        if (eoisBackDvPreference != null && eoisBackDvOn) {
                            settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_EOIS_DV_BACK, false);
                            eoisBackDvPreference.setChecked(!eoisBackDvPreference.isChecked());
                        }
                        if (eoisFrontDvPreference != null && eoisFrontDvOn) {
                            settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_EOIS_DV_FRONT, false);
                            eoisFrontDvPreference.setChecked(!eoisFrontDvPreference.isChecked());
                        }
                    } else {
                        /*
                        ManagedSwitchPreference eoisBackDvPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_EOIS_DV_BACK);
                        ManagedSwitchPreference eoisFrontDvPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_EOIS_DV_FRONT);
                        boolean eoisBackDvOn = settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DV_BACK);
                        boolean eoisFrontDvOn = settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DV_FRONT);
                        if (eoisBackDvPreference != null && !eoisBackDvOn) {
                            settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_EOIS_DV_BACK, true);
                            eoisBackDvPreference.setChecked(!eoisBackDvPreference.isChecked());
                        }
                        if (eoisFrontDvPreference != null && !eoisFrontDvOn) {
                            settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_EOIS_DV_FRONT, true);
                            eoisFrontDvPreference.setChecked(!eoisFrontDvPreference.isChecked());
                        }
                        */
                        /* Bug 474696 set the encode type is default H265 when opened slow motion @{ */
                        if (CameraUtil.isSupportH265()) {
                            ListPreference encodePreference =
                                    (ListPreference) findPreference(Keys.KEY_VIDEO_ENCODE_TYPE);
                            if (encodePreference != null && sm != null) {
                                String encodeLastValue = sm.getString(SettingsManager.SCOPE_GLOBAL,
                                        Keys.KEY_VIDEO_ENCODE_TYPE_LAST);
                                if (encodeLastValue != null) {
                                    encodePreference
                                            .setEntries(R.array.pref_video_encode_type_entries_add_H256);
                                    encodePreference
                                            .setEntryValues(R.array.pref_video_encode_type_entry_values);
                                    sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_ENCODE_TYPE,
                                            encodeLastValue);
                                    encodePreference.setValue(encodeLastValue);
                                    encodePreference.setSummary(encodePreference.getEntry());
                                    sm.set(SettingsManager.SCOPE_GLOBAL,
                                            Keys.KEY_VIDEO_ENCODE_TYPE_LAST, null);
                                }
                            }
                        }
                        /* @} */
                    }
                }
            }

            // TIME LAPSE - SLOW MOTION and EOIS
            if (key.equals(Keys.KEY_VIDEO_TIME_LAPSE_FRAME_INTERVAL)
                    && preference instanceof ListPreference) {
                ListPreference listPreference = (ListPreference) preference;
                if (!mContext.getString(
                        R.string.pref_timelapse_entry_value_default).equals(
                        listPreference.getValue())) {
                    ListPreference slowMotionPreference = (ListPreference) findPreference(Keys.KEY_VIDEO_SLOW_MOTION);
                    if (slowMotionPreference != null) {
                        slowMotionPreference.setValue(mContext.getString(R.string.pref_entry_value_one));
                    }
                    ManagedSwitchPreference eoisBackDvPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_EOIS_DV_BACK);
                    ManagedSwitchPreference eoisFrontDvPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_EOIS_DV_FRONT);
                    boolean eoisBackDvOn = settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DV_BACK);
                    boolean eoisFrontDvOn = settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DV_FRONT);
                    if (eoisBackDvPreference != null && eoisBackDvOn) {
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_EOIS_DV_BACK, false);
                        eoisBackDvPreference.setChecked(!eoisBackDvPreference.isChecked());
                    }
                    if (eoisFrontDvPreference != null && eoisFrontDvOn) {
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_EOIS_DV_FRONT, false);
                        eoisFrontDvPreference.setChecked(!eoisFrontDvPreference.isChecked());
                    }
                /*
                } else {
                    ManagedSwitchPreference eoisBackDvPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_EOIS_DV_BACK);
                    ManagedSwitchPreference eoisFrontDvPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_EOIS_DV_FRONT);
                    boolean eoisBackDvOn = settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DV_BACK);
                    boolean eoisFrontDvOn = settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DV_FRONT);
                    if (eoisBackDvPreference != null && !eoisBackDvOn) {
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_EOIS_DV_BACK, true);
                        eoisBackDvPreference.setChecked(!eoisBackDvPreference.isChecked());
                    }
                    if (eoisFrontDvPreference != null && !eoisFrontDvOn) {
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_EOIS_DV_FRONT, true);
                        eoisFrontDvPreference.setChecked(!eoisFrontDvPreference.isChecked());
                    }
                */
                }
                return;
            }

            // TIMESTAMPS - CONTINUE_CAPTURE
            if (key.equals(Keys.KEY_CAMERA_TIME_STAMP) && preference instanceof ManagedSwitchPreference) {
                ManagedSwitchPreference switchPreference = (ManagedSwitchPreference) preference;
                if (switchPreference.isChecked()) {
                    ListPreference burstPreference = (ListPreference) findPreference(Keys.KEY_CAMERA_CONTINUE_CAPTURE);
                    if (burstPreference == null) return;

                    if (!mContext.getString(R.string.pref_camera_burst_entry_defaultvalue)
                            .equals(burstPreference.getValue())) {
                        burstPreference.setValue(mContext
                                .getString(R.string.pref_camera_burst_entry_defaultvalue));
                        toastString = mContext.getString(R.string.timestamps_mutex);
                    }
                    if (toastString != null) {
                        showMutexToast(toastString);
                    }
                }
                return;
            }
            // MIRROR - CONTINUE_CAPTURE - Bug 545447
            if (key.equals(Keys.KEY_FRONT_CAMERA_MIRROR)&& preference instanceof ManagedSwitchPreference) {
                ManagedSwitchPreference switchPreference = (ManagedSwitchPreference) preference;
                if (switchPreference.isChecked()) {
                    ListPreference burstPreference = (ListPreference) findPreference(Keys.KEY_CAMERA_CONTINUE_CAPTURE);
                    if (burstPreference == null)
                        return;

                    if (!mContext.getString(R.string.pref_camera_burst_entry_defaultvalue)
                            .equals(burstPreference.getValue())) {
                        burstPreference.setValue(mContext
                                .getString(R.string.pref_camera_burst_entry_defaultvalue));
                        toastString = mContext.getString(R.string.mirror_mutex);
                    }
                    if (toastString != null) {
                        showMutexToast(toastString);
                    }
                }
                return;
            }

            // EOIS_DV_BACK - SLOW MOTION and TIME LAPSE
            if (key.equals(Keys.KEY_EOIS_DV_BACK)
                    && preference instanceof ManagedSwitchPreference) {
                ManagedSwitchPreference switchPreference = (ManagedSwitchPreference) preference;
                if(!switchPreference.isChecked()){
                    return;
                }
                if (settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL,
                            Keys.KEY_EOIS_DV_BACK)) {
                    ListPreference timeLapsePreference = (ListPreference) findPreference(Keys.KEY_VIDEO_TIME_LAPSE_FRAME_INTERVAL);
                    if (timeLapsePreference != null) {
                        timeLapsePreference.setValue(mContext.getString(R.string.pref_timelapse_entry_value_default));
                    }
                    String timeLapseDefaultValue = mContext.getString(R.string.pref_timelapse_entry_value_default);
                    SettingsManager sm = new SettingsManager(mContext);
                    sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_TIME_LAPSE_FRAME_INTERVAL, timeLapseDefaultValue);

                    ListPreference slowMotionPreference = (ListPreference) findPreference(Keys.KEY_VIDEO_SLOW_MOTION);
                    if (slowMotionPreference != null) {
                        slowMotionPreference.setValue(mContext.getString(R.string.pref_entry_value_one));
                    }
                    String slowMotionDefaultValue = mContext.getString(R.string.pref_entry_value_one);
                    sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_SLOW_MOTION, slowMotionDefaultValue);

                    ListPreference videoEncoderTypePreference = (ListPreference) findPreference(Keys.KEY_VIDEO_ENCODE_TYPE);
                    if (videoEncoderTypePreference != null) {
                        videoEncoderTypePreference.setValue(mContext.getString(R.string.pref_video_encode_type_value_default));
                        videoEncoderTypePreference.setSummary(R.string.pref_video_encode_type_value_default);
                    }
                    String videoEncoderTypeDefaultValue = mContext.getString(R.string.pref_video_encode_type_value_default);
                    sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_ENCODE_TYPE, videoEncoderTypeDefaultValue);
                    if (entryValue_2160P != null) {
                        ListPreference videoQualityBackPreference = (ListPreference) findPreference(Keys.KEY_VIDEO_QUALITY_BACK);
                        String videoQualityBackValue = videoQualityBackPreference.getValue();
                        if (entryValue_2160P.equals(videoQualityBackValue)) {
                            sm.setToDefault(SettingsManager.SCOPE_GLOBAL,
                                    Keys.KEY_VIDEO_QUALITY_BACK);
                        }
                    }
                }
            }

            // EOIS_DV_FRONT - SLOW MOTION and TIME LAPSE
            if (key.equals(Keys.KEY_EOIS_DV_FRONT)
                    && preference instanceof ManagedSwitchPreference) {
                ManagedSwitchPreference switchPreference = (ManagedSwitchPreference) preference;
                if(!switchPreference.isChecked()){
                    return;
                }
                if (settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL,
                            Keys.KEY_EOIS_DV_FRONT)) {
                    ListPreference timeLapsePreference = (ListPreference) findPreference(Keys.KEY_VIDEO_TIME_LAPSE_FRAME_INTERVAL);
                    if (timeLapsePreference != null) {
                        timeLapsePreference.setValue(mContext.getString(R.string.pref_timelapse_entry_value_default));
                    }
                    String timeLapseDefaultValue = mContext.getString(R.string.pref_timelapse_entry_value_default);
                    SettingsManager sm = new SettingsManager(mContext);
                    sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_TIME_LAPSE_FRAME_INTERVAL, timeLapseDefaultValue);

                    ListPreference slowMotionPreference = (ListPreference) findPreference(Keys.KEY_VIDEO_SLOW_MOTION);
                    if (slowMotionPreference != null) {
                        slowMotionPreference.setValue(mContext.getString(R.string.pref_entry_value_one));
                    }
                    String slowMotionDefaultValue = mContext.getString(R.string.pref_entry_value_one);
                    sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_SLOW_MOTION, slowMotionDefaultValue);

                    ListPreference videoEncoderTypePreference = (ListPreference) findPreference(Keys.KEY_VIDEO_ENCODE_TYPE);
                    if (videoEncoderTypePreference != null) {
                        videoEncoderTypePreference.setValue(mContext.getString(R.string.pref_video_encode_type_value_default));
                    }
                    String videoEncoderTypeDefaultValue = mContext.getString(R.string.pref_video_encode_type_value_default);
                    sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_ENCODE_TYPE, videoEncoderTypeDefaultValue);
                }
            }

            if (key.equals(Keys.KEY_VIDEO_ENCODE_TYPE)
                    && preference instanceof ListPreference) {
                ListPreference listPreference = (ListPreference) preference;
                if (!mContext.getString(
                        R.string.pref_video_encode_type_value_default).equals(
                        listPreference.getValue())) {
                    ManagedSwitchPreference eoisBackDvPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_EOIS_DV_BACK);
                    ManagedSwitchPreference eoisFrontDvPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_EOIS_DV_FRONT);
                    boolean eoisBackDvOn = settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DV_BACK);
                    boolean eoisFrontDvOn = settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DV_FRONT);
                    if (eoisBackDvPreference != null && eoisBackDvOn) {
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_EOIS_DV_BACK, false);
                        eoisBackDvPreference.setChecked(!eoisBackDvPreference.isChecked());
                    }
                    if (eoisFrontDvPreference != null && eoisFrontDvOn) {
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_EOIS_DV_FRONT, false);
                        eoisFrontDvPreference.setChecked(!eoisFrontDvPreference.isChecked());
                    }
                }
            }

            // SPRD:Add for HighISO Mutex
            if (key.equals(Keys.KEY_HIGH_ISO)
                    && preference instanceof ManagedSwitchPreference) {
                ManagedSwitchPreference highisoPreference = (ManagedSwitchPreference) preference;
                String highisoToast = null;
                if (Keys.isHighISOOn(settingsManager)) {// SPRD:Fix for bug 570519
                    if (Keys.isHdrOn(settingsManager)) {
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_HDR,
                                false);
                        highisoToast = mContext.getString(R.string.highiso_mutex);
                    }
                    ListPreference burstPreference = (ListPreference) findPreference(Keys.KEY_CAMERA_CONTINUE_CAPTURE);
                    if (burstPreference != null) {
                        if (!mContext.getString(R.string.pref_camera_burst_entry_defaultvalue)
                                .equals(burstPreference.getValue())) {
                            burstPreference.setValue(mContext
                                    .getString(R.string.pref_camera_burst_entry_defaultvalue));
                            highisoToast = mContext.getString(R.string.highiso_mutex);
                        }
                    }
                    String cameraScope = "_preferences_camera_0";
                    String flash = settingsManager.getString(cameraScope,
                            Keys.KEY_FLASH_MODE);
                    if (!"off".equals(flash)) {
                        settingsManager.set(cameraScope, Keys.KEY_FLASH_MODE, "off");
                        highisoToast = mContext.getString(R.string.highiso_mutex);
                    }
                    if (Keys.isZslOn(settingsManager)) {
                        ManagedSwitchPreference zslPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_CAMERA_ZSL_DISPLAY);
                        if (zslPreference != null) {
                            zslPreference.setChecked(false);
                        }
                        highisoToast = mContext.getString(R.string.highiso_mutex);
                    }
                }

                if (highisoToast != null) {
                    showMutexToast(highisoToast);
                }
                return;
            }

            // CAMERA_RECORD_VOICE - CONTINUE_CAPTURE, FREEZE_FRAME_DISPLAY
            if (key.equals(Keys.KEY_CAMERA_RECORD_VOICE)
                    && preference instanceof ManagedSwitchPreference) { //Camera Record Voice Mutex with Burstcapture
                ManagedSwitchPreference switchPreference = (ManagedSwitchPreference) preference;
                if (switchPreference.isChecked()) {
                    ListPreference burstPreference = (ListPreference)findPreference(Keys.KEY_CAMERA_CONTINUE_CAPTURE);
                    if (burstPreference != null) {
                        burstPreference.setValue(mContext.getString(R.string.pref_camera_burst_entry_defaultvalue));
                    }

                    ManagedSwitchPreference freezePreference =
                            (ManagedSwitchPreference) findPreference(Keys.KEY_FREEZE_FRAME_DISPLAY);
                    if (freezePreference != null) {
                        freezePreference.setChecked(false);
                    }

                    if ("smile".equals(settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                            Keys.KEY_CAMERA_AI_DATECT))) {
                        ListPreference aiPreference = (ListPreference) findPreference(Keys.KEY_CAMERA_AI_DATECT);
                        aiPreference.setValue(Keys.CAMERA_AI_DATECT_VAL_OFF);
                    }
                }
                return;
            }

            if (key.equals(Keys.KEY_VIDEO_QUALITY_BACK)
                    && preference instanceof ListPreference) {
                ListPreference videoQualityBackPreference = (ListPreference) findPreference(Keys.KEY_VIDEO_QUALITY_BACK);
                String videoQualityBackValue = settingsManager.getString(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_QUALITY_BACK);
                SettingsManager sm = new SettingsManager(mContext);
                if (entryValue_2160P != null && entryValue_2160P.equals(videoQualityBackValue)) {
                    if (CameraUtil.isSupportH265()) {
                        ListPreference encodePreference =
                                (ListPreference) findPreference(Keys.KEY_VIDEO_ENCODE_TYPE);
                        String encodeValue = encodePreference.getValue();
                        sm.set(SettingsManager.SCOPE_GLOBAL,
                                Keys.KEY_VIDEO_ENCODE_TYPE_LAST,
                                encodeValue);
                        encodePreference
                                .setEntries(R.array.pref_video_encode_type_entries_H265);
                        encodePreference
                                .setEntryValues(R.array.pref_video_encode_type_entry_values_H265);
                        String videoEncodetTypeValue = mContext
                                .getString(R.string.pref_video_encode_type_entry_value_h265);
                        sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_ENCODE_TYPE,
                                videoEncodetTypeValue);
                        encodePreference.setValue(mContext
                                .getString(R.string.pref_video_encode_type_entry_value_h265));
                        encodePreference
                                .setSummary(R.string.pref_video_encode_type_entry_h265);
                    }

                    ManagedSwitchPreference eoisBackDvPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_EOIS_DV_BACK);
                    ManagedSwitchPreference eoisFrontDvPreference = (ManagedSwitchPreference) findPreference(Keys.KEY_EOIS_DV_FRONT);
                    boolean eoisBackDvOn = settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DV_BACK);
                    boolean eoisFrontDvOn = settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DV_FRONT);
                    if (eoisBackDvPreference != null && eoisBackDvOn) {
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_EOIS_DV_BACK, false);
                        eoisBackDvPreference.setChecked(!eoisBackDvPreference.isChecked());
                    }
                    if (eoisFrontDvPreference != null && eoisFrontDvOn) {
                        settingsManager.set(SettingsManager.SCOPE_GLOBAL,Keys.KEY_EOIS_DV_FRONT, false);
                        eoisFrontDvPreference.setChecked(!eoisFrontDvPreference.isChecked());
                    }
                }else {
                    if (CameraUtil.isSupportH265()) {
                        ListPreference encodePreference =
                                (ListPreference) findPreference(Keys.KEY_VIDEO_ENCODE_TYPE);
                        String encodeLastValue = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                                Keys.KEY_VIDEO_ENCODE_TYPE_LAST);
                        if (encodeLastValue != null) {
                            encodePreference
                                    .setEntries(R.array.pref_video_encode_type_entries_add_H256);
                            encodePreference
                                    .setEntryValues(R.array.pref_video_encode_type_entry_values);
                            sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_ENCODE_TYPE,
                                    encodeLastValue);
                            encodePreference.setValue(encodeLastValue);
                            encodePreference.setSummary(encodePreference.getEntry());
                            sm.set(SettingsManager.SCOPE_GLOBAL,
                                    Keys.KEY_VIDEO_ENCODE_TYPE_LAST, null);
                        }
                    }
                }
            }
        }

        private void showMutexToast(String toastString) {
            Toast.makeText(mContext, toastString, Toast.LENGTH_LONG).show();
        }
        /* @}*/

        /*
         * SPRD Bug:474694 Feature:Reset Settings. @{
         */
        public String mCameraScope;
        private int mCurrentModeIndex;

        public CameraSettingsFragment(String mCameraScope, int currentModeIndex, Context context) {
            this.mCameraScope = mCameraScope;
            this.mCurrentModeIndex = currentModeIndex;
            this.mContext = context;
        }

        public CameraSettingsFragment() {
            mPrefKey = null;
        }

        public void showAlertDialog(final boolean isCamera) {
            AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
            final AlertDialog alertDialog = builder.create();
            /* SPRD: Fix bug 560276 reset scenery @{ */
            builder.setTitle(mContext.getString(R.string.pref_restore_detail));
            builder.setMessage(mContext.getString(R.string.restore_message));
            /* @} */
            builder.setPositiveButton(mContext.getString(R.string.restore_done),
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            mResetCamera = true;
                            SettingsManager sm = new SettingsManager(mContext);
                            /* SPRD: Fix bug 560276 reset scenery @{ */
                            sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMER_RESET, true);
                            resetCameraSettings(sm);
                            sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_RESET, true);
                            resetVideoSettings(sm);
                            if (UCamUtill.isGifEnable()) {
                                sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_GIF_RESET, true);
                                resetGifSettings(sm);
                            }
                            resetScenerySettings(sm);
                            /* @} */
                            for (int i = 0; i < contexts.size(); i++) {
                                Context context = contexts.get(i);
                                ((CameraSettingsActivity) context).finish();
                            }
                            contexts.clear();
                           mResetCamera = false;
                        }
                    });
            builder.setNegativeButton(mContext.getString(R.string.restore_cancel),
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            alertDialog.dismiss();
                        }
                    });
            AlertDialog dialog = builder.create();
            dialog.show();
        }

        /* SPRD: Fix bug 560276 reset scenery @{ */
        private void resetScenerySettings(SettingsManager settingsManager) {
            settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_SCENERY_MODE_RESET, true);
        }
        /* @} */

        private void resetGifSettings(SettingsManager settingsManager) {
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_GIF_MODE_PIC_SIZE);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_GIF_MODE_NUM_SIZE);
            /* SPRD: fix bug540582 set default for flash @{ */
            if (mCameraScope != null) {
                settingsManager.setToDefault("_preferences_camera_0", Keys.KEY_GIF_FLASH_MODE);
                settingsManager.setToDefault("_preferences_camera_1", Keys.KEY_GIF_FLASH_MODE);
            }
            /* @} */
            // SPRD: Fix bug 560276 reset scenery
            settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_GIF_MODE_SWITCHER, true);
        }

        public void showGifAlertDialog() {
            AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
            final AlertDialog alertDialog = builder.create();
            builder.setTitle(mContext.getString(R.string.pref_gif_restore_detail));
            builder.setMessage(mContext.getString(R.string.gif_restore_message));
            builder.setPositiveButton(mContext.getString(R.string.restore_done),
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            SettingsManager sm = new SettingsManager(mContext);
                            sm.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_GIF_RESET, true);
                            resetGifSettings(sm);
                            for (int i = 0; i < contexts.size(); i++) {
                                Context context = contexts.get(i);
                                ((CameraSettingsActivity) context).finish();
                            }
                            contexts.clear();
                        }
                    });
            builder.setNegativeButton(mContext.getString(R.string.restore_cancel),
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            // TODO Auto-generated method stub 236
                            alertDialog.dismiss();
                        }
                    });
            AlertDialog dialog = builder.create();
            dialog.show();
        }

        private void resetCameraSettings(SettingsManager settingsManager) {
            // SPRD Bug:494930 Do not show Location Dialog when resetting settings.
            settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_RECORD_LOCATION,
                    Keys.RECORD_LOCATION_OFF);//SPRD: fix for bug 499642 delete location save function
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_PICTURE_SIZE_FRONT);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_PICTURE_SIZE_BACK);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_COUNTDOWN_DURATION);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMER_ANTIBANDING);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_SCENE_MODE);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_HDR);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_FOCUS_MODE);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_WHITE_BALANCE);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_JPEG_QUALITY);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_GRID_LINES);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_SHOULD_SHOW_REFOCUS_VIEWER_CLING);
            settingsManager
                    .setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_HDR_PLUS_FLASH_MODE);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_SHOULD_SHOW_SETTINGS_BUTTON_CLING);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_FREEZE_FRAME_DISPLAY);
            settingsManager
                    .setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_COLOR_EFFECT);
            settingsManager
                    .setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_CONTINUE_CAPTURE);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_ISO);
            Keys.setManualExposureCompensation(settingsManager, false);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_JPEG_QUALITY);

            // SPRD: Fix bug 572473 add for usb storage support
            updatePreferredStorage();

            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMER_METERING);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_CONTRAST);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_SATURATION);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_BRIGHTNESS);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_AI_DATECT);
            /* SPRD: Fix bug 567399 VGesture and AiDetect is mutex @{ */
            settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_AI_DATECT_LAST,
                    Keys.CAMERA_AI_DATECT_VAL_OFF);
            /* @} */
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_SHUTTER_SOUND);
            // SPRD Bug:513927 reset Makeup
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_BEAUTY_ENTERED);
            settingsManager.set(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_MAKEUP_MODE_LEVEL, mContext.getResources().getInteger(R.integer.ucam_makup_default_value));

            /* SPRD: Fix 474843 Add for Filter Feature @{ */
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_FILTER_ENTERED);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_FILTER_TYPE);
            /* @} */

            // SPRD bug:528308 reset time_stamp
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_TIME_STAMP);
            // SPRD bug:534257 reset EIS and OIS
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DC_BACK);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DC_FRONT);

            if (mCameraScope != null) {
                settingsManager.setToDefault("_preferences_camera_0", Keys.KEY_FLASH_MODE);
                /**
                 * SPRD: mutex - pay attention to specialMutexDefault() which said the reason.
                settingsManager.setToDefault("_preferences_camera_1", Keys.KEY_FLASH_MODE);
                */
                Keys.specialMutexDefault(settingsManager);
            }

            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_ZSL_DISPLAY);
            //SPRD:fix bug545312 reset mirror
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_FRONT_CAMERA_MIRROR);
            // SPRD: fix bug474860 reset quickcapture
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_QUICK_CAPTURE);
            /* SPRD: Fix bug 567394 when switch user the state of the quickcamera is wrong @{ */
            Settings.Global.putString(getActivity().getContentResolver()
                    , "camera_quick_capture_userid_" + ActivityManager.getCurrentUser()
                    , settingsManager.getStringDefault(Keys.KEY_QUICK_CAPTURE));
            /* @} */
            // settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
            // Keys.KEY_TOUCH_TO_CAPTURE);
             settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_VGESTURE);
             settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_HIGH_ISO);

             // SPRD: Fix bug 535110, Photo voice record.
             settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_RECORD_VOICE);
             /* SPRD:Bug 535058 New feature: volume */
             settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,Keys.KEY_CAMERA_VOLUME);
        }

        private void resetVideoSettings(SettingsManager settingsManager) {
            // SPRD Bug:494930 Do not show Location Dialog when resetting settings.
            settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_RECORD_LOCATION,
                    Keys.RECORD_LOCATION_OFF);//SPRD: fix for bug 499642 delete location save function
            settingsManager
                    .setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_QUALITY_FRONT);

            // SPRD: for bug 509708 add time lapse
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_VIDEO_TIME_LAPSE_FRAME_INTERVAL);

            // SPRD: for bug 532100 set default slow for DV
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_VIDEO_SLOW_MOTION);

            /* SPRD: Fix bug 566194 back quality and encode type can not be reset.
             * To solve bug, we move the following sets to the back of KEY_VIDEO_SLOW_MOTION,
             * no better way is found because of the fact that ListPreference.getValue() does
             * not always get the same value just set by SettingsManager @{ */
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_QUALITY_BACK);
            settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_QUALITY_BACK_LAST, null);

            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_ENCODE_TYPE);
            settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_VIDEO_ENCODE_TYPE_LAST, null);
            /* @} */

            if (mCameraScope != null) {
                settingsManager.setToDefault("_preferences_camera_0",
                        Keys.KEY_VIDEOCAMERA_FLASH_MODE);
                settingsManager.setToDefault("_preferences_camera_1",
                        Keys.KEY_VIDEOCAMERA_FLASH_MODE);
            }
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_GRID_LINES);

            // SPRD: Fix bug 572473 add for usb storage support
            updatePreferredStorage();

            /* SPRD Bug:495676 set default antibanding for DV */
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_VIDEO_ANTIBANDING);

            // settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
            // Keys.KEY_VIDEO_SLOW_MOTION);

            /* SPRD: Fix Bug 535139, set default color effect for DV */
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_VIDEO_COLOR_EFFECT);

            /* SPRD: Fix Bug 535139, set default white balance for DV */
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_VIDEO_WHITE_BALANCE);

            // SPRD bug:534257 reset EIS and OIS
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DV_BACK);
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_EOIS_DV_FRONT);
        }

        private void setEntriesForSelection(List<Size> selectedSizes,
                ListPreference preference, String key) {
            if (selectedSizes == null) {
                return;
            }

            SettingsManager sm = new SettingsManager(mContext);
            String mDefault = null;

            String[] entries = new String[selectedSizes.size()];
            String[] entryValues = new String[selectedSizes.size()];
            for (int i = 0; i < selectedSizes.size(); i++) {
                Size size = selectedSizes.get(i);
                entries[i] = getSizeSummaryString(size);
                entryValues[i] = SettingsUtil.sizeToSettingString(size);
                if (i == 0) {
                    mDefault = entryValues[0];
                }
            }
            preference.setEntries(entries);
            preference.setEntryValues(entryValues);
            sm.setDefaults(key, mDefault, entryValues);
        }
        /* @} */
        /*SPRD:fix bug537963 pull sd card when lock screen
         * @{
         */
        private void installIntentFilter() {
            // install an intent filter to receive SD card related events.
            IntentFilter intentFilter =
                    new IntentFilter(Intent.ACTION_MEDIA_EJECT);
            intentFilter.addAction(Intent.ACTION_MEDIA_SCANNER_STARTED);
            intentFilter.addDataScheme("file");
            mReceiver = new MyBroadcastReceiver();
            mContext.registerReceiver(mReceiver, intentFilter);
        }

        private BroadcastReceiver mReceiver = null;

        private class MyBroadcastReceiver extends BroadcastReceiver {
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                Log.d(TAG, "onReceive action="+action);
                if (action.equals(Intent.ACTION_MEDIA_EJECT)) {
                    ListPreference storagePreference = (ListPreference) findPreference(Keys.KEY_CAMERA_STORAGE_PATH);
                    if (storagePreference.getDialog() != null) {
                        storagePreference.getDialog().dismiss();
                    }
                    loadStorageDirectories();

                    SettingsManager settingsManager = new SettingsManager(mContext);
                    String currentStorage = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                                Keys.KEY_CAMERA_STORAGE_PATH);

                    /* SPRD: Fix bug 572473 add for usb storage support @{ */
                    if (!StorageUtil.getStoragePathState(currentStorage)) {
                        updatePreferredStorage();
                    }
                    /* @} */
                    setEntriesForSelectionStorage(mSupportedStorage, storagePreference);

                } else if (action.equals(Intent.ACTION_MEDIA_SCANNER_STARTED)) {
                    Log.d(TAG, "on media scanner: " + action);
                    ListPreference storagePreference = (ListPreference) findPreference(Keys.KEY_CAMERA_STORAGE_PATH);
                    if (storagePreference.getDialog() != null) {
                        storagePreference.getDialog().dismiss();
                    }
                    loadStorageDirectories();
                    setEntriesForSelectionStorage(mSupportedStorage, storagePreference);
                }
            }
        }

        /**
         * SPRD: add for feature bug 572473 OTG storage support
         *
         * update preferred storage from storage list
         */
        public void updatePreferredStorage() {
            ListPreference storagePreference = (ListPreference) findPreference(Keys.KEY_CAMERA_STORAGE_PATH);
            List<String> supportedStorages = MultiStorage.getSupportedStorage();
            SettingsManager settingsManager = new SettingsManager(mContext);
            if (supportedStorages.contains(StorageUtil.KEY_DEFAULT_EXTERNAL)) {
                storagePreference.setValue(mContext.getString(R.string.storage_path_external_default));
                settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_STORAGE_PATH,
                        MultiStorage.KEY_DEFAULT_EXTERNAL);
                // SPRD: Fix bug 577390, pictures captured from QuickCamera are always saved in built-in storage
                Settings.Global.putString(getActivity().getContentResolver()
                       , "camera_quick_capture_storage_path", MultiStorage.KEY_DEFAULT_EXTERNAL);

            } else if (supportedStorages.contains(StorageUtil.KEY_DEFAULT_INTERNAL)) {
                storagePreference.setValue(mContext
                        .getString(R.string.storage_path_internal_default));
                settingsManager.set(SettingsManager.SCOPE_GLOBAL,
                        Keys.KEY_CAMERA_STORAGE_PATH,
                        MultiStorage.KEY_DEFAULT_INTERNAL);
                // SPRD: Fix bug 577390, pictures captured from QuickCamera are always saved in built-in storage
                Settings.Global.putString(getActivity().getContentResolver()
                        , "camera_quick_capture_storage_path", MultiStorage.KEY_DEFAULT_INTERNAL);

            } else {
                if (supportedStorages.size() != 0) {
                    storagePreference.setValue(supportedStorages.get(0));
                    settingsManager.set(SettingsManager.SCOPE_GLOBAL,
                            Keys.KEY_CAMERA_STORAGE_PATH,
                            supportedStorages.get(0));
                    // TODO
                }
            }

        }
        /* @} */
    }

    /*
     * SPRD Bug:474694 Feature:Reset Settings. @{
     */
    public static final String CAMERA_SCOPE = "camera_scope";
    public String mCameraScope;
    private static boolean mResetCamera = false;
    /* @} */

    /*
     * SPRD: Fix bug 474843, New feature of Filter. @{
     */
    public static final String CURRENT_MODULE = "module_index";
    private int mCurrentModeIndex;
    /* @} */

    public static final String CAMERA_MODULE_SCOPENAME = "camera_module_scopename";
}
