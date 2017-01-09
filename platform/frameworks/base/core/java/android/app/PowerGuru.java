/*
 ** Copyright 2015 The Spreadtrum.com
 */

package android.app;

import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.SystemProperties;
import android.os.RemoteException;
import android.os.WorkSource;
import android.util.Slog;

import java.util.ArrayList;
import java.util.List;

/**
 * @hide internal api
 */
public class PowerGuru extends AbsPowerGuru{

    private static final String TAG = "PowerGuru";

	/*******members******/
    private final IPowerGuru mService;


    /**
     * package private on purpose
     */
    PowerGuru(IPowerGuru service, Context ctx) {
        super(service, ctx);
        mService = service;
    }

    //methods
    public void testHello(){
        try {
            mService.testHello();
        } catch (RemoteException ex) {
            loge("remote err:"+ex);
        }
    }

    //for alarm manager
    public boolean notifyPowerguruAlarm(int type, long when,long whenElapsed,
        long windowLength,long maxWhen,long interval, PendingIntent operation) {

        try {
            return mService.notifyPowerguruAlarm(type,when,whenElapsed,
                    windowLength,maxWhen,interval, operation);
        } catch (RemoteException ex) {
            loge("remote err:"+ex);
            return false;//return false while exception occurred
        }
    }

    public List<PowerGuruAlarmInfo> getBeatList() {
        try {
            return mService.getBeatList();
        } catch (RemoteException ex) {
            loge("remote err:"+ex);
            return null;
        }
    }

    //used by app
    public List<String> getWhiteList() {
        try {
            return mService.getWhiteList();
        } catch (RemoteException ex) {
            loge("remote err:"+ex);
            return null;
        }
    }

    public boolean delWhiteAppfromList(String appname) {
        try {
            return mService.delWhiteAppfromList(appname);
        } catch (RemoteException ex) {
            loge("remote err:"+ex);
            return false;
        }
    }

    public List<String> getWhiteCandicateList() {
        try {
            return mService.getWhiteCandicateList();
        } catch (RemoteException ex) {
            loge("remote err:"+ex);
            return null;
        }
    }

    public boolean addWhiteAppfromList(String appname) {
        try {
            return mService.addWhiteAppfromList(appname);
        } catch (RemoteException ex) {
            loge("remote err:"+ex);
            return false;
        }
    }

    //for debug
    private void loge(String info){
        Slog.e(TAG,info);
    }

    private void logd(String info){
        Slog.d(TAG,info);
    }
}
