/** Created by spreadst */
package com.android.sprdlauncher2;

import android.app.ActivityManager;
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

import com.android.sprdlauncher2.R;



public class WallpaperSettings extends PreferenceActivity implements OnPreferenceClickListener {
    private static final String TAG = "WallpaperSettings";

    //for SharedPreference XML NAME
    public static final boolean ALL_APP_DEFAULT = true;
    public static final boolean WALLPAPER_DEFAULT = false;

    private static final int REQUEST_PICK_WALLPAPER = 0;

    /* SPRD: bug269209 2014-01-19 can not set lockscreen wallpaper @{ */
    private static final String KEY_WORKSPACE_WALLPAPER = "set_wallpager";
    private static final String KEY_LOCKSCREEN_WALLPAPER = "set_lockscreen_wallpager";
    private static final String KEY_ALL_WALLPAPER = "set_all_wallpager";
    /* SPRD: bug269209 2014-01-19 can not set lockscreen wallpaper @} */

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.wallpaper_settings);
        initValue();
    }

    private void initValue(){

        Preference setWallpager = findPreference("set_wallpager");
        setWallpager.setOnPreferenceClickListener(this);

        Preference setLockscreenWallpager = findPreference("set_lockscreen_wallpager");
        setLockscreenWallpager.setOnPreferenceClickListener(this);

        Preference setAllWallpager = findPreference("set_all_wallpager");
        setAllWallpager.setOnPreferenceClickListener(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    static final String WORKSPACE_SETTINGS_INTENT = "workspace_settings_request";
    static final String GO_SET_LOCKSCREEN = "go_set_lockscreen";

    /* SPRD: bug269209 2014-01-19 can not set lockscreen wallpaper @{ */
    @Override
    public boolean onPreferenceClick(Preference preference) {
        String key = preference.getKey();
        if(KEY_WORKSPACE_WALLPAPER.equals(key)){
            final Intent pickWallpaper = new Intent(Intent.ACTION_SET_WALLPAPER);
            pickWallpaper.putExtra(WallpaperInfo.WALLPAPER_TARGET_ID,
                    WallpaperInfo.WALLPAPER_DEFAULT_TYPE);
            pickWallpaper.setComponent(new ComponentName(this,
                    WallpaperPickerActivity.class));
            startActivityForResult(pickWallpaper, REQUEST_PICK_WALLPAPER);
        }
        else if(KEY_LOCKSCREEN_WALLPAPER.equals(key)){
            final Intent pickWallpaper = new Intent(Intent.ACTION_SET_WALLPAPER);
            pickWallpaper.putExtra(WallpaperInfo.WALLPAPER_TARGET_ID,
                    WallpaperInfo.WALLPAPER_LOCKSCREEN_TYPE);
            pickWallpaper.setComponent(new ComponentName(this,
                    WallpaperPickerActivity.class));
            startActivityForResult(pickWallpaper, REQUEST_PICK_WALLPAPER);
        }else if(KEY_ALL_WALLPAPER.equals(key)){
            final Intent pickWallpaper = new Intent(Intent.ACTION_SET_WALLPAPER);
            pickWallpaper.putExtra(WallpaperInfo.WALLPAPER_TARGET_ID,
                    WallpaperInfo.WALLPAPER_ALL_TYPE);
            pickWallpaper.setComponent(new ComponentName(this,
                    WallpaperPickerActivity.class));
            startActivityForResult(pickWallpaper, REQUEST_PICK_WALLPAPER);
        }
        return true;
    }

    public static final String SET_IDLE_PAPER = "SET_IDLE_PAPER";
    public static final String SELECT_IDLE_PAPER_NAME = "WALLPAPER_NAME";

    @Override
    protected void onActivityResult(final int requestCode, int resultCode, final Intent data) {
        finish();
    }
    /* SPRD: bug269209 2014-01-19 can not set lockscreen wallpaper @} */
}
