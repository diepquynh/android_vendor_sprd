/**
 *
 */
package com.ucamera.ucam.modules.utils;

import com.android.camera.util.ApiHelper;
import android.content.pm.PackageManager;
import android.Manifest;
import android.app.Activity;

import android.content.Context;

/**
 * @author sprd
 *
 */
public class UCamUtill {

    private final static String TARGET_TS_UCAM_MAKEUP_BEAUTY_ENABL = "persist.sys.ucam.beauty";
    private final static String TARGET_TS_UCAM_MAKEUP_PUZZLE_ENABLE = "persist.sys.ucam.puzzle";
    private final static String TARGET_TS_UCAM_MAKEUP_PEDIT_ENABLE = "persist.sys.ucam.edit";
    private final static String TARGET_TS_TIMESTAMP_ENABL = "persist.sys.cam.timestamp";
    private final static String TARGET_TS_GIF_NENABL = "persist.sys.cam.gif";
    private final static String TARGET_TS_SCENERY_NENABL = "persist.sys.cam.scenery";
    private final static String TARGET_TS_QUICK_CAPTURE_ENABL = "persist.sys.cam.quick";
    private final static String TARGET_TS_VGESTURE_ENABL = "persist.sys.cam.vgesture";
    private final static String TARGET_TS_FILTER_NENABL = "persist.sys.ucam.filter";

    private static boolean isUcamBeautyEnable;
    private static boolean isUcamPuzzleEnable;
    private static boolean isUcamEditEnable;
    private static boolean isTimeStampEnable;
    private static boolean isGifEnable;
    private static boolean isSceneryEnable ;
    private static boolean isQuickCaptureEnabled ;
    private static boolean isVgesture;
    private static boolean isUcamFilterEnable;

    public static void initialize(Context context) {
        isUcamBeautyEnable = android.os.SystemProperties.getBoolean(
                TARGET_TS_UCAM_MAKEUP_BEAUTY_ENABL, true);
        isUcamPuzzleEnable = android.os.SystemProperties.getBoolean(
                TARGET_TS_UCAM_MAKEUP_PUZZLE_ENABLE, true);
        isUcamEditEnable = android.os.SystemProperties.getBoolean(
                TARGET_TS_UCAM_MAKEUP_PEDIT_ENABLE, true);
        isTimeStampEnable = android.os.SystemProperties.getBoolean(
                TARGET_TS_TIMESTAMP_ENABL, true);
        isGifEnable = android.os.SystemProperties.getBoolean(
                TARGET_TS_GIF_NENABL, true);
        isSceneryEnable = android.os.SystemProperties.getBoolean(
                TARGET_TS_SCENERY_NENABL, true);
        isQuickCaptureEnabled = android.os.SystemProperties.getBoolean(
                 TARGET_TS_QUICK_CAPTURE_ENABL, true);
        isVgesture = android.os.SystemProperties.getBoolean(
                TARGET_TS_VGESTURE_ENABL, true);
        isUcamFilterEnable = android.os.SystemProperties.getBoolean(
                TARGET_TS_FILTER_NENABL, true);
    }

    public static boolean isUcamBeautyEnable() {
        return isUcamBeautyEnable;
    }

    public static boolean isUcamFilterEnable() {
        return isUcamFilterEnable;
    }

    public static boolean isUcamPuzzleEnable() {
        return isUcamPuzzleEnable;
    }

    public static boolean isUcamEditEnable() {
        return isUcamEditEnable;
    }

    /* SPRD:fix bug514045 Some Activity about UCamera lacks method of checkpermission@{ */
    public static boolean checkPermissions(Activity mActivity) {
        if (!ApiHelper.isMOrHigher()) {
            return true;
        }

        if (mActivity.checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED &&
                mActivity.checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED &&
                mActivity.checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
            return true;
        }
        return false;
    }
    /* }@ */

    public static boolean isTimeStampEnable() {
        return isTimeStampEnable;
    }
    public static boolean isGifEnable() {
        return isGifEnable;
    }

    public static boolean isSceneryEnable() {
        return isSceneryEnable;
    }

    public static boolean isQuickCaptureEnabled(){
        return isQuickCaptureEnabled;
    }

    public static boolean isVgestureEnnable(){
        return isVgesture;
    }
}
