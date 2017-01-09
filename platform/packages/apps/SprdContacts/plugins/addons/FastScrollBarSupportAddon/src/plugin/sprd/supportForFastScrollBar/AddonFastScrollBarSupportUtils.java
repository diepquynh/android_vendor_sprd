package plugin.sprd.supportForFastScrollBar;

import android.app.AddonManager;
import android.content.Context;
import android.view.LayoutInflater;
import com.sprd.contacts.common.plugin.FastScrollBarSupportUtils;

public class AddonFastScrollBarSupportUtils extends FastScrollBarSupportUtils
        implements AddonManager.InitialCallback {
    private static final String TAG = "AddonFastScrollBarSupportUtils";
    private Context mAddonContext;
    private Context mContext;

    public AddonFastScrollBarSupportUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean hasSupportFastScrollBar() {
        return true;
    }
}
