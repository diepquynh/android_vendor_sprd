
package com.android.sprd.telephony;

import android.app.Service;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.telephony.TelephonyManager;

public class RadioInteractorFactory{

    public static final String TAG = "RadioInteractorFactory";
    private static RadioInteractorFactory sInstance;
    Context mContext;

    private RadioInteractorHandler[] mRadioInteractorHandler;

    public static RadioInteractorFactory init(Context context) {
        synchronized (RadioInteractorFactory.class) {
            if (sInstance == null) {
                sInstance = new RadioInteractorFactory(context);
            } else {
                UtilLog.loge("RadioInteractorFactory", "init() called multiple times!  sInstance = "
                        + sInstance);
            }
            return sInstance;
        }
    }

    public RadioInteractorFactory(Context context) {
        mContext = context;
        RadioInteractorNotifier radioInteractorNotifier = initRadioInteractorNotifier();
        startRILoop(radioInteractorNotifier);
        RadioInteractorProxy.init(context,radioInteractorNotifier, mRadioInteractorHandler);
    }

    public static RadioInteractorFactory getInstance() {
        return sInstance;
    }

    public void startRILoop(RadioInteractorNotifier radioInteractorNotifier) {
        int numPhones = TelephonyManager.getDefault().getPhoneCount();
        mRadioInteractorHandler = new RadioInteractorHandler[numPhones];
        for (int i = 0; i < numPhones; i++) {
            mRadioInteractorHandler[i] = new RadioInteractorHandler(
                    new RadioInteractorCore(mContext, i), radioInteractorNotifier,mContext);
         }
    }

    public RadioInteractorHandler getRadioInteractorHandler(int slotId) {
        if (mRadioInteractorHandler != null && slotId < mRadioInteractorHandler.length) {
            return mRadioInteractorHandler[slotId];
        }
        return null;
    }

    public RadioInteractorNotifier initRadioInteractorNotifier() {
        return new RadioInteractorNotifier();
    }
}
