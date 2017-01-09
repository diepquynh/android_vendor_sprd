
package com.android.callsettings.callbarring;

import android.app.ActionBar;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.view.MenuItem;

import com.android.ims.ImsManager;
import com.android.ims.internal.ImsManagerEx;
import com.android.ims.internal.IImsServiceEx;
import com.android.ims.internal.IImsRegisterListener;
import com.android.callsettings.TimeConsumingPreferenceActivity;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.Phone;

import android.content.Context;
import android.content.Intent;

import com.android.callsettings.SubscriptionInfoHelper;
import com.android.callsettings.R;
import java.util.ArrayList;
import com.android.internal.telephony.GsmCdmaPhoneEx;
import com.android.callsettings.plugins.CallSettingsCMCCHelper;

public class CallBarringOptions extends TimeConsumingPreferenceActivity
        implements CallBarringEditPreferencePreferenceListener {
    private static final String LOG_TAG = "CallBarringOptions";
    private final boolean DBG = true;// Debug.isDebug();

    private static final String BUTTON_AO_KEY = "button_ao_key";
    private static final String BUTTON_OI_KEY = "button_oi_key";
    private static final String BUTTON_OX_KEY = "button_ox_key";
    private static final String BUTTON_AI_KEY = "button_ai_key";
    private static final String BUTTON_IR_KEY = "button_ir_key";
    private static final String BUTTON_AB_KEY = "button_ab_key";
    private static final String BUTTON_CHGPWD_KEY = "button_chgpwd_key";
    private static final String KEY_TOGGLE = "toggle";
    private static final String KEY_STATUS = "status";
    private static final String KEY_PASSWORD = "password";

    /* SPRD: add for volte @{ */
    private static final String BUTTON_CALL_OUT = "button_call_out";
    private static final String BUTTON_CALL_IN = "button_call_in";
    private Preference mButtonCallOut;
    private Preference mButtonCallIn;
    /* @} */

    private CallBarringEditPreference mButtonAO;
    private CallBarringEditPreference mButtonOI;
    private CallBarringEditPreference mButtonOX;
    private CallBarringEditPreference mButtonAI;
    private CallBarringEditPreference mButtonIR;
    private CallBarringEditPreference mButtonAB;
    private CallBarringChgPwdPreference mButtonChgPwd;

    private final ArrayList<CallBarringEditPreference> mPreferences =
            new ArrayList<CallBarringEditPreference>();
    private boolean mFirstResume;
    private Bundle mIcicle;
    private GsmCdmaPhoneEx mPhone;
    private SubscriptionInfoHelper mSubscriptionInfoHelper;
    /* SPRD: fix bug590100 @{ */
    private boolean mIsVolteEnable;
    private boolean mIsImsListenerRegistered;
    private IImsServiceEx mIImsServiceEx;
    private Context mContext;
    /* @} */

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        addPreferencesFromResource(R.xml.callbarring_options_ex);

        mSubscriptionInfoHelper = new SubscriptionInfoHelper(this, getIntent());

        mSubscriptionInfoHelper.setActionBarTitle(
                getActionBar(), getResources(), R.string.call_barring_settings);
        mPhone = (GsmCdmaPhoneEx) mSubscriptionInfoHelper.getPhone();
        Log.i(LOG_TAG, "mPhone:" + mPhone);

        final ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            actionBar.setNavigationMode(ActionBar.NAVIGATION_MODE_STANDARD);
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setDisplayShowTitleEnabled(true);
        }

        PreferenceScreen prefSet = getPreferenceScreen();
        /* SPRD: add for volte @{ */
        mButtonCallOut = prefSet.findPreference(BUTTON_CALL_OUT);
        mButtonCallIn = prefSet.findPreference(BUTTON_CALL_IN);
        /* @} */
        mButtonAO = (CallBarringEditPreference) prefSet.findPreference(BUTTON_AO_KEY);
        mButtonOI = (CallBarringEditPreference) prefSet.findPreference(BUTTON_OI_KEY);
        mButtonOX = (CallBarringEditPreference) prefSet.findPreference(BUTTON_OX_KEY);
        mButtonAI = (CallBarringEditPreference) prefSet.findPreference(BUTTON_AI_KEY);
        mButtonIR = (CallBarringEditPreference) prefSet.findPreference(BUTTON_IR_KEY);
        mButtonAB = (CallBarringEditPreference) prefSet.findPreference(BUTTON_AB_KEY);
        mButtonChgPwd = (CallBarringChgPwdPreference) prefSet.findPreference(BUTTON_CHGPWD_KEY);

        mButtonAO.setParentActivity(this, mButtonAO.mReason);
        mButtonOI.setParentActivity(this, mButtonOI.mReason);
        mButtonOX.setParentActivity(this, mButtonOX.mReason);
        mButtonAI.setParentActivity(this, mButtonAI.mReason);
        mButtonIR.setParentActivity(this, mButtonIR.mReason);
        mButtonAB.setParentActivity(this, mButtonAB.mReason);
        mButtonAB.setNeedEcho(false);
        mButtonAB.setToggled(true);
        mButtonChgPwd.setParentActivity(this, 0);

        mPreferences.add(mButtonAO);
        mPreferences.add(mButtonOI);
        mPreferences.add(mButtonOX);
        mPreferences.add(mButtonAI);
        mPreferences.add(mButtonIR);
        /* SPRD: add for volte @{ */
        if (ImsManager.isVolteEnabledByPlatform(this)) {
            prefSet.removePreference(mButtonAO);
            prefSet.removePreference(mButtonOI);
            prefSet.removePreference(mButtonOX);
            prefSet.removePreference(mButtonAI);
            prefSet.removePreference(mButtonIR);
        } else {
            prefSet.removePreference(mButtonCallOut);
            prefSet.removePreference(mButtonCallIn);
        }
        /* @} */
        // we wait to do the initialization until onResume so that the
        // TimeConsumingPreferenceActivity dialog can display as it
        // relies on onResume / onPause to maintain its foreground state.
        mFirstResume = true;
        mIcicle = icicle;
        /* SPRD: fix bug590100 @{ */
        mContext = this;
        if (!CallSettingsCMCCHelper.getInstance(this).isCallOutOptionEnableInVolte()) {
            tryRegisterImsListener();
        }
        /* @} */
    }

    @Override
    public void onResume() {
        super.onResume();

        if (mFirstResume) {
            if (mIcicle == null) {
                if (DBG)
                    Log.d(LOG_TAG, "start to init ");
                /* SPRD: add for volte @{ */
                if (!ImsManager.isVolteEnabledByPlatform(this)) {
                    /* @} */
                    for (CallBarringEditPreference pre : mPreferences) {
                        pre.setListener(this);
                        pre.init(this, false, mPhone);
                    }
                }
            } else {
                /* SPRD: add for volte @{ */
                if (!ImsManager.isVolteEnabledByPlatform(this)) {
                    /* @} */
                    for (CallBarringEditPreference pref : mPreferences) {
                        Bundle bundle = mIcicle.getParcelable(pref.getKey());
                        pref.setToggled(bundle.getBoolean(KEY_TOGGLE));
                        CallBarringInfo cb = new CallBarringInfo();
                        cb.password = bundle.getString(KEY_PASSWORD);
                        cb.status = bundle.getInt(KEY_STATUS);
                        pref.handleCallBarringResult(cb);
                        pref.init(this, true, mPhone);
                    }
                }
            }
            mButtonAB.setListener(this);
            mButtonAB.init(this, true, mPhone);
            mButtonAB.setEnabled(true);
            mButtonChgPwd.init(this, false, mPhone);
            mFirstResume = false;
            mIcicle = null;
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        for (CallBarringEditPreference pref : mPreferences) {
            Bundle bundle = new Bundle();
            bundle.putBoolean(KEY_TOGGLE, pref.isToggled());
            if (pref.mCallBarringInfo != null) {
                bundle.putString(KEY_PASSWORD, pref.mCallBarringInfo.password);
                bundle.putInt(KEY_STATUS, pref.mCallBarringInfo.status);
            }
            outState.putParcelable(pref.getKey(), bundle);
        }
    }

    @Override
    public void onUpdate(int reason) {
        Log.d(LOG_TAG, "onUpdate, reason:  " + reason);
        updateSummary(reason);
        super.onUpdate(reason);
    }

    @Override
    public void onFinished(Preference preference, boolean reading) {
        super.onFinished(preference, reading);
    }

    public void onChange(Preference preference, int reason) {
        if (DBG)
            Log.d(LOG_TAG, "onChange, reason:  " + reason);
        if (!((CallBarringEditPreference) preference).getNeedEcho()) {
            cancelAll();
        } else {
            ((CallBarringEditPreference) preference).queryCallBarringAfterSet(this, reason);
        }
    }

    public Phone getPhone() {
        return mPhone;
    }

    public void updateSummary(int reason) {
        switch (reason) {
            case GsmCdmaPhoneEx.CB_REASON_AO:
                handleCallBarringResult(mButtonOI);
                handleCallBarringResult(mButtonOX);
                break;
            case GsmCdmaPhoneEx.CB_REASON_AI:
                handleCallBarringResult(mButtonIR);
                break;
            case GsmCdmaPhoneEx.CB_REASON_OI:
            case GsmCdmaPhoneEx.CB_REASON_OX:
                handleCallBarringResult(mButtonAO);
                break;
            case GsmCdmaPhoneEx.CB_REASON_IR:
                handleCallBarringResult(mButtonAI);
                break;
        }
    }

    public void cancelAll() {
        for (CallBarringEditPreference pre : mPreferences) {
            if (pre.mCallBarringInfo.status == 1) {
                handleCallBarringResult(pre);
            }
        }
    }

    public void handleCallBarringResult(CallBarringEditPreference ePreference) {
        ePreference.mCallBarringInfo.status = 0;
        ePreference.setToggled(false);
        ePreference.setPassWord(null);
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
        }
        return super.onOptionsItemSelected(item);
    };

    /* SPRD: add for VOLTE@{ */
    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        if (preference == mButtonCallIn) {
            /* SPRD: bug #549172 @{ */
            new Thread(new Runnable() {
                @Override
                public void run() {
                    if (DBG)
                        Log.d(LOG_TAG, "incoming barring.....");
                    mPhone.queryFacilityLock(CommandsInterface.CB_FACILITY_BA_MT, "",
                            CommandsInterface.SERVICE_CLASS_VOICE, null);
                    if (DBG)
                        Log.d(LOG_TAG, "AT cmd sent");
                }
            }).start();
            /* @} */
            // SPRD: fix bug 457829
            final Intent intent = getNewIntent();
            intent.setClassName("com.android.callsettings",
                    "com.android.callsettings.callbarring.CallInBarringEditPreference");
            startActivity(intent);
        } else if (preference == mButtonCallOut) {
            /* SPRD: bug #541136 @{ */
            new Thread(new Runnable() {
                @Override
                public void run() {
                    if (DBG)
                        Log.d(LOG_TAG, "outgoing barring.....");
                    mPhone.queryFacilityLock(CommandsInterface.CB_FACILITY_BA_MO, "",
                            CommandsInterface.SERVICE_CLASS_VOICE, null);
                    if (DBG)
                        Log.d(LOG_TAG, "AT cmd sent");
                }
            }).start();
            /* @} */

            // SPRD: fix bug 457829
            final Intent intent = getNewIntent();
            intent.setClassName("com.android.callsettings",
                    "com.android.callsettings.callbarring.CallOutBarringEditPreference");
            startActivity(intent);
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    /* @} */

    /* SPRD: fix bug 457829 @{ */
    private Intent getNewIntent() {
        Intent intent = new Intent();

        if (getIntent() == null) {
            return intent;
        }

        int subId = getIntent().getIntExtra(SubscriptionInfoHelper.SUB_ID_EXTRA,
                SubscriptionInfoHelper.NO_SUB_ID);
        String subLabel = getIntent().getStringExtra(SubscriptionInfoHelper.SUB_LABEL_EXTRA);
        intent.putExtra(SubscriptionInfoHelper.SUB_ID_EXTRA, subId);
        intent.putExtra(SubscriptionInfoHelper.SUB_LABEL_EXTRA, subLabel);
        return intent;
    }
    /* @} */

    /* SPRD: fix bug590100 @{ */
    private synchronized void tryRegisterImsListener(){
        if(ImsManager.isVolteEnabledByPlatform(mContext)){
            mIImsServiceEx = ImsManagerEx.getIImsServiceEx();
            if(mIImsServiceEx != null){
                try{
                    if(!mIsImsListenerRegistered){
                        mIsImsListenerRegistered = true;
                        mIImsServiceEx.registerforImsRegisterStateChanged(mImsUtListenerExBinder);
                    }
                }catch(RemoteException e){
                    Log.e(LOG_TAG, "regiseterforImsException", e);
                }
            }
        }
    }

    private void unRegisterImsListener(){
        if(ImsManager.isVolteEnabledByPlatform(mContext)){
            try{
                if(mIsImsListenerRegistered){
                    mIsImsListenerRegistered = false;
                    mIImsServiceEx.unregisterforImsRegisterStateChanged(mImsUtListenerExBinder);
                }
            }catch(RemoteException e){
                Log.e(LOG_TAG, "unRegisterImsListener", e);
            }
        }
    }

    private final IImsRegisterListener.Stub mImsUtListenerExBinder = new IImsRegisterListener.Stub(){
        @Override
        public void imsRegisterStateChange(boolean isRegistered){
            if(mIsVolteEnable != isRegistered){
                mIsVolteEnable = isRegistered;
                if (!CallSettingsCMCCHelper.getInstance(mContext).isCallOutOptionEnableInVolte()
                        && mIsVolteEnable
                        && mButtonCallOut != null) {
                    mButtonCallOut.setEnabled(false);
                } else {
                    mButtonCallOut.setEnabled(true);
                }
            }
        }
    };

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unRegisterImsListener();
    }
    /* @} */
}
