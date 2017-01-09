package plugin.sprd.dtmftoneSupport;

import com.android.phone.TelcelDTMFToneSupportHelper;
import android.app.AddonManager;
import android.content.Context;

public class DTMFToneSupportPlugin extends TelcelDTMFToneSupportHelper implements AddonManager.InitialCallback {

    public void DTMFToneSupportPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public boolean isSupportDTMFTone() {
        return false;
    }
}