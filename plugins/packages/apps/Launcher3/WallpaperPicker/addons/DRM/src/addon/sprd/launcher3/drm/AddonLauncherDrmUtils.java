package addon.sprd.launcher3.drm;

import android.app.AddonManager;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.android.launcher3.SprdLauncherDrmUtils;

public class AddonLauncherDrmUtils extends SprdLauncherDrmUtils implements
        AddonManager.InitialCallback {
    public static final String KEY_PICK_IMAGE_FOR_WALLPAPER = "applyForWallpaper";
    private String TAG = "AddonLauncherDrmUtils";

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    @Override
    public void addIntentExtra(Intent intent) {
        Log.d(TAG, "intent = " + intent);
        intent.putExtra(KEY_PICK_IMAGE_FOR_WALLPAPER, true);
    }
}
