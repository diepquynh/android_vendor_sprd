package plugin.sprd.keyguardaddon;

import android.app.AddonManager;
import android.content.Context;
import android.content.Intent;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManagerEx;
import android.util.Log;

import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.IccCardConstantsEx;
import com.android.internal.telephony.IccCardConstantsEx.State;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.PhoneConstants;
import com.android.keyguard.KeyguardSupportSimlock;
import com.android.sprd.telephony.uicc.IccCardStatusEx;

import java.util.HashMap;

public class KeyguardPluginForSimlock extends KeyguardSupportSimlock implements
        AddonManager.InitialCallback {

    private String TAG = "KeyguardPluginForSimlock";
    private Context mAddonContext;
    private TelephonyManager mTelephonyManager;
    private HashMap<Integer, SimDataEx> mSimDatasEx = new HashMap<Integer, SimDataEx>();

    public KeyguardPluginForSimlock() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mTelephonyManager = TelephonyManager.getDefault();
        mAddonContext = context;
        return clazz;
    }

    public String getSimLockStatusString(int slotId){
        int id = -1;
        int state = TelephonyManagerEx.getSimStateEx(slotId);
        if (TelephonyManager.SIM_STATE_NETWORK_LOCKED == state) {
            id = com.android.plugin.keyguard.R.string.simlock_pn_locked;
        } else if (IccCardStatusEx.SIM_STATE_NETWORKSUBSET_LOCKED == state) {
            id = com.android.plugin.keyguard.R.string.simlock_pu_locked;
        } else if (IccCardStatusEx.SIM_STATE_SERVICEPROVIDER_LOCKED == state) {
            id = com.android.plugin.keyguard.R.string.simlock_pp_locked;
        } else if (IccCardStatusEx.SIM_STATE_CORPORATE_LOCKED == state) {
            id = com.android.plugin.keyguard.R.string.simlock_pc_locked;
        } else if (IccCardStatusEx.SIM_STATE_SIM_LOCKED == state) {
            id = com.android.plugin.keyguard.R.string.simlock_ps_locked;
        } else if (IccCardStatusEx.SIM_STATE_NETWORK_LOCKED_PUK == state) {
            id = com.android.plugin.keyguard.R.string.simlock_pn_puk_locked;
        } else if (IccCardStatusEx.SIM_STATE_NETWORK_SUBSET_LOCKED_PUK == state) {
            id = com.android.plugin.keyguard.R.string.simlock_pu_puk_locked;
        } else if (IccCardStatusEx.SIM_STATE_SERVICE_PROVIDER_LOCKED_PUK == state) {
            id = com.android.plugin.keyguard.R.string.simlock_pp_puk_locked;
        } else if (IccCardStatusEx.SIM_STATE_CORPORATE_LOCKED_PUK == state) {
            id = com.android.plugin.keyguard.R.string.simlock_pc_puk_locked;
        } else if (IccCardStatusEx.SIM_STATE_SIM_LOCKED_PUK == state) {
            id = com.android.plugin.keyguard.R.string.simlock_ps_puk_locked;
        } else if (IccCardStatusEx.SIM_STATE_SIM_LOCKED_FOREVER == state) {
            id = com.android.plugin.keyguard.R.string.simlock_forever_locked;
        }
        if (id == -1) return null;
        String simlockStatus = mAddonContext.getResources().getString(id);
        Log.d(TAG, "getSimLockStatus = "+simlockStatus );
        return simlockStatus ;
    }
    /* @} */

    public boolean isSimlockStatusChange (Intent intent) {
        boolean ret = false;
        State state;
        if (!TelephonyIntents.ACTION_SIM_STATE_CHANGED.equals(intent.getAction())) {
            throw new IllegalArgumentException("only handles intent ACTION_SIM_STATE_CHANGED");
        }
        int slotId = intent.getIntExtra(PhoneConstants.SLOT_KEY, 0);
        int subId = intent.getIntExtra(PhoneConstants.SUBSCRIPTION_KEY,
                SubscriptionManager.INVALID_SUBSCRIPTION_ID);
        String stateExtra = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
        if (IccCardConstants.INTENT_VALUE_ICC_LOCKED.equals(stateExtra)) {
            final String lockedReason = intent
                    .getStringExtra(IccCardConstants.INTENT_KEY_LOCKED_REASON);
            if (IccCardConstants.INTENT_VALUE_LOCKED_NETWORK.equals(lockedReason)) {
                state = State.NETWORK_LOCKED;
            } else if (IccCardConstantsEx.INTENT_VALUE_LOCKED_NS.equals(lockedReason)) {
                state = State.NETWORK_SUBSET_LOCKED;
            } else if (IccCardConstantsEx.INTENT_VALUE_LOCKED_SP.equals(lockedReason)) {
                state = State.SERVICE_PROVIDER_LOCKED;
            } else if (IccCardConstantsEx.INTENT_VALUE_LOCKED_CP.equals(lockedReason)) {
                state = State.CORPORATE_LOCKED;
            } else if (IccCardConstantsEx.INTENT_VALUE_LOCKED_SIM.equals(lockedReason)) {
                state = State.SIM_LOCKED;
            } else if (IccCardConstantsEx.INTENT_VALUE_LOCKED_NETWORK_PUK.equals(lockedReason)) {
                state = State.NETWORK_LOCKED_PUK;
            } else if (IccCardConstantsEx.INTENT_VALUE_LOCKED_NS_PUK.equals(lockedReason)) {
                state = State.NETWORK_SUBSET_LOCKED_PUK;
            } else if (IccCardConstantsEx.INTENT_VALUE_LOCKED_SP_PUK.equals(lockedReason)) {
                state = State.SERVICE_PROVIDER_LOCKED_PUK;
            } else if (IccCardConstantsEx.INTENT_VALUE_LOCKED_CP_PUK.equals(lockedReason)) {
                state = State.CORPORATE_LOCKED_PUK;
            } else if (IccCardConstantsEx.INTENT_VALUE_LOCKED_SIM_PUK.equals(lockedReason)) {
                state = State.SIM_LOCKED_PUK;
            } else if (IccCardConstantsEx.INTENT_VALUE_LOCKED_FOREVER.equals(lockedReason)) {
                state = State.SIM_LOCKED_FOREVER;
            } else {
                state = State.UNKNOWN;
            }
        } else {
            state = State.UNKNOWN;
        }
        SimDataEx data = mSimDatasEx.get(subId);
        if (data == null) {
            data = new SimDataEx(state, slotId, subId);
            mSimDatasEx.put(subId, data);
            if (state != State.UNKNOWN) {
                ret = true;
            }
        } else {
            if (data.simState != state) {
                data.simState = state;
                ret = true;
            }
            data.subId = subId;
            data.slotId = slotId;
        }
        return ret;
    }

    public State getSimStateEx(int subId) {
        if (mSimDatasEx.containsKey(subId)) {
            return mSimDatasEx.get(subId).simState;
        } else {
            return State.UNKNOWN;
        }
    }

    private static class SimDataEx {
        public State simState;
        public int slotId;
        public int subId;

        SimDataEx(State state, int slot, int id) {
            simState = state;
            slotId = slot;
            subId = id;
        }

        public String toString() {
            return "SimDataEx{state=" + simState + ",slotId=" + slotId + ",subId=" + subId + "}";
        }
    }
}
