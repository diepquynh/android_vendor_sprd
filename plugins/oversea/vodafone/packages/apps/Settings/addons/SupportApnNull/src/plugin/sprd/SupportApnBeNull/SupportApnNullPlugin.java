package plugin.sprd.SupportApnBeNull;

import android.app.AddonManager;
import android.content.Context;
import com.sprd.settings.SupportApnNullUtils;
import android.util.Log;

public class SupportApnNullPlugin extends SupportApnNullUtils implements
        AddonManager.InitialCallback {

    private final String TAG = "SupportApnNullPlugin";
    private Context mAddonContext;

    public SupportApnNullPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean IsSupportApnNull() {
        Log.d(TAG, "IsSupportApnNull: true");
        return true;
    }
}