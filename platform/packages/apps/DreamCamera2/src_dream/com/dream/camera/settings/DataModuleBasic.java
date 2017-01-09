package com.dream.camera.settings;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.content.res.TypedArray;
import android.util.ArraySet;
import android.util.Log;

import com.android.camera.settings.Keys;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;
import com.dream.camera.settings.DataConfig.SettingStoragePosition;

public abstract class DataModuleBasic {

    public interface DreamSettingChangeListener {
        public void onDreamSettingChangeListener(HashMap<String, String> keys);
    }

    public interface DreamSettingResourceChangeListener {
        public void onDreamSettingResourceChange();
    }

    /**
     * provide operation to get/set key using sharedpreferences 1. file path
     * associated with sharedpreferences 2. real operation(set/get key values)
     * with sharedpreferences
     * 
     */
    public class DataSPAndPath {
        // path of sharedpreferences
        public String mPath = null;
        // store position (SettingStoragePosition)
        public int mPosition = SettingStoragePosition.POSITION_ERROR;
        public SharedPreferences mSharePreferences = null;
        private OnSharedPreferenceChangeListener mSPCL = null;

        public DataSPAndPath(int position, String path) {
            mPath = path;
            initSharePreferences();
            // mSPCL = new OnSharedPreferenceChangeListener() {
            //
            // @Override
            // public void onSharedPreferenceChanged(
            // SharedPreferences sharedPreferences, String key) {
            // onSettingChange(mPosition, key);
            // }
            //
            // };
            // mSharePreferences.registerOnSharedPreferenceChangeListener(mSPCL);
        }

        private void initSharePreferences() {
            synchronized (mLock) {
                if (mSharePreferences == null) {
                    mSharePreferences = DreamSettingUtil.openPreferences(
                            mContext, mPath);
                }
            }
        }

        public void set(String key, String value) {

            synchronized (mLock) {
                mSharePreferences.edit().putString(key, value).apply();
            }

        }

        public String getString(String key, String defaultValue) {
            synchronized (mLock) {
                return mSharePreferences.getString(key, defaultValue);
            }
        }

        public void clear() {
            synchronized (mLock) {
                if (mSharePreferences != null && mSPCL != null) {
                    mSharePreferences
                            .unregisterOnSharedPreferenceChangeListener(mSPCL);
                }
                mSharePreferences = null;
                mPath = null;
                mPosition = DataConfig.SettingStoragePosition.POSITION_ERROR;
            }
        }

        public boolean isSet(String key) {
            synchronized (mLock) {
                return mSharePreferences.contains(key);
            }
        }
    }

    /*
     * configuration data struct map xml configuration file item
     */
    public class DataStorageStruct {
        // preference key
        public String mKey;
        // the position to storage
        public int mStorePosition;
        // default value
        public String mDefaultValue;
        // value list which used by set to camera
        public CharSequence[] mEntryValues;
        // used to selected or show
        public CharSequence[] mEntries;
        // storage the restorage value
        public String mRestorageValue;

        public DataStorageStruct() {
        }

        public DataStorageStruct(TypedArray type) {
            mKey = type.getString(0);
            mStorePosition = DataConfig.SettingStoragePosition.positionList[type
                    .getInt(1, 0)];
            mDefaultValue = type.getString(2);
            mEntryValues = type.getTextArray(3);
            mEntries = type.getTextArray(4);

        }

        public void copy(DataStorageStruct dataStorageStruct){
            this.mKey = dataStorageStruct.mKey;
            this.mStorePosition = dataStorageStruct.mStorePosition;
            this.mDefaultValue = dataStorageStruct.mDefaultValue;
            this.mEntryValues = dataStorageStruct.mEntryValues;
            this.mEntries = dataStorageStruct.mEntries;
            this.mRestorageValue = dataStorageStruct.mRestorageValue;
        }

        public void setDefaults(String defaultValue) {
            mDefaultValue = defaultValue;
        }

        public void setDefaults(int defaultValue) {
            mDefaultValue = Integer.toString(defaultValue);
        }

        public void setDefaults(boolean defaultValue) {
            mDefaultValue = defaultValue ? "1" : "0";
        }

        public String getDefaultString() {
            return mDefaultValue;
        }

        public Integer getDefaultInteger() {
            return mDefaultValue == null ? 0 : Integer.parseInt(mDefaultValue);
        }

        public boolean getDefaultBoolean() {
            return mDefaultValue == null ? false : (Integer
                    .parseInt(mDefaultValue) != 0);
        }

        public int getIndexOfValue(String value) {

            int index = -1;

            if (value == null || value.length() == 0) {
                return index;
            }

            if (mEntryValues == null || mEntryValues.length == 0) {
                return index;
            }

            for (int i = 0; i < mEntryValues.length; i++) {
                if (value.equals(mEntryValues[i])) {
                    index = i;
                    break;
                }
            }

            return index;
        }

        public String getValueOfIndex(int index) {

            if (index < 0) {
                return null;
            }

            if (mEntryValues == null || mEntryValues.length <= index) {
                return null;
            }

            return mEntryValues[index].toString();

        }

        public int getIndexOfSummary(String value) {

            int index = -1;

            if (value == null || value.length() == 0) {
                return index;
            }

            if (mEntries == null || mEntries.length == 0) {
                return index;
            }

            for (int i = 0; i < mEntries.length; i++) {
                if (value.equals(mEntries[i])) {
                    index = i;
                    break;
                }
            }

            return index;
        }

        public String getSummaryOfIndex(int index) {

            if (index < 0) {
                return null;
            }

            if (mEntries == null || mEntries.length <= index) {
                return null;
            }

            return mEntries[index].toString();

        }

        public String getSummary(String value) {
            int index = getIndexOfValue(value);
            if (index != -1 && index < mEntries.length) {
                return mEntries[index].toString();
            }
            return null;
        }

        public boolean isContainValue(String value) {
            for (int i = 0; i < mEntryValues.length; i++) {
                if (mEntryValues[i].equals(value)) {
                    return true;
                }
            }
            return false;
        }

        @Override
        public String toString() {
            String s = "key = " + mKey + " storePosition = " + mStorePosition
                    + " mDefaultValue = " + mDefaultValue + " mEntryValues = ";
            for (CharSequence value : mEntryValues) {
                s += value + ",";
            }

            s += " mEntries = ";

            for (CharSequence entry : mEntries) {
                s += entry + ",";
            }

            return s;
        }
    }

    private static final String TAG = "DataModuleBasic";
    protected Context mContext;
    // current module category
    protected String mCategory;
    // current module default storeposition
    protected int mDefaultStorePosition;
    // current data setting
    DataStructSetting mDataSetting;

    // data list which current module support
    protected HashMap<String, Object> mSupportDataMap = new HashMap<String, Object>();
    // data list which current module support mutex
    protected HashMap<String, Object> mMutexDataMap = new HashMap<String, Object>();

    // control which setting should be visible to user
    protected List<String> mShowItemsSet = new ArrayList<String>();

    public DataModuleBasic(Context context) {
        mContext = context;
    }

    // protected void onSettingChange(int mPosition, String key) {
    // }

    public DataStructSetting getDataSetting() {
        return mDataSetting;
    }

    /**
     * initialize current module(camera/photo/video) data according datasetting
     * 
     * @param dataSetting
     */
    public void initializeData(DataStructSetting dataSetting) {
        mDataSetting = dataSetting;

        // generate support configuration data resourceID
        int supportdataResourceID = DreamSettingUtil
                .getSupportDataResourceID(dataSetting);

        // generate support data map
        if (supportdataResourceID != -1) {
            Log.d(TAG, "initializeData -- generateSupportDataList supportdataResourceID:" + supportdataResourceID);
            generateSupportDataList(supportdataResourceID);
        }

        // generate mutex data resourceID
        int mutexDataResourceID = DreamSettingUtil
                .getMutexDataResourceID(dataSetting);

        // generate mutex data map
        if (mutexDataResourceID != -1) {
            generateMutexDataList(mutexDataResourceID);
        }

        // initialize show item resourceID
        int showItemSetID = DreamSettingUtil
                .getPreferenceUIConfigureID(dataSetting);

        // generate show item data
        if (showItemSetID != -1) {
            generateShowItemList(showItemSetID);
        }
        // setEntryAndEntryValues for list
        fillEntriesAndSummaries();
    }

    private void generateSupportDataList(int resourceID) {
        synchronized (mLock) {
            Log.e(TAG, "======== support data list start ========== resourceID:" + resourceID);
             TypedArray types = mContext.getResources().obtainTypedArray(resourceID);
             mSupportDataMap.clear();
             if (types != null) {
                 for (int i = 0; i < types.length(); i++) {
                     TypedArray type = mContext.getResources().obtainTypedArray(
                             types.getResourceId(i, -1));

                     if (type != null) {
                         DataStorageStruct data = new DataStorageStruct(type);
                         mSupportDataMap.put(data.mKey, data);
                         data.mRestorageValue = getString(data.mKey);
                         Log.e(TAG, data.toString());
                     }
                 }
             }
             types.recycle();
             Log.e(TAG, "===== support data list end =====");
        }
    }

    private void generateMutexDataList(int otherDataResourceID) {
        synchronized (mLock) {
            Log.e(TAG, "===== mutex data list start =====");
            TypedArray types = mContext.getResources().obtainTypedArray(
                    otherDataResourceID);
                mMutexDataMap.clear();
                if (types != null) {
                    for (int i = 0; i < types.length(); i++) {
                        TypedArray type = mContext.getResources().obtainTypedArray(
                                types.getResourceId(i, -1));
                        if (type != null) {
                            DataStorageStruct data = new DataStorageStruct(type);
                            mMutexDataMap.put(data.mKey, data);
                            Log.e(TAG, data.toString());
                        }
                    }
                }
                types.recycle();

                Log.e(TAG, "===== mutex data list end =====");
        }
    }

    private void generateShowItemList(int showItemSetID) {
        synchronized (mLock) {
            mShowItemsSet = Arrays.asList(mContext.getResources().getStringArray(
                    showItemSetID));
            Log.e(TAG, "===== ui show item start =====");
            for (String value : mShowItemsSet) {
                Log.e(TAG, "  " + value);
            }
            Log.e(TAG, "===== ui show item end =====");
        }
   

    }

    /**
     * generage dataStorageStruct according resource id
     * 
     * @param resourceID
     * @return
     */
    protected DataStorageStruct generateSingleDataStorageStruct(int resourceID) {
        TypedArray type = mContext.getResources().obtainTypedArray(resourceID);

        if (type != null) {
            DataStorageStruct data = new DataStorageStruct(type);
            return data;
        }

        return null;
    }

    /**
     * add/modify the data(support data/ mutex data) dynamic, the child shoud
     * implement according self logic
     */
    protected abstract void fillEntriesAndSummaries();

    /**
     * if this key can be used under current module by user
     * 
     * @param key
     * @return
     */
    public boolean isEnableSettingConfig(String key) {
        synchronized (mLock) {
            if (mSupportDataMap != null) {
                return mSupportDataMap.containsKey(key)
                        && !DataConfig.FEATURE_SWITCHER.contains(key);
            } else {
                return false;
                }
        }
    }

    protected Set<String> mutexListSet = new ArraySet<String>();
    protected Set<String> restoreListSet = new ArraySet<String>();

    protected void changeAndNotify(String key, Object newValue){
        String oldValue = getString(key);
        putSetting(key, newValue);
        Log.e(TAG, "user change settings key = " + key + " old value = "
                + oldValue + " new value = " + newValue);

        Set<String> keyList = new ArraySet<String>();
        keyList.add(key);
        mutexListSet.add(key);

        // setMutex
        setMutex(key, newValue, keyList);
        int toastResId = -1;
        if (keyList.size() > 1) {
            toastResId = showToast(key, oldValue, getString(key));
        }

        if (toastResId != -1) {
            Log.e(TAG,
                    "toast string = "
                            + mContext.getResources().getString(toastResId));
        }

        mutexListSet.clear();
        restoreListSet.clear();

        updateSummaries(keyList);

        // update
        notifyKeyChange(keyList);
    }

    /**
     * User can use this to change the key/values and notify the listener the
     * key must be in supportdatamap
     * 
     * @param key
     * @param newValue
     */
    public void changeSettings(String key, Object newValue) {
        if (!isEnableSettingConfig(key)) {
            return;
        }

        if (newValue == null) {
            return;
        }

        changeAndNotify(key, newValue);
    }

    public void changeMutexSetting(String key, Object newValue){
        if (!mMutexDataMap.containsKey(key)) {
            return;
        }

        if (newValue == null) {
            return;
        }
        changeAndNotify(key,newValue);
    }

    public void changeSettingsByIndex(String key, int index) {

        Log.e(TAG, "user change Settings By Index index = " + index);

        String value = getValueByIndex(key, index);
        changeSettings(key, value);

    }

    private void putSetting(String key, Object newValue) {
        synchronized (mLock){
            DataStorageStruct data = (DataStorageStruct) mSupportDataMap.get(key);

            if (newValue instanceof Boolean) {
                set(key, (boolean) newValue);
            } else if (newValue instanceof Integer) {
                set(key, (int) newValue);
            } else if (newValue instanceof String) {
                set(key, (String) newValue);
            }
            if (data != null) {
                data.mRestorageValue = getString(key);
            }
        }
    }

    private String getValueByIndex(String key, int index) {
        synchronized (mLock) {
            String value = null;
            DataStorageStruct data = null;
            if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
                data = (DataStorageStruct) mSupportDataMap.get(key);
                if (data != null) {
                    value = data.getValueOfIndex(index);
                }
            }
            return value;
      }
    }

    public void updateSummaries(Set<String> keyList) {

    }

    protected int showToast(String key, String oldValue, String newValue) {
        return -1;
    }

    protected void notifyKeyChange(Set<String> keyList) {

        HashMap<String, String> updateParamaters = new HashMap<String, String>();

        String sAll = "";
        String sNotifyString = "";

        for (String key : keyList) {
            String value = getString(key);
            if (isEnableSettingConfig(key)) {
                updateParamaters.put(key, value);
                sNotifyString += " key = " + key + " value = " + value;
            }
            sAll += " key = " + key + " value = " + value;
        }

        Log.e(TAG, "all change :    " + sAll);
        Log.e(TAG, "notify user Key Change  :    " + sNotifyString);

        synchronized (mLock) {
            for (int i = 0; i < mSettingChangedListeners.size(); i++) {
                DreamSettingChangeListener listener = mSettingChangedListeners
                        .get(i);
                if (listener != null) {
                    listener.onDreamSettingChangeListener(updateParamaters);
                }
            }
        }
    }

    /**
     * the key which user changed, should change the mutex item value and add
     * key to list, return to caller
     * 
     * @param key
     * @param newValue
     * @param keyList
     */
    protected abstract void setMutex(String key, Object newValue,
            Set<String> keyList);

    protected void continueSetMutex(String key, Object newValue,
            Set<String> keyList, String description) {
        if (description != null) {
            Log.e(TAG, description);
        }

        if (mutexListSet.contains(key)) {
            Log.e(TAG, "continueSetMutex return because mutexListSet contains " + key);
            return;
        }

        String value = getString(key);

        if (value == null) {
            return;
        }

        if (restoreListSet.contains(key)) {
            restoreListSet.remove(key);
        }
        mutexListSet.add(key);
        if (newValue instanceof Boolean
                && value == (DreamSettingUtil.convert((Boolean) newValue))) {
            return;
        } else if (newValue instanceof Integer
                && value == (DreamSettingUtil.convert((int) newValue))) {
            return;
        } else if (newValue instanceof String
                && value.equals((String) newValue)) {
            return;
        }

        if (newValue instanceof Boolean) {
            set(key, (Boolean) newValue);
        } else if (newValue instanceof Integer) {
            set(key, (Integer) newValue);
        } else if (newValue instanceof String) {
            set(key, (String) newValue);
        }
        Log.e(TAG, "continueSetMutex key = " + key + " old value = " + value
                + " new value = " + newValue);
        keyList.add(key);
        setMutex(key, newValue, keyList);

    }

    protected void continueSetRestore(String key, Set<String> keyList,
            String description) {

        if (description != null) {
            Log.e(TAG, description);
        }

        if (mutexListSet.contains(key)) {
            Log.e(TAG, "continueSetRestore return because mutexListSet contains " + key);
            return;
        }

        restoreListSet.add(key);

        String value = getString(key);

        if (value == null || getRestoreValue(key) == null || value.equals(getRestoreValue(key))) {
            Log.e(TAG, "continueSetRestore return because already restorevalue key = " + key);
            return;
        }
        setToRestoreValue(key);
        keyList.add(key);
        Log.e(TAG, "continueSetRestore key = " + key + " old value = " + value
                + " new value = " + getString(key));
        setMutex(key, getString(key), keyList);

    }

    public void set(String key, String value) {
        synchronized (mLock) {
            if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mSupportDataMap
                        .get(key);
                set(data.mStorePosition, key, value);
//                Log.e(TAG, "set value because contains key = " + key + " value = " + value);
            } else if (mMutexDataMap != null && mMutexDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mMutexDataMap.get(key);
                set(data.mStorePosition, key, value);
//                Log.e(TAG, "set value because contains key = " + key + " value = " + value);
            } else {
                set(mDefaultStorePosition, key, value);
//                Log.e(TAG, "set value to default position key = " + key + " value = " + value);
            }
        }
    }

    public void set(String key, int value) {
        set(key, DreamSettingUtil.convert(value));
    }

    public void set(String key, boolean value) {
        set(key, DreamSettingUtil.convert(value));
    }

    public void setValueByIndex(String key, int index) {
        synchronized (mLock) {
            if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mSupportDataMap
                        .get(key);
                set(key, data.getValueOfIndex(index));
            } else if (mMutexDataMap != null && mMutexDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mMutexDataMap.get(key);
                set(key, data.getValueOfIndex(index));
            }
        }
    }

    private void setToRestoreValue(String key) {
        synchronized (mLock) {
            if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mSupportDataMap
                        .get(key);
                if (data.mRestorageValue == null) {
                    set(key, data.mDefaultValue);
                } else {
                    set(key, data.mRestorageValue);
                }
            }
        }
    }

    public String getRestoreValue(String key) {
        synchronized (mLock) {
            if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mSupportDataMap
                        .get(key);
                String value = data.mDefaultValue;
                if (data.mRestorageValue != null) {
                    value = data.mRestorageValue;
                }
                return value;
            }
            return null;
        }
    }

    public String getString(String key) {
        synchronized (mLock)  {
            // Log.e(TAG, "getString() key = " + key);
            String value = null;
            if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mSupportDataMap.get(key);
                value = getString(data.mStorePosition, key, data.getDefaultString());

                if (!data.isContainValue(value)) {
                    value = data.mDefaultValue;
                }

            } else if (mMutexDataMap != null && mMutexDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mMutexDataMap.get(key);
                value = getString(data.mStorePosition, key, data.mDefaultValue);

                if (!data.isContainValue(value)) {
                    value = data.mDefaultValue;
                }

            } else {
                value = getString(mDefaultStorePosition, key, null);
            }
            // Log.e(TAG, "getString() key = " + key + " value = " + value);
            return value;
         }
    }

    public String getString(String key, String dValue) {
        synchronized (mLock) {
            String value = dValue;
            if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mSupportDataMap
                        .get(key);
                value = getString(data.mStorePosition, key, dValue);
                if (!data.isContainValue(value)) {
                    value = dValue;
                }
            } else if (mMutexDataMap != null && mMutexDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mMutexDataMap.get(key);
                value = getString(data.mStorePosition, key, dValue);
                if (!data.isContainValue(value)) {
                    value = dValue;
                }
            } else {
                value = getString(mDefaultStorePosition, key, dValue);
            }
            if (value == null) {
                value = dValue;
            }
            return value;
        }
    }

    public int getInt(String key) {
        String value = getString(key);

        if (value != null) {
            return DreamSettingUtil.convertToInt(value);
        }
        return -1;
    }

    public int getInt(String key, int dValue) {
        String value = getString(key, DreamSettingUtil.convert(dValue));

        if (value != null) {
            return DreamSettingUtil.convertToInt(value);
        }
        return dValue;
    }

    public boolean getBoolean(String key) {
        String value = getString(key);

        if (value != null) {
            return DreamSettingUtil.convertToBoolean(value);
        }
        return false;
    }

    public boolean getBoolean(String key, Boolean dValue) {
        String value = getString(key, DreamSettingUtil.convert(dValue));

        if (value != null) {
            return DreamSettingUtil.convertToBoolean(value);
        }
        return dValue;
    }

    public String getStringDefault(String key) {
        synchronized (mLock) {
            String value = null;
            if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mSupportDataMap
                        .get(key);
                value = data.mDefaultValue;
            } else if (mMutexDataMap != null && mMutexDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mMutexDataMap.get(key);
                value = data.mDefaultValue;
            } else {
                value = null;
            }
            return value;
 }


    }

    public int getIntDefault(String key) {

        String value = getStringDefault(key);

        if (value != null) {
            return DreamSettingUtil.convertToInt(value);
        }
        return -1;

    }

    public boolean getBooleanDefault(String key) {

        String value = getStringDefault(key);

        if (value != null) {
            return DreamSettingUtil.convertToBoolean(value);
        }
        return false;

    }

    public int getIndexOfCurrentValue(String key) {
        synchronized (mLock) {
            int value = -1;
            String sValue = null;
            if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mSupportDataMap
                        .get(key);
                sValue = getString(data.mStorePosition, key,
                        data.getDefaultString());
                value = data.getIndexOfValue(sValue);
            } else if (mMutexDataMap != null && mMutexDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mMutexDataMap.get(key);
                sValue = getString(data.mStorePosition, key,
                        data.getDefaultString());
                value = data.getIndexOfValue(sValue);
            }
            return value;
        }
    }

    public int getIndexOfDefaultValue(String key) {
        synchronized (mLock)  {
            int value = -1;
            if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mSupportDataMap
                        .get(key);
                value = data.getIndexOfValue(data.mDefaultValue);
            } else if (mMutexDataMap != null && mMutexDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mMutexDataMap.get(key);
                value = data.getIndexOfValue(data.mDefaultValue);
            }
            return value;
}

    }

    public HashMap<String, String> getAllSettingsForUI() {
        synchronized (mLock) {
            HashMap<String, String> allParamaters = new HashMap<String, String>();
            Set<String> keySet = mSupportDataMap.keySet();
            for (String key : keySet) {
                if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
                    DataStorageStruct data = (DataStorageStruct) mSupportDataMap
                            .get(key);
                    String value = null;
                    if (mShowItemsSet.contains(key)) {
                        value = getString(data.mStorePosition, key,
                                data.getDefaultString());
                    } else {
                        value = data.getDefaultString();
                    }
                    allParamaters.put(key, value);
                }
            }
            return allParamaters;
        }
    }

    public boolean isSet(String key) {
        synchronized (mLock) {
            int position = mDefaultStorePosition;
            if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mSupportDataMap
                        .get(key);
                position = data.mStorePosition;
            } else if (mMutexDataMap != null && mMutexDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mSupportDataMap
                        .get(key);
                position = data.mStorePosition;
            }

            return isSet(position, key);
        }


    }

    public HashMap<String, Object> getSupportSettingsList() {
        synchronized (mLock) {
            Log.d(TAG, "getSupportSettingsList size = " + mSupportDataMap.size());
            return mSupportDataMap;
        }
    }

    public Set<String> getSupportSettingKeys() {
        synchronized (mLock) {
            return mSupportDataMap.keySet();
        }
    }

    public HashMap<String, String> getSupportSettingDefaultKV() {
        HashMap<String, String> defaultKVList = new HashMap<String, String>();

        Set<String> keySet = mSupportDataMap.keySet();
        for (String key : keySet) {

            if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
                DataStorageStruct data = (DataStorageStruct) mSupportDataMap
                        .get(key);
                if (data != null) {
                    defaultKVList.put(key, data.mDefaultValue);
                }
            }
        }
        return defaultKVList;
    }

    public List<String> getShowItemsSet() {
        synchronized (mLock) {
            return mShowItemsSet;
        }
    }

    public abstract boolean isSet(int position, String key);

    public abstract void set(int position, String key, String value);

    public abstract String getString(int position, String key,
            String defaultValue);

    // ***************************** listener *****************************

    protected ArrayList<DreamSettingChangeListener> mSettingChangedListeners = new ArrayList<DreamSettingChangeListener>();
    protected ArrayList<DreamSettingResourceChangeListener> mResourceChangeListeners = new ArrayList<DataModuleBasic.DreamSettingResourceChangeListener>();
    private Object mLock = new Object();

    /**
     * Remove all OnSharedPreferenceChangedListener's. This should be done in
     * onDestroy.
     */
    public void removeAllListeners() {
        synchronized (mLock) {
            if (mSettingChangedListeners != null
                    && mSettingChangedListeners.size() > 0) {
                mSettingChangedListeners.clear();
            }
            if (mResourceChangeListeners != null
                    || mResourceChangeListeners.size() > 0) {
                mResourceChangeListeners.clear();
            }
        }
    }

    /**
     * Remove a specific SettingsListener. This should be done in onPause if a
     * listener has been set.
     */
    public void removeListener(DreamSettingResourceChangeListener listener) {
        synchronized (mLock) {
            if (listener == null || mResourceChangeListeners == null) {
                throw new IllegalArgumentException();
            }

            if (!mResourceChangeListeners.contains(listener)) {
                return;
            }
            mResourceChangeListeners.remove(listener);
        }
    }

    public void removeListener(DreamSettingChangeListener listener) {
        synchronized (mLock) {
            if (listener == null || mSettingChangedListeners == null) {
                throw new IllegalArgumentException();
            }

            if (!mSettingChangedListeners.contains(listener)) {
                return;
            }
            mSettingChangedListeners.remove(listener);
        }
    }

    /**
     * Add an OnSettingChangedListener to the SettingsManager, which will
     * execute onSettingsChanged when any SharedPreference has been updated.
     */
    public void addListener(final DreamSettingChangeListener listener) {
        synchronized (mLock) {
            if (listener == null || mSettingChangedListeners == null) {
                throw new IllegalArgumentException(
                        "OnSettingChangedListener cannot be null.");
            }

            if (mSettingChangedListeners.contains(listener)) {
                return;
            }
            mSettingChangedListeners.add(listener);
        }
    }

    public void addListener(final DreamSettingResourceChangeListener listener) {
        synchronized (mLock) {
            if (listener == null || mResourceChangeListeners == null) {
                throw new IllegalArgumentException(
                        "OnSettingChangedListener cannot be null.");
            }

            if (mResourceChangeListeners.contains(listener)) {
                return;
            }
            mResourceChangeListeners.add(listener);
        }
    }

    public abstract void clear();

    public abstract void destroy();

    public void clearRestoreData() {
//        Set<String> keySet = mSupportDataMap.keySet();
//        for (String key : keySet) {
//            if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
//                DataStorageStruct data = (DataStorageStruct) mSupportDataMap
//                        .get(key);
//                if (data != null) {
//                    data.mRestorageValue = null;
//                }
//            }
//        }
        initializeData(mDataSetting);
    }

    public void resetResource() {
        synchronized (mLock) {
            HashMap<String, Object> tempDataMap = new HashMap<String, Object>();
            tempDataMap.putAll(mSupportDataMap);

            initializeData(mDataSetting);
            Log.e(TAG, "========== resetResource() start =======");
            Set<String> keySet = mSupportDataMap.keySet();
            for (String key : keySet) {
                if (mSupportDataMap != null && mSupportDataMap.containsKey(key)) {
                    DataStorageStruct data = (DataStorageStruct) mSupportDataMap
                            .get(key);
                    DataStorageStruct tempData = (DataStorageStruct) tempDataMap
                            .get(key);
                    data.mRestorageValue = tempData.mRestorageValue;
                    Log.e(TAG, data.toString());
                }
            }
            Log.e(TAG, "========== resetResource() end =======");
            for (int i = 0; i < mResourceChangeListeners.size(); i++) {
                DreamSettingResourceChangeListener listener = mResourceChangeListeners.get(i);
                if (listener != null) {
                    listener.onDreamSettingResourceChange();
                }
            }
        }
    }

    public void initializeStaticParams(CameraProxy proxy){}

}
