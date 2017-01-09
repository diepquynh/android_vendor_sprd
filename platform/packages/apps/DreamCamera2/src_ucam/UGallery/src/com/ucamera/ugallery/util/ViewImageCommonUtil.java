/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.ucamera.ugallery.util;

import java.io.IOException;
import java.lang.reflect.Field;
import java.text.SimpleDateFormat;
import java.util.Date;

import com.ucamera.ugallery.MenuHelper;
import com.ucamera.ugallery.gallery.IImage;
import com.ucamera.ugallery.gallery.VideoObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.media.ExifInterface;
import android.text.format.DateFormat;
import android.text.format.Formatter;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import com.ucamera.ugallery.R;

public class ViewImageCommonUtil {





    private static final String TAG = "ViewImageCommonUtil";

    public static void showImageDetails(Activity activity , IImage image) {
        if (image == null) {
            return;
        }
        final View d = View.inflate(activity, R.layout.detailsview, null);
        ((TextView) d.findViewById(R.id.details_image_title)).setText(image.getTitle());

        long length = getImageFileSize(image);
        String lengthString = length < 0 ? MenuHelper.EMPTY_STRING : Formatter.formatFileSize(activity, length);
        ((TextView) d.findViewById(R.id.details_file_size_value)).setText(lengthString);

        int dimensionWidth = image.getWidth();
        int dimensionHeight = image.getHeight();
        d.findViewById(R.id.details_frame_rate_row).setVisibility(View.GONE);
        d.findViewById(R.id.details_bit_rate_row).setVisibility(View.GONE);
        d.findViewById(R.id.details_codec_row).setVisibility(View.GONE);
        if(ImageManager.isVideo(image)) {
            int duration = ((VideoObject)image).getDuration();
            String dur = MenuHelper.formatDuration(activity,duration);
            String mimeType = image.getMimeType();
            if (dur != null) {
                ((TextView) d.findViewById(R.id.details_duration_value)).setText(dur);
            } else {
                d.findViewById(R.id.details_duration_row).setVisibility(View.GONE);
            }
            if( mimeType != null) {
                ((TextView) d.findViewById(R.id.details_format_value)).setText(mimeType);
            } else {
                d.findViewById(R.id.details_format_row).setVisibility(View.GONE);
            }
        } else {
            d.findViewById(R.id.details_duration_row).setVisibility(View.GONE);
            d.findViewById(R.id.details_format_row).setVisibility(View.GONE);
        }
        String value = null;
        if (dimensionWidth > 0 && dimensionHeight > 0) {
            value = String.format(activity.getString(R.string.text_details_dimension_x), dimensionWidth, dimensionHeight);
        }
        if (value != null) {
            ((TextView) d.findViewById(R.id.details_resolution_value)).setText(value);
        } else {
            d.findViewById(R.id.details_resolution_row).setVisibility(View.GONE);
        }

        value = getImageDateTime(activity, image, false);
        if (value != MenuHelper.EMPTY_STRING) {
            ((TextView) d.findViewById(R.id.details_date_taken_value)).setText(value);
        } else {
            d.findViewById(R.id.details_date_taken_row).setVisibility(View.GONE);
        }

        // Show more EXIF header details for JPEG images.
        if (MenuHelper.JPEG_MIME_TYPE.equals(image.getMimeType())) {
            MenuHelper.showExifInformation(image, d, activity);
        } else {
            MenuHelper.hideExifInformation(d);
        }

        TextView tv_format = (TextView) d.findViewById(R.id.details_format_value);
        String imageType = image.getMimeType();
        if (imageType != null && imageType.startsWith("image/")) {
            tv_format.setText(imageType.substring(imageType.indexOf("/") + 1, imageType.length())
                    .toUpperCase());
        }

        final AlertDialog.Builder builder = new AlertDialog.Builder(activity);
        builder.setIcon(android.R.drawable.ic_dialog_info)
               .setTitle(R.string.text_image_details)
               .setView(d)
               .setNeutralButton(R.string.details_ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
               }).show();
    }

    public static long getImageFileSize(IImage image) {
        java.io.InputStream data = image.fullSizeImageData();
        if (data == null) {
            return -1;
        }
        try {
            return data.available();
        } catch (java.io.IOException ex) {
            return -1;
        } finally {
            try {
                data.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public static void setDialogDissmiss(DialogInterface dialog,boolean close) {
        try {
            Field field = dialog.getClass().getSuperclass().getDeclaredField("mShowing");
            field.setAccessible(true);
            field.set(dialog, close);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static String getImageDateTime(Activity activity, IImage image , boolean isOnlyDate) {
        String dateTime = MenuHelper.EMPTY_STRING;
        ExifInterface exif = MenuHelper.getExif(image);
        /*
         * BUG FIX: 5858
         * FIX COMMENT: Maybe it exists the third format in ExifInterface.TAG_DATETIME attribute. so we try catch it.
         * DATE: 2014-01-21
         */
        if (exif != null && exif.getAttribute(ExifInterface.TAG_DATETIME) != null) {
            try {
                dateTime = exif.getAttribute(ExifInterface.TAG_DATETIME);
                if(dateTime != null && dateTime.contains(":") && dateTime.contains(" ")) {
                    String[] ss = dateTime.split(" ");
                    if (ss[0] != null) {
                        if(isOnlyDate) {
                            dateTime = ss[0].replace(":", "-");
                        } else {
                            dateTime = ss[0].replace(":", "-") + " " + ss[1];
                        }
                    }
                } else {
                    dateTime = formatDate(activity, Long.valueOf(dateTime), isOnlyDate);
                }
            } catch (Exception e) {
                dateTime = MenuHelper.EMPTY_STRING;
            }
        }

        if (MenuHelper.EMPTY_STRING.equals(dateTime)) {
            long dateTaken = image.getDateTaken();
            if (dateTaken != 0) {
                dateTime = formatDate(activity, dateTaken, isOnlyDate);
            }
        }
        Log.d(TAG, "getImageDateTime() dateTime:" + dateTime);
        return dateTime;
    }

    private static String formatDate(Activity activity, long time,
            boolean isOnlyDate) {
        boolean is24Hour = DateFormat.is24HourFormat(activity);
        Date date = new Date(time);
        SimpleDateFormat dateFormat = null;
        if (isOnlyDate) {
            dateFormat = new SimpleDateFormat("yyyy-MM-dd") ;
        } else {
            dateFormat = is24Hour ? new SimpleDateFormat(
                    "yyyy-MM-dd HH:mm:ss") : new SimpleDateFormat(
                            "yyyy-MM-dd h:mm:ss a");
        }
        return dateFormat.format(date);
    }
}
