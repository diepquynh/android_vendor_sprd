package plugin.sprd.fdndial;

import android.app.AddonManager;
import android.content.Context;
import android.content.Intent;

import com.sprd.plugin.FdnDialUtils;
public class FdnDialPlugin extends FdnDialUtils implements AddonManager.InitialCallback {
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public FdnDialPlugin() {
    }

    public String getAction() {
        String action = Intent.ACTION_DIAL;
        return action;
    }
}