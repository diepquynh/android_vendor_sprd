
package com.sprd.cmccvideoplugin;

import android.app.Activity;
import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.Context;
import android.os.Handler;
import android.util.Log;

import com.sprd.gallery3d.cmcc.VideoCmccUtils;
import com.android.gallery3d.app.MoviePlayer;

public class AddonVideoCmccUtils extends VideoCmccUtils implements AddonManager.InitialCallback {

    private static final String TAG = "AddonVideoCmccUtils";
    private Context mPluginContext;

    public AddonVideoCmccUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        Log.d(TAG, "onCreateAddon");
        mPluginContext = context;
        return clazz;
    }

    /**
     * SPRD: This method is used to register message broadcast
     */
    public void initMessagingUtils(final Activity activity) {
        Log.d(TAG,"AddonVideoCmccUtils initMessagingUtils");
        CmccMessagingUtils.getInstance().initMessagingUtils(activity, mPluginContext);
    }

    /**
     * SPRD: This method is used to unregister message broadcast
     */
    public void releaseMessagingUtils() {
        Log.d(TAG,"AddonVideoCmccUtils releaseMessagingUtils");
        CmccMessagingUtils.getInstance().releaseMessagingUtils();
    }

    /**
     * SPRD: This method is used to cancel messaging remind dialog
     */
    public void destoryMessagingDialog() {
        Log.d(TAG,"AddonVideoCmccUtils destoryMessagingDialog");
        CmccMessagingUtils.getInstance().destoryMessagingDialog();
    }

    /**
     * SPRD: This method is used to set player use to pause video
     */
    public void initPlayer(MoviePlayer player) {
        Log.d(TAG,"AddonVideoCmccUtils initPlayer");
        CmccMessagingUtils.getInstance().initPlayer(player);
    }

    /**
     * SPRD: This method is used to judge whether it is timeout or not
     */
    public boolean ifIsPhoneTimeout(long current){
        return CmccPhoneUtils.getInstance().ifIsPhoneTimeout(current);
    }
}
