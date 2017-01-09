package plugin.sprd.stksessionend;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

import com.sprd.stk.StkPluginsHelper;


public class StkSessionEnd extends StkPluginsHelper implements
        AddonManager.InitialCallback {

    private Context mAddonContext;

    public StkSessionEnd() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean needCloseMenu() {
        Log.d("StkAppPlugin-StkSessionEnd" , "needCloseMenu in StkSessionEnd");
        return true;
    }
}
