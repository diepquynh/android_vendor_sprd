package com.sprd.gallery3d.app;

import java.util.List;

import com.android.gallery3d.app.FloatPlayerService;
import com.android.gallery3d.ui.Log;
import com.sprd.gallery3d.aidl.IFloatWindowController;

import android.app.ActivityManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;

public class FloatWindowAIDLService extends Service {
    private static final String TAG = "FloatWindowAIDLService";

    IFloatWindowController.Stub stub = new IFloatWindowController.Stub() {

        @Override
        public boolean closeFloatWindow() throws RemoteException {
            // TODO Auto-generated method stub
            if (isServiceRunning(FloatWindowAIDLService.this,
                    FloatPlayerService.class.getName())) {
                Intent intent = new Intent(FloatWindowAIDLService.this,
                FloatPlayerService.class);
                stopService(intent);
                return true;
            }
            return false;
       }
    };

    @Override
    public void onCreate() {
        // TODO Auto-generated method stub
        Log.d(TAG, "onCreate()");
        super.onCreate();
    }

    @Override
    public IBinder onBind(Intent arg0) {
        // TODO Auto-generated method stub
        Log.d(TAG, "onBind()");
        return stub;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // TODO Auto-generated method stub
        Log.d(TAG, "onStartCommand()");
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public boolean onUnbind(Intent intent) {
        // TODO Auto-generated method stub
        Log.d(TAG, "onUnbind()");
        return super.onUnbind(intent);
    }

    @Override
    public void onDestroy() {
        // TODO Auto-generated method stub
        Log.d(TAG, "onDestroy()");
        super.onDestroy();
    }

    public static boolean isServiceRunning(Context mContext, String className) {
        boolean isRunning = false;
        ActivityManager activityManager = (ActivityManager) mContext
                .getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningServiceInfo> serviceList = activityManager
                .getRunningServices(200);
        if(serviceList != null){
            if (!(serviceList.size() > 0)) {
                 return false;
            }
            for (int i = 0; i < serviceList.size(); i++) {
                    if (className.equals(serviceList.get(i).service.getClassName()) == true) {
                       isRunning = true;
                       break;
                    }
            }
        }
        return isRunning;
    }
}
