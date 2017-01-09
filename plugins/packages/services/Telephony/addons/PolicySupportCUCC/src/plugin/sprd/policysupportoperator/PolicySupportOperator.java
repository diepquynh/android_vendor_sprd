package plugin.sprd.policysupportoperator;

import android.util.Log;
import android.app.AddonManager;
import android.content.Context;
import com.android.internal.telephony.policy.IccPolicy;
import com.android.internal.telephony.policy.RadioTaskManager;


public class PolicySupportOperator extends RadioTaskManager implements
        AddonManager.InitialCallback {

    private static final String LOG_TAG = "PolicySupportCUCC";
    private Context mAddonContext;

    public PolicySupportOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public IccPolicy createIccPolicy() {
        Log.d(LOG_TAG, "create cucc policy");
        return new CUCCPolicy();
    }
}
