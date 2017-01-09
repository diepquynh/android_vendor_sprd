package plugin.sprd.telephonysupportorange;

import android.app.AddonManager;
import android.content.Context;
import com.android.internal.telephony.plugin.TelephonyForOrangeUtils;

public class TelephonySupportOrange extends TelephonyForOrangeUtils implements
        AddonManager.InitialCallback {

    private Context mAddonContext;
    public TelephonySupportOrange(){
    }
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }
    public boolean IsSupportOrange() {
        return true;
    }
}
