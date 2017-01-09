/*
 * Copyright © 2016 Spreadtrum Communications Inc.
 */

package android.app;

import android.os.IBinder;
import android.os.Parcel;
import android.os.Parcelable;
import android.content.Intent;
import android.os.RemoteException;

import java.util.List;
import java.util.ArrayList;
import java.util.Arrays;

/**
 * @hide
 */
public abstract class ActivityManagerNativeEx extends ActivityManagerNative implements IActivityManager {

    @Override
    public boolean onTransact(int code, Parcel data, Parcel reply, int flags)
        throws RemoteException{
        switch (code) {
            //add for kill-stop front app process when phone call is incoming. ->
            case KILL_STOP_FRONT_APP_TRANSACTION: {
                data.enforceInterface(IActivityManager.descriptor);
                killStopFrontApp(data.readInt());
                reply.writeNoException();
                return true;
            }

            case START_HOME_PRE: {
                data.enforceInterface(IActivityManager.descriptor);
                startHomePre();
                reply.writeNoException();
                return true;
            }
            //<-
            // add for bug 284692, add set process protect status interface ->
            case SET_PROCESS_PROTECT_STATUS_BYPID: {
                data.enforceInterface(IActivityManager.descriptor);
                int pid = data.readInt();
                int status = data.readInt();
                setProcessProtectStatus(pid, status);
                reply.writeNoException();
                return true;
            }
            case SET_PROCESS_PROTECT_STATUS:{
                data.enforceInterface(IActivityManager.descriptor);
                String appName = data.readString();
                int status = data.readInt();
                setProcessProtectStatus(appName, status);
                reply.writeNoException();
                return true;
            }
            case SET_PROCESS_PROTECT_AREA:{
                data.enforceInterface(IActivityManager.descriptor);
                String appName = data.readString();
                int minAdj = data.readInt();
                int maxAdj = data.readInt();
                int protectLevel = data.readInt();
                setProcessProtectArea(appName, minAdj, maxAdj, protectLevel);
                reply.writeNoException();
                return true;
            }
            //<-
            //modify for Bug#618149 begin
            case GET_RECENT_TASK_THUMBNAIL_TRANSACTION: {
                data.enforceInterface(IActivityManager.descriptor);
                Intent intent = Intent.CREATOR.createFromParcel(data);
                ActivityManager.TaskThumbnail taskThumbnail = getRecentTaskThumbnail(intent);
                reply.writeNoException();
                if (taskThumbnail != null) {
                    reply.writeInt(1);
                    taskThumbnail.writeToParcel(reply, Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
                } else {
                    reply.writeInt(0);
                }
                return true;
            }
            case REMOVE_PENDING_UPDATE_THUMBNAIL_TASK_TRANSACTION: {
                data.enforceInterface(IActivityManager.descriptor);
                removePendingUpdateThumbTask();
                reply.writeNoException();
                return true;
            }
            case GET_APP_AS_LAUNCH_TRANSACTION: {
                data.enforceInterface(IActivityManager.descriptor);
                String packageName = data.readString();
                List<AppAs.AppAsData> list =  getAppAsLauncherList(packageName);
                reply.writeNoException();
                int N = list != null ? list.size() : -1;
                reply.writeInt(N);
                int i;
                for (i=0; i<N; i++) {
                    AppAs.AppAsData info = list.get(i);
                    info.writeToParcel(reply, 0);
                }
                return true;
            }
            case SET_APP_AS_ENABLE_TRANSACTION: {
                data.enforceInterface(IActivityManager.descriptor);
                int flag = data.readInt();
                String callingPackage= data.readString();
                String basePackage = data.readString();
                setAppAsEnable(flag, callingPackage, basePackage);
                reply.writeNoException();
                return true;
            }
            case GET_APP_AS_STATUS_TRANSACTION: {
                data.enforceInterface(IActivityManager.descriptor);
                String callingPackage= data.readString();
                String basePackage = data.readString();
                int flag = getAppAsStatus(callingPackage, basePackage);
                reply.writeNoException();
                reply.writeInt(flag);
                return true;
            }
            case GET_APP_AS_TRANSACTION: {
                data.enforceInterface(IActivityManager.descriptor);
                List<AppAs.AppAsData> list =  getAppAsList();
                reply.writeNoException();
                int N = list != null ? list.size() : -1;
                reply.writeInt(N);
                int i;
                for (i=0; i<N; i++) {
                    AppAs.AppAsData info = list.get(i);
                    info.writeToParcel(reply, 0);
                }
                return true;
            }
            case GET_APP_AS_RECORD_TRANSACTION: {
                data.enforceInterface(IActivityManager.descriptor);
                List<AppAs.AppAsRecord> list =  getAppAsRecordList();
                reply.writeNoException();
                int N = list != null ? list.size() : -1;
                reply.writeInt(N);
                int i;
                for (i=0; i<N; i++) {
                    AppAs.AppAsRecord info = list.get(i);
                    info.writeToParcel(reply, 0);
                }
                return true;
            }
        //modify for Bug#618149 end
        }
        return super.onTransact(code, data, reply, flags);
    }
}

class ActivityManagerProxyEx extends ActivityManagerProxy implements IActivityManager {
    IBinder mRemote;

    public ActivityManagerProxyEx(IBinder remote) {
        super(remote);
        mRemote = remote;
    }

    //add for kill-stop front app process when phone call is incoming.->
    public void startHomePre() throws RemoteException {
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        data.writeInterfaceToken(IActivityManager.descriptor);
        mRemote.transact(START_HOME_PRE, data, reply, 0);
        reply.readException();
        data.recycle();
        reply.recycle();
    }

    public void killStopFrontApp(int func) throws RemoteException {
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        data.writeInterfaceToken(IActivityManager.descriptor);
        data.writeInt(func);
        mRemote.transact(KILL_STOP_FRONT_APP_TRANSACTION, data, reply, 0);
        reply.readException();
        data.recycle();
        reply.recycle();
    }
    //<-

    // add for bug 284692, add set process protect status interface ->
    public void setProcessProtectStatus(int pid, int status) throws RemoteException {
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        data.writeInterfaceToken(IActivityManager.descriptor);
        data.writeInt(pid);
        data.writeInt(status);
        mRemote.transact(SET_PROCESS_PROTECT_STATUS_BYPID, data, reply, 0);
        reply.readException();
        data.recycle();
        reply.recycle();
    }

    /**
     * @param status process ptotected status, value of {@link ActivityManager#PROCESS_STATUS_IDLE}, {@link ActivityManager#PROCESS_STATUS_RUNNING}, {@link ActivityManager#PROCESS_STATUS_MAINTAIN}, {@link ActivityManager#PROCESS_STATUS_PERSISTENT}
     */
    public void setProcessProtectStatus(String appName, int status) throws RemoteException {
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        data.writeInterfaceToken(IActivityManager.descriptor);
        data.writeString(appName);
        data.writeInt(status);
        mRemote.transact(SET_PROCESS_PROTECT_STATUS, data, reply, 0);
        reply.readException();
        data.recycle();
        reply.recycle();
    }

    /**
     * @param protectLevel the lever of process ptotected area, value of {@link ActivityManager#PROCESS_PROTECT_CRITICAL}, {@link ActivityManager#PROCESS_PROTECT_IMPORTANCE}, {@link ActivityManager#PROCESS_PROTECT_NORMAL}
     */
    public void setProcessProtectArea(String appName,  int minAdj, int maxAdj, int protectLevel) throws RemoteException{
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        data.writeInterfaceToken(IActivityManager.descriptor);
        data.writeString(appName);
        data.writeInt(minAdj);
        data.writeInt(maxAdj);
        data.writeInt(protectLevel);
        mRemote.transact(SET_PROCESS_PROTECT_AREA, data, reply, 0);
        reply.readException();
        data.recycle();
        reply.recycle();
    }
    //<-
    //modify for Bug#618149 begin
    public ActivityManager.TaskThumbnail getRecentTaskThumbnail(Intent intent) throws RemoteException {
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        data.writeInterfaceToken(IActivityManager.descriptor);
        intent.writeToParcel(data,0);
        mRemote.transact(GET_RECENT_TASK_THUMBNAIL_TRANSACTION, data, reply, 0);
        reply.readException();
        ActivityManager.TaskThumbnail taskThumbnail = null;
        if (reply.readInt() != 0) {
            taskThumbnail = ActivityManager.TaskThumbnail.CREATOR.createFromParcel(reply);
        }
        data.recycle();
        reply.recycle();
        return taskThumbnail;
    }
    public void removePendingUpdateThumbTask() throws RemoteException {
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        data.writeInterfaceToken(IActivityManager.descriptor);
        mRemote.transact(REMOVE_PENDING_UPDATE_THUMBNAIL_TASK_TRANSACTION, data, reply, 0);
        reply.readException();
        data.recycle();
        reply.recycle();
    }
    //modify for Bug#618149 end
    public List<AppAs.AppAsData> getAppAsLauncherList(String packageName) throws RemoteException {
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        data.writeInterfaceToken(IActivityManager.descriptor);
        data.writeString(packageName);
        mRemote.transact(GET_APP_AS_LAUNCH_TRANSACTION, data, reply, 0);
        reply.readException();
        ArrayList<AppAs.AppAsData> mList = null;
        int N = reply.readInt();
        if (N >= 0) {
            mList = new ArrayList<AppAs.AppAsData>();
            while (N > 0) {
                AppAs.AppAsData info =
                        AppAs.AppAsData.CREATOR
                        .createFromParcel(reply);
                mList.add(info);
                N--;
            }
        }
        data.recycle();
        reply.recycle();
        return mList;
    }

    public void setAppAsEnable(int flag, String callingPackage, String basePackage) throws RemoteException {
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        data.writeInterfaceToken(IActivityManager.descriptor);
        data.writeInt(flag);
        data.writeString(callingPackage);
        data.writeString(basePackage);
        mRemote.transact(SET_APP_AS_ENABLE_TRANSACTION, data, reply, 0);
        reply.readException();
        data.recycle();
        reply.recycle();
    }

    public int getAppAsStatus(String callingPackage, String basePackage) throws RemoteException {
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        data.writeInterfaceToken(IActivityManager.descriptor);
        data.writeString(callingPackage);
        data.writeString(basePackage);
        mRemote.transact(GET_APP_AS_STATUS_TRANSACTION, data, reply, 0);
        reply.readException();
        int flag = 0;
        flag = data.readInt();
        data.recycle();
        reply.recycle();
        return flag;
    }

    public List<AppAs.AppAsData> getAppAsList() throws RemoteException {
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        data.writeInterfaceToken(IActivityManager.descriptor);
        mRemote.transact(GET_APP_AS_TRANSACTION, data, reply, 0);
        reply.readException();
        ArrayList<AppAs.AppAsData> mList = null;
        int N = reply.readInt();
        if (N >= 0) {
            mList = new ArrayList<AppAs.AppAsData>();
            while (N > 0) {
                AppAs.AppAsData info =
                        AppAs.AppAsData.CREATOR
                        .createFromParcel(reply);
                mList.add(info);
                N--;
            }
        }
        data.recycle();
        reply.recycle();
        return mList;
    }

    public List<AppAs.AppAsRecord> getAppAsRecordList() throws RemoteException {
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        data.writeInterfaceToken(IActivityManager.descriptor);
        mRemote.transact(GET_APP_AS_RECORD_TRANSACTION, data, reply, 0);
        reply.readException();
        ArrayList<AppAs.AppAsRecord> mList = null;
        int N = reply.readInt();
        if (N >= 0) {
            mList = new ArrayList<AppAs.AppAsRecord>();
            while (N > 0) {
                AppAs.AppAsRecord info =
                        AppAs.AppAsRecord.CREATOR
                        .createFromParcel(reply);
                mList.add(info);
                N--;
            }
        }
        data.recycle();
        reply.recycle();
        return mList;
    }
}
