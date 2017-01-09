
package com.sprd.generalsecurity.network;

// Need the following import to get access to the app resources, since this
// class is in a sub-package.



import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import com.sprd.generalsecurity.utils.Contract;

import android.os.Bundle;
import com.sprd.generalsecurity.utils.TeleUtils;


public class SimStateReceiver extends BroadcastReceiver {
    private static String TAG = "SimStateReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.e(TAG, "intent:" + intent);
        if (intent == null || intent.getStringExtra("ss") == null) {
            return;
        }
        if (intent.getStringExtra("ss").equalsIgnoreCase("loaded")) {
            Log.e(TAG, "sim loaded: " + TeleUtils.getSimNumber(context, 0)
                    + ":" + TeleUtils.getSimNumber(context, 1));
            Intent it = new Intent(context, DataFlowService.class);
            it.putExtra(Contract.EXTRA_SIM_STATE, true);
            context.startService(it);
        }
        // debugIntent(context, intent);

    }

    private void debugIntent(Context context, Intent intent) {
        Bundle extras = intent.getExtras();
        if (extras != null) {
            for (String key: extras.keySet()) {
                Log.e(TAG, "key[" + key + "]:" + extras.get(key) );
            }
        } else {
            Log.e(TAG, "no extras");
        }
        Log.e(TAG, "================================");
    }
}
