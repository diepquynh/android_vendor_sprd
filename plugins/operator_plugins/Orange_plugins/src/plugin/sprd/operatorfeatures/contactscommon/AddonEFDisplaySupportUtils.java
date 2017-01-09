package plugin.sprd.supportForEFDisplay;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.sprd.contacts.common.plugin.EFDisplaySupportUtils;

public class AddonEFDisplaySupportUtils extends EFDisplaySupportUtils implements AddonManager.InitialCallback {
    private static final String TAG = "AddonEFDisplaySupportUtils";
    private Context mAddonContext;
    private Context mContext;
    public AddonEFDisplaySupportUtils() {
    }
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean isEFDisplaySupport() {
        return true;
    }
}
