package com.android.sprd.telephony;

interface IRadioInteractorCallback
{
    void onRadiointeractorEvent();
    void onRadiointeractorEmbmsEvent();
    void onSuppServiceFailedEvent(int service);
    void onbandInfoEvent(String info);
}
