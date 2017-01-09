package plugin.sprd.stkcuccoperator;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.sprd.stk.StkCuccOperatorPluginsHelper;


public class StkCuccOperator extends StkCuccOperatorPluginsHelper implements
        AddonManager.InitialCallback {

    private Context mAddonContext;

    public StkCuccOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean isCUCCOperator() {
        Log.d("StkCuccOperator" , "isCUCCOperator()");
        return true;
    }
}
