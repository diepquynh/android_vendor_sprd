package com.sprd.incallui.inCallUITelcelPlugin;

import android.app.AddonManager;
import android.content.Context;
import android.content.res.Resources.NotFoundException;

import android.util.Log;

import com.android.incallui.R;
import com.sprd.incallui.InCallUITelcelHelper;


public class InCallUITelcelPlugin extends InCallUITelcelHelper implements
        AddonManager.InitialCallback {

    private static final String TAG = "InCallUITelcelPlugin";
    private final String SPECIAL_VOICE_CLEAR_CODE = "*00015";
    private Context mContext;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    public void InCallUITelcelPlugin() {

    }

    @Override
    public boolean isVoiceClearCodeLabel(Context context, String callStateLabel) {
        try {
            String unobtainableNumber = context.getString(R.string.callFailed_unobtainable_number);
            String congestion = context.getString(R.string.callFailed_congestion);
            String userBusy = context.getString(R.string.callFailed_userBusy);
            String limitExceeded = context.getString(R.string.callFailed_limitExceeded);

            if (unobtainableNumber.equals(callStateLabel)
                    || congestion.equals(callStateLabel)
                    || userBusy.equals(callStateLabel)
                    || limitExceeded.equals(callStateLabel)) {
                return true;
            }
            return false;
        } catch (NotFoundException e) {
            Log.e(TAG, "NotFoundException when getString.");
            e.printStackTrace();
            return false;
        }
    }

    @Override
    public boolean isSpecialVoiceClearCode(String number) {
        if (SPECIAL_VOICE_CLEAR_CODE.equals(number)) {
            return true;
        }
        return false;
    }
}
