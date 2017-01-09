package plugin.sprd.supportoperator;
/* SPRD: modify by bug474989 @{ */
import android.app.AddonManager;
import android.content.Context;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionInfo;
import android.util.Log;

import  com.android.systemui.statusbar.policy.SystemUIPluginsHelper;
import  com.android.systemui.R;

public class SystemUIFeaturesForOperator extends SystemUIPluginsHelper implements
        AddonManager.InitialCallback {

    public static final String LOG_TAG = "SystemUIPluginForCMCC";
    private Context mAddonContext;
    public static final int ABSENT_SIM_COLOR = 0xFFFFFFFF;

    public SystemUIFeaturesForOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    /**
     * CMCC case:
     * 1.Can not show spn when plmn should be shown.
     */
    public String updateNetworkName(Context context, boolean showSpn, String spn, boolean showPlmn,
            String plmn) {
        Log.d(LOG_TAG, "updateNetworkName showSpn = " + showSpn + " spn = " + spn
                + " showPlmn = " + showPlmn + " plmn = " + plmn);
        StringBuilder str = new StringBuilder();
        if (showPlmn && plmn != null) {
            /* SPRD: add for CMCC don't allow display emergency calls only @{ */
            if (context.getString(com.android.internal.R.string.emergency_calls_only)
                    .equals(plmn)) {
                plmn = context.getResources()
                        .getString(com.android.internal.R.string.lockscreen_carrier_default);
            }
            /* @} */
            str.append(plmn);
        } else if (showSpn && spn != null) {
            str.append(spn);
        }
        return str.toString();
    }

    /**
     * CMCC case:
     * 1.Operator names of different sims should be shown in different colors.
     */

    public int getSubscriptionInfoColor(Context context, int subId) {
        SubscriptionManager subManager = SubscriptionManager.from(mAddonContext);
        SubscriptionInfo subInfo = subManager.getActiveSubscriptionInfo(subId);
       return subInfo == null ? ABSENT_SIM_COLOR : subInfo.getIconTint();
    }


    public int getNoSimIconId() {
        return R.drawable.stat_sys_no_sim_cmcc_ex;
    }

    public int getNoServiceIconId() {
        return R.drawable.stat_sys_signal_null;
    }

    public int getSimStandbyIconId() {
        return R.drawable.stat_sys_signal_standby_ex;
    }

    /* SPRD: Add for CMCC Volte HD icon in BUG 530739. @{ */
    public int getVoLTEIcon() {
        return R.drawable.stat_sys_volte_cmcc;
    }
    /* @} */

    /* SPRD: add for BUG 474780 @{ */
    public boolean showDataUseInterface() {
        return false;
    }
    /* @} */

    /**
     * CMCC new case : Not allow user to set network type to 3g2g
     * for bug 522182
     */
    public boolean show4GInQS() {
        return false;
    }
}
/* @} */
