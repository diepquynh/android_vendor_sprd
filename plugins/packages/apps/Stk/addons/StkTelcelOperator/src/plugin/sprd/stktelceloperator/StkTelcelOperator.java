package plugin.sprd.stktelceloperator;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.sprd.stk.StkTelcelOperatorPluginsHelper;


public class StkTelcelOperator extends StkTelcelOperatorPluginsHelper implements
        AddonManager.InitialCallback {

    private Context mAddonContext;

    public StkTelcelOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean isTelcelOperator() {
        Log.d("StkTelcelOperator" , "isTelcelOperator()");
        return true;
    }
}
