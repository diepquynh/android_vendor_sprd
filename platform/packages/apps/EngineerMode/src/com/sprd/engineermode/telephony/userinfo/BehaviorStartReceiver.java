package com.sprd.engineermode.telephony.userinfo;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.widget.Toast;
import android.util.Log;
import android.os.SystemProperties;
import com.sprd.engineermode.telephony.userinfo.NetInfoRecordActivity;
import com.sprd.engineermode.telephony.userinfo.NetinfoRecordService;
import com.sprd.engineermode.telephony.userinfo.BehaviorInfoRecordActivity;
import com.sprd.engineermode.telephony.userinfo.BehaviorRecordService;

public class BehaviorStartReceiver extends BroadcastReceiver{

    private static final String TAG = "BehaviorStartReceiver";
    @Override
    public void onReceive(Context context,Intent intent){
        BehaviorInfoRecordActivity.bServiceIntent=new Intent(context.getApplicationContext(),BehaviorRecordService.class);
        BehaviorInfoRecordActivity.mIsOpenBehaviorRecord=true;
        BehaviorInfoRecordActivity.mIsOpenAll=true;
        SystemProperties.set("persist.sys.open.user.record", "true");
        context.startService(BehaviorInfoRecordActivity.bServiceIntent);
    }
}
