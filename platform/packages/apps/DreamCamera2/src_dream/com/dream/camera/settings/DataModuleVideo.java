package com.dream.camera.settings;

import java.util.ArrayList;
import java.util.Set;

import android.content.Context;
import android.media.CamcorderProfile;
import android.util.ArraySet;
import android.util.Log;

import com.android.camera2.R;
import com.android.camera.settings.Keys;
import com.dream.camera.settings.DataConfig.CategoryType;
import com.android.camera.util.CameraUtil;
import com.android.camera.settings.CameraPictureSizesCacher;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;
import com.android.camera.settings.SettingsUtil.SelectedVideoQualities;
import com.dream.camera.settings.DataModuleBasic.DataStorageStruct;

public class DataModuleVideo extends DataModuleInterfacePV {
    private String mSlowMotion = null;

    public DataModuleVideo(Context context) {
        super(context);
        mCategory = CategoryType.CATEGORY_VIDEO;
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
    protected void setEVSVideoQualities(String key,
                                        SelectedVideoQualities selectedQualities) {
        Log.i(TAG, "gzl child setEVSVideoQualities/in");
        super.setEVSVideoQualities(key, selectedQualities);
        setVideoEncode(Keys.KEY_VIDEO_ENCODE_TYPE);
        Set<String> keyList = new ArraySet<String>();
        keyList.add(Keys.KEY_VIDEO_QUALITY_BACK);
        keyList.add(Keys.KEY_VIDEO_ENCODE_TYPE);
        notifyKeyChange(keyList);
    }
    @Override
    protected void fillEntriesAndSummaries() {
        if(mPictureSizes != null){
            if(mPictureSizes.videoQualitiesFront != null){
                // front picture size
                setEVSVideoQualities(Keys.KEY_VIDEO_QUALITY_FRONT,
                        mPictureSizes.videoQualitiesFront.orNull());
            }

            if(mPictureSizes.videoQualitiesBack != null){
                // back picture size
                setEVSVideoQualities(Keys.KEY_VIDEO_QUALITY_BACK,
                        mPictureSizes.videoQualitiesBack.orNull());
            }
        }

        // set video encode
        setVideoEncode(Keys.KEY_VIDEO_ENCODE_TYPE);

        // set slow motion
        setSlowMotion(Keys.KEY_VIDEO_SLOW_MOTION);

        setAntibanding();
    }

    private void setVideoEncode(String key) {
        DataStorageStruct data = (DataStorageStruct) mSupportDataMap.get(key);
        if (data == null) {
            return;
        }
        if (CameraUtil.isSupportH265()) {
            if ((null != (DataStorageStruct) mSupportDataMap
                    .get(Keys.KEY_VIDEO_SLOW_MOTION))
                    || (CamcorderProfile.hasProfile(0, CamcorderProfile.QUALITY_2160P)
                    && getString(Keys.KEY_VIDEO_QUALITY_BACK) != null
                    && getString(Keys.KEY_VIDEO_QUALITY_BACK).equals("large"))) {
                DataStorageStruct struct = generateSingleDataStorageStruct(R.array.pref_video_encode_type_array);
                //originalEncodeDataStorageStruct = new DataStorageStruct();
                //originalEncodeDataStorageStruct.copy(data);
                data.mEntries = mContext.getResources().getTextArray(
                        R.array.pref_video_encode_type_entries_H256);
                data.mEntryValues = mContext.getResources().getTextArray(
                        R.array.pref_video_encode_type_entry_values_H265);
                data.mDefaultValue = mContext
                        .getString(R.string.pref_video_encode_type_entry_value_h265);
            } else {
                data.mEntries = mContext.getResources().getTextArray(
                        R.array.pref_video_encode_type_entries_add_H256);
                data.mEntryValues = mContext.getResources().getTextArray(
                        R.array.pref_video_encode_type_entry_values);
                data.mDefaultValue = mContext
                        .getString(R.string.pref_video_encode_type_entry_value_h264);
            }
            Log.e(TAG, "setVideoEncode() " + data.toString());
        }
    }

    private void setSlowMotion(String key) {
        DataStorageStruct data = (DataStorageStruct) mSupportDataMap.get(key);
        if (data == null) {
            return;
        }
        if (mSlowMotion != null) {
            String[] slowMotionValues = mSlowMotion.split(",");
            int k = 0;
            if (slowMotionValues != null && slowMotionValues.length > 1
                    && "0".equals(slowMotionValues[0])
                    && "1".equals(slowMotionValues[1])) {
                k = 1;
            }

            if (slowMotionValues != null) {
                String[] slowMotionEntries = new String[slowMotionValues.length
                        - k - 1];
                String[] slowMotionEntryValues = new String[slowMotionValues.length
                        - k - 1];
                int i = 0;
                for (String entryValue : slowMotionValues) {
                    if ("0".equals(entryValue) || "1".equals(entryValue)) {
                        continue;
                    } else {
                        slowMotionEntries[i] = entryValue;
                        slowMotionEntryValues[i] = entryValue;
                    }
                    ++i;
                }

                data.mEntries = slowMotionEntries;
                data.mEntryValues = slowMotionEntryValues;
                data.mDefaultValue = data.mEntryValues[0].toString();
            }
        }
    }

    private void setAntibanding(){
        if (mInfos2 == null) {
            Log.i(TAG, "setAntibanding() mInfos2 == null");
            return;
        }
        if (!isEnableSettingConfig(Keys.KEY_VIDEO_ANTIBANDING)){
            return;
        }
        DataStorageStruct data = (DataStorageStruct) mSupportDataMap.get(Keys.KEY_VIDEO_ANTIBANDING);
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

    @Override
    protected void setMutex(String key, Object newValue, Set<String> keyList) {
        String entryValue = getString(key);
        switch (key) {
        case Keys.KEY_VIDEO_QUALITY_BACK:
            setMutexBackVideoQuality(key, entryValue, keyList);
            break;
        case Keys.KEY_EOIS_DV_BACK:
            setMutexEoisDvBack(key, entryValue, keyList);
            break;
        case Keys.KEY_VIDEO_BEAUTY_ENTERED:
            setMutexVideoBeauty(key, entryValue, keyList);
            break;
        default:
            break;
        }

    }

    private void setMutexVideoBeauty(String key, String entryValue,
            Set<String> keyList) {
        if (getBoolean(key)) {
            // mutex with back video quality
            if (isFourKSelected() && !keyList.contains(Keys.KEY_VIDEO_QUALITY_BACK)) {
                mOriginalVideoQualityBackValueString = getString(Keys.KEY_VIDEO_QUALITY_BACK);
                continueSetMutex(Keys.KEY_VIDEO_QUALITY_BACK, "medium", keyList,
                        "4k mutex with back beauty");
                replaceVideoQualityBackStruct(Keys.KEY_VIDEO_QUALITY_BACK, keyList);
            }
            // mutex with eois
            if (getBoolean(Keys.KEY_EOIS_DV_BACK)) {
                continueSetMutex(Keys.KEY_EOIS_DV_BACK, false, keyList,
                        "back eois mutex with video beauty");
            }
        } else {
            if (isFourKSelected() ) {
                restoreVideoQualityBackStruct(Keys.KEY_VIDEO_QUALITY_BACK, keyList);
                continueSetRestore(Keys.KEY_VIDEO_QUALITY_BACK, keyList,
                        "4k resotre back video beauty");
            }
            continueSetRestore(Keys.KEY_EOIS_DV_BACK, keyList,
                    "video beauty resotre back eois");
        }
    }

    /* SPRD:replace && restore 4k @{ */
    private DataStorageStruct originalVideoQualityBackDataStorageStruct = null;
    private String mOriginalVideoQualityBackValueString = null;

    private void replaceVideoQualityBackStruct(String key, Set<String> keylist) {

        DataStorageStruct data = (DataStorageStruct) mSupportDataMap.get(key);

        if (data == null) {
            return;
        }
        originalVideoQualityBackDataStorageStruct = new DataStorageStruct();
        originalVideoQualityBackDataStorageStruct.copy(data);

        if (mOriginalVideoQualityBackValueString != null) {
            originalVideoQualityBackDataStorageStruct.mRestorageValue = mOriginalVideoQualityBackValueString;
        }

        ArrayList<String> entries = new ArrayList<String>();
        ArrayList<String> entryValues = new ArrayList<String>();

        for (int i = 0; i < data.mEntries.length; i++) {
            if (i == 0) {
                continue;
            } else {
                entries.add(data.mEntries[i].toString());
            }
        }

        for (int i = 0; i < data.mEntryValues.length; i++) {
            if (i == 0) {
                continue;
            } else {
                entryValues.add(data.mEntryValues[i].toString());
            }
        }

        data.mDefaultValue = data.mRestorageValue = mContext
                .getString(R.string.pref_video_quality_medium);
        data.mEntries = entries.toArray(new String[0]);
        data.mEntryValues = entryValues.toArray(new String[0]);
        if (mOriginalVideoQualityBackValueString.equals(mContext
                .getString(R.string.pref_video_quality_large))) {
            set(Keys.KEY_VIDEO_QUALITY_BACK,
                    mContext.getString(R.string.pref_video_quality_medium));
        } else {
            set(Keys.KEY_VIDEO_QUALITY_BACK,
                    mOriginalVideoQualityBackValueString);
        }

        keylist.add(key);
    }

    private void restoreVideoQualityBackStruct(String key, Set<String> keylist) {
        if (originalVideoQualityBackDataStorageStruct != null) {
            mSupportDataMap.put(key,
                    originalVideoQualityBackDataStorageStruct);
            keylist.add(key);
        }
    }
    private boolean isFourKSelected() {
        return CamcorderProfile.hasProfile(0, CamcorderProfile.QUALITY_2160P);
    }
    /* @} */

    private void setMutexBackVideoQuality(String key, String entryValue,
            Set<String> keyList) {
        if (CamcorderProfile.hasProfile(0, CamcorderProfile.QUALITY_2160P)) {
            if (getString(Keys.KEY_VIDEO_QUALITY_BACK).equals("large")) {
                if (getBoolean(Keys.KEY_EOIS_DV_BACK)) {
                    continueSetMutex(Keys.KEY_EOIS_DV_BACK, false, keyList,
                            "4k mutex with back eis");
                }
                mOriginalEncodeDataStorageStructValueString = getString(Keys.KEY_VIDEO_ENCODE_TYPE);
                if (!"h265".equals(getString(Keys.KEY_VIDEO_ENCODE_TYPE))) {
                    continueSetMutex(
                            Keys.KEY_VIDEO_ENCODE_TYPE,
                            mContext.getString(R.string.pref_video_encode_type_entry_value_h265),
                            keyList, "4k mutex with h265");
                }
                replaceEncodeStruct(R.array.pref_video_encode_h265_type_array,
                        keyList);
                // mutex with video beauty
                if (getBoolean(Keys.KEY_VIDEO_BEAUTY_ENTERED)) {
                    continueSetMutex(Keys.KEY_VIDEO_BEAUTY_ENTERED, false,
                            keyList, "BackVideoQuality mutex with video beauty");
                }
            } else {
                continueSetRestore(Keys.KEY_EOIS_DV_BACK, keyList,
                        "4k resotre back eis");
                restoreEncodeStruct(keyList);
                continueSetRestore(Keys.KEY_VIDEO_ENCODE_TYPE, keyList,
                        "4k resotre h265");
                continueSetRestore(Keys.KEY_VIDEO_BEAUTY_ENTERED, keyList,
                        "BackVideoQuality resotre video beauty");
            }
        }
    }

    private DataStorageStruct originalEncodeDataStorageStruct = null;
    private String mOriginalEncodeDataStorageStructValueString = null;
    private void replaceEncodeStruct(int arrayResourceID, Set<String> keylist) {
        originalEncodeDataStorageStruct = (DataStorageStruct) mSupportDataMap
                .get(Keys.KEY_VIDEO_ENCODE_TYPE);
        if(mOriginalEncodeDataStorageStructValueString != null){
            originalEncodeDataStorageStruct.mRestorageValue = mOriginalEncodeDataStorageStructValueString;
        }
        DataStorageStruct struct = generateSingleDataStorageStruct(arrayResourceID);
        mSupportDataMap.put(Keys.KEY_VIDEO_ENCODE_TYPE, struct);
        keylist.add(Keys.KEY_VIDEO_ENCODE_TYPE);
    }

    private void restoreEncodeStruct(Set<String> keylist) {
        if (originalEncodeDataStorageStruct != null) {
            mSupportDataMap.put(Keys.KEY_VIDEO_ENCODE_TYPE,
                    originalEncodeDataStorageStruct);
            keylist.add(Keys.KEY_VIDEO_ENCODE_TYPE);
        }
    }

    private void setMutexEoisDvBack(String key, String entryValue,
            Set<String> keyList) {
        if (getBoolean(Keys.KEY_EOIS_DV_BACK)) {
            if (CamcorderProfile.hasProfile(0, CamcorderProfile.QUALITY_2160P)) {
                if (getString(Keys.KEY_VIDEO_QUALITY_BACK).equals("large")) {
                    continueSetMutex(Keys.KEY_VIDEO_QUALITY_BACK, "medium", keyList,
                            "back eis mutex with 4k");
                }
            }
            // mutex with video beauty
            if(getBoolean(Keys.KEY_VIDEO_BEAUTY_ENTERED)){
                continueSetMutex(Keys.KEY_VIDEO_BEAUTY_ENTERED, false, keyList,
                        "back eois mutex with video beauty");
            }
        } else {
            continueSetRestore(Keys.KEY_VIDEO_QUALITY_BACK, keyList,
                    "back eis resotre 4k");
            continueSetRestore(Keys.KEY_VIDEO_BEAUTY_ENTERED, keyList,
                    "back eois resotre video beauty");
        }
    }

    @Override
    public void initializeStaticParams(CameraProxy proxy) {
        super.initializeStaticParams(proxy);
        if (loader != null) {
            Set<String> keyList = new ArraySet<String>();
            // video qualities
            if (mDataSetting.mIsFront) {
                if (mPictureSizes.videoQualitiesFront == null) {
                    mPictureSizes.videoQualitiesFront = loader
                            .loadFrontVideoQualities(mDataSetting.mCameraID);
                    // front picture size
                    setEVSVideoQualities(Keys.KEY_VIDEO_QUALITY_FRONT,
                            mPictureSizes.videoQualitiesFront.orNull());
                    keyList.add(Keys.KEY_VIDEO_QUALITY_FRONT);
                }
            } else {
                if (mPictureSizes.videoQualitiesBack == null) {
                    mPictureSizes.videoQualitiesBack  = loader.loadBackVideoQualities(mDataSetting.mCameraID);
                    // back picture size
                    setEVSVideoQualities(Keys.KEY_VIDEO_QUALITY_BACK,
                            mPictureSizes.videoQualitiesBack.orNull());
                    keyList.add(Keys.KEY_VIDEO_QUALITY_BACK);
                }
            }

            if (mSlowMotion == null) {
                mSlowMotion = CameraPictureSizesCacher
                        .getCacheSlowMotionForCamera(mContext);
                if (null == mSlowMotion) {
                    mSlowMotion = CameraPictureSizesCacher
                            .getSlowMotionForCamera(mContext, proxy);
                }
                setSlowMotion(Keys.KEY_VIDEO_SLOW_MOTION);
                keyList.add(Keys.KEY_VIDEO_SLOW_MOTION);
            }

            if(getBoolean(Keys.KEY_VIDEO_BEAUTY_ENTERED)){
                changeAndNotify(Keys.KEY_VIDEO_BEAUTY_ENTERED, getString(Keys.KEY_VIDEO_BEAUTY_ENTERED) );
            }

            if (keyList.size() > 0) {
                notifyKeyChange(keyList);
            }
        }
    }

}
