package com.sprd.telephonyPlugin;
import android.app.AddonManager;
import com.android.services.telephony.plugin.CallSettingsPluginsHelper;
import android.content.Context;

public class CallSettingsForOperatorPlugin extends CallSettingsPluginsHelper
        implements AddonManager.InitialCallback {

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public CallSettingsForOperatorPlugin() {
        // TODO Auto-generated constructor stub
    }

    @Override
    public boolean isSupportEditVoicemail() {
        return false;
    }
}
