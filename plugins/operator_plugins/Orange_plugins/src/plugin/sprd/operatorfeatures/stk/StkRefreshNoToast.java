package plugin.sprd.stknotoast;

import android.app.AddonManager;
import android.content.Context;

import com.sprd.stk.StkRefreshPluginsHelper;


public class StkRefreshNoToast extends StkRefreshPluginsHelper implements
        AddonManager.InitialCallback {

    private Context mAddonContext;

    public StkRefreshNoToast() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean refreshNoToast() {
        return true;
    }
}
