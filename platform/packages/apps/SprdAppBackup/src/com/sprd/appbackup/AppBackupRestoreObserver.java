package com.sprd.appbackup;

import android.os.RemoteException;

import com.sprd.appbackup.service.IAppBackupRestoreObserver;

public class AppBackupRestoreObserver extends IAppBackupRestoreObserver.Stub{

    public final static int FLAG_NODATA = -2;
    public final static int FLAG_FAIL = -1;
    public final static int FLAG_SUCCESS = 0;
    public final static int FLAG_SDCARD_STORAGE_LACK = 1;
    public final static int FLAG_INTERNAL_STORAGE_LACK = 2;
    public final static int FLAG_FILE_INVLALD = 3;
    public final static int FLAG_DUPLICATION_UNSUPPORT = 4;
    public final static int FLAG_DUPLICATION_SUCCESS = 5;

    private int mCurrent;
    private int mTotal;
    private int mResult;
    private String mUnit;

    public AppBackupRestoreObserver(){
        mCurrent = 0;
        mTotal = 100;
        mUnit = "";
    }

    public void onUpdate(int current, int total) throws RemoteException {
        mCurrent = current;
        mTotal = total;
        mUnit = "";
    }

    public void onUpdateWithUnit(int current, int total, String unit)
            throws RemoteException {
        mCurrent = current;
        mTotal = total;
        if (unit == null) {
            mUnit = "";
        } else {
            mUnit = unit;
        }
    }
    public String getUnit() {
        return mUnit;
    }
    @Override
    public void onResult(int resultCode) throws RemoteException {
       mResult = resultCode;
    }
    public int getmCurrent() {
        return mCurrent;
    }
    public int getmTotal() {
        return mTotal;
    }
    public int getResult(){
        return mResult;
    }
    @Override
    public String toString() {
        return String.valueOf(mCurrent) +"/"+String.valueOf(mTotal)+"/"+String.valueOf(mResult);
    }

}
