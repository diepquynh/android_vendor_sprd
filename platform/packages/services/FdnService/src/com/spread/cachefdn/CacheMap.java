package com.spread.cachefdn;

import java.util.HashMap;

import android.content.Context;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.os.Handler;
import android.util.Log;

public class CacheMap extends HashMap<String, FdnRecord> {

	public CacheMap(Context ctx, String szSubID, int nType) {
		mCtx = ctx;
		mszSubID = new String(szSubID);
		mnType = nType;
//		initFromSim();
	}

	public CacheMap(Context ctx, String szSubID) {
		this(ctx, szSubID, FDNDefine.FDN_FLAG);
	}

	public boolean initFromSim() {

		clear();

		Cursor cur = null;
		try {
			cur = getContext().getContentResolver().query(
					Uri.parse(FDNDefine.getUri(getSubID(), getCacheType())),
					FDNDefine.ADDRESS_BOOK_COLUMN_NAMES, null, null, null);
			if (null == cur || cur.getCount() == 0) {
				return false;
			}
			while (cur.moveToNext()) {
				FdnRecord ins = new FdnRecord(cur, getCacheType());
				String szKey = getNumberKey(ins.getmNumber());

				if (szKey != null) {
					Log.i("FDNProcess", "CacheMap put number key------------>szkey = ["+szKey+"]");
					super.put(szKey, ins);
				}

				if (!FDNDefine.isEmpty(ins.getmEmails())) {
					Log.i("FDNProcess", "CacheMap put email key------------>szkey = ["+ins.getmEmails()+"]");
					super.put(ins.getmEmails(), ins);
				}

			}
			return true;
		} finally {
			if (cur != null) {
				cur.close();
			}
		}
	}

	private String getNumberKey(String szNumber) {
		if (FDNDefine.isEmpty(szNumber)) {
			return null;
		}
		int nSize = szNumber.length();
		if (nSize <= getPhoneNumLen()) {
			return szNumber;
		} else {
			return szNumber.substring(nSize - getPhoneNumLen());
		}
	}

	public String getSubID() {
		return mszSubID;
	}

	public Context getContext() {
		return mCtx;
	}

	public int getPhoneNumLen() {
		return mnMaxLenNumber;
	}

	protected void setPhoneNumLen(int lengthFlag) {
		mnMaxLenNumber = lengthFlag;
	}

	public int getCacheType() {
		return mnType;
	}

	private int mnMaxLenNumber;
	private Context mCtx;
	private final String mszSubID;
	private final int mnType;

}
