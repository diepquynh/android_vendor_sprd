package com.android.callsettings.callforward;

import com.android.ims.ImsManager;
import com.android.ims.ImsCallForwardInfo;
import com.android.internal.telephony.CallForwardInfo;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.Phone;
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
import android.telephony.TelephonyManager;
import android.telephony.VoLteServiceState;
import android.util.Log;
import android.view.MenuItem;

import java.util.ArrayList;
import com.android.ims.internal.ImsCallForwardInfoEx;
import com.android.ims.internal.ImsManagerEx;
import com.android.ims.internal.IImsServiceEx;
import com.android.ims.internal.IImsRegisterListener;
import com.android.callsettings.TimeConsumingPreferenceActivity;
import com.android.callsettings.SubscriptionInfoHelper;
import com.android.callsettings.R;

import static com.android.callsettings.TimeConsumingPreferenceActivity.FDN_CHECK_FAILURE;

public class GsmUmtsVideoCallForwardOptions extends TimeConsumingPreferenceActivity {
    private static final String LOG_TAG = "GsmUmtsVideoCallForwardOptions";
    private final boolean DBG = true;

    private static final String NUM_PROJECTION[] = {
        android.provider.ContactsContract.CommonDataKinds.Phone.NUMBER
    };

    private static final String BUTTON_VCFU_KEY   = "button_vcfu_key";
    private static final String BUTTON_VCFB_KEY   = "button_vcfb_key";
    private static final String BUTTON_VCFNRY_KEY = "button_vcfnry_key";
    private static final String BUTTON_VCFNRC_KEY = "button_vcfnrc_key";
    private static final int ACF_REASON_ID_PRIFIX = 0;
    private static final int VCF_REASON_ID_PRIFIX = 1<<8;
    private CallForwardEditPreference mButtonVCFU;
    private CallForwardEditPreference mButtonVCFB;
    private CallForwardEditPreference mButtonVCFNRy;
    private CallForwardEditPreference mButtonVCFNRc;
    private final ArrayList<CallForwardEditPreference> mVCFPreferences
            = new ArrayList<CallForwardEditPreference> ();
    private int mVTInitIndex = 0;

    private static final String KEY_TOGGLE = "toggle";
    private static final String KEY_STATUS = "status";
    private static final String KEY_NUMBER = "number";

    private TimeConsumingPreferenceListener mTcpListener;
    private Context mContext;
    private int mPhoneId = 0;

    private boolean mFirstResume;
    private Bundle mIcicle;
    private Phone mPhone;
    private SubscriptionInfoHelper mSubscriptionInfoHelper;
    private TelephonyManager mTelephonyManager;
    // SPRD: modify for bug544093
    private static final int CF_VIDEO_SERVICE_CALSS = 2;
    private boolean mIsVolteEnable;
    private boolean mIsImsListenerRegistered;
    private IImsServiceEx mIImsServiceEx;
    private boolean mIsFDNOn;

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        /* SPRD: add for callforward time @{ */
        mContext = this.getApplicationContext();
        addPreferencesFromResource(R.xml.video_callforward_options);
        /* @} */

        mSubscriptionInfoHelper = new SubscriptionInfoHelper(this, getIntent());
        mSubscriptionInfoHelper.setActionBarTitle(
                getActionBar(), getResources(), R.string.call_forwarding_settings_with_label);
        mPhone = mSubscriptionInfoHelper.getPhone();
        // SPRD: add for callforward time
        mPhoneId = mPhone.getPhoneId();

        PreferenceScreen prefSet = getPreferenceScreen();
        mTcpListener = this;

        mButtonVCFU = (CallForwardEditPreference) prefSet.findPreference(BUTTON_VCFU_KEY);
        mButtonVCFB = (CallForwardEditPreference) prefSet.findPreference(BUTTON_VCFB_KEY);
        mButtonVCFNRy = (CallForwardEditPreference) prefSet.findPreference(BUTTON_VCFNRY_KEY);
        mButtonVCFNRc = (CallForwardEditPreference) prefSet.findPreference(BUTTON_VCFNRC_KEY);
        tryRegisterImsListener();

        mButtonVCFU.setParentActivity(this, mButtonVCFU.reason | VCF_REASON_ID_PRIFIX);
        mButtonVCFB.setParentActivity(this, mButtonVCFB.reason | VCF_REASON_ID_PRIFIX);
        mButtonVCFNRy.setParentActivity(this, mButtonVCFNRy.reason | VCF_REASON_ID_PRIFIX);
        mButtonVCFNRc.setParentActivity(this, mButtonVCFNRc.reason | VCF_REASON_ID_PRIFIX);

        mVCFPreferences.add(mButtonVCFU);
        mVCFPreferences.add(mButtonVCFB);
        mVCFPreferences.add(mButtonVCFNRy);
        mVCFPreferences.add(mButtonVCFNRc);

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

        refreshVideoButton();
        if (mFirstResume) {
            if (mIcicle == null) {
                if (DBG) Log.d(LOG_TAG, "start to init ");
                if (mIsVolteEnable) {
                    mVCFPreferences.get(mVTInitIndex).init(this, false, mPhone);
                }
            } else {
                mVTInitIndex = mVCFPreferences.size();
                restoreCFPreferencesFromIcicle(mIcicle, mVCFPreferences);
            }
            mFirstResume = false;
            mIcicle = null;
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        for (CallForwardEditPreference pref : mVCFPreferences) {
            Bundle bundle = new Bundle();
            bundle.putBoolean(KEY_TOGGLE, pref.isToggled());
            if (pref.mImsCallForwardInfoEx != null) {
                bundle.putString(KEY_NUMBER, pref.mImsCallForwardInfoEx.mNumber);
                bundle.putInt(KEY_STATUS, pref.mImsCallForwardInfoEx.mStatus);
            }
            outState.putParcelable(pref.getKey(), bundle);
        }
    }

    @Override
    public void onFinished(Preference preference, boolean reading) {
        if (mVTInitIndex < mVCFPreferences.size()-1 && !isFinishing()) {
            mVTInitIndex++;
            mVCFPreferences.get(mVTInitIndex).init(this, false, mPhone);
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

            /* SPRD: function VOLTE call forward query support. @{ */
            int serviceClass = ACF_REASON_ID_PRIFIX;
            if (ImsManager.isVolteEnabledByPlatform(mContext)) {
                serviceClass = requestCode & 0xff00;
                requestCode = (requestCode & 0xff);
            }
            Log.d(LOG_TAG, "onActivityResult: serviceClass=" + serviceClass +
                    " / requestCode=" + requestCode);

            if (serviceClass == VCF_REASON_ID_PRIFIX) {
                switch (requestCode) {
                    case CommandsInterface.CF_REASON_UNCONDITIONAL:
                        mButtonVCFU.onPickActivityResult(cursor.getString(0));
                        break;
                    case CommandsInterface.CF_REASON_BUSY:
                        mButtonVCFB.onPickActivityResult(cursor.getString(0));
                        break;
                    case CommandsInterface.CF_REASON_NO_REPLY:
                        mButtonVCFNRy.onPickActivityResult(cursor.getString(0));
                        break;
                    case CommandsInterface.CF_REASON_NOT_REACHABLE:
                        mButtonVCFNRc.onPickActivityResult(cursor.getString(0));
                        break;
                    default:
                        break;
                }
            } else {
                //TODO:maybe handle other class in future
            }
            /* @} */
        } finally {
            if (cursor != null) {
                cursor.close();
                cursor = null;
            }
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        final int itemId = item.getItemId();
        if (itemId == android.R.id.home) {  // See ActionBar#setDisplayHomeAsUpEnabled()
            // SPRD: modify for bug544979
            GsmUmtsAllCallForwardOptions.goUpToTopLevelSetting(this, mSubscriptionInfoHelper);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    /* SPRD: add for bug544979 @{ */
    @Override
    public void onBackPressed() {
        GsmUmtsAllCallForwardOptions.goUpToTopLevelSetting(this, mSubscriptionInfoHelper);
    }
    /* @} */

    /* SPRD: add for callforward time @{ */
    @Override
    protected void onDestroy() {
        super.onDestroy();
        unRegisterImsListener();
    }

    private void updatePrefCategoryEnabled(Preference preference) {
        if (preference == mButtonVCFU) {
            if (mButtonVCFU.getStatus() == 1) {
                mButtonVCFB.setEnabled(false);
                mButtonVCFNRc.setEnabled(false);
                mButtonVCFNRy.setEnabled(false);
            } else {
                mButtonVCFB.setEnabled(true);
                mButtonVCFNRc.setEnabled(true);
                mButtonVCFNRy.setEnabled(true);
            }
        }
    }
    /* SPRD: function VOLTE call forward query support. @{ */
    private void restoreCFPreferencesFromIcicle(Bundle icicle,
            ArrayList<CallForwardEditPreference> preferences) {
        if (icicle != null && preferences!=null && !preferences.isEmpty()) {

            for (CallForwardEditPreference pref : preferences) {
                pref.init(this, true, mPhone);
                Bundle bundle = icicle.getParcelable(pref.getKey());
                pref.setToggled(bundle.getBoolean(KEY_TOGGLE));
                ImsCallForwardInfoEx cf = new ImsCallForwardInfoEx();
                cf.mNumber = bundle.getString(KEY_NUMBER);
                cf.mStatus = bundle.getInt(KEY_STATUS);
                pref.handleCallForwardVResult(cf);
            }
        }
    }

    public void onUpdateTwinsPref(boolean toggled, int arg1, int arg2, String arg3, String arg4) {
        int reason = arg2;
        if (reason == 0) {
            mVCFPreferences.get(reason).setToggled(toggled);
            /* SPRD: add for bug531765 @{ */
            mVCFPreferences.get(reason).setPhoneNumber(arg3);
            mVCFPreferences.get(reason).updateSummaryText();
            mVCFPreferences.get(reason).setEnabled(true);

            if (toggled || !mIsVolteEnable) {
                mButtonVCFB.setEnabled(false);
                mButtonVCFNRc.setEnabled(false);
                mButtonVCFNRy.setEnabled(false);
            } else {
                mButtonVCFB.setEnabled(true);
                mButtonVCFNRc.setEnabled(true);
                mButtonVCFNRy.setEnabled(true);
            }
        } else {
            if (reason >= 0 && reason < mVCFPreferences.size()) {
                mVCFPreferences.get(reason).setToggled(toggled);
                mVCFPreferences.get(reason).setPhoneNumber(arg3);
                mVCFPreferences.get(reason).updateSummaryText();
            }
        }
    }

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
    }

    public void refreshVideoButton() {
        if (!mIsVolteEnable || mIsFDNOn) {
            mButtonVCFU.setEnabled(false);
            mButtonVCFB.setEnabled(false);
            mButtonVCFNRc.setEnabled(false);
            mButtonVCFNRy.setEnabled(false);
        } else {
         if (mButtonVCFU != null) {
            mButtonVCFU.setEnabled(true);
            if (!mButtonVCFU.isToggled()) {
               mButtonVCFB.setEnabled(true);
               mButtonVCFNRc.setEnabled(true);
               mButtonVCFNRy.setEnabled(true);
            }
         }
        }
    }

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
                refreshVideoButton();
            }
        }
    };
}
