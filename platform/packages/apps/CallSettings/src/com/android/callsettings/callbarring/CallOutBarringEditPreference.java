
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

public class CallOutBarringEditPreference extends TimeConsumingPreferenceActivity
        implements CallBarringEditPreferencePreferenceListener {

    private static final String LOG_TAG = "CallOutBarringEditPreference";
    private final boolean DBG = true;// Debug.isDebug();
    private static final String BUTTON_AO_KEY = "button_ao_key";
    private static final String BUTTON_OI_KEY = "button_oi_key";
    private static final String BUTTON_OX_KEY = "button_ox_key";
    private static final String KEY_TOGGLE = "toggle";
    private static final String KEY_STATUS = "status";
    private static final String KEY_PASSWORD = "password";

    private CallBarringEditPreference mButtonAO;
    private CallBarringEditPreference mButtonOI;
    private CallBarringEditPreference mButtonOX;

    private final ArrayList<CallBarringEditPreference> mPreferences =
            new ArrayList<CallBarringEditPreference>();
    private boolean mFirstResume;
    private Bundle mIcicle;
    private GsmCdmaPhoneEx mPhone;
    private SubscriptionInfoHelper mSubscriptionInfoHelper;

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        addPreferencesFromResource(R.xml.calloutbarring_options_ex);

        mSubscriptionInfoHelper = new SubscriptionInfoHelper(this, getIntent());
        mSubscriptionInfoHelper.setActionBarTitle(
                getActionBar(), getResources(), R.string.callout_barring_settings);
        mPhone = (GsmCdmaPhoneEx)mSubscriptionInfoHelper.getPhone();
        Log.i("CallBarring", "OUT mPhone:" + mPhone);
        final ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            actionBar.setNavigationMode(ActionBar.NAVIGATION_MODE_STANDARD);
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setDisplayShowTitleEnabled(true);
        }

        PreferenceScreen prefSet = getPreferenceScreen();
        mButtonAO = (CallBarringEditPreference) prefSet.findPreference(BUTTON_AO_KEY);
        mButtonOI = (CallBarringEditPreference) prefSet.findPreference(BUTTON_OI_KEY);
        mButtonOX = (CallBarringEditPreference) prefSet.findPreference(BUTTON_OX_KEY);

        mButtonAO.setParentActivity(this, mButtonAO.mReason);
        mButtonOI.setParentActivity(this, mButtonOI.mReason);
        mButtonOX.setParentActivity(this, mButtonOX.mReason);

        mPreferences.add(mButtonAO);
        mPreferences.add(mButtonOI);
        mPreferences.add(mButtonOX);

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
            case GsmCdmaPhoneEx.CB_REASON_OI:
            case GsmCdmaPhoneEx.CB_REASON_OX:
                handleCallBarringResult(mButtonAO);
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
