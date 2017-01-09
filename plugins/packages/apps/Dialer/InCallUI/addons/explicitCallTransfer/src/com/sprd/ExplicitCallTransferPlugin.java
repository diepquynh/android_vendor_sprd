package com.sprd.incallui.explicitCallTransferPlugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

import com.android.incallui.CallList;
import com.android.sprd.incallui.ExplicitCallTransferPluginHelper;
import com.android.sprd.telephony.RadioInteractor;
import com.android.sprd.incallui.InCallUiUtils;

/**
 * Various utilities for dealing with phone number strings.
 */
public class ExplicitCallTransferPlugin extends ExplicitCallTransferPluginHelper implements AddonManager.InitialCallback {
    private Context mAddonContext;
    private static final String TAG = "[ExplicitCallTransferPlugin]";

    public ExplicitCallTransferPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        log("clazz: " + clazz);
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void explicitCallTransfer(Context context){
        RadioInteractor radioInteractor = new RadioInteractor(context);
        radioInteractor.explicitCallTransfer(InCallUiUtils.getCurrentPhoneId(context));
    }

    @Override
    public boolean shouldEnableTransferButton() {
        // According to 3GPP TS23.091, only when background call is HOLDING and foreground call
        // is DIALING, ALERTING, or ACTIVE, transfer button will display.
        CallList calllist = CallList.getInstance();
        return calllist != null
                && calllist.getBackgroundCall() != null // HOLDING
                && calllist.getOutgoingOrActive() != null; // DIALING/ALERTING/ACTIVE
    }

    private static void log(String msg) {
        Log.d(TAG, msg);
    }
}
