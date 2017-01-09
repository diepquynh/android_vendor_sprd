package plugin.sprd.stksetupcall;

import android.app.AddonManager;
import android.content.Context;

import com.sprd.stk.StkSetupCallPluginsHelper;


public class StkSetupCall extends StkSetupCallPluginsHelper implements
        AddonManager.InitialCallback {

    private Context mAddonContext;

    public StkSetupCall() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean needConfirm() {
        return true;
    }
}
