
package com.sprd.voicetrigger;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.sprd.voicetrigger.provider.ContentProviderHelper;

public class VoiceTriggerReceiver extends BroadcastReceiver {

    private final static String TAG = "VoiceTriggerReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        final String action = intent.getAction();
        Log.d(TAG, "VoiceTriggerReceiver action: " + action);

        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            Log.d(TAG, "action: " + action);
            // begin open service
            if (ContentProviderHelper.isOpenSwitch(context)) {
                Intent actionIntent = new Intent();
                actionIntent.setAction("com.sprd.voicetrigger.VoiceTriggerService");
                actionIntent.setPackage(context.getPackageName());
                context.startService(actionIntent);
            }
        }
    }
}
