
package com.android.phone;

import android.content.Context;
import android.preference.PreferenceScreen;

public class TeleServiceFactoryEx extends TeleServiceFactory {
    private NetworkTypeOptions mNetworkTypeOptions;

    public void updateNetworkTypeOptions(Context context, PreferenceScreen prefSet,
            int phoneSubId) {

        if (mNetworkTypeOptions == null) {
            mNetworkTypeOptions = new NetworkTypeOptions(context);
        }
        //SPRD: modify for 610272
        mNetworkTypeOptions.updateNetworkOptions(phoneSubId,prefSet);

    }

    public void disposeNetworkTypeOptions() {
        // SPRD: MODIFY FOR BUG 601368
        if (mNetworkTypeOptions != null) {
            mNetworkTypeOptions.dispose();
            mNetworkTypeOptions = null;
        }
    }

    public void UpdateEnabledNetworksValueAndSummary(int NetworkMode) {
        // SPRD: MODIFY FOR BUG 601368
        if (mNetworkTypeOptions != null) {
            mNetworkTypeOptions.UpdateEnabledNetworksValueAndSummary(NetworkMode);
        }
    }
}
