package com.dream.camera.settings;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Set;

import android.content.Context;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceCategory;
import android.preference.PreferenceGroup;
import android.util.AttributeSet;
import android.util.Log;

import com.dream.camera.settings.DataModuleBasic.DataStorageStruct;
import com.dream.camera.settings.DataModuleBasic.DreamSettingChangeListener;

public abstract class DreamUISettingPartBasic extends PreferenceCategory
        implements OnPreferenceChangeListener, DreamSettingChangeListener {

    public interface OnCameraSettingChangeListener {
        public void onCameraSettingChange(Set<String> paramaterList);
    }

    private static final String TAG = "DreamUISettingPartBasic";

    protected DataModuleBasic mDataModule;
    protected int mResourceID = -1;
    protected OnCameraSettingChangeListener mUpdateCameraSetting;

    public DreamUISettingPartBasic(Context context) {
        super(context);

    }

    public DreamUISettingPartBasic(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public DreamUISettingPartBasic(Context context, AttributeSet attrs,
            int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public DreamUISettingPartBasic(Context context, AttributeSet attrs,
            int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    /*SPRD: fix bug 606536 not add ui change listener when back from secure camera @*/
    public void addListener() {
        mDataModule.addListener(this);
    }
    /* @ */

    public void changContent() {

        //mDataModule.addListener(this);

        updatePreItemAccordingConfig(this);

        // update UI display according properties config
        updatePreItemsAccordingProperties();

        // setEntryAndEntryValues for list
        fillEntriesAndSummaries(this);

        // set data for each preference
        initializeData(this);

    }

    private void updatePreItemAccordingConfig(PreferenceGroup group) {
        ArrayList<String> keyList = getAllPreKeyList(group);
        keyList.removeAll(mDataModule.getShowItemsSet());

        for (int i = 0; i < keyList.size(); i++) {
            Log.e(TAG, "remove key = " + keyList.get(i));
            Preference pref = group.findPreference(keyList.get(i));
            if (pref != null) {
                group.removePreference(pref);
            }
        }
    }

    /**
     * two operations should be done here. one is updatePreList.put(key, false);
     * another one is setChecked for switchPreference, setValue/setValueIndex
     * for listPreference;
     * 
     * @param pre
     */
    protected abstract void updatePreItemsAccordingProperties();

    /**
     * Recursively go through settings and fill entries and summaries of our
     * preferences.
     */
    private void fillEntriesAndSummaries(PreferenceGroup group) {
        for (int i = 0; i < group.getPreferenceCount(); ++i) {
            Preference pref = group.getPreference(i);
            if (pref == null) {
                continue;
            }

            if (pref instanceof PreferenceGroup) {
                fillEntriesAndSummaries((PreferenceGroup) pref);
            }

            if (pref instanceof ListPreference) {
                DreamUIPreferenceItemList pl = (DreamUIPreferenceItemList) pref;
                DataStorageStruct data = (DataStorageStruct) mDataModule
                        .getSupportSettingsList().get(pref.getKey());

                if (data != null) {
                    pl.setEntries(data.mEntries);
                    pl.setEntryValues(data.mEntryValues);
                }

            }

        }
    }

    private void initializeData(PreferenceGroup group) {

        for (int i = 0; i < group.getPreferenceCount(); i++) {
            DreamUIPreferenceItemInterface pref = (DreamUIPreferenceItemInterface) group
                    .getPreference(i);

            ((Preference) pref).setOnPreferenceChangeListener(this);
            pref.initializeData(this);

        }

    }

    private ArrayList<String> getAllPreKeyList(PreferenceGroup group) {

        ArrayList<String> keyList = new ArrayList<String>();

        for (int i = 0; i < group.getPreferenceCount(); i++) {
            Preference pref = group.getPreference(i);
            keyList.add(pref.getKey());

            if (pref instanceof PreferenceGroup) {
                keyList.addAll(getAllPreKeyList((PreferenceGroup) pref));
            }

        }

        return keyList;
    }

    /**
     * Recursively traverses the tree from the given group as the route and
     * tries to delete the preference. Traversal stops once the preference was
     * found and removed.
     */
    protected boolean recursiveDelete(PreferenceGroup group,
            Preference preference) {
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

    // ******************************************************************************************************************

    public boolean getPersistedBoolean(String key) {
        if (mDataModule != null) {
            return mDataModule.getBoolean(key);
        }

        return false;
    }

    public boolean persistBoolean(String key, boolean value) {
        if (mDataModule != null) {
            mDataModule.changeSettings(key, value);
//            mDataModule.set(key, value);
            return true;
        }
        return false;
    }

    public String getPersistedString(String key) {
        if (mDataModule != null) {
            return mDataModule.getString(key);
        }

        return null;
    }

    public boolean persistString(String key, String value) {
        if (mDataModule != null) {
            mDataModule.changeSettings(key, value);
//            mDataModule.set(key, value);
            return true;
        }

        return false;
    }

    public String getListSummaryFromKey(String key) {

        String entry = mDataModule.getString(key);

        DataStorageStruct data = (DataStorageStruct) mDataModule
                .getSupportSettingsList().get(key);
        if (data != null) {
            return data.getSummary(entry);
        }
        return "No value";

    }

    public String getListValue(String key) {
        return mDataModule.getString(key);
    }

    public String getListSummaryFromEntry(String key, String entry) {

        DataStorageStruct data = (DataStorageStruct) mDataModule
                .getSupportSettingsList().get(key);
        if (data != null) {
            return data.getSummary(entry);
        }
        return "";
    }

    public CharSequence[] getListEntries(String key) {
        DataStorageStruct data = (DataStorageStruct) mDataModule
                .getSupportSettingsList().get(key);
        if(data != null){
            return data.mEntries;
        }
        return new CharSequence[]{};
    }

    public CharSequence[] getListEntryValues(String key) {
        DataStorageStruct data = (DataStorageStruct) mDataModule
                .getSupportSettingsList().get(key);
        if(data != null){
            return data.mEntryValues;
        }
        return new CharSequence[]{};
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {

        String key = preference.getKey();
        if (preference instanceof DreamUIPreferenceItemSwitch) {
            boolean value = (Boolean) newValue;
            Log.e(TAG, "onPreferenceChange_type_switch key = " + key
                    + " newValue = " + value);

        } else if (preference instanceof DreamUIPreferenceItemList) {

            // change summary and value
            String value = (String) newValue;
            ListPreference listPre = (ListPreference) preference;
            Log.e(TAG, "onPreferenceChange_type_list key = " + key
                    + " newValue = " + value);
            listPre.setSummary(getListSummaryFromEntry(key, value));
            listPre.setValue(value);
        }

//        mDataModule.changeSettings(preference.getKey(), newValue);

        return true;
    }

    @Override
    public void onDreamSettingChangeListener(HashMap<String, String> keys) {
        Set<String> keyList = keys.keySet();
        for (String key : keyList) {
            DreamUIPreferenceItemInterface pre = (DreamUIPreferenceItemInterface) findPreference(key);
            if (pre != null) {
                pre.update();
            }
        }
    }

    public void releaseSource(){
        if(mDataModule != null){
            mDataModule.removeListener(this);
        }
    }
}
