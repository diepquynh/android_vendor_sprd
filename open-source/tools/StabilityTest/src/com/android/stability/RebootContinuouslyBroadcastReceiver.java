package com.android.stability;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

public class RebootContinuouslyBroadcastReceiver extends BroadcastReceiver {

	@Override
	public void onReceive(Context context, Intent intent) {
		Context stabilityContext = null;
		try {
			stabilityContext = context.createPackageContext("com.android.stability", 
					Context.CONTEXT_INCLUDE_CODE|Context.CONTEXT_IGNORE_SECURITY);
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		SharedPreferences mPrefs = stabilityContext.getSharedPreferences("reboot_pref", Context.MODE_WORLD_READABLE|Context.MODE_WORLD_WRITEABLE);
		if (mPrefs.getInt("reboot_enable", 0) == 0) {
			return;
		}
		// TODO Auto-generated method stub
        Intent i = new Intent(context,RebootContinuouslyActivity.class);
		i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(i);  
	}
}
