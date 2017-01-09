package plugin.sprd.logRejectedCalls;

import android.app.AddonManager;
import android.content.Context;
import android.provider.CallLog.Calls;
import com.android.server.telecom.CallState;
import android.telecom.DisconnectCause;
import android.util.Log;

import com.sprd.server.telecom.LogRejectedCallsUtils;

// SPRD: modify for bug447629.
public class AddonLogRejectedCallsPlugin extends LogRejectedCallsUtils
        implements AddonManager.InitialCallback {
    private static final String TAG = "AddonLogRejectedCallsPlugin";

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    // default constructor
    public AddonLogRejectedCallsPlugin() {
    }

    public int getCallLogType(boolean isIncoming, int disconnectCause) {
        Log.d(TAG, "getCallLogType");
        int type;
        if (!isIncoming) {
            type = Calls.OUTGOING_TYPE;
        } else if (disconnectCause == DisconnectCause.MISSED
                || disconnectCause == DisconnectCause.REJECTED) {
            type = Calls.MISSED_TYPE;
        } else {
            type = Calls.INCOMING_TYPE;
        }
        return type;
    }

    public boolean shouldShowMissedCallNotification(int disConnectCause,
            int oldState, int newState) {
        Log.d(TAG, "shouldShowMissedCallNotification");
        if (oldState == CallState.RINGING && newState == CallState.DISCONNECTED &&
                ((disConnectCause == DisconnectCause.MISSED) || (disConnectCause == DisconnectCause.REJECTED))) {
            return true;
        } else {
            return false;
        }
    }
}
