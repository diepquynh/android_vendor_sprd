/** Created by spreadst */
package com.android.sprdlauncher2;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.util.Log;

import com.android.sprdlauncher2.R;

public class WorkspaceStyleSettings extends PreferenceActivity implements OnPreferenceClickListener {
    private static final String TAG = "workspaceStyleSettings";
    // the workspace_setting.xml key and SharePreference key are same
    public static final String KEY_ANIMATION_STYLE = "workspace_pref_key_animation";
    public static final String PAGEVIEW_KEY_ANIMATION_STYLE = "pageview_pref_key_animation";

    // for SharedPreference XML NAME
    public static final String WORKSPACE_STYLE = "workspace_style";
    public static final int ANIMATION_DEFAULT = 0;// 0 is no animation. 4 is
                                                  // stack
    ListPreference workspaceStyle;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.workspace_style_settings);
        initAnimationValue(getSharedPreferences(WORKSPACE_STYLE,
                Context.MODE_PRIVATE));
    }

    private void initAnimationValue(SharedPreferences rootSharedPreferences) {
        int workSpaceOldValue = rootSharedPreferences.getInt(
                KEY_ANIMATION_STYLE, ANIMATION_DEFAULT);
        String[] anims = getResources().getStringArray(
                R.array.workspace_style_entries);
        String[] values = getResources().getStringArray(
                R.array.workspace_style_values);
        for (int i = 0; i < anims.length; i++) {
            Preference p = new Preference(this);
            p.setTitle(anims[i]);
            p.setKey(values[i]);
            p.setOnPreferenceClickListener(this);
            p.setWidgetLayoutResource(i == workSpaceOldValue ? R.layout.radio_button_on
                    : R.layout.radio_button_off);
            getPreferenceScreen().addPreference(p);
        }
    }

    //Update current selection
    private void updatePreferceSelection(Preference preference ){
        PreferenceScreen ps = getPreferenceScreen();
        int count = ps.getPreferenceCount();
        for(int i=0; i < count; ++i) {
            Preference prf = ps.getPreference(i);
            if(prf.equals( preference)) {
                prf.setWidgetLayoutResource(R.layout.radio_button_on);
            } else {
                prf.setWidgetLayoutResource(R.layout.radio_button_off);
            }
            /**
             * setWidgetLayoutResource only set the id;
             * we should invoke an redraw
             */
            prf.setEnabled(false);
            prf.setEnabled(true);
        }
    }

    public boolean onPreferenceClick(Preference preference) {
        int value = Integer.valueOf(preference.getKey());
        try {
            SharedPreferences.Editor mSharedEditor = getSharedPreferences(
                    WORKSPACE_STYLE, Context.MODE_PRIVATE).edit();
            mSharedEditor.putInt(KEY_ANIMATION_STYLE, value).commit();
            updatePreferceSelection(preference);
        } catch (NumberFormatException e) {
           Log.e(TAG, "could not persist animation setting", e);
        }
        finish();
        return false;
    }
}
