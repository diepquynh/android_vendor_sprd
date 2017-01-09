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
 * limitations under the License.
 */
package com.android.quicksearchbox.preferences;

import java.util.HashMap;

import com.android.quicksearchbox.R;
import com.android.quicksearchbox.util.Util;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;
import android.view.Window;

/**
 * Fragment for selecting engine items
 */
public class SearchEngineFragment extends PreferenceFragment {

    public static final String KEY_ENGINE = "enginelist";
    public static final int MSG_REFRESH = 1;
    public static String EngineKey[] = null;
    public static String EngineSummary[] = null;
    private PreferenceGroup mEngineProfileList;
    private String mSelectedEngineKey;
    private HashMap<String, SearchEnginePreference> mAllEngineMap;

    /* SPRD:modify 20140203 Spreast of 275516  @{ */
    private final static int BAIDU_SEARCH_ID     = 0;
    private final static int GOOGLE_SEARCH_ID    = 1;
    private final static int CMCC_SEARCH_ID      = 2;
    private final static int YAHOO_SEARCH_ID     = 3;
    private final static int BING_SEARCH_ID      = 4;
    /* @} */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.preferences_searchengine_items);
        mAllEngineMap = new HashMap<String, SearchEnginePreference>();
        fillList();
        this.getActivity().setTitle(R.string.search_engine_site);

    }

    final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            int what = msg.what;
            switch (what) {
            case MSG_REFRESH:
                updateSelectedState((String) msg.obj);
                break;
            default:
                break;
            }
        }
    };

    @Override
    public void onDestroy() {
        /* SPRD:modify 20130911 Spreast of 214740 search show nullPointerException @{ */
        if(mAllEngineMap != null){
            mAllEngineMap.clear();
            mAllEngineMap = null;
            super.onDestroy();
        }
        /* @} */
    }

    private void fillList() {
        mEngineProfileList = (PreferenceGroup) findPreference("web_engine_list");
        EngineKey = getResources()
                .getStringArray(R.array.search_engine_entries);
        EngineSummary = getResources().getStringArray(
                R.array.search_engine_summary);
        PreferenceCategory cate = (PreferenceCategory) findPreference("engine_profiles_list");

        for (int id = 0; id < EngineKey.length; id++) {
            String displayName = EngineKey[id];
            // SPRD:531737 remove 139search
            if (id == CMCC_SEARCH_ID) {
                continue;
            }

            /* SPRD:modify 20140203 Spreast of 275516  @{ */
            if (id == YAHOO_SEARCH_ID ) {
                continue;
            }
            /* @} */

            SearchEnginePreference pref = new SearchEnginePreference(
                    getActivity());
            pref.setKey(com.android.quicksearchbox.util.Util.ENGINES[id]);
            pref.setId(id);
            pref.setTitle(displayName);
            pref.setSummary(EngineSummary[id]);
            pref.setPersistent(true);
            cate.addPreference(pref);
            mAllEngineMap.put(pref.getKey(), pref);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        //SPRD: 493103 set search engine in global settings to avoid crash
        mSelectedEngineKey = Settings.Global.getString(getActivity()
                .getContentResolver(),
                com.android.quicksearchbox.util.Util.MYENGINE);
        /* SPRD: Modify for bug 620610 change default SearchEngine to google {@ */
        if (TextUtils.isEmpty(mSelectedEngineKey)) {
            // defaule
            mSelectedEngineKey = com.android.quicksearchbox.util.Util.ENGINES[1];
        }
        /* @} */
        Message msg = mHandler.obtainMessage(MSG_REFRESH, this);
        msg.obj = mSelectedEngineKey;
        mHandler.sendMessageDelayed(msg, 100);
    }

    private void updateSelectedState(String chooseKey) {
        /* SPRD:modify 20130911 Spreast of 214740 search show nullPointerException @{ */
        if (mAllEngineMap != null) {
            for (HashMap.Entry<String, SearchEnginePreference> entry : mAllEngineMap
                    .entrySet()) {
                if (chooseKey.equals(entry.getKey())) {
                    ((SearchEnginePreference) entry.getValue()).setChecked(true);
                    mSelectedEngineKey = chooseKey;
                    /* SPRD: Modify for bug 620610 change default SearchEngine to google {@ */
                    if (TextUtils.isEmpty(mSelectedEngineKey)) {
                        // default
                        mSelectedEngineKey = com.android.quicksearchbox.util.Util.ENGINES[1];
                    }
                    /* @} */
                    //SPRD: 493103 set search engine in global settings to avoid crash
                    Settings.Global.putString(getActivity().getContentResolver(),
                            com.android.quicksearchbox.util.Util.MYENGINE,
                            mSelectedEngineKey);
                } else {
                    ((SearchEnginePreference) entry.getValue()).setChecked(false);
                }
            }
        }
        /* @} */
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {
        if (preference instanceof SearchEnginePreference) {
            Message msg = mHandler.obtainMessage(MSG_REFRESH, this);
            msg.obj = preference.getKey();
            mHandler.sendMessage(msg);
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

}
