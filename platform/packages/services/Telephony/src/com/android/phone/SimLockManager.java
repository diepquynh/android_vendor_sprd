package com.android.phone;

import android.app.AddonManager;
import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class SimLockManager {
    private static final String TAG = "SimLockManager";
    private static SimLockManager mInstance;

    public SimLockManager() {
    }

    public synchronized static SimLockManager getInstance(Context context,
            int featureId) {
        if (mInstance != null) {
            return mInstance;
        }
        mInstance = (SimLockManager) new AddonManager(context).getAddon(
                featureId, SimLockManager.class);
        return mInstance;
    }

    public void registerForSimLocked(Context context) {
        Log.d(TAG, "registerForSimLocked: empty method");
    }

    public void processSimStateChanged(Context context) {
        Log.d(TAG, "processSimStateChanged: empty method");
    }

    public void showPanel(Context context, Message msg) {
        Log.d(TAG, "showPanel: empty method");
    }

    public void sendEMDialStart(Context context) {
        Log.d(TAG, "sendEMDialStart: empty method");
    }

    public void sendEMDialStop(Context context) {
        Log.d(TAG, "sendEMDialStop: empty method");
    }

    public Message decodeMessage(int simState, int phoneId) {
        Log.d(TAG, "decodeMessage: empty method");
        return null;
    }

    public void showPanelForUnlockByNv(Context context) {
        Log.d(TAG, "showPanelForUnlockByNv: empty method");
    }
}
