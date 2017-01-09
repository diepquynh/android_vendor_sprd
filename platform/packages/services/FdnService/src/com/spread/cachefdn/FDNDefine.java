package com.spread.cachefdn;

import android.annotation.SuppressLint;
import android.telephony.TelephonyManager;

public class FDNDefine {
	public static final String[] ADDRESS_BOOK_COLUMN_NAMES = new String[] {
			"name", "number", "emails" };

	public static String getUri(String szSubid, int nType) {
		switch (nType) {
		case FDN_FLAG:
			return getFDNUri(szSubid);

		case FDN_WHITE_LIST:
		default:
			break;
		}
		return null;
	}

	private static String getFDNUri(String szSubid) {
		if (!isEmpty(szSubid)) {
			if(isMultiSimEnabled()){				
//				return URI_MORE_SUB_LND +szSubid;
				return URI_MORE_SUBID +szSubid;
			}
			 return URI_SINGAL_SUB;
//			return URI_SINGAL_SUB_LND;
		} else {
			return null;
		}
	}

	@SuppressLint("NewApi")
	public static boolean isEmpty(String szString) {
		if (szString == null || szString.isEmpty()) {
			return true;
		} else {
			return false;
		}

	}
	
	private static boolean isMultiSimEnabled(){
		return TelephonyManager.getDefault().isMultiSimEnabled();
	}

	public static final int NAME_INDEX = 0;
	public static final int NUMBER_INDEX = 1;
	public static final int EMAILS_INDEX = 2;
	private static final String URI_SINGAL_SUB = "content://icc/fdn";
	private static final String URI_MORE_SUBID = "content://icc/fdn/subId/";

	public static final int FDN_FLAG = 0x00000001;
	public static final int FDN_WHITE_LIST = 0x00000002;

	// test example
	private static final String URI_SINGAL_SUB_LND = "content://icc/lnd";
	private static final String URI_MORE_SUB_LND = "content://icc/lnd/subId/";
}
