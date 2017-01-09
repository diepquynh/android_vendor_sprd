
package com.sprd.firewall;

import java.util.Date;

import com.android.internal.telephony.BlockChecker;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.os.PowerManager;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.util.Log;

public class FireWallApp extends Application {
    private static final String TAG = "FireWallApp";

    private static final boolean VERBOSE = true;

    private TelephonyManager telMgr;

    static final Object mStartingServiceSync = new Object();

    static PowerManager.WakeLock mStartingService;

    @Override
    public void onCreate() {
        super.onCreate();
        if (VERBOSE)
            Log.v(TAG, "onCreate");
        if (telMgr == null) {
            telMgr = (TelephonyManager) this.getSystemService(Context.TELEPHONY_SERVICE);
        }
    }

    public static void beginStartingService(Context context, Intent intent) {
        if (VERBOSE)
            Log.v(TAG, "beginStartingService");
        synchronized (mStartingServiceSync) {
            if (VERBOSE)
                Log.v(TAG, "mStartingServiceSync");
            if (mStartingService == null) {
                PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
                mStartingService = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
                        "StartingAlertService");
                mStartingService.setReferenceCounted(false);
            }
            mStartingService.acquire();
            if (VERBOSE)
                Log.v(TAG, "startService");
            context.startService(intent);
        }
    }

    public static void finishStartingService(ProcessIncomingService processIncomingService,
            int serviceId) {
        if (VERBOSE)
            Log.v(TAG, "finishStartingService start");
        if (mStartingService != null) {
            if (processIncomingService.stopSelfResult(serviceId)) {
                mStartingService.release();
            }
        }
        if (VERBOSE)
            Log.v(TAG, "finishStartingService finish");
    }
}
