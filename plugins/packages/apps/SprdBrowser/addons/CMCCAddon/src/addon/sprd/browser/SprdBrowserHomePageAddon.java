
package addon.sprd.browser;

import com.sprd.custom.SprdBrowserHomePageAddonStub;
import android.content.Context;
import android.app.AddonManager;

public class SprdBrowserHomePageAddon extends SprdBrowserHomePageAddonStub implements AddonManager.InitialCallback {

    private Context mContext;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }
    @Override
    public String getHomePageString() {
        return mContext.getResources().getString(R.string.homepage);
    }
}
