package com.sprd.server.telecom;

import android.app.AddonManager;
import android.content.Context;
import android.media.AudioManager;
import android.media.SoundPool;

import com.android.server.telecom.Log;
import com.android.server.telecom.R;

/**
 * Created by sprd on 1/17/17.
 */
public class TelcelCallWaitingToneHelper {
    private static final String LOG_TAG = "TelcelCallwaitingToneHelper";
    private static TelcelCallWaitingToneHelper sInstance;
    private SoundPool mCallWaitingToneSoundPool;

    public TelcelCallWaitingToneHelper() {
    }

    public static TelcelCallWaitingToneHelper getInstance(Context context) {
        if (sInstance == null) {
            synchronized (TelcelCallWaitingToneHelper.class) {
                AddonManager addonManager = new AddonManager(context);
                sInstance = (TelcelCallWaitingToneHelper) addonManager.getAddon(R.string.feature_telcel_callwaiting_tone_plugin,
                        TelcelCallWaitingToneHelper.class);
            }
        }
        Log.d(LOG_TAG, "Get instance : " + sInstance);
        return sInstance;

    }

    public void stop3rdCallWaitingTone () {
    }

    public void play3rdCallWaitingTone () {
    }

    public boolean is3rdCallWaitingToneSupport () {
        return false;
    }
}
