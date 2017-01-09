package plugin.sprd.supportoperator;
/* SPRD: modify by bug474987 @{ */
import android.app.AddonManager;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import com.android.systemui.statusbar.policy.SystemUIPluginsHelper;
import com.android.systemui.R;

public class SystemUIFeaturesForOperator extends SystemUIPluginsHelper implements
        AddonManager.InitialCallback {

    public static final String LOG_TAG = "SystemUIPluginForCUCC";
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
     * CUCC case:
     * 1.Can not show spn when plmn should be shown.
     * 2.Can not display something like "emergency call" no matter what the situation is.
     */
    public String updateNetworkName(Context context, boolean showSpn, String spn, boolean showPlmn,
            String plmn) {
        Log.d(LOG_TAG, "updateNetworkName showSpn = " + showSpn + " spn = " + spn
                + " showPlmn = " + showPlmn + " plmn = " + plmn);
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
     * CUCC case:
     * Operator names of multi-sim should be shown in different colors
     */
    public int getSubscriptionInfoColor(Context context, int subId) {
        SubscriptionManager subManager = SubscriptionManager.from(mAddonContext);
        SubscriptionInfo subInfo = subManager.getActiveSubscriptionInfo(subId);
       return subInfo == null ? ABSENT_SIM_COLOR : subInfo.getIconTint();
    }

    /**
     * CUCC case:
     * Show special no sim icon for each slot
     */
    public int getNoSimIconId() {
        return R.drawable.stat_sys_no_sim_cucc_ex;
    }

    public int getNoServiceIconId() {
        return R.drawable.stat_sys_signal_null;
    }

    /**
     * CUCC case:
     * Show mobile card icon
     */
    public int getSimCardIconId(int subId) {
        int phoneId = SubscriptionManager.getSlotId(subId);
        return SubscriptionManager.isValidSlotId(phoneId)? SIM_CARD_ID[phoneId] : 0;
    }

    public int getSimStandbyIconId() {
        return R.drawable.stat_sys_signal_standby_ex;
    }

    final int[] SIM_CARD_ID = {
        R.drawable.stat_sys_card1_cucc_ex,
        R.drawable.stat_sys_card2_cucc_ex,
        R.drawable.stat_sys_card3_cucc_ex
    };

    /* SPRD: modify by BUG 474976 @{ */
    public boolean getBoolShowRatFor2G() {
        return true;
    }

    protected boolean showLteFor4G() {
        return true;
    }
    /* @} */

    /* SPRD: Add HD audio icon in cucc for bug 536924. @{ */
    public Drawable getHdVoiceDraw() {
        return mAddonContext.getResources().getDrawable(
                plugin.sprd.supportoperator.R.drawable.stat_sys_hd_voice_sprd_cucc);
    }
    /* @} */
}
/* @} */
