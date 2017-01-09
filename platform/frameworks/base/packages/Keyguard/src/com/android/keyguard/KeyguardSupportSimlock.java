
package com.android.keyguard;

import android.app.AddonManager;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import com.android.keyguard.R;
import com.android.internal.telephony.IccCardConstantsEx.State;

public class KeyguardSupportSimlock {
    static KeyguardSupportSimlock sInstance;
    public static final String TAG = "KeyguardSupportSimlock";

    public KeyguardSupportSimlock() {
    }

    public static KeyguardSupportSimlock getInstance() {
        if (sInstance != null)
            return sInstance;
        sInstance = (KeyguardSupportSimlock) AddonManager.getDefault().getAddon(
                R.string.plugin_keyguard_simlock, KeyguardSupportSimlock.class);
        return sInstance;
    }

    public String getSimLockStatusString(int slotId){
        Log.d(TAG, "unimplement!" );
        return null;
    }

    public boolean isSimlockStatusChange (Intent intent) {
        Log.d(TAG, "unimplement!" );
        return false;
    }

    public State getSimStateEx(int subId) {
        Log.d(TAG, "unimplement!" );
        return State.UNKNOWN;
    }
}
