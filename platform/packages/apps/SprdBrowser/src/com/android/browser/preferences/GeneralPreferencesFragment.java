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

import java.util.ArrayList;
import java.util.Arrays;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.res.Resources;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.text.InputType;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

import com.android.browser.BrowserPreferencesPage;
import com.android.browser.BrowserSettings;
import com.android.browser.PreferenceKeys;
import com.android.browser.R;
import com.android.browser.UrlUtils;
import com.android.browser.homepages.HomeProvider;
import com.android.browser.util.Util;
import com.android.browser.sitenavigation.SiteNavigation;
import android.text.InputFilter;

public class GeneralPreferencesFragment extends PreferenceFragment
        implements Preference.OnPreferenceChangeListener {

    static final String TAG = "PersonalPreferencesFragment";

    static final String BLANK_URL = "about:blank";
    static final String CURRENT = "current";
    static final String BLANK = "blank";
    static final String DEFAULT = "default";
    static final String MOST_VISITED = "most_visited";
    static final String OTHER = "other";

    /**
     * Add for navigation tab
     *@{
     */
    static final String BROWSER_HOME = "browser_home";
    /*@}*/
    static final String PREF_HOMEPAGE_PICKER = "homepage_picker";

    String[] mChoices, mValues;
    String mCurrentPage;
    private AlertDialog mHomePageDialog;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Resources res = getActivity().getResources();
        mChoices = res.getStringArray(R.array.pref_homepage_choices);
        mValues = res.getStringArray(R.array.pref_homepage_values);
        mCurrentPage = getActivity().getIntent()
                .getStringExtra(BrowserPreferencesPage.CURRENT_PAGE);

        // Load the XML preferences file
        addPreferencesFromResource(R.xml.general_preferences);

        ListPreference pref = (ListPreference) findPreference(PREF_HOMEPAGE_PICKER);

        /**
         * Add for navigation tab
         *@{
         */
        ArrayList<String> values = new ArrayList<String>();
        ArrayList<String> choices = new ArrayList<String>();
        choices.addAll(Arrays.asList(mChoices));
        values.addAll(Arrays.asList(mValues));
        if (Util.BROWSER_SITE_NAVIGATION_SUPPORT) {
            String navigation = res.getString(R.string.tab_site_navigation);
            if (values.size()-2 >= 0) {
                choices.add(values.size()-2, navigation);
                values.add(values.size()-2, BROWSER_HOME);
            } else {
                choices.add(navigation);
                values.add(BROWSER_HOME);
            }
        }

        /*remove current and most visited in mainpage setting */
        for (int i = 0; i < values.size(); i++) {
            if (CURRENT.equals(values.get(i)) || MOST_VISITED.equals(values.get(i))) {
                choices.remove(i);
                values.remove(i);
            }
        }
        /*remove current and most visited in mainpage setting */

        mValues = new String[values.size()];
        values.toArray(mValues);
        mChoices = new String[choices.size()];
        choices.toArray(mChoices);

        pref.setEntries(mChoices);
        pref.setEntryValues(mValues);
        /*@}*/

        pref.setPersistent(false);
        pref.setValue(getHomepageValue());
        pref.setSummary(getHomepageSummary());
        pref.setOnPreferenceChangeListener(this);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        dismissHomePageDialog();
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object objValue) {
        if (getActivity() == null) {
            // We aren't attached, so don't accept preferences changes from the
            // invisible UI.
            Log.w("PageContentPreferencesFragment", "onPreferenceChange called from detached fragment!");
            return false;
        }

        if (pref.getKey().equals(PREF_HOMEPAGE_PICKER)) {
            BrowserSettings settings = BrowserSettings.getInstance();
            if (CURRENT.equals(objValue)) {
                settings.setHomePage(mCurrentPage);
            }
            if (BLANK.equals(objValue)) {
                settings.setHomePage(BLANK_URL);
            }
            if (DEFAULT.equals(objValue)) {
                settings.setHomePage(BrowserSettings.getFactoryResetHomeUrl(
                        getActivity()));
            }
            if (MOST_VISITED.equals(objValue)) {
                settings.setHomePage(HomeProvider.MOST_VISITED);
            }
            /**
             * Add for navigation tab
             *@{
             */
            if (BROWSER_HOME.equals(objValue)) {
                settings.setHomePage(SiteNavigation.SITE_NAVIGATION);
            }
            /*@}*/
            if (OTHER.equals(objValue)) {
                promptForHomepage((ListPreference) pref);
                return false;
            }
            pref.setSummary(getHomepageSummary());
            ((ListPreference)pref).setValue(getHomepageValue());
            return false;
        }

        return true;
    }

    void dismissHomePageDialog() {
        if (mHomePageDialog != null) {
            mHomePageDialog.dismiss();
            mHomePageDialog = null;
        }
    }

    void promptForHomepage(final ListPreference pref) {
        final BrowserSettings settings = BrowserSettings.getInstance();
        final EditText editText = new EditText(getActivity());
        editText.setInputType(InputType.TYPE_CLASS_TEXT
                | InputType.TYPE_TEXT_VARIATION_URI);
        /*remove current in mainpage setting */
        /*editText.setText(settings.getHomePage());*/
        if (mCurrentPage == null) {
            editText.setText(BrowserSettings.getFactoryResetHomeUrl(getActivity()));
        } else if (mCurrentPage.equals(BLANK_URL) || mCurrentPage.equals(SiteNavigation.SITE_NAVIGATION) || mCurrentPage.equals(HomeProvider.MOST_VISITED)) {
            editText.setText(BrowserSettings.getFactoryResetHomeUrl(getActivity()));
        } else {
            editText.setText(mCurrentPage);
        }
        /*remove current in mainpage setting */
        //limit the text length
        int max_length = getActivity().getResources().getInteger(R.integer.max_url_length);
        editText.setFilters(new InputFilter[]{ new InputFilter.LengthFilter(max_length)});
        editText.setSelectAllOnFocus(true);
        editText.setSingleLine(true);
        editText.setImeActionLabel(null, EditorInfo.IME_ACTION_DONE);
        dismissHomePageDialog();
        mHomePageDialog = new AlertDialog.Builder(getActivity())
                .setView(editText)
                .setPositiveButton(android.R.string.ok, new OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        String homepage = editText.getText().toString().trim();
                        homepage = UrlUtils.smartUrlFilter(homepage);
                        settings.setHomePage(homepage);
                        pref.setValue(getHomepageValue());
                        pref.setSummary(getHomepageSummary());
                    }
                })
                .setNegativeButton(android.R.string.cancel, new OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.cancel();
                    }
                })
                .setTitle(R.string.pref_set_homepage_to)
                .create();
        editText.setOnEditorActionListener(new OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE) {
                    mHomePageDialog.getButton(AlertDialog.BUTTON_POSITIVE).performClick();
                    return true;
                }
                return false;
            }
        });
        mHomePageDialog.getWindow().setSoftInputMode(
                WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
        mHomePageDialog.show();
    }

    String getHomepageValue() {
        BrowserSettings settings = BrowserSettings.getInstance();
        String homepage = settings.getHomePage();
        /**
         * Add for navigation tab
         *@{
         */
        if (SiteNavigation.SITE_NAVIGATION.equals(homepage)) {
            return BROWSER_HOME;
        }
        /*@}*/
        if (TextUtils.isEmpty(homepage) || BLANK_URL.endsWith(homepage)) {
            return BLANK;
        }
        if (HomeProvider.MOST_VISITED.equals(homepage)) {
            return MOST_VISITED;
        }
        String defaultHomepage = BrowserSettings.getFactoryResetHomeUrl(
                getActivity());
        if (TextUtils.equals(defaultHomepage, homepage)) {
            return DEFAULT;
        }
        /*remove current in mainpage setting */
        /*
        if (TextUtils.equals(mCurrentPage, homepage)) {
            return CURRENT;
        }
        */
        /*remove current in mainpage setting */
        return OTHER;
    }

    String getHomepageSummary() {
        BrowserSettings settings = BrowserSettings.getInstance();
        if (settings.useMostVisitedHomepage()) {
            return getHomepageLabel(MOST_VISITED);
        }
        String homepage = settings.getHomePage();
        if (TextUtils.isEmpty(homepage) || BLANK_URL.equals(homepage)) {
            return getHomepageLabel(BLANK);
        }
        /**
         * Add for navigation tab
         *@{
         */
        if (SiteNavigation.SITE_NAVIGATION.equals(homepage)) {
            return getHomepageLabel(BROWSER_HOME);
        }
        /*@}*/
        return homepage;
    }

    String getHomepageLabel(String value) {
        for (int i = 0; i < mValues.length; i++) {
            if (value.equals(mValues[i])) {
                return mChoices[i];
            }
        }
        return null;
    }
}
