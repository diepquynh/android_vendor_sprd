package com.sprd.incallui.addonInCallUIPlugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.widget.TextView;

import com.android.incallui.InCallPresenter;
import com.android.incallui.InCallPresenter.InCallState;
import com.android.phone.common.animation.AnimUtils;
import com.sprd.incallui.InCallUIPlugin;

/**
 * Various utilities for dealing with phone number strings.
 */
public class AddonInCallUIPlugin extends InCallUIPlugin implements AddonManager.InitialCallback {
    private Context mAddonContext;
    private static final String TAG = "[CMCC::AddonInCallUIPlugin]";

    public AddonInCallUIPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        log("clazz: " + clazz);
        mAddonContext = context;
        return clazz;
    }

    public void setPrimaryCallElapsedTime(TextView elapsedTimeT) {
        // DO Nothing for cmcc implication.
        log("setPrimaryCallElapsedTime");
        final InCallState newState = InCallPresenter.getInstance().getInCallState();
        if (newState != InCallState.INCALL) {
            AnimUtils.fadeOut(elapsedTimeT, -1);
        }
    }

    public boolean isOnlyDislpayActiveCall() {
        log("isOnlyDislpayActiveCall");
        return true;
    }

    private static void log(String msg) {
        Log.d(TAG, msg);
    }
}
