package com.spread.cachefdn;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.telephony.TelephonyManager;
import com.android.internal.telephony.TelephonyIntents;
import android.util.Log;


public class WakeUpBroadCast extends BroadcastReceiver {

    private boolean mHasStarted = false;

	@Override
	public void onReceive(Context context, Intent intent) {
        Log.i("FdnService", "WakeUpBroadCast---->onReceive()");
	}
}

