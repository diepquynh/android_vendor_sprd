package com.android.sprd.telephony;

interface IRadioInteractorlService
{
    int getRemainTimes(int type, int slotId);
    void setSimPower(int onOff, int slotId);
}

