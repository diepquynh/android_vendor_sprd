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

import com.android.browser.PreferenceKeys;
import com.android.browser.R;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.os.UserHandle;
import android.os.UserManager;
import android.content.pm.UserInfo;
import android.content.Context;
import android.preference.PreferenceCategory;

public class PrivacySecurityPreferencesFragment extends PreferenceFragment
        implements Preference.OnPreferenceChangeListener {

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Load the preferences from an XML resource
        addPreferencesFromResource(R.xml.privacy_security_preferences);

        Preference e = findPreference(PreferenceKeys.PREF_PRIVACY_CLEAR_HISTORY);
        e.setOnPreferenceChangeListener(this);
        /*Sprd:remove passWordCategory when login as guest begin */
        UserManager sUserManager = (UserManager)getActivity().getSystemService(Context.USER_SERVICE);
        if (sUserManager != null){
            UserInfo sUserInfo = sUserManager.getUserInfo(UserHandle.myUserId());
            if ((sUserInfo != null && sUserInfo.isGuest())||UserHandle.myUserId()!=0){
                PreferenceCategory passWordCategory = (PreferenceCategory) findPreference("password_category");
                getPreferenceScreen().removePreference(passWordCategory);
            }
        }
        /*Sprd:remove passWordCategory when login as guest end */
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object objValue) {
        if (pref.getKey().equals(PreferenceKeys.PREF_PRIVACY_CLEAR_HISTORY)
                && ((Boolean) objValue).booleanValue() == true) {
            // Need to tell the browser to remove the parent/child relationship
            // between tabs
            getActivity().setResult(Activity.RESULT_OK, (new Intent()).putExtra(Intent.EXTRA_TEXT,
                    pref.getKey()));
            return true;
        }

        return false;
    }

}
