package com.sprd.telecom.telecomCmccPlugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

import com.android.server.telecom.Call;
import com.sprd.server.telecom.TelecomCmccHelper;
import android.telecom.VideoProfile;

import com.android.server.telecom.CallsManager;
import android.widget.Toast;
import com.sprd.incallui.inCallUICmccPlugin.R;
/**
 * This class is used to manager Telecom CMCC Plugin
 */
public class TelecomCmccPlugin extends TelecomCmccHelper implements AddonManager.InitialCallback {

    private static final String TAG = "TelecomCmccPlugin";
    private Context mContext;

    public TelecomCmccPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        log("clazz: " + clazz);
        mContext = context;
        return clazz;
    }

    private static void log(String msg) {
        Log.d(TAG, msg);
    }

    //add for hangup  incoming call when in vt call
    public boolean hangUpIncomingCall(Call incomingCall, Call activeCall) {
        log("is Cmcc project...... ");
        if (incomingCall != null
                && activeCall != null
                && ((VideoProfile.isVideo(activeCall.getVideoState()) || (!VideoProfile
                        .isVideo(activeCall.getVideoState()) && VideoProfile.isVideo(incomingCall
                        .getVideoState()))))) {
            log("is Cmcc project reject incomming call ");
            return true;
        }
        return false;
    }

    /* SPRD: Add for CMCC requirement bug574817 */
    public boolean shouldPreventAddVideoCall(CallsManager callsManager, int intentVideoState,boolean hasVideocall) {
        log("shouldPreventAddVideoCall");
        if ((callsManager != null && hasVideocall)
                || (VideoProfile.isBidirectional(intentVideoState) && callsManager.getActiveCall() != null)) {
            Toast.makeText(mContext, mContext.getString(R.string.duplicate_video_call_not_allowed),
                    Toast.LENGTH_LONG).show();
            return true;
        }
        return false;
    }
    /* @} */
}