package com.spread.cachefdn;

import android.database.Cursor;
import android.text.TextUtils;
import android.util.Log;

public class FdnRecord extends FDNDefine {

	private FdnRecord(int nType) {
		mFlag = FDN_FLAG;
	}

	public FdnRecord(Cursor cursor, int nType) {
		this(nType);
		if (cursor == null) {
			return;
		}
		try {
            if(cursor.getString(NAME_INDEX) != null){
			   mName = cursor.getString(NAME_INDEX);
            }
            if(cursor.getString(NUMBER_INDEX) != null){
			   mNumber = cursor.getString(NUMBER_INDEX);
            }
            if(cursor.getString(EMAILS_INDEX) != null){
			    mEmails = cursor.getString(EMAILS_INDEX);
            }
		} catch (Exception e) {
			e.printStackTrace();
			init(null, null, null);
		}
        Log.i("FdnRecord", "FdnRecord contrunct----> "+toString());

		if (isEmpty(mName, mEmails)) {
			System.out.println("FdnRecord [mName]  and  [mEmails]  is empty ");
		}

	}

	public void init(String mName, String mNumber, String mEmails) {

		this.mName = mName;
		this.mNumber = mNumber;
		this.mEmails = mEmails;
	}

	public String getmName() {
		return mName;
	}

	public void setmName(String mName) {
		this.mName = mName;
	}

	public String getmNumber() {
		return mNumber;
	}

	public void setmNumber(String mNumber) {
		this.mNumber = mNumber;
	}

	public String getmEmails() {
		return mEmails;
	}

	public void setmEmails(String mEmails) {
		this.mEmails = mEmails;
	}

	public boolean isEmpty(String name, String emails) {
		if (TextUtils.isEmpty(name) && TextUtils.isEmpty(emails)) {
			return true;
		}
		return false;
	}

	@Override
	public String toString() {
		return "FdnRecord [mName=" + mName + ", mNumber=" + mNumber
				+ ", mEmails=" + mEmails + "]";
	}

	public void setFDNFlag() {
		mFlag = FDN_FLAG;
	}

	public void setWhileFlag() {
		mFlag = FDN_WHITE_LIST;
	}

	public boolean isFDNFlag() {
		return ((mFlag & FDN_FLAG) == FDN_FLAG);
	}

	public boolean istWhileFlag() {
		return ((mFlag & FDN_WHITE_LIST) == FDN_WHITE_LIST);
	}

	private int mFlag;
	private String mName;
	private String mNumber;
	private String mEmails;

}
