/*
 * Copyright (C) 2011 The Android Open Source Project
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

import android.content.Context;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.webkit.WebView;

import com.android.browser.BrowserSettings;
import com.android.browser.PreferenceKeys;
import com.android.browser.R;

import java.text.NumberFormat;
import com.android.browser.util.Util;

public class AccessibilityPreferencesFragment extends PreferenceFragment
        implements Preference.OnPreferenceChangeListener {

    NumberFormat mFormat;
    // Used to pause/resume timers, which are required for WebViewPreview
    WebView mControlWebView;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mControlWebView = new WebView(getActivity());
        addPreferencesFromResource(R.xml.accessibility_preferences);
        BrowserSettings settings = BrowserSettings.getInstance();
        mFormat = NumberFormat.getPercentInstance();

        Preference e = findPreference(PreferenceKeys.PREF_MIN_FONT_SIZE);
        e.setOnPreferenceChangeListener(this);
        updateMinFontSummary(e, settings.getMinimumFontSize());
        e = findPreference(PreferenceKeys.PREF_TEXT_ZOOM);
        e.setOnPreferenceChangeListener(this);
        updateTextZoomSummary(e, settings.getTextZoom());
        /* SPRD:Bug 491303 Delete the double tap menu because it is not support in Android 6.0. 2015.12.26 */
        // e = findPreference(PreferenceKeys.PREF_DOUBLE_TAP_ZOOM);
        // e.setOnPreferenceChangeListener(this);
        // updateDoubleTapZoomSummary(e, settings.getDoubleTapZoom());
        /* hide
        e = findPreference(PreferenceKeys.PREF_INVERTED_CONTRAST);
        e.setOnPreferenceChangeListener(this);
        updateInvertedContrastSummary(e, (int) (settings.getInvertedContrast() * 100));
        */

        /**
         * Add for horizontal screen
         *@{
         */
        e = findPreference(PreferenceKeys.PREF_HORIZONTAL_SCREEN);
        if (!Util.SUPPORT_HORIZONTAL_SCREEN) {
            getPreferenceScreen().removePreference(e);
        }
        /*@}*/

        /**
         * Add for text format
         *@{
        */
        e = findPreference(PreferenceKeys.PREF_TEXT_FORMAT);
        if (!Util.SUPPORT_TEXT_FORMAT) {
            getPreferenceScreen().removePreference(e);
        } else {
            e.setOnPreferenceChangeListener(this);
            e.setSummary(getVisualTextFormatName(getPreferenceScreen().getSharedPreferences()
                .getString(PreferenceKeys.PREF_TEXT_FORMAT, null)));
        }
        /*@}*/
    }

    @Override
    public void onResume() {
        super.onResume();
        mControlWebView.resumeTimers();
    }

    @Override
    public void onPause() {
        super.onPause();
        mControlWebView.pauseTimers();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mControlWebView.destroy();
        mControlWebView = null;
    }

    void updateMinFontSummary(Preference pref, int minFontSize) {
        Context c = getActivity();
        pref.setSummary(c.getString(R.string.pref_min_font_size_value, minFontSize));
    }

    void updateTextZoomSummary(Preference pref, int textZoom) {
        pref.setSummary(mFormat.format(textZoom / 100.0));
    }

    void updateDoubleTapZoomSummary(Preference pref, int doubleTapZoom) {
        pref.setSummary(mFormat.format(doubleTapZoom / 100.0));
    }

    void updateInvertedContrastSummary(Preference pref, int contrast) {
        pref.setSummary(mFormat.format(contrast / 100.0));
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object objValue) {
        if (getActivity() == null) {
            // We aren't attached, so don't accept preferences changes from the
            // invisible UI.
            return false;
        }

        if (PreferenceKeys.PREF_MIN_FONT_SIZE.equals(pref.getKey())) {
            updateMinFontSummary(pref, BrowserSettings
                    .getAdjustedMinimumFontSize((Integer) objValue));
        }
        if (PreferenceKeys.PREF_TEXT_ZOOM.equals(pref.getKey())) {
            BrowserSettings settings = BrowserSettings.getInstance();
            updateTextZoomSummary(pref, settings
                    .getAdjustedTextZoom((Integer) objValue));
        }
        /* SPRD:Bug 491303 Delete the double tap menu because it is not support in Android 6.0. 2015.12.26 */
        /* if (PreferenceKeys.PREF_DOUBLE_TAP_ZOOM.equals(pref.getKey())) {
            BrowserSettings settings = BrowserSettings.getInstance();
            updateDoubleTapZoomSummary(pref, settings
                    .getAdjustedDoubleTapZoom((Integer) objValue));
        }
        */
        /* hide
        if (PreferenceKeys.PREF_INVERTED_CONTRAST.equals(pref.getKey())) {
            updateInvertedContrastSummary(pref,
                    (int) ((10 + (Integer) objValue) * 10));
        }
        */

        /**
         * Add for text format
         *@{
        */
        if (PreferenceKeys.PREF_TEXT_FORMAT.equals(pref.getKey())) {
            pref.setSummary(getVisualTextFormatName((String) objValue));
        }
        /*@}*/
        return true;
    }

    /**
     * Add for text format
     *@{
    */
    private CharSequence getVisualTextFormatName(String enumName) {
        CharSequence[] visualNames = getResources().getTextArray(R.array.pref_text_format_choices);
        CharSequence[] enumNames = getResources().getTextArray(R.array.pref_text_format_values);

        // Sanity check
        if (visualNames.length != enumNames.length) {
            return "";
        }

        for (int i = 0; i < enumNames.length; i++) {
            if (enumNames[i].equals(enumName)) {
                return visualNames[i];
            }
        }

        return "";
    }
    /*@}*/
}
