package plugins.sprd.Folder;

import java.lang.ref.WeakReference;

import com.android.launcher3.LauncherProvider;
import com.android.launcher3.VolteAppsProvider;
import com.android.launcher3.Folderplugins.LauncherAppStateUtils;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

public class LauncherAppStateUtilsAddon extends LauncherAppStateUtils implements AddonManager.InitialCallback {
    private static WeakReference<VolteAppsProvider> sVolteAppsProvider;
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    @Override
    public void setLauncherCustomeProvider(VolteAppsProvider provider){
        sVolteAppsProvider = new WeakReference<VolteAppsProvider>(provider);
    }

    @Override
    public VolteAppsProvider getLauncherAppsCustomeProvider(){
        return sVolteAppsProvider.get();
    }
}