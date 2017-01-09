package plugin.sprd.supportoperator;
/* SPRD: modify by bug474987 @{ */
import android.app.AddonManager;
import android.content.Context;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import com.android.systemui.statusbar.policy.SystemUIPluginsHelper;
import com.android.systemui.R;

public class SystemUIFeaturesForOperator extends SystemUIPluginsHelper implements
        AddonManager.InitialCallback {

    public static final String LOG_TAG = "SystemUIPluginForReliance";
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
     * Reliance case:
     * 1.Can not show spn when plmn should be shown.
     * 2.Can not display something like "emergency call" no matter what the situation is.
     */
    public String updateNetworkName(Context context, boolean showSpn, String spn, boolean showPlmn,
            String plmn, int phoneId) {
        Log.d(LOG_TAG, "updateNetworkName showSpn=" + showSpn + " spn=" + spn
                + " showPlmn=" + showPlmn + " plmn=" + plmn);
        StringBuilder str = new StringBuilder();
        if (showPlmn && plmn != null) {
            if (context.getString(com.android.internal.R.string.emergency_calls_only)
                    .equals(plmn)) {
                plmn = context.getResources()
                        .getString(com.android.internal.R.string.lockscreen_carrier_default);
            }
            str.append(plmn);
        } else if (showSpn && spn != null) {
            str.append(spn);
        }

        return str.toString();
    }

    /**
     * Reliance case:
     * Operator names of multi-sim should be shown in different colors
     */
    public int getSubscriptionInfoColor(Context context, int subId) {
        SubscriptionManager subManager = SubscriptionManager.from(mAddonContext);
        SubscriptionInfo subInfo = subManager.getActiveSubscriptionInfo(subId);
       return subInfo == null ? ABSENT_SIM_COLOR : subInfo.getIconTint();
    }

    /**
     * Reliance case:
     * Show special no sim icon for each slot.
     */
    public int getNoSimIconId() {
        return R.drawable.stat_sys_no_sim_ex;
    }

    public int getNoServiceIconId() {
        return R.drawable.stat_sys_signal_null_reliance;
    }

    public int getSimStandbyIconId() {
        return R.drawable.stat_sys_signal_standby_sprd_reliance;
    }

    public int getSimCardIconId(int subId) {
        return 0;
    }

    /**
     * Reliance case:
     * Show mobile signal group layout
     */
    public int getMobileGroupLayout() {
        return R.layout.mobile_signal_group_reliance;
    }

    /**
     * Reliance case:
     * Show new signal icons
     */
    public boolean isReliance() {
        return true;
    }

    public int getLteIconId() {
        return R.drawable.stat_sys_data_fully_connected_lte_sprd_reliance;
    }

    public int getRoamIcon() {
        return R.drawable.stat_sys_data_connected_roam_sprd_reliance;
    }

    public int getLteIcon() {
        return R.drawable.stat_sys_data_fully_connected_lte_sprd_reliance;
    }

    public int getGIcon() {
        return R.drawable.stat_sys_data_fully_connected_g_sprd_reliance;
    }

    public int getEIcon() {
        return R.drawable.stat_sys_data_fully_connected_e_sprd_reliance;
    }

    public int getHIcon() {
        return R.drawable.stat_sys_data_fully_connected_h_sprd_reliance;
    }

    public int getHPIcon() {
        return R.drawable.stat_sys_data_fully_connected_hp_sprd_reliance;
    }

    public int getThreeGIcon() {
        return R.drawable.stat_sys_data_fully_connected_3g_sprd_reliance;
    }

    public int getFourGIcon() {
        return R.drawable.stat_sys_data_fully_connected_4g_sprd_reliance;
    }

    public int getOneXIcon() {
        return R.drawable.stat_sys_data_fully_connected_1x_sprd_reliance;
    }

    public int getFourGLte() {
        return R.drawable.stat_sys_data_fully_connected_4g_lte_sprd_reliance;
    }

    public int getDataInOutIcon() {
        return R.drawable.stat_sys_data_inout_sprd_reliance;
    }

    public int getDataInIcon() {
        return R.drawable.stat_sys_data_in_sprd_reliance;
    }

    public int getDataOutIcon() {
        return R.drawable.stat_sys_data_out_sprd_reliance;
    }

    public int getDataDefaultIcon() {
        return R.drawable.stat_sys_data_default_sprd_reliance;
    }

    public int getSignalZeroIcon() {
        return R.drawable.stat_sys_signal_0_sprd_reliance;
    }

    public int getSignalOneIcon() {
        return R.drawable.stat_sys_signal_1_sprd_reliance;
    }

    public int getSignalTwoIcon() {
        return R.drawable.stat_sys_signal_2_sprd_reliance;
    }

    public int getSignalThreeIcon() {
        return R.drawable.stat_sys_signal_3_sprd_reliance;
    }

    public int getSignalFourIcon() {
        return R.drawable.stat_sys_signal_4_sprd_reliance;
    }

    public int getVoLTEIcon() {
        return 0;
    }

    public int getSignalVoLTEIcon() {
        return R.drawable.stat_sys_data_connected_volte_hd_sprd_reliance;
    }
}
/* @} */