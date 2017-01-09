package com.dream.camera.settings;

import android.content.Context;
import android.util.Log;
import android.widget.Toast;

import com.android.camera.settings.Keys;
import com.android.camera.settings.PictureSizeLoader.PictureSizes;
import com.android.camera2.R;
import com.android.ex.camera2.portability.CameraAgentFactory;
import com.android.ex.camera2.portability.CameraDeviceInfo;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;
import com.dream.camera.settings.DataConfig.CategoryType;
import com.dream.camera.settings.DataModuleBasic.DataStorageStruct;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.android.camera.util.CameraUtil;

import android.util.ArraySet;

import java.util.Set;

public class DataModulePhoto extends DataModuleInterfacePV {

    public static final String TAG = "DataModulePhoto";

    public DataModulePhoto(Context context) {
        super(context);
        mCategory = CategoryType.CATEGORY_PHOTO;
        mDefaultStorePosition = DataConfig.SettingStoragePosition.positionList[2];
    }

    @Override
    public void changeCurrentModule(DataStructSetting dataSetting) {
        super.changeCurrentModule(dataSetting);

        // Change the handle of sharepreference
        changSPB(dataSetting);
        initializeData(dataSetting);
    }

    @Override
    protected void setMutex(String key, Object newValue, Set<String> keyList) {
        String entryValue = getString(key);

        switch (key) {
        case Keys.KEY_CAMERA_AI_DATECT:
            setMutexAIDetect(key, entryValue, keyList);
            break;
        case Keys.KEY_CAMERA_COLOR_EFFECT:
            setMutexColorEffect(key, entryValue, keyList);
            break;
        case Keys.KEY_COUNTDOWN_DURATION:
            setMutexCountDown(key, entryValue, keyList);
            break;
        case Keys.KEY_FLASH_MODE:
            setMutexFlash(key, entryValue, keyList);
            break;
        case Keys.KEY_CAMERA_HDR:
            setMutexHDR(key, entryValue, keyList);
            break;
        case Keys.KEY_SCENE_MODE:
            setMutexSceneMode(key, entryValue, keyList);
            break;
        case Keys.KEY_CAMERA_VGESTURE:
            if (UCamUtill.isVgestureEnnable()) {
                setMutexVgesture(key, entryValue, keyList);
            }
            break;
        case Keys.KEY_FRONT_CAMERA_MIRROR:
            if (CameraUtil.isFrontCameraMirrorEnable()) {
                setMutexMirror(key, entryValue, keyList);
            }
        break;
        case Keys.KEY_CAMERA_BEAUTY_ENTERED:
            setMutexBeauty(key, entryValue, keyList);
            break;
        case Keys.KEY_CAMERA_ZSL_DISPLAY:
            setMutexZSL(key, entryValue, keyList);
            break;
        case Keys.KEY_HIGH_ISO:
            setMutexHighISO(key, entryValue, keyList);
            break;
        case Keys.KEY_CAMERA_FILTER_TYPE:
            setMutexFilterType(key, entryValue, keyList);
            break;
        case Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE:
            setMutexGifPhotoFlash(key, entryValue, keyList);
            break;
        /* SPRD:Fix bug 602341,set Mutex TimeStamp and FrameFreeze @{ */
        case Keys.KEY_CAMERA_TIME_STAMP:
            setMutexTimeStamp(key, entryValue, keyList);
            break;
        case Keys.KEY_FREEZE_FRAME_DISPLAY:
            setMutexFrameFreeze(key, entryValue, keyList);
            break;
        /* @} */
        default:
            break;
        }

    }

    private void setMutexGifPhotoFlash(String key, String entryValue,
            Set<String> keyList) {
        // GIF FLASH --- PHOTO FLASH SYNC
        if(getString(Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE).equals("torch")){
            continueSetMutex(Keys.KEY_FLASH_MODE, "on", keyList,
                    "gif flash sync with photo flash");
        }else{
            continueSetMutex(Keys.KEY_FLASH_MODE, "off", keyList,
                    "gif flash sync with photo flash");
        }

    }

    private void setMutexMirror(String key, String entryValue, Set<String> keyList) {
        if (CameraUtil.isFrontCameraMirrorEnable()
                && !"0".equals(getString(Keys.KEY_CAMERA_VGESTURE))) {
            continueSetMutex(
                    Keys.KEY_CAMERA_VGESTURE,
                    mContext.getString(R.string.preference_switch_item_default_value_false),
                    keyList, "mirror mutex with vgesture");
        }else{
            continueSetRestore(Keys.KEY_CAMERA_VGESTURE, keyList,
                    "mirror resotre vgesture");
        }
    }
    private String mOriginalAIValueString = null;
    private void setMutexFilterType(String key, String entryValue,
            Set<String> keyList) {
        int filterTypeValue = getInt(Keys.KEY_CAMERA_FILTER_TYPE);
        Log.i(TAG, "filter type value = " + filterTypeValue);
        if (101 == filterTypeValue ) {
            // Filter - AI
            mOriginalAIValueString = getString(Keys.KEY_CAMERA_AI_DATECT);
            if (!mContext.getString(R.string.pref_ai_detect_entry_value_off)
                    .equals(getString(Keys.KEY_CAMERA_AI_DATECT))) {
                continueSetMutex(
                        Keys.KEY_CAMERA_AI_DATECT,
                        mContext.getString(R.string.pref_ai_detect_entry_value_off),
                        keyList, "filter mutex with ai");
            }
            replaceAIDataStruct(R.array.pref_camera_ai_detect_off_key_array, keyList);

        } else {
            restoreAIDataStruct(keyList);
            continueSetRestore(Keys.KEY_CAMERA_AI_DATECT, keyList,
                    "filter restore ai");
        }
    }

    private void setMutexHighISO(String key, String entryValue,
            Set<String> keyList) {
        // SPRD:Add for highISO Mutex
        if (getBoolean(Keys.KEY_HIGH_ISO)) {
            // highISO --- zsl
            if (!"0".equals(getString(Keys.KEY_CAMERA_ZSL_DISPLAY))) {
                continueSetMutex(
                        Keys.KEY_CAMERA_ZSL_DISPLAY,
                        mContext.getString(R.string.preference_switch_item_default_value_false),
                        keyList, "highISO mutex with zsl");
            }

            // highISO - FLASH
            if (!"off".equals(getString(Keys.KEY_FLASH_MODE))) {
                continueSetMutex(Keys.KEY_FLASH_MODE, "off", keyList,
                        "highISO mutex with flash");
            }
        } else {
            continueSetRestore(Keys.KEY_CAMERA_ZSL_DISPLAY, keyList,
                    "highISO restore zsl");
            continueSetRestore(Keys.KEY_FLASH_MODE, keyList,
                    "highISO restore falsh");
        }
    }

    private void setMutexZSL(String key, String entryValue, Set<String> keyList) {
        if (getBoolean(Keys.KEY_CAMERA_ZSL_DISPLAY)) {

            // HDR MUTEX
            if (getBoolean(Keys.KEY_CAMERA_HDR)) {
                Log.e(TAG, "setMutexFlash   current hdr value = " + true
                        + "  need set mutex");
                continueSetMutex(Keys.KEY_CAMERA_HDR, false, keyList,
                        "zsl mutex with hdr");
            }

            // VGESTURE mutex
            if (UCamUtill.isVgestureEnnable()
                    && !"0".equals(getString(Keys.KEY_CAMERA_VGESTURE))) {
                continueSetMutex(
                        Keys.KEY_CAMERA_VGESTURE,
                        mContext.getString(R.string.preference_switch_item_default_value_false),
                        keyList, "zsl mutex with vgesture");
            }
            // SPRD Add ZSL-highISO
            if (getBoolean(Keys.KEY_HIGH_ISO)) {
                continueSetMutex(Keys.KEY_HIGH_ISO, false, keyList,
                        "zsl mutex with highISO");
            }
        } else {
            continueSetRestore(Keys.KEY_CAMERA_HDR, keyList, "zsl restore hdr");
            continueSetRestore(Keys.KEY_CAMERA_VGESTURE, keyList,
                    "zsl resotre vgesture");
            continueSetRestore(Keys.KEY_HIGH_ISO, keyList,
                    "zsl restore highISO");
        }
    }

    private void setMutexSceneMode(String key, String entryValue,
            Set<String> keyList) {

        if (!"auto".equals(getString(Keys.KEY_SCENE_MODE))) {
            // SCENE MODE --- MUTEX FLASH
            if (!"off".equals(getString(Keys.KEY_FLASH_MODE))) {
                continueSetMutex(Keys.KEY_FLASH_MODE, "off", keyList,
                        "scene mode mutex with flash");
            }
        } else {
            continueSetRestore(Keys.KEY_FLASH_MODE, keyList,
                    "scene mode restore flash");
        }
    }

    private void setMutexHDR(String key, String entryValue, Set<String> keyList) {
        // HDR --- MUTEX WITH FLASH & COLOR EFFECT
        if (!"0".equals(entryValue)) {
            // HDR - FLASH
            if (!"off".equals(getString(Keys.KEY_FLASH_MODE))) {
                continueSetMutex(Keys.KEY_FLASH_MODE, "off", keyList,
                        "hdr mutex with flash");
            }

            // HDR - COLOR EFFECT
            if (!mContext.getString(
                    R.string.pref_camera_color_effect_entry_value_none).equals(
                    getString(Keys.KEY_CAMERA_COLOR_EFFECT))) {
                continueSetMutex(
                        Keys.KEY_CAMERA_COLOR_EFFECT,
                        mContext.getString(R.string.pref_camera_color_effect_entry_value_none),
                        keyList, "hdr mutex with color effect");
            }

            // HDR - ZSL
            if (getBoolean(Keys.KEY_CAMERA_ZSL_DISPLAY)) {
                continueSetMutex(Keys.KEY_CAMERA_ZSL_DISPLAY, false, keyList,
                        "hdr mutex with zsl");
            }

        }
        // RESTORE
        else {
            continueSetRestore(Keys.KEY_FLASH_MODE, keyList,
                    "hdr restore falsh");
            continueSetRestore(Keys.KEY_CAMERA_COLOR_EFFECT, keyList,
                    "hdr restore color effect");
            continueSetRestore(Keys.KEY_CAMERA_ZSL_DISPLAY, keyList,
                    "hdr restore zsl");
        }

    }

    private void setMutexFlash(String key, String entryValue,
            Set<String> keyList) {
        // FLASH --- MUTEX WITH HDR
        if (!"off".equals(entryValue)) {
            // FLASH - HDR
            if (getBoolean(Keys.KEY_CAMERA_HDR)) {
                continueSetMutex(Keys.KEY_CAMERA_HDR, false, keyList,
                        "flash mutex with hdr");
            }

            // FLASH - SCENE MODE
            if (!"auto".equals(getString(Keys.KEY_SCENE_MODE))) {
                continueSetMutex(Keys.KEY_SCENE_MODE, "auto", keyList,
                        "flash mutex with scene mode");
            }

            // FLASH-highISO
            if (getBoolean(Keys.KEY_HIGH_ISO)) {
                continueSetMutex(Keys.KEY_HIGH_ISO, false, keyList,
                        "flash mutex with highISO");
            }
        }
        // RESTORE
        else {
            continueSetRestore(Keys.KEY_CAMERA_HDR, keyList,
                    "flash restore hdr");
            continueSetRestore(Keys.KEY_SCENE_MODE, keyList,
                    "flash restore scene mode");
            continueSetRestore(Keys.KEY_HIGH_ISO, keyList,
                    "flash restore highISO");
        }

        if(!entryValue.equals("on")){
            continueSetMutex(Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE, "off", keyList, "sync photo flash with gif flash");
        }else {
            continueSetMutex(Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE, "torch", keyList, "sync photo flash with gif flash");
        }
    }

    private void setMutexCountDown(String key, String entryValue,
            Set<String> keyList) {
        // COUNTDOWN --- MUTEX WITH SMILE
        if (!"0".equals(entryValue)) {
            // COUNTDOWN - VGESTURE
            if (UCamUtill.isVgestureEnnable()
                    && !"0".equals(getString(Keys.KEY_CAMERA_VGESTURE))) {
                continueSetMutex(
                        Keys.KEY_CAMERA_VGESTURE,
                        mContext.getString(R.string.preference_switch_item_default_value_false),
                        keyList, "countDown mutex with VGESTURE");
            }
        }
        // RESTORE
        else {
            if (UCamUtill.isVgestureEnnable()) {
                continueSetRestore(Keys.KEY_CAMERA_VGESTURE, keyList,
                        "countDown restore vgesture");
            }
        }
    }

    private void setMutexColorEffect(String key, String entryValue,
            Set<String> keyList) {
        // COLOR EFFECT --- MUTEX WITH HDR, AI
        if (!mContext.getString(
                R.string.pref_camera_color_effect_entry_value_none).equals(
                entryValue)) {
            // COLOR EFFECT - HDR
            if (getBoolean(Keys.KEY_CAMERA_HDR)) {
                continueSetMutex(Keys.KEY_CAMERA_HDR, false, keyList,
                        "color effect mutex with hdr");
            }
            // COLOR EFFECT - AI
            if ("face".equals(mContext.getString(R.string.pref_ai_detect_entry_value_face)) || "smile".equals(mContext.getString(R.string.pref_ai_detect_entry_value_smile))) {
                continueSetMutex(Keys.KEY_CAMERA_AI_DATECT,
                    mContext.getString(R.string.pref_ai_detect_entry_value_off),
                    keyList, "color effect mutex with ai");
            }
            if (UCamUtill.isVgestureEnnable()
                    && !"0".equals(getString(Keys.KEY_CAMERA_VGESTURE))) {
                continueSetMutex(
                        Keys.KEY_CAMERA_VGESTURE,
                        mContext.getString(R.string.preference_switch_item_default_value_false),
                        keyList, "color mutex with VGESTURE");
            }
        }
        // RESTORE
        else {
            continueSetRestore(Keys.KEY_CAMERA_HDR, keyList,
                    "color effect restore hdr");
            continueSetRestore(Keys.KEY_CAMERA_AI_DATECT, keyList,
                    "color effect restore AI Datect");
        }
    }

    private void setMutexAIDetect(String key, String entryValue,
            Set<String> keyList) {
        // SMILE --- MUTEX WITH COUNTDOWN
        if (mContext.getString(R.string.pref_ai_detect_entry_value_smile)
                .equals(entryValue)) {
        }
        // FACE --- MUTEX WITH color effect
            if (mContext.getString(R.string.pref_ai_detect_entry_value_face).equals(entryValue) || mContext.getString(R.string.pref_ai_detect_entry_value_smile).equals(entryValue)) {
                // FACE - COLOR EFFECT
                if (!mContext.getString(R.string.pref_camera_color_effect_entry_value_none).equals(
                        getString(Keys.KEY_CAMERA_COLOR_EFFECT))) {
                    continueSetMutex(Keys.KEY_CAMERA_COLOR_EFFECT,
                            mContext.getString(R.string.pref_camera_color_effect_entry_value_none),
                            keyList, "ai mutex with color effect");
                }
            }
        // RESTORE
        else {
            continueSetRestore(Keys.KEY_CAMERA_COLOR_EFFECT, keyList,
                    "AI Datect restore color effect");
        }
    }



    private void setMutexVgesture(String key, String entryValue,
            Set<String> keyList) {
        // Vgesture --- MUTEX WITH COUNTDOWN, AI DETECT, MAKEUP
        if (!"0".equals(entryValue)) {
            // Vgesture --- COUNTDOWN
            if (!"0".equals(getString(Keys.KEY_COUNTDOWN_DURATION))) {
                continueSetMutex(Keys.KEY_COUNTDOWN_DURATION, "0", keyList,
                        "vgesture mutex with count down");
            }

            // Vgesture --- MAKEUP
//            if (!"0".equals(getString(Keys.KEY_CAMERA_BEAUTY_ENTERED))) {
//                continueSetMutex(
//                        Keys.KEY_CAMERA_BEAUTY_ENTERED,
//                        mContext.getString(R.string.preference_switch_item_default_value_false),
//                        keyList, "vgesture mutex with makeup");
//            }

            // Vgesture --- zsl
            if (!"0".equals(getString(Keys.KEY_CAMERA_ZSL_DISPLAY))) {
                continueSetMutex(
                        Keys.KEY_CAMERA_ZSL_DISPLAY,
                        mContext.getString(R.string.preference_switch_item_default_value_false),
                        keyList, "vgesture mutex with zsl");
            }

         // Vgesture --- mirror
            if (!"0".equals(getString(Keys.KEY_FRONT_CAMERA_MIRROR))) {
                continueSetMutex(
                        Keys.KEY_FRONT_CAMERA_MIRROR,
                        mContext.getString(R.string.preference_switch_item_default_value_false),
                        keyList, "vgesture mutex with mirror");
            }

            // ui check 181
            // Vgesture - AI
            if (!mContext.getString(R.string.pref_ai_detect_entry_value_face)
                    .equals(getString(Keys.KEY_CAMERA_AI_DATECT))) {
                continueSetMutex(
                        Keys.KEY_CAMERA_AI_DATECT,
                        mContext.getString(R.string.pref_ai_detect_entry_value_face),
                        keyList, "vgesture mutex with ai");
            }

            // Vgesture - COLOR EFFECT
            if (!mContext.getString(
                    R.string.pref_camera_color_effect_entry_value_none).equals(
                    getString(Keys.KEY_CAMERA_COLOR_EFFECT))) {
                continueSetMutex(
                        Keys.KEY_CAMERA_COLOR_EFFECT,
                        mContext.getString(R.string.pref_camera_color_effect_entry_value_none),
                        keyList, "vgesture mutex with color effect");
            }

            replaceAIDataStruct(R.array.pref_camera_ai_detect_onlyface_key_array, keyList);

        }
        // RESTORE
        else {
            restoreAIDataStruct(keyList);
            continueSetRestore(Keys.KEY_COUNTDOWN_DURATION, keyList,
                    "vgesture restore countdown");
//            continueSetRestore(Keys.KEY_CAMERA_BEAUTY_ENTERED, keyList,
//                    "vgesture restore makeup");
            continueSetRestore(Keys.KEY_CAMERA_ZSL_DISPLAY, keyList,
                    "vgesture restore zsl");
            continueSetRestore(Keys.KEY_FRONT_CAMERA_MIRROR, keyList,
                    "vgesture restore mirror");
            continueSetRestore(Keys.KEY_CAMERA_AI_DATECT, keyList,
                    "vgesture restore ai");
            continueSetRestore(Keys.KEY_CAMERA_COLOR_EFFECT, keyList,
                    "vgesture restore color effect");

            if (!keyList.contains(Keys.KEY_CAMERA_AI_DATECT)) {
                keyList.add(Keys.KEY_CAMERA_AI_DATECT);
            }
        }
    }

    /* SPRD:Fix bug 602341,set Mutex TimeStamp and FrameFreeze @{ */
    private void setMutexTimeStamp(String key, String entryValue, Set<String> keyList) {
        if (getBoolean(Keys.KEY_CAMERA_TIME_STAMP)
                && getBoolean(Keys.KEY_FREEZE_FRAME_DISPLAY)) {
            continueSetMutex(
                    Keys.KEY_FREEZE_FRAME_DISPLAY,
                    mContext.getString(R.string.preference_switch_item_default_value_false),
                    keyList, "timeStamp mutex freezeframe");
        }else{
            continueSetRestore(Keys.KEY_FREEZE_FRAME_DISPLAY, keyList,
                    "timeStamp resotre freezeframe");
        }
    }

    private void setMutexFrameFreeze(String key, String entryValue, Set<String> keyList) {
        if (getBoolean(Keys.KEY_CAMERA_TIME_STAMP)
                && getBoolean(Keys.KEY_FREEZE_FRAME_DISPLAY)) {
            continueSetMutex(
                    Keys.KEY_CAMERA_TIME_STAMP,
                    mContext.getString(R.string.preference_switch_item_default_value_false),
                    keyList, "timeStamp mutex freezeframe");
        }else{
            continueSetRestore(Keys.KEY_CAMERA_TIME_STAMP, keyList,
                    "freezeframe resotre timeStamp");
        }
    }
    /* @} */

    private DataStorageStruct originalAIDetectDataStorageStruct = null;

    private void replaceAIDataStruct(int arrayResourceID, Set<String> keylist) {
        originalAIDetectDataStorageStruct = (DataStorageStruct) mSupportDataMap
                .get(Keys.KEY_CAMERA_AI_DATECT);
        if(mOriginalAIValueString != null){
            originalAIDetectDataStorageStruct.mRestorageValue = mOriginalAIValueString;
        }
        DataStorageStruct struct = generateSingleDataStorageStruct(arrayResourceID);
        mSupportDataMap.put(Keys.KEY_CAMERA_AI_DATECT, struct);
        keylist.add(Keys.KEY_CAMERA_AI_DATECT);
    }

    private void restoreAIDataStruct(Set<String> keylist) {
        if (originalAIDetectDataStorageStruct != null) {
            mSupportDataMap.put(Keys.KEY_CAMERA_AI_DATECT,
                    originalAIDetectDataStorageStruct);
            keylist.add(Keys.KEY_CAMERA_AI_DATECT);
        }
    }

    private void setMutexBeauty(String key, String entryValue,
            Set<String> keyList) {
        // MAKEUP --- MUTEX WITH Vgesture
//        if (!"0".equals(entryValue)) {
//            if (UCamUtill.isVgestureEnnable()
//                    && !"0".equals(getString(Keys.KEY_CAMERA_VGESTURE))) {
//                continueSetMutex(
//                        Keys.KEY_CAMERA_VGESTURE,
//                        mContext.getString(R.string.preference_switch_item_default_value_false),
//                        keyList, "makeup mutex with vgesture");
//            }
//        }
//        // RESTORE
//        else {
//            if (UCamUtill.isVgestureEnnable()) {
//                continueSetRestore(Keys.KEY_CAMERA_VGESTURE, keyList,
//                        "makeup restore vgesture");
//            }
//        }
    }

    @Override
    protected int showToast(String key, String oldValue, String newValue) {
        int toastResId = -1;
        /*
        switch (key) {
        case Keys.KEY_FLASH_MODE:
            if (!"off".equals(newValue) && "off".equals(oldValue)) {
                toastResId = R.string.flash_mutex;
            }
            break;
        case Keys.KEY_CAMERA_HDR:
            if (DreamSettingUtil.convertToBoolean(newValue)) {
                toastResId = R.string.hdr_mutex;
            }
            break;
        case Keys.KEY_CAMERA_COLOR_EFFECT:
            String mutexValue = mContext
                    .getString(R.string.pref_camera_color_effect_entry_value_none);
            if (!mutexValue.equals(newValue) && mutexValue.equals(oldValue)) {
                toastResId = R.string.color_effect_mutex;
            }
            break;
        case Keys.KEY_CAMERA_AI_DATECT:
            if (mContext.getString(R.string.pref_ai_detect_entry_value_smile)
                    .equals(newValue))
                toastResId = R.string.smile_mutex;
            break;
        case Keys.KEY_COUNTDOWN_DURATION:
            if (!"0".equals(newValue) && "0".equals(oldValue)) {
                toastResId = R.string.count_down_mutex;
            }
            break;
        case Keys.KEY_SCENE_MODE:
            if (!"auto".equals(newValue) && "auto".equals(oldValue)) {
                toastResId = R.string.scene_mutex;
            }
            break;
        case Keys.KEY_CAMERA_ZSL_DISPLAY:
            if (DreamSettingUtil.convertToBoolean(newValue)) {
                toastResId = R.string.zsl_mutex;
            }
            break;
        case Keys.KEY_HIGH_ISO:
            if (DreamSettingUtil.convertToBoolean(newValue)) {
                toastResId = R.string.highiso_mutex;
            }
            break;
        case Keys.KEY_CAMERA_VGESTURE:
            if (DreamSettingUtil.convertToBoolean(newValue)) {
                toastResId = R.string.vgesture_mutex;
            }
            break;
        case Keys.KEY_FRONT_CAMERA_MIRROR:
            if (DreamSettingUtil.convertToBoolean(newValue)) {
                toastResId = R.string.mirror_mutex;
            }
            break;
        case Keys.KEY_CAMERA_FILTER_TYPE:
            if ("101".equals(newValue) && !"101".equals(oldValue) && !"off".equals(mOriginalAIValueString)) {
                toastResId = R.string.face_filter_mutex;
            }
            break;
        }

        if (toastResId != -1) {
            Toast.makeText(mContext, toastResId, Toast.LENGTH_SHORT).show();
        }
        */
        return toastResId;
    }

    @Override
    protected void fillEntriesAndSummaries() {
        if (mPictureSizes != null) {
            // front picture size
            setEVSPicturesize(Keys.KEY_PICTURE_SIZE_FRONT,
                    CameraUtil.filterSize(
                            mPictureSizes.frontCameraSizes,
                            Keys.KEY_PICTURE_SIZE_FRONT,
                            mDataSetting.mMode, mContext));

            // back picture size
            setEVSPicturesize(Keys.KEY_PICTURE_SIZE_BACK,
                    CameraUtil.filterSize(
                            mPictureSizes.backCameraSizes,
                            Keys.KEY_PICTURE_SIZE_BACK,
                            mDataSetting.mMode, mContext));
        }

        /* SPRD:Fix bug 447953 @{ */
        setEVSAIDetect();

        /* change the current display item when vgesture is open */
        setEVSAIDectectAccordingVgesture();

        setAntibanding();

    }

    private void setEVSAIDectectAccordingVgesture() {
        if (isEnableSettingConfig(Keys.KEY_CAMERA_VGESTURE)) {
            if (!"0".equals(getString(Keys.KEY_CAMERA_VGESTURE))) {
                originalAIDetectDataStorageStruct = (DataStorageStruct) mSupportDataMap
                        .get(Keys.KEY_CAMERA_AI_DATECT);
                DataStorageStruct onlyFace = generateSingleDataStorageStruct(R.array.pref_camera_ai_detect_onlyface_key_array);
                mSupportDataMap.put(Keys.KEY_CAMERA_AI_DATECT, onlyFace);
                Log.e(TAG,
                        "setEVSAIDectectAccordingVgesture() "
                                + onlyFace.toString());
            }
        }
    }

    private void setEVSAIDetect() {
        if (isEnableSettingConfig(Keys.KEY_CAMERA_FILTER_TYPE) && 101 == getInt(Keys.KEY_CAMERA_FILTER_TYPE)) {
            DataStorageStruct struct = generateSingleDataStorageStruct(R.array.pref_camera_ai_detect_off_key_array);
            originalAIDetectDataStorageStruct = new DataStorageStruct();
            DataStorageStruct data = (DataStorageStruct) mSupportDataMap.get(Keys.KEY_CAMERA_AI_DATECT);
            Log.e(TAG,"zxt data restorevalue="+data.mRestorageValue);
            if (data == null) {
                return;
            }
            originalAIDetectDataStorageStruct.copy(data);
            data.mEntries = struct.mEntries;
            data.mEntryValues = struct.mEntryValues;
            data.mDefaultValue = struct.mDefaultValue;
            return;
        }

        if (mInfos2 == null) {
            Log.e(TAG, "setEVSAIDetect() mInfos2 == null");
        }
        if (mInfos2 != null && !mInfos2.getSmileEnable()) {
            Log.e(TAG, "setEVSAIDetect() mInfos2 != null && !mInfos2.getSmileEnable()");
        }
        if (mInfos2 != null && !mInfos2.getSmileEnable()) {
            DataStorageStruct data = (DataStorageStruct) mSupportDataMap
                    .get(Keys.KEY_CAMERA_AI_DATECT);

            if (data == null) {
                return;
            }

            data.mEntries = mContext.getResources().getTextArray(
                    R.array.pref_camera_ai_detect_entries_removesmile);
            data.mEntryValues = mContext.getResources().getTextArray(
                    R.array.pref_camera_ai_detect_entryvalues_removesmile);
            Log.e(TAG, "setEVSAIDetect() " + data.toString());
        }
    }

    private void setAntibanding(){
        if (mInfos2 == null) {
            Log.i(TAG, "setAntibanding() mInfos2 == null");
            return;
        }
        if (!isEnableSettingConfig(Keys.KEY_CAMER_ANTIBANDING)){
            return;
        }
        DataStorageStruct data = (DataStorageStruct) mSupportDataMap.get(Keys.KEY_CAMER_ANTIBANDING);
        if (data == null) {
            Log.i(TAG, "setAntibanding() data == null");
            return;
        }
        data.mEntryValues = mContext.getResources().getTextArray(
                R.array.pref_camera_antibanding_entryvalues);
        if (mInfos2.getAntibandAutoEnable()){
            data.mEntries = mContext.getResources().getTextArray(
                    R.array.pref_camera_antibanding_entries_addauto);
            data.mDefaultValue = data.mEntryValues[2].toString();
        }
    }

    /*SPRD:fix bug 622818 add for exposure to adapter auto @{*/
    private void setExposureCompensationValue(CameraProxy proxy) {
        if (proxy != null) {
            if (!isEnableSettingConfig(Keys.KEY_EXPOSURE)){
                return;
            }

            DataStorageStruct data = (DataStorageStruct) mSupportDataMap.get(Keys.KEY_EXPOSURE);
            if (data == null) {
                Log.i(TAG, "setExposureCompensationValue data == null");
                return;
            }

            int minExposureCompensation = proxy.getCapabilities().getMinExposureCompensation();
            int maxExposureCompensation = proxy.getCapabilities().getMaxExposureCompensation();
            float exposureCompensationStep = proxy.getCapabilities().getExposureCompensationStep();
            int step = (int)(1/exposureCompensationStep);
            minExposureCompensation -= step;

            String[] entryValues = new String[(mContext.getResources().getTextArray(
                    R.array.pref_camera_exposure_key_entryvalues)).length];
            for (int i = 0; i < entryValues.length && (minExposureCompensation += step) <= maxExposureCompensation; i++) {
                //Log.i(TAG, "minExposureCompensation = " + minExposureCompensation);
                entryValues[i] = Integer.toString(minExposureCompensation);
            }
            data.mEntryValues = entryValues;

        }
    }
    /* @} */

    @Override
    public void initializeStaticParams(CameraProxy proxy) {
        super.initializeStaticParams(proxy);
        if (loader != null) {
            Set<String> keyList = new ArraySet<String>();
            // picture size
            if (mDataSetting.mIsFront) {
                if (mPictureSizes.frontCameraSizes == null) {
                    mPictureSizes.frontCameraSizes = loader
                            .loadFrontPictureSize(mDataSetting.mCameraID, proxy);
                    // front picture size
                    setEVSPicturesize(Keys.KEY_PICTURE_SIZE_FRONT,
                            CameraUtil.filterSize(
                                    mPictureSizes.frontCameraSizes,
                                    Keys.KEY_PICTURE_SIZE_FRONT,
                                    mDataSetting.mMode, mContext));
                    keyList.add(Keys.KEY_PICTURE_SIZE_FRONT);
                }

            } else {
                if (mPictureSizes.backCameraSizes == null) {
                    mPictureSizes.backCameraSizes = loader.loadBackPictureSize(
                            mDataSetting.mCameraID, proxy);
                    // back picture size
                    setEVSPicturesize(Keys.KEY_PICTURE_SIZE_BACK,
                            CameraUtil.filterSize(
                                    mPictureSizes.backCameraSizes,
                                    Keys.KEY_PICTURE_SIZE_BACK,
                                    mDataSetting.mMode, mContext));
                    keyList.add(Keys.KEY_PICTURE_SIZE_BACK);
                }

            }

            if(getBoolean(Keys.KEY_VIDEO_BEAUTY_ENTERED)){
                changeAndNotify(Keys.KEY_VIDEO_BEAUTY_ENTERED, getString(Keys.KEY_VIDEO_BEAUTY_ENTERED) );
            }

            setExposureCompensationValue(proxy);

            if(keyList.size() > 0){
                notifyKeyChange(keyList);
            }
        }
    }
    @Override
    public void destroy(){
        Set<String> keyList = new ArraySet<String>();
        restoreAIDataStruct(keyList);
        continueSetRestore(Keys.KEY_CAMERA_AI_DATECT, keyList,
                "filter restore ai");
        originalAIDetectDataStorageStruct = null;
    }

    @Override
    public void clearRestoreData() {
        super.clearRestoreData();
        originalAIDetectDataStorageStruct = null;
    }
}
