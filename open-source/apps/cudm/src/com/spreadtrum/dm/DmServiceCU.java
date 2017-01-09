package com.spreadtrum.dm;

import android.app.Service;
import android.app.Notification;
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
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.PhoneConstants;

import java.nio.ByteBuffer;
import android.os.Handler;
import android.os.Message;
import android.net.Uri;
import com.android.internal.telephony.TelephonyIntents;

import android.app.AlertDialog;
import android.content.DialogInterface;

import android.provider.Telephony;
import android.content.ContentValues;
import android.database.Cursor;
import com.android.internal.telephony.TelephonyProperties;
import com.android.internal.telephony.PhoneFactory;
import android.app.PendingIntent;
import android.app.Activity;

public class DmServiceCU extends Service {
    private String TAG = DmReceiverCU.DM_TAG + "DmService: ";

    //add for 105942 begin
    private static final boolean IS_CONFIG_SELFREG_REPLY = true;
    private static final String SENT_SMS_ACTION ="com.spreadtrum.dm.SENT_SMS";
    private static final String DELIVERED_SMS_ACTION ="com.spreadtrum.dm.DELIVERED_SMS";
    //add for 105942 end

    private static final String mLastImsiFile = "lastImsi.txt";

    private static TelephonyManager[] mTelephonyManager;

    private static boolean mIsHaveSendMsg[] ; // if have send self registe
                                                   // message

    private static boolean mIsSelfRegOkNew; // if self registe ok
    //private static boolean mIsSelfRegOk[]; // if self registe ok
    private static boolean mSelfRegSwitch = true; // true: open self registe
                                                  // function false:close self
                                                  // registe function

    private static final String PREFERENCE_NAME = "LastImsi";

    private static int MODE = MODE_PRIVATE;

    private static DmServiceCU mInstance = null;

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

    private static String mImeiStr[];

    private static boolean mIsDebugMode;

    private static boolean mIsRealNetParam; // if current is read net parameter

    private static final String DM_CONFIG = "DmConfig";

    private static final String ITEM_DEBUG_MODE = "DebugMode";

    private static final String ITEM_REAL_PARAM = "RealParam";

    private static final String ITEM_MANUFACTORY = "Manufactory";

    private static final String ITEM_MODEL = "Model";

    private static final String ITEM_SOFTVER = "SoftVer";

    private static final String ITEM_IMEI = "IMEI";

    private static final String ITEM_IMEI2 = "IMEI2";

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
    private static final String REAL_SERVER_ADDR = "http://dm.wo.com.cn:6001";

    private static final String REAL_SMS_ADDR = "10655459";
    //private static final String REAL_SMS_ADDR = "10655464";
    
    private static final String REAL_SMS_PORT = "26680";

    private static final String REAL_APN = APN_CMDM;

    // Lab net parameter
    private static final String LAB_SERVER_ADDR = "http://dm.wo.com.cn:6001";

    // private static final String LAB_SERVER_ADDR = "http://218.206.176.97";
    private static final String LAB_SMS_ADDR =  "10655464"; //"1065840409";

    private static final String LAB_SMS_PORT = "26680";  //"16998";

    private static final String LAB_APN = APN_CMWAP;

    //TODO: to be confirm

    private Handler mHandler;

    private Uri mUri;

    private Cursor mCursor;

    private boolean mSmsReady[];

    private boolean mInService[];

    private PhoneStateListener[] mPhoneStateListener;

    private int mPhoneCnt = 0;
    private int curPhoneId = 0;
    public int mStartid= 0;
    private boolean isRegSendMessageReceiver = false;
    
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            //TelephonyIntents.ACTION_IS_SIM_SMS_READY
            if (action.startsWith("android.intent.action.ACTION_IS_SIM_SMS_READY")) {//|| action.equals(TelephonyIntents.ACTION_IS_SIM2_SMS_READY)) {
                int phoneId = intent.getIntExtra("phoneId", 0);
                mSmsReady[phoneId] = intent.getBooleanExtra("isReady", false);
                boolean isReged = getRegState();
                boolean isCuPhone = matchOperatorPhone(phoneId);
                boolean isNeedReg = isNeedSelfReg();
                Log.d(TAG, "isReged = " + isReged + " isCuPhone = " + isCuPhone + " isNeedReg = " + isNeedReg);
                if(!isReged && isCuPhone && isNeedReg){
                	if (mSmsReady[phoneId]) {
                		if (TelephonyManager.SIM_STATE_READY == mTelephonyManager[phoneId].getSimState()
                                && mInService[phoneId]) {
                			 curPhoneId = phoneId;
                			 sendSelfRegMsg(phoneId);
                		}
                	}
                }
            }
        }
    };

 /*   private boolean getIsCmccCard(int phoneId){
        String cmccStr1 = "46000";
        String cmccStr2 = "46002";
        String cmccStr3 = "46007";
        String cuStr1 = "46001";
        String cuStr2 = "46006";
		
        String curOper = mTelephonyManager[phoneId].getNetworkOperator();

        Log.d(TAG, "getIsCmccCard  phoneId ="+phoneId+" curOper:"+curOper);
        if (curOper.equals(cuStr1) || curOper.equals(cuStr2) ){
            return true; 
        }else{
            return false; 
        }
    }*/
   
    private boolean matchOperatorPhone(int phoneId){
    	String CUOperatorCode1 = "46001";
    	String CUOperatorCode2 = "46006";
    	boolean ret = false;
    	String curOperatorId = mTelephonyManager[phoneId].getNetworkOperator();
        Log.d(TAG, "getIsCmccCard  phoneId ="+phoneId+" curOperatorId:"+curOperatorId);
        if (curOperatorId.equals(CUOperatorCode1) || curOperatorId.equals(CUOperatorCode1) ){
            ret = true;
        }
        return ret;
    }

    private PhoneStateListener getPhoneStateListener(final int phoneId) {
        PhoneStateListener phoneStateListener = new PhoneStateListener() {
            @Override
            public void onServiceStateChanged(ServiceState serviceState) {
                Log.d(TAG, "onServiceStateChanged: phoneId = " + phoneId + ",current state = " + serviceState.getState());


                // judge if network is ready
                if (ServiceState.STATE_IN_SERVICE == serviceState.getState()) {
                    Log.d(TAG, "onServiceStateChanged: STATE_IN_SERVICE");
                    mInService[phoneId] = true;
                    // sim card is ready
                    boolean isReged = getRegState();
                    boolean isCuPhone = matchOperatorPhone(phoneId);
                    boolean isNeedReg = isNeedSelfReg();
                    Log.d(TAG, "isReged = " + isReged + " isCuPhone = " + isCuPhone + " isNeedReg = " + isNeedReg);
                    if(!isReged && isCuPhone && isNeedReg){
                    	if (TelephonyManager.SIM_STATE_READY == mTelephonyManager[phoneId].getSimState()
                                    &&  mSmsReady[phoneId]) {
                    		curPhoneId = phoneId;
                    		 sendSelfRegMsg(phoneId);
                    	}
                    }
                }
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
    	mContext = getBaseContext();
        mPhoneCnt = TelephonyManager.getPhoneCount();
        mSmsReady = new boolean[mPhoneCnt];
        mInService = new boolean[mPhoneCnt];
        mIsSelfRegOkNew = false;
        //mIsSelfRegOk = new boolean[mPhoneCnt];
        mIsHaveSendMsg = new boolean[mPhoneCnt];
        mImeiStr = new String[mPhoneCnt];	
        mTelephonyManager = new TelephonyManager[mPhoneCnt];
        mPhoneStateListener = new PhoneStateListener[mPhoneCnt];
        setRegState(mContext,mIsSelfRegOkNew);
        for(int phoneId=0; phoneId<mPhoneCnt; phoneId++){
            mTelephonyManager[phoneId] = (TelephonyManager) getSystemService(TelephonyManager.
                    getServiceName(Context.TELEPHONY_SERVICE,phoneId));
            mPhoneStateListener[phoneId] = getPhoneStateListener(phoneId);
            mTelephonyManager[phoneId].listen(mPhoneStateListener[phoneId],
                    PhoneStateListener.LISTEN_SERVICE_STATE);
            setIsHaveSendSelfRegMsg(mContext,phoneId, false);
        }


        mInstance = this;
        /**** set foreground **************/
		Notification notify = new Notification();
		super.startForeground(0,notify);
		/**** set foreground **************/
        mHandler = new DMHandler();
        //TODO: to be confirm
//        mDmNativeInterface = new DmNativeInterface(mContext, mHandler);  
        initParam();
        // Listen for broadcast intents that indicate the SMS is ready
        int num;
        IntentFilter filter = new IntentFilter("android.intent.action.ACTION_IS_SIM_SMS_READY");//TelephonyIntents.ACTION_IS_SIM_SMS_READY
        Intent intent1 = registerReceiver(mReceiver, filter);
		Log.d(TAG, "onCreate: ACTION_IS_SIM_SMS_READY register ");
		for (num= 0; num < mPhoneCnt; num++)
		{
	         filter = new IntentFilter(TelephonyManager.getAction("android.intent.action.ACTION_IS_SIM_SMS_READY",num));//TelephonyIntents.ACTION_IS_SIM_SMS_READY
	         intent1 = registerReceiver(mReceiver, filter);
			Log.d(TAG, "onCreate: ACTION_IS_SIM_SMS_READY register "+num);
		}
    }
    @Override
    public void onDestroy() {
        // Stop listening for service state
        stopListeningServiceState();
        unregisterReceiver(mReceiver);
	if(true == isRegSendMessageReceiver){
        	unregisterReceiver(sendMessageReceiver);
	}
        Log.d(TAG, "onDestroy: DmServiceCU is killed!");
        mInstance = null;
        mContext = null;
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
            boolean isSimMatchCustomerPhone = true;
            if (!isNeedSelfReg())
            {
	            Log.d(TAG, "onStart: not need to register");
	            Intent stop = new Intent("com.android.dm.stop"); 
	            mContext.sendBroadcast(stop);
            }
            if (getSelfRegSwitch()) {
            	for (int i= 0; i< mPhoneCnt; i++)
            	{
            		if (mTelephonyManager[i].hasIccCard())
            		{
            			isSimMatchCustomerPhone = matchOperatorPhone(i);
            			if(isSimMatchCustomerPhone){
            				sendSelfRegMsg(i);
            				break;
            			}
            		}//if (mTelephonyManager[i].hasIccCard())
            	}//for (int i= 0; i< mPhoneCnt; i++)
            }// if (getSelfRegSwitch())
        } else if (intent.getAction().equals("com.android.dm.NIA")) {
            Log.d(TAG, "onStart: com.android.dm.NIA");
        }
    }

    public static DmServiceCU getInstance() {
        if (null == mInstance) {
            mInstance = new DmServiceCU();
        }
        return mInstance;
    }

    //TODO: to be confirm
//    public DmJniInterface getDmInterface() {
//        return mDmInterface;
//    }

    public static Context getContext() {
        // Log.d("DM ==> DmServiceCU: ", "getContext: mContext = " + mContext);
        return mContext;
    }

    // Stop listening for service state
    public void stopListeningServiceState() {
        for(int phoneId = 0; phoneId < mPhoneCnt; phoneId++){
            if (null != mTelephonyManager[phoneId]) {
            	mTelephonyManager[phoneId].listen(mPhoneStateListener[phoneId],0);
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

    // init dm relative parameter
    private void initParam() {
        SharedPreferences sharedPreferences;
        sharedPreferences = getSharedPreferences(DM_CONFIG, MODE);

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
        Log.d("SmsReg", "mIsDebugMode = " + mIsDebugMode);
        
        mManufactory = sharedPreferences.getString(ITEM_MANUFACTORY, "Spreadtrum");
    	mModel = sharedPreferences.getString(ITEM_MODEL, "SP7710");
        mSoftVer = SystemProperties.get("ro.build.display.id","JZ056K");
        
        if (mIsDebugMode) {
            // init manufacture
            //mManufactory = sharedPreferences.getString(ITEM_MANUFACTORY, "sprd");
        	//mModel = sharedPreferences.getString(ITEM_MODEL, "sp7710ga");
            //mManufactory = sharedPreferences.getString(ITEM_MANUFACTORY, "MTK");            
            //mModel = sharedPreferences.getString(ITEM_MODEL, "6573");
            //mSoftVer = sharedPreferences.getString(ITEM_SOFTVER, "EZXBASE_N_00.39.A4I");

            for (int i= 0; i<mPhoneCnt;i++)
            mImeiStr[i] = sharedPreferences.getString(ITEM_IMEI, mTelephonyManager[i].getDeviceId());
        } else {
            //mManufactory = sharedPreferences.getString(ITEM_MANUFACTORY, "MTK");
            //mModel = sharedPreferences.getString(ITEM_MODEL, "6573");
            //mSoftVer = SystemProperties.get("ro.hisense.software.version",
                    //"EZXBASE_N_00.39.A4I");
             for (int i= 0; i<mPhoneCnt;i++)
            mImeiStr[i] = mTelephonyManager[i].getDeviceId();
        }

        if (SystemProperties.get("ro.hisense.cmcc.test", "0").equals("1")) {
            setRealNetParam(mContext, false, false);
        } else {
            // init if use real net parameter
            mIsRealNetParam = sharedPreferences.getBoolean(ITEM_REAL_PARAM, true);
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
       // initConnectParam();
    }

    // init dm connect network param,include apn/proxy/port
    private void initConnectParam() {
        createDmApn();

        // according to cmcc test flag to decide current server relative
        // parameters
        SharedPreferences sharedPreferences;
        sharedPreferences = getSharedPreferences(DM_CONFIG, MODE);
        if (SystemProperties.get("ro.hisense.cmcc.test", "0").equals("1")) {
            setRealNetParam(mContext, false, true);
        } else {
            // init if use real net parameter
            mIsRealNetParam = sharedPreferences.getBoolean(ITEM_REAL_PARAM, true);
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
    private String getLastImsi() {
        SharedPreferences sharedPreferences = getSharedPreferences(PREFERENCE_NAME, MODE);
        String imsi =  sharedPreferences.getString("IMSI", "");;
	    Log.d(TAG, "getLastImsi : imsi = " + imsi);
	    return imsi;
    }
    
    protected boolean saveImsi(Context context,String imsi){
    	Log.d(TAG, "Save Imsi = " + imsi);
    	SharedPreferences sharedPreferences = context.getSharedPreferences(PREFERENCE_NAME, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
	    editor.putString("IMSI", imsi);
	    editor.commit();
	    return true;   	
    }

    // get current self registe state
/*    protected boolean isSelfRegOk(int phoneId) {
        SharedPreferences sharedPreferences = getSharedPreferences(PREFERENCE_NAME, MODE);	
	    if (phoneId == 0)
	        mIsSelfRegOk[phoneId] = sharedPreferences.getBoolean("IsSelfRegOk", false);
		else
		if (phoneId == 1)
	        mIsSelfRegOk[phoneId] = sharedPreferences.getBoolean("IsSecondSelfRegOk", false);
		else
		return true;
	
        Log.d(TAG, "isSelfRegOk:"+phoneId+" return " + mIsSelfRegOk[phoneId]);
        return mIsSelfRegOk[phoneId];
    }
*/
    protected void setRegState(Context context, boolean isSuccess){
    	mIsSelfRegOkNew= isSuccess;
        SharedPreferences sharedPreferences = context.getSharedPreferences(PREFERENCE_NAME, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();		
	    editor.putBoolean("IsSelfRegOk", isSuccess);		
        editor.commit();
    }
    protected boolean getRegState(){
        SharedPreferences sharedPreferences = getSharedPreferences(PREFERENCE_NAME, MODE);
        mIsSelfRegOkNew= sharedPreferences.getBoolean("IsSelfRegOk", false);
        return mIsSelfRegOkNew;
    }
    
 
    public boolean isNeedSelfReg()
    {
    	boolean ret = true;
    	String savedImsi = getLastImsi();
    	if(savedImsi == null){
    		Log.d(TAG, "There is no register at all, need to register");
    		return ret;
    	}
    	boolean isSimMatchCustomerPhone = false;
    	for (int i= 0; i< mPhoneCnt; i++)
    	{
    		if (mTelephonyManager[i].hasIccCard())
    		{
    			isSimMatchCustomerPhone = matchOperatorPhone(i);
    			if(isSimMatchCustomerPhone){
    				if(savedImsi.equals(getImsi(i))){
    					Log.d(TAG, "The saved imsi is equal to phone + "+i+" imsi , the savedImsi = " + savedImsi);
					if(mContext != null){
    						setRegState(mContext, true);
					}
    					return false;
    				}
    			}
    		}
    	}
		return ret;
    }
	
    // judge is have send self registe message
    protected boolean isHaveSendSelfRegMsg(int cnt) {
        SharedPreferences sharedPreferences = getSharedPreferences(PREFERENCE_NAME, MODE);

        mIsHaveSendMsg[cnt] = sharedPreferences.getBoolean("IsHaveSendSelfRegMsg", false);
		
        Log.d(TAG, "isHaveSendSelfRegMsg: return " + mIsHaveSendMsg[cnt]);
        return mIsHaveSendMsg[cnt];
    }

    private void setIsHaveSendSelfRegMsg(Context context, int cnt, boolean isHaveSend) {
        Log.d(TAG, "setIsHaveSendSelfRegMsg: to " + isHaveSend);
        if (cnt < 0 || cnt > 2)
		{
	        Log.d(TAG, "setIsHaveSendSelfRegMsg: count invalid " + cnt);
	        return;
		}
        mIsHaveSendMsg[cnt] = isHaveSend;
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
                DmServiceCU.getInstance().setProxy(mContext, null);
                DmServiceCU.getInstance().setProxyPort(mContext, null);
            }
        } else {
            setServerAddr(context, LAB_SERVER_ADDR);
            setSmsAddr(context, LAB_SMS_ADDR);
            setSmsPort(context, LAB_SMS_PORT);
            if (isSetApn) {
                setAPN(context, LAB_APN);
                DmServiceCU.getInstance().setProxy(mContext, "10.0.0.172");
                DmServiceCU.getInstance().setProxyPort(mContext, "80");
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
        final String selection = "name = 'CMCC DM' and numeric=\""
                + android.os.SystemProperties.get(
                        TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, curPhoneId), "") + "\"";
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

    public String getImei(int cnt) {
	if (cnt >=mPhoneCnt ) return null;
        Log.d(TAG, "getImei: " + mImeiStr);
	if (null == mImeiStr[cnt])
		{
            mImeiStr[cnt] = mTelephonyManager[cnt].getDeviceId();
        	Log.d(TAG, "getImei: " + cnt +"---" + mImeiStr);
		}
        //todo
        //need to fix
       // return "861683010001601";
        return mImeiStr[cnt];
    }
    public String getImsi(int phoneId) {
	if (phoneId >=mPhoneCnt ) return null;
        String imsi;
        imsi = mTelephonyManager[phoneId].getSubscriberId();
        Log.d(TAG, "getImsi: " + phoneId +"---" + imsi);
        return imsi;
    }
    protected void setImei(Context context, int cnt, String imei) {
	if (cnt >= mPhoneCnt || cnt < 0) 
		{
	        Log.d(TAG, "setImei invalid count: " + cnt);
		return;
		}
        Log.d(TAG, "setImei: " + imei);
        mImeiStr[cnt] = imei;

        SharedPreferences sharedPreferences = context.getSharedPreferences(DM_CONFIG, MODE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
	if ( cnt == 0)
		{
        editor.putString(ITEM_IMEI, imei);
        editor.commit();
		}
	else
		{
        editor.putString(ITEM_IMEI2, imei);
        editor.commit();
		}
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
    private void sendMsgBody(int pcount) {
        if (pcount < 0 || pcount > mPhoneCnt)
        	{
        	Log.d(TAG, "sendMsgBody invalid pcount: " + pcount);
        	return ;
        	}
        String destAddr = getSmsAddr();
        short destPort = (short) Integer.parseInt(getSmsPort());
        short srcPort = destPort;
        SmsManager smsManager = SmsManager.getDefault(pcount); //SmsManager.getDefault(curPhoneId);
        byte[] data; // sms body byte stream
        String smsBody; // sms body string format
        String imei = getImei(0);
        String imsi = getImsi(pcount);
        String softVer = getSoftwareVersion();
        String manStr = getManufactory();
        String modStr = getModel();

        Log.d(TAG, "sendMsgBody: Enter!");

        // smsbody: IMEI:860206000003972/Hisense/TS7032/TI7.2.01.22.00
        smsBody = "IMEI:" + imei + "/" +imsi +"/" + manStr + "/" + modStr + "/" + softVer;
        Log.d(TAG, "sendMsgBody: " + smsBody);
        data = smsBody.getBytes();
		
       /* for (int i = 0; i < smsBody.length(); i++) {
		
            Log.d(TAG, "sendMsgBody: ============= data[" + i + "] = " + data[i] + "hex" + Integer.toHexString(data[i]&0xff) + "\n");
        }*/

        Log.d(TAG, "sendMsgBody: dest addr = " + destAddr);
        Log.d(TAG, "sendMsgBody: dest port = " + destPort);

        //add for 105942 begin
        Log.d(TAG, "sendMsgBody: IS_CONFIG_SELFREG_REPLY = " + IS_CONFIG_SELFREG_REPLY);
       // if (false == IS_CONFIG_SELFREG_REPLY){
        if (true == IS_CONFIG_SELFREG_REPLY){
            Intent sentIntent = new Intent(SENT_SMS_ACTION);  
            PendingIntent sentPI = PendingIntent.getBroadcast(this, 0, sentIntent,  0);  
              
            Intent deliverIntent = new Intent(DELIVERED_SMS_ACTION);  
            PendingIntent deliverPI = PendingIntent.getBroadcast(this, 0,  deliverIntent, 0);

            IntentFilter mFilter = new IntentFilter(SENT_SMS_ACTION);
            mFilter.addAction(DELIVERED_SMS_ACTION);
            registerReceiver(sendMessageReceiver, mFilter);
            isRegSendMessageReceiver = true;

            smsManager.sendDmDataMessage(destAddr, null, destPort, srcPort, data, sentPI, deliverPI );
            return;
         }
        //add for 105942 end
       
	Log.d(TAG, "destAddr = "+destAddr + "  destPort = " + destPort + "  srcPort = " + srcPort + " data = " + data);
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
			Log.d(TAG, "sendMessageReceiver onReceive actionName=" + actionName + " resultCode="+resultCode);
                        if (actionName.equals(SENT_SMS_ACTION) 
                            || actionName.equals(DELIVERED_SMS_ACTION) ){
                                switch (resultCode) {
                                case Activity.RESULT_OK:
					if(mContext != null){
						TelephonyManager mTelephonyManager = (TelephonyManager) mContext.getSystemService(
						        TelephonyManager.getServiceName(mContext.TELEPHONY_SERVICE, curPhoneId));                                        
						String imsi = mTelephonyManager.getSubscriberId();
						Log.d(TAG, "The saved imsi is " + imsi);
						saveImsi(mContext,imsi);
						setRegState(mContext, true);
					
					}
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
    private void sendSelfRegMsg(int phoneId) {
        Log.d(TAG, "enter sendSelfRegMsg() mContext = " + mContext);
        if (!getSelfRegSwitch()) {
            Log.d(TAG, "sendSelfRegMsg: self registe switch is closed, no need send self registe message!");
            stopListeningServiceState();
            return;
        }        
        sendMsgBody(phoneId);
        setIsHaveSendSelfRegMsg(mContext, phoneId, true);
    }

    // Send self registe message for debug mode
    protected void sendSelfRegMsgForDebug() {
        // send message directly under debug mode
        Log.d(TAG, "sendSelfRegMsgForDebug: Enter!");
	for (int i = 0; i<mPhoneCnt; i++)
		{
		/*if (getIsCmccCard(i))
			{
	       		 sendMsgBody(i);
		         setIsHaveSendSelfRegMsg(mContext,0, true);
			}*/
		}
    }
}
