
package com.sprd.gallery3d.cmcc;

import android.app.Activity;
import android.app.AddonManager;
import android.content.Context;
import android.net.Uri;
import android.util.Log;
import android.view.MenuItem;

import com.android.gallery3d.app.MoviePlayer;
import com.android.gallery3d.R;

public class VideoCmccUtils {

    private static final String TAG = "VideoCmccUtils";
    static VideoCmccUtils sInstance;

    public static VideoCmccUtils getInstance() {
        if (sInstance != null)
            return sInstance;
        sInstance = (VideoCmccUtils) AddonManager.getDefault().getAddon(
                R.string.feature_cmcc_video, VideoCmccUtils.class);
        return sInstance;
    }

    public VideoCmccUtils() {
    }

    /**
     * SPRD: This method is used to register message broadcast
     */
    public void initMessagingUtils(final Activity activity) {
        Log.d(TAG,"VideoCmccUtils initMessagingUtils");
    }

    /**
     * SPRD: This method is used to unregister message broadcast
     */
    public void releaseMessagingUtils() {
        Log.d(TAG,"VideoCmccUtils releaseMessagingUtils");
    }

    /**
     * SPRD: This method is used to cancel messaging remind dialog
     */
    public void destoryMessagingDialog() {
        Log.d(TAG,"VideoCmccUtils destoryMessagingDialog");
    }

    /**
     * SPRD: This method is used to set player use to pause video
     */
    public void initPlayer(MoviePlayer player) {
        Log.d(TAG,"VideoCmccUtils initPlayer");
    }

    /**
     * SPRD: This method is used to judge whether it is timeout or not
     */
    public boolean ifIsPhoneTimeout(long current) {
        return false;
    }
}
