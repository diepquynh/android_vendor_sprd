
package com.spreadtrum.dm;

import com.android.internal.telephony.Phone;
//import com.android.internal.telephony.PhoneFactory;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log; //import android.provider.Telephony;
//import android.provider.Telephony.Sms.Intents;
import android.telephony.SmsMessage;
import android.os.Bundle;
import android.telephony.TelephonyManager;

import android.content.ServiceConnection;
import android.content.ComponentName;
import android.os.IBinder; //import com.redbend.vdm.VdmException;
import android.provider.Settings;
import android.widget.Toast;

public class DmReceiver extends BroadcastReceiver {

    protected static final String DM_TAG = "DM ==> ";

    private String TAG = DM_TAG + "DmReceiver: ";

    private static final String WAP_PUSH_RECEIVED_ACTION = "android.provider.Telephony.WAP_PUSH_RECEIVED";

    private static final String DATA_SMS_RECEIVED_ACTION = "android.intent.action.DATA_SMS_RECEIVED";

    private static final String WALLPAPER_CHANGED_ACTION = "android.intent.action.WALLPAPER_CHANGED";

    private static final String DM_MIME_WBXML = "application/vnd.syncml.dm+wbxml";

    private static final String DM_MIME_XML = "application/vnd.syncml.dm+xml";

    private static final String DM_MIME_NOTIFY = "application/vnd.syncml.notification";
    
    private static final String SMSSOUND_ACTION = "com.android.dm.SmsSound";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();

        Log.d(TAG, "onReceive, action is " + action);

        if (action.equals(Intent.ACTION_BOOT_COMPLETED)) {
            Log.d(TAG, "onReceive, ACTION_BOOT_COMPLETED");            
            if(0 != Settings.System.getInt(context.getContentResolver(), "dm_config", 1)){                
                Intent selfRegService = new Intent("com.android.dm.SelfReg");
                context.startService(selfRegService);
            }else{
                Log.d(TAG, "onReceive, ACTION_BOOT_COMPLETED not run");
            }
        } else if (DATA_SMS_RECEIVED_ACTION.equals(action)) {
            Log.d(TAG, "onReceive, DATA_SMS_RECEIVED_ACTION");
            processDataSms(context, intent);
        } else if (WAP_PUSH_RECEIVED_ACTION.equals(action)) {
            Log.d(TAG, "onReceive, WAP_PUSH_RECEIVED_ACTION");            
            if(0 != Settings.System.getInt(context.getContentResolver(), "dm_config", 1)){                
                processWapPush(context, intent);
            }else{
                Log.d(TAG, "onReceive, WAP_PUSH_RECEIVED_ACTION not run");
            }
            
            //processWapPush(context, intent);
        } else if (SMSSOUND_ACTION.equals(action)) {
            Bundle bundle= intent.getExtras();
            String smsSoundUrlString = bundle.getString("smssound");
            Log.i(TAG, "DmSmsSoundReceiver  smsSoundUrlString = " + smsSoundUrlString);
            if(null != smsSoundUrlString){              
                DmService.getInstance().setSMSSoundUri(smsSoundUrlString);
            }else {
                Log.i(TAG, "DmSmsSoundReceiver:smsSoundUrlString is null");
            }   
        }        
/*
        else if (WALLPAPER_CHANGED_ACTION.equals(action))
        {
            Log.d(TAG, "onReceive, WAP_PUSH_RECEIVED_ACTION");

            processwallpaper(context, intent);
        }
*/
        else {
            Log.d(TAG, "onReceive, DM need not process!");
        }
    }

    // judge if the address is dm self registe sms address
    private boolean isDmSmsAddress(Context context, String addr) {
        boolean result = false;
        String selfRegSmsAddr = DmService.getInstance().getSmsAddr();

        if (addr.equals(selfRegSmsAddr)) {
            result = true;
        }
        Log.d(TAG, "isDmSmsAddress : return " + result);
        return result;
    }

    // parse reply message content
    private boolean parseReplyMsgContent(Context context, String msgBody, int phoneId) {
        boolean result = true; // is processed message
        //TelephonyManager mTelephonyManager = (TelephonyManager) context
        //       .getSystemService(Context.TELEPHONY_SERVICE);
        TelephonyManager mTelephonyManager = (TelephonyManager) context.getSystemService(
                TelephonyManager.getServiceName(Context.TELEPHONY_SERVICE, phoneId));
        
        String imsi = mTelephonyManager.getSubscriberId();
        String imei = DmService.getInstance().getImei();
        String okReply = "IMEI:" + imei + "/1";
        String failReply = "IMEI:" + imei + "/0";
        DmService dmService = DmService.getInstance();

        Log.d(TAG, "parseReplyMsgContent: msgBody is " + msgBody);
        Log.d(TAG, "parseReplyMsgContent: okReply is " + okReply);
        Log.d(TAG, "parseReplyMsgContent: failReply is " + failReply);
        if (msgBody.equals(okReply)) {
            Log.d(TAG, "parseReplyMsgContent: self registe successfully!"+phoneId);
        	 dmService.setCurrentPhoneID(phoneId);

            // save current card imsi to file
            dmService.saveImsi(context, imsi);
            // set self registe state to success
            dmService.setSelfRegState(context, true);
            dmService.stopListeningServiceState();
            if (dmService.isDebugMode()) {
                ShowMessage(context, "Self registe OK!");
            }
            // DmService.getContext().stopService(new
            // Intent("com.android.dm.SelfReg"));
        } else if (msgBody.equals(failReply)) {
            // self registe fail, need not save imsi
            Log.d(TAG, "parseReplyMsgContent: self registe fail!");
            // set self registe state to fail
            dmService.setSelfRegState(context, false);
            dmService.stopListeningServiceState();
            if (dmService.isDebugMode()) {
                ShowMessage(context, "Self registe fail!");
            }
            // DmService.getContext().stopService(new
            // Intent("com.android.dm.SelfReg"));
        } else {
            Log.d(TAG, "parseReplyMsgContent: not for dm!");
            result = false;
        }
        return result;
    }

    // process data message content
    private boolean processDataSms(Context context, Intent intent) {
        Bundle bundle = intent.getExtras();
        int phoneId = intent.getIntExtra(Phone.PHONE_ID, 0);
        SmsMessage[] msgs = null;
        String str = "";
        boolean result = false;

        if (bundle != null) {
            Log.d(TAG, "processDataSms: bundle is not null");
            // ---retrieve the SMS message received---
            Object[] pdus = (Object[]) bundle.get("pdus");
            msgs = new SmsMessage[pdus.length];
            for (int i = 0; i < msgs.length; i++) {
                msgs[i] = SmsMessage.createFromPdu((byte[]) pdus[i]);

                str = msgs[i].getOriginatingAddress();
                Log.d(TAG, "getOriginatingAddress : " + str);
                if (isDmSmsAddress(context, str)) {
                    str = msgs[i].getMessageBody().toString();
                    Log.d(TAG, "getMessageBody : " + str);
                    if (parseReplyMsgContent(context, str, phoneId)) {
                        // if have processed dm self registe reply message,
                        // break this cycle
                        Log.d(TAG, "processDataSms: have processed dm reply message");
                        result = true;
                        break;
                    }
                }
            }
        } else {
            Log.d(TAG, "processDataSms: bundle is null");
        }

        return result;
    }

    // simu para collection or setting
    private boolean processwallpaper(Context context, Intent intent) {
        boolean result = false;
        String addr = "1065840409";

        Log.d(TAG, "simulate  processWapPush: enter!");

        if (DmService.getInstance().isDebugMode()) {
            ShowMessage(context, "Receive dm push message!");
        }

        byte[] header = new byte[6];
        byte[] data = new byte[30];
/*
        header[0] = -60;
        header[1] = -81;
        header[2] = -121;
        header[3] = 0;
        data[0] = 114;
        data[1] = 119;
        data[2] = -30;
        data[3] = -69;
        data[4] = -32;
        data[5] = 51;
        data[6] = 118;
        data[7] = -29;
        data[8] = 43;
        data[9] = -22;
        data[10] = -71;
        data[11] = 41;
        data[12] = 1;
        data[13] = -120;
        data[14] = 66;
        data[15] = 99;
        data[16] = 2;
        data[17] = -8;
        data[18] = 0;
        data[19] = 0;
*/
        header[0] = -60;
        header[1] = -81;
        header[2] = -121;
        header[3] = 0;

        data[0] = -122;
        data[1] = -88;
        data[2] = -84;
        data[3] = 73;
        data[4] = -65;
        data[5] = -102;
        data[6] = -63;
        data[7] = -8;
        data[8] = 40;
        data[9] = 101;
        data[10] = -93;
        data[11] = 4;
        data[12] = -45;
        data[13] = -20;
        data[14] = 114;
        data[15] = -53;
        data[16] = 2;
        data[17] = -8;
        data[18] = 0;
        data[19] = 0;
        data[20] = 0;
        data[21] = -108;
        data[22] = 123;
        data[23] = 5;
        data[24] = 79;
        data[25] = 77;
        data[26] = 65;
        data[27] = 68;
        data[28] = 77;

        int i;
        for (i = 0; i < header.length; i++) {
            Log.d(TAG, "header[" + i + "] = " + header[i]);
        }

        Log.d(TAG, "data.length = " + data.length);
        for (i = 0; i < 20; i++) {
            Log.d(TAG, "data[" + i + "] = " + data[i]);
        }
        Intent vdmIntent = new Intent("com.android.dm.NIA");
        Bundle extras = new Bundle();
        extras.putByteArray("msg_body", data);
        extras.putString("msg_org", addr);
        vdmIntent.putExtras(extras);
        context.startService(vdmIntent);

        return result;
    }

    // process wap push message
    private boolean processWapPush(Context context, Intent intent) {
        boolean result = false;
        String type = intent.getType(); // data type
        String addr = intent.getStringExtra("from");
        String dmSmsAddr = DmService.getInstance().getSmsAddr();
                                     
        Log.d(TAG, "processWapPush: enter!");
        Log.d(TAG, "processWapPush: data type  " + type);
        Log.d(TAG, "processWapPush: from  " + addr);

        if (((DM_MIME_WBXML.equals(type)) || (DM_MIME_XML.equals(type)) || (DM_MIME_NOTIFY.equals(type))) && (addr.equals(dmSmsAddr))) {
            Log.d(TAG, "processWapPush: is for dm");

            if (DmService.getInstance().isDebugMode()) {
                ShowMessage(context, "Receive dm push message!");
            }

            byte[] header = intent.getByteArrayExtra("header");
            byte[] data = intent.getByteArrayExtra("data");
            int i;
            for (i = 0; i < header.length; i++) {
                Log.d(TAG, "header[" + i + "] = " + header[i]);
            }

            Log.d(TAG, "data.length = " + data.length);
            for (i = 0; i < data.length; i++) {
                Log.d(TAG, "data[" + i + "] = " + Integer.toHexString(data[i]&0xff));
            }
            Intent vdmIntent = new Intent("com.android.dm.NIA");
            Bundle extras = new Bundle();
            extras.putByteArray("msg_body", data);
            extras.putString("msg_org", addr);
            vdmIntent.putExtras(extras);
            context.startService(vdmIntent);
        }

        return result;
    }

    private Toast mToast;

    private void ShowMessage(Context context, CharSequence msg) {
        if (mToast == null)
            mToast = Toast.makeText(context, msg, Toast.LENGTH_LONG);
        mToast.setText(msg);
        mToast.show();
    }
}
