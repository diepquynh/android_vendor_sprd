
package com.android.sprd.telephony;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.telephony.TelephonyManager;
import android.content.Context;

public class RadioInteractorService extends Service {

    public static final String TAG = "RadioInteractorService";

    private RadioInteractorHandler[] mRadioInteractorHandler;

    @Override
    public IBinder onBind(Intent arg0) {
        UtilLog.logd(TAG, "onBind onBind onBind......");
        return new RadioInteractorBinder();
    }

    @Override
    public void onCreate() {
        super.onCreate();
        UtilLog.logd(TAG, " onCreate....." + android.os.Process.myPid());
        RadioInteractorFactory.init(this);
    }

    public class RadioInteractorBinder extends Binder {
        public RadioInteractorService getService() {
            return RadioInteractorService.this;
        }
    }

}
