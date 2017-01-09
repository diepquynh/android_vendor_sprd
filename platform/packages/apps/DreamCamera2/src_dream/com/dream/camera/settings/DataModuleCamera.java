package com.dream.camera.settings;

import android.content.Context;
import android.provider.Settings;
import android.util.Log;

import com.android.camera.settings.Keys;
import com.android.camera2.R;
import com.dream.camera.settings.DataConfig.CategoryType;
import com.dream.camera.settings.DataConfig.DataStoragePath;
import com.dream.camera.settings.DataConfig.SettingStoragePosition;
import com.sprd.camera.storagepath.MultiStorage;

import java.util.List;
import java.util.Set;

public class DataModuleCamera extends DataModuleBasic {
    public static final String TAG = "DataModuleCamera";
    // camera public setting
    protected static DataSPAndPath mCameraSPB;

    private Context mContext;

    public DataModuleCamera(Context context) {
        super(context);
        mContext = context;
        mCategory = CategoryType.CATEGORY_CAMERA;
        mDefaultStorePosition = DataConfig.SettingStoragePosition.positionList[1];
        createCameraSPB();
    }

    public void createCameraSPB() {
        if (mCameraSPB == null) {
            mCameraSPB = new DataSPAndPath(
                    SettingStoragePosition.POSITION_CAMERA_PUBLIC,
                    DataStoragePath.PRE_CAMERA_CATEGORY_PUBLIC_SETTING);
        }

    }

    @Override
    protected void setMutex(String key, Object newValue, Set<String> keyList) {
        // keyList.add(key);
        //
        // if(key.equals(Keys.KEY_RECORD_LOCATION)){
        // boolean value = (boolean) newValue;
        // set(Keys.KEY_CAMERA_SHUTTER_SOUND, !value);
        // keyList.add(Keys.KEY_CAMERA_SHUTTER_SOUND);
        // }
    }

    @Override
    public boolean isSet(int position, String key) {
        return mCameraSPB.isSet(key);
    }

    @Override
    public void set(int position, String key, String value) {
        mCameraSPB.set(key, value);
    }

    @Override
    public String getString(int position, String key, String defaultValue) {
        return mCameraSPB.getString(key, defaultValue);
    }

    @Override
    public void clear() {
        if (mCameraSPB != null) {
            mCameraSPB.clear();
        }

    }

    @Override
    public void destroy() {
        clear();
    }

    private DataStructSetting getSettingData() {
        DataStructSetting settingData = new DataStructSetting();
        settingData.mCategory = mCategory;
        return settingData;
    }

    @Override
    protected void fillEntriesAndSummaries() {
        // storage path
        setEVSStoragePath();

    }

    /**
     * SPRD: Add Storage Entries and EntryValues for storage setting list
     *
     */
    private void setEVSStoragePath() {
        DataStorageStruct data = (DataStorageStruct) mSupportDataMap.get(Keys.KEY_CAMERA_STORAGE_PATH);
        List<String> supportedStorage = MultiStorage.getSupportedStorage();

        if (data == null || supportedStorage == null) {
            Log.d(TAG, "setEVSStoragePath: data or supportedStorage is null");
            return;
        }

        String[] entries = new String[supportedStorage.size()];
        String[] entryValues = new String[supportedStorage.size()];

        for (int i = 0; i < supportedStorage.size(); i++) {
            String value = supportedStorage.get(i);
            entryValues[i] = value;
            entries[i] = getStorageSummeryString(value);
        }

        data.mEntries = entries;
        data.mEntryValues = entryValues;

        if (supportedStorage.contains(MultiStorage.KEY_DEFAULT_EXTERNAL)) {
            if (!isSet(Keys.KEY_CAMERA_STORAGE_PATH)) {
                data.mDefaultValue = MultiStorage.KEY_DEFAULT_EXTERNAL;
                set(Keys.KEY_CAMERA_STORAGE_PATH, MultiStorage.KEY_DEFAULT_EXTERNAL);
                Settings.Global.putString(mContext.getContentResolver(),
                        "camera_quick_capture_storage_path", MultiStorage.KEY_DEFAULT_EXTERNAL);
            } else {
                data.mDefaultValue = getString(Keys.KEY_CAMERA_STORAGE_PATH);
                set(Keys.KEY_CAMERA_STORAGE_PATH, data.mDefaultValue);
                Settings.Global.putString(mContext.getContentResolver(),
                        "camera_quick_capture_storage_path", data.mDefaultValue);
                Log.d(TAG, "external != null and KEY_CAMERA_STORAGE_PATH is set");
            }
        } else if (supportedStorage.contains(MultiStorage.KEY_DEFAULT_INTERNAL)) {
            if (!isSet(Keys.KEY_CAMERA_STORAGE_PATH)
                        || ((isSet(Keys.KEY_CAMERA_STORAGE_PATH)
                        && !MultiStorage.KEY_DEFAULT_INTERNAL.equals(getString(Keys.KEY_CAMERA_STORAGE_PATH, ""))))) {
                data.mDefaultValue = MultiStorage.KEY_DEFAULT_INTERNAL;
                set(Keys.KEY_CAMERA_STORAGE_PATH, getString(Keys.KEY_CAMERA_STORAGE_PATH));
                Settings.Global.putString(mContext.getContentResolver(),
                        "camera_quick_capture_storage_path", MultiStorage.KEY_DEFAULT_INTERNAL);
            } else {
                Log.d(TAG, "Internal != null and KEY_CAMERA_STORAGE_PATH is set");
            }
        } else {
            if (supportedStorage.size() != 0) {
                data.mDefaultValue = supportedStorage.get(0);
                set(Keys.KEY_CAMERA_STORAGE_PATH, supportedStorage.get(0));
                Settings.Global.putString(mContext.getContentResolver(),
                        "camera_quick_capture_storage_path", supportedStorage.get(0));
            }
        }
        Log.e(TAG, " setEVSStoragePath() " + data.toString());
    }

    /**
     * use the storage value such as "Internal","External" or "USB1" get the diffrent
     * strings dependence different locations or different usb devices
     * @param value the value of storage
     * @return strings of storage dependence different locations or different usb devices
     */
    private String getStorageSummeryString(String value) {
        String entry = null;
        if (MultiStorage.KEY_DEFAULT_INTERNAL.equals(value)) {
            entry = mContext.getResources().getString(
                    R.string.storage_path_internal);
        } else if (MultiStorage.KEY_DEFAULT_EXTERNAL.equals(value)) {
            entry = mContext.getString(R.string.storage_path_external);
        } else {
            // SPRD: Fix bug 572473 add for usb storage support
            entry = value;
        }
        return entry;
    }

    public void updateSummaries(Set<String> keyList) {
        if (keyList.contains(Keys.KEY_CAMERA_STORAGE_PATH)) {
            setEVSStoragePath();
        }
    }
}
