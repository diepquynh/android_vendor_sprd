
package com.cucc.homepage;


import com.android.internal.telephony.TelephonyProperties;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemProperties;
import android.provider.Settings.System;
import android.telephony.TelephonyManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.net.NetworkInfo;
import android.util.Log;
import android.telephony.SubscriptionManager;


public class homePage extends Activity {
    /** Called when the activity is first created. */
    private static final String url = "http://android.wo.com.cn/";
    private ConnectivityManager mConnMgr = null;
    private int DefaultDataPhoneId = -1;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mConnMgr = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        DefaultDataPhoneId = SubscriptionManager.from(getApplicationContext()).getDefaultDataPhoneId();

        if (isCurrentCard(DefaultDataPhoneId)
                || mConnMgr.getNetworkInfo(ConnectivityManager.TYPE_WIFI).isConnected()) {
            Uri uri = Uri.parse(url);
            Intent intent = new Intent(Intent.ACTION_VIEW, uri);
            startActivity(intent);
        } else {
            AlertDialog aDialog =new AlertDialog.Builder(this)
            .setTitle(R.string.usim_recommanded)
            .setIcon(android.R.drawable.ic_dialog_alert)
            .setMessage(R.string.usim_recommanded_for_cucc_app)
            .setPositiveButton(R.string.pick_yes,
                    new DialogInterface.OnClickListener() {

                public void onClick(DialogInterface dialog,
                        int which) {
                    Uri uri = Uri.parse(url);
                    Intent intent = new Intent(Intent.ACTION_VIEW, uri);
                    startActivity(intent);
                }
            })
            .setNegativeButton(R.string.pick_no,new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog,
                        int which) {
                    onPause();
                }
            }).show();
        }

    }

    @Override
    protected void onPause() {
        super.onPause();
        finish();
    }

    private boolean isCurrentCard(int phoneId) {
        String operator = SystemProperties.get("ro.operator", "");
        String numeric = TelephonyManager.from(getApplicationContext()).getSimOperatorNumericForPhone(phoneId);
        if ("cucc".equals(operator)) {
            if (numeric.equals("46001") || numeric.equals("46006")
            || numeric.equals("45407")) {
                return true;
            } else {
                return false;
            }
        }
        return false;
    }
}
