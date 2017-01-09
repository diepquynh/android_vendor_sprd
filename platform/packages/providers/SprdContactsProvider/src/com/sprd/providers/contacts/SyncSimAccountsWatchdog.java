
package com.sprd.providers.contacts;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.os.UserHandle;
import android.provider.Telephony;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.os.Handler;

import com.android.internal.telephony.IccCard;
import com.android.internal.telephony.TelephonyIntents;

/**
 * SPRD:
 *
 * @{
 */

class UserSwicthReceiver extends BroadcastReceiver {

    private static final String TAG = UserSwicthReceiver.class.getSimpleName();

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent != null && intent.getAction().equals(Intent.ACTION_USER_SWITCHED)) {
            Log.e(TAG, "ACTION_USER_SWITCHED");
            int mCurrentUserId = intent.getIntExtra(Intent.EXTRA_USER_HANDLE, -1);
            Intent intent2 = new Intent("sync_sim_fake_boot_completed");
            intent2.putExtra(Intent.EXTRA_USER_HANDLE, mCurrentUserId);
            context.sendBroadcast(intent2);
        }
    }
}

public class SyncSimAccountsWatchdog extends Service {

    private static final String TAG = SyncSimAccountsWatchdog.class.getSimpleName();
    UserSwicthReceiver mReceiver = null;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        mReceiver = new UserSwicthReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_USER_SWITCHED);
        getApplicationContext().registerReceiver(mReceiver, filter);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        //if (intent == null) {
            bootstrap();
        //}
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        if(mReceiver != null) {
            getApplicationContext().unregisterReceiver(mReceiver);
        }
    }

    private void bootstrap() {
        Log.e(TAG, "SyncSimAccountsWatchdog: bootstrap myId:" + UserHandle.myUserId());
        /**
         * Sprd: add account while not receive ICC_LOADED original :sendBroadcast(new
         * Intent("sync_sim_fake_boot_completed"));
         *
         * @{
         */
        Handler mhandler = new Handler();
        mhandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                sendBroadcast(new Intent("sync_sim_fake_boot_completed"));
            }
        }, 500);
        /**
         * @}
         */
    }
}
/**
 * @}
 */
