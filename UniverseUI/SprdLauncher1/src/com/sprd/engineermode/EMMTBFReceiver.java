/** Created by Spreadtrum */
package com.sprd.engineermode;

import java.util.ArrayList;
import com.android.sprdlauncher1.Launcher;
import com.android.sprdlauncher1.LauncherAppState;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class EMMTBFReceiver extends BroadcastReceiver {

    public static final int LAUNCHER_SHORTCURSET = 0;
    public static final String ACTION_UPDATE_WORKSPACE_FROM_MTBF = "com.sprd.engineermode.action.MTBF";
    public static final String ACTION_OPERATION_RESULT_FOR_MTBF = "com.sprd.engineermode.action.MTBFRSP";
    public static final String RESULT_OK = "Ok";
    public static final String RESULT_FAIL = "Fail";

    public static final String KEY_PACKAGE_NAME = "PACKAGE NAME";
    public static final String KEY_SETITEM = "SETITEM";
    public static final String KEY_RESULT = "RESULT";
    public static final String LAUNCHER_PACKAGE_NAME = "com.android.sprdlauncher1";
    public static final String NAMELIST = "namelist";

    private static final String TAG = "EMMTBFReceiver";

    public EMMTBFReceiver() {
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        /* SPRD: Fix bug 281219, disable MTBF feature of Launcher @{ */
        return;
        /*
        Log.i(TAG, "onReceiveMTBF=" + intent.getAction());
        String action = intent.getAction();
        if (ACTION_UPDATE_WORKSPACE_FROM_MTBF.equals(action)) {
            final Launcher launcher = LauncherAppState.getInstance().getLauncher();
            if (launcher != null) {
                ArrayList<String> appList;
                appList = intent.getStringArrayListExtra(NAMELIST);
                Log.i(TAG, "appList= " + appList);
                launcher.bindMTBFShortcut(appList);
            } else {
                sendMTBFResult(context, RESULT_FAIL);
            }
        }
        */
        /* @} */
    }

    public static void sendMTBFResult(Context context,String result) {
        Intent resultIntent = new Intent();
        resultIntent.setAction(ACTION_OPERATION_RESULT_FOR_MTBF);
        Bundle bundle = new Bundle();
        bundle.putString(KEY_PACKAGE_NAME, LAUNCHER_PACKAGE_NAME);
        bundle.putInt(KEY_SETITEM, LAUNCHER_SHORTCURSET);
        bundle.putString(KEY_RESULT, result);
        resultIntent.putExtras(bundle);
        context.sendBroadcast(resultIntent);
    }
}
