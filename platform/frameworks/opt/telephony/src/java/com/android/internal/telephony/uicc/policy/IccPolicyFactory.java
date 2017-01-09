
package com.android.internal.telephony.uicc.policy;

import android.app.AddonManager;
import android.content.Context;
import android.content.res.Resources;

public class IccPolicyFactory {

    public IccPolicyFactory() {
    }

    protected IccPolicy createIccPolicy() {
        return new DefaultPolicy();
    }

    static IccPolicyFactory getInstance(Context context) {
        IccPolicyFactory factory = null;
        factory = (IccPolicyFactory) new AddonManager(context).getAddon(
                Resources.getSystem().getString(com.android.internal.R.string.feature_create_icc_policy),
                IccPolicyFactory.class);
        if (factory == null) {
            factory = new IccPolicyFactory();
        }
        return factory;
    }
}
