package plugin.sprd.teleservicesupportoperator;

import android.app.AddonManager;
import android.content.Context;
import android.content.res.Resources;
import android.preference.ListPreference;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppType;
import com.android.internal.telephony.TeleUtils;
import com.android.phone.R;
import com.sprd.phone.TeleServicePluginsHelper;


public class TeleServiceSupportOperator extends TeleServicePluginsHelper implements
        AddonManager.InitialCallback {

    public static final String LOG_TAG = "SystemUIPluginForReliance";
    private Context mAddonContext;

    public TeleServiceSupportOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean isSupportPlmn() {
        Log.d(LOG_TAG, "isSupportPlmn = " + false);
        return false;
    }

    /* SPRD: Add a dialog in NetworkSetting for bug 531844 in reliance case. @{ */
    public boolean showDataOffWarning() {
        Log.d(LOG_TAG, "showDataOffWarning = " + true);
        return true;
    }
    /* @} */
}
