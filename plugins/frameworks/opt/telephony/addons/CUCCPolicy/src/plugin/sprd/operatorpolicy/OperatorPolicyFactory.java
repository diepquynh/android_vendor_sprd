package plugin.sprd.operatorpolicy;

import android.telephony.Rlog;
import android.app.AddonManager;
import android.content.Context;
import com.android.internal.telephony.uicc.policy.IccPolicy;
import com.android.internal.telephony.uicc.policy.IccPolicyFactory;


public class OperatorPolicyFactory extends IccPolicyFactory implements
        AddonManager.InitialCallback {

    private static final String LOG_TAG = "CUCCPolicyFactory";
    private Context mAddonContext;

    public OperatorPolicyFactory() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public IccPolicy createIccPolicy() {
        Rlog.d(LOG_TAG, "create cucc policy");
        return new CUCCPolicy();
    }
}
