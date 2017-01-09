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

import com.android.quicksearchbox.R;

/**
 * 'Device' preferences.
 */
public class DeviceSearchFragment extends SettingsFragmentBase {

    @Override
    protected int getPreferencesResourceId() {
        /*add 20130319 Spreadst of 139493 title is error start*/
        this.getActivity().setTitle(R.string.device_settings_category_title);
        /*add 20130319 Spreadst of 139493 title is error end*/
        return R.xml.device_search_preferences;
    }

}
