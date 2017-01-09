package plugin.sprd.teleservicesupportoperator;

import android.app.AddonManager;
import android.content.Context;
import android.telephony.RadioAccessFamily;
import android.util.Log;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.database.ContentObserver;
import android.provider.Settings;
import android.provider.Settings.Global;
import android.os.Handler;
import android.os.UserHandle;

import com.android.internal.telephony.SubscriptionController;
import com.android.internal.telephony.PhoneFactory;
import com.sprd.phone.TeleServicePluginsHelper;

/**
 * Plugin implementation for telephony service related modification.
 * @Author SPRD
 */
public class TeleServiceSupportOperator extends TeleServicePluginsHelper implements
        AddonManager.InitialCallback {

    private Context mAddonContext;
    private PreferenceScreen mPrefSet;
    private SwitchPreference mDataSwitch;


    public TeleServiceSupportOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public PreferenceScreen needShwoDataSwitch(PreferenceScreen prefSet,SwitchPreference dataSwitch,int subId,Context context) {
        mPrefSet = prefSet;
        mDataSwitch = dataSwitch;
        SubscriptionManager subscirptionManager = SubscriptionManager.from(context);
        if (mPrefSet != null && mDataSwitch != null && subscirptionManager.getDefaultDataSubscriptionId() == subId) {
            mPrefSet.addPreference(dataSwitch);
        }
        return mPrefSet;
    }

    public boolean needShowNetworkType(Context context,int subId) {
        int phoneId = SubscriptionManager.getPhoneId(subId);
        if (SubscriptionManager.isValidPhoneId(phoneId)) {
            int rafMax = PhoneFactory.getPhone(phoneId).getRadioAccessFamily();
            return (rafMax & RadioAccessFamily.RAF_LTE) == RadioAccessFamily.RAF_LTE;
        }
        return false;
    }

    public boolean removeNetworkType (Context context,int subId) {
            return !needShowNetworkType(context,subId) && subId == SubscriptionManager.getDefaultDataSubscriptionId();
    }

    public SwitchPreference updateDataSwitch(SwitchPreference dataSwitch,int subId) {
        mDataSwitch = dataSwitch;
        if (mDataSwitch != null) {
            TelephonyManager telephonyManager = TelephonyManager.from(mAddonContext);
            SubscriptionManager subscirptionManager = SubscriptionManager.from(mAddonContext);
            boolean isDataEnable = telephonyManager.getDataEnabled(subId);
            int phoneId = SubscriptionManager.getPhoneId(subId);
            Log.d("TeleServiceSupportOperator","updateDataSwitch subId ="+subId+",phoneId="+phoneId+",isDataEnable="+isDataEnable);
            mDataSwitch.setChecked(isDataEnable);
            boolean canSetDataEnable = (SubscriptionManager
                    .getSimStateForSlotIdx(phoneId) == TelephonyManager.SIM_STATE_READY);
            Log.d("TeleServiceSupportOperator","updateDataSwitch canSetDataEnable="+canSetDataEnable);
            mDataSwitch.setEnabled(canSetDataEnable);
        }
        return mDataSwitch;
    }

    private ContentObserver mMobileDataObserver = new ContentObserver(new Handler()) {
        @Override
        public void onChange(boolean selfChange) {
            SubscriptionManager subscirptionManager = SubscriptionManager.from(mAddonContext);
            int subId = subscirptionManager.getDefaultDataSubscriptionId();
            updateDataSwitch(mDataSwitch,subId);
        }
    };

    public void needRegisterMobileData(Context context) {
        if (context != null) {
            context.getContentResolver().registerContentObserver(
                    Settings.Global.getUriFor(Settings.Global.MOBILE_DATA +SubscriptionManager.MAX_SUBSCRIPTION_ID_VALUE), true,
                    mMobileDataObserver,UserHandle.USER_OWNER);
        }
    }

    public void needUnregisterMobieData(Context context) {
        if (context != null) {
            context.getContentResolver().unregisterContentObserver(mMobileDataObserver);
        }
    }

    public SwitchPreference needSetDataEnable(SwitchPreference beforePref,int subId) {
        mDataSwitch = beforePref;
        TelephonyManager tm = TelephonyManager.from(mAddonContext);
        boolean isDataEnable = tm.getDataEnabled(subId);
        Log.d("TeleServiceSupportOperator","needSetDataEnable checked = "+mDataSwitch.isChecked()+",subId="+subId+",isDataEnable="+isDataEnable);
        if ((!mDataSwitch.isChecked()) != isDataEnable) {
            tm.setDataEnabled(subId, !mDataSwitch.isChecked());
            mDataSwitch.setChecked(!mDataSwitch.isChecked());
        }
        return mDataSwitch;
    }
}
