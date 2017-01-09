
package com.spreadtrum.dm;

import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.content.Intent;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.net.NetworkInfo;
import android.os.SystemProperties;
//import com.android.internal.telephony.PhoneFactory;

	
public class DmNetwork {
    private static final String TAG = DmReceiver.DM_TAG + "DmNetwork: ";

    private ConnectivityManager mConnMgr = null;

    private static BroadcastReceiver mNetworkStateIntentReceiver = null;

    private IntentFilter mNetworkStateChangedFilter = null;

    private final Object mConnectLock = new Object();

    private int mConnectStatus = -1;
    
    private boolean isInit = false;

    Context mContext;

    public int connectiontype = -1;

    private static DmNetwork mInstance;

    String mType = DmService.getInstance().getAPN();

    public static DmNetwork getInstance() {
        return mInstance;
    }

    DmNetwork(Context context) {
        mContext = context;
        mInstance = this;
    }

    public void init() {        
        if(!isInit){
		Log.v(TAG,"dmnetwork init ,phoneid:"+DmService.getInstance().getCurrentPhoneID());
            mConnMgr = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
// 	mConnMgr = (ConnectivityManager) mContext.getSystemService(TelephonyManager.getServiceName(Context.CONNECTIVITY_SERVICE, DmService.getInstance().getCurrentPhoneID()));
            create_netstate_receiver();
            registe_netstate_receiver();
        }
        isInit = true;
    }

    public void end() {
        unregiste_netstate_receiver();
	mConnMgr = null;
	  isInit = false;
    }

    public boolean isConnected() {
        NetworkInfo networkInfo;

        if(mConnMgr!=null){
            networkInfo = mConnMgr.getActiveNetworkInfo();
            if(null == networkInfo || !networkInfo.isConnected())
                return false;
            if(//ConnectivityManager.TYPE_MOBILE_MMS
		ConnectivityManager.getNetworkTypeByPhoneId(DmService.getInstance().getCurrentPhoneID(), ConnectivityManager.TYPE_MOBILE_DM)
				!= networkInfo.getType())
                return false;
            return true;
        }

        return false;
    }

    private boolean isNetworkAvailable(String modem) {
        NetworkInfo networkInfo;

        if(mConnMgr!=null){
            networkInfo = mConnMgr.getActiveNetworkInfo();
            if(null == networkInfo || !networkInfo.isConnected())
                return false;
        }
        return true;
    }

    public int beginConnectivity(String type) {

        if (mConnMgr == null) {
            Log.v(TAG, "[ppp link] begin connectivity fail, mConnMgr is null");
            return PhoneConstants.APN_REQUEST_FAILED;
        }
        Log.v(TAG, "[ppp link] begin connectivity, type: " + type);

        mType = type;

        // int result = 0;

         connectiontype=ConnectivityManager.TYPE_MOBILE_DM;
        //connectiontype = ConnectivityManager.TYPE_MOBILE_MMS;

        if (isNetworkAvailable(null)) {
        } else {
           
            synchronized(mConnectLock)
            	{
            	try{
            	mConnectLock.wait(5 * 1000);
		}catch (InterruptedException e) {
                        Log.d(TAG, " Active networkinfo InterruptedException ");
                    }
            	}
        }
       mConnectStatus = mConnMgr.startUsingNetworkFeature(ConnectivityManager.TYPE_MOBILE,
               TelephonyManager.getServiceName(PhoneConstants.FEATURE_ENABLE_DM, DmService.getInstance().getCurrentPhoneID()));

        switch (mConnectStatus) {
            case PhoneConstants.APN_REQUEST_STARTED: {
                synchronized (mConnectLock) {
                    try {
                        mConnectLock.wait(30 * 1000); // change from 30 to 60
                        Log.d(TAG, "mConnectLock got the notify mConnectStatus "
                                + mConnectStatus);
                    } catch (InterruptedException e) {
                        Log.d(TAG, " InterruptedException ");
                    }

                    return mConnectStatus;
                }
            }
            case PhoneConstants.APN_ALREADY_ACTIVE:
            case PhoneConstants.APN_TYPE_NOT_AVAILABLE:
            case PhoneConstants.APN_REQUEST_FAILED:
			Log.d(TAG, " status: "+mConnectStatus);
                // case Phone.APN_WIFI_OPENED:
                // case Phone.APN_WIFI_ACTIVE:
                return mConnectStatus;
                /*
                 * case Phone.APN_BUSY_NOW: { if
                 * (SystemProperties.get("ro.hisense.cmcc.test",
                 * "0").equals("1")) { NetworkInfo[] info =
                 * mConnMgr.getAllNetworkInfo(); if (info != null) { for (int i
                 * = 0; i < info.length; i++) { if (info[i].getType() ==
                 * ConnectivityManager.TYPE_MOBILE_WAP && info[i].getState() ==
                 * NetworkInfo.State.CONNECTED) { connectiontype =
                 * ConnectivityManager.TYPE_MOBILE_WAP;
                 * mConnMgr.startUsingNetworkFeature
                 * (ConnectivityManager.TYPE_MOBILE, Phone.FEATURE_ENABLE_WAP);
                 * return Phone.APN_ALREADY_ACTIVE; } } } } } break;
                 */
            default:
                break;
        }

        return mConnectStatus;
    }

    public void endConnectivity() {
        try {
            if (mConnMgr != null) {
                Log.v(TAG, "[ppp link] end connectivity");

                /*
                 * if (connectiontype == ConnectivityManager.TYPE_MOBILE_WAP) {
                 * mConnMgr
                 * .stopUsingNetworkFeature(ConnectivityManager.TYPE_MOBILE,
                 * Phone.FEATURE_ENABLE_WAP); } else {
                 * mConnMgr.stopUsingNetworkFeature
                 * (ConnectivityManager.TYPE_MOBILE, Phone.FEATURE_ENABLE_DM); }
                 */
                mConnMgr.stopUsingNetworkFeature(ConnectivityManager.TYPE_MOBILE,
                        TelephonyManager.getServiceName(PhoneConstants.FEATURE_ENABLE_DM, DmService.getInstance().getCurrentPhoneID()));
            }
        } finally {
            connectiontype = -1;
            // do nothing
        }
    }

    private void create_netstate_receiver() {
        if (mNetworkStateIntentReceiver != null) {
            return;
        }
        Log.v(TAG, "[ppp link] create a net state receiver");        
        mNetworkStateChangedFilter = new IntentFilter();
        mNetworkStateChangedFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);

        mNetworkStateIntentReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                Log.v(TAG, "[ppp link] network state receiver on-receive");
                Log.v(TAG, "[ppp link] intent.getAction() = " + intent.getAction());
			//int phoneId = intent.getIntExtra("phoneId", 0);
                Log.v(TAG, "[ppp link] network state receiver on-receive phoneid:");                
			
                if (intent.getAction().equals(ConnectivityManager.CONNECTIVITY_ACTION)
				) {
                    Log.v(TAG, "[ppp link] network state receiver got CONNECTIVITY_ACTION");                    
                    boolean Connectivity = false;
                    NetworkInfo ni = (NetworkInfo) intent
                            .getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);
             
                    if (ni != null && (ni.getType() == ConnectivityManager.TYPE_MOBILE_DM  ||
		    (ni.getType() ==
                  ConnectivityManager.getNetworkTypeByPhoneId(DmService.getInstance().getCurrentPhoneID(), ConnectivityManager.TYPE_MOBILE_DM))
					)) {                        
                        
                        if (ni.getState() == NetworkInfo.State.CONNECTED) {
                            Connectivity = true;
                            Log.d(TAG, "connected: " + Connectivity);                            

                            if (Connectivity) {
                                //                            
                                mConnectStatus = PhoneConstants.APN_ALREADY_ACTIVE;
                                //TODO: to be confirm
//                                DmJniInterface.setNetworkState(mConnectStatus);
                                synchronized (mConnectLock) {
                                    try {
                                        Log.d(TAG, "notify mConnectLock ");
                                        mConnectLock.notifyAll();
                                    } catch (Exception e) {
                                        Log.d(TAG, e.toString());
                                    }
                                }

                            } else {                                
                                mConnectStatus = PhoneConstants.APN_REQUEST_FAILED;
                                //TODO: to be confirm
                                //DmJniInterface.setNetworkState(mConnectStatus);
                                synchronized (mConnectLock) {
                                    try {
                                        Log.d(TAG, "notify mConnectLock ");
                                        mConnectLock.notifyAll();
                                    } catch (Exception e) {
                                        Log.d(TAG, e.toString());
                                    }
                                }
                            }
                        } else {
                            Connectivity = false;
                        }

                    }
                }
            }
        };
    }

    private void registe_netstate_receiver() {        
        if (mNetworkStateChangedFilter != null && mNetworkStateIntentReceiver != null) {
            Log.v(TAG, "[ppp link] registe receiver");            
            mContext.registerReceiver(mNetworkStateIntentReceiver, mNetworkStateChangedFilter);
        }
    }

    private void unregiste_netstate_receiver() {        
        if (mNetworkStateIntentReceiver != null) {
            Log.v(TAG, "[ppp link] unregiste receiver");
            mContext.unregisterReceiver(mNetworkStateIntentReceiver);
            mNetworkStateIntentReceiver = null;
            mNetworkStateChangedFilter = null;
        }
    }
}
