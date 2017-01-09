package plugin.sprd.defaultappsettings;

import android.app.AddonManager;
import android.content.Context;
import com.android.settings.applications.SupportDefaultAppSettings;

public class AddonDefaultAppSettings extends SupportDefaultAppSettings implements AddonManager.InitialCallback {

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public boolean isSupport(){
        return true;
    }
}
