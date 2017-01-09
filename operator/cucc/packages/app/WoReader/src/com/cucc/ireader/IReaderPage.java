
package com.cucc.ireader;

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

public class IReaderPage extends Activity {
    /** Called when the activity is first created. */
    private static final String url = "http://iread.wo.com.cn/";
    private ConnectivityManager mConnMgr = null;
    private int DefaultDataPhoneId = -1;
    private NetworkInfo mNet = null;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mConnMgr = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        DefaultDataPhoneId = SubscriptionManager.from(getApplicationContext()).getDefaultDataPhoneId();
        mNet = mConnMgr.getActiveNetworkInfo();
        String numeric =
                TelephonyManager.getTelephonyProperty(DefaultDataPhoneId,
                        TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, "");
        boolean isCurrentCard = numeric.equals("46001") || numeric.equals("46006") || numeric.equals("46009");
        if (isCurrentCard || (mNet != null && mNet.getType() == ConnectivityManager.TYPE_WIFI)) {
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

}
