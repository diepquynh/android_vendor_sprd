
package com.sprd.engineermode.debuglog;

import android.os.Bundle;
import android.os.Handler;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.TwoStatePreference;

import com.sprd.engineermode.R;
import com.sprd.engineermode.EMSwitchPreference;

public class AndroidUtilsActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener {

    //public static final String KEY_SLIDE_SETTINGS = "key_slide_settings";
     /* SPRD:modify for Bug 653299 add switch for starting window. @{ */
    private static final String KEY_STARTING_WINDOW = "startingwindow";
    private static final String STARTING_WINDOW__ENABLED = "persist.sys.startingwindow";
    private TwoStatePreference mStartingWindow;
    /* @} */


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_android_utils);
        //Preference slideSettingsPreference = (Preference) findPreference(KEY_SLIDE_SETTINGS);

	/* SPRD:modify for Bug 653299 add switch for starting window. @{ */
        mStartingWindow = (TwoStatePreference) findPreference(KEY_STARTING_WINDOW);
        mStartingWindow.setOnPreferenceChangeListener(this);
        /* @} */
    }

    @Override
    public void onResume() {
        super.onResume();
 
        /* SPRD:modify for Bug 653299 add switch for starting window. @{ */
        if (mStartingWindow != null) {
            mStartingWindow.setChecked(SystemProperties.getBoolean(STARTING_WINDOW__ENABLED, true));
        }
        /* @} */
    }

     @Override
    public boolean onPreferenceChange(Preference preference, Object keyValue) {
         if (preference == mStartingWindow) {
            SystemProperties.set(STARTING_WINDOW__ENABLED, Boolean.toString((boolean) keyValue));
            return true;
        }
        /* @} */
        return false;
    }
}
