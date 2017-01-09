/*
 * Copyright (C) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Contributed by: Giesecke & Devrient GmbH.
 */

package org.simalliance.openmobileapi.uiccterminal;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.telephony.IccOpenLogicalChannelResponse;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManagerEx;
import android.util.Log;

import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import java.util.MissingResourceException;
import java.util.NoSuchElementException;

import org.simalliance.openmobileapi.internal.ByteArrayConverter;
import org.simalliance.openmobileapi.service.ITerminalService;
import org.simalliance.openmobileapi.service.SmartcardError;
import org.simalliance.openmobileapi.service.OpenLogicalChannelResponse;
import org.simalliance.openmobileapi.internal.Util;

/* SPRD add for SPRD RIL Adaptation. @{*/
import com.android.sprd.telephony.RadioInteractor;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
/*@}*/

public final class UiccTerminal extends Service {

    private static final String TAG = "UiccTerminal";

    public static final String ACTION_SIM_STATE_CHANGED = "org.simalliance.openmobileapi.action.SIM_STATE_CHANGED";

    private TelephonyManager mTelephonyManager;

    //NOTE: Bug#525009 support OMAPI use new method of TelephonyManagerEx to replace hidden method of TelephonyManager BEG -->
    private TelephonyManagerEx mTelephonyManagerEx;
    //<-- support OMAPI use new method of TelephonyManagerEx to replace hidden method of TelephonyManager END

    private List<Integer> mChannelIds;

    private BroadcastReceiver mSimReceiver;

    private String mCurrentSelectedFilePath;

    private byte[] mAtr;

    /* SPRD add for SPRD RIL Adaptation. @{*/
    private RadioInteractor mRadioInteractor;
    /*@}*/

    @Override
    public IBinder onBind(Intent intent) {
        return new TerminalServiceImplementation();
    }

    @Override
    public void onCreate() {
        mTelephonyManager = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
        //NOTE: Bug#525009 support OMAPI use new method of TelephonyManagerEx to replace hidden method of TelephonyManager BEG -->
        mTelephonyManagerEx = (TelephonyManagerEx) getSystemService("phone_ex");
        //<-- support OMAPI use new method of TelephonyManagerEx to replace hidden method of TelephonyManager END
        /* SPRD add for SPRD RIL Adaptation. @{*/
        mRadioInteractor = new RadioInteractor(this);
        /*@}*/
        // Constructor
        mChannelIds = new ArrayList<>();
        // Occupy mChannelIds[0] to avoid return channel number = 0 on openLogicalChannel
        mChannelIds.add(0xFFFFFFFF);
        registerSimStateChangedEvent();
        mCurrentSelectedFilePath = "";
        mAtr = null;
    }

    @Override
    public void onDestroy() {
        unregisterSimStateChangedEvent();
        super.onDestroy();
    }

    /**
     * Performs all the logic for opening a logical channel.
     *
     * @param aid The AID to which the channel shall be opened, empty string to
     * specify "no AID".
     *
     * @return The index of the opened channel ID in the mChannelIds list.
     */
    private OpenLogicalChannelResponse iccOpenLogicalChannel(String aid, byte p2)
            throws NoSuchElementException, MissingResourceException, IOException {
        Log.d(TAG, "iccOpenLogicalChannel > " + aid);
        // Remove any previously stored selection response
        IccOpenLogicalChannelResponse response;
        if (p2 == 0) {
            /* SPRD modify to support multi-slot. Currently it takes the Primary SIM Card. @{
            Original:
            response = mTelephonyManager.iccOpenLogicalcccChannel(aid);
            */
            response = mRadioInteractor.iccOpenLogicalChannel(aid, getIccSlot());
            /*@}*/
        } else {
            /* SPRD add for SPRD RIL Adaptation. @{
            Original:
            response = mTelephonyManager.iccOpenLogicalChannel(aid, p2);
            */
            response = mRadioInteractor.iccOpenLogicalChannel(aid, p2, getIccSlot());
            /*@}*/
        }
        int status = response.getStatus();
        if (status != IccOpenLogicalChannelResponse.STATUS_NO_ERROR) {
            Log.d(TAG, "iccOpenLogicalChannel failed.");
            // An error occured.
            if (status == IccOpenLogicalChannelResponse.STATUS_MISSING_RESOURCE) {
                return null;
            }
            if (status == IccOpenLogicalChannelResponse.STATUS_NO_SUCH_ELEMENT) {
                throw new NoSuchElementException("Applet not found");
            }
            throw new IOException("iccOpenLogicalChannel failed");
        }
        // Operation succeeded
        // Set the select response
        Log.d(TAG, "iccOpenLogicalChannel < "
                        + ByteArrayConverter.byteArrayToHexString(response.getSelectResponse()));
        // Save channel ID. First check if there is any channelID which is empty
        // to reuse it.
        for (int i = 1; i < mChannelIds.size(); i++) {
            if (mChannelIds.get(i) == 0) {
                mChannelIds.set(i, response.getChannel());
                return new OpenLogicalChannelResponse(i, response.getSelectResponse());
            }
        }
        // If no channel ID is empty, append one at the end of the list.
        mChannelIds.add(response.getChannel());
        return new OpenLogicalChannelResponse(mChannelIds.size() - 1, response.getSelectResponse());
    }

    private void registerSimStateChangedEvent() {
        Log.v(TAG, "register to android.intent.action.SIM_STATE_CHANGED event");

        IntentFilter intentFilter = new IntentFilter("android.intent.action.SIM_STATE_CHANGED");
        mSimReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if ("android.intent.action.SIM_STATE_CHANGED".equals(intent.getAction())) {
                    Bundle extras = intent.getExtras();
                    boolean simReady = (extras != null)
                            && "READY".equals(extras.getString("ss"));
                    boolean simLoaded = (extras != null)
                            && "LOADED".equals(extras.getString("ss"));
                    if (simReady || simLoaded) {
                        Log.i(TAG, "SIM is ready or loaded. Checking access rules for updates.");
                        Intent i = new Intent(ACTION_SIM_STATE_CHANGED);
                        sendBroadcast(i);
                    }
                }
            }
        };
        registerReceiver(mSimReceiver, intentFilter);
    }

    private void unregisterSimStateChangedEvent() {
        if (mSimReceiver != null) {
            Log.v(TAG, "unregister SIM_STATE_CHANGED event");
            unregisterReceiver(mSimReceiver);
            mSimReceiver = null;
        }
    }

    /* SPRD add to get the Primary SIM Card. @{*/
    private int getIccSlot() {
        //NOTE: Bug#525009 support OMAPI use new method of TelephonyManagerEx to replace hidden method of TelephonyManager BEG -->
        int slot = mTelephonyManagerEx.getDefaultDataPhoneId();
        //<-- support OMAPI use new method of TelephonyManagerEx to replace hidden method of TelephonyManager END
        Log.v(TAG, "getIccSlot = " + slot);
        return slot;
    }

    /**
     * Returns the response APDU for a command APDU sent through SIM_IO.
     * @param fileID
     * @param command
     * @param p1 P1 value of the APDU command.
     * @param p2 P2 value of the APDU command.
     * @param p3 P3 value of the APDU command.
     * @param filePath
     * @param slotId int
     * @return The APDU response.
     */
    private byte[] iccExchangeSimIOForSlot(int fileID, int command, int p1, int p2, int p3,
            String filePath, int slotId) {
        SubscriptionManager subscriptionManager = SubscriptionManager.from(this);
        SubscriptionInfo subscriptionInfo = subscriptionManager.getActiveSubscriptionInfoForSimSlotIndex(slotId);
        if (subscriptionInfo != null) {
            int subId = subscriptionInfo.getSubscriptionId();
            //NOTE: Bug#525009 support OMAPI use new method of TelephonyManagerEx to replace hidden method of TelephonyManager BEG -->
            return mTelephonyManagerEx.iccExchangeSimIO(subId,fileID,command,p1,p2,p3,filePath);
            //<-- support OMAPI use new method of TelephonyManagerEx to replace hidden method of TelephonyManager END
        }
        return null;
    }
    /*@}*/

    /**
     * The Terminal service interface implementation.
     */
    final class TerminalServiceImplementation extends ITerminalService.Stub {

        @Override
        public OpenLogicalChannelResponse internalOpenLogicalChannel(
                byte[] aid,
                byte p2,
                SmartcardError error) throws RemoteException {
            try {
                return iccOpenLogicalChannel(ByteArrayConverter.byteArrayToHexString(aid), p2);
            } catch (Exception e) {
                Log.e(TAG, "Exception at internalOpenLogicalChannel", e);
                error.set(e);
                return null;
            }

        }

        @Override
        public void internalCloseLogicalChannel(int channelNumber, SmartcardError error)
                throws RemoteException {
            try {
                if (channelNumber == 0) {
                    return;
                }
                if (mChannelIds.get(channelNumber) == 0) {
                    throw new IllegalStateException("Channel not open");
                }
                /* SPRD modify to support multi-slot. Currently it takes the Primary SIM Card. @{
                Original:
                if (!mTelephonyManager.iccCloseLogicalChannel(mChannelIds.get(channelNumber))) {
                */
                if (!mRadioInteractor.iccCloseLogicalChannel(mChannelIds.get(channelNumber), getIccSlot())) {
                /*@}*/
                    throw new IOException("Close channel failed");
                }
                mChannelIds.set(channelNumber, 0);
            } catch (Exception e) {
                Log.e(TAG, "Exception at internalCloseLogicalChannel", e);
                error.set(e);
            }
        }

        @Override
        public byte[] internalTransmit(byte[] command, SmartcardError error) throws RemoteException {
            try {
                Log.d(TAG, "internalTransmit > " + ByteArrayConverter.byteArrayToHexString(command));
                int cla = Util.clearChannelNumber(command[0]) & 0xFF;
                int ins = command[1] & 0xff;
                int p1 = command[2] & 0xff;
                int p2 = command[3] & 0xff;
                int p3 = -1;
                if (command.length > 4) {
                    p3 = command[4] & 0xff;
                }
                String data = null;
                if (command.length > 5) {
                    data = ByteArrayConverter.byteArrayToHexString(command, 5, command.length - 5);
                }

                int channelNumber = Util.parseChannelNumber(command[0]);

                String response;
                if (channelNumber == 0) {
                    /* SPRD modify to support multi-slot. Currently it takes the Primary SIM Card. @{
                    Original:
                    response = mTelephonyManager.iccTransmitApduBasicChannel(cla, ins, p1, p2, p3, data);
                    */
                    response = mRadioInteractor.iccTransmitApduBasicChannel(cla, ins, p1, p2, p3, data, getIccSlot());
                    /*@}*/
                } else {
                    if ((channelNumber > 0) && (mChannelIds.get(channelNumber) == 0)) {
                        throw new IOException("Channel not open");
                    }

                    /* SPRD modify to support multi-slot. Currently it takes the Primary SIM Card. @{
                    Original:
                    response = mTelephonyManager.iccTransmitApduLogicalChannel(
                                mChannelIds.get(channelNumber), cla, ins, p1, p2, p3, data);
                    */
                    response = mRadioInteractor.iccTransmitApduLogicalChannel(
                                mChannelIds.get(channelNumber), cla, ins, p1, p2, p3, data, getIccSlot());
                    /*@}*/
                }
                Log.d(TAG, "internalTransmit < " + response);
                return ByteArrayConverter.hexStringToByteArray(response);
            } catch (Exception e) {
                Log.e(TAG, "Exception at internalTransmit", e);
                error.set(e);
                return null;
            }
        }

        @Override
        public byte[] getAtr() {
            //if (mAtr == null) {
                /* SPRD add for SPRD RIL Adaptation. @{
                Original:
                String atr = mTelephonyManager.iccGetAtr();
                */
                String atr = mRadioInteractor.iccGetAtr(getIccSlot());
                /*@}*/
                Log.d(TAG, "atr = " + (atr == null ? "" : atr));
                if (atr != null && !"".equals(atr)) {
                    mAtr = ByteArrayConverter.hexStringToByteArray(atr);
                }
            //}
            return mAtr;
        }

        @Override
        public boolean isCardPresent() throws RemoteException {
            /* SPRD modify to support multi-slot. Currently it takes the Primary SIM Card. @{
            Original:
            return mTelephonyManager != null && mTelephonyManager.hasIccCard();
            */
            return mTelephonyManager != null && mTelephonyManager.hasIccCard(getIccSlot());
            /*@}*/
        }

        @Override
        public byte[] simIOExchange(int fileID, String filePath, byte[] cmd, SmartcardError error)
                throws RemoteException {
            try {
                int ins;
                int p1 = cmd[2] & 0xff;
                int p2 = cmd[3] & 0xff;
                int p3 = cmd[4] & 0xff;
                switch (cmd[1]) {
                    case (byte) 0xB0:
                        ins = 176;
                        break;
                    case (byte) 0xB2:
                        ins = 178;
                        break;
                    case (byte) 0xA4:
                        ins = 192;
                        p1 = 0;
                        p2 = 0;
                        p3 = 15;
                        break;
                    default:
                        throw new IOException("Unknown SIM_IO command");
                }

                if (filePath != null && filePath.length() > 0) {
                    mCurrentSelectedFilePath = filePath;
                }

                /* SPRD modify to support multi-slot. Currently it takes the Primary SIM Card. @{
                Original:
                return mTelephonyManager.iccExchangeSimIO(
                        fileID, ins, p1, p2, p3, mCurrentSelectedFilePath);
                */
                return iccExchangeSimIOForSlot(
                        fileID, ins, p1, p2, p3, mCurrentSelectedFilePath, getIccSlot());
                /*@}*/
            } catch (Exception e) {
                Log.e(TAG, "Exception at simIOExchange", e);
                error.set(e);
                return null;
            }
        }

        @Override
        public String getSeStateChangedAction() {
            return ACTION_SIM_STATE_CHANGED;
        }
    }
}
