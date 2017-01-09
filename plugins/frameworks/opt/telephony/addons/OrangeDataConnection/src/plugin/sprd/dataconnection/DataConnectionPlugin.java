package plugin.sprd.dataconnection;

import android.app.AddonManager;
import android.content.Context;
import com.android.internal.telephony.plugin.DataConnectionUtils;
import android.util.Log;

public class DataConnectionPlugin extends DataConnectionUtils implements
        AddonManager.InitialCallback {

    private static final String TAG = "OrangeDataConnection";
    private Context mAddonContext;

    public DataConnectionPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean supportTrafficClass() {
        Log.d(TAG, "supportTrafficClass : true");
        return true;
    }
}
