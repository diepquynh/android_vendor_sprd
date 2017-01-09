/** Create by Spreadst */

package com.spreadst.hasmterscreen;

/**
 *
 * hasmter animation
 *
 */
import android.content.Context;
import android.content.Intent;

public class HasmterIntentAction {

    public static final int ACTION_CALL = 0;
    public static final int ACTION_SMS = 1;
    public static final int ACTION_HOME = 2;
    public static final int ACTION_CAMERA = 3;

    public static void startHasmterIntentAction(Context context, int actionId) throws Exception {
        try {
            switch (actionId) {
                case ACTION_CALL:
                    startCallIntent(context);
                    break;
                case ACTION_CAMERA:
                    startCameraIntent(context);
                    break;
                case ACTION_SMS:
                    startSMSIntent(context);
                    break;
                default:
                    startHomeIntent(context);
                    break;
            }
        } catch (Exception ex) {
            throw new Exception(ex.getMessage());
        }
    }

    private static void startCallIntent(Context context) {
        if (context != null) {
            Intent intent = new Intent(Intent.ACTION_CALL_BUTTON);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK |
                    Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
            context.startActivity(intent);
        }
    }

    private static void startSMSIntent(Context context) {
        if (context != null) {
            Intent intent = new Intent(Intent.ACTION_MAIN);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                    | Intent.FLAG_ACTIVITY_SINGLE_TOP
                    | Intent.FLAG_ACTIVITY_CLEAR_TOP);
            intent.setType("vnd.android-dir/mms-sms");
            context.startActivity(intent);
        }
    }

    private static void startCameraIntent(Context context) {
        if (context != null) {
            Intent intent = new Intent(Intent.ACTION_CAMERA_BUTTON, null);
            context.sendOrderedBroadcast(intent, null);
        }
    }

    private static void startHomeIntent(Context context) {
    }
}
