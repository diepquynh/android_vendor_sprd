package plugin.sprd.stksetupmenu;

import android.app.AddonManager;
import android.content.Context;

import com.sprd.stk.StkSetupMenuPluginsHelper;


public class StkSetupMenu extends StkSetupMenuPluginsHelper implements
        AddonManager.InitialCallback {

    private Context mAddonContext;

    public StkSetupMenu() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean isShowSetupMenuTitle() {
        return true;
    }
}
