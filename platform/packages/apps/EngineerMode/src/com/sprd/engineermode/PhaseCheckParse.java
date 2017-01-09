package com.sprd.engineermode;

import android.util.Log;
import android.os.SystemProperties;

import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.Parcel;

public class PhaseCheckParse {
    private static String TAG = "PhaseCheckParse";
    private static int TYPE_WRITE_CHARGE_SWITCH = 6;

    private IBinder binder;
    private static PhaseCheckParse mPhaseCheckParse;

    private PhaseCheckParse() {
        binder = ServiceManager.getService("phasechecknative");

        if(binder != null)
            Log.e(TAG, "Get The service connect!");
        else
            Log.e(TAG, "connect Error!!");
    }

    public static PhaseCheckParse getInstance() {
        if(mPhaseCheckParse == null) {
            mPhaseCheckParse = new PhaseCheckParse();
        }
        return mPhaseCheckParse;
    }

    public boolean writeChargeSwitch(int value) {
        try{
            Parcel data = Parcel.obtain();
            Parcel reply = Parcel.obtain();
            data.writeInt(value);
            binder.transact(TYPE_WRITE_CHARGE_SWITCH, data, reply, 0);
            Log.e(TAG, "writeChargeSwitch data = " + reply.readString() + " SUCESS!!");
            data.recycle();
            return true;
        }catch (Exception ex) {
            Log.e(TAG, "Exception " , ex);
            return false;
        }
    }
}
