package com.android.server.power;

import android.app.IPowerGuru;
import android.app.PendingIntent;
import android.app.PowerGuruAlarmInfo;

import android.content.Context;

import java.util.List;

public abstract class AbsPowerGuruService extends IPowerGuru.Stub{

    public AbsPowerGuruService(Context context) {

    }

    public static boolean isEnabled() {
        return false;
    }

    public void testHello(){

    }

    public boolean notifyPowerguruAlarm(int type, long when,long whenElapsed, long windowLength,
           long maxWhen, long interval, PendingIntent operation) {
    return true;
    }

    public List<PowerGuruAlarmInfo> getBeatList() {
    return null;
    }

    public List<String> getWhiteList() {
    return null;
    }

    public boolean delWhiteAppfromList(String appname) {
    return false;
    }

    public List<String> getWhiteCandicateList() {
    return null;
    }

    public boolean addWhiteAppfromList(String appname) {
    return false;
    }

}

