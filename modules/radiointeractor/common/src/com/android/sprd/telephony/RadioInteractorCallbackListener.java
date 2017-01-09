
package com.android.sprd.telephony;

import com.android.sprd.telephony.RadioInteractorCore.SuppService;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.AsyncResult;

public class RadioInteractorCallbackListener {

    public static final int LISTEN_NONE = 0;
    public static final int LISTEN_RADIOINTERACTOR_EVENT = 0x00000001;
    public static final int LISTEN_RI_CONNECTED_EVENT = 0x00000002;
    public static final int LISTEN_RADIOINTERACTOR_EMBMS_EVENT = 0x00000004;
    public static final int LISTEN_VIDEOPHONE_CODEC_EVENT = 0x00000008;
    public static final int LISTEN_VIDEOPHONE_DSCI_EVENT = 0x00000010;
    public static final int LISTEN_VIDEOPHONE_STRING_EVENT = 0x00000020;
    public static final int LISTEN_VIDEOPHONE_REMOTE_MEDIA_EVENT = 0x00000040;
    public static final int LISTEN_VIDEOPHONE_MM_RING_EVENT = 0x00000080;
    public static final int LISTEN_VIDEOPHONE_RELEASING_EVENT = 0x00000100;
    public static final int LISTEN_VIDEOPHONE_RECORD_VIDEO_EVENT = 0x00000200;
    public static final int LISTEN_VIDEOPHONE_MEDIA_START_EVENT = 0x00000400;
    public static final int LISTEN_SUPP_SERVICE_FAILED_EVENT = 0x00000800;
    public static final int LISTEN_ECC_NETWORK_CHANGED_EVENT = 0x00001000;
    public static final int LISTEN_RAU_SUCCESS_EVENT = 0x00002000;
    public static final int LISTEN_CLEAR_CODE_FALLBACK_EVENT = 0x00004000;
    public static final int LISTEN_SIMLOCK_NOTIFY_EVENT = 0x00008000;
    public static final int LISTEN_BAND_INFO_EVENT = 0x00010000;

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
                    case LISTEN_SUPP_SERVICE_FAILED_EVENT:
                        RadioInteractorCallbackListener.this.onSuppServiceFailedEvent(msg.arg1);

                    case LISTEN_VIDEOPHONE_CODEC_EVENT:
                        RadioInteractorCallbackListener.this.onVideoPhoneCodecEvent(msg.obj);
                        break;

                    case LISTEN_VIDEOPHONE_DSCI_EVENT:
                        RadioInteractorCallbackListener.this.onVideoPhoneDsciEvent(msg.obj);
                        break;

                    case LISTEN_VIDEOPHONE_STRING_EVENT:
                        RadioInteractorCallbackListener.this.onVideoPhoneStringEvent(msg.obj);
                        break;

                    case LISTEN_VIDEOPHONE_REMOTE_MEDIA_EVENT:
                        RadioInteractorCallbackListener.this.onVideoPhoneRemoteMediaEvent(msg.obj);
                        break;

                    case LISTEN_VIDEOPHONE_MM_RING_EVENT:
                        RadioInteractorCallbackListener.this.onVideoPhoneMMRingEvent(msg.obj);
                        break;

                    case LISTEN_VIDEOPHONE_RELEASING_EVENT:
                        RadioInteractorCallbackListener.this.onVideoPhoneReleasingEvent(msg.obj);
                        break;

                    case LISTEN_VIDEOPHONE_RECORD_VIDEO_EVENT:
                        RadioInteractorCallbackListener.this.onVideoPhoneRecordVideoEvent(msg.obj);
                        break;

                    case LISTEN_VIDEOPHONE_MEDIA_START_EVENT:
                        RadioInteractorCallbackListener.this.onVideoPhoneMediaStartEvent(msg.obj);
                        break;

                    case LISTEN_ECC_NETWORK_CHANGED_EVENT:
                        RadioInteractorCallbackListener.this.onEccNetChangedEvent(msg.obj);
                        break;

                    case LISTEN_RAU_SUCCESS_EVENT:
                        RadioInteractorCallbackListener.this.onRauSuccessEvent();
                        break;

                    case LISTEN_CLEAR_CODE_FALLBACK_EVENT:
                        RadioInteractorCallbackListener.this.onClearCodeFallbackEvent();
                        break;

                    case LISTEN_SIMLOCK_NOTIFY_EVENT:
                        RadioInteractorCallbackListener.this.onSimLockNotifyEvent(msg);
                        break;

                    case LISTEN_BAND_INFO_EVENT:
                        RadioInteractorCallbackListener.this.onbandInfoEvent(msg.obj);
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

        public void onSuppServiceFailedEvent(int service) {
            Message.obtain(mHandler, LISTEN_SUPP_SERVICE_FAILED_EVENT, service, 0, null).sendToTarget();
        }

        public void onbandInfoEvent(String info) {
            AsyncResult ret = new AsyncResult (null, info, null);
            Message.obtain(mHandler, LISTEN_BAND_INFO_EVENT, 0, 0, ret).sendToTarget();
        }
    };

    public void onRadiointeractorEvent() {
    }

    public void onRiConnectedEvent() {
    }

    public void onRadiointeractorEmbmsEvent() {
    }

    public void onSuppServiceFailedEvent(int service) {
    }

    public void onVideoPhoneCodecEvent(Object object){
    }

    public void onVideoPhoneDsciEvent(Object object){
    }

    public void onVideoPhoneStringEvent(Object object){
    }

    public void onVideoPhoneRemoteMediaEvent(Object object){
    }

    public void onVideoPhoneMMRingEvent(Object object){
    }

    public void onVideoPhoneReleasingEvent(Object object){
    }

    public void onVideoPhoneRecordVideoEvent(Object object){
    }

    public void onVideoPhoneMediaStartEvent(Object object){
    }

    public void onEccNetChangedEvent(Object object){
    }

    public void onRauSuccessEvent() {
    }

    public void onClearCodeFallbackEvent() {
    }

    public void onSimLockNotifyEvent(Message msg){
    }

    public void onbandInfoEvent(Object object) {
    }
}
