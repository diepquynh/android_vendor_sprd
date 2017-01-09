package com.android.phone;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.telephony.TelephonyManager;
import android.util.Log;
import com.android.sprd.telephony.RadioInteractor;

/** Helper to support fast shutdown */
public class FastShutdownHelper extends BroadcastReceiver {
    private static final String TAG = "FastShutdownHelper";
    private static final int SHUTDOWN_TIMEOUT = 5 * 1000;
    private static final String RI_SERVICE_NAME =
            "com.android.sprd.telephony.server.RADIOINTERACTOR_SERVICE";
    private static final String RI_SERVICE_PACKAGE =
            "com.android.sprd.telephony.server";
    private static FastShutdownHelper sInstance;
    private Context mContext;
    private RadioInteractor mRi;

    private ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mRi = new RadioInteractor(mContext);
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            mRi = null;
        }
    };

    public FastShutdownHelper(Context context) {
        mContext = context;
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SHUTDOWN);
        context.registerReceiver(this, filter);

        Intent serviceIntent = new Intent(RI_SERVICE_NAME);
        serviceIntent.setPackage(RI_SERVICE_PACKAGE);
        context.bindService(serviceIntent, mConnection, Context.BIND_AUTO_CREATE);
    }

    public static void init(Context context) {
        if (sInstance == null) {
            sInstance = new FastShutdownHelper(context);
        } else {
            Log.d(TAG, "FastShutdownHelper.init() called multiple times");
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "onReceive() action=" + intent.getAction());
        final PendingResult result = goAsync();

        new Thread() {
            public void run() {
                boolean ok = shutdownRadios();

                if (!ok) {
                    try {
                        sleep(SHUTDOWN_TIMEOUT);
                    } catch (InterruptedException e) {}
                }

                result.finish();
            }
        }.start();
    }

    private boolean shutdownRadios() {
        int numPhones = TelephonyManager.getDefault().getPhoneCount();
        boolean result = true;

        if (mRi != null) {
            for (int i = 0; i < numPhones; i++) {
                result = result && mRi.requestShutdown(i);
            }
        } else {
            Log.d(TAG, "shutdownRadios() befor radio interactor is started");
            // RadioInteractor is not started yet, immediatley finish the broadcast
            return true;
        }

        Log.d(TAG, "shutdownRadios() result=" + result);
        return result;
    }
}
