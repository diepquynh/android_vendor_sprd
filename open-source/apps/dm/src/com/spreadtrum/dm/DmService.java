
package com.spreadtrum.dm;

import android.app.Service;
import android.app.Notification;
import android.app.NotificationManager;

import android.content.Context;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;

import android.content.SharedPreferences;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.telephony.SmsManager;
import android.os.IBinder;
import android.util.Log;
import android.os.SystemProperties;

import com.android.internal.telephony.Phone;
//import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.PhoneConstants;

import java.nio.ByteBuffer;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.net.Uri;
import com.android.internal.telephony.TelephonyIntents;

//import com.redbend.vdm.NIAMsgHandler.UIMode;
import android.app.AlertDialog;
import android.content.DialogInterface;

import android.provider.Settings;
import android.provider.Telephony;
import android.content.ContentValues;
import android.database.Cursor;
import com.android.internal.telephony.TelephonyProperties;
import com.spreadtrum.dm.transaction.DMTransaction;
import com.spreadtrum.dm.vdmc.MyTreeIoHandler;
import com.spreadtrum.dm.vdmc.Vdmc;
//import com.android.internal.telephony.PhoneFactory;
import android.app.PendingIntent;
import android.app.Activity;

public class DmService extends Service {
    private String TAG = "DmService";

    //add for 105942 begin
    private static final boolean IS_CONFIG_SELFREG_REPLY = false;
    private static final String SENT_SMS_ACTION ="com.spreadtrum.dm.SENT_SMS";
    private static final String DELIVERED_SMS_ACTION ="com.spreadtrum.dm.DELIVERED_SMS";
    //add for 105942 end

    private static final String mLastImsiFile = "lastImsi.txt";

    private static TelephonyManager[] mTelephonyManager;

    private static boolean mIsHaveSendMsg = false; // if have send self registe
                                                   // message

    private static boolean mIsSelfRegOk = false; // if self registe ok

    private static boolean mSelfRegSwitch = true; // true: open self registe
                                                  // function false:close self
                                                  // registe function

    private static final String PREFERENCE_NAME = "LastImsi";
    
    // bug 292626 begin
    private static final String PREFERENCE_AGPS = "AgpsParam";
    private static final String AGPS_SERVER = "AGPSServer";
    private static final String AGPS_NAME = "AGPSName";
    private static final String AGPS_IAPID = "IAPID";
    private static final String AGPS_PORT = "AGPSServerPort";
    private static final String AGPS_PROVIDERIP = "ProviderIP";
    private static final String AGPS_PREFCONREF = "PrefConRef";
    private static final String AGPS_CONREF = "ConRef";
    private static final String AGPS_CONNPROFILE = "ConApn";
    // bug 292626 end

    private static int MODE = MODE_PRIVATE;

    // private static DmService mInstance = null;
    private static DmService mInstance = null;

    private static Context mContext;

    private static int pppstatus = PhoneConstants.APN_TYPE_NOT_AVAILABLE;

    private static String mSmsAddr;

    private static String mSmsPort;

    private static String mServerAddr;

    private static String mApn = null;

    private static String mApnTemp = null;

    private static String mProxyTemp = null;

    private static String mProxyPortTemp = null;

    private static String mManufactory;

    private static String mModel;

    private static String mSoftVer;

    private static String mImeiStr;

    private static boolean mIsDebugMode;

    private static boolean mIsRealNetParam; // if current is read net parameter

    private static final String DM_CONFIG = "DmConfig";
    //Fix 204720 on 20130822:the alert sound should be same to sms sound
    private static final String SMS_SOUND = "SMS_SOUND";

    private static final String ITEM_DEBUG_MODE = "DebugMode";

    private static final String ITEM_REAL_PARAM = "RealParam";

    private static final String ITEM_MANUFACTORY = "Manufactory";

    private static final String ITEM_MODEL = "Model";

    private static final String ITEM_SOFTVER = "SoftVer";

    private static final String ITEM_IMEI = "IMEI";

    private static final String ITEM_SERVER_ADDR = "ServerAddr";

    private static final String ITEM_SMS_ADDR = "SmsAddr";

    private static final String ITEM_SMS_PORT = "SmsPort";

    private static final String ITEM_APN = "APN";

    private static final String ITEM_PROXY = "Proxy";

    private static final String ITEM_PROXY_PORT = "ProxyPort";

    private static final String ITEM_SELFREG_SWITCH = "SelfRegSwitch";

    // add by lihui
    private static final String ITEM_SERVER_NONCE = "ServerNonce";

    private static final String ITEM_CLIENT_NONCE = "ClientNonce";

    public static final String APN_CMDM = "cmdm";

    public static final String APN_CMWAP = "cmwap";

    public static final String APN_CMNET = "cmnet";

    // Real net parameter
    private static final String REAL_SERVER_ADDR = "http://dm.monternet.com:7001";

    private static final String REAL_SMS_ADDR = "10654040";

    private static final String REAL_SMS_PORT = "16998";

    private static final String REAL_APN = APN_CMDM;

    // Lab net parameter
    private static final String LAB_SERVER_ADDR = "http://218.206.176.97:7001";

    // private static final String LAB_SERVER_ADDR = "http://218.206.176.97";
    private static final String LAB_SMS_ADDR = "1065840409";

    private static final String LAB_SMS_PORT = "16998";

    private static final String LAB_APN = APN_CMWAP;

    private final Object keepcurphone = new Object(); 
    //TODO: to be confirm
//    private DmJniInterface mDmInterface;

//    private DMTransaction mDmTransaction;

    private static DmNetwork mNetwork = null;

    private static MyTreeIoHandler mTreeIoHandler = null;

    private Handler mHandler;

    private Uri mUri;

    private Cursor mCursor;

    private boolean mSmsReady[];

    private boolean mInService[];

    private  DmNativeInterface mDmNativeInterface;

    private PhoneStateListener[] mPhoneStateListener;

    private int mPhoneCnt = 0;
    private int curPhoneId = 0;
    public int mStartid= 0;  
    
    //CMCC Config share prefence
    private static final String CMCC_CONFIG = "CMCCConfig";
    
    private static int mCurrentCMCCSimNum;
  //4 send sms
    private static final String ITEM_CMCC_REGISTED_STATUS = "DmRegistedStatus";
    private static final String ITEM_CMCC_LAST_IMSI = "DmLastImsi";

    private static final String ITEM_CMCC_REJECT_STATUS_PHONE0 = "DmRejectStatusPhone0";
    private static final String ITEM_CMCC_REJECT_IMSI0 = "DmRejectImsi0";
    private static final String ITEM_CMCC_REJECT_STATUS_PHONE1 = "DmRejectStatusPhone1";
    private static final String ITEM_CMCC_REJECT_IMSI1 = "DmRejectImsi1";
    
    private static final String ITEM_CMCC_IMSI_1 = "DmImsi1";
    private static final String ITEM_CMCC_IMSI_2 = "DmImsi2";
    
    private static String mImsi1;
    private static String mImsi2;    
    private static String mRejectImsi0;    
    private static String mRejectImsi1;
    private static boolean mIsRejectPhone0;
    private static boolean mIsRejectPhone1;
    
    //4 NIA
    private static final String ITEM_CMCC_ALLOWED_DATACONNECT = "DmAllowDataConnect";
    private static final String ITEM_CMCC_DATACONNECT_ALWAYS_STATUS = "DmDataConnectAlwaysStatus";      
    private static boolean mAllowedDataConnect;
    private static boolean mDataConnectAlwaysStatus;
    
    public static final int SEND_SELF_SMS = 20;
    public static final int DATA_CONNECT_CONFRIM = 21;    
    public static boolean isDialogShowed = false;
    private static boolean mSystemShutDown = false;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.startsWith(TelephonyIntents.ACTION_IS_SIM_SMS_READY)) {//|| action.equals(TelephonyIntents.ACTION_IS_SIM2_SMS_READY)) {

                int phoneId = intent.getIntExtra(TelephonyIntents.EXTRA_PHONE_ID, 0);
                mSmsReady[phoneId] = intent.getBooleanExtra("isReady", true);
                initCMCCConfig();
                Log.d(TAG, "[sms]onReceive ACTION_IS_SIM_SMS_READY mSmsReady["+ phoneId + "] = " + mSmsReady[phoneId]);
                if (mSmsReady[phoneId]) {
                    if (TelephonyManager.SIM_STATE_READY == mTelephonyManager[phoneId].getSimState()
                            && mInService[phoneId]) {

                        if (getIsCmccCard(phoneId)) {
	                    curPhoneId = phoneId;	                    
                            Log.d(TAG, "[sms]onReceive ACTION_IS_SIM_SMS_READY: is cmcc card! curPhoneId = " + curPhoneId);

                            // send self registe message
                            if(mPhoneCnt > 1 /*TelephonyManager.isMultiSim() */){
                                int otherPhoneId = phoneId == 0 ? 1 : 0;

                                if(!mTelephonyManager[otherPhoneId].hasIccCard() || mSmsReady[otherPhoneId] && mInService[otherPhoneId]){
                                    //sendSelfRegMsg();                                    
                                    showDialog4SendDMSms();                                    
                                }
                            }else{
                                //sendSelfRegMsg();                                
                                showDialog4SendDMSms();                                
                            }
                        } else {
                            Log.d(TAG, "[sms]onReceive ACTION_IS_SIM_SMS_READY: not cmcc card!");
                            stopListeningServiceState(phoneId);
                            if(mPhoneCnt > 1 ){
                                int otherPhoneId = (phoneId == 0) ? 1 : 0;
                                curPhoneId = otherPhoneId;				
                                if (mSmsReady[otherPhoneId] && mInService[otherPhoneId] && getIsCmccCard(otherPhoneId)
                                    && TelephonyManager.SIM_STATE_READY == mTelephonyManager[otherPhoneId].getSimState()){
                                    Log.d(TAG, "[sms]onReceive ACTION_IS_SIM_SMS_READY: use other sim selfReg PhoneId="+otherPhoneId);
                                    curPhoneId = otherPhoneId;
                                    //sendSelfRegMsg();
                                    showDialog4SendDMSms();
                                }
                            }
                        }
                    } else {
                        Log.d(TAG, "[sms]onReceive ACTION_IS_SIM_SMS_READY: sim state = "
                                + mTelephonyManager[phoneId].getSimState());
                    }
                }
            }

        }
    };
    
    private BroadcastReceiver mShutdownReceiver = new BroadcastReceiver(){
    	public void onReceive(Context context, Intent intent) {
    		Log.d(TAG, "onReceive: ACTION_SHUTDOWN, mSystemShutDown is true");
    		mSystemShutDown = true;
    	}
    };

    private boolean getIsCmccCard(int phoneId){
        String cmccStr1 = "46000";
        String cmccStr2 = "46002";
        String cmccStr3 = "46007";
        String curOper = mTelephonyManager[phoneId].getNetworkOperator();        
        Log.d(TAG, "getIsCmccCard  phoneId ="+phoneId+" curOper:"+curOper);
        if (curOper.equals(cmccStr1) || curOper.equals(cmccStr2) || curOper.equals(cmccStr3)){
            return true; 
        }else{
            return false; 
        }
    }

    private PhoneStateListener getPhoneStateListener(final int phoneId) {
        PhoneStateListener phoneStateListener = new PhoneStateListener() {
            @Override
            public void onServiceStateChanged(ServiceState serviceState) {                
                Log.d(TAG, "onServiceStateChanged: phoneId = " + phoneId + ",current state = " + serviceState.getState());
		synchronized(keepcurphone)
		{
                // judge if network is ready
                if (ServiceState.STATE_IN_SERVICE == serviceState.getState()) {
                    Log.d(TAG, "onServiceStateChanged: STATE_IN_SERVICE");
                    
                    if(mSystemShutDown){
                    	return;
                    }
                    
                    mInService[phoneId] = true;
                    // sim card is ready
                    if (TelephonyManager.SIM_STATE_READY == mTelephonyManager[phoneId].getSimState()
                            && (mSmsReady[phoneId] || "true".equals(SystemProperties.get(TelephonyManager.getProperty("gsm.sim.smsready", phoneId),"false" )))) {
      
                        if (getIsCmccCard(phoneId)) {
	                    curPhoneId = phoneId;	                    
                            Log.d(TAG, "onServiceStateChanged: is cmcc card! curPhoneId = " + curPhoneId);
                            initCMCCConfig();//SPRD:326895
                            // send self registe message
                            if(mPhoneCnt > 1 /*TelephonyManager.isMultiSim()*/){
                                int otherPhoneId = phoneId == 0 ? 1 : 0;
                                if(!mTelephonyManager[otherPhoneId].hasIccCard() || mSmsReady[otherPhoneId] && mInService[otherPhoneId]){
                                    //sendSelfRegMsg();                                    
                                    showDialog4SendDMSms();
                                }
                            }else{
                                //sendSelfRegMsg();                                
                                showDialog4SendDMSms();
                            }
                        } else {
                            Log.d(TAG, "onServiceStateChanged: not cmcc card!");                            
                            stopListeningServiceState(phoneId);
                            if(mPhoneCnt > 1 ){
                                int otherPhoneId = (phoneId == 0) ? 1 : 0;
                                curPhoneId = otherPhoneId;
                                if (mSmsReady[otherPhoneId] && mInService[otherPhoneId] && getIsCmccCard(otherPhoneId)
                                    && TelephonyManager.SIM_STATE_READY == mTelephonyManager[otherPhoneId].getSimState()){
                                    Log.d(TAG, "onServiceStateChanged use other sim selfReg PhoneId="+otherPhoneId);
                                    curPhoneId = otherPhoneId;
                                    //sendSelfRegMsg();                                    
                                    showDialog4SendDMSms();
                                }
                            }
                        }
                    } else {
                        Log.d(TAG, "onServiceStateChanged: sim state = "
                                + mTelephonyManager[phoneId].getSimState());
                    }
                }
		} //synchronized
            }
        };
        return phoneStateListener;
    }


    private class DMHandler extends Handler {

        public void handleMessage(Message msg) {

        }
    };

    @Override
    public void onCreate() {
        mPhoneCnt = TelephonyManager.getPhoneCount();
        mSmsReady = new boolean[mPhoneCnt];
        mInService = new boolean[mPhoneCnt];

        Log.d(TAG, "onCreate: mPhoneCnt "+mPhoneCnt);	
	
        mTelephonyManager = new TelephonyManager[mPhoneCnt];
        mPhoneStateListener = new PhoneStateListener[mPhoneCnt];

        for(int phoneId=0; phoneId<mPhoneCnt; phoneId++){
            mTelephonyManager[phoneId] = (TelephonyManager) getSystemService(TelephonyManager.
                    getServiceName(Context.TELEPHONY_SERVICE,phoneId));
            mPhoneStateListener[phoneId] = getPhoneStateListener(phoneId);
            mTelephonyManager[phoneId].listen(mPhoneStateListener[phoneId],
                    PhoneStateListener.LISTEN_SERVICE_STATE);
        }

        mContext = getBaseContext();

        mInstance = this;
        Log.d(TAG, "OnCreate: mInstance = " + mInstance);
/**** set foreground **************/
//	NotificationManager mNotificationManager=null;
//        mNotificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
//	Notification notify = new Notification();
//	notify.flags |= Notification.FLAG_FOREGROUND_SERVICE;
//	super.startForeground(10,notify);
//        Log.d(TAG, "OnCreate: setForeground ");
//	mNotificationManager.notify(10, notify);
/**** set foreground **************/

        mHandler = new DMHandler();
        //TODO: to be confirm
        mDmNativeInterface = new DmNativeInterface(mContext, mHandler);
    
        initParam();
        /*
        if (!mIsHaveInit)
        // control this init process only run once
        { // init dm relative parameters
            initParam(); // Start listening to service state
            if (getSelfRegSwitch()) {
                if (isNeedSelfReg()) {
                    mTelephonyManager.listen(mPhoneStateListener,
                            PhoneStateListener.LISTEN_SERVICE_STATE);
                } else {
                    setSelfRegState(true);
                }
            }
            mIsHaveInit = true;
        }
         */

//        mDmInterface = new DmJniInterface(mContext);
//        DMNativeMethod.JsaveDmJniInterfaceObject(mDmInterface);
//
//        mDmTransaction = new DMTransaction(mContext, mHandler);
//        DMNativeMethod.JsaveDmTransactionObject(mDmTransaction);

        if (mNetwork == null) {
            mNetwork = new DmNetwork(mContext);
        } else {
        }
		
        if (mTreeIoHandler == null) {
            mTreeIoHandler = new MyTreeIoHandler(mContext);
        } else {
        }


        // Listen for broadcast intents that indicate the SMS is ready
/*        
        IntentFilter filter = new IntentFilter(TelephonyIntents.ACTION_IS_SIM_SMS_READY);
        IntentFilter filter2 = new IntentFilter(TelephonyIntents.ACTION_IS_SIM2_SMS_READY);
        Intent intent1 = registerReceiver(mReceiver, filter);
		Log.d(TAG, "onCreate: ACTION_IS_SIM_SMS_READY register ");
        if(mPhoneCnt == 2){
            Intent intent2 = registerReceiver(mReceiver, filter2);
		Log.d(TAG, "onCreate: ACTION_IS_SIM2_SMS_READY register ");
        }
*/
		int num;
	        IntentFilter filter = new IntentFilter(TelephonyIntents.ACTION_IS_SIM_SMS_READY);
	        Intent intent1 = registerReceiver(mReceiver, filter);
			Log.d(TAG, "onCreate: ACTION_IS_SIM_SMS_READY register ");
		for (num= 0; num < mPhoneCnt; num++)
		{
	         filter = new IntentFilter(TelephonyManager.getAction(TelephonyIntents.ACTION_IS_SIM_SMS_READY,num));//TelephonyIntents.ACTION_IS_SIM_SMS_READY
	         intent1 = registerReceiver(mReceiver, filter);
			Log.d(TAG, "onCreate: ACTION_IS_SIM_SMS_READY register "+num);
		}
		//bug272743 begin
		IntentFilter shutdownFilter = new IntentFilter();
		shutdownFilter.addAction(Intent.ACTION_SHUTDOWN);
		registerReceiver(mShutdownReceiver, shutdownFilter);
		//bug272743 end
    }

    @Override
    public void onDestroy() {
        // Stop listening for service state           
        stopListeningServiceState();                
        unregisterReceiver(mReceiver);
        unregisterReceiver(mShutdownReceiver);//bug272743
        Log.d(TAG, "onDestroy: DmService is killed!");
        mInstance = null;
        mContext = null;
	mDmNativeInterface = null;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Deprecated
    public void onStart(Intent intent, int startId) {
        Log.d(TAG, "onStart: intent = " + intent + ", startId = " + startId);              
                
        mStartid = startId;
        if (intent == null) {
            return;
        }

        if (intent.getAction().equals("com.android.dm.SelfReg")) {
            Log.d(TAG, "onStart: com.android.dm.SelfReg");
            // Start listening to service state
            if (getSelfRegSwitch()) {
                setIsHaveSendSelfRegMsg(mContext, false);
                for(int phoneId = 0; phoneId < mPhoneCnt; phoneId++){
//                    mTelephonyManager[phoneId].listen(mPhoneStateListener[phoneId],
//                            PhoneStateListener.LISTEN_SERVICE_STATE);
                }
                /*
                 * if (isNeedSelfReg()) { setIsHaveSendSelfRegMsg(mContext,
                 * false); mTelephonyManager.listen(mPhoneStateListener,
                 * PhoneStateListener.LISTEN_SERVICE_STATE); } else {
                 * setSelfRegState(mContext, true); //mContext.stopService(new
                 * Intent("com.android.dm.SelfReg")); }
                 */
            }
        } else if (intent.getAction().equals("com.android.dm.NIA")) {
            Log.d(TAG, "onStart: com.android.dm.NIA");            
            SharedPreferences sharedPreferences = mContext.getSharedPreferences(CMCC_CONFIG, MODE);
            mAllowedDataConnect = sharedPreferences.getBoolean(ITEM_CMCC_ALLOWED_DATACONNECT, true);
            mDataConnectAlwaysStatus = sharedPreferences.getBoolean(ITEM_CMCC_DATACONNECT_ALWAYS_STATUS, false);

            if (!mAllowedDataConnect && mDataConnectAlwaysStatus) {
                Log.d(TAG,
                        "NIA not run beacuse user won't to connect data");
                return;
            } else if (mAllowedDataConnect && mDataConnectAlwaysStatus) {
                Log.d(TAG,
                        "NIA run without alert dialog beacuse user choose always connect data");                
                startVDM4NIA(intent);
            } else {
                showDialog4DataConnect(intent);
            }
        }
    }
    
    
    public void startVDM4NIA(Intent intent){      
        Log.d(TAG,"startVDM4NIA");
        if ( isSelfRegOk()){
                initConnectParam(); // insure dm connect network param is properly
                
                byte[] body = intent.getByteArrayExtra("msg_body");
                String origin = intent.getStringExtra("msg_org");
                
                Vdmc.getInstance().startVDM(mContext, Vdmc.SessionType.DM_SESSION_SERVER, body, origin);
        }
        else{
            Log.d(TAG, "onStart: selfregister not ok");
        }            
    }
    //
    

    public static DmService getInstance() {
        if (null == mInstance) {
            mInstance = new DmService();
            // Log.d("DM ==> DmService: ",
            // "getInstance: new DmService() , mInstance = " + mInstance);
        }
        // Log.d("DM ==> DmService: ", "getInstance: mInstance = " + mInstance);
        return mInstance;
    }

    //TODO: to be confirm
//    public DmJniInterface getDmInterface() {
//        return mDmInterface;
//    }

    public DmNativeInterface getDmNativeInterface() {
        if(mDmNativeInterface == null)
        {
	Log.d(TAG, "getDmNativeInterface is null, reCreate DMNativeInterface");
        mDmNativeInterface = new DmNativeInterface(mContext, mHandler);			
        	}
        return mDmNativeInterface;
    }
    public void clearDmNativeInterface() {
	Log.d(TAG, "clearDmNativeInterface");
	mDmNativeInterface = null;
    }
    public static Context getContext() {
        // Log.d("DM ==> DmService: ", "getContext: mContext = " + mContext);
        return mContext;
    }

    // Stop listening for service state
    public void stopListeningServiceState() {
        for(int phoneId = 0; phoneId < mPhoneCnt; phoneId++){
            if (null != mTelephonyManager[phoneId]) {
                mTelephonyManager[phoneId].listen(mPhoneStateListener[phoneId], 0);
            }
        }
        Log.d(TAG, "stop listen service state for all phone");        
    }

    public void stopListeningServiceState(int phoneId) {
        if (null != mTelephonyManager[phoneId]) {
            mTelephonyManager[phoneId].listen(mPhoneStateListener[phoneId], 0);
        }
        Log.d(TAG, "stop listen service state for phone " + phoneId);
    }
    //Fix 204720 on 20130822:the alert sound should be same to sms sound start
    public void setSMSSoundUri(String newSmsSoundString){
		if (mContext == null) {
			Log.d(TAG, " The mContext is null,so return ");
			return;
		}
    	SharedPreferences smsSoundSP;        
        smsSoundSP = getSharedPreferences(SMS_SOUND,MODE);
        String smsSoundString = smsSoundSP.getString("smssound_dm", "Default");
        if(!smsSoundString.equals(newSmsSoundString)){
        	SharedPreferences.Editor editor = smsSoundSP.edit();
            editor.putString("smssound_dm", newSmsSoundString);
            editor.commit();
        }        
    }
    //Fix 204720 on 20130822:the alert sound should be same to sms sound end
    // init dm relative parameter
    private void initParam() {
        SharedPreferences sharedPreferences;
      //Fix 204720 on 20130822:the alert sound should be same to sms sound
        SharedPreferences smsSoundSP;
        sharedPreferences = getSharedPreferences(DM_CONFIG, MODE);
        
        smsSoundSP = getSharedPreferences(SMS_SOUND,MODE);                
        Log.i(TAG, "initParam smssound_dm = " + smsSoundSP.getString("smssound_dm", "Default"));
        
        // init self registe switch
        if ((SystemProperties.get("ro.hisense.cmcc.test.datong", "0").equals("1"))
                || (SystemProperties.get("ro.hisense.cta.test", "0").equals("1"))) {
            // close self registe function
            setSelfRegSwitch(mContext, false);
        } else {
            // default is open
            mSelfRegSwitch = sharedPreferences.getBoolean(ITEM_SELFREG_SWITCH, true);
        }

        // init debug mode
        mIsDebugMode = sharedPreferences.getBoolean(ITEM_DEBUG_MODE, false);

        if (mIsDebugMode) {
            // init manufacture
            if(SystemProperties.get("ro.product.board.customer", "none").equalsIgnoreCase("cgmobile")){
                mManufactory = sharedPreferences.getString(ITEM_MANUFACTORY, "YuLong");

                // init model
                mModel = sharedPreferences.getString(ITEM_MODEL, "Coolpad 8019");

                // init software version
                // String softVer =
                // SystemProperties.get("ro.hisense.software.version",
                // Build.UNKNOWN);
                mSoftVer = sharedPreferences.getString(ITEM_SOFTVER, "4.4.001.P1.8019");
            }else{
                mManufactory = sharedPreferences.getString(ITEM_MANUFACTORY, "K-Touch");

                // init model
                mModel = sharedPreferences.getString(ITEM_MODEL, "K-Touch T621");

                // init software version
                // String softVer =
                // SystemProperties.get("ro.hisense.software.version",
                // Build.UNKNOWN);
                mSoftVer = sharedPreferences.getString(ITEM_SOFTVER, "4G_W4_TD_MocorDroid2.2_W11.32");
            }

            // init imei
            //mTelephonyManager = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
            mImeiStr = sharedPreferences.getString(ITEM_IMEI, mTelephonyManager[0].getDeviceId());
        } else {
            if(SystemProperties.get("ro.product.board.customer", "none").equalsIgnoreCase("cgmobile")){
                        // init manufacture
                        // mManufactory = "4G";
                        mManufactory = sharedPreferences.getString(ITEM_MANUFACTORY, "YuLong");
                        // init model
                        // mModel = "W4";
                        mModel = sharedPreferences.getString(ITEM_MODEL, "Coolpad 8019");

                        // init software version
                        mSoftVer = SystemProperties.get("ro.build.display.id",
                                "4.4.001.P1.8019");
            }else{
                        // init manufacture
                        // mManufactory = "4G";
                        mManufactory = sharedPreferences.getString(ITEM_MANUFACTORY, "K-Touch");
                        // init model
                        // mModel = "W4";
                        mModel = sharedPreferences.getString(ITEM_MODEL, "K-Touch T621");

                        // init software version
                        mSoftVer = SystemProperties.get("ro.hisense.software.version",
                                "T72_V2.0_111230_CMCC_120104_W11.49_p3.2");
            }

            // init imei
            //mTelephonyManager = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
            mImeiStr = mTelephonyManager[0].getDeviceId();
        }

        createDmApn();
        // according to cmcc test flag to decide current server relative
        // parameters
        if (SystemProperties.get("ro.hisense.cmcc.test", "0").equals("1")) {
            setRealNetParam(mContext, false, false);
        } else {
            // init if use real net parameter
            if(SystemProperties.get("ro.product.board.customer", "none").equalsIgnoreCase("cgmobile")){
                mIsRealNetParam = sharedPreferences.getBoolean(ITEM_REAL_PARAM, false);
            }else{
                mIsRealNetParam = sharedPreferences.getBoolean(ITEM_REAL_PARAM, true);
            }
        }

        // init server address/sms address/sms port
        if (mIsRealNetParam) {
            mServerAddr = sharedPreferences.getString(ITEM_SERVER_ADDR, REAL_SERVER_ADDR);
            mSmsAddr = sharedPreferences.getString(ITEM_SMS_ADDR, REAL_SMS_ADDR);
            mSmsPort = sharedPreferences.getString(ITEM_SMS_PORT, REAL_SMS_PORT);
        } else {
            mServerAddr = sharedPreferences.getString(ITEM_SERVER_ADDR, LAB_SERVER_ADDR);
            mSmsAddr = sharedPreferences.getString(ITEM_SMS_ADDR, LAB_SMS_ADDR);
            mSmsPort = sharedPreferences.getString(ITEM_SMS_PORT, LAB_SMS_PORT);
        }               
        
        // init apn/proxy/port
        //initConnectParam();
    }

    private void initCMCCConfig(){
        SharedPreferences cmccConfig;                                
        cmccConfig = getSharedPreferences(CMCC_CONFIG,MODE);
        SharedPreferences.Editor editor = cmccConfig.edit();
          
        mAllowedDataConnect = cmccConfig.getBoolean(ITEM_CMCC_ALLOWED_DATACONNECT, true);
        mDataConnectAlwaysStatus = cmccConfig.getBoolean(ITEM_CMCC_DATACONNECT_ALWAYS_STATUS, false);
        Log.d(TAG, "initCMCCConfig mAllowedDataConnect = " + mAllowedDataConnect + " mDataConnectAlwaysStatus = " + mDataConnectAlwaysStatus);
        
        mImsi1 = cmccConfig.getString(ITEM_CMCC_IMSI_1, "default");
        mImsi2 = cmccConfig.getString(ITEM_CMCC_IMSI_2, "default");
        Log.d(TAG, "initCMCCConfig ITEM_CMCC_IMSI_1: " + mImsi1 + " ITEM_CMCC_IMSI_2 = " + mImsi2);
        
        mIsRejectPhone0 = cmccConfig.getBoolean(ITEM_CMCC_REJECT_STATUS_PHONE0, false); 
        mIsRejectPhone1 = cmccConfig.getBoolean(ITEM_CMCC_REJECT_STATUS_PHONE1, false);                 
        mRejectImsi0 = cmccConfig.getString(ITEM_CMCC_REJECT_IMSI0, "default");
        mRejectImsi1 = cmccConfig.getString(ITEM_CMCC_REJECT_IMSI1, "default");
        
        Log.d(TAG, "initCMCCConfig ITEM_CMCC_REJECT_STATUS_PHONE0 = " + mIsRejectPhone0 + " ITEM_CMCC_REJECT_IMSI0 =" + mRejectImsi0);
        Log.d(TAG, "initCMCCConfig ITEM_CMCC_REJECT_STATUS_PHONE1 = " + mIsRejectPhone1 + " ITEM_CMCC_REJECT_IMSI1 =" + mRejectImsi1);
           
        Log.d(TAG, "initCMCCConfig mPhoneCnt  imsiPhone0 = " + mImsi1 + " imsiPhone1 = " + mImsi2);
        
        if(mPhoneCnt > 1){                        
            mImsi1 = mTelephonyManager[0].getSubscriberId();
            mImsi2 = mTelephonyManager[1].getSubscriberId();                
            Log.d(TAG, "initCMCCConfig mPhoneCnt  imsiPhone0 = " + mImsi1 + " imsiPhone1 = " + mImsi2);
            
            if(((mImsi1 != null) && (mImsi1.startsWith("46000") || mImsi1.startsWith("46002") || mImsi1.startsWith("46007"))) &&
            		((mImsi2 != null) && (mImsi2.startsWith("46000") || mImsi2.startsWith("46002") || mImsi2.startsWith("46007")))){
            	Log.d(TAG, "initCMCCConfig phone0 and phone1 is sim card");
            	mCurrentCMCCSimNum = 2;
            }else if(((mImsi1 != null) && (mImsi1.startsWith("46000") || mImsi1.startsWith("46002") || mImsi1.startsWith("46007"))) ||
            		((mImsi2 != null) && (mImsi2.startsWith("46000") || mImsi2.startsWith("46002") || mImsi2.startsWith("46007")))){
            	Log.d(TAG, "initCMCCConfig phone0 or phone1 is sim card");
            	mCurrentCMCCSimNum = 1;
            }else{
            	Log.d(TAG, "initCMCCConfig phone0 and phone1 is not sim card");
            	mCurrentCMCCSimNum = 0;
            }                       
        }else{//for single sim card status        	
        	mImsi1 = mTelephonyManager[0].getSubscriberId();
        	Log.d(TAG, "initCMCCConfig mPhoneCnt  imsiPhone0 only one sim card contained mImsi1 = " + mImsi1);
        	
        	if((mImsi1 != null) && (mImsi1.startsWith("46000") || mImsi1.startsWith("46002") || mImsi1.startsWith("46007"))){
        		Log.d(TAG, "initCMCCConfig phone0 is sim card");
        		mCurrentCMCCSimNum = 1;
        	}else{
        		Log.d(TAG, "initCMCCConfig phone0 is not sim card");
        		mCurrentCMCCSimNum = 0;
        	}
        }        

        Log.d(TAG, "initCMCCConfig mCurrentCMCCSimNum = " + mCurrentCMCCSimNum);                
        editor.putString(ITEM_CMCC_IMSI_1, mImsi1);
        editor.putString(ITEM_CMCC_IMSI_2, mImsi2);                 
        editor.commit();                      
    }
    
    // init dm connect network param,include apn/proxy/port
    private void initConnectParam() {

       //add for 106648 begin
        int attempts = 0;
        
        while (attempts < 60) {
          String numeric = android.os.SystemProperties.get(
                  TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, curPhoneId), "");
          	Log.e("michael", "initConnectParam numeric = " + numeric + " attempts = " + attempts);
            if (numeric != null && numeric.length() >= 5) {
                Log.d(TAG, "initConnectParam numeric: " + numeric);
                break;
            }
            
            try {
                Thread.sleep(1000);
            } catch (InterruptedException ie) {
                // Keep on going until max attempts is reached.
                Log.e(TAG, "initConnectParam attempts error!!");
            }
            attempts++;
        }
        Log.d(TAG, "initConnectParam attempts: " + attempts);
        //add for 106648 end

	
        createDmApn();

        // according to cmcc test flag to decide current server relative
        // parameters
        SharedPreferences sharedPreferences;
        sharedPreferences = getSharedPreferences(DM_CONFIG, MODE);
        if (SystemProperties.get("ro.hisense.cmcc.test", "0").equals("1")) {
            setRealNetParam(mContext, false, true);
        } else {
            // init if use real net parameter
            if(SystemProperties.get("ro.product.board.customer", "none").equalsIgnoreCase("cgmobile")){
                mIsRealNetParam = sharedPreferences.getBoolean(ITEM_REAL_PARAM, false);
            }else{
                mIsRealNetParam = sharedPreferences.getBoolean(ITEM_REAL_PARAM, true);
            }
        }

        if (mIsRealNetParam) {
            // mApn = sharedPreferences.getString(ITEM_APN, REAL_APN);
            mApn = getInitApn(mContext);
            if (mApn == null) {
                mApn = REAL_APN;
                setAPN(mContext, mApn);
            }
        } else {
            // mApn = sharedPreferences.getString(ITEM_APN, LAB_APN);
            mApn = getInitApn(mContext);
            if (mApn == null || mApn.equals(REAL_APN) ) {
                mApn = LAB_APN;
                setAPN(mContext, mApn);
            }
        }
        if (mApn.equals(APN_CMWAP)) {
            // cmwap apn, need set proxy and proxy port
            String str = null;
            str = getProxy(mContext);
            if (str == null || !str.equals("10.0.0.172")) {
                setProxy(mContext, "10.0.0.172");
            }
            str = getProxyPort(mContext);
            if (str == null || !str.equals("80")) {
                setProxyPort(mContext, "80");
            }
        } else {
            // cmnet or cmdm apn, no need set proxy and proxy port
            if (getProxy(mContext) != null) {
                setProxy(mContext, null);
            }
            if (getProxyPort(mContext) != null) {
                setProxyPort(mContext, null);
            }
        }
    }

    private void createDmApn() {    	
        // Add new apn
          String numeric = android.os.SystemProperties.get(
                  TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, curPhoneId), "");	          
          
        if (numeric == null || numeric.length() < 5) {
            Log.d(TAG, "createDMApn numeric: " + numeric);            
            return;
        }
        
        final String selection = "name = 'CMCC DM' and numeric=\"" + numeric + "\"";

        Log.d(TAG, "createDmApn: selection = " + selection);

        mCursor = mContext.getContentResolver().query(
		(curPhoneId == 0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(curPhoneId,null), null,
                selection, null, null);
		        
        if (mCursor != null && mCursor.getCount() > 0) {
            Log.d(TAG, "createDMApn mCursor.getCount(): " + mCursor.getCount());            
            mCursor.close();
            return;
        }
        
        ContentValues values = new ContentValues();
        values.put(Telephony.Carriers.NAME, "CMCC DM");
        if (numeric != null && numeric.length() > 4 ) {
            // Country code
            String mcc = numeric.substring(0, 3);
            // Network code
            String mnc = numeric.substring(3);
            // Auto populate MNC and MCC for new entries, based on what SIM
            // reports
            values.put(Telephony.Carriers.MCC, mcc);
            values.put(Telephony.Carriers.MNC, mnc);
            values.put(Telephony.Carriers.NUMERIC, numeric);
        }
        values.put(Telephony.Carriers.TYPE, "dm");
	 if ( numeric.equals("46002") || numeric.equals("46000") || numeric.equals("46007"))
	{
	        mUri = getContentResolver().insert((curPhoneId == 0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(curPhoneId,null), values);
			
	        if (mUri == null) {
	            Log.w(TAG, "Failed to insert new telephony provider into " +
	                   Telephony.Carriers.CONTENT_URI +"curPhoneId"+ curPhoneId);
	            return;
	        }
	 }
    }

    // get last successful self registe card imsi
    private String getLastImsi() {
        SharedPreferences sharedPreferences = getSharedPreferences(PREFERENCE_NAME, MODE);
        String imsi = sharedPreferences.getString("IMSI", "");
        Log.d(TAG, "getLastImsi : imsi = " + imsi);
        return imsi;
    }

    // save current self registe success card imsi
    protected boolean saveImsi(Context context, String imsi) {
        Log.d(TAG, "saveImsi: imsi = " + imsi);
        SharedPreferences sharedPreferences = context.getSharedPreferences(PREFERENCE_NAME, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString("IMSI", imsi);
        editor.commit();
        return true;
    }

    // get current self registe state
    protected boolean isSelfRegOk() {
        SharedPreferences sharedPreferences = getSharedPreferences(PREFERENCE_NAME, MODE);
        mIsSelfRegOk = sharedPreferences.getBoolean("IsSelfRegOk", false);
        Log.d(TAG, "isSelfRegOk: return " + mIsSelfRegOk);
        return mIsSelfRegOk;
    }

    // set current self registe state
    protected void setSelfRegState(Context context, boolean isSuccess) {
        Log.d(TAG, "setSelfRegState: to " + isSuccess);
        mIsSelfRegOk = isSuccess;
        SharedPreferences sharedPreferences = context.getSharedPreferences(PREFERENCE_NAME, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putBoolean("IsSelfRegOk", isSuccess);
        editor.commit();
    }

    /* 
     * get selfreg sim index
     * return
     *  0xff    not display sim index in DM State Menu
    */
    public int getSelfRegSimIndex(){
            String lastImsi = getLastImsi();

            if (mPhoneCnt > 1 && isSelfRegOk() && null != lastImsi){
                for(int phoneId=0; phoneId < mPhoneCnt; phoneId++){
                    String simImsi = mTelephonyManager[phoneId].getSubscriberId();
                    if (null != simImsi && simImsi.equals(lastImsi)){
                        Log.d(TAG, "getSelfRegSimIndex display SIM"  + (phoneId+1));
                        return phoneId;
                    }
                }
            }
             Log.d(TAG, "getSelfRegSimIndex dont display sim index");
            return 0xff;
    }

    // judge if simcard change since last successful self registe
    private boolean isSimcardChange() {
        boolean result = false;
        String curImsi[] = new String[2];
	int phoneId = 0;
        for(phoneId=0; phoneId < mPhoneCnt; phoneId++){
            curImsi[phoneId] = mTelephonyManager[phoneId].getSubscriberId();
        }

        String lastImsi = getLastImsi();

        Log.d(TAG, "isSimcardChange: curImsi = " + curImsi[0] + " " + curImsi[1]);
        Log.d(TAG, "isSimcardChange: lastImsi = " + lastImsi);

        // NOTE: if string compare is ok , should use memcmp etc.
        if (curImsi[0] == null && curImsi[1] == null) {
            Log.d(TAG, "isSimcardChange: Error !!! curImsi is null! ");
            result = false; // if can't get imsi, no need to send selfreg
                            // message
        } else {
            if ((lastImsi.equals(curImsi[0])) /*|| (lastImsi.equals(curImsi[1]))*/) {
		   curPhoneId = 0;
		    Log.d(TAG, "isSimcardChange: selfregok: "+curPhoneId);
                result = false;
            } else 
              if ((lastImsi.equals(curImsi[1])) /*|| (lastImsi.equals(curImsi[1]))*/) {
		   curPhoneId = 1;
		    Log.d(TAG, "isSimcardChange: selfregok: "+curPhoneId);
                result = false;
            } else {
		
                result = false;
                
                if(mCurrentCMCCSimNum > 1){
                    Log.d(TAG," isSimcardChange --> there are 2 cmcc sim card");
                    
                    //no send sms 2 sim card change each other when both are rejected
                    if(mIsRejectPhone0 && mIsRejectPhone1){
                        if(mRejectImsi0.equals(curImsi[1]) && mRejectImsi1.equals(curImsi[0])){
                            Log.d(TAG," isSimcardChange --> No need regester beacese double Sim are rejected and they chang to each other positon");
                            
                            SharedPreferences sharedPreferences = mContext.getSharedPreferences(CMCC_CONFIG, MODE);
                            SharedPreferences.Editor editor = sharedPreferences.edit();
                            editor.putString(ITEM_CMCC_REJECT_IMSI0, curImsi[0]);
                            editor.putString(ITEM_CMCC_REJECT_IMSI1, curImsi[1]);
                            editor.commit(); 
                            
                            return false;
                        }
                    }
                                        
                    if (mIsRejectPhone0 && mRejectImsi0.equals(curImsi[0])) {
                        Log.d(TAG," isSimcardChange phone0 has been reject ,go to check phone 1");
                        if (mIsRejectPhone1 && mRejectImsi1.equals(curImsi[1])){
                            Log.d(TAG," isSimcardChange --> Double sim card have been reject");                            
                        }else{
                            Log.d(TAG," isSimcardChange --> phone 1 will go to sms register");
                            result = true;
                            curPhoneId = 1;
                        }
                    }else{
                        Log.d(TAG," isSimcardChange --> phone 0 will go to sms register");
                        result = true;
                        curPhoneId = 0;
                    }                    
                }else{
                    Log.d(TAG," isSimcardChange --> there is 1 cmcc sim card");
                    for (phoneId = 0; phoneId < mPhoneCnt; phoneId++) {
                        if (getIsCmccCard(phoneId)) {
                            result = true;
                            curPhoneId = phoneId;
                            Log.d(TAG,"isSimcardChange: Changed and select phonid = " + phoneId);
                            break;
                        }
                    }
                }                                                
            }
        }
        Log.d(TAG, "isSimcardChange: result = " + result );

        stopListeningServiceState();
        
        return result;
    }

    // judge if is need self registe
    protected boolean isNeedSelfReg() {
        boolean result = false;

        if (isSimcardChange()) {
            result = true;
        }
        Log.d(TAG, "isNeedSelfReg: " + result);
        return result;
    }

    // judge is have send self registe message
    protected boolean isHaveSendSelfRegMsg() {
        SharedPreferences sharedPreferences = getSharedPreferences(PREFERENCE_NAME, MODE);
        mIsHaveSendMsg = sharedPreferences.getBoolean("IsHaveSendSelfRegMsg", false);
        Log.d(TAG, "isHaveSendSelfRegMsg: return " + mIsHaveSendMsg);
        return mIsHaveSendMsg;
    }

    private void setIsHaveSendSelfRegMsg(Context context, boolean isHaveSend) {
        Log.d(TAG, "setIsHaveSendSelfRegMsg: to " + isHaveSend);
        mIsHaveSendMsg = isHaveSend;
        SharedPreferences sharedPreferences = context.getSharedPreferences(PREFERENCE_NAME, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putBoolean("IsHaveSendSelfRegMsg", isHaveSend);
        editor.commit();
    }

    // get self registe switch
    protected boolean getSelfRegSwitch() {
        Log.d(TAG, "getSelfRegSwitch: " + mSelfRegSwitch);
        return mSelfRegSwitch;
    }

    // set self registe switch
    protected void setSelfRegSwitch(Context context, boolean isOpen) {
        Log.d(TAG, "setSelfRegSwitch: " + isOpen);
        mSelfRegSwitch = isOpen;

        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putBoolean(ITEM_SELFREG_SWITCH, isOpen);
        editor.commit();
    }

    public boolean isDebugMode() {
        Log.d(TAG, "isDebugMode: " + mIsDebugMode);
        return mIsDebugMode;
    }

    protected void setDebugMode(Context context, boolean isDebugMode) {
        Log.d(TAG, "setDebugMode: " + isDebugMode);
        mIsDebugMode = isDebugMode;

        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putBoolean(ITEM_DEBUG_MODE, isDebugMode);
        editor.commit();
    }

    protected boolean isRealNetParam() {
        Log.d(TAG, "isRealParam: " + mIsRealNetParam);
        return mIsRealNetParam;
    }

    protected void setRealNetParam(Context context, boolean isRealParam, boolean isSetApn) {
        Log.d(TAG, "setRealNetParam: " + isRealParam);
        mIsRealNetParam = isRealParam;

        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putBoolean(ITEM_REAL_PARAM, isRealParam);
        editor.commit();

        if (mIsRealNetParam) {
            setServerAddr(context, REAL_SERVER_ADDR);
            setSmsAddr(context, REAL_SMS_ADDR);
            setSmsPort(context, REAL_SMS_PORT);
            if (isSetApn) {
                setAPN(context, REAL_APN);
                DmService.getInstance().setProxy(mContext, null);
                DmService.getInstance().setProxyPort(mContext, null);
            }
        } else {
            setServerAddr(context, LAB_SERVER_ADDR);
            setSmsAddr(context, LAB_SMS_ADDR);
            setSmsPort(context, LAB_SMS_PORT);
            if (isSetApn) {
                setAPN(context, LAB_APN);
                DmService.getInstance().setProxy(mContext, "10.0.0.172");
                DmService.getInstance().setProxyPort(mContext, "80");
            }
        }
    }

    public String getServerAddr() {
        Log.d(TAG, "getServerAddr: " + mServerAddr);
        return mServerAddr;
    }

    protected void setServerAddr(Context context, String serverAddr) {
        Log.d(TAG, "setServerAddr: " + serverAddr);
        mServerAddr = serverAddr;

        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString(ITEM_SERVER_ADDR, serverAddr);
        editor.commit();
    }

    protected String getSmsAddr() {
        Log.d(TAG, "getSmsAddr: " + mSmsAddr);
        return mSmsAddr;
    }

    protected void setSmsAddr(Context context, String smsAddr) {
        Log.d(TAG, "setSmsAddr: " + smsAddr);
        mSmsAddr = smsAddr;

        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString(ITEM_SMS_ADDR, smsAddr);
        editor.commit();
    }

    protected String getSmsPort() {
        Log.d(TAG, "getSmsPort: " + mSmsPort);
        return mSmsPort;
    }

    protected void setSmsPort(Context context, String smsPort) {
        Log.d(TAG, "setSmsPort: " + smsPort);
        mSmsPort = smsPort;

        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString(ITEM_SMS_PORT, smsPort);
        editor.commit();
    }

    private String getInitApn(Context context) {    	
        String str = null;
         String numeric = android.os.SystemProperties.get(
                 TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, curPhoneId), "");
         
        final String selection = "name = 'CMCC DM' and numeric=\""
                + numeric + "\"";
        Log.d(TAG, "getInitApn: selection = " + selection);
        Cursor cursor = context.getContentResolver().query((curPhoneId == 0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(curPhoneId,null), null,
                selection, null, null);        
        if (cursor != null) {
            if (cursor.getCount() > 0 && cursor.moveToFirst()) {
                str = cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.APN));
            }
            cursor.close();
        }

        Log.d(TAG, "getInitApn: " + str);
        return str;
    }

    public String getSavedAPN() {
        Log.d(TAG, "getSavedAPN: " + mApnTemp);
        return mApnTemp;
    }

    public String getAPN() {
        Log.d(TAG, "getAPN: " + mApn);
	mApn = getInitApn(mContext);
        return mApn;
    }

    public int getCurrentPhoneID(){
        Log.d(TAG, "getCurrentPhoneID: " + curPhoneId);
        return curPhoneId;
   	}

    public void setCurrentPhoneID(int pid){
        Log.d(TAG, "getCurrentPhoneID: " + pid);
 	 curPhoneId = pid;
        return ;
   	}
    public void setOnlyAPN(Context context, String apn)
    {
	     Log.d(TAG, "setOnlyAPN: " + apn);
            mApn = apn;
            mApnTemp = null;			
    }
    public void setOnlyProxy(Context context, String proxy)
    {
	     Log.d(TAG, "setOnlyProxy: " + proxy);
            mProxyTemp = null;
    }
    public void setOnlyProxyPort(Context context, String port)
    {
	     Log.d(TAG, "setOnlyProxyPort: " + port);
            mProxyPortTemp= null;
    }
    public void setAPN(Context context, String apn) {
        Log.d(TAG, "setAPN: " + apn);

        // SharedPreferences sharedPreferences =
        // context.getSharedPreferences(DM_CONFIG, MODE);
        // SharedPreferences.Editor editor = sharedPreferences.edit();
        // editor.putString(ITEM_APN, apn);
        // editor.commit();
     //   if (!Vdmc.getInstance().isVDMRunning()) {
            mApn = apn;
            final String selection = "name = 'CMCC DM' and numeric=\""
                    + android.os.SystemProperties.get(
                            TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, curPhoneId), "") + "\"";
            Log.d(TAG, "setAPN: selection = " + selection);

            ContentValues values = new ContentValues();

            values.put(Telephony.Carriers.APN, apn);
            int c = values.size() > 0 ? context.getContentResolver().update(
                    (curPhoneId == 0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(curPhoneId,null), values, selection, null) : 0;
            Log.d(TAG, "setAPN: values.size() = " + values.size());
            Log.d(TAG, "setAPN: update count = " + c);

            mApnTemp = null;
   /*     } else {
            mApnTemp = apn;
            Log.d(TAG, "setAPN: dm session is running, save value temporarily!");
        }
  */        
    }

    public String getSavedProxy() {
        Log.d(TAG, "getSavedProxy: " + mProxyTemp);
        return mProxyTemp;
    }

    public String getProxy(Context context) {
        // SharedPreferences sharedPreferences =
        // context.getSharedPreferences(DM_CONFIG, MODE);
        // SharedPreferences.Editor editor = sharedPreferences.edit();
        // String str = sharedPreferences.getString(ITEM_PROXY, "10.0.0.172");

        String str = null;
        final String selection = "name = 'CMCC DM' and numeric=\""
               + android.os.SystemProperties.get(
                       TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, curPhoneId), "") + "\"";

        
        Cursor cursor = 	context.getContentResolver().query(
        (curPhoneId==0)? Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(curPhoneId,null),
        null, selection, null, null);
        
        if (cursor != null) {
            if (cursor.getCount() > 0 && cursor.moveToFirst()) {
                str = cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.PROXY));
            }
            cursor.close();
        }

        Log.d(TAG, "getProxy: " + str);
        return str;
    }

    public void setProxy(Context context, String proxy) {
        Log.d(TAG, "setProxy: " + proxy);

        // SharedPreferences sharedPreferences =
        // context.getSharedPreferences(DM_CONFIG, MODE);
        // SharedPreferences.Editor editor = sharedPreferences.edit();
        // editor.putString(ITEM_PROXY, proxy);
        // editor.commit();

  //      if (!Vdmc.getInstance().isVDMRunning()) {
            final String selection = "name = 'CMCC DM' and numeric=\""
                    + android.os.SystemProperties.get(
                            TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, curPhoneId), "") + "\"";
            ContentValues values = new ContentValues();

            values.put(Telephony.Carriers.PROXY, proxy);
            int c = values.size() > 0 ? context.getContentResolver().update(
                    (curPhoneId== 0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(curPhoneId,null), 
                    values, selection, null) : 0;
            Log.d(TAG, "setProxy: values.size() = " + values.size());
            Log.d(TAG, "setProxy: update count = " + c);

            mProxyTemp = null;
/*        } else {
            mProxyTemp = proxy;
            Log.d(TAG, "setProxy: dm session is running, save value temporarily!");
        }
*/        
    }

    public String getSavedProxyPort() {
        Log.d(TAG, "getSavedProxyPort: " + mProxyPortTemp);
        return mProxyPortTemp;
    }

    public String getProxyPort(Context context) {
        // SharedPreferences sharedPreferences =
        // context.getSharedPreferences(DM_CONFIG, MODE);
        // SharedPreferences.Editor editor = sharedPreferences.edit();
        // String str = sharedPreferences.getString(ITEM_PROXY_PORT, "80");

        String str = null;
        // final String selection = "name = 'CMCC DM' and numeric=\""
        final String selection = "name = 'CMCC DM' and numeric=\""
                + android.os.SystemProperties.get(
                        TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, curPhoneId), "") + "\"";

        Cursor cursor = context.getContentResolver().query(
        (curPhoneId== 0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(curPhoneId,null),
        null,      selection, null, null);

        if (cursor != null) {
            if (cursor.getCount() > 0 && cursor.moveToFirst()) {
                str = cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.PORT));
            }
            cursor.close();
        }

        Log.d(TAG, "getProxyPort: " + str);
        return str;
    }

    public void setProxyPort(Context context, String port) {
        Log.d(TAG, "setProxyPort: " + port);

        // SharedPreferences sharedPreferences =
        // context.getSharedPreferences(DM_CONFIG, MODE);
        // SharedPreferences.Editor editor = sharedPreferences.edit();
        // editor.putString(ITEM_PROXY_PORT, port);
        // editor.commit();

 //       if (!Vdmc.getInstance().isVDMRunning()) {
            final String selection = "name = 'CMCC DM' and numeric=\""
                    + android.os.SystemProperties.get(
                            TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, curPhoneId), "") + "\"";
            ContentValues values = new ContentValues();

            values.put(Telephony.Carriers.PORT, port);
            int c = values.size() > 0 ? context.getContentResolver().update(
                    (curPhoneId== 0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(curPhoneId,null), 
                    values, selection, null) : 0;
            Log.d(TAG, "setProxyPort: values.size() = " + values.size());
            Log.d(TAG, "setProxyPort: update count = " + c);

            mProxyPortTemp = null;
/*        } else {
            mProxyPortTemp = port;
            Log.d(TAG, "setProxyPort: dm session is running, save value temporarily!");
        }
*/        
    }

    // add by lihui
    public void getServerNonce(Context context, byte[] data) {
        ByteBuffer buf = null;
        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        String str = sharedPreferences.getString(ITEM_SERVER_NONCE, "ffff");
        Log.d(TAG, "getServerNonce:= " + str);
        if (data == null) {
            Log.d(TAG, "read: data is null!");
            return;
        }
        buf = ByteBuffer.wrap(data);
        Log.d(TAG, "read: buf = " + buf);
        buf.put(str.getBytes());

    }

    public void setServerNonce(Context context, byte[] data) {
        String ServerNonce = new String(data);
        Log.d(TAG, "setServerNonce:=" + ServerNonce);

        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString(ITEM_SERVER_NONCE, ServerNonce);
        editor.commit();
    }

    public void getClientNonce(Context context, byte[] data) {
        ByteBuffer buf = null;
        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        String str = sharedPreferences.getString(ITEM_CLIENT_NONCE, "ffff");
        Log.d(TAG, "getClientNonce= " + str);

        if (data == null) {
            Log.d(TAG, "read: data is null!");
            return;
        }
        buf = ByteBuffer.wrap(data);
        Log.d(TAG, "read: buf = " + buf);
        buf.put(str.getBytes());

    }

    public void setClientNonce(Context context, byte[] data) {
        String ClientNonce = new String(data);
        Log.d(TAG, "setClientNonce= " + ClientNonce);

        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString(ITEM_CLIENT_NONCE, ClientNonce);
        editor.commit();
    }

    public String getManufactory() {
        Log.d(TAG, "getManufactory: " + mManufactory);
        return mManufactory;
    }

    protected void setManufactory(Context context, String manufactory) {
        Log.d(TAG, "setManufactory: " + manufactory);
        mManufactory = manufactory;

        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString(ITEM_MANUFACTORY, manufactory);
        editor.commit();
    }

    public String getModel() {
        Log.d(TAG, "getModel: " + mModel);
        return mModel;
    }

    protected void setModel(Context context, String model) {
        Log.d(TAG, "setModel: " + model);
        mModel = model;

        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString(ITEM_MODEL, model);
        editor.commit();
    }

    public String getImei() {
        Log.d(TAG, "getImei: " + mImeiStr);
	if (null == mImeiStr)
		{
            mImeiStr = mTelephonyManager[0].getDeviceId();
        	Log.d(TAG, "getImei: " + mImeiStr);
		}
        //todo
        //need to fix
       //return "861683010001602";
	     return mImeiStr;
    }

    protected void setImei(Context context, String imei) {
        Log.d(TAG, "setImei: " + imei);
        mImeiStr = imei;

        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString(ITEM_IMEI, imei);
        editor.commit();
    }

    public String getSoftwareVersion() {
        Log.d(TAG, "getSoftwareVersion: " + mSoftVer);
        return mSoftVer;
    }

    protected void setSoftwareVersion(Context context, String softVer) {
        Log.d(TAG, "setSoftwareVersion: " + softVer);
        mSoftVer = softVer;

        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString(ITEM_SOFTVER, softVer);
        editor.commit();
    }

    // send message body
    private void sendMsgBody() {
        String destAddr = getSmsAddr();
        short destPort = (short) Integer.parseInt(getSmsPort());
        short srcPort = destPort;
        SmsManager smsManager = SmsManager.getDefault(curPhoneId);
        Log.d(TAG, "sendMsgBody: curPhoneId = " + curPhoneId);
        byte[] data; // sms body byte stream
        String smsBody; // sms body string format
        String imei = getImei();
        String softVer = getSoftwareVersion();
        String manStr = getManufactory();
        String modStr = getModel();
        //michael
        //imei = "861683010001602";
        Log.d(TAG, "sendMsgBody: Enter!");

        // smsbody: IMEI:860206000003972/Hisense/TS7032/TI7.2.01.22.00
        smsBody = "IMEI:" + imei + "/" + manStr + "/" + modStr + "/" + softVer;
        Log.d(TAG, "sendMsgBody: " + smsBody);
        data = smsBody.getBytes();
        for (int i = 0; i < smsBody.length(); i++) {
            Log.d(TAG, "sendMsgBody: ============= data[" + i + "] = " + data[i] + "\n");
        }

        Log.d(TAG, "sendMsgBody: dest addr = " + destAddr);
        Log.d(TAG, "sendMsgBody: dest port = " + destPort);

        //add for 105942 begin
        Log.d(TAG, "sendMsgBody: IS_CONFIG_SELFREG_REPLY = " + IS_CONFIG_SELFREG_REPLY);
        if (false == IS_CONFIG_SELFREG_REPLY){
            Intent sentIntent = new Intent(SENT_SMS_ACTION);  
	    sentIntent.putExtra("phone_id", curPhoneId);
            PendingIntent sentPI = PendingIntent.getBroadcast(this, 0, sentIntent,  0);  
              
            Intent deliverIntent = new Intent(DELIVERED_SMS_ACTION);  
	    deliverIntent.putExtra("phone_id", curPhoneId);
            PendingIntent deliverPI = PendingIntent.getBroadcast(this, 0,  deliverIntent, 0);

            IntentFilter mFilter = new IntentFilter(SENT_SMS_ACTION);
            mFilter.addAction(DELIVERED_SMS_ACTION);
            registerReceiver(sendMessageReceiver, mFilter);

            smsManager.sendDmDataMessage(destAddr, null, destPort, srcPort, data, sentPI, deliverPI );
            return;
         }
        //add for 105942 end
       

        smsManager.sendDmDataMessage(destAddr, null, /* use the default SMSC */
        destPort, srcPort, data, null, /* do not need listen to send result */
        null /* do not require delivery report */);
    }

    
   //add for 105942 begin
    private BroadcastReceiver sendMessageReceiver = new BroadcastReceiver() {  
            @Override  
            public void onReceive(Context context, Intent intent) {
                        String actionName = intent.getAction();
                        int resultCode = getResultCode();
			int phoneid = intent.getIntExtra("phone_id", 0);
			Log.d(TAG, "sendMessageReceiver onReceive phoneid=" + phoneid);
			Log.d(TAG, "sendMessageReceiver onReceive actionName=" + actionName + " resultCode="+resultCode);
                        if (actionName.equals(SENT_SMS_ACTION) 
                            || actionName.equals(DELIVERED_SMS_ACTION) ){
                                switch (resultCode) {
                                case Activity.RESULT_OK:
					curPhoneId = phoneid;
                                        TelephonyManager mTelephonyManager = (TelephonyManager) mContext.getSystemService(
                                                TelephonyManager.getServiceName(mContext.TELEPHONY_SERVICE, curPhoneId));
                                        
                                        String imsi = mTelephonyManager.getSubscriberId();

                                        saveImsi(mContext, imsi);
                                        setSelfRegState(mContext, true);
                                        stopListeningServiceState();
                                        break;
                                default:
                                         Log.d(TAG, "sendMessageReceiver Send Error ");
                                        break;
                                }
                        } 
                }
   };
    //add for 105942 end
    

    // Send self registe message
    private void sendSelfRegMsg() {
        Log.d(TAG, "enter sendSelfRegMsg()");
         initConnectParam();
        if (!getSelfRegSwitch()) {
            Log.d(TAG, "sendSelfRegMsg: self registe switch is closed, no need send self registe message!");
            stopListeningServiceState();
            return;
        }

        if (isHaveSendSelfRegMsg()) {
            Log.d(TAG, "sendSelfRegMsg: have send self registe message!");
            stopListeningServiceState();
            return;
        }

        Log.d(TAG, "sendSelfRegMsg: Enter!");
        stopListeningServiceState();
        if (isNeedSelfReg()) {
	    synchronized( keepcurphone)
	    	{
            	sendMsgBody();
	        setIsHaveSendSelfRegMsg(mContext, true);
	    	}
        } else {
            setSelfRegState(mContext, true);
        }
    }

    // Send self registe message for debug mode
    protected void sendSelfRegMsgForDebug() {
        // send message directly under debug mode
        Log.d(TAG, "sendSelfRegMsgForDebug: Enter!");
        sendMsgBody();
        //showDialog4SendDMSms();
        //setIsHaveSendSelfRegMsg(mContext, true);
    }

    
    public void showDialog4SendDMSms() {                
        Log.d(TAG, "showDialog4SendDMSms");
        if(isDialogShowed){
            Log.d(TAG, "showDialog4SendDMSms The dialog is showed,so return");
            return;
        }
        if(mCurrentCMCCSimNum == 0){
            Log.d(TAG, "showDialog4SendDMSms mCurrentCMCCSimNum is 0,can not send sms for DM Register");
            return;
        }else if(mCurrentCMCCSimNum == 1){//register success will not come here 
            Log.d(TAG, "showDialog4SendDMSms mCurrentCMCCSimNum is 1");
            if(isNeedSelfReg()){
                Log.d(TAG, "showDialog4SendDMSms mCurrentCMCCSimNum == 1 curPhoneId = " + curPhoneId);
                //no send sms when the sim card has been rejected even change the sim position
                if((mIsRejectPhone0 && mRejectImsi0.equals(mTelephonyManager[curPhoneId].getSubscriberId())) ||
                   (mIsRejectPhone1 && mRejectImsi1.equals(mTelephonyManager[curPhoneId].getSubscriberId()))){
                    Log.d(TAG, "Do not show dialog --> user has reject register when one sim card and no sim changed");
                    return;
                }else{
                    Log.d(TAG, " Show dialog when insert one cmcc card ");
                    SharedPreferences sharedPreferences = mContext.getSharedPreferences(CMCC_CONFIG, MODE);
                    SharedPreferences.Editor editor = sharedPreferences.edit();  
                    if(0 == curPhoneId){
                        editor.putBoolean(ITEM_CMCC_REJECT_STATUS_PHONE0, false);
                        editor.putString(ITEM_CMCC_REJECT_IMSI0, "default");  
                    }else{
                        editor.putBoolean(ITEM_CMCC_REJECT_STATUS_PHONE1, false);
                        editor.putString(ITEM_CMCC_REJECT_IMSI1, "default");  
                    }                                                                          
                    editor.commit(); 
                }
                
                startSmsSelfDialog(30);                
            }
        }else {//there are 2 sim insert
            Log.d(TAG, "showDialog4SendDMSms mCurrentCMCCSimNum is 2 curPhoneId = " + curPhoneId);
            if(isNeedSelfReg()){
                //if( 0 == curPhoneId){ 
                    if(mIsRejectPhone0){
                        if(mRejectImsi0.equals(mTelephonyManager[0].getSubscriberId())){
                            Log.d(TAG, "Do not show dialog --> phone 0 is rejected and the sim card is not change,goto check phone1");
                            if(mIsRejectPhone1){
                                if(mRejectImsi1.equals(mTelephonyManager[1].getSubscriberId())){
                                    Log.d(TAG, "Do not show dialog --> phone 1 is rejected and the sim card is not change,double sim can not send sms");
                                    return;
                                }else{//update the reject(rejected before) state when insert a new another sim card
                                    Log.d(TAG, " Show dialog and chang the Reject to false --> phone 1 sim card is change");
                                    //curPhoneId = 1;
                                    SharedPreferences sharedPreferences = mContext.getSharedPreferences(CMCC_CONFIG, MODE);
                                    SharedPreferences.Editor editor = sharedPreferences.edit();   
                                    editor.putBoolean(ITEM_CMCC_REJECT_STATUS_PHONE1, false);
                                    editor.putString(ITEM_CMCC_REJECT_IMSI1, "default");                                                        
                                    editor.commit(); 
                                }
                            }                                                  
                        }else{//update the reject(rejected before) state when insert a new another sim card
                            Log.d(TAG, " Show dialog and chang the Reject to false --> phone 0 sim card is change");
                            //curPhoneId = 0;
                            SharedPreferences sharedPreferences = mContext.getSharedPreferences(CMCC_CONFIG, MODE);
                            SharedPreferences.Editor editor = sharedPreferences.edit();   
                            editor.putBoolean(ITEM_CMCC_REJECT_STATUS_PHONE0, false);
                            editor.putString(ITEM_CMCC_REJECT_IMSI0, "default");                                                        
                            editor.commit(); 
                        }
                    }

                    startSmsSelfDialog(30);
            }                                                                       
        }                
    }
    
    private void startSmsSelfDialog(int timeout){
        Intent intent = new Intent(mContext, DmAlertDialog.class);
        int intentFlags = Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP;

        intent.setFlags(intentFlags);
        intent.putExtra("dialogId", SEND_SELF_SMS);
        intent.putExtra("message", "Allow DM Send Message?");
        intent.putExtra("timeout", timeout);
        
        mContext.startActivity(intent);
        isDialogShowed = true;
        resetDataConnectConfig();
    }
    
    public void NotifySendSelfSMS(){
        Log.d(TAG, "NotifySendSelfSMS ");
        sendSelfRegMsg();
    }
    
    //Register failed,no popup dialog when reboot phone
    private void saveCMCCDMRegisteState(String imsi,boolean status) {   
        Log.d(TAG, "saveCMCCDMRegisteFailState  imsi = " + imsi + " status = " + status);
        SharedPreferences sharedPreferences = mContext.getSharedPreferences(CMCC_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString(ITEM_CMCC_LAST_IMSI, imsi);
        editor.putBoolean(ITEM_CMCC_REGISTED_STATUS, status);        
        editor.commit();
    }

    
    public void saveStatus4CancelAllowRegiste(){
        Log.d(TAG, "saveStatus4CancelAllowRegiste" );
        //stopListeningServiceState();
        TelephonyManager mTelephonyManager = (TelephonyManager) mContext.getSystemService(
                TelephonyManager.getServiceName(mContext.TELEPHONY_SERVICE, curPhoneId));
        
        String imsi = mTelephonyManager.getSubscriberId();
        
        Log.d(TAG, "saveStatus4CancelAllowRegiste imsi = " + imsi + " curPhoneId = " + curPhoneId);
        
        SharedPreferences sharedPreferences = mContext.getSharedPreferences(CMCC_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();

        if( 0 == curPhoneId){
            editor.putBoolean(ITEM_CMCC_REJECT_STATUS_PHONE0, true);
            editor.putString(ITEM_CMCC_REJECT_IMSI0, imsi);
            mIsRejectPhone0 = true;
            mRejectImsi0 = imsi;
        }else{
            editor.putBoolean(ITEM_CMCC_REJECT_STATUS_PHONE1, true);
            editor.putString(ITEM_CMCC_REJECT_IMSI1, imsi);
            mIsRejectPhone1 = true;
            mRejectImsi1 = imsi;
        }
        
        editor.commit(); 
    }
    
    public void saveStatus4AllowRegiste(){
        Log.d(TAG, "saveStatus4AllowRegiste curPhoneId = " + curPhoneId);
        //stopListeningServiceState();
        TelephonyManager mTelephonyManager = (TelephonyManager) mContext.getSystemService(
                TelephonyManager.getServiceName(mContext.TELEPHONY_SERVICE, curPhoneId));
        
        //String imsi = mTelephonyManager.getSubscriberId();//Bug 290164:Fix the converity bug
        
        SharedPreferences sharedPreferences = mContext.getSharedPreferences(CMCC_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();   

        if( 0 == curPhoneId){
            editor.putBoolean(ITEM_CMCC_REJECT_STATUS_PHONE0, false);
            editor.putString(ITEM_CMCC_REJECT_IMSI0, "default");
        }else{
            editor.putBoolean(ITEM_CMCC_REJECT_STATUS_PHONE1, false);
            editor.putString(ITEM_CMCC_REJECT_IMSI1, "default");
        }
        editor.commit(); 
    }

    public void showDialog4DataConnect(Intent niaIntent) {
        Log.d(TAG, "showDialog4DataConnect");
        Intent intent = new Intent(mContext, DmAlertDialog.class);
        int intentFlags = Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP;

        intent.setFlags(intentFlags);
        intent.putExtra("dialogId", DATA_CONNECT_CONFRIM);
        intent.putExtra("message", "Allow DM Send Message?");
        intent.putExtra("timeout", 300);
        
//        byte[] data = niaIntent.getByteArrayExtra("msg_body");
//        for (int i = 0; i < data.length; i++) {            
//            Log.d(TAG, "data[" + i + "] = " + Integer.toHexString(data[i]&0xff));
//        }
        
        Bundle extras = new Bundle();
        extras.putByteArray("msg_body", niaIntent.getByteArrayExtra("msg_body"));
        extras.putString("msg_org", niaIntent.getStringExtra("msg_org"));
        intent.putExtras(extras);
        
        mContext.startActivity(intent);
    }    
    
    public void saveStatus4DataConnect(boolean isAllowDataConnect,boolean isDataConnectAlwaysStatus){
        Log.d(TAG, "saveStatus4DataConnect isAllowDataConnect = " + isAllowDataConnect + " isDataConnectAlwaysStatus = " + isDataConnectAlwaysStatus);
        
        mAllowedDataConnect = isAllowDataConnect;
        mDataConnectAlwaysStatus = isDataConnectAlwaysStatus;
        
        SharedPreferences sharedPreferences = mContext.getSharedPreferences(CMCC_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();   
        
        editor.putBoolean(ITEM_CMCC_ALLOWED_DATACONNECT, isAllowDataConnect);
        editor.putBoolean(ITEM_CMCC_DATACONNECT_ALWAYS_STATUS, isDataConnectAlwaysStatus);
        editor.commit(); 
    }
            
    private void resetDataConnectConfig() {
        Log.d(TAG, "resetDataConnectConfig");
        SharedPreferences sharedPreferences = mContext.getSharedPreferences(CMCC_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();   
        
        editor.putBoolean(ITEM_CMCC_ALLOWED_DATACONNECT, true);
        editor.putBoolean(ITEM_CMCC_DATACONNECT_ALWAYS_STATUS, false);
        editor.commit();
    }
    
    public void cleanDailogControllFlag(){
        Log.d(TAG, "cleanDailogControllFlag ");
        SharedPreferences cmccConfig;                                
        cmccConfig = getSharedPreferences(CMCC_CONFIG,MODE);
        SharedPreferences.Editor editor = cmccConfig.edit();

        editor.putBoolean(ITEM_CMCC_ALLOWED_DATACONNECT, true);
        editor.putBoolean(ITEM_CMCC_DATACONNECT_ALWAYS_STATUS, false);
        editor.putBoolean(ITEM_CMCC_REJECT_STATUS_PHONE0, false);
        editor.putBoolean(ITEM_CMCC_REJECT_STATUS_PHONE1, false);
        editor.putString(ITEM_CMCC_REJECT_IMSI0, "default");
        editor.putString(ITEM_CMCC_REJECT_IMSI1, "default");
        editor.putString(ITEM_CMCC_IMSI_1, "default");
        editor.putString(ITEM_CMCC_IMSI_2, "default");        
                     
        editor.commit();                      
    }
    
    // bug 292626 begin
    public String getAGPSApn(Context context) {
        String str = null;
        boolean isExsit = false;
        final String selection = "name = 'CMSUPL' and numeric=\""
                + android.os.SystemProperties.get(TelephonyManager.getProperty(
                        TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC,
                        curPhoneId), "") + "\"";
        Log.d(TAG, "getAGPSApn: selection = " + selection);
        Cursor cursor = context.getContentResolver().query(
                (curPhoneId == 0) ? Telephony.Carriers.CONTENT_URI
                        : Telephony.Carriers.CONTENT_URI_SIM2, null, selection,
                null, null);

        if (null != cursor) {
            if (cursor.getCount() > 0 && cursor.moveToFirst()) {
                str = cursor.getString(cursor
                        .getColumnIndexOrThrow(Telephony.Carriers.APN));
                isExsit = true;
            }
            cursor.close();
        }

        if (!isExsit) {

            // taipinglai 如果手机中没有预置CMSUPL，则从SharedPreferences读取，确保AGPS参数采集可以执行成功            
            str = getAGPSParam(context,
                    MyTreeIoHandler.AGPS_CONNPROFILE_IO_HANDLER);
            Log.d(TAG, "not found CMSUPL obtained from SharedPreferences ");
        }

        Log.d(TAG, "getAGPSApn: " + str);
        return str;
    }
    
    public void setAGPSApn(Context context, String apn) {
        final String selection = "name = 'CMSUPL' and numeric=\""
                + android.os.SystemProperties.get(TelephonyManager.getProperty(
                        TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC,
                        curPhoneId), "") + "\"";
        Log.d(TAG, "setAGPSApn: selection = " + selection);

        ContentValues values = new ContentValues();

        values.put(Telephony.Carriers.APN, apn);
        int c = values.size() > 0 ? context.getContentResolver().update(
                (curPhoneId == 0) ? Telephony.Carriers.CONTENT_URI
                        : Telephony.Carriers.CONTENT_URI_SIM2, values,
                selection, null) : 0;

        if (0 == c) {

            // taipinglai
            // 如果手机中没有预置CMSUPL，则保存至SharedPreferences中，确保AGPS参数配置可以执行成功 Mar 24
            setAGPSParam(context, apn,
                    MyTreeIoHandler.AGPS_CONNPROFILE_IO_HANDLER);
            Log.d(TAG, "not found CMSUPL save to SharedPreferences ");
        }

        Log.d(TAG, "setAGPSApn: values.size() = " + values.size() + " c = " + c);
        return;
    }
    
    public String getAGPSParam(Context context, int type) {
        SharedPreferences sharedPreferences = context.getSharedPreferences(
                PREFERENCE_AGPS, MODE);
        String str = "";

        switch (type) {
        case MyTreeIoHandler.AGPS_SERVER_IO_HANDLER:
            str = sharedPreferences.getString(AGPS_SERVER, "221.176.0.55:7275");
            break;
        case MyTreeIoHandler.AGPS_SERVER_NAME_IO_HANDLER:
            str = sharedPreferences.getString(AGPS_NAME,
                    "China Mobile AGPS server");
            break;
        case MyTreeIoHandler.AGPS_IAPID_IO_HANDLER:
            str = sharedPreferences.getString(AGPS_IAPID, "ap0004");
            break;
        case MyTreeIoHandler.AGPS_PORT_IO_HANDLER:
            str = sharedPreferences.getString(AGPS_PORT, "IPv4address:port");
            break;
        case MyTreeIoHandler.AGPS_PROVIDER_ID_IO_HANDLER:
            str = sharedPreferences.getString(AGPS_PROVIDERIP, "221.176.0.55");
            break;
        case MyTreeIoHandler.AGPS_PREFCONREF_ID_IO_HANDLER:
            str = sharedPreferences.getString(AGPS_PREFCONREF, "CMCC WAP");
            break;
        case MyTreeIoHandler.AGPS_CONREF_IO_HANDLER:
            str = sharedPreferences.getString(AGPS_CONREF, "");
            break;
        case MyTreeIoHandler.AGPS_CONNPROFILE_IO_HANDLER:
            str = sharedPreferences.getString(AGPS_CONNPROFILE, "cmnet");
            break;
        default:
            Log.d(TAG, "getAGPSParam error type = " + type);
            break;
        }

        Log.d(TAG, "getAGPSParam : str = " + str + " type = " + type);
        return str;
    }
    
    public boolean setAGPSParam(Context context, String str, int type) {
        SharedPreferences sharedPreferences = context.getSharedPreferences(
                PREFERENCE_AGPS, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        boolean isErrorType = false;

        switch (type) {
        case MyTreeIoHandler.AGPS_SERVER_IO_HANDLER:
            editor.putString(AGPS_SERVER, str);
            break;
        case MyTreeIoHandler.AGPS_SERVER_NAME_IO_HANDLER:
            editor.putString(AGPS_NAME, str);
            break;
        case MyTreeIoHandler.AGPS_IAPID_IO_HANDLER:
            editor.putString(AGPS_IAPID, str);
            break;
        case MyTreeIoHandler.AGPS_PORT_IO_HANDLER:
            editor.putString(AGPS_PORT, str);
            break;
        case MyTreeIoHandler.AGPS_PROVIDER_ID_IO_HANDLER:
            editor.putString(AGPS_PROVIDERIP, str);
            break;
        case MyTreeIoHandler.AGPS_PREFCONREF_ID_IO_HANDLER:
            editor.putString(AGPS_PREFCONREF, str);
            break;
        case MyTreeIoHandler.AGPS_CONREF_IO_HANDLER:
            editor.putString(AGPS_CONREF, str);
            break;
        case MyTreeIoHandler.AGPS_CONNPROFILE_IO_HANDLER:
            editor.putString(AGPS_CONNPROFILE, str);
            break;
        default:
            isErrorType = true;
            Log.d(TAG, "setAGPSParam error type = " + type);
            break;
        }

        if (!isErrorType) {
            editor.commit();
        }
        Log.d(TAG, "setAGPSParam : str = " + str + " type = " + type);
        return (!isErrorType);
    }
    // bug 292626 end
    
}
