package plugin.sprd.operatorfeatures;

import java.util.ArrayList;
import java.util.List;

import com.android.internal.telephony.SubscriptionController;
import com.android.internal.telephony.EccPluginHelper;
import com.android.internal.telephony.TeleUtils;

import android.app.AddonManager;
import android.app.ActivityThread;
import android.content.Context;
import android.os.SystemProperties;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;

/**
 * Orange plugin for ECC
 */
public class OperatorEccPlugin extends EccPluginHelper implements
        AddonManager.InitialCallback {

    private static final String LOG_TAG = "OrangeEccPlugin";
    private static String[] ORANGE_MCC_MNC = {
            "20801"
    };
    private Context mAddonContext;
    private TelephonyManager mTelephonyManager;

    public OperatorEccPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        mTelephonyManager = TelephonyManager.from(context);
        return clazz;
    }

    /**
     * Orange ECC case:
     * 1.Orange SIM Card inserted in one Slot and no SIM in the other one.
     * 2.Non-Orange SIM Card inserted in one Slot and no SIM in the other one.
     * 3.Orange SIM Card inserted in one Slot and non-Orange SIM in the other one.
     * 4.No SIM card inserted in both Slots.
     *
     * This plugin works in phone process. Use {@link SubscriptionController}.
     *
     * @param simEccs
     */
    public void customizedEccList(String[] simEccs) {
        int phoneCount = mTelephonyManager.getPhoneCount();
        String[] eccLists = new String[phoneCount];
        SubscriptionController subController = SubscriptionController.getInstance();

        int[] subIds = subController.getActiveSubIdList();
        if (subIds.length == 1) {
            int subId = subIds[0];
            int phoneId = subController.getPhoneId(subId);
            if (SubscriptionManager.isValidPhoneId(phoneId)) {
                String simMccMnc = mTelephonyManager.getSimOperatorNumericForSubscription(subId);
                if (isOrangeSIMCard(simMccMnc)) {
                    // [case 1]
                    eccLists[phoneId] = "112,911,000,110,118,119,999,08";
                } else {
                    // [case 2]
                    eccLists[phoneId] = TeleUtils.concatenateEccList("112,911", simEccs[phoneId]);
                }
            }
        } else if (subIds.length > 1) {
            // [case 3]
            for (int i = 0; i < phoneCount; i++) {
                eccLists[i] = TeleUtils.concatenateEccList("112,911", simEccs[i]);
            }
        } else {
            // [case 4]
            // Radio of phone 0 will be setup when no SIM card inserted in both Slots.
            eccLists[0] = "112,911,000,110,118,119,999,08";
        }

        for (int i = 0; i < phoneCount; i++) {
            Log.d(LOG_TAG, "updateEccPropertiesForOrange ECC[" + i + "]: " + eccLists[i]);
            SystemProperties.set(i == 0 ? "ril.ecclist" : ("ril.ecclist" + i), eccLists[i]);
        }
    }

    private boolean isOrangeSIMCard(String mccMnc) {
        for (String orangeMccMnc : ORANGE_MCC_MNC) {
            if (orangeMccMnc.equals(mccMnc)) {
                return true;
            }
        }
        return false;
    }
}
