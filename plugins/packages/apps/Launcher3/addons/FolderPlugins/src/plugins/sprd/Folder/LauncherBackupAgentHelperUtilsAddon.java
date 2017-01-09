package plugins.sprd.Folder;

import android.app.AddonManager;
import com.android.launcher3.Folderplugins.LauncherBackupAgentHelperUtils;
import android.content.Context;

public class LauncherBackupAgentHelperUtilsAddon extends LauncherBackupAgentHelperUtils implements AddonManager.InitialCallback {

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    @Override
    public boolean isRunCustomeClingDismissed(){
        return true;
    }
}