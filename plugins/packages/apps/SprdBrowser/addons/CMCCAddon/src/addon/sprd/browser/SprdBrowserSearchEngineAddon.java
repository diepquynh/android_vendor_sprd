
package addon.sprd.browser;

import com.sprd.custom.SprdBrowserSearchEngineAddonStub;
import android.content.Context;
import android.app.AddonManager;

public class SprdBrowserSearchEngineAddon extends SprdBrowserSearchEngineAddonStub implements AddonManager.InitialCallback {

    private Context mContext;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }
    @Override
    public String getSearchEngineString() {
        return "baidu";
    }
}
