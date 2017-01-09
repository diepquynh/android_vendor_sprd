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
    private static String[] ORANGE_SPN = {
            "orange"
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
    public void customizedEccList(String[] simEccs,String[] networkEccs) {
        int phoneCount = mTelephonyManager.getPhoneCount();
        String[] eccLists = new String[phoneCount];
        SubscriptionController subController = SubscriptionController.getInstance();

        int[] subIds = subController.getActiveSubIdList();
        if (subIds.length == 1) {
            int subId = subIds[0];
            int phoneId = subController.getPhoneId(subId);
            if (SubscriptionManager.isValidPhoneId(phoneId)) {
                if (isOrangeSIMCard(subId)) {
                    // [case 1]
                    //Add ECC Number from Network
                    eccLists[phoneId] = TeleUtils.concatenateEccList(TeleUtils.removeDupNumber("112,911,000,110,118,119,999,08,120,122",networkEccs[phoneId]),networkEccs[phoneId]);
                } else {
                    // [case 2]
                    //Add ECC Number from SIM
                    eccLists[phoneId] = TeleUtils.concatenateEccList(TeleUtils.removeDupNumber("112,911", simEccs[phoneId]),simEccs[phoneId]);
                    //Add ECC Number from Network
                    eccLists[phoneId] = TeleUtils.concatenateEccList(TeleUtils.removeDupNumber(eccLists[phoneId],networkEccs[phoneId]),networkEccs[phoneId]);
                }
            }
        } else if (subIds.length > 1) {
            // [case 3]
            for (int i = 0; i < phoneCount; i++) {
                //Add ECC Number from SIM
                eccLists[i] = TeleUtils.concatenateEccList(TeleUtils.removeDupNumber("112,911", simEccs[i]),simEccs[i]);
                //Add ECC Number from Network
                eccLists[i] = TeleUtils.concatenateEccList(TeleUtils.removeDupNumber(eccLists[i],networkEccs[i]),networkEccs[i]);
            }
        } else {
            // [case 4]
            // Radio of phone 0 will be setup when no SIM card inserted in both Slots.
            //Add ECC Number from Network
            eccLists[0] = TeleUtils.concatenateEccList(TeleUtils.removeDupNumber("112,911,000,110,118,119,999,08,120,122",networkEccs[0]),networkEccs[0]);
        }

        for (int i = 0; i < phoneCount; i++) {
            Log.d(LOG_TAG, "updateEccPropertiesForOrange ECC[" + i + "]: " + eccLists[i]);
            SystemProperties.set(i == 0 ? "ril.ecclist.real" : ("ril.ecclist.real" + i),
                    eccLists[i]);
            SystemProperties.set(i == 0 ? "ril.ecclist" : ("ril.ecclist" + i),
                    TeleUtils.removeCategory(eccLists[i]));
        }
    }

    private boolean isOrangeSIMCard(int subId) {
        String simMccMnc = mTelephonyManager.getSimOperatorNumeric(subId);
        String simSpn = mTelephonyManager.getSimOperatorName(subId);

        Log.i(LOG_TAG, " SubId[" + subId + "]: The current mcc + mnc is  " + simMccMnc
                + ", the current Service Provider Name (SPN) is  " + simSpn);
        for (String orangeSpn : ORANGE_SPN) {
            if (orangeSpn.equals(simSpn)) {
                return true;
            }
        }
        for (String orangeMccMnc : ORANGE_MCC_MNC) {
            if (orangeMccMnc.equals(simMccMnc)) {
                return true;
            }
        }
        return false;
    }
}
