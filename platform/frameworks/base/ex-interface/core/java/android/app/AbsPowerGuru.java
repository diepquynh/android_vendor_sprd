package android.app;

import android.content.Context;
import android.os.SystemProperties;
import java.util.List;

public abstract class AbsPowerGuru {

    public static final String ACTION_POWERGURU_DATA_CHANGED = "sprd.intent.action.powerguru.PKG_CHANGED";

    public static final String HEARTBEAT_ENABLE = "persist.sys.heartbeat.enable";

    public AbsPowerGuru(IPowerGuru service, Context ctx) {

    }

    public static boolean isEnabled() {
        if (1 == SystemProperties.getInt(HEARTBEAT_ENABLE, 1)){
            return true;
        }
    return false;
    }

    public void testHello(){

    }

    public boolean notifyPowerguruAlarm(int type, long when,long whenElapsed,
           long windowLength,long maxWhen,long interval, PendingIntent operation) {
    return false;
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

