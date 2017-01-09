package com.sprd.drmvideoplugin;

import android.app.Activity;
import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.drm.DrmManagerClientEx;
import android.net.Uri;
import android.util.Log;
import android.view.MenuItem;
import android.widget.Toast;

import com.android.gallery3d.app.MoviePlayer;
import com.sprd.gallery3d.drm.VideoDrmUtils;

import com.sprd.drmvideoplugin.R;

public class AddonVideoDrmUtils extends VideoDrmUtils implements AddonManager.InitialCallback {

    private static final String TAG = "AddonVideoDrmUtils";
    private static DrmManagerClientEx sDrmManagerClient = null;
    private Context mPluginContext;

    public AddonVideoDrmUtils() {
    }

    public static DrmManagerClientEx getDrmManagerClient() {
        return sDrmManagerClient;
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        Log.d(TAG, "onCreateAddon");
        sDrmManagerClient = new DrmManagerClientEx(context);
        mPluginContext = context;
        return clazz;
    }

    /**
     * SPRD: Update mFilePath in VideoDrmUtils
     *
     * A protected member is used to indicate absolut file path
     * which is playing now. For the purpose of isolating all drm
     * implementation, this path is maintained in addon class and
     * original code can not access it. This method is used to update
     * mFilePath at appropriate place. It is an empty function in
     * VideoDrmUtils.
     */
    @Override
    public void getFilePathByUri(Uri uri, Context context) {
        mFilePath = DrmUtil.getFilePathByUri(uri, context);
        Log.d(TAG, "getFilePathByUri: " + mFilePath);
    }

    /**
     * SPRD: This method is used to control the share menu item
     *
     * Return value represents current state of share menu item.
     * trur for enable and false for disable. This return value
     * should be changed when it is used in plugin apk. If current file
     * can be transfered, enable the item and return false. And if not, disable
     * the item and return true.
     */
    @Override
    public boolean disableShareMenu(MenuItem shareItem) {
        if (!DrmUtil.isDrmSupportTransfer(mFilePath)) {
            shareItem.setVisible(false);
            Log.d(TAG, "share menu item is disabled");
            return true;
        } else {
            shareItem.setVisible(true);
            return false;
        } 
    }

    /**
     * SPRD: Set mConsumeForPause Flag
     * mConsumeForPause is used to indicate consume or not when suspend is invoked
     * If back is pressed or playing finish, just suspend and consume
     * If home key is pressed, suspend but not consume(push to background and will be back later)
     */
    @Override
    public void needToConsume(boolean consume) {
        mConsumeForPause = consume;
    }

    /**
     * SPRD: Get mConsumeForPause Flag
     *
     * isFinishedByUser means user do not want to stop playing but just want to leave for a while.
     * When home is pressed, maybe user just want to push the playing activity to background.
     * So in this scenario, isFinishedByUser will return true and drm file does not need to be
     * consumed right.
     */
    @Override
    public boolean isConsumed() {
        Log.d(TAG, "isConsumed: " + !mConsumeForPause);
        return !mConsumeForPause;
    }

    @Override
    public boolean isDrmFile(String filePath, String mimeType) {
        return DrmUtil.isDrmFile(filePath, mimeType);
    }

    @Override
    public boolean isDrmFile() {
        Log.d(TAG, "isDrmFile");
        return DrmUtil.isDrmFile(mFilePath, null);
    }

    @Override
    public void checkRightBeforePlay(final Activity activity, final MoviePlayer mp) {
        if (DrmUtil.isDrmValid(mFilePath)) {
            if (mIsStopped && !DrmUtil.getDrmFileType(mFilePath).equals(DrmUtil.FL_DRM_FILE)) {
                new AlertDialog.Builder(activity).
                        setTitle(mPluginContext.getString(R.string.drm_consume_title)).
                        setMessage(mPluginContext.getString(R.string.drm_consume_hint)).
                        setPositiveButton(mPluginContext.getString(R.string.channel_setting_ok),
                                new AlertDialog.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        mp.playVideoWrapper();
                                        mIsStopped = false;
                                    }
                                }).
                        setNegativeButton(mPluginContext.getString(R.string.channel_setting_cancel), null).
                        show();
            } else {
                mp.playVideoWrapper();
            }
        } else {
            //Context context = activity.getApplicationContext();
            Toast.makeText(mPluginContext, mPluginContext.getString(R.string.drm_file_invalid), Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    public void checkRightBeforeChange(final Activity activity, final MoviePlayer mp) {
        if (DrmUtil.isDrmValid(mFilePath)) {
            if (!DrmUtil.getDrmFileType(mFilePath).equals(DrmUtil.FL_DRM_FILE)) {
                new AlertDialog.Builder(activity).
                        setTitle(mPluginContext.getString(R.string.drm_consume_title)).
                        setMessage(mPluginContext.getString(R.string.drm_consume_hint)).
                        setPositiveButton(mPluginContext.getString(R.string.channel_setting_ok),
                                new AlertDialog.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        mp.changeVideoWrapper();
                                    }
                                }).
                        setNegativeButton(mPluginContext.getString(R.string.channel_setting_cancel),
                                new AlertDialog.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        activity.finish();
                                    }
                                }).
                        show();

            } else {
                mp.changeVideoWrapper();
            }
        } else {
            //Context context = activity.getApplicationContext();
            Toast.makeText(mPluginContext, mPluginContext.getString(R.string.drm_file_invalid), Toast.LENGTH_SHORT).show();
            activity.finish();
        }
    }

    @Override
    public void setStopState(boolean state) {
        mIsStopped = state;
    }
}
