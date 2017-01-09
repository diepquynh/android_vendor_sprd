
package com.android.callsettings.callbarring;

import android.app.ActionBar;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.view.MenuItem;

import com.android.callsettings.R;
import com.android.callsettings.TimeConsumingPreferenceActivity;
import com.android.internal.telephony.Phone;
import com.android.callsettings.SubscriptionInfoHelper;
import com.android.internal.telephony.GsmCdmaPhoneEx;

import java.util.ArrayList;

public class CallInBarringEditPreference extends TimeConsumingPreferenceActivity
        implements CallBarringEditPreferencePreferenceListener {

    private static final String LOG_TAG = "CallInBarringEditPreference";
    private final boolean DBG = true;// Debug.isDebug();
    private static final String BUTTON_AI_KEY = "button_ai_key";
    private static final String BUTTON_IR_KEY = "button_ir_key";
    private static final String KEY_TOGGLE = "toggle";
    private static final String KEY_STATUS = "status";
    private static final String KEY_PASSWORD = "password";

    private CallBarringEditPreference mButtonAI;
    private CallBarringEditPreference mButtonIR;

    private final ArrayList<CallBarringEditPreference> mPreferences =
            new ArrayList<CallBarringEditPreference>();
    private boolean mFirstResume;
    private Bundle mIcicle;
    private GsmCdmaPhoneEx mPhone;
    private SubscriptionInfoHelper mSubscriptionInfoHelper;

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        addPreferencesFromResource(R.xml.callinbarring_options_ex);

        mSubscriptionInfoHelper = new SubscriptionInfoHelper(this, getIntent());
        mSubscriptionInfoHelper.setActionBarTitle(
                getActionBar(), getResources(), R.string.callin_barring_settings);
        mPhone = (GsmCdmaPhoneEx)mSubscriptionInfoHelper.getPhone();
        Log.i("CallBarring", "in mPhone:" + mPhone);
        final ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            actionBar.setNavigationMode(ActionBar.NAVIGATION_MODE_STANDARD);
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setDisplayShowTitleEnabled(true);
        }

        PreferenceScreen prefSet = getPreferenceScreen();
        mButtonAI = (CallBarringEditPreference) prefSet.findPreference(BUTTON_AI_KEY);
        mButtonIR = (CallBarringEditPreference) prefSet.findPreference(BUTTON_IR_KEY);

        mButtonAI.setParentActivity(this, mButtonAI.mReason);
        mButtonIR.setParentActivity(this, mButtonIR.mReason);

        mPreferences.add(mButtonAI);
        mPreferences.add(mButtonIR);

        // we wait to do the initialization until onResume so that the
        // TimeConsumingPreferenceActivity dialog can display as it
        // relies on onResume / onPause to maintain its foreground state.
        mFirstResume = true;
        mIcicle = icicle;
    }

    @Override
    public void onResume() {
        super.onResume();

        if (mFirstResume) {
            if (mIcicle == null) {
                if (DBG)
                    Log.d(LOG_TAG, "start to init ");
                for (CallBarringEditPreference pre : mPreferences) {
                    pre.setListener(this);
                    pre.init(this, false, mPhone);
                }
            } else {
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
            mFirstResume = false;
            mIcicle = null;
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        for (CallBarringEditPreference pref : mPreferences) {
            Bundle bundle = new Bundle();
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

    @Override
    protected void onStop() {
        super.onStop();
        finish();
    }

    public void onChange(Preference preference, int reason) {
        if (DBG)
            Log.d(LOG_TAG, "onChange, reason:  " + reason);
        if (!((CallBarringEditPreference) preference).getNeedEcho()) {
            cancelAll();
        } else {
            ((CallBarringEditPreference) preference).queryCallBarringAfterSet(this, reason);

            /* SPRD: add for bug549172 @{ */
            if (reason == GsmCdmaPhoneEx.CB_REASON_AI) {
                int action = (mButtonIR.isToggled() ? GsmCdmaPhoneEx.CB_ACTION_ENABLE
                        : GsmCdmaPhoneEx.CB_ACTION_DISABLE);

                mButtonIR.queryCallInBarringAfterSet(this, action,
                        GsmCdmaPhoneEx.CB_REASON_IR);
            } else if (reason == GsmCdmaPhoneEx.CB_REASON_IR) {
                int action = (mButtonAI.isToggled() ? GsmCdmaPhoneEx.CB_ACTION_ENABLE
                        : GsmCdmaPhoneEx.CB_ACTION_DISABLE);

                mButtonAI.queryCallInBarringAfterSet(this, action,
                        GsmCdmaPhoneEx.CB_REASON_AI);
            }
            /* @} */
        }
    }

    public Phone getPhone() {
        return mPhone;
    }

    public void updateSummary(int reason) {
        switch (reason) {
            case GsmCdmaPhoneEx.CB_REASON_AI:
                handleCallBarringResult(mButtonIR);
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

}
