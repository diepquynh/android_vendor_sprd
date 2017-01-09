/** Created by Spreadst */

package plugin.sprd.location;

import android.app.AddonManager;
import android.content.Context;

import com.sprd.settings.GPSHelper;

public class GPSAddon extends GPSHelper implements AddonManager.InitialCallback {

    private static final String TAG = GPSAddon.class.getSimpleName();
    private Context mContext;

    public GPSAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    public boolean isSupportCmcc() {
        return true;
    }

}
