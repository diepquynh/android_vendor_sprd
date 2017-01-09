package com.android.callsettings.callforward;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.PersistableBundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.view.MenuItem;

import android.util.Log;
import android.telephony.CarrierConfigManagerEx;
import android.telephony.TelephonyManager;
import android.telephony.SubscriptionManager;
import android.text.TextUtils;

import com.android.ims.ImsManager;
import com.android.internal.telephony.PhoneConstants;
import com.android.callsettings.plugins.CallSettingsCMCCHelper;
import com.android.callsettings.SubscriptionInfoHelper;
import com.android.callsettings.R;

public class GsmUmtsAllCallForwardOptions extends PreferenceActivity {
    private static final String LOG_TAG = "GsmUmtsAllCallForwardOptions";
    private final boolean DBG = true;

    private static final String VIDEO_CALL_FORWARDING_KEY = "video_call_forwarding_key";
    private static final String AUDIO_CALL_FORWARDING_KEY = "audio_call_forwarding_key";
    private SubscriptionInfoHelper mSubscriptionInfoHelper;
    /* SPRD: add for bug626699 @{ */
    private TelephonyManager mTeleMgr;
    private static final String KEY_PLMNS_CALLFORWARD_ONLY = "plmns_callforward_only";
    /* @} */
    private Context mContext;

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        addPreferencesFromResource(R.xml.gsm_umts_all_call_forward_options);

        mContext = this;
        // SPRD: add for bug626699
        mTeleMgr = (TelephonyManager) this.getSystemService(Context.TELEPHONY_SERVICE);
        mSubscriptionInfoHelper = new SubscriptionInfoHelper(this, getIntent());
        mSubscriptionInfoHelper.setActionBarTitle(
                getActionBar(), getResources(), R.string.labelCF);
        init(getPreferenceScreen(), mSubscriptionInfoHelper);

        if (mSubscriptionInfoHelper.getPhone().getPhoneType() != PhoneConstants.PHONE_TYPE_GSM) {
            //disable the entire screen
            getPreferenceScreen().setEnabled(false);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        final int itemId = item.getItemId();
        if (itemId == android.R.id.home) {
            finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onBackPressed() {
        finish();
    }

    public void init(PreferenceScreen prefScreen, SubscriptionInfoHelper subInfoHelper) {
        Preference callForwardingPref = prefScreen.findPreference(AUDIO_CALL_FORWARDING_KEY);
        /* SPRD: modify for bug626699 @{ */
        Intent callForwardingIntent = subInfoHelper.getIntent(GsmUmtsCallForwardOptions.class);
        boolean isOnlySupportVoiceCall = isOnlySupportVoiceCallForward(subInfoHelper.getSubId());
        callForwardingIntent.putExtra(KEY_PLMNS_CALLFORWARD_ONLY, isOnlySupportVoiceCall);
        callForwardingPref.setIntent(callForwardingIntent);
        if (DBG) {
            log("isOnlySupportVoiceCall = " + isOnlySupportVoiceCall);
        }
        /* @} */
        Preference videoCallForwardingPref = prefScreen.findPreference(VIDEO_CALL_FORWARDING_KEY);
        videoCallForwardingPref
                .setIntent(subInfoHelper.getIntent("com.android.callsettings",
                        "com.android.callsettings.callforward.GsmUmtsVideoCallForwardOptions"));
        if (!ImsManager.isVolteEnabledByPlatform(mContext)
                // SPRD: add for bug 611966
                || !CallSettingsCMCCHelper.getInstance(mContext).isVideoCallForwardSupport()) {
            prefScreen.removePreference(videoCallForwardingPref);
        }
        final SubscriptionManager subscriptionManager = SubscriptionManager.from(this);
        if (subscriptionManager.getDefaultDataSubscriptionId() != subInfoHelper.getSubId()) {
            videoCallForwardingPref.setEnabled(false);
        }
    }

    public static void goUpToTopLevelSetting(
        Activity activity, SubscriptionInfoHelper subscriptionInfoHelper) {
        Intent intent = subscriptionInfoHelper.getIntent(GsmUmtsAllCallForwardOptions.class);
        intent.setAction(Intent.ACTION_MAIN);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        activity.startActivity(intent);
        activity.finish();
    }


    /* SPRD: add for bug626699 @{ */
    private boolean isOnlySupportVoiceCallForward(int subId) {
        boolean isSupport = false;
        String plmn = mTeleMgr.getSimOperatorNumeric(subId);
        String plmnsConfig = "";
        CarrierConfigManagerEx carrierMgr = CarrierConfigManagerEx.from(mContext);
        if (carrierMgr != null) {
            PersistableBundle config = carrierMgr.getConfigForSubId(subId);
            if (config != null){
                plmnsConfig = config.getString(CarrierConfigManagerEx.KEY_PLMNS_CALLFORWARD_ONLY, "");
            }
        }
        if (DBG) {
            log("isOnlySupportVoiceCallForward = " + plmnsConfig + "opreator is " + plmn);
        }
        if (!TextUtils.isEmpty(plmnsConfig)) {
            String[] plmnArray = plmnsConfig.split(",");
            for (String plmns : plmnArray) {
                if (plmn.equals(plmns)) {
                    isSupport = true;
                }
            }
        } else {
            if (DBG) {
                log("plmnsConfig is null");
            }
        }
        return isSupport;
    }

    private void log(String msg) {
        Log.d(LOG_TAG, msg);
    }
    /* @} */
}
