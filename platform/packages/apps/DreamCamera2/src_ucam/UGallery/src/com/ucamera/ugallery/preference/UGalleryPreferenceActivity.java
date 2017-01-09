/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery.preference;

import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.widget.CheckBox;
import com.ucamera.ugallery.MyFullDialogActivity;
import com.ucamera.ugallery.R;
import com.ucamera.ugallery.UGalleryConst;
import com.ucamera.ugallery.gallery.privateimage.ImagePassWordDialog;
import com.ucamera.ugallery.gallery.privateimage.ImagePassWordDialog.PrivateAbulmCallback;
import com.ucamera.ugallery.gallery.privateimage.util.PasswordUtils;
import com.ucamera.ugallery.integration.Build;
import com.ucamera.ugallery.preference.OtherPreference.OtherPerenceOnClickListener;

public class UGalleryPreferenceActivity extends PreferenceActivity implements OnPreferenceClickListener, OnSharedPreferenceChangeListener, OtherPerenceOnClickListener {

//    private CheckBoxPreference mPwdSettingPref;
    private CheckBoxPreference mCreatPwdSettingPref;
    private SharedPreferences mSharedPre;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.ugallery_preferences);

        PreferenceScreen preferences = getPreferenceScreen();
        preferences.getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
//        mPwdSettingPref = (CheckBoxPreference)preferences.findPreference(UGalleryConst.KEY_PASSWORD_SETTING);
//        mPwdSettingPref.setOnPreferenceClickListener(this);
        mCreatPwdSettingPref = ((CheckBoxPreference) preferences.findPreference(UGalleryConst.KEY_OTHER_CREATPWD));
        mCreatPwdSettingPref.setOnPreferenceClickListener(this);
        mSharedPre = PreferenceManager.getDefaultSharedPreferences(this);
        if (PasswordUtils.isPasswordFileExist()) {
            mCreatPwdSettingPref.setChecked(true);
        } else {
            mCreatPwdSettingPref.setChecked(false);
        }
        ((OtherPreference) preferences.findPreference(UGalleryConst.KEY_SETTING_OTHER_SHOW_TIPS)).setListener(this);
        if( !Build.HIDE_ADVANCE_SETTINGS) {
            ((OtherPreference) preferences.findPreference(UGalleryConst.KEY_OTHER_FEEDBACK)).setListener(this);
            ((OtherPreference) preferences.findPreference(UGalleryConst.KEY_OTHER_ABOUTUS)).setListener(this);
        } else {
            PreferenceCategory prefCat =  (PreferenceCategory)preferences.getPreference(0);
            prefCat.removePreference(((OtherPreference) preferences.findPreference(UGalleryConst.KEY_OTHER_FEEDBACK)));
            prefCat.removePreference(((OtherPreference) preferences.findPreference(UGalleryConst.KEY_OTHER_ABOUTUS)));
        }
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    public void onClickFeedback() {
        Intent intent = new Intent(this, MyFullDialogActivity.class);
        intent.putExtra(UGalleryConst.DIALOG_TYPE_SETTINGS, MyDialogStub.FeedBackDialogStubType);
        this.startActivity(intent);
    }

    @Override
    public void onClickAboutUs() {
        Intent intent = new Intent(this, MyFullDialogActivity.class);
        intent.putExtra(UGalleryConst.DIALOG_TYPE_SETTINGS, MyDialogStub.AboutUsDialogStubType);
        this.startActivity(intent);
    }

    @Override
    public void onClickShowTips() {
        mSharedPre.edit().putBoolean(UGalleryConst.KEY_SECRET_ABLUM_FRIST_SHOW_TIPS_FINISHED, false).commit();
        this.finish();
    }

    @Override
    public boolean onPreferenceClick(final Preference preference) {
        String key = preference.getKey();
/*        if(UGalleryConst.KEY_PASSWORD_SETTING.equals(key)) {
            final boolean isChecked = mSharedPre.getBoolean(key, true);
            mSharedPre.edit().putBoolean(UGalleryConst.KEY_SHOW_SCANNING, true).commit();
            if(PasswordUtils.isPasswordFileExist()) {
                ImagePassWordDialog dialog =  new ImagePassWordDialog(UGalleryPreferenceActivity.this ,false, R.string.text_settings_title, new PrivateAbulmCallback() {

                    @Override
                    public void controlPrivateAbulmOFF() {
                        mPwdSettingPref.setChecked(!isChecked);
                    }
                    @Override
                    public void controlPrivateAbulmNO() {
                        mPwdSettingPref.setChecked(isChecked);
                    }
                });
                dialog.show();
            }
            return true;
        } else */
        if (UGalleryConst.KEY_OTHER_CREATPWD.equals(key)){
            final boolean isChecked = mSharedPre.getBoolean(key, false);
            ImagePassWordDialog dialog =  new ImagePassWordDialog(UGalleryPreferenceActivity.this, true, new PrivateAbulmCallback() {
                @Override
                public void controlPrivateAbulmOFF() {
                    mCreatPwdSettingPref.setChecked(!isChecked);
                }
                @Override
                public void controlPrivateAbulmNO() {
                    mCreatPwdSettingPref.setChecked(isChecked);
                }
            });
            dialog.show();
            return true;
        }
        return false;
    }
}
