/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.view.KeyEvent;

import com.ucamera.uphoto.preference.MyListPreference;
import com.ucamera.uphoto.preference.MyPathPreference;

public class UPhotoPreferencesActivity extends PreferenceActivity implements OnSharedPreferenceChangeListener{
    private MyListPreference mPictureSizeListPreference;
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.uphoto_preferences);

        PreferenceScreen preferences = getPreferenceScreen();
        preferences.getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
        mPictureSizeListPreference = (MyListPreference) preferences.findPreference(ImageEditConstants.PREF_UPHOTO_PICTURE_SIZE_KEY);
        SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
        String pictureSizeValue = sharedPreferences.getString(ImageEditConstants.PREF_UPHOTO_PICTURE_SIZE_KEY,
                getString(R.string.text_uphoto_picture_size_default_value));
        mPictureSizeListPreference.setSummary(pictureSizeValue);
        mPictureSizeListPreference.setValue(pictureSizeValue);
    }

    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        if(ImageEditConstants.PREF_UPHOTO_PICTURE_SIZE_KEY.equals(key)) {
            String pictureSizeValue = sharedPreferences.getString(key, getString(R.string.text_uphoto_picture_size_default_value));
            mPictureSizeListPreference.setSummary(pictureSizeValue);
            mPictureSizeListPreference.setValue(pictureSizeValue);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
//        StatApi.onPause(this);
    }
    @Override
    protected void onResume() {
        super.onResume();
//        StatApi.onResume(this);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_MENU) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }
}
