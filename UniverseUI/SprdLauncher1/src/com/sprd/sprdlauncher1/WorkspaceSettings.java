/** Created by Spreadst */
package com.sprd.launcher3;

import android.app.WallpaperInfo;
import android.app.WallpaperManager;
import android.appwidget.AppWidgetManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.provider.Settings;
import android.util.Log;
import android.content.ComponentName;

import com.android.sprdlauncher1.Launcher;
import com.android.sprdlauncher1.R;
import com.android.sprdlauncher1.WallpaperPickerActivity;

/* SPRD: bug268719 2014-01-19 can not set lockscreen wallpaper @{ */
public class WorkspaceSettings extends PreferenceActivity implements Preference.OnPreferenceChangeListener, OnPreferenceClickListener {
    private static final String TAG = "workspaceStyleSettings";

    // preference key for workspace effects
    public static final String KEY_ANIMATION_STYLE = "workspace_pref_key_animation";
    // preference key for apps view and widets view effects
    public static final String PAGEVIEW_KEY_ANIMATION_STYLE = "pageview_pref_key_animation";

    // SharedPreference file name
    public static final String WORKSPACE_STYLE = "workspace_style";
    // SPRD: fix bug253654 transform effect error ,default no animation now
    public static final int ANIMATION_DEFAULT = 0; // 0 is no animation

    private static final int REQUEST_PICK_WALLPAPER = 0;
    private static final int REQUEST_PICK_IDLE_WALLPAPER = 1;

    private static final String KEY_CHOOSE_APP = "go_all_applications";
    private static final String KEY_CHOOSE_WIDGET = "go_all_widgets";
    private static final String KEY_WORKSPACE_WALLPAPER = "set_wallpager";
    private static final String KEY_MAINMENU_WALLPAPER = "set_mainmenu_wallpager";
    private static final String KEY_LOCKSCREEN_WALLPAPER = "set_lockscreen_wallpager";
    private static final String KEY_ALL_WALLPAPER = "set_all_wallpager";

    public static final String WORKSPACE_SETTINGS_INTENT = "workspace_settings_request";
    public static final String GO_ALL_APP = "goAllApplication";
    public static final String GO_ALL_WIGETS = "goAllWiget";
    public static final String SELECT_IDLE_PAPER_NAME = "WALLPAPER_NAME";
    public static final String SET_IDLE_PAPER = "SET_IDLE_PAPER";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.workspace_settings);
        initValue(getSharedPreferences(WORKSPACE_STYLE, Context.MODE_PRIVATE));
    }

    private void initValue(SharedPreferences rootSharedPreferences){

        initAnimationValue(rootSharedPreferences);
        Preference gotoApps = findPreference("go_all_applications");
        gotoApps.setOnPreferenceClickListener(this);
        Preference gotoWidgets = findPreference("go_all_widgets");
        gotoWidgets.setOnPreferenceClickListener(this);
        Preference setWallpager = findPreference("set_wallpager");
        setWallpager.setOnPreferenceClickListener(this);

        Preference setLockscreenWallpager = findPreference("set_lockscreen_wallpager");
        setLockscreenWallpager.setOnPreferenceClickListener(this);

        Preference setAllWallpager = findPreference("set_all_wallpager");
        setAllWallpager.setOnPreferenceClickListener(this);
    }

    private void initAnimationValue(SharedPreferences rootSharedPreferences){
        // workspace for animation
        ListPreference workspaceStyle = (ListPreference) findPreference(KEY_ANIMATION_STYLE);
        int workSpaceOldValue  = rootSharedPreferences.getInt(KEY_ANIMATION_STYLE,ANIMATION_DEFAULT);
        workspaceStyle.setValue(String.valueOf(workSpaceOldValue));
        CharSequence[] summaries = getResources().getTextArray(R.array.workspace_style_summaries);
        workspaceStyle.setSummary(summaries[workSpaceOldValue]);
        workspaceStyle.setOnPreferenceChangeListener(this);

        // apps and widgets animation
        ListPreference pageViewStyle = (ListPreference) findPreference(PAGEVIEW_KEY_ANIMATION_STYLE);
        int  pageViewOldValue = rootSharedPreferences.getInt(PAGEVIEW_KEY_ANIMATION_STYLE, ANIMATION_DEFAULT);
        pageViewStyle.setValue(String.valueOf(pageViewOldValue));
        CharSequence[] pageviewsummaries = getResources().getTextArray(R.array.pageview_style_summaries);
        pageViewStyle.setSummary(pageviewsummaries[pageViewOldValue]);
        pageViewStyle.setOnPreferenceChangeListener(this);

    }

    private void updateAnimationsSummary(Object value,String prefKey) {
        CharSequence[] summaries = null;
        CharSequence[] values = null;
        ListPreference preference = null;

        if (KEY_ANIMATION_STYLE.equals(prefKey)) {
            preference = (ListPreference) findPreference(KEY_ANIMATION_STYLE);
            summaries = getResources().getTextArray(R.array.workspace_style_summaries);
            values = preference.getEntryValues();
        } else if (PAGEVIEW_KEY_ANIMATION_STYLE.equals(prefKey)) {
            preference = (ListPreference) findPreference(PAGEVIEW_KEY_ANIMATION_STYLE);
            summaries = getResources().getTextArray(R.array.pageview_style_summaries);
            values = preference.getEntryValues();
        }

        if (values != null && summaries != null && preference != null) {
            for (int i = 0; i < values.length; i++) {
                if (values[i].equals(value)) {
                    preference.setSummary(summaries[i]);
                    break;
                }
            }
        }
    }

    private void onChangeAnimation(String newValue,String prefKey){
        try {
            int value = Integer.parseInt(newValue);

            SharedPreferences.Editor mSharedEditor =
                    getSharedPreferences(WORKSPACE_STYLE, Context.MODE_PRIVATE).edit();
            mSharedEditor.putInt(prefKey, value).commit();

            updateAnimationsSummary(newValue,prefKey);

        } catch (NumberFormatException e) {
            Log.e(TAG, "could not persist animation setting", e);
        }
    }


    public boolean onPreferenceChange(Preference preference, Object objValue) {
        String key = preference.getKey();
        if (KEY_ANIMATION_STYLE.equals(key)) {
            onChangeAnimation((String) objValue,KEY_ANIMATION_STYLE);
        } else if (PAGEVIEW_KEY_ANIMATION_STYLE.equals(key)) {
            onChangeAnimation((String) objValue,PAGEVIEW_KEY_ANIMATION_STYLE);
        }

        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        String key = preference.getKey();
        if (KEY_CHOOSE_APP.equals(key)) {
            Intent intent = new Intent(this,Launcher.class);
            intent.putExtra(WORKSPACE_SETTINGS_INTENT, GO_ALL_APP);
            startActivity(intent);
        } else if (KEY_CHOOSE_WIDGET.equals(key)) {
            Intent intent = new Intent(this,Launcher.class);
            intent.putExtra(WORKSPACE_SETTINGS_INTENT, GO_ALL_WIGETS);
            startActivity(intent);
        } else if (KEY_WORKSPACE_WALLPAPER.equals(key)) {
            final Intent pickWallpaper = new Intent(Intent.ACTION_SET_WALLPAPER);
            pickWallpaper.putExtra(WallpaperInfo.WALLPAPER_TARGET_ID,
                WallpaperInfo.WALLPAPER_DEFAULT_TYPE);
            pickWallpaper.setComponent(new ComponentName(this,
                    WallpaperPickerActivity.class));
            startActivityForResult(pickWallpaper, REQUEST_PICK_WALLPAPER);
        } else if (KEY_LOCKSCREEN_WALLPAPER.equals(key)) {
            final Intent pickWallpaper = new Intent(Intent.ACTION_SET_WALLPAPER);
            pickWallpaper.putExtra(WallpaperInfo.WALLPAPER_TARGET_ID,
                    WallpaperInfo.WALLPAPER_LOCKSCREEN_TYPE);
            pickWallpaper.setComponent(new ComponentName(this,
                    WallpaperPickerActivity.class));
            startActivityForResult(pickWallpaper, REQUEST_PICK_WALLPAPER);
        } else if (KEY_ALL_WALLPAPER.equals(key)) {
            final Intent pickWallpaper = new Intent(Intent.ACTION_SET_WALLPAPER);
            pickWallpaper.putExtra(WallpaperInfo.WALLPAPER_TARGET_ID,
                    WallpaperInfo.WALLPAPER_ALL_TYPE);
            pickWallpaper.setComponent(new ComponentName(this,
                    WallpaperPickerActivity.class));
            startActivityForResult(pickWallpaper, REQUEST_PICK_WALLPAPER);
        }
        return true;
    }

    @Override
    protected void onActivityResult(final int requestCode, int resultCode, final Intent data) {
        /* SPRD: add for bug 257794 @{ */
        if (requestCode == REQUEST_PICK_WALLPAPER) {
            setResult(resultCode);
            if (resultCode != RESULT_OK) {
                return;
            }
        }
        /* @} */
        finish();
    }
}
/* SPRD: bug268719 2014-01-19 can not set lockscreen wallpaper @} */

