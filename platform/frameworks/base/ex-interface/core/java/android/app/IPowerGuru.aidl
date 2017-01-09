/* //device/java/android/android/app/IPowerGuru.aidl
**
** Copyright 2015, The Spreadtrum.com
*/
package android.app;

import android.app.PendingIntent;
import android.app.PowerGuruAlarmInfo;


/**
 * System private API for talking with the power guru service.
 *
 * {@hide}
 */
interface IPowerGuru {

    void testHello();

    //for alarmmanager
    boolean notifyPowerguruAlarm(int type, long when,long whenElapsed, long windowLength,
            long maxWhen,long interval, in PendingIntent operation);
    List<PowerGuruAlarmInfo> getBeatList();

    //for app
    List<String> getWhiteList();
    boolean delWhiteAppfromList(String appname);
    List<String> getWhiteCandicateList();
    boolean addWhiteAppfromList(String appname);
}
