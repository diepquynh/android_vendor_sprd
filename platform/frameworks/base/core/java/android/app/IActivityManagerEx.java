/*
 * Copyright © 2016 Spreadtrum Communications Inc.
 */

package android.app;

import android.os.IBinder;
import android.os.IInterface;
import android.content.Intent;
import android.os.RemoteException;

import java.util.List;
/**
 * @hide
 */
public interface IActivityManagerEx extends IInterface {

    //add for kill-stop front app process when phone call is incoming.->
    public void startHomePre() throws RemoteException;
    public void killStopFrontApp(int func) throws RemoteException;

    int KILL_STOP_FRONT_APP_TRANSACTION = IBinder.FIRST_CALL_TRANSACTION+405;
    int START_HOME_PRE = IBinder.FIRST_CALL_TRANSACTION+406;
    //<-

    // add for bug 284692, add set process protect status interface ->
    public void setProcessProtectStatus(int pid, int status) throws RemoteException;
    public void setProcessProtectStatus(String appName, int status) throws RemoteException ;
    public void setProcessProtectArea(String appName,  int minAdj, int maxAdj, int protectLevel) throws RemoteException;
    int SET_PROCESS_PROTECT_STATUS_BYPID = IBinder.FIRST_CALL_TRANSACTION+401;
    int SET_PROCESS_PROTECT_STATUS =IBinder.FIRST_CALL_TRANSACTION+402;
    int SET_PROCESS_PROTECT_AREA = IBinder.FIRST_CALL_TRANSACTION+403;
    //<-
    //modify for Bug#618149 begin
    public void removePendingUpdateThumbTask() throws RemoteException;
    public ActivityManager.TaskThumbnail getRecentTaskThumbnail(Intent intent) throws RemoteException;
    int GET_RECENT_TASK_THUMBNAIL_TRANSACTION = IBinder.FIRST_CALL_TRANSACTION+407;
    int REMOVE_PENDING_UPDATE_THUMBNAIL_TASK_TRANSACTION = IBinder.FIRST_CALL_TRANSACTION+408;
    public List<AppAs.AppAsData> getAppAsLauncherList(String name) throws RemoteException;
    int GET_APP_AS_LAUNCH_TRANSACTION = IBinder.FIRST_CALL_TRANSACTION+409;
    public void setAppAsEnable(int flag, String name, String basePackage) throws RemoteException;
    int SET_APP_AS_ENABLE_TRANSACTION = IBinder.FIRST_CALL_TRANSACTION+410;
    public int getAppAsStatus(String name, String basePackage) throws RemoteException;
    int GET_APP_AS_STATUS_TRANSACTION = IBinder.FIRST_CALL_TRANSACTION+411;
    public List<AppAs.AppAsData> getAppAsList() throws RemoteException;
    int GET_APP_AS_TRANSACTION = IBinder.FIRST_CALL_TRANSACTION+412;
    public List<AppAs.AppAsRecord> getAppAsRecordList() throws RemoteException;
    int GET_APP_AS_RECORD_TRANSACTION = IBinder.FIRST_CALL_TRANSACTION+413;
    //modify for Bug#618149 end
}
