
package com.android.sprd.telephony;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;

public class RadioInteractorCallbackListener {

    public static final int LISTEN_NONE = 0;
    public static final int LISTEN_RADIOINTERACTOR_EVENT = 0x00000001;
    public static final int LISTEN_RI_CONNECTED_EVENT = 0x00000002;
    public static final int LISTEN_RADIOINTERACTOR_EMBMS_EVENT = 0x00000004;

    final Handler mHandler;
    int mSlotId = -1;

    public RadioInteractorCallbackListener() {
        this(0, Looper.myLooper());
    }

    public RadioInteractorCallbackListener(int slotId) {
        this(slotId, Looper.myLooper());
    }

    public RadioInteractorCallbackListener(int slotId, Looper looper) {
        mSlotId = slotId;
        mHandler = new Handler(looper) {
            public void handleMessage(Message msg) {
                switch (msg.what) {

                    case LISTEN_RADIOINTERACTOR_EVENT:
                        RadioInteractorCallbackListener.this.onRadiointeractorEvent();
                        break;

                    case LISTEN_RI_CONNECTED_EVENT:
                        RadioInteractorCallbackListener.this.onRiConnectedEvent();
                        break;

                    case LISTEN_RADIOINTERACTOR_EMBMS_EVENT:
                        RadioInteractorCallbackListener.this.onRadiointeractorEmbmsEvent();
                        break;

                }
            };
        };
    }

    IRadioInteractorCallback mCallback = new IRadioInteractorCallback.Stub() {

        public void onRadiointeractorEvent() {
            Message.obtain(mHandler, LISTEN_RADIOINTERACTOR_EVENT, 0, 0, null).sendToTarget();
        }

        public void onRadiointeractorEmbmsEvent() {
            Message.obtain(mHandler, LISTEN_RADIOINTERACTOR_EMBMS_EVENT, 0, 0, null).sendToTarget();
        }

    };

    public void onRadiointeractorEvent() {
    }

    public void onRiConnectedEvent() {
    }

    public void onRadiointeractorEmbmsEvent() {
    }

}
