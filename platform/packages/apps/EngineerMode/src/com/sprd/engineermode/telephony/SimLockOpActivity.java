
package com.sprd.engineermode.telephony;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.content.Intent;
import android.util.Log;

import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class SimLockOpActivity extends PreferenceActivity {
    private static final String TAG = "SimLockOpActivity";

    static final String KEY_ADD_LOCK = "add_and_lock";
    static final String KEY_ADD = "add";
    static final String KEY_LOCK = "lock";
    static final String KEY_UNLOCK = "unlock";
    static final String KEY_SIMLOCK_TYPE = "simlock_type";
    static final String KEY_SIMLOCK_NETWORK = "networkpersonalization";
    static final String KEY_SIMLOCK_NETWORK_SUBSET = "networksubsetpersonalization";
    static final String KEY_SIMLOCK_SERVICE = "serviceproviderpersonalization";
    static final String KEY_SIMLOCK_CORPORATE = "corporatepersonalization";
    static final String KEY_SIMLOCK_SIM = "simpersonalization";
    private static final String KEY_SIMLOCK_LOCK = "simlock_lock";
    private static final String KEY_SIMLOCK_UNLOCK = "simlock_unlock";
    private static final String KEY_SIMLOCK_ADD = "simlock_add";
    private static final String KEY_SIMLOCK_REMOVE = "simlock_remove";
    private static final String KEY_SIMLOCK_UNLOCK_PERMANENTLY = "simlock_unlockpermanently";

    private static final int SIMLOCK_INIT = 1;

    private static final int SIMLOCK_INIT_END = 1;
    private SimLock mSimLock = null;
    private String mCurrSimLockType = null;

    private SimLockHandler mSimlockHandler;
    private Handler mUiHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {

            switch (msg.what) {
                case SIMLOCK_INIT_END:
                    String result = (String) msg.obj;
                    if (result.equals("true")) {
                        initState();
                    }
                    break;

                default:
                    break;
            }
        }
    };

    class SimLockHandler extends Handler {

        public SimLockHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case SIMLOCK_INIT:
                    String type = (String) msg.obj;
                    if(type == null){
                        return;
                    }
                    if (type.equals(KEY_SIMLOCK_NETWORK)) {
                        mSimLock = new SimLock(SimLock.TYPE_NETWORK);
                    } else if (type.equals(KEY_SIMLOCK_NETWORK_SUBSET)) {
                        mSimLock = new SimLock(SimLock.TYPE_NETWORK_SUBSET);
                    } else if (type.equals(KEY_SIMLOCK_SERVICE)) {
                        mSimLock = new SimLock(SimLock.TYPE_SERVICE);
                    } else if (type.equals(KEY_SIMLOCK_CORPORATE)) {
                        mSimLock = new SimLock(SimLock.TYPE_CORPORATE);
                    } else if (type.equals(KEY_SIMLOCK_SIM)) {
                        mSimLock = new SimLock(SimLock.TYPE_SIM);
                    }
                    Message initSimLockEnd = mUiHandler.obtainMessage(SIMLOCK_INIT_END,
                            String.valueOf(mSimLock.isInited()));
                    mUiHandler.sendMessage(initSimLockEnd);
                    break;

                default:
                    break;
            }
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_simlockop);
        mCurrSimLockType = this.getIntent().getStringExtra(KEY_SIMLOCK_TYPE);

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mSimlockHandler = new SimLockHandler(ht.getLooper());

        // findPreference("simlock_lock").setSummary(mCurrentSIMLOCKType);
    }

    public void onResume() {
        super.onResume();
        Message initSimLock = mSimlockHandler.obtainMessage(SIMLOCK_INIT, mCurrSimLockType);
        mSimlockHandler.sendMessage(initSimLock);
    }

    private void initState() {
        if (mSimLock.isLocked()) {
            findPreference(KEY_SIMLOCK_UNLOCK).setEnabled(true);
        } else {
            findPreference(KEY_SIMLOCK_UNLOCK).setEnabled(false);
        }

        if (!mSimLock.isSimLockInfoEmpty() && !mSimLock.isLocked()) {
            findPreference(KEY_SIMLOCK_LOCK).setEnabled(true);
        } else {
            findPreference(KEY_SIMLOCK_LOCK).setEnabled(false);
        }

        if (!mSimLock.isSimLockInfoFull() && !mSimLock.isLocked()) {
            findPreference(KEY_SIMLOCK_ADD).setEnabled(true);
        } else {
            findPreference(KEY_SIMLOCK_ADD).setEnabled(false);
        }

        if (!mSimLock.isSimLockInfoEmpty() && !mSimLock.isLocked()) {
            findPreference(KEY_SIMLOCK_REMOVE).setEnabled(true);
        } else {
            findPreference(KEY_SIMLOCK_REMOVE).setEnabled(false);
        }

        // unlock permanently cannnot run
        findPreference(KEY_SIMLOCK_UNLOCK_PERMANENTLY).setEnabled(false);

        // if (!mSimLock.isUnlockPermanently() && !mSimLock.isLocked()) {
        // findPreference("KEY_SIMLOCK_UNLOCK_PERMANENTLY").setEnabled(true);
        // } else {
        // findPreference("KEY_SIMLOCK_UNLOCK_PERMANENTLY").setEnabled(false);
        // }
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {
        String key = preference.getKey();

        if (key.equals(KEY_SIMLOCK_LOCK)) {
            Intent intent = new Intent(this, SimLockLockActivity.class);
            intent.putExtra(KEY_SIMLOCK_TYPE, mCurrSimLockType);
            intent.putExtra(KEY_ADD_LOCK, KEY_LOCK);
            startActivity(intent);
        } else if (key.equals(KEY_SIMLOCK_UNLOCK)) {
            Intent intent = new Intent(this, SimLockUnlockActivity.class);
            intent.putExtra(KEY_SIMLOCK_TYPE, mCurrSimLockType);
            startActivity(intent);
        } else if (key.equals(KEY_SIMLOCK_ADD)) {
            Intent intent = new Intent(this, SimLockLockActivity.class);
            intent.putExtra(KEY_SIMLOCK_TYPE, mCurrSimLockType);
            intent.putExtra(KEY_ADD_LOCK, KEY_ADD);
            startActivity(intent);
        } else if (key.equals(KEY_SIMLOCK_REMOVE)) {
        } else if (key.equals(KEY_SIMLOCK_UNLOCK_PERMANENTLY)) {
        }

        return false;
    }

    @Override
    protected void onDestroy() {
        if (mSimlockHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mSimlockHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        finish();
        super.onBackPressed();
    }
}
