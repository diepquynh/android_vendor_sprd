/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.fmradio;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.os.Environment;
import android.os.EnvironmentEx;
import android.os.StatFs;
import android.os.storage.StorageManager;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.util.Log;
import android.util.TypedValue;
import android.view.View.MeasureSpec;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

/**
 * SPRD:
 *
 * @{
 */
import java.io.File;
import java.io.IOException;
import android.content.pm.PackageManager;
import android.Manifest;
import android.os.SystemProperties;
import android.os.storage.VolumeInfo;

/**
 * @}
 */

/**
 * This class provider interface to compute station and frequency, get project string
 */
public class FmUtils {
    private static final String TAG = "FmUtils";

    /**
     * SPRD: bug490888 Seek station with 50kHz step. Original Android code: // FM station variables
     * public static final int DEFAULT_STATION = 1000; public static final float
     * DEFAULT_STATION_FLOAT = computeFrequency(DEFAULT_STATION); // maximum station frequency
     * private static final int HIGHEST_STATION = 1080; // minimum station frequency private static
     * final int LOWEST_STATION = 875; // station step private static final int STEP = 1; // convert
     * rate private static final int CONVERT_RATE = 10;
     * 
     * @{
     */
    public static final boolean support50ksearch = "1".equals(SystemProperties.get(
            "persist.support.50ksearch", "0"));
    // FM station variables
    public static final int DEFAULT_STATION = support50ksearch ? 10000 : 1000;
    // convert rate
    public static final int CONVERT_RATE = support50ksearch ? 100 : 10;
    public static final float DEFAULT_STATION_FLOAT = computeFrequency(DEFAULT_STATION);
    // maximum station frequency
    private static final int HIGHEST_STATION = support50ksearch ? 10800 : 1080;
    // minimum station frequency
    private static final int LOWEST_STATION = support50ksearch ? 8750 : 875;
    // station step
    private static final int STEP = support50ksearch ? 5 : 1;
    /**
     * @}
     */

    /**
     * SPRD: bug474747, recording path selection.
     * 
     * @{
     */
    public static int FM_RECORD_STORAGE_PATH = 0;
    public static String FM_RECORD_STORAGE_PATH_NAME = "";
    /**
     * @}
     */
    public static final int STORAGE_PATH_INTERNAL_CATEGORY = 0;
    public static final int STORAGE_PATH_EXTERNAL_CATEGORY = 1;
    public static final int STORAGE_PATH_USB_CATEGORY = 2;

    // SPRD: bug568587, Regularly power off.
    public static float GLOBAL_CUSTOM_TIME = 15;
    // minimum storage space for record (512KB).
    // Need to check before starting recording and during recording to avoid
    // recording keeps going but there is no free space in sdcard.
    public static final long LOW_SPACE_THRESHOLD = 512 * 1024;
    // Different city may have different RDS information.
    // We define 100 miles (160934.4m) to distinguish the cities.
    public static final double LOCATION_DISTANCE_EXCEED = 160934.4;
    private static final String FM_LOCATION_LATITUDE = "fm_location_latitude";
    private static final String FM_LOCATION_LONGITUDE = "fm_location_longitude";
    private static final String FM_IS_FIRST_TIME_PLAY = "fm_is_first_time_play";
    private static final String FM_IS_SPEAKER_MODE = "fm_is_speaker_mode";
    private static final String FM_IS_FIRST_ENTER_STATION_LIST = "fm_is_first_enter_station_list";
    private static final String FM_IS_RDS_OPEN = "fm_is_rds_open";
    private static final String FM_IS_AF_OPEN = "fm_is_af_open";
    // StorageManager For FM record
    private static StorageManager sStorageManager = null;

    public static float getHighestFrequency() {
        return computeFrequency(HIGHEST_STATION);
    }

    /**
     * Whether the frequency is valid.
     * 
     * @param station The FM station
     * @return true if the frequency is in the valid scale, otherwise return false
     */
    public static boolean isValidStation(int station) {
        boolean isValid = (station >= LOWEST_STATION && station <= HIGHEST_STATION);
        return isValid;
    }

    /**
     * Compute increase station frequency
     * 
     * @param station The station frequency
     * @return station The frequency after increased
     */
    public static int computeIncreaseStation(int station) {
        int result = station + STEP;
        if (result > HIGHEST_STATION) {
            result = LOWEST_STATION;
        }
        return result;
    }

    /**
     * Compute decrease station frequency
     * 
     * @param station The station frequency
     * @return station The frequency after decreased
     */
    public static int computeDecreaseStation(int station) {
        int result = station - STEP;
        if (result < LOWEST_STATION) {
            result = HIGHEST_STATION;
        }
        return result;
    }

    /**
     * Compute station value with given frequency
     * 
     * @param frequency The station frequency
     * @return station The result value
     */
    public static int computeStation(float frequency) {
        return (int) (frequency * CONVERT_RATE);
    }

    /**
     * Compute frequency value with given station
     * 
     * @param station The station value
     * @return station The frequency
     */
    public static float computeFrequency(int station) {
        return (float) station / CONVERT_RATE;
    }

    /**
     * According station to get frequency string
     * 
     * @param station for 100KZ, range 875-1080
     * @return string like 87.5
     */
    public static String formatStation(int station) {
        float frequency = (float) station / CONVERT_RATE;
        /**
         * SPRD: bug490888 Seek station with 50kHz step. Original Android code: DecimalFormat
         * decimalFormat = new DecimalFormat("0.0");
         * 
         * @{
         */
        DecimalFormat decimalFormat;
        if (support50ksearch) {
            decimalFormat = new DecimalFormat("0.00");
        } else {
            decimalFormat = new DecimalFormat("0.0");
        }
        /**
         * @}
         */
        decimalFormat.setDecimalFormatSymbols(new DecimalFormatSymbols(Locale.US));
        return decimalFormat.format(frequency);
    }

    /**
     * Get the phone storage path
     * 
     * @return The phone storage path
     */
    public static String getDefaultStoragePath() {
        /**
         * SPRD: bug474747, recording path selection. & bug529776 Check the main card state,change
         * get internal storage path method. Original Android code: return
         * Environment.getExternalStorageDirectory().getPath();
         * 
         * @{
         */
        if (FM_RECORD_STORAGE_PATH == 0) {
            Log.i(TAG, "getDefaultStoragePath :phone");
            return EnvironmentEx.getInternalStoragePath().getPath();
        } else if (FM_RECORD_STORAGE_PATH == 1) {
            Log.i(TAG, "getDefaultStoragePath :SDCARD");
            /* SPRD: bug569412 FM show "SD card is missing" when start recording @{ */
            if (!Environment.MEDIA_MOUNTED.equals(EnvironmentEx.getExternalStoragePathState())) {
                String def_storageSD = String.valueOf(mContext.getResources().getString(
                        R.string.storage_sd));
                if (FmUtils.FM_RECORD_STORAGE_PATH_NAME.equals(def_storageSD)) {
                    Log.i(TAG, "SDCARD UNMOUNTED getDefaultStoragePath :phone");
                    FM_RECORD_STORAGE_PATH = 0;
                    return EnvironmentEx.getInternalStoragePath().getPath();
                } else {
                    Log.i(TAG, "no SDCARD exist,getDefaultStoragePath :USB device");
                    return getDefautlUSBDevicePathName();
                }
            } else {
                String path = "";
                File canonicalFile = null;
                try {
                    File externalStorageFile = EnvironmentEx.getExternalStoragePath();
                    if (externalStorageFile != null) {
                        canonicalFile = EnvironmentEx.getExternalStoragePath().getCanonicalFile();
                    }
                    if (canonicalFile != null) {
                        path = canonicalFile.getPath();
                    }
                } catch (IOException e) {
                    Log.e(TAG, "getCanonicalFile error:", e);
                }
                return path;
            }
            /* @} */
        } else {
            Log.i(TAG, "SDCARD exist, getDefaultStoragePath :USB device");
            return getDefautlUSBDevicePathName();
        }
        /**
         * @}
         */
    }

    /**
     * Get the default storage state
     * 
     * @return The default storage state
     */
    public static String getDefaultStorageState(Context context) {
        ensureStorageManager(context);
        String state = sStorageManager.getVolumeState(getDefaultStoragePath());
        return state;
    }

    private static void ensureStorageManager(Context context) {
        if (sStorageManager == null) {
            sStorageManager = (StorageManager) context.getSystemService(Context.STORAGE_SERVICE);
        }
    }

    /**
     * Get the FM play list path
     * 
     * @param context The context
     * @return The FM play list path
     */
    public static String getPlaylistPath(Context context) {
        ensureStorageManager(context);
        String[] externalStoragePaths = sStorageManager.getVolumePaths();
        String path = externalStoragePaths[0] + "/Playlists/";
        return path;
    }

    /**
     * Check if has enough space for record
     * 
     * @param recordingSdcard The recording sdcard path
     * @return true if has enough space for record
     */
    public static boolean hasEnoughSpace(String recordingSdcard) {
        boolean ret = false;
        try {
            StatFs fs = new StatFs(recordingSdcard);
            long blocks = fs.getAvailableBlocks();
            long blockSize = fs.getBlockSize();
            long spaceLeft = blocks * blockSize;
            ret = spaceLeft > LOW_SPACE_THRESHOLD ? true : false;
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "hasEnoughSpace, sdcard may be unmounted:" + recordingSdcard);
        }
        return ret;
    }

    /**
     * Get the latest searched location
     * 
     * @return the list of latitude and longitude
     */
    public static double[] getLastSearchedLocation(Context context) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);

        String strLatitude = prefs.getString(FM_LOCATION_LATITUDE, "0.0");
        String strLongitude = prefs.getString(FM_LOCATION_LONGITUDE, "0.0");
        double latitude = Double.valueOf(strLatitude);
        double longitude = Double.valueOf(strLongitude);
        return new double[] {
                latitude, longitude
        };
    }

    /**
     * Set the last searched location
     */
    public static void setLastSearchedLocation(Context context, double latitude, double longitude) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        SharedPreferences.Editor editor = prefs.edit();
        String strLatitude = Double.valueOf(latitude).toString();
        String strLongitude = Double.valueOf(longitude).toString();
        editor.putString(FM_LOCATION_LATITUDE, strLatitude);
        editor.putString(FM_LOCATION_LONGITUDE, strLongitude);
        editor.commit();
    }

    /**
     * check it is the first time to use Fm
     */
    public static boolean isFirstTimePlayFm(Context context) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        boolean isFirstTime = prefs.getBoolean(FM_IS_FIRST_TIME_PLAY, true);
        return isFirstTime;
    }

    /**
     * Called when first time play FM.
     * 
     * @param context The context
     */
    public static void setIsFirstTimePlayFm(Context context) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        SharedPreferences.Editor editor = prefs.edit();
        editor.putBoolean(FM_IS_FIRST_TIME_PLAY, false);
        editor.commit();
    }

    /**
     * check it is the first time enter into station list page
     */
    public static boolean isFirstEnterStationList(Context context) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        boolean isFirstEnter = prefs.getBoolean(FM_IS_FIRST_ENTER_STATION_LIST, true);
        if (isFirstEnter) {
            SharedPreferences.Editor editor = prefs.edit();
            editor.putBoolean(FM_IS_FIRST_ENTER_STATION_LIST, false);
            editor.commit();
        }
        return isFirstEnter;
    }

    /**
     * Create the notification large icon bitmap from layout
     * 
     * @param c The context
     * @param text The frequency text
     * @return The large icon bitmap with frequency text
     */
    public static Bitmap createNotificationLargeIcon(Context c, String text) {
        Resources res = c.getResources();
        int width = (int) res.getDimension(android.R.dimen.notification_large_icon_width);
        int height = (int) res.getDimension(android.R.dimen.notification_large_icon_height);
        LinearLayout iconLayout = new LinearLayout(c);
        iconLayout.setOrientation(LinearLayout.VERTICAL);
        iconLayout.setBackgroundColor(c.getResources().getColor(R.color.theme_primary_color));
        iconLayout.setDrawingCacheEnabled(true);
        iconLayout.layout(0, 0, width, height);
        TextView iconText = new TextView(c);
        /**
         * SPRD: bug523048, textsize is too large. Original Android code:
         * iconText.setTextSize(24.0f);
         * 
         * @{
         */
        iconText.setTextSize(TypedValue.COMPLEX_UNIT_DIP, 24.0f);
        /**
         * @}
         */
        iconText.setTextColor(res.getColor(R.color.theme_title_color));
        iconText.setText(text);
        iconText.measure(MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED),
                MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
        int left = (int) ((width - iconText.getMeasuredWidth()) * 0.5);
        int top = (int) ((height - iconText.getMeasuredHeight()) * 0.5);
        iconText.layout(left, top, iconText.getMeasuredWidth() + left,
                iconText.getMeasuredHeight() + top);
        iconLayout.addView(iconText);
        iconLayout.layout(0, 0, width, height);

        iconLayout.buildDrawingCache();
        Bitmap largeIcon = Bitmap.createBitmap(iconLayout.getDrawingCache());
        iconLayout.destroyDrawingCache();
        return largeIcon;
    }

    /**
     * Get whether speaker mode is in use when audio focus lost.
     * 
     * @param context the Context
     * @return true for speaker mode, false for non speaker mode
     */
    public static boolean getIsSpeakerModeOnFocusLost(Context context) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);

        return prefs.getBoolean(FM_IS_SPEAKER_MODE, false);
    }

    /**
     * Set whether speaker mode is in use.
     * 
     * @param context the Context
     * @param isSpeaker speaker state
     */
    public static void setIsSpeakerModeOnFocusLost(Context context, boolean isSpeaker) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        SharedPreferences.Editor editor = prefs.edit();
        editor.putBoolean(FM_IS_SPEAKER_MODE, isSpeaker);
        editor.commit();
    }

    /**
     * SPRD: bug495859, request runtime permissions
     */
    public static final int FM_PERMISSIONS_REQUEST_CODE = 2016;

    public static boolean hasLocationPermission(Context context) {
        if (context.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) == PackageManager.PERMISSION_GRANTED
                &&
                context.checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED) {
            return true;
        }
        return false;
    }

    public static boolean checkBuildRecordingPermission(Context context) {
        if (context.checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED
                &&
                context.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
            return true;
        }
        int numPermissionsToRequest = 0;
        boolean requestMicrophonePermission = false;
        boolean requestStoragePermission = false;

        if (context.checkSelfPermission(Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            requestMicrophonePermission = true;
            numPermissionsToRequest++;
        }

        if (context.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestStoragePermission = true;
            numPermissionsToRequest++;
        }
        String[] permissionsToRequest = new String[numPermissionsToRequest];
        int permissionsRequestIndex = 0;
        if (requestMicrophonePermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.RECORD_AUDIO;
            permissionsRequestIndex++;
        }
        if (requestStoragePermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.WRITE_EXTERNAL_STORAGE;
            permissionsRequestIndex++;
        }
        ((FmRecordActivity) context).requestPermissions(permissionsToRequest,
                FM_PERMISSIONS_REQUEST_CODE);
        return false;
    }

    public static boolean hasStoragePermission(Context context) {
        if (context.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
            return true;
        }
        return false;
    }

    public static boolean isRDSOpen(Context context) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        boolean isRDSOpen = prefs.getBoolean(FM_IS_RDS_OPEN, false);
        return isRDSOpen;
    }

    public static boolean isAFOpen(Context context) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        boolean isAFOpen = prefs.getBoolean(FM_IS_AF_OPEN, false);
        return isAFOpen;
    }

    public static void setRDSState(Context context, boolean state) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        prefs.edit().putBoolean(FM_IS_RDS_OPEN, state).commit();
    }

    public static void setAFState(Context context, boolean state) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        prefs.edit().putBoolean(FM_IS_AF_OPEN, state).commit();
    }

    /**
     * SPRD: change actionMode statusBar color
     */
    public static void updateStatusBarColor(Activity activity, boolean actionMode) {
        if (actionMode) {
            int cabStatusBarColor = activity.getResources().getColor(
                    R.color.action_mode_statusbar_color);
            activity.getWindow().setStatusBarColor(cabStatusBarColor);
        } else {
            int cabStatusBarColor = activity.getResources().getColor(
                    R.color.theme_primarydark_color);
            activity.getWindow().setStatusBarColor(cabStatusBarColor);
        }
    }

    /* SPRD:add feature for support OTG. @{ */
    public static Context mContext = null;

    private static String getDefautlUSBDevicePathName() {
        StorageManager storageManager = mContext.getSystemService(StorageManager.class);
        List<VolumeInfo> volumes = storageManager.getVolumes();
        Collections.sort(volumes, VolumeInfo.getDescriptionComparator());
        VolumeInfo myVol = null;
        for (VolumeInfo vol : volumes) {
            File file = vol.getPath();
            if (Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState(file))) {
                String usbDeviceName = storageManager.getBestVolumeDescription(vol);
                if (usbDeviceName.equals(FmUtils.FM_RECORD_STORAGE_PATH_NAME)) {
                    myVol = vol;
                    break;
                }
            }
        }

        if (myVol != null && myVol.isMountedReadable()) {
            final File path = myVol.getPath();
            return path.getPath();
        }
        return "";
    }

    /* @} */
    /**
     * temp porting code for AndroidN: Replace Environment.getInternalStoragePath().getPath()
     */
    public static String getInternalStoragePath() {
        String path = "/storage/emulated/0";
        return path;
    }

    /* SPRD: Fix for bug 589228 forbid turn on FM in airplane mode. @{ */
    public static boolean isAirplane(Context context) {
        boolean isair = Settings.System.getInt(context.getContentResolver(), Settings.System.AIRPLANE_MODE_ON, 0) ==1;
        Log.d(TAG,"isAirplaneMode: "+isair );
        return  isair;
    }
    /* Bug 589228 End@} */

    /* SPRD: Fix for bug 596494 Remove and reconnect the OTG storage, the recording file save path still display OTG storage. @{ */
    public static void checkAndResetStoragePath(Context context,String removedPath) {
        String FM_RECORD_DEFAULT_PATH = "default_path";
        String FM_RECORD_DEFAULT_PATH_NAME = "default_path_name";
        String recordInternalStorage = String.valueOf(context.getResources().getString(R.string.storage_phone));
        String recordExternalStorage = String.valueOf(context.getResources().getString(R.string.storage_sd));
        //get saved default storage path informations
        SharedPreferences storageSP = context.getSharedPreferences("fm_record_storage",Context.MODE_PRIVATE);
        SharedPreferences.Editor storageEdit = storageSP.edit();
        int path = storageSP.getInt(FM_RECORD_DEFAULT_PATH, STORAGE_PATH_INTERNAL_CATEGORY);
        String pathName = storageSP.getString(FM_RECORD_DEFAULT_PATH_NAME, recordInternalStorage);
        switch (path) {
        //external storage
        case STORAGE_PATH_EXTERNAL_CATEGORY:
            if (!Environment.MEDIA_MOUNTED.equals(EnvironmentEx.getExternalStoragePathState())) {
                FM_RECORD_STORAGE_PATH = STORAGE_PATH_INTERNAL_CATEGORY;
                FM_RECORD_STORAGE_PATH_NAME = recordInternalStorage;
                storageEdit.putInt(FM_RECORD_DEFAULT_PATH, FM_RECORD_STORAGE_PATH);
                storageEdit.putString(FM_RECORD_DEFAULT_PATH_NAME, recordInternalStorage);
                storageEdit.commit();
            }
            break;
        //usb devices
        case STORAGE_PATH_USB_CATEGORY:
            StorageManager storageManager = mContext.getSystemService(StorageManager.class);
            List<VolumeInfo> volumes = storageManager.getVolumes();
            Collections.sort(volumes, VolumeInfo.getDescriptionComparator());
            VolumeInfo myVol = null;
            boolean isDefaultExist = false;
            for (VolumeInfo vol : volumes) {
                File file = vol.getPath();
                if (Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState(file))) {
                    String usbDeviceName = storageManager.getBestVolumeDescription(vol);
                    if (usbDeviceName.equals(pathName)) {
                        isDefaultExist = true;
                        break;
                    }
                }
            }
            //the default usb device has been removed
            if (!isDefaultExist) {
                if (!Environment.MEDIA_MOUNTED.equals(EnvironmentEx.getExternalStoragePathState())) {
                    FM_RECORD_STORAGE_PATH = STORAGE_PATH_INTERNAL_CATEGORY;
                    FM_RECORD_STORAGE_PATH_NAME = recordInternalStorage;
                    storageEdit.putInt(FM_RECORD_DEFAULT_PATH, FM_RECORD_STORAGE_PATH);
                    storageEdit.putString(FM_RECORD_DEFAULT_PATH_NAME, recordInternalStorage);
                }else{
                    FM_RECORD_STORAGE_PATH = STORAGE_PATH_EXTERNAL_CATEGORY;
                    FM_RECORD_STORAGE_PATH_NAME = recordExternalStorage;
                    storageEdit.putInt(FM_RECORD_DEFAULT_PATH, FM_RECORD_STORAGE_PATH);
                    storageEdit.putString(FM_RECORD_DEFAULT_PATH_NAME, recordExternalStorage);
                }
                storageEdit.commit();
            }
            break;
        default:
            break;
        }
        return;
    }
    /* Bug 596494 End@} */
}
