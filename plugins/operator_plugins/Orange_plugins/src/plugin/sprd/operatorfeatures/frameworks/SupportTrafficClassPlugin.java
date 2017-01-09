package plugin.sprd.SupportTrafficClass;

import android.app.AddonManager;
import android.content.Context;
import com.android.internal.telephony.plugin.SupportTrafficClassUtils;
import android.util.Log;

public class SupportTrafficClassPlugin extends SupportTrafficClassUtils implements
        AddonManager.InitialCallback {

    private static final String TAG = "SupportTrafficClassPlugin";
    private Context mAddonContext;

    public SupportTrafficClassPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean isSupportTracfficClass() {
        Log.d(TAG, "isSupportTracfficClass : true");
        return true;
    }
}