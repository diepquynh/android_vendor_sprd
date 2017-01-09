/*
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 *
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

package com.ucamera.ucam.utils;

//import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.PendingIntent;
import android.content.ActivityNotFoundException;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.hardware.Camera.Parameters;
import android.location.Location;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Build;
import android.os.ParcelFileDescriptor;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.TypedValue;
import android.view.Display;
import android.view.OrientationEventListener;
import android.view.View;
import android.view.WindowManager;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.animation.LinearInterpolator;
import android.view.animation.ScaleAnimation;
import android.view.animation.TranslateAnimation;
import android.widget.Toast;

import com.ucamera.uphoto.R;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.util.List;

/**
 * Collection of utility functions used in this package.
 */
public abstract class UiUtils {
    private static final String TAG = "Util";

    public static final String REVIEW_ACTION        = "com.android.camera.action.REVIEW";
    // See android.hardware.Camera.ACTION_NEW_PICTURE.
    public static final String ACTION_NEW_PICTURE   = "android.hardware.action.NEW_PICTURE";
    // See android.hardware.Camera.ACTION_NEW_VIDEO.
    public static final String ACTION_NEW_VIDEO     = "android.hardware.action.NEW_VIDEO";

    private static float sPixelDensity = 1;
    private static int   sPixelHeight  = -1;
    private static int   sPixelWidth   = -1;
    private static int   sDensityDpi   = 1;
    private static int   sMenuItemWidth = -1;
    private static int   sMenuItemHeight = -1;
    private static int   sEffectItemWidht = -1;
    private static int   sEffectItemHeight = -1;
    private static int   sSettingsHeight   = -1;
    private static int sShutterBarHeight = -1;
    private static boolean sHighMemo = false;

    public static final int DIRECTION_UP = 0;
    public static final int DIRECTION_DOWN = 1;
    public static final int DIRECTION_LEFT = 2;
    public static final int DIRECTION_RIGHT = 3;

    private UiUtils() {}

    public static void initialize(Context context) {
        com.ucamera.uphoto.UiUtils.initialize(context);
        DisplayMetrics metrics = new DisplayMetrics();
        WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        wm.getDefaultDisplay().getMetrics(metrics);
        sPixelDensity = metrics.density;
        sPixelHeight  = metrics.heightPixels;
        sPixelWidth   = metrics.widthPixels;
        sDensityDpi = metrics.densityDpi;
        int[] itemParam;
//        if(com.ucamera.ucam.variant.Build.isHosin()) {
//            itemParam = context.getResources().getIntArray(R.array.gallery_img_layout_params_hosin);
//        }else if (Models.AMAZON_KFTT.equals(Models.getModel())) {
//                itemParam = context.getResources().getIntArray(
//                        R.array.gallery_img_layout_params_kindle);
//        }else {
//               itemParam = context.getResources().getIntArray(R.array.gallery_img_layout_params);
//        }
//        sMenuItemWidth = Math.round(itemParam[0] * (sPixelDensity < 1.0f ? 1.2f : sPixelDensity));
//        sMenuItemHeight = Math.round(itemParam[1] * (sPixelDensity < 1.0f ? 1.2f : sPixelDensity));
        Drawable drawable = context.getResources().getDrawable(R.drawable.magiclens_menu_category_item_bk);
        sEffectItemWidht = drawable.getIntrinsicWidth();
        sEffectItemHeight = drawable.getIntrinsicHeight();

        sSettingsHeight = (int) (213 * sPixelDensity);

//        sShutterBarHeight = (int) (context.getResources().getDrawable(R.drawable.basebar_bg)
//                .getIntrinsicHeight());
//        int totalMem = getTotalMemory(context);
//        LogUtils.debug(TAG, "init(): totalMem is " + totalMem + "MB");
//        if(totalMem >= 512) {
//            sHighMemo = true;
//        }
        LogUtils.debug(TAG, "screen size : "+sPixelWidth+"x"+sPixelHeight+",density :"+sPixelDensity+",sDensityDpi : "+sDensityDpi
                + ", sHighMemo is " + sHighMemo + ", sShutterBarHeight is " + sShutterBarHeight + ", sMenuItem.WH is (" + sMenuItemWidth
                + ", " + sMenuItemHeight + "), sEffectItem.WH is (" + sEffectItemWidht + ", " + sEffectItemHeight + "), sSettingsHeight is "
                + sSettingsHeight);
    }

    public static int dpToPixel(int dp) {
        return Math.round(sPixelDensity * dp);
    }

    public static final int screenWidth() { return sPixelWidth;}
    public static final int screenHeight(){ return sPixelHeight;}
    public static final float screenDensity(){ return sPixelDensity;}
    public static final int screenDensityDPI() {return sDensityDpi;}
//    public static final int menuItemWidth() {return sMenuItemWidth;}
//    public static final int menuItemHeight() {return sMenuItemHeight;}
    public static final int effectItemWidth() {return sEffectItemWidht;}
    public static final int effectItemHeight() {return sEffectItemHeight;}
    public static final int settingsHeight()   {return sSettingsHeight;}
    public static final boolean highMemo() {return sHighMemo; }
    public static final int shutterBarHeight() {return 0 /*sShutterBarHeight*/;}

    public static int[] getRelativeLocation(View reference, View view) {
        int loc[] = new int[2];
        reference.getLocationInWindow(loc);
        int referenceX = loc[0];
        int referenceY = loc[1];
        view.getLocationInWindow(loc);
        loc[0] -= referenceX;
        loc[1] -= referenceY;
        return loc;
    }

//    public static boolean pointInView(float x, float y, View v, int orientation) {
//
//      int loc[] = new int[2];
//      v.getLocationInWindow(loc);
//      Rect rc = null;
//      if(ApiHelper.HAS_VIEW_ROTATION_METHOD
//              && ((com.ucamera.ucam.variant.Build.MODULE_MENU_GRID.isOff() && com.ucamera.ucam.variant.Build.MODULE_MENU_GALLERY.isOff()) || v.getId() == R.id.top_settings)) {
//          switch (orientation) {
//              case 0:
//                  rc = new Rect(loc[0], loc[1], loc[0] + v.getWidth(), loc[1] + v.getHeight());
//                  break;
//              case 90:
//                  rc = new Rect(loc[0], loc[1] - v.getWidth(), loc[0] + v.getHeight(), loc[1]);
//                  break;
//              case 180:
//                  rc = new Rect(loc[0] - v.getWidth(), loc[1] - v.getHeight(), loc[0], loc[1]);
//                  break;
//              case 270:
//                  rc = new Rect(loc[0] - v.getHeight(), loc[1], loc[0], loc[1] + v.getWidth());
//                  break;
//              default: // orientation may equal OrientationEventListener.ORIENTATION_UNKNOWN(-1)
//                  rc = new Rect(loc[0], loc[1], loc[0] + v.getWidth(), loc[1] + v.getHeight());
//          }
//      } else {
//          rc = new Rect(loc[0], loc[1], loc[0] + v.getWidth(), loc[1] + v.getHeight());
//      }
//
//      return rc.contains((int)x, (int)y);
//  }
//
//    public static boolean pointOutView(float x, float y, int orientation) {
//        boolean canDismissMenu = false;
//        if(ApiHelper.HAS_VIEW_ROTATION_METHOD) {
//            switch (orientation) {
//                case 0:
//                case 180:
//                    canDismissMenu = (x > UiUtils.screenWidth() * calculMenuWidthRate())
//                            || (y > UiUtils.screenHeight() - UiUtils.shutterBarHeight());
//                    break;
//                case 90:
//                case 270:
//                    canDismissMenu = (y > UiUtils.screenWidth() * calculMenuWidthRate());
//                    break;
//                case OrientationEventListener.ORIENTATION_UNKNOWN:
//                    canDismissMenu = (x > UiUtils.screenWidth() * calculMenuWidthRate())
//                            || (y > UiUtils.screenHeight() - UiUtils.shutterBarHeight());
//                    break;
//            }
//        } else {
//            canDismissMenu = ((x > UiUtils.screenWidth() * calculMenuWidthRate())
//                    || (y > UiUtils.screenHeight() - UiUtils.shutterBarHeight()));
//        }
//
//        return canDismissMenu;
//    }

    public static void fadeIn(View view, float startAlpha, float endAlpha, long duration) {
        if (view.getVisibility() == View.VISIBLE) return;

        view.setVisibility(View.VISIBLE);
        Animation animation = new AlphaAnimation(startAlpha, endAlpha);
        animation.setDuration(duration);
        view.startAnimation(animation);
    }

    public static void fadeIn(View view) {
        fadeIn(view, 0F, 1F, 400);
        // We disabled the button in fadeOut(), so enable it here.
        view.setEnabled(true);
    }

    public static void fadeOut(View view) {
        if (view.getVisibility() != View.VISIBLE) return;

        // Since the button is still clickable before fade-out animation
        // ends, we disable the button first to block click.
        view.setEnabled(false);
        Animation animation = new AlphaAnimation(1F, 0F);
        animation.setDuration(400);
        view.startAnimation(animation);
        view.setVisibility(View.GONE);
    }

//    @SuppressWarnings("deprecation")
//    //@TargetApi(Build.VERSION_CODES.HONEYCOMB_MR2)
//    private static Point getDefaultDisplaySize(Activity activity, Point size) {
//        Display d = activity.getWindowManager().getDefaultDisplay();
//        if (Build.VERSION.SDK_INT >= ApiHelper.VERSION_CODES.HONEYCOMB_MR2) {
//            d.getSize(size);
//        } else {
//            size.set(d.getWidth(), d.getHeight());
//        }
//        return size;
//    }

//    public static ResolutionSize getOptimalPreviewSize(Activity activity,
//            List<ResolutionSize> sizes, double targetRatio) {
//        // Use a very small tolerance because we want an exact match.
//        final double ASPECT_TOLERANCE = 0.001;
//        if (sizes == null) return null;
//
//        ResolutionSize optimalSize = null;
//        double minDiff = Double.MAX_VALUE;
//
//        // Because of bugs of overlay and layout, we sometimes will try to
//        // layout the viewfinder in the portrait orientation and thus get the
//        // wrong size of preview surface. When we change the preview size, the
//        // new overlay will be created before the old one closed, which causes
//        // an exception. For now, just get the screen size.
//        Point point = getDefaultDisplaySize(activity, new Point());
//        int targetHeight = Math.min(point.x, point.y);
//        // Try to find an size match aspect ratio and size
//        for (ResolutionSize size : sizes) {
//            /*
//             * BUG FIX:5835
//             * BUG COMMENT: java.lang.ArithmeticException: divide by zero
//             * BUG DATE:2014-01-20
//             */
//            if(size.height == 0){
//               continue;
//            }
//            double ratio = (double) size.width / size.height;
//            if (Math.abs(ratio - targetRatio) > ASPECT_TOLERANCE) continue;
//            if (Math.abs(size.height - targetHeight) < minDiff) {
//                optimalSize = size;
//                minDiff = Math.abs(size.height - targetHeight);
//            }
//        }
//        // Cannot find the one match the aspect ratio. This should not happen.
//        // Ignore the requirement.
//        if (optimalSize == null) {
//            Log.w(TAG, "No preview size match the aspect ratio");
//            minDiff = Double.MAX_VALUE;
//            for (ResolutionSize size : sizes) {
//                if (Math.abs(size.height - targetHeight) < minDiff) {
//                    optimalSize = size;
//                    minDiff = Math.abs(size.height - targetHeight);
//                }
//            }
//        }
//        return optimalSize;
//    }
//
//    // Returns the largest picture size which matches the given aspect ratio.
//    public static ResolutionSize getOptimalVideoSnapshotPictureSize(
//            List<ResolutionSize> sizes, double targetRatio) {
//        // Use a very small tolerance because we want an exact match.
//        final double ASPECT_TOLERANCE = 0.001;
//        if (sizes == null) return null;
//
//        ResolutionSize optimalSize = null;
//
//        // Try to find a size matches aspect ratio and has the largest width
//        for (ResolutionSize size : sizes) {
//            double ratio = (double) size.width / size.height;
//            if (Math.abs(ratio - targetRatio) > ASPECT_TOLERANCE) continue;
//            if (optimalSize == null || size.width > optimalSize.width) {
//                optimalSize = size;
//            }
//        }
//
//        // Cannot find one that matches the aspect ratio. This should not happen.
//        // Ignore the requirement.
//        if (optimalSize == null) {
//            Log.w(TAG, "No picture size match the aspect ratio");
//            for (ResolutionSize size : sizes) {
//                if (optimalSize == null || size.width > optimalSize.width) {
//                    optimalSize = size;
//                }
//            }
//        }
//        return optimalSize;
//    }

//    public static void showErrorAndFinish(final Activity activity, int msgId) {
//        DialogInterface.OnClickListener buttonListener =
//                new DialogInterface.OnClickListener() {
//                    public void onClick(DialogInterface dialog, int which) {
//                activity.finish();
//            }
//        };
//        TypedValue out = new TypedValue();
//        activity.getTheme().resolveAttribute(android.R.attr.alertDialogIcon, out, true);
//        new AlertDialog.Builder(activity)
//                .setCancelable(false)
//                .setTitle(R.string.camera_error_title)
//                .setMessage(msgId)
//                .setNeutralButton(R.string.dialog_ok, buttonListener)
//                .setIcon(out.resourceId)
//                .show();
//    }

    public static boolean isUriValid(Uri uri, ContentResolver resolver) {
        if (uri == null) return false;
        try {
            ParcelFileDescriptor pfd = resolver.openFileDescriptor(uri, "r");
            if (pfd == null) {
                return false;
            }
            pfd.close();
        } catch (IOException ex) {
            return false;
        }
        return true;
    }

    public static void viewUri(Uri uri, Context context) {
        if (!isUriValid(uri, context.getContentResolver())) {
            Log.e(TAG, "Uri invalid. uri=" + uri);
            return;
        }

        try {
            context.startActivity(new Intent(UiUtils.REVIEW_ACTION, uri));
        } catch (ActivityNotFoundException ex) {
            try {
                context.startActivity(new Intent(Intent.ACTION_VIEW, uri));
            } catch (ActivityNotFoundException e) {
                Log.e(TAG, "review image fail. uri=" + uri, e);
            }
        }
    }

    public static void broadcastNewPicture(Context context, Uri uri) {
        context.sendBroadcast(new Intent(ACTION_NEW_PICTURE, uri));
        // Keep compatibility
        context.sendBroadcast(new Intent("com.android.camera.NEW_PICTURE", uri));
    }

    public static void setGpsParameters(Parameters parameters, Location loc, long data_taken) {
        // Clear previous GPS location from the parameters.
        parameters.removeGpsData();

        // We always encode GpsTimeStamp
        parameters.setGpsTimestamp(data_taken / 1000);

        // Set GPS location.
        if (loc != null) {
            double lat = loc.getLatitude();
            double lon = loc.getLongitude();
            boolean hasLatLon = (lat != 0.0d) || (lon != 0.0d);

            if (hasLatLon) {
                Log.d(TAG, "Set gps location");
                parameters.setGpsLatitude(lat);
                parameters.setGpsLongitude(lon);
                parameters.setGpsProcessingMethod(loc.getProvider().toUpperCase());
                if (loc.hasAltitude()) {
                    parameters.setGpsAltitude(loc.getAltitude());
                } else {
                    // for NETWORK_PROVIDER location provider, we may have
                    // no altitude information, but the driver needs it, so
                    // we fake one.
                    parameters.setGpsAltitude(0);
                }
                if (loc.getTime() != 0) {
                    // Location.getTime() is UTC in milliseconds.
                    // gps-timestamp is UTC in seconds.
                    long utcTimeSeconds = loc.getTime() / 1000;
                    parameters.setGpsTimestamp(utcTimeSeconds);
                }
            } else {
                loc = null;
            }
        }
    }

    public static Animation createTranslateAnimation(Context context, int position, int from) {
        Animation anim;
        switch (from) {
            case DIRECTION_UP:
                anim = new TranslateAnimation(0.0f, 0, 10.0f + position, position);
                break;
            case DIRECTION_DOWN:
                anim = new TranslateAnimation(0.0f, 0, -position - 10, -position);
                break;
            case DIRECTION_LEFT:
                anim = new TranslateAnimation(10.0f + position, position, 0, 0);
                break;
            case DIRECTION_RIGHT:
                anim = new TranslateAnimation(-position - 10, -position, 0, 0);
                break;
            default:
                throw new IllegalArgumentException(Integer.toString(from));
        }
        anim.setDuration(1000);
        anim.setRepeatCount(Animation.INFINITE);
        anim.setInterpolator(AnimationUtils.loadInterpolator(context,
                    android.R.anim.accelerate_decelerate_interpolator));
        return anim;
    }

    public static Animation createTranslateAnimation(Context context,float x_from,float x_to,float y_from,float y_to, long duration){
        Animation anim = null;
        anim = new TranslateAnimation(x_from, x_to, y_from, y_to);
        anim.setDuration(duration);
        anim.setFillAfter(false);
        return anim;
    }

    public static Animation createRelativeTranslateAnim(Context context,float x_from,float x_to,float y_from,float y_to, long duration) {
        Animation anim = new TranslateAnimation(Animation.RELATIVE_TO_SELF, x_from,
                Animation.RELATIVE_TO_SELF, x_to, Animation.RELATIVE_TO_SELF, y_from,
                Animation.RELATIVE_TO_SELF, y_to);
        anim.setDuration(duration);
        anim.setFillAfter(false);
        return anim;
    }

    public static Animation createFlickerAnimation(int interval) {
        Animation mFlickerAnimation = new AlphaAnimation(1, 0); // Change alpha from fully visible to invisible
        mFlickerAnimation.setDuration(interval);
        mFlickerAnimation.setInterpolator(new LinearInterpolator()); // do not alter animation rate
        mFlickerAnimation.setRepeatCount(Animation.INFINITE);
        mFlickerAnimation.setRepeatMode(Animation.REVERSE); // Reverse animation at the end so the button will fade back in
        return mFlickerAnimation;
    }

    public static void showTopSettingsAnimation(View rootView) {
        if (rootView == null)
            return;
        ScaleAnimation topAnimationShow = new ScaleAnimation(0.1f, 1, 0.1f, 1,
                Animation.RELATIVE_TO_SELF, 0.5f,
                Animation.RELATIVE_TO_SELF, 0.5f);
        topAnimationShow.setDuration(300);
        rootView.startAnimation(topAnimationShow);
        rootView.setVisibility(View.VISIBLE);
    }

    public static void dismissViewAnimation(View rootView) {
        if (rootView == null)
            return;
        ScaleAnimation topAnimationDis = new ScaleAnimation(
                1f, 0.1f, 1f, 0.1f,
                Animation.RELATIVE_TO_SELF, 0.5f,
                Animation.RELATIVE_TO_SELF, 0.5f);
        topAnimationDis.setDuration(300);
        rootView.startAnimation(topAnimationDis);
        rootView.setVisibility(View.GONE);
    }


    /**
     * Display a simple alert dialog with the given text and title.
     *
     * @param context Android context in which the dialog should be displayed
     * @param title Alert dialog title
     * @param message Alert dialog message
     */
    public static void showAlert(Context context, String title, String message) {
        new AlertDialog.Builder(context)
                .setIcon(android.R.drawable.ic_dialog_alert)
                .setTitle(title)
                .setMessage(message)
                .show();
    }

    public static boolean isNetworkAvailable(Context context) {
        ConnectivityManager connManager =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo networkInfo = connManager.getActiveNetworkInfo();
        if (networkInfo != null) {
            return networkInfo.isAvailable();
        } else {
            return false;
        }
    }

    public static boolean checkNetworkShowAlert(Context context) {
        if (isNetworkAvailable(context))
            return true;

        showAlert(context, context.getString(android.R.string.dialog_alert_title),
                    context.getString(R.string.sns_msg_network_unavailable));
        return false;
    }

    public static void browseURLIfNetworkAvailable(Activity act, String url) {
        if(checkNetworkShowAlert(act)) {
            Intent intent = new Intent();
            intent.setAction(Intent.ACTION_VIEW);
            intent.addCategory(Intent.CATEGORY_BROWSABLE);
            intent.setData(Uri.parse(url));
            try {
                act.startActivity(intent);
            } catch(ActivityNotFoundException e) {
                Toast.makeText(act, R.string.text_activity_is_not_found, Toast.LENGTH_LONG).show();
            }
        }
    }

    public static boolean isNetworkConnected(Context ctx) {
        boolean isConnected = false;
        try {
            ConnectivityManager manager = (ConnectivityManager) ctx
                    .getSystemService(Context.CONNECTIVITY_SERVICE);
            NetworkInfo networks = manager.getActiveNetworkInfo();
            if (networks != null) {
                isConnected = networks.isConnected();
            }
        } catch (Exception e) {
        }
        return isConnected;
    }

    public static boolean isSDcardMounted() {
        return android.os.Environment.getExternalStorageState().equals(android.os.Environment.MEDIA_MOUNTED);
    }

    public static String getSize(long v) {
        String str = "";
        if (v == 0) {
            return "0";
        }
        DecimalFormat df = (DecimalFormat) NumberFormat.getInstance();
        df.setMaximumFractionDigits(2);
        double fv = v;
        if (v < 1024) {
            str = "" + v + "Bytes";
        }
        else if (v < 1048576) {
            fv = fv / 1024.0;
            str = "" + df.format(fv) + "K";
        }
        else {
            fv = fv / 1048576.0;
            str = "" + df.format(fv) + "M";
        }
        return str;
    }

    public static void installApp(Context context, String path) {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        Uri apkUri = Uri.fromFile(new File(path));
        intent.setDataAndType(apkUri, "application/vnd.android.package-archive");
        context.startActivity(intent);
    }

    public static void changeFileModel(String filePath) {
        try {
            String command = "chmod 755 " + filePath;
            Runtime runtime = Runtime.getRuntime();
            runtime.exec(command);
        } catch (Exception e) {
            e.printStackTrace();
        }

    }
}
