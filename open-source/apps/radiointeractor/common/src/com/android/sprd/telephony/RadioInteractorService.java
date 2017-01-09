
package com.android.sprd.telephony;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.telephony.TelephonyManager;

public abstract class RadioInteractorService extends Service {

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
        RadioInteractorNotifier radioInteractorNotifier = initRadioInteractorNotifier();
        startRILoop(radioInteractorNotifier);
        init(radioInteractorNotifier, mRadioInteractorHandler);
        RadioInteractor.setService(this);
    }

    public void startRILoop(RadioInteractorNotifier radioInteractorNotifier) {
        int numPhones = TelephonyManager.getDefault().getPhoneCount();
        mRadioInteractorHandler = new RadioInteractorHandler[numPhones];
        for (int i = 0; i < numPhones; i++) {
            mRadioInteractorHandler[i] = new RadioInteractorHandler(
                    new RadioInteractorCore(this, i), radioInteractorNotifier);
        }

    }

    public class RadioInteractorBinder extends Binder {
        public RadioInteractorService getService() {
            return RadioInteractorService.this;
        }
    }

    public RadioInteractorHandler getRadioInteractorHandler(int slotId) {
        if (mRadioInteractorHandler != null && slotId < mRadioInteractorHandler.length) {
            return mRadioInteractorHandler[slotId];
        }
        return null;
    }

    public abstract void init(RadioInteractorNotifier radioInteractorNotifier,
            RadioInteractorHandler[] radioInteractorHandler);

    public RadioInteractorNotifier initRadioInteractorNotifier() {
        return new RadioInteractorNotifier();
    }

}
