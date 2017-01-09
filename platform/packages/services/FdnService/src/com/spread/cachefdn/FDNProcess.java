package com.spread.cachefdn;

import java.util.HashMap;
import java.util.List;

import android.app.ActivityManager;
import android.content.Context;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.os.Handler;
import android.telephony.PhoneNumberUtils;
import android.util.Log;

public class FDNProcess extends CommandDefine {

	public FDNProcess(Context ctx, String szSubID) {
		mIns = new CacheMap(ctx, szSubID);
		mIns.setPhoneNumLen(PHONE_NUMBER_MAX_LENGTH_FLAG);
		mIns.initFromSim();
		setInit(true);
        registerDatabaseChange(ctx );
	}

	public String getSubID() {
		return getCache().getSubID();
	}

	public int getCacheType(){
		return getCache().getCacheType();
	}

	public boolean registerDatabaseChange(Context ctx)
	{
		if(Uri.parse(FDNDefine.getUri(getSubID(), getCacheType())) != null){
		  myObserver = new MyObserver(mHandler);
		  ctx.getContentResolver().registerContentObserver(Uri.parse(FDNDefine.getUri(getSubID(), getCacheType())), true, myObserver);
		  Log.i(TAG, "FdnService registerDatabaseChange is sucessful ----url--->"+FDNDefine.getUri(getSubID(), getCacheType()));
		  return true;
		}
		Log.i(TAG, "FdnService registerDatabaseChange is failed ");
		return false;
	}

	public int process(int nCommand, long lParam, List<String> szValues,
			Object objvalue, List<Object> listobj) {
		if (isFDN(nCommand)) {
            Log.i(TAG, "process is FDN");
			return ProcessFDN(nCommand, lParam, szValues, objvalue, listobj);
		} else if (isCache(nCommand)) {
            Log.i(TAG, "process is Cache");
			return ProcessCache(nCommand, lParam, szValues, objvalue, listobj);
		} else if (isStatus(nCommand)) {
            Log.i(TAG, "process is Status");
			return ProcessStatus(nCommand, lParam, szValues, objvalue, listobj);
		} else {
			System.out.println(" Command Error =[0x"
					+ Integer.toHexString(nCommand) + "]");
			return CommandDefine.PROCESS_IS_ERRO;
		}

	}

	private int ProcessFDN(int nCommand, long lParam, List<String> szValues,
			Object objvalue, List<Object> listobj) {
		switch (nCommand) {
		case FDN_ENABLE:
			initCahe();
			return CommandDefine.PROCESS_IS_OK;

		case FDN_DISABLE:
            clearCache();
            return CommandDefine.PROCESS_IS_OK;
		}
		return CommandDefine.PROCESS_IS_ERRO;
	}

	private int ProcessCache(int nCommand, long lParam, List<String> szValues,
			Object objvalue, List<Object> listobj) {
		// objvalue setcallback
		switch (nCommand) {
		case CACHE_INIT:
			initCahe();
			return CommandDefine.PROCESS_IS_OK;

		case COMPARE_NUMBER_LENGTH:
			setPhoneNumLen((int)lParam);
            return CommandDefine.PROCESS_IS_OK;
			
		case DATA_SOURCE_SYNC:
		case CACHE_INIT_FORCE:
		case DATA_SOURCE_CHANGE:
			dataSourceChanged();
			return CommandDefine.PROCESS_IS_OK;

		case COMPARE_SINGEL_NUMBER:
			if(ActivityManager.isUserAMonkey()){
				Log.i(TAG, "COMPARE_SINGEL_NUMBER---number = ["+szValues.get(0)+"]");	
			}
			if(compare(szValues.get(0))){
				 return  CommandDefine.PROCESS_FDN_FILTER;
			}else{
				 return CommandDefine.PROCESS_FDN_NO_FILTER;
			}

			
		case COMPARE_MULTI_NUMBER:
			for (String valueitem :szValues) {
				compare(valueitem);
			}
			return CommandDefine.PROCESS_IS_OK;
		}
		return CommandDefine.PROCESS_IS_ERRO;
	}

	private int ProcessStatus(int nCommand, long lParam, List<String> szValues,
			Object objvalue, List<Object> listobj) {
		switch (nCommand) {
		case SIM_ABSENT:
		case RADIO_POWER_OFF:
			clearCache();
			return CommandDefine.PROCESS_IS_OK;

		case SIM_READY:
		case RADIO_POWER_ON:
			initCahe();
			return CommandDefine.PROCESS_IS_OK;

		}
		return CommandDefine.PROCESS_IS_ERRO;
	}

	private synchronized void initCahe() {
		if (!isInit()) {
			if (getCache() != null) {
				getCache().initFromSim();
				setInit(true);
				Log.i(TAG, "initCahe the cache fdn is sucess");
			}
		}

	}

	private synchronized void clearCache() {
		if (isInit()) {
			if (getCache() != null) {
				getCache().clear();
				setInit(false);
				Log.i(TAG, "clearCache the cache fdn is sucess");
			}
		}
	}

	private void dataSourceChanged() {
		if (isInit()) {
			setInit(false);
		}
		initCahe();
		Log.i(TAG, "dataSourceChanged the cache fdn is sucess");
	}

	private void dataSourceSync() {

	}

	private boolean isPhoneNumber(String sznumber) {
		if (FDNDefine.isEmpty(sznumber)) {
			Log.i(TAG, "the number is null or empty");
			return false;
		}
		if (PhoneNumberUtils.isGlobalPhoneNumber(sznumber)) {
			return true;
		}
		return false;
	}

	private boolean compare(String DestinationContact) {
        Log.d(TAG, "mIns.size:" + mIns.size());
        for(String key : mIns.keySet()) {
               Log.d(TAG, "mIns.get(" + key + "):" + mIns.get(key));
        }
      

		String szString[] = new String[1];
		szString[0] = DestinationContact;
		Log.i(TAG, "process is compare for one ");
		return (compare(szString))[0];
	}

	private boolean[] compare(String[] DestinationContacts) {
		int nIndex = 0;
		int nLen = DestinationContacts.length;
        Log.i(TAG, "the DestinationContacts.length is =["+nLen+"]");
		if (nLen <= 0) {
			Log.i(TAG, "the DestinationContacts is null or empty");
		}

		boolean bRet[] = new boolean[nLen];
		for (String szNumber : DestinationContacts) {
			bRet[nIndex++] = isInCache(szNumber);
		}
		Log.i(TAG, "process is compare for one more persion ");
		return bRet;
	}



	private boolean isInCache(String szNumber) {
		Log.i(TAG, "isInCache szNumber ----= ["+szNumber+"]");
		if (!isPhoneNumber(szNumber)) {
			return getCache().containsKey(szNumber);
		}

		int nLen = szNumber.length();
        Log.i(TAG, "isInCache szNumber.length() ----= ["+szNumber.length()+"]");
		if (nLen <= getCache().getPhoneNumLen()) {
            Log.i(TAG, "<=  isInCache getCache().containsKey(szNumber) ----= ["+getCache().containsKey(szNumber)+"]");
			return getCache().containsKey(szNumber);
		} else {
			String szTemp = szNumber.substring(nLen
					- getCache().getPhoneNumLen());
            Log.i(TAG, "> length > 9 --szTemp--= ["+szTemp+"]");
			Log.i(TAG, "> isInCache getCache().containsKey(szTemp) ----= ["+getCache().containsKey(szTemp)+"]");
			return getCache().containsKey(szTemp);
		}
	}

	public void setPhoneNumLen(int nMaxLen){
		if(!isInit()){
				getCache().setPhoneNumLen(nMaxLen);
		}
		else{
			System.out.println(" =========>>>>>System Has Init, Must NOT ReEnter Function ");
		}
	}
	


	private final CacheMap getCache() {
		return mIns;
	}
	protected void setInit(boolean bInit) {
		misHasInit = bInit;
	}

	protected boolean isInit() {
		return misHasInit;
	}
	private final CacheMap mIns;

	private boolean misHasInit = false;

	private static final String EMAIL_FLAG_CHARACTER = "@";
	private static final String TAG = "FDNProcess";
	private static final int PHONE_NUMBER_MAX_LENGTH_FLAG = 9;
	private MyObserver myObserver;
	private Handler mHandler = new Handler();
	
	
	/**********************************************************************************************
	 *  ContentObserver Database Change For Sim Card
	 *********************************************************************************************/
	class MyObserver extends ContentObserver{
		public MyObserver(Handler handler) {
			super(handler);
		}
	 
		public void onChange(boolean selfChange){
			System.out.println("  =====>>>> onChange(boolean selfChange)");

			mHandler.post(new Runnable() {
				@Override
				public void run() {
				   dataSourceChanged();	
				}
			});
		}
		
		public void onChange(boolean selfChange, Uri uri){
            if(getCache() != null){
                //add for bug 629861 --begin
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        getCache().initFromSim();
                    }
                });
                //add for bug 629861 --end
            }
			System.out.println("  =====>>>> onChange(boolean selfChange, Uri uri)"); 
		}
		 
	}
}
