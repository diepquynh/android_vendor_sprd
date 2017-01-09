/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */

/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.ucamera.ugallery;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.location.Geocoder;
import android.media.ExifInterface;
import android.net.Uri;
import android.os.Build;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.ucamera.ugallery.gallery.IImage;
import com.ucamera.ugallery.util.Models;

import java.io.Closeable;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.List;

/**
 * A utility class to handle various kinds of menu operations.
 */
public class MenuHelper {
    private static final String TAG = "MenuHelper";
    public static final String EMPTY_STRING = "";
    public static final String JPEG_MIME_TYPE = "image/jpeg";
    public static final float INVALID_LATLNG = 255f;
    public static final int NO_STORAGE_ERROR = -1;
    public static final int CANNOT_STAT_ERROR = -2;

    /**
     * Activity result code used to report crop results.
     */
    public static final int RESULT_COMMON_MENU_CROP = 490;

    public static boolean hasLatLngData(IImage image) {
        ExifInterface exif = getExif(image);
        if (exif == null) {
            return false;
        }
        float latlng[] = new float[2];
        return exif.getLatLong(latlng);
    }

    private static void setDetailsValue(View d, String text, int valueId) {
        ((TextView) d.findViewById(valueId)).setText(text);
    }

    private static void hideDetailsRow(View d, int rowId) {
        d.findViewById(rowId).setVisibility(View.GONE);
    }

    private static class UpdateLocationCallback implements ReverseGeocoderTask.Callback {
        WeakReference<View> mView;

        public UpdateLocationCallback(WeakReference<View> view) {
            mView = view;
        }

        public void onComplete(String location) {
            // View d is per-thread data, so when setDetailsValue is
            // executed by UI thread, it doesn't matter whether the
            // details dialog is dismissed or not.
            View view = mView.get();
            if (view == null) {
                return;
            }
            if (!location.equals(MenuHelper.EMPTY_STRING)) {
                MenuHelper.setDetailsValue(view, location, R.id.details_location_value);
            } else {
                MenuHelper.hideDetailsRow(view, R.id.details_location_row);
            }
        }
    }

    private static void setLatLngDetails(final View d, Activity context, ExifInterface exif) {
//        float[] latlng = new float[2];
//        boolean hasLatLong = exif.getLatLong(latlng);
//        String latitude = exif.getAttribute(ExifInterface.TAG_GPS_LATITUDE);
//        String longitude = exif.getAttribute(ExifInterface.TAG_GPS_LONGITUDE);
//        if (hasLatLong || (!TextUtils.isEmpty(latitude) && !TextUtils.isEmpty(longitude))) {
//            if(!hasLatLong) {
//                latlng[0] = (float)ConvertGPS.convertToDecimal(latitude);
//                latlng[1] = (float)ConvertGPS.convertToDecimal(longitude);
//            }
//            setDetailsValue(d, String.valueOf(latlng[0]), R.id.details_latitude_value);
//            setDetailsValue(d, String.valueOf(latlng[1]), R.id.details_longitude_value);
//
//            if (latlng[0] == INVALID_LATLNG || latlng[1] == INVALID_LATLNG) {
//                hideDetailsRow(d, R.id.details_latitude_row);
//                hideDetailsRow(d, R.id.details_longitude_row);
//                hideDetailsRow(d, R.id.details_location_row);
//                return;
//            }
//
//            UpdateLocationCallback cb = new UpdateLocationCallback(new WeakReference<View>(d));
//            Geocoder geocoder = new Geocoder(context);
//            new ReverseGeocoderTask(geocoder, latlng, cb).execute();
//        } else {
//            hideDetailsRow(d, R.id.details_latitude_row);
//            hideDetailsRow(d, R.id.details_longitude_row);
//            hideDetailsRow(d, R.id.details_location_row);
//        }
    }

    /**
     * public static HashMap<String, String> getExifData(IImage image) { if
     * (!JPEG_MIME_TYPE.equals(image.getMimeType())) { return null; } return
     * ExifInterface.loadExifData(image.getDataPath()); }
     *
     * @param image image
     * @return ExifInterface
     */
    public static ExifInterface getExif(IImage image) {
        if (!JPEG_MIME_TYPE.equals(image.getMimeType())) {
            return null;
        }

        try {
            return new ExifInterface(image.getDataPath());
        } catch (IOException ex) {
            Log.e(TAG, "cannot read exif", ex);
            return null;
        }
    }

    /**
     * @param d d
     */
    public static void hideExifInformation(View d) {
        hideDetailsRow(d, R.id.details_make_row);
        hideDetailsRow(d, R.id.details_model_row);
        hideDetailsRow(d, R.id.details_whitebalance_row);
        hideDetailsRow(d, R.id.details_latitude_row);
        hideDetailsRow(d, R.id.details_longitude_row);
        hideDetailsRow(d, R.id.details_location_row);
        hideDetailsRow(d, R.id.details_focal_length_row);
        hideDetailsRow(d, R.id.details_aperture_row);
    }

    /**
     * @param image image
     * @param d d
     * @param activity activity
     */
    public static void showExifInformation(IImage image, View d, Activity activity) {
        ExifInterface exif = getExif(image);
        if (exif == null) {
            hideExifInformation(d);
            return;
        }

        String value = exif.getAttribute(ExifInterface.TAG_MAKE);
        if (TextUtils.isEmpty(value) || value.trim().length() == 0) {
            value = Build.MANUFACTURER;
        }
        if (!TextUtils.isEmpty(value)) {
            setDetailsValue(d, value, R.id.details_make_value);
        } else {
            hideDetailsRow(d, R.id.details_make_row);
        }

        value = exif.getAttribute(ExifInterface.TAG_MODEL);
        if (TextUtils.isEmpty(value) || value.trim().length() == 0 ) {
            value = Build.MODEL;
        }
        if (!TextUtils.isEmpty(value)) {
            setDetailsValue(d, value, R.id.details_model_value);
        } else {
            hideDetailsRow(d, R.id.details_model_row);
        }

        value = getWhiteBalanceString(exif, activity);
        if (!TextUtils.isEmpty(value)) {
            setDetailsValue(d, value, R.id.details_whitebalance_value);
        } else {
            hideDetailsRow(d, R.id.details_whitebalance_row);
        }

        Double focalLength = exif.getAttributeDouble(ExifInterface.TAG_FOCAL_LENGTH, -1);
        if (focalLength > 0) {
            setDetailsValue(d, String.valueOf(focalLength) + "MM", R.id.details_focal_length_value);
        } else {
            hideDetailsRow(d, R.id.details_focal_length_row);
        }
        value = exif.getAttribute(ExifInterface.TAG_APERTURE);
        if (!TextUtils.isEmpty(value)) {
            if(Models.isLaJiaoPepper()) {
                value = "2.2";
            }
            setDetailsValue(d, "F " + value, R.id.details_aperture_value);
        } else {
            hideDetailsRow(d, R.id.details_aperture_row);
        }
        setLatLngDetails(d, activity, exif);
    }

    /**
     * Returns a human-readable string describing the white balance value.
     * Returns empty string if there is no white balance value or it is not
     * recognized.
     *
     * @param exif exif
     * @return String
     */
    private static String getWhiteBalanceString(ExifInterface exif, Activity act) {
        int whitebalance = exif.getAttributeInt(ExifInterface.TAG_WHITE_BALANCE, -1);
        if (whitebalance == -1) {
            return "";
        }

        switch (whitebalance) {
            case ExifInterface.WHITEBALANCE_AUTO:
                return act.getString(R.string.auto);
            case ExifInterface.WHITEBALANCE_MANUAL:
                return act.getString(R.string.manual);
            default:
                return "";
        }
    }

    static void deleteImage(Activity activity, Runnable onDelete, int deleteCount) {
        String title = activity.getString(R.string.text_delete_image_title);
        String message = deleteCount > 1 ? activity.getString(R.string.text_delete_multi_message)
                : activity.getString(R.string.text_delete_single_message);
        confirmAction(activity, title, message, onDelete);
    }

    static void deleteVideo(Activity activity, Runnable onDelete, int deleteCount) {
        String title = activity.getString(R.string.text_delete_video_title);
        String message = deleteCount > 1 ? activity.getString(R.string.text_delete_multi_video_message)
                : activity.getString(R.string.text_delete_single_video_message);
        confirmAction(activity, title, message, onDelete);
    }

    static void lockImage(Activity activity, Runnable onDelete) {
        String title = activity.getString(R.string.text_lock_image_title);
        String message = activity.getString(R.string.text_lock_image_message);
        confirmAction(activity, title, message, onDelete);
    }

    static void unLockImage(Activity activity, Runnable onDelete) {
        String title = activity.getString(R.string.text_unlock_image_title);
        String message = activity.getString(R.string.text_unlock_image_message);
        confirmAction(activity, title, message, onDelete);
    }
    /**
     * @param context context
     * @param title title
     * @param message message
     * @param action action
     */
    public static void confirmAction(Context context, String title, String message, final Runnable action) {
        OnClickListener listener = new OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                switch (which) {
                    case DialogInterface.BUTTON_POSITIVE:
                        if (action != null) {
                            action.run();
                        }
                        break;
                    default:
                        break;
                }
            }
        };
        Dialog dialog = new AlertDialog.Builder(context)
            .setIcon(android.R.drawable.ic_dialog_alert)
            .setTitle(title)
            .setMessage(message)
            .setCancelable(false)
            .setPositiveButton(R.string.picture_delete_ok, listener)
            .setNegativeButton(R.string.picture_delete_cancel, listener)
            .create();
        dialog.show();
    }

    public static String formatDuration(final Context context,
            int durationSec) {
        int duration = durationSec / 1000;
        int h = duration / 3600;
        int m = (duration - h * 3600) / 60;
        int s = duration - (h * 3600 + m * 60);
        String durationValue;
        if (h == 0) {
            durationValue = String.format(
                    context.getString(R.string.details_ms), m, s);
        } else {
            durationValue = String.format(
                    context.getString(R.string.details_hms), h, m, s);
        }
        return durationValue;
    }

    public static long getImageFileSize(IImage image) {
        java.io.InputStream data = image.fullSizeImageData();
        if (data == null) return -1;
        try {
            return data.available();
        } catch (java.io.IOException ex) {
            return -1;
        } finally {
            closeSilently(data);
        }
    }

    public static void closeSilently(Closeable c) {
        if (c != null) {
            try {
                c.close();
            } catch (Throwable e) {
                // ignore
            }
        }
    }

    // This is a hack before we find a solution to pass a permission to other
    // applications. See bug #1735149, #1836138.
    // Checks if the URI is on our whitelist:
    // content://media/... (MediaProvider)
    // file:///sdcard/... (Browser download)
    public static boolean isWhiteListUri(Uri uri) {
        if (uri == null) return false;

        String scheme = uri.getScheme();
        String authority = uri.getAuthority();

        if (scheme.equals("content") && authority.equals("media")) {
            return true;
        }

        if (scheme.equals("file")) {
            List<String> p = uri.getPathSegments();

            if (p.size() >= 1 && p.get(0).equals("sdcard")) {
                return true;
            }
        }

        return false;
    }

}
