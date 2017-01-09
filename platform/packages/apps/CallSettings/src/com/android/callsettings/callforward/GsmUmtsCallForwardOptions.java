package com.android.callsettings.callforward;

import com.android.ims.ImsManager;
import com.android.ims.ImsCallForwardInfo;
import com.android.internal.telephony.CallForwardInfo;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.Phone;
import com.android.callsettings.callforward.CallForwardTimeEditPreFragement;
import com.android.callsettings.TimeConsumingPreferenceListener;

import android.app.ActionBar;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.database.Cursor;
import android.os.Bundle;
import android.os.RemoteException;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.telephony.PhoneStateListener;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.VoLteServiceState;
import android.util.Log;
import android.view.MenuItem;

import java.util.ArrayList;
import com.android.ims.internal.ImsManagerEx;
import com.android.ims.internal.IImsServiceEx;
import com.android.ims.internal.IImsRegisterListener;
import com.android.callsettings.TimeConsumingPreferenceActivity;
import com.android.callsettings.SubscriptionInfoHelper;
import com.android.callsettings.R;
import com.android.callsettings.plugins.CallSettingsCMCCHelper;

import static com.android.callsettings.TimeConsumingPreferenceActivity.FDN_CHECK_FAILURE;


public class GsmUmtsCallForwardOptions extends TimeConsumingPreferenceActivity
implements CallForwardTimeEditPreFragement.Listener {
    private static final String LOG_TAG = "GsmUmtsCallForwardOptions";
    private final boolean DBG = true;

    private static final String NUM_PROJECTION[] = {
        android.provider.ContactsContract.CommonDataKinds.Phone.NUMBER
    };

    private static final String BUTTON_CFU_KEY   = "button_cfu_key";
    private static final String BUTTON_CFB_KEY   = "button_cfb_key";
    private static final String BUTTON_CFNRY_KEY = "button_cfnry_key";
    private static final String BUTTON_CFNRC_KEY = "button_cfnrc_key";
    // SPRD: add for callforward time
    private static final String BUTTON_CFT_KEY = "button_cft_key";

    /* SPRD: function VOLTE call forward query support. @{ */

    private static final String KEY_TOGGLE = "toggle";
    private static final String KEY_STATUS = "status";
    private static final String KEY_NUMBER = "number";

    private CallForwardEditPreference mButtonCFU;
    private CallForwardEditPreference mButtonCFB;
    private CallForwardEditPreference mButtonCFNRy;
    private CallForwardEditPreference mButtonCFNRc;

    /* SPRD: add for callforward time @{ */
    private Preference mButtonCFT;
    SharedPreferences mPrefs;
    public static final String PREF_PREFIX = "phonecalltimeforward_";
    TimeConsumingPreferenceListener tcpListener;
    Context mContext;
    private int mPhoneId = 0;
    /* @} */
    /* SPRD: add for bug 478880 @{ */
    private static final int CFU_PREF_REASON = 0;
    private static final String CFT_STATUS_ACTIVE = "1";
    /* @} */
    // SPRD: modify for bug544093
    private static final int CF_AUDIO_SERVICE_CALSS = 1;

    private final ArrayList<CallForwardEditPreference> mPreferences =
            new ArrayList<CallForwardEditPreference> ();
    private int mInitIndex= 0;

    private boolean mFirstResume;
    private Bundle mIcicle;
    private Phone mPhone;
    private SubscriptionInfoHelper mSubscriptionInfoHelper;
    // SPRD: modify for bug552643
    private int mSubId = 0;
    private boolean mIsVolteEnable;
    private boolean mIsImsListenerRegistered;
    private IImsServiceEx mIImsServiceEx;
    private boolean mIsFDNOn;

    //SPRD: add for bug612905
    private boolean mHasMultiWindow = false;

    /* SPRD: add for bug626699 @{ */
    private static final String KEY_PLMNS_CALLFORWARD_ONLY = "plmns_callforward_only";
    private boolean mOnlySupportVoiceCall = false;
    /* @} */

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        /* SPRD: add for callforward time @{ */
        mContext = this.getApplicationContext();
        addPreferencesFromResource(R.xml.callforward_options_ex);

        mSubscriptionInfoHelper = new SubscriptionInfoHelper(this, getIntent());
        mSubscriptionInfoHelper.setActionBarTitle(
                getActionBar(), getResources(), R.string.call_forwarding_settings_with_label);
        //SPRD: add for bug626699
        mOnlySupportVoiceCall = getIntent().getBooleanExtra(KEY_PLMNS_CALLFORWARD_ONLY, false);
        Log.d(LOG_TAG, "mOnlySupportVoiceCall = " + mOnlySupportVoiceCall);
        mPhone = mSubscriptionInfoHelper.getPhone();
        // SPRD: add for callforward time
        mPhoneId = mPhone.getPhoneId();
        // SPRD: modify for bug552643
        mSubId = mPhone.getSubId();

        PreferenceScreen prefSet = getPreferenceScreen();
        mButtonCFU = (CallForwardEditPreference) prefSet.findPreference(BUTTON_CFU_KEY);
        mButtonCFB = (CallForwardEditPreference) prefSet.findPreference(BUTTON_CFB_KEY);
        mButtonCFNRy = (CallForwardEditPreference) prefSet.findPreference(BUTTON_CFNRY_KEY);
        mButtonCFNRc = (CallForwardEditPreference) prefSet.findPreference(BUTTON_CFNRC_KEY);
        tryRegisterImsListener();

        mButtonCFU.setParentActivity(this, mButtonCFU.reason);
        mButtonCFB.setParentActivity(this, mButtonCFB.reason);
        mButtonCFNRy.setParentActivity(this, mButtonCFNRy.reason);
        mButtonCFNRc.setParentActivity(this, mButtonCFNRc.reason);

        mPreferences.add(mButtonCFU);
        mPreferences.add(mButtonCFB);
        mPreferences.add(mButtonCFNRy);
        mPreferences.add(mButtonCFNRc);

        /* SPRD: add for callforward time @{ */
        mButtonCFT = prefSet.findPreference(BUTTON_CFT_KEY);
        mPrefs = mContext.getSharedPreferences(PREF_PREFIX + mPhoneId,
                mContext.MODE_PRIVATE);
        if (ImsManager.isVolteEnabledByPlatform(mContext)) {
            // SPRD: modify for bug552643
            CallForwardTimeEditPreFragement.addListener(this);
            tcpListener = this;
        } else{
            if (mButtonCFT != null && prefSet != null) {
                prefSet.removePreference(mButtonCFT);
            }
        }
        /* @} */

        // we wait to do the initialization until onResume so that the
        // TimeConsumingPreferenceActivity dialog can display as it
        // relies on onResume / onPause to maintain its foreground state.

        mFirstResume = true;
        mIcicle = icicle;

        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            // android.R.id.home will be triggered in onOptionsItemSelected()
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        // SPRD: modify for bug539931
        refreshCFTButton();
        /* SPRD: add for bug612905 @{ */
        if (mHasMultiWindow || isInMultiWindowMode()) {
            if (DBG) {
                Log.d(LOG_TAG, "now is in multi-mode or quit the multi-mode");
            }
            mInitIndex = 0;
            // SPRD: add for bug626699
            mPreferences.get(mInitIndex).init(this, false, mPhone, mOnlySupportVoiceCall);
            mHasMultiWindow = false;
        } else {
        /* @} */
            if (mFirstResume) {
                if (mIcicle == null) {
                    if (DBG) {
                        Log.d(LOG_TAG, "start to init ");
                    }
                    // SPRD: add for bug626699
                    mPreferences.get(mInitIndex).init(this, false, mPhone, mOnlySupportVoiceCall);
                } else {
                    mInitIndex = mPreferences.size();

                    for (CallForwardEditPreference pref : mPreferences) {
                        Bundle bundle = mIcicle.getParcelable(pref.getKey());
                        pref.setToggled(bundle.getBoolean(KEY_TOGGLE));
                        CallForwardInfo cf = new CallForwardInfo();
                        cf.number = bundle.getString(KEY_NUMBER);
                        cf.status = bundle.getInt(KEY_STATUS);
                        // SPRD: modify for bug594590
                        pref.init(this, true, mPhone, mOnlySupportVoiceCall);
                        pref.handleCallForwardResult(cf);

                    }
                }
                mFirstResume = false;
                mIcicle = null;
            }
        }

        /* SPRD: add for bug 478880 @{ */
        if (ImsManager.isVolteEnabledByPlatform(mContext)) {
            updateCFTSummaryText();
        }
        /* @} */
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        for (CallForwardEditPreference pref : mPreferences) {
            Bundle bundle = new Bundle();
            bundle.putBoolean(KEY_TOGGLE, pref.isToggled());
            if (pref.callForwardInfo != null) {
                bundle.putString(KEY_NUMBER, pref.callForwardInfo.number);
                bundle.putInt(KEY_STATUS, pref.callForwardInfo.status);
            }
            outState.putParcelable(pref.getKey(), bundle);
        }
    }

    /* SPRD: add for bug612905 @{ */
    @Override
    public void onMultiWindowModeChanged(boolean isInMultiWindowMode) {
        // Left deliberately empty. There should be no side effects if a direct
        // subclass of Activity does not call super.
        Log.d(LOG_TAG, "onMultiWindowModeChanged");
        if (!isInMultiWindowMode) {
            mHasMultiWindow = true;
        }
    }
    /* @} */
    @Override
    public void onFinished(Preference preference, boolean reading) {
        if (mInitIndex < mPreferences.size()-1 && !isFinishing()) {
            mInitIndex++;
            // SPRD: add for bug626699
            mPreferences.get(mInitIndex).init(this, false, mPhone, mOnlySupportVoiceCall);
        } else if (mInitIndex == mPreferences.size()-1 && !isFinishing()) {
            if (mIsVolteEnable
                    || CallSettingsCMCCHelper.getInstance(this)
                    .isCallTimeForwardSupportIn3g2g()) {
                mInitIndex++;
                mPreferences.get(0).initCallTimeForward();
            }
        }
        super.onFinished(preference, reading);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (DBG) Log.d(LOG_TAG, "onActivityResult: done");
        if (resultCode != RESULT_OK) {
            if (DBG) Log.d(LOG_TAG, "onActivityResult: contact picker result not OK.");
            return;
        }
        Cursor cursor = null;
        try {
            cursor = getContentResolver().query(data.getData(),
                NUM_PROJECTION, null, null, null);
            if ((cursor == null) || (!cursor.moveToFirst())) {
                if (DBG) Log.d(LOG_TAG, "onActivityResult: bad contact data, no results found.");
                return;
            }

                switch (requestCode) {
                    case CommandsInterface.CF_REASON_UNCONDITIONAL:
                        mButtonCFU.onPickActivityResult(cursor.getString(0));
                        break;
                    case CommandsInterface.CF_REASON_BUSY:
                        mButtonCFB.onPickActivityResult(cursor.getString(0));
                        break;
                    case CommandsInterface.CF_REASON_NO_REPLY:
                        mButtonCFNRy.onPickActivityResult(cursor.getString(0));
                        break;
                    case CommandsInterface.CF_REASON_NOT_REACHABLE:
                        mButtonCFNRc.onPickActivityResult(cursor.getString(0));
                        break;
                    default:
                        break;
                }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        final int itemId = item.getItemId();
        if (itemId == android.R.id.home) {  // See ActionBar#setDisplayHomeAsUpEnabled()
            // SPRD: add for bug 611966
            GsmUmtsAllCallForwardOptions.goUpToTopLevelSetting(this, mSubscriptionInfoHelper);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    /* SPRD: add for bug544979 @{ */
    @Override
    public void onBackPressed() {
        // SPRD: add for bug 611966
        GsmUmtsAllCallForwardOptions.goUpToTopLevelSetting(this, mSubscriptionInfoHelper);
    }
    /* @} */

    /* SPRD: add for callforward time @{ */
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (ImsManager.isVolteEnabledByPlatform(mContext)) {
            CallForwardTimeEditPreFragement.removeListener(this);
        }
        unRegisterImsListener();
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        if (preference == mButtonCFT) {
            final Intent intent = new Intent();
            intent.setClassName("com.android.callsettings",
                    "com.android.callsettings.callforward.CallForwardTimeEditPreference");
            intent.putExtra("phone_id", String.valueOf(mPhoneId));
            startActivity(intent);
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }


    @Override
    public void onCallForawrdTimeStateChanged(String number){
        mInitIndex = 0;
        mPreferences.get(mInitIndex).init(this, false, mPhone);
        /* SPRD: add for bug 478880 @{ */
        updateCFTSummaryText();
        if (number != null) {
            /* SPRD: add for bug552643 @{ */
            SharedPreferences prefs = mContext.getSharedPreferences(
                CallForwardEditPreference.PREF_PREFIX + mPhoneId, mContext.MODE_PRIVATE);
            mButtonCFU.saveStringPrefs(
                    CallForwardEditPreference.PREF_PREFIX + mPhoneId + "_" + CFU_PREF_REASON,
                    number, prefs);
        }
        /* @} */
        /* @} */
    }

    /**
     * SPRD: add for bug 478880 @{
     *
     */
    private void updateCFTSummaryText() {
        CharSequence mSummary;
        if (ImsManager.isVolteEnabledByPlatform(mContext)) {
            // SPRD: modify for bug539931
            if (CFT_STATUS_ACTIVE.equals(mPrefs.getString(PREF_PREFIX
                    + "status_" + mPhoneId, ""))) {
                mSummary = mPrefs
                        .getString(PREF_PREFIX + "num_" + mPhoneId, "");
            } else {
                mSummary = mContext.getText(R.string.sum_cft_disabled);
            }
            mButtonCFT.setSummary(mSummary);
        }
    }
    /** @} */

    public void savePrefData(String key, String value) {
        Log.w(LOG_TAG, "savePrefData(" + key + ", " + value + ")");
        if (mPrefs != null) {
            try {
                SharedPreferences.Editor editor = mPrefs.edit();
                editor.putString(key, value);
                editor.apply();
            } catch (Exception e) {
                Log.e(LOG_TAG, "Exception happen.");
            }
        }
    }

    @Override
    public void onEnableStatus(Preference preference, int status) {
        if (status == 1) {
            if (mButtonCFT != null) {
                mButtonCFT.setEnabled(false);
            }
        } else if (ImsManager.isVolteEnabledByPlatform(mContext)) {
            refreshCFTButton();
            /* SPRD: add for bug549240 @{ */
            if (CFT_STATUS_ACTIVE.equals(
                    mPrefs.getString(PREF_PREFIX + "status_" + mPhoneId, ""))) {
                mButtonCFU.setEnabled(false);
            }
            /* @} */
            // SPRD: modify for bug552643
            updateCFTSummaryText();
        }
        if (preference == mButtonCFU) {
            Log.i(LOG_TAG, "onEnableStatus...status = " + status);
            updatePrefCategoryEnabled(mButtonCFU);
        }
    }
    /* @} */

    public static boolean checkVideoCallServiceClass(int sc) {
        return (sc & CommandsInterface.SERVICE_CLASS_DATA) != 0
            ||(sc & CommandsInterface.SERVICE_CLASS_DATA_SYNC) != 0
            ||(sc & CommandsInterface.SERVICE_CLASS_DATA_ASYNC) != 0
            ||(sc & CommandsInterface.SERVICE_CLASS_PACKET) != 0
            ||(sc & CommandsInterface.SERVICE_CLASS_PAD) != 0;
    }

    public static boolean checkServiceClassSupport(int sc) {
        return (sc & CommandsInterface.SERVICE_CLASS_DATA) != 0
            ||(sc & CommandsInterface.SERVICE_CLASS_DATA_SYNC) != 0
            ||(sc & CommandsInterface.SERVICE_CLASS_DATA_ASYNC) != 0
            ||(sc & CommandsInterface.SERVICE_CLASS_PACKET) != 0
            ||(sc & CommandsInterface.SERVICE_CLASS_PAD) != 0
            ||(sc & CommandsInterface.SERVICE_CLASS_VOICE) != 0;
    }

    private void updatePrefCategoryEnabled(Preference preference) {
        if(preference == mButtonCFU){
            if(mButtonCFU.isToggled()){
                mButtonCFB.setEnabled(false);
                mButtonCFNRc.setEnabled(false);
                mButtonCFNRy.setEnabled(false);
            }else{
                mButtonCFB.setEnabled(true);
                mButtonCFNRc.setEnabled(true);
                mButtonCFNRy.setEnabled(true);
            }
        }
    }
    /* SPRD: function VOLTE call forward query support. @{ */

    @Override
    public void onError(Preference preference, int error) {
        Log.d(LOG_TAG, "onError, preference=" + preference.getKey() + ", error=" + error);
        /* SPRD: add for bug495303 @{ */
        CallForwardEditPreference pref = null;
        if (preference instanceof CallForwardEditPreference) {
            pref = (CallForwardEditPreference)preference;
            if (pref != null && !pref.isInitializing()) {
                pref.setEnabled(false);
            }
        }
        if (pref != null && !pref.isInitializing()) {
            super.onError(preference,error);
        }
        /* @} */

        if (error == FDN_CHECK_FAILURE) {
            mIsFDNOn = true;
        } else {
            mIsFDNOn = false;
        }
        if (mInitIndex == mPreferences.size()-1) {
            refreshCFTButton();
        }
    }

    /* SPRD: add for bug539931 @{ */
    public void refreshCFTButton() {
        if (ImsManager.isVolteEnabledByPlatform(mContext) && mButtonCFT != null) {
            if ((mButtonCFU.isToggled()
                    && !CFT_STATUS_ACTIVE.equals(
                            mPrefs.getString(PREF_PREFIX + "status_" + mPhoneId, "")))
                    // SPRD: modify for bug544925 && 552776
                        || (!mIsVolteEnable && !CallSettingsCMCCHelper.getInstance(this)
                                .isCallTimeForwardSupportIn3g2g())
                        || mIsFDNOn) {
                mButtonCFT.setEnabled(false);
            } else {
                mButtonCFT.setEnabled(true);
            }
        }
    }
    /* @} */

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
                // SPRD: modify for bug594515
                final SubscriptionManager subscriptionManager = SubscriptionManager.from(mContext);
                mIsVolteEnable = isRegistered
                        && (mSubId == subscriptionManager.getDefaultDataSubscriptionId());
                refreshCFTButton();
            }
        }
    };
}
