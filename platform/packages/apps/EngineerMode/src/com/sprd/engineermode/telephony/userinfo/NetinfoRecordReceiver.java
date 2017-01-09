package com.sprd.engineermode.telephony.userinfo;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.widget.Toast;
import android.util.Log;
import android.app.Service;
import android.os.SystemProperties;

public class NetinfoRecordReceiver extends BroadcastReceiver{
    private static final String TAG = "NetinfoRecordReceiver";
    private final String OPEN_NETINFO_RECORD_ACTION="sprd.beta.netinfo.start";
    private final String STOP_NETINFO_RECORD_ACTION="sprd.beta.netinfo.stop";
    @Override
    public void onReceive(Context context,Intent intent){
        String action = intent.getAction();
        //begin bug600587 modify by bo.yan 20160918
        if(action==null){
            return;
        }
        //end bug600587 modify by bo.yan 20160918
        if(action.equals(OPEN_NETINFO_RECORD_ACTION)){
            NetInfoRecordActivity.serviceIntent=new Intent(context.getApplicationContext(),NetinfoRecordService.class);
            NetInfoRecordActivity.mIsOpenNetinfoRecord=true;
            NetInfoRecordActivity.mIsOpenAllNetInfoItems=true;
            SystemProperties.set("persist.sys.open.net.record", "true");
            NetInfoRecordActivity.mIsCloseAllNetInfoItems=false;
            context.startService(NetInfoRecordActivity.serviceIntent);
        }
        if(action.equals(STOP_NETINFO_RECORD_ACTION)){
            if(NetInfoRecordActivity.serviceIntent!=null){
                context.stopService(NetInfoRecordActivity.serviceIntent);
                NetInfoRecordActivity.serviceIntent=null;
                SystemProperties.set("persist.sys.open.net.record", "false");
                NetInfoRecordActivity.mIsOpenNetinfoRecord=false;
                NetInfoRecordActivity.mIsOpenAllNetInfoItems=false;
                NetInfoRecordActivity.mIsCloseAllNetInfoItems=true;
            }
        }
    }
}
