package com.sprd.gallery3d.drm;

import android.app.Activity;
import android.app.AddonManager;
import android.content.Context;
import android.net.Uri;
import android.view.MenuItem;

import com.android.gallery3d.app.MoviePlayer;
import com.android.gallery3d.R;

public class VideoDrmUtils {
    static VideoDrmUtils sInstance;
    protected String mFilePath = null;

    // This flag is used to indicate consume or not when suspend is invoked
    // If back is pressed or playing finish, just suspend and consume
    // If home key is pressed, suspend but not consume(push to background and will be back later)
    protected boolean mConsumeForPause = false;

    protected boolean mIsStopped = false;

    public static VideoDrmUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (VideoDrmUtils) AddonManager.getDefault().getAddon(R.string.feature_drm_video, VideoDrmUtils.class);
        return sInstance;
    }

    public VideoDrmUtils() {
    }

    public void getFilePathByUri(Uri uri, Context context) {
    }

    /**
     * SPRD: This method is used to control the share menu item
     * When drm feature is disabled, videos are always able to be shared,
     * so just return false here which means share menu item isn't disabled.
     *
     * Return value represents current state of share menu item.
     * trur for enable and false for disable. This return value
     * should be changed when it is used in plugin apk. If current file
     * can be transfered, enable the item and return false. And if not, disable
     * the item and return true.
     */
    public boolean disableShareMenu(MenuItem shareItem) {
        return false;
    }

    /**
     * Set mConsumeForPause flag
     */
    public void needToConsume(boolean consume) {
    }

    public boolean isDrmFile(String filePath, String mimeType) {
        return false;
    }

    public boolean isDrmFile() {
        return false;
    }

    public void checkRightBeforePlay(final Activity activity, final MoviePlayer mp) {
    }

    public void checkRightBeforeChange(final Activity activity, final MoviePlayer mp) {
    }

    public void setStopState(boolean state) {
    }

    public boolean isConsumed() {
        return false;
    }
}
