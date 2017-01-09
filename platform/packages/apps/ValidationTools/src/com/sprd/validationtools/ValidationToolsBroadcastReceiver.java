package com.sprd.validationtools;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;

public class ValidationToolsBroadcastReceiver extends BroadcastReceiver{
	
	public ValidationToolsBroadcastReceiver(){
	}
	
	@Override
	public void onReceive(Context context, Intent intent) {
            Uri uri = intent.getData();
            if(uri == null) return ;
            String host = uri.getHost();
            
            Intent i = new Intent(Intent.ACTION_MAIN);
            i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            
		if("83789".equals(host)){
                i.setClass(context, ValidationToolsMainActivity.class);
                context.startActivity(i);
                }	
	}

}
