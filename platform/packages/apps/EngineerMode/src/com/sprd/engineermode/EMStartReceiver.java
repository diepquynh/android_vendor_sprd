package com.sprd.engineermode;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;
import java.lang.String;
import com.android.internal.telephony.TelephonyIntents;
import android.os.SystemProperties;

public class  EMStartReceiver extends BroadcastReceiver {

    private static final String TAG = "EMStartReceiver";

    public EMStartReceiver() {
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String host = null;
        Uri uri = intent.getData();
        if (uri != null) {
            host = uri.getHost();
        } else {
            Log.d(TAG,"uri is null");
            return;
        }
        Intent i = new Intent(Intent.ACTION_MAIN);
        i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        if("83781".equals(host)){
            i.setClass(context, EngineerModeActivity.class);
            context.startActivity(i);
        }
        else {
            if(SystemProperties.get("ro.product.board.customer", "none").equalsIgnoreCase("cgmobile")){
                //cg add by xuyouqin add cgversioninfo    start
                if("837868".equals(host)) {
                    i.setClass(context, cgversioninfo.class);
                    context.startActivity(i);
                } else if("837866".equals(host)) {
                    i.setClass(context, yulongversioninfo.class);
                    context.startActivity(i);
                }
                //cg add by xuyouqin add cgversioninfo    end
            }
        }
    }
}

