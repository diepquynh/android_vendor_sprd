package plugin.sprd.telephony.uicc;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.ProxyController;
import com.android.internal.telephony.SubscriptionController;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.TelephonyPluginUtils;
import com.android.sprd.telephony.RadioInteractor;

import android.app.AddonManager;
import android.content.Context;
import android.content.Intent;
import android.os.PersistableBundle;
import android.os.UserHandle;
import android.provider.Settings;
import android.telephony.CarrierConfigManagerEx;
import android.telephony.RadioAccessFamily;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManagerEx;
import android.text.TextUtils;
import android.util.Log;

/**
 * Created by SPRD for CMCC new case:Mobile card priority strategy
 */

public class CMCCTelephonyPlugin extends TelephonyPluginUtils implements
        AddonManager.InitialCallback {

    private static final String LOG_TAG = "CMCCTelephonyPlugin";
    private Context mAddonContext;
    private int PHONE_COUNT = TelephonyManager.getDefault().getPhoneCount();

    public CMCCTelephonyPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public void setDefaultDataSubId(Context context, int subId) {
        enforceModifyPhoneState("setDefaultDataSubId",context);

        if (subId == SubscriptionManager.DEFAULT_SUBSCRIPTION_ID) {
            throw new RuntimeException("setDefaultDataSubId called with DEFAULT_SUB_ID");
        }

        if (!needSkipSetRadioCapability(context,subId)) {

            ProxyController proxyController = ProxyController.getInstance();
            Log.d(LOG_TAG, "[setDefaultDataSubId] num phones=" + PHONE_COUNT + ", subId=" + subId);

            if (SubscriptionManager.isValidSubscriptionId(subId)) {
                // Only re-map modems if the new default data sub is valid
                RadioAccessFamily[] rafs = new RadioAccessFamily[PHONE_COUNT];
                boolean atLeastOneMatch = false;
                for (int phoneId = 0; phoneId < PHONE_COUNT; phoneId++) {
                    Phone phone = PhoneFactory.getPhone(phoneId);
                    int raf;
                    int id = phone.getSubId();
                    if (id == subId) {
                        // TODO Handle the general case of N modems and M subscriptions.
                        raf = proxyController.getMaxRafSupported();
                        atLeastOneMatch = true;
                    } else {
                        // TODO Handle the general case of N modems and M subscriptions.
                        raf = proxyController.getMinRafSupported();
                    }
                    Log.d(LOG_TAG, "[setDefaultDataSubId] phoneId=" + phoneId + " subId=" + id + " RAF=" + raf);
                    rafs[phoneId] = new RadioAccessFamily(phoneId, raf);
                }
                if (atLeastOneMatch) {
                    proxyController.setRadioCapability(rafs);
                } else {
                    Log.d(LOG_TAG, "[setDefaultDataSubId] no valid subId's found - not updating.");
                }
            }
        }

        // FIXME is this still needed?
        updateAllDataConnectionTrackers();

        Settings.Global.putInt(context.getContentResolver(),
                Settings.Global.MULTI_SIM_DATA_CALL_SUBSCRIPTION, subId);
        broadcastDefaultDataSubIdChanged(context,subId);
    }

    private void enforceModifyPhoneState(String message,Context context) {
        context.enforceCallingOrSelfPermission(
                android.Manifest.permission.MODIFY_PHONE_STATE, message);
    }

    private void updateAllDataConnectionTrackers() {
        // Tell Phone Proxies to update data connection tracker
        Log.d(LOG_TAG,"[updateAllDataConnectionTrackers] sPhones.length=" + PHONE_COUNT);
        for (int phoneId = 0; phoneId < PHONE_COUNT; phoneId++) {
            Log.d(LOG_TAG,"[updateAllDataConnectionTrackers] phoneId=" + phoneId);
            Phone phone = PhoneFactory.getPhone(phoneId);
            if (phone != null) {
                phone.updateDataConnectionTracker();
            }
        }
    }

    public boolean needSkipSetRadioCapability(Context context,int setSubId) {
        CarrierConfigManagerEx configManagerEx = CarrierConfigManagerEx.from(context);
        PersistableBundle carrierConfig = null;
        if (configManagerEx != null) {
            carrierConfig = configManagerEx.getConfigForDefaultPhone();
        }
        if (carrierConfig != null && !carrierConfig.getBoolean(CarrierConfigManagerEx.KEY_FEATURE_SET_FAKE_SIM_ENABLE_BOOL)) {
            Log.d(LOG_TAG,"fake sim enable false");
            return false;
        }

        RadioInteractor radioInteractor = new RadioInteractor(context);
        SubscriptionController subscriptionController = SubscriptionController.getInstance();
        int setPhoneid =subscriptionController .getPhoneId(setSubId);
        int absentCount = 0;
        int flag = 0;
        boolean [] isCmccCard = new boolean[PHONE_COUNT];

        if (SubscriptionManager.isValidPhoneId(setPhoneid)) {
            for (int i=0 ;i<PHONE_COUNT;i++) {
                String iccid = radioInteractor.getIccID(i);
                Log.d(LOG_TAG,"iccid ="+iccid+", i ="+i);
                if (TextUtils.isEmpty(iccid)) {
                    absentCount ++;
                } else if (iccid.startsWith("898600") || iccid.startsWith("898607")) {
                    flag |= 1 << i;
                    isCmccCard [i] = true;
                } else {
                    flag |= 0 << i;
                }
            }
            Log.d(LOG_TAG,"flag ==" +flag+",setPhoneid ="+setPhoneid+",isCmccCard["+setPhoneid+"]="+isCmccCard[setPhoneid]+",absentCount="+absentCount);
            if (absentCount == 0 && flag >0 && flag < (1<<PHONE_COUNT)) {
                TelephonyManagerEx tmx = TelephonyManagerEx.from(context);
                if (!isCmccCard[setPhoneid] ) {
                    if (tmx.isSimSlotSupportLte(setPhoneid)) {
                        needChangeRadioCapbility(setPhoneid);
                    }
                    return true;
                }
            }
        }
        return false;
    }

    private void needChangeRadioCapbility(int phoneId) {
        RadioAccessFamily[] rafs = new RadioAccessFamily[PHONE_COUNT];
        ProxyController proxyController = ProxyController.getInstance();
        for (int i = 0; i < PHONE_COUNT; i++) {
            int raf;
            if (i == phoneId) {
                raf = proxyController.getMinRafSupported();
            } else {
                raf = proxyController.getMaxRafSupported();
            }
            Log.d(LOG_TAG, "[needChangeRadioCapbility] phoneId=" + i + " RAF=" + raf);
            rafs[i] = new RadioAccessFamily(i, raf);
        }
        proxyController.setRadioCapability(rafs);
    }

    private void broadcastDefaultDataSubIdChanged(Context context,int subId) {
        // Broadcast an Intent for default data sub change
        Log.d(LOG_TAG,"[broadcastDefaultDataSubIdChanged] subId=" + subId);
        Intent intent = new Intent(TelephonyIntents.ACTION_DEFAULT_DATA_SUBSCRIPTION_CHANGED);
        intent.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING);
        intent.putExtra(PhoneConstants.SUBSCRIPTION_KEY, subId);
        context.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
    }

}
