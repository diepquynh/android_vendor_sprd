package com.sprd.cellbroadcastreceiver.util;

import java.util.List;
import java.util.ArrayList;

import android.os.RemoteException;
import android.telephony.SubscriptionInfo;
import com.android.internal.telephony.ISmsEx;
import android.os.ServiceManager;
import android.telephony.SubscriptionManager;
import com.android.internal.telephony.SubscriptionController;
import android.content.Context;
import android.util.Log;
public class SystemAdapter {

  private static SystemAdapter mIns = null;
 
  private SystemAdapter( )
  {
	     System.out.print("<<<=====SystemAdapter  Init Instance =======>>>" );
  }
 
  public static  synchronized SystemAdapter getInstance()
  {
	     if( mIns == null){
	    	   mIns = new SystemAdapter();
	     }
	     return mIns;
  }
  
  public static synchronized  void  ReleaseInstance()
  {
	     if( mIns != null){
	    	   mIns =  null;
	     }
  }
  
	 public    int getPhoneId(int nsubid){
		 return SubscriptionController.getInstance().getPhoneId(nsubid);
	 }
	 	 
	 
	 
	 public boolean commonInterfaceForMessaging(int commonType, long szSubId, String szString, int[] data)
	 {
		
        boolean success = false;
        try {
              ISmsEx iccISms = ISmsEx.Stub.asInterface(ServiceManager.getService("ismsEx"));
              Log.d("SystemAdapter"," iccISms = " + iccISms + " success = " + success);
              if (iccISms != null) {
                success = iccISms.commonInterfaceForMessaging(commonType,
                        szSubId, null, data);
            }
        } catch (RemoteException ex) {
            // ignore it
        }
        Log.d("SystemAdapter"," success = " + success);
        return success;
      }
	 
}
