
package com.spreadtrum.dm.vdmc;

import android.os.Message;
import android.util.Log;
import android.content.Context;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.Looper;
import android.net.Uri;
import android.database.Cursor;

import android.provider.Telephony;
import android.content.ContentValues;

import com.android.internal.telephony.TelephonyProperties;
import com.spreadtrum.dm.DmNativeInterface;
import com.spreadtrum.dm.DmNetwork;
import com.spreadtrum.dm.DmService;
import android.os.Bundle;
import android.os.Handler;
import android.content.ContentResolver;
import android.content.Intent;
import android.database.ContentObserver;
import java.lang.Thread;
import com.android.internal.telephony.Phone;
//import com.android.internal.telephony.PhoneFactory;
import android.telephony.TelephonyManager;
public class Vdmc {
    protected static final String DM_TAG = "DM ==> ";

    private static String TAG = DM_TAG + "Vdmc: ";

    private static Vdmc me = null;

    private static Context _appContext = null;

    private PowerManager mPowerManager;

    private PowerManager.WakeLock mWakeLock;

    public static final int DM_NULL_DIALOG = 0;

    public static final int DM_NIA_INFO_DIALOG = 1;

    public static final int DM_NIA_CONFIRM_DIALOG = 2;

    public static final int DM_ALERT_INFO_DIALOG = 3;

    public static final int DM_ALERT_CONFIRM_DIALOG = 4;

    public static final int DM_ALERT_SINGLE_CHOICE_DIALOG = 5;

    public static final int DM_ALERT_MULTI_CHOICE_DIALOG = 6;

    public static final int DM_CONFIRM_DOWNLOAD_DIALOG = 7;

    public static final int DM_CONFIRM_UPDATE_DIALOG = 8;

    public static final int DM_SIMULATE_UPDATE_DIALOG = 9;

    public static final int DM_PROGRESS_DIALOG = 10;

    public static boolean isDmSetting = false;

    public static boolean isWAPSetting = false;

    protected VdmcMsgHandler _msgHandler = null;

    public static String tmpnetapn = null;

    public static String tmpnetport = null;

    public static String tmpnetproxy = null;

    public static String tmpnetuser = null;

    public static String tmpnetpwd = null;

    public static String tmpwapapn = null;

    public static String tmpwapport = null;

    public static String tmpwapproxy = null;

    public static String tmpwapuser = null;

    public static String tmpwappwd = null;

    public static String tmpmmsmmsc = null;

    public static String tmpbrowserapn = null;

    public static String tmpstreamapn = null;

    public static String tmpdmwapapn=null;
    public static String tmpdmpwapport=null;
    public static String tmpdmpwapproxy=null;
    public static String tmpdmpwapconn=null;
    
    // bug 292626 begin
    public static String tmpCmsuplApn = null;
    // bug 292626 end

    public static boolean tmpUpdateStream = false;
    public static boolean tmpStreamUpdated = false;
    public static int tmpUpdateStreamNum = 0;
    public static int tmpStreamUpdatedNum = 0;
    private final Object mwait = new Object();
		
    public enum DM_START_RESULT_E {
        DM_START_NONE, DM_START_SUCC, DM_START_FAIL, DM_START_DONE,
    };

    public enum MMIDM_DM_STATE {
        DM_NONE, // dm init state
        DM_START, // dm starte state
        DM_RUN, // dm run state
        DM_CANCEL, // dm cancel state
    };

    public enum SessionType // dm session type
    {
        DM_SESSION_NONE, DM_SESSION_USER, DM_SESSION_CLIENT, DM_SESSION_SERVER,
    };

    private static SessionType _sessionType = SessionType.DM_SESSION_NONE;

    private static String _lastSessionState = "NULL";

    private static int _lastError = 0;

    public static Vdmc getInstance() {
        if (me == null) {
            me = new Vdmc();
        }
        return me;
    }

    public static Context getAppContext() {
        return DmService.getContext();
    }

    private static class DMThread extends Thread {

        private int dmtype;

        private byte[] dmmessage;

        private int dmmsglen;

        public DMThread(int type, byte[] message, int msglen) {
            dmtype = type;
            dmmessage = message;
            dmmsglen = msglen;
            Log.d(TAG, "DMThread : created !");
        }

        @Override
        public void run() {
            /*
             * for (int i=0; i<dmmessage.length; i++){ Log.d(TAG,
             * "dmmessage["+i+"] = "+dmmessage[i]); }
             */
          //   Looper.prepare();
            Log.d(TAG, "call : JMMIDM_StartVDM !");
            DmService.getInstance().getDmNativeInterface().spdm_jni_start(dmtype, dmmessage, dmmsglen);
            Log.d(TAG, "call : JMMIDM_StartVDM END !");
        //Looper.loop();
        }

    }
	
  private ContentObserver mob = new ContentObserver(new Handler()) {
     		  public void onChange(final boolean selfChange) {
                Log.v(TAG, "DMContentObserver received notification");
			synchronized(mwait)
				{
				   	try{
						tmpStreamUpdatedNum++;
                				Log.v(TAG, "DMContentObserver received notification" +tmpStreamUpdatedNum );
						tmpStreamUpdated = true;
						mwait.notifyAll();
				   		}catch (Exception e) {
                                        Log.d(TAG, e.toString());
                                    }
				}
       			 }
            } ;
  /*
   public void registerVDMobserver()
   	{
	  final Uri CURI = Uri.parse("content://com.android.gallery3d/movieview");
           tmpUpdateStreamNum++;
	   Log.d(TAG,"dm registerVDMobserver ok:"+tmpUpdateStreamNum);   	
   	    if ( !tmpUpdateStream )
   	    	{
   	    	Log.d(TAG,"dm registerVDMobserver ok");
   	     _appContext.getContentResolver().registerContentObserver(
           		 CURI, true, mob);
		tmpUpdateStream = true;
   	    }
   	}
   */
    public void startVDM(Context context, SessionType type, byte[] message, String msgOrigin) {

        int result;

        // Log.d(TAG, "startVDM : message = " + message);
        Log.d(TAG, "startVDM : msgOrigin = " + msgOrigin);
        Log.d(TAG, "startVDM : me = " + me);        
	
	tmpUpdateStream = false;
	tmpStreamUpdated = false;
	tmpUpdateStreamNum = 0;
	tmpStreamUpdatedNum = 0;
        // back up session type
        _sessionType = type;
  //      _msgHandler = new VdmcMsgHandler();
        _appContext = context;

        // Log.d(TAG, "startVDM : _msgHandler = " + _msgHandler);

        if (!DmService.getInstance().getDmNativeInterface().spdm_jni_isDmRunning()) {            
            int i = message.length;
            int inttype = 0;
        /*    while (message[i] != 0)
                i++;*/

            if (type == Vdmc.SessionType.DM_SESSION_NONE)
                inttype = 0;
            else if (type == SessionType.DM_SESSION_USER)
                inttype = 1;
            else if (type == SessionType.DM_SESSION_CLIENT)
                inttype = 2;
            else if (type == SessionType.DM_SESSION_SERVER)
                inttype = 3;            
            DmNetwork.getInstance().init();

            DMThread dmthd = new DMThread(inttype, message, i);

            mPowerManager = (PowerManager) context.getSystemService(context.POWER_SERVICE);
            mWakeLock = null;
            try {
                mWakeLock = mPowerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "dm");
                mWakeLock.acquire();
            } catch (SecurityException e) {
                Log.w(TAG, "No permission to acquire wake lock", e);
                mWakeLock = null;
            }
            Log.d(TAG, "startVDM : acquire wake lock ");
            dmthd.start();

            isDmSetting = false;

            Log.d(TAG, "startVDM : End!");
        } else {
            // DMNativeMethod.JMMIDM_ExitDM();
            Log.d(TAG, "DM is already run :return!");
        }

    }
	
    Handler apnhandler = new Handler(){

        @Override
        public void handleMessage(Message msg) {
            Bundle bundle = (Bundle)msg.obj;
            String name = bundle.getString("name");
            String apn = bundle.getString("apn");
            String where = "numeric=?" + " and name=?";
            ContentValues values = new ContentValues();
            values.put("apn", apn);
            
            ContentResolver cr = _appContext.getContentResolver();
            int rowId = cr.update(
		 (DmService.getInstance().getCurrentPhoneID()==0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(DmService.getInstance().getCurrentPhoneID(),null), 
		values, where,
                    new String[] {
                            android.os.SystemProperties.get(
                                    TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, DmService.getInstance().getCurrentPhoneID()), ""),name
                    });
        }
      
    };

    public void stopVDM() {
        Log.d(TAG, "stopVDM : begin!");

        _sessionType = SessionType.DM_SESSION_NONE;
        if (isDmSetting) 
	{
            isDmSetting = false;
		Log.d(TAG, "writeDMapnParam entering " );
			
            final String selection = "(name = 'CMCC DM' or name='CMCCDM_USIM') and numeric=\""
                    + android.os.SystemProperties.get(
                            TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, DmService.getInstance().getCurrentPhoneID()), "") + "\"";
            Log.d(TAG, "writeCMCCDMParam 	selection:	= " + selection);
            ContentValues values = new ContentValues();
            values.put(Telephony.Carriers.APN, tmpdmwapapn);
            values.put(Telephony.Carriers.PORT, tmpdmpwapport);
            values.put(Telephony.Carriers.PROXY, tmpdmpwapproxy);

            int c = values.size() > 0 ? DmService.getInstance().getContext().getContentResolver().update(
                     (DmService.getInstance().getCurrentPhoneID()==0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(DmService.getInstance().getCurrentPhoneID(),null), 
                    values, selection, null) : 0;

            Log.d(TAG, "writecmdmParam: " + ", value = " + tmpdmwapapn
                            + ", update count = " + c);
			
            DmService.getInstance().setOnlyAPN(_appContext, tmpdmwapapn);
            DmService.getInstance().setOnlyProxy(_appContext, tmpdmpwapproxy);
            DmService.getInstance().setOnlyProxyPort(_appContext,tmpdmpwapport);
        }
 //test0
        if (tmpmmsmmsc != null) {
		Log.d(TAG, "writeMMSParam entering " );
            String numric = android.os.SystemProperties.get(
                    TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, DmService.getInstance().getCurrentPhoneID()), "");
            final String selection = "(name = 'CMCCWAP' or name='CMWAP' or name='CMCCMMS_SIM') and numeric="
                    + numric;
		Log.d(TAG, "writeMMSParam entering1 " );

            ContentValues values = new ContentValues();
            values.put(Telephony.Carriers.MMSC, tmpmmsmmsc);
		Log.d(TAG, "writeMMSParam updating.... " );

            int c = _appContext.getContentResolver().update(
                     (DmService.getInstance().getCurrentPhoneID()==0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(DmService.getInstance().getCurrentPhoneID(),null), 
                   values, selection, null) ;
            Log.d(TAG, "writeMMSParam: update count = " + c);
        }
        
        tmpmmsmmsc = null;
 //test1
        if (tmpnetapn != null || tmpnetport!=null || tmpnetproxy != null) {
		Log.d(TAG, "writeCMNETParam entering " );
            final String selection = "(name = 'CMCCNET' or name='CMNET' or name='CMCCNET_USIM') and numeric=\""
                    + android.os.SystemProperties.get(
                            TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, DmService.getInstance().getCurrentPhoneID()), "") + "\"";
            Log.d(TAG, "writeGprsCmnetParam 	selection:	= " + selection);
            ContentValues values = new ContentValues();
		if (tmpnetapn != null)
            values.put(Telephony.Carriers.APN, tmpnetapn);
            values.put(Telephony.Carriers.PORT, tmpnetport);
            values.put(Telephony.Carriers.PROXY, tmpnetproxy);
            values.put(Telephony.Carriers.USER, tmpnetuser);
            values.put(Telephony.Carriers.PASSWORD, tmpnetpwd);
            int c = values.size() > 0 ? DmService.getInstance().getContext().getContentResolver().update(
                     (DmService.getInstance().getCurrentPhoneID()==0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(DmService.getInstance().getCurrentPhoneID(),null), 
                    values, selection, null) : 0;

            Log.d(TAG, "writeGprsCmnetParam: " + ", value = " + tmpwapapn
                            + ", update count = " + c);
        }
        tmpnetapn = null;
        tmpnetport = null;
        tmpnetproxy = null;
        tmpnetuser = null;
        tmpnetpwd = null;
/*
		if (tmpbrowserapn != null)
		{
		Log.d(TAG, "writeBrowserapnParam entering " );
		
		ContentResolver cr = _appContext.getContentResolver();
		Uri uri = Uri.parse("content://browser/apn_setting");
		Cursor cursor = cr.query(uri, new String[]{"name"}, "_id=?", new String[]{"1"}, null);
		if(cursor.moveToNext()){
			String name = cursor.getString(0);
			Bundle bundle = new Bundle();
			bundle.putString("name", name);
			bundle.putString("apn",tmpbrowserapn);
			
			Message msg = apnhandler.obtainMessage(1,bundle);
			apnhandler.sendMessage(msg);
		}
		
		cursor.close();

		}

		tmpbrowserapn = null;
        if (tmpstreamapn != null)
        {
        Intent i = new Intent("com.android.dm.vdmc");
            i.putExtra("type", "conn_prof");
            i.putExtra("value", tmpstreamapn);
            _appContext.sendBroadcast(i);
            Log.d(TAG, "DM Broadcast set streaming info");           
        }
 
 	tmpstreamapn = null;
*/
        if (tmpwapapn != null || tmpwapproxy!=null ||  tmpwapport!= null) {
		Log.d(TAG, "writeCMWAPapnParam entering " );
			
            final String selection = "(name = 'CMCCWAP' or name='CMWAP' or name='CMCCWAP_USIM') and numeric=\""
                    + android.os.SystemProperties.get(
                            TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, DmService.getInstance().getCurrentPhoneID()), "") + "\"";
            Log.d(TAG, "writeGprsCmwapParam 	selection:	= " + selection);
            ContentValues values = new ContentValues();
		if (tmpwapapn != null)
            values.put(Telephony.Carriers.APN, tmpwapapn);
		if ( tmpwapport!= null)
            values.put(Telephony.Carriers.PORT, tmpwapport);
		if (tmpwapproxy != null)
            values.put(Telephony.Carriers.PROXY, tmpwapproxy);
            values.put(Telephony.Carriers.USER, tmpwapuser);
            values.put(Telephony.Carriers.PASSWORD, tmpwappwd);

            int c = values.size() > 0 ? DmService.getInstance().getContext().getContentResolver().update(
                     (DmService.getInstance().getCurrentPhoneID()==0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(DmService.getInstance().getCurrentPhoneID(),null), 
                    values, selection, null) : 0;

            Log.d(TAG, "writeGprsCmwapParam: " + ", value = " + tmpwapapn
                            + ", update count = " + c);
        }
        
        tmpwapapn = null;
        tmpwapport = null;
        tmpwapproxy = null;
        tmpwapuser = null;
        tmpwappwd = null;

		if (tmpdmwapapn!=null ||tmpdmpwapport!=null ||  tmpdmpwapproxy != null)
		{
		Log.d(TAG, "writeCMCCWAP DMapnParam entering " );
		
		    final String selection = "(name = 'CMCCWAP DM') and numeric=\""
				   + android.os.SystemProperties.get(
				           TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, DmService.getInstance().getCurrentPhoneID()), "")
				   + "\"";
			Log.d(TAG, "writedmwapParam 	selection:	= " + selection);
		    ContentValues values = new ContentValues();

	        values.put(Telephony.Carriers.APN, tmpdmwapapn);
    		values.put(Telephony.Carriers.PORT, tmpdmpwapport);
    		values.put(Telephony.Carriers.PROXY, tmpdmpwapproxy);

		
			int c = values.size() > 0 ? DmService.getInstance().getContext().getContentResolver().update(
				 (DmService.getInstance().getCurrentPhoneID()==0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(DmService.getInstance().getCurrentPhoneID(),null),
				values, selection, null) : 0;
	
	        Log.d(TAG, "writedmwapParam: " +  ", value = " + tmpdmwapapn + ", update count = " + c);
   		}
   		
		tmpdmwapapn = null;
		tmpdmpwapport = null;
		tmpdmpwapproxy = null;
		tmpdmpwapconn = null;		
        // bug 292626 begin
        if (null != tmpCmsuplApn) {
            DmService.getInstance().setAGPSApn(_appContext,
                    tmpCmsuplApn.toLowerCase());
            tmpCmsuplApn = null;
        }
        // bug 292626 end     
		/*
		while (tmpUpdateStream && tmpStreamUpdatedNum<tmpUpdateStreamNum)
		{
			synchronized(mwait)
				{
				  try{
				  	Log.d(TAG, " STOP VDMC waiting streaming update ");
            				mwait.wait(8* 1000);
					}catch (InterruptedException e) {
                      		       Log.d(TAG, " STOP VDMC wait Exception ");
                   			 }
				}
		}
	_appContext.getContentResolver().unregisterContentObserver(mob);
	*/
	tmpUpdateStream = false;
	tmpStreamUpdated = false;
	
        _appContext = null;
        me = null;
	
        if (mWakeLock != null && mWakeLock.isHeld()) {
            Log.d(TAG, "stopVDM : release wake lock ");
            mWakeLock.release();
        }
	System.gc();	
        Log.d(TAG, "stopVDM : END!");
	//System.runFinalization(); 
	
	//DmService.getInstance().clearDmNativeInterface();
	//DmService.getInstance().getDmNativeInterface();
    }

    public boolean isVDMRunning() {

        return DmService.getInstance().getDmNativeInterface().spdm_jni_isDmRunning();
    }

    // get current session type
    public SessionType getSessionType() {
        return _sessionType;
    }

    // get last session state
    public static String getLastSessionState() {
        return _lastSessionState;
    }

    // get last error code
    public static int getLastError() {
        return _lastError;
    }

    // Android event handling
    public void sendMessage(Message msg) {
//        if (_msgHandler != null)
         //   _msgHandler.sendMessage(msg);
    }

}
