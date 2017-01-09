/*
 * Copyright (C) 2010 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.browser.preferences;

import com.android.browser.BrowserActivity;
import com.android.browser.PreferenceKeys;
import com.android.browser.R;

import android.content.Intent;
import android.content.res.Resources;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.webkit.GeolocationPermissions;
import android.webkit.ValueCallback;
import android.webkit.WebStorage;

import java.util.Map;
import java.util.Set;

import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.Context;
import com.android.browser.Controller;
import android.os.Parcelable;
import com.android.browser.BrowserSettings;
import com.android.browser.util.Util;

public class AdvancedPreferencesFragment extends PreferenceFragment
        implements Preference.OnPreferenceChangeListener, Preference.OnPreferenceClickListener {

    String absolutepathStr ="";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //modify for permissions
        if (savedInstanceState != null && savedInstanceState.getParcelable("PermissionObj") != null) {
            Controller.mPermissionObj = savedInstanceState.getParcelable("PermissionObj");
        }

        // Load the XML preferences file
        addPreferencesFromResource(R.xml.advanced_preferences);

        PreferenceScreen websiteSettings = (PreferenceScreen) findPreference(
                PreferenceKeys.PREF_WEBSITE_SETTINGS);
        websiteSettings.setFragment(WebsiteSettingsFragment.class.getName());

        Preference e = findPreference(PreferenceKeys.PREF_DEFAULT_ZOOM);
        e.setOnPreferenceChangeListener(this);
        e.setSummary(getVisualDefaultZoomName(
                getPreferenceScreen().getSharedPreferences()
                .getString(PreferenceKeys.PREF_DEFAULT_ZOOM, null)) );

        e = findPreference(PreferenceKeys.PREF_DEFAULT_TEXT_ENCODING);
        e.setOnPreferenceChangeListener(this);
        /*text encoding menu do not show summary*/
        updateListPreferenceSummary((ListPreference) e);

        e = findPreference(PreferenceKeys.PREF_RESET_DEFAULT_PREFERENCES);
        e.setOnPreferenceChangeListener(this);
        /**
         *Add for customise SearchEngine
         *Original Android code:
         *e = findPreference(PreferenceKeys.PREF_SEARCH_ENGINE);
         *e.setOnPreferenceChangeListener(this);
         *updateListPreferenceSummary((ListPreference) e);
         *@{
         */
        ListPreference enginePref = (ListPreference)findPreference(PreferenceKeys.PREF_SEARCH_ENGINE);
        String defaultEngine = BrowserSettings.getInstance().getSearchEngineName();
        defaultEngine = updateSearchEngineName(defaultEngine);
        enginePref.setValue(defaultEngine);
        enginePref.setSummary(defaultEngine);
        enginePref.setOnPreferenceChangeListener(this);
        updateListPreferenceSummary(enginePref);
        /*@}*/

        e = findPreference(PreferenceKeys.PREF_PLUGIN_STATE);
        e.setOnPreferenceChangeListener(this);
        updateListPreferenceSummary((ListPreference) e);
        /*
         * for download_storage_save_path
         *@{
         */
        SavePathPreference savePathSettings = (SavePathPreference) findPreference(PreferenceKeys.PREF_SAVE_PATH);
        if (Util.SUPPORT_SELECT_DOWNLOAD_PATH) {
            savePathSettings.setOnPreferenceClickListener(this);
            IntentFilter intentFilterReceiveFilePath = new IntentFilter("com.android.browser.SendToBrowserASavePath");
            getActivity().registerReceiver(mReceiver, intentFilterReceiveFilePath);
        } else {
            getPreferenceScreen().removePreference(savePathSettings);
        }
        /*@}*/

        //remove default zoom preference
        e = findPreference(PreferenceKeys.PREF_DEFAULT_ZOOM);
        PreferenceCategory webSiteSettingsCategory = (PreferenceCategory) findPreference("website_settings_category");
        webSiteSettingsCategory.removePreference(e);
    }

    private String updateSearchEngineName(String searchEngineName) {
        String shortEngineName = searchEngineName;
        if (searchEngineName != null){
            if (searchEngineName.contains("_")){
                int index = searchEngineName.indexOf("_");
                shortEngineName = searchEngineName.substring(0, index);
            }
        }
        Log.i("AdvancedPreferencesFragment", "updateSearchEngine searchEngineName = " + searchEngineName);
        Resources res = getActivity().getResources();
        String[] searchEngines = res.getStringArray(R.array.search_engines);
        for (int i = 0; i < searchEngines.length; i++) {
            String name = searchEngines[i];
            if (name.contains(shortEngineName)){
                Log.i("AdvancedPreferencesFragment", "updateSearchEngine set " + name);
                return name;
            }
        }
        return searchEngineName;
    }

    void updateListPreferenceSummary(ListPreference e) {
        e.setSummary(e.getEntry());
    }

    /*
     * We need to set the PreferenceScreen state in onResume(), as the number of
     * origins with active features (WebStorage, Geolocation etc) could have
     * changed after calling the WebsiteSettingsActivity.
     */
    @Override
    public void onResume() {
        super.onResume();
        final PreferenceScreen websiteSettings = (PreferenceScreen) findPreference(
                PreferenceKeys.PREF_WEBSITE_SETTINGS);
        websiteSettings.setEnabled(false);
        WebStorage.getInstance().getOrigins(new ValueCallback<Map>() {
            @Override
            public void onReceiveValue(Map webStorageOrigins) {
                if ((webStorageOrigins != null) && !webStorageOrigins.isEmpty()) {
                    websiteSettings.setEnabled(true);
                }
            }
        });
        GeolocationPermissions.getInstance().getOrigins(new ValueCallback<Set<String> >() {
            @Override
            public void onReceiveValue(Set<String> geolocationOrigins) {
                if ((geolocationOrigins != null) && !geolocationOrigins.isEmpty()) {
                    websiteSettings.setEnabled(true);
                }
            }
        });
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object objValue) {
        if (getActivity() == null) {
            // We aren't attached, so don't accept preferences changes from the
            // invisible UI.
            Log.w("AdvancedPreferencesFragment", "onPreferenceChange called from detached fragment!");
            return false;
        }

        if (pref.getKey().equals(PreferenceKeys.PREF_DEFAULT_ZOOM)) {
            pref.setSummary(getVisualDefaultZoomName((String) objValue));
            return true;
        } else if (pref.getKey().equals(PreferenceKeys.PREF_RESET_DEFAULT_PREFERENCES)) {
            Boolean value = (Boolean) objValue;
            if (value.booleanValue() == true) {
                startActivity(new Intent(BrowserActivity.ACTION_RESTART, null,
                        getActivity(), BrowserActivity.class));
                return true;
            }
        } else if (pref.getKey().equals(PreferenceKeys.PREF_PLUGIN_STATE)
                || pref.getKey().equals(PreferenceKeys.PREF_SEARCH_ENGINE)
                /*text encoding menu do not show summary*/
                || pref.getKey().equals(PreferenceKeys.PREF_DEFAULT_TEXT_ENCODING)) {
            ListPreference lp = (ListPreference) pref;
            lp.setValue((String) objValue);
            updateListPreferenceSummary(lp);
            return false;
        }
        return false;
    }

    /*
     * for download_storage_save_path
     *@{
     */
     @Override
     public boolean onPreferenceClick(Preference pref) {
        if (getActivity() == null) {
            // We aren't attached, so don't accept preferences click from the
            // invisible UI.
            Log.w("AdvancedPreferencesFragment", "onPreferenceClick called from detached fragment!");
            return false;
        }

        if (pref.getKey().equals(PreferenceKeys.PREF_SAVE_PATH)) {
            ((SavePathPreference)pref).selectDownloadStorage();
            return true;
        }
        return false;
    }
    /*@}*/

    private CharSequence getVisualDefaultZoomName(String enumName) {
        Resources res = getActivity().getResources();
        CharSequence[] visualNames = res.getTextArray(R.array.pref_default_zoom_choices);
        CharSequence[] enumNames = res.getTextArray(R.array.pref_default_zoom_values);

        // Sanity check
        if (visualNames.length != enumNames.length) {
            return "";
        }

        int length = enumNames.length;
        for (int i = 0; i < length; i++) {
            if (enumNames[i].equals(enumName)) {
                return visualNames[i];
            }
        }

        return "";
    }

    /*
     * for download_storage_save_path
     *@{
     */
    @Override
    public void onDestroy() {
        // TODO Auto-generated method stub
        super.onDestroy();
        if (Util.SUPPORT_SELECT_DOWNLOAD_PATH) {
            //modify for permissions
            Controller.isSavePathEditorActivityStart = false;
            Controller.mPermissionObj = null;
            if (mReceiver != null)
                getActivity().unregisterReceiver(mReceiver);
        }
    }

    //modify for permissions
    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        if (Controller.mPermissionObj != null && Controller.mPermissionObj instanceof Intent) {
            outState.putParcelable("PermissionObj", (Parcelable)Controller.mPermissionObj);
        }
    }

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals("com.android.browser.SendToBrowserASavePath")) {
                Bundle bundle= intent.getExtras();
                absolutepathStr = bundle.getString("absolutepath");
                SavePathPreference savePathSettings = (SavePathPreference) findPreference(PreferenceKeys.PREF_SAVE_PATH);
                savePathSettings.setSummary(absolutepathStr);
            }
        }
    };

    @Override
    public void onStart() {
        super.onStart();
        if (Util.SUPPORT_SELECT_DOWNLOAD_PATH) {
            SavePathPreference savePathSettings = (SavePathPreference) findPreference(PreferenceKeys.PREF_SAVE_PATH);
            savePathSettings.updateSavePath();
        }
    }

    /*@}*/
}
