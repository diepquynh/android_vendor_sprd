package com.android.phone;

import com.android.internal.telephony.TelephonyIntentsEx;
import static com.android.internal.telephony.TelephonyIntents.SECRET_CODE_ACTION;

import android.content.Context;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.net.Uri;
import android.os.SystemProperties;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManagerEx;
import android.util.Log;
import android.widget.Toast;

import com.android.phone.R;
import com.android.sprd.telephony.RadioInteractor;
import com.android.sprd.telephony.uicc.IccCardStatusEx;

public class ShowSimLockBroadcastReceiver extends BroadcastReceiver {
    private static final String TAG = "ShowSimLockBroadcastReceiver";

    public ShowSimLockBroadcastReceiver() {
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String host = null;
        Uri uri = intent.getData();
        if (uri != null) {
            host = uri.getHost();
        } else {
            Log.d(TAG,"uri is null");
            return;
        }
        if ("0808".equals(host)){
            handleShowSimlockUnlockBySim(context);
        } else if ("54321".equals(host)) {
            handleOnekeyUnlock(context);
        } else if ("2413".equals(host)) {
            handleShowSimlockUnlockByNv(context);
        } else {
            Log.d(TAG, "Unhandle host[" + host + "]");
        }
    }

    private void handleShowSimlockUnlockBySim(Context context) {
        if (SystemProperties.getBoolean("ro.simlock.unlock.autoshow", true)) {
            Log.d(TAG, "Return for autoshow is turned on.");
            return;
        } else {
            TelephonyManager tm = (TelephonyManager)TelephonyManager.from(context);
            if (tm != null) {
                int simlockSlotFlag = 0;
                int phoneCount = tm.getPhoneCount();
                for (int i = 0; i < phoneCount; i++) {
                    if (TelephonyManagerEx.checkSimlockLocked(i)) {
                        simlockSlotFlag |= (1 << i);
                    }
                }
                Log.d(TAG, "simlockSlotFlag = " + simlockSlotFlag);
                if (simlockSlotFlag != 0) {
                    Intent intent = new Intent(TelephonyIntentsEx.SHOW_SIMLOCK_UNLOCK_SCREEN_ACTION);
                    intent.putExtra(TelephonyIntentsEx.EXTRA_SIMLOCK_UNLOCK, simlockSlotFlag);
                    context.sendBroadcast(intent);
                } else {
                    Toast.makeText(context,
                            context.getString(R.string.sim_lock_none),
                            Toast.LENGTH_LONG).show();
                }
            } else {
                    Toast.makeText(context,
                            context.getString(R.string.sim_lock_try_later),
                            Toast.LENGTH_LONG).show();
            }
        }
    }

    private void handleOnekeyUnlock(Context context) {
        if(!SystemProperties.getBoolean("ro.simlock.onekey.lock", false)) {
            Log.d(TAG, "Return for onekey unlock is turn off.");
            return;
        }
        Intent intentForSimLock = new Intent();
        intentForSimLock.setClass(context, ChooseSimLockTypeActivity.class);
        intentForSimLock.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(intentForSimLock);
        Log.d(TAG,"handleSimNetworkLock");
    }

    private void handleShowSimlockUnlockByNv(Context context) {
        if (!SystemProperties.getBoolean("ro.simlock.unlock.bynv", false)) {
            Log.d(TAG, "Return for showing unlock screen by nv is turned off.");
            return;
        } else {
            RadioInteractor radioInteractor = new RadioInteractor(context);
            int isNetworkLock = radioInteractor.getSimLockStatus(IccCardStatusEx.UNLOCK_NETWORK, 0);
            int isNetworkSubsetLock = radioInteractor.getSimLockStatus(IccCardStatusEx.UNLOCK_NETWORK_SUBSET, 0);
            int isServiceProviderLock = radioInteractor.getSimLockStatus(IccCardStatusEx.UNLOCK_SERVICE_PORIVDER, 0);
            int isCorporateLock = radioInteractor.getSimLockStatus(IccCardStatusEx.UNLOCK_CORPORATE, 0);
            int isSimLock = radioInteractor.getSimLockStatus(IccCardStatusEx.UNLOCK_SIM, 0);

            if (isNetworkLock > 0 || isNetworkSubsetLock > 0 || isServiceProviderLock > 0
                    || isCorporateLock > 0 || isSimLock > 0) {
                Intent intent = new Intent(TelephonyIntentsEx.SHOW_SIMLOCK_UNLOCK_SCREEN_BYNV_ACTION);
                context.sendBroadcast(intent);
                Log.d(TAG, "handleShowSimlockUnlockByNv has sent broadcast.");
            } else if (isNetworkLock == 0 && isNetworkSubsetLock == 0 && isServiceProviderLock == 0
                     && isCorporateLock == 0 && isSimLock == 0 ) {
                Toast.makeText(context,
                        context.getString(R.string.simlock_unlocked),
                        Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(context,
                        context.getString(R.string.sim_lock_try_later),
                        Toast.LENGTH_LONG).show();
            }
        }
    }
}
