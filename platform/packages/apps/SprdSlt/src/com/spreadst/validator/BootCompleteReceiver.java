
package com.spreadst.validator;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.util.Log;

public class BootCompleteReceiver extends BroadcastReceiver {

    private static final String TAG = "Validator_BootCompleteReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        Log.d(TAG, "Receive action ::: " + action);
        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            SharedPreferences sltValue = context.getSharedPreferences(UIDialog.SLT_VALUE, Context.MODE_PRIVATE);
            boolean bootStart = sltValue.getBoolean(UIDialog.BOOT_COMPLETE_START, false);
            if (bootStart) {
                Intent i = new Intent(context, ValidateService.class);
                i.putExtra(ValidateService.ID, ValidateService.BOOT_COMPLETED);
                context.startService(i);
            }
        } else if (Intent.ACTION_SCREEN_OFF.equals(action)) {
        } else if (Intent.ACTION_SCREEN_ON.equals(action)) {
        } else {
        }
    }
}
