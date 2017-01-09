/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.android.internal.telephony.gsm;

import android.content.Context;

import com.android.ims.ImsManager;
import com.android.internal.telephony.*;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppState;
import com.android.internal.util.ArrayUtils;

import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.ConnectivityManager.NetworkCallback;
import android.os.*;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.telephony.Rlog;
import com.android.sprd.telephony.RadioInteractor;


/**
 * The motto for this file is:
 * "NOTE:    By using the # as a separator, most cases are expected to be unambiguous." -- TS 22.030
 * 6.5.2 {@hide}
 */
public final class GsmMmiCodeEx extends GsmMmiCode {
    static final String LOG_TAG = "GsmMmiCodeEx";
    static final String SC_COLP = "76";
    static final String SC_COLR = "77";
    public static final int UNLOCK_PIN = 0;
    public static final int UNLOCK_PIN2 = 1;
    public static final int UNLOCK_PUK = 2;
    public static final int UNLOCK_PUK2 = 3;

    static final int EVENT_QUERY_LI_COMPLETE = 8;
    /* SPRD: Bug 597510 Read correct remain times @{ */
    private static final String[] PROPERTY_REMAINTIMES = {
        "gsm.sim.pin.remaintimes",
        "gsm.sim.pin2.remaintimes",
        "gsm.sim.puk.remaintimes",
        "gsm.sim.puk2.remaintimes"
    };
    /* @} */
    private static final int MIN_REMAINTIMES = 0;
/*    byte OEM_REQ_NO_SUBFUNCID = 0;
    byte OEM_REQ_FUNCID_QUERY_COLP_COLR = 0x05;
    byte OEM_REQ_SUBFUNCID_QUERY_COLP = 0x01;
    byte OEM_REQ_SUBFUNCID_QUERY_COLR = 0x02;
    byte OEM_REQ_FUNCID_MMI_ENTER_SIM = 0x06;*/
    RadioInteractor mRi;
    private static final int QUERY_LI_RESPONSE_ERROR = -1;
    private static final int QUERY_LI_RESPONSE_DISABLE = 0;
    private static final int QUERY_LI_RESPONSE_ENABLE = 1;
    private static final int QUERY_LI_FOR_SUPP_PWD_REG = 100;
    private static final int QUERY_LI_SUPP_PWD_REG_SUCCESS = 0;
    private static final int QUERY_LI_SUPP_PWD_REG_FAIL = 1;
    /* SPRD: modify for bug627022 @{ */
    private static final String[] SUPPLEMENTARY_SERVICE = {
        // Called line presentation
        SC_CLIP,
        SC_CLIR,
        SC_COLP,
        SC_COLR,
        // Call Forwarding
        SC_CFU,
        SC_CFB,
        SC_CFNRy,
        SC_CFNR,
        SC_CF_All,
        SC_CF_All_Conditional,
        // Call Waiting
        SC_WAIT,
        // Call Barring
        SC_BAOC,
        SC_BAOIC,
        SC_BAOICxH,
        SC_BAIC,
        SC_BAICr,
        SC_BA_ALL,
        SC_BA_MO,
        SC_BA_MT
    };
    private static final int NETWORK_REQUEST_TIMEOUT_MILLIS = 200 * 1000;
    private NetworkCallback mNetworkRequestCallback;
    private ConnectivityManager mConnectivityManager;
    private boolean isNetWorkRequestFirst = true;
    /* @} */

    public GsmMmiCodeEx(GsmCdmaPhone phone, UiccCardApplication app) {
        super(phone, app);
        mRi = new RadioInteractor(mPhone.getContext());
    }

    /** Process a MMI code or short code...anything that isn't a dialing number */
    @Override
    public void processCode() throws CallStateException {
            /* SPRD： modify for bug627022 @{ */
            if (mSc != null) {
                Rlog.d(LOG_TAG, "isVolteEnable:"+ImsManager.isVolteEnabledByPlatform(mContext)
                        +"isEnahnce:"+ImsManager.isEnhanced4gLteModeSettingEnabledByUser(mContext)
                        +"is primary card? " + (SubscriptionManager.from(mContext)
                        .getDefaultDataSubscriptionId() == mPhone.getSubId())
                        + "  defaultdata:" + SubscriptionManager.from(mContext)
                                .getDefaultDataSubscriptionId()
                        + "  nowSubId:" + mPhone.getSubId());
            }
            if (mSc != null
                    && ImsManager.isVolteEnabledByPlatform(mContext)
                    && ImsManager.isEnhanced4gLteModeSettingEnabledByUser(mContext)
                    && SubscriptionManager.from(mContext)
                            .getDefaultDataSubscriptionId() == mPhone.getSubId()
                    && ArrayUtils.contains(SUPPLEMENTARY_SERVICE, mSc)) {
                Rlog.d(LOG_TAG, "open xcap for ss");
                mConnectivityManager = (ConnectivityManager) mContext.getSystemService(
                        Context.CONNECTIVITY_SERVICE);
                if (mNetworkRequestCallback == null) {
                    mNetworkRequestCallback = new NetworkRequestCallback();
                }
                isNetWorkRequestFirst = true;
                requestOpenNetWork();
                /* @} */
            } else {
                processCodeAfterRequest();
            }
        }

    public void processCodeAfterRequest() throws CallStateException {
        try {
            if (mSc != null && mSc.equals(SC_COLP)) {
                Rlog.d(LOG_TAG, "is COLP");
                if (isInterrogate()) {
                    /* SPRD: modify for bug641723 @{ */
                    /*byte[] data = getRequestRawBytes(
                            OEM_REQ_FUNCID_QUERY_COLP_COLR,
                            OEM_REQ_SUBFUNCID_QUERY_COLP);
                    ((CommandsInterface) (mPhone.mCi)).invokeOemRilRequestRaw(
                            data, obtainMessage(EVENT_QUERY_LI_COMPLETE, this));*/
                    int returnValue = mRi.queryColp(mPhone.getPhoneId());
                    Message msg = Message.obtain(this, EVENT_QUERY_LI_COMPLETE);
                    msg.arg1 = returnValue;
                    msg.sendToTarget();
                    /* @} */
                } else {
                    throw new RuntimeException("Invalid or Unsupported MMI Code");
                }

            } else if (mSc != null && mSc.equals(SC_COLR)) {
                Rlog.d(LOG_TAG, "is COLR");
                if (isInterrogate()) {
                    /* SPRD: modify for bug641723 @{ */
                    /*byte[] data = getRequestRawBytes(OEM_REQ_FUNCID_QUERY_COLP_COLR,
                            OEM_REQ_SUBFUNCID_QUERY_COLR);
                    ((CommandsInterface) (mPhone.mCi)).invokeOemRilRequestRaw(data,
                            obtainMessage(EVENT_QUERY_LI_COMPLETE, this));*/
                    int returnValue = mRi.queryColr(mPhone.getPhoneId());
                    Message msg = Message.obtain(this, EVENT_QUERY_LI_COMPLETE);
                    msg.arg1 = returnValue;
                    msg.sendToTarget();
                    /* @} */
                } else {
                    throw new RuntimeException("Invalid or Unsupported MMI Code");
                }
            }
            else if (mSc != null && mSc.equals(SC_PWD)) {
                Rlog.d(LOG_TAG, "Supp Service Password registration");
                // sia = fac
                // sib = old pwd
                // sic = new pwd
                // pwd = new pwd
                String facility;
                if (isActivate() || isRegister()) {
                    // Even though ACTIVATE is acceptable, this is really termed a REGISTER
                    mAction = ACTION_REGISTER;

                    if (mSia == null) {
                        // If sc was not specified, treat it as BA_ALL.
                        facility = CommandsInterface.CB_FACILITY_BA_ALL;
                    } else {
                        facility = scToBarringFacility(mSia);
                    }
                    /**
                     * SPRD: porting ussd feature for bug475740 @{ Not send cm_service_req
                     * @orig if (newPwd.equals(mPwd)) { mPhone.mCi.changeBarringPassword(facility,
                     *       oldPwd, newPwd, obtainMessage(EVENT_SET_COMPLETE, this)); } else { //
                     *       password mismatch; return error
                     *       handlePasswordError(com.android.internal.R.string.passwordIncorrect); }
                     */
                    /* SPRD: modify for bug641723 @{ */
                    /*byte[] data = getStringRequestRawBytes(mPoundString,
                            OEM_REQ_FUNCID_MMI_ENTER_SIM, OEM_REQ_NO_SUBFUNCID);
                    ((CommandsInterface) (mPhone.mCi)).invokeOemRilRequestRaw(data,
                            obtainMessage(EVENT_QUERY_LI_COMPLETE, this));*/
                    int returnValue = mRi.mmiEnterSim(mPoundString, mPhone.getPhoneId());
                    Message msg = Message.obtain(this, EVENT_QUERY_LI_COMPLETE);
                    msg.arg1 = returnValue;
                    msg.arg2 = QUERY_LI_FOR_SUPP_PWD_REG;
                    msg.sendToTarget();
                    /* @} */
                    /** @} */
                } else {
                    throw new RuntimeException("Invalid or Unsupported MMI Code");
                }

            } else if (isPinPukCommand()) {
                // TODO: This is the same as the code in CmdaMmiCode.java,
                // MmiCode should be an abstract or base class and this and
                // other common variables and code should be promoted.

                // sia = old PIN or PUK
                // sib = new PIN
                // sic = new PIN
                String oldPinOrPuk = mSia;
                String newPinOrPuk = mSib;
                int pinLen = newPinOrPuk.length();
                if (isRegister()) {
                    if (!newPinOrPuk.equals(mSic)) {
                        // password mismatch; return error
                        /**
                         * SPRD: porting ussd feature for bug475740 @{
                         * @orig handlePasswordError(com.android.internal.R.string.mismatchPin);
                         */
                        if (mSc.equals(SC_PIN)) {
                            handlePasswordError(com.android.internal.R.string.mismatchPin);
                        } else if (mSc.equals(SC_PIN2)) {
                            handlePasswordError(com.android.internal.R.string.mismatchPin2);
                        } else if (mSc.equals(SC_PUK)) {
                            // SPRD: modify for bug600635
                            handlePasswordError(com.android.internal.R.string.mismatchPin);
                        } else if (mSc.equals(SC_PUK2)) {
                            handlePasswordError(com.android.internal.R.string.mismatchPin2);
                        }
                        /** @} */
                    } else if (pinLen < 4 || pinLen > 8) {
                        // invalid length
                        handlePasswordError(com.android.internal.R.string.invalidPin);
                    } else if (mSc.equals(SC_PIN)
                            && mUiccApplication != null
                            && mUiccApplication.getState() == AppState.APPSTATE_PUK) {
                        // Sim is puk-locked
                        handlePasswordError(com.android.internal.R.string.needPuk);
                    } else if (mUiccApplication != null) {
                        Rlog.d(LOG_TAG, "process mmi service code using UiccApp sc=" + mSc);

                        // We have an app and the pre-checks are OK
                        if (mSc.equals(SC_PIN)) {
                            /**
                             * SPRD: porting ussd feature for bug475740 @{
                             * @orig mUiccApplication.changeIccLockPassword(oldPinOrPuk,
                             *       newPinOrPuk, obtainMessage(EVENT_SET_COMPLETE, this));
                             */
                            if (oldPinOrPuk.length() < 4 || oldPinOrPuk.length() > 8) {
                                handlePasswordError(com.android.internal.R.string.invalidPin);
                                return;
                            }
                            if (mUiccApplication.getIccLockEnabled()) {
                                mUiccApplication.changeIccLockPassword(oldPinOrPuk, newPinOrPuk,
                                        obtainMessage(EVENT_SET_COMPLETE, this));
                            } else {
                                handlePasswordError(com.android.internal.R.string.enablePin);
                            }
                            /** @} */
                        } else if (mSc.equals(SC_PIN2)) {
                            /* SPRD: add for bug514467 @{ */
                            if (oldPinOrPuk.length() < 4 || oldPinOrPuk.length() > 8) {
                                handlePasswordError(com.android.internal.R.string.invalidPin);
                                return;
                            }
                            /* @} */
                            mUiccApplication.changeIccFdnPassword(oldPinOrPuk, newPinOrPuk,
                                    obtainMessage(EVENT_SET_COMPLETE, this));
                        } else if (mSc.equals(SC_PUK)) {
                            /* SPRD: add for bug494346 @{ */
                            if (oldPinOrPuk.length() != 8) {
                                handlePasswordError(com.android.internal.R.string.invalidPuk);
                                return;
                            }
                            /* @} */
                            mUiccApplication.supplyPuk(oldPinOrPuk, newPinOrPuk,
                                    obtainMessage(EVENT_SET_COMPLETE, this));
                        } else if (mSc.equals(SC_PUK2)) {
                            /* SPRD: add for bug514467 @{ */
                            if (oldPinOrPuk.length() != 8) {
                                handlePasswordError(com.android.internal.R.string.puklengthlimit);
                                return;
                            }
                            /* @} */
                            mUiccApplication.supplyPuk2(oldPinOrPuk, newPinOrPuk,
                                    obtainMessage(EVENT_SET_COMPLETE, this));
                        } else {
                            throw new RuntimeException("uicc unsupported service code=" + mSc);
                        }
                    } else {
                        throw new RuntimeException("No application mUiccApplicaiton is null");
                    }
                } else {
                    throw new RuntimeException("Ivalid register/action=" + mAction);
                }
            } else {
                super.processCode();
            }
        } catch (RuntimeException exc) {
            Rlog.d(LOG_TAG, "RuntimeException " + exc);
            mState = State.FAILED;
            mMessage = mContext.getText(com.android.internal.R.string.mmiError);
            mPhone.onMMIDone(this);
        }
    }

    private void handlePasswordError(int res) {
        mState = State.FAILED;
        StringBuilder sb = new StringBuilder(getScString());
        sb.append("\n");
        sb.append(mContext.getText(res));
        mMessage = sb;
        mPhone.onMMIDone(this);
    }

    @Override
    public void
            handleMessage(Message msg) {
        AsyncResult ar;

        switch (msg.what) {
        /* SPRD: porting ussd feature for bug475740 @{ */
            case EVENT_QUERY_LI_COMPLETE:
                // SRPD: modify for bug641723
                onQueryLiComplete(msg);
                break;
            /* @} */
            case EVENT_SET_COMPLETE:
                ar = (AsyncResult) (msg.obj);

                onSetComplete(msg, ar);
                break;

            case EVENT_SET_CFF_COMPLETE:
                ar = (AsyncResult) (msg.obj);

                /*
                * msg.arg1 = 1 means to set unconditional voice call forwarding
                * msg.arg2 = 1 means to enable voice call forwarding
                */
                if ((ar.exception == null) && (msg.arg1 == 1)) {
                    boolean cffEnabled = (msg.arg2 == 1);
                    if (mIccRecords != null) {
                        mPhone.setVoiceCallForwardingFlag(1, cffEnabled, mDialingNumber);
                    }
                }

                onSetComplete(msg, ar);
                break;
            default:
                super.handleMessage(msg);
        }
    }

    private CharSequence getErrorMessage(AsyncResult ar) {

        if (ar.exception instanceof CommandException) {
            CommandException.Error err = ((CommandException) (ar.exception)).getCommandError();
            if (err == CommandException.Error.FDN_CHECK_FAILURE) {
                Rlog.i(LOG_TAG, "FDN_CHECK_FAILURE");
                return mContext.getText(com.android.internal.R.string.mmiFdnError);
            } else if (err == CommandException.Error.USSD_MODIFIED_TO_DIAL) {
                Rlog.i(LOG_TAG, "USSD_MODIFIED_TO_DIAL");
                return mContext.getText(com.android.internal.R.string.stk_cc_ussd_to_dial);
            } else if (err == CommandException.Error.USSD_MODIFIED_TO_SS) {
                Rlog.i(LOG_TAG, "USSD_MODIFIED_TO_SS");
                return mContext.getText(com.android.internal.R.string.stk_cc_ussd_to_ss);
            } else if (err == CommandException.Error.USSD_MODIFIED_TO_USSD) {
                Rlog.i(LOG_TAG, "USSD_MODIFIED_TO_USSD");
                return mContext.getText(com.android.internal.R.string.stk_cc_ussd_to_ussd);
            } else if (err == CommandException.Error.SS_MODIFIED_TO_DIAL) {
                Rlog.i(LOG_TAG, "SS_MODIFIED_TO_DIAL");
                return mContext.getText(com.android.internal.R.string.stk_cc_ss_to_dial);
            } else if (err == CommandException.Error.SS_MODIFIED_TO_USSD) {
                Rlog.i(LOG_TAG, "SS_MODIFIED_TO_USSD");
                return mContext.getText(com.android.internal.R.string.stk_cc_ss_to_ussd);
            } else if (err == CommandException.Error.SS_MODIFIED_TO_SS) {
                Rlog.i(LOG_TAG, "SS_MODIFIED_TO_SS");
                return mContext.getText(com.android.internal.R.string.stk_cc_ss_to_ss);
            }
        }

        return mContext.getText(com.android.internal.R.string.mmiError);
    }

    private CharSequence getScString() {
        if (mSc != null) {
            if (isServiceCodeCallBarring(mSc)) {
                return mContext.getText(com.android.internal.R.string.BaMmi);
            } else if (isServiceCodeCallForwarding(mSc)) {
                return mContext.getText(com.android.internal.R.string.CfMmi);
            } else if (mSc.equals(SC_CLIP)) {
                return mContext.getText(com.android.internal.R.string.ClipMmi);
            } else if (mSc.equals(SC_CLIR)) {
                return mContext.getText(com.android.internal.R.string.ClirMmi);
            } else if (mSc.equals(SC_PWD)) {
                return mContext.getText(com.android.internal.R.string.PwdMmi);
            } else if (mSc.equals(SC_WAIT)) {
                return mContext.getText(com.android.internal.R.string.CwMmi);
            } else if (isPinPukCommand()) {
                /**
                 * SPRD: porting ussd feature for bug475740 @{
                 * @orig return mContext.getText(com.android.internal.R.string.PinMmi);
                 */
                if (mSc.equals(SC_PIN)) {
                    return mContext.getText(com.android.internal.R.string.PinMmi);
                } else if (mSc.equals(SC_PIN2)) {
                    return mContext.getText(com.android.internal.R.string.Pin2Mmi);
                } else if (mSc.equals(SC_PUK)) {
                    return mContext.getText(com.android.internal.R.string.PukMmi);
                } else if (mSc.equals(SC_PUK2)) {
                    return mContext.getText(com.android.internal.R.string.Puk2Mmi);
                }

            } else if (mSc.equals(SC_COLP)) {
                return mContext.getText(com.android.internal.R.string.ColpMmi);
            } else if (mSc.equals(SC_COLR)) {
                return mContext.getText(com.android.internal.R.string.ColrMmi);
            }
            /** @} */
        }

        return "";
    }

    private void onSetComplete(Message msg, AsyncResult ar) {
        StringBuilder sb = new StringBuilder(getScString());
        sb.append("\n");

        if (ar.exception != null) {
            mState = State.FAILED;
            if (ar.exception instanceof CommandException) {
                CommandException.Error err = ((CommandException) (ar.exception)).getCommandError();
                if (err == CommandException.Error.PASSWORD_INCORRECT) {
                    if (isPinPukCommand()) {
                        // look specifically for the PUK commands and adjust
                        // the message accordingly.
                        /**
                         * SPRD: porting ussd feature for bug475740
                         * @orig if (mSc.equals(SC_PUK) || mSc.equals(SC_PUK2)) {
                         *       sb.append(mContext.getText( com.android.internal.R.string.badPuk));
                         *       } else { sb.append(mContext.getText(
                         *       com.android.internal.R.string.badPin)); }
                         */
                        int type = 0;
                        if (mSc.equals(SC_PUK)) {
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.badPuk));
                            type = UNLOCK_PUK;
                        } else if (mSc.equals(SC_PUK2)) {
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.badPuk2));
                            type = UNLOCK_PUK2;
                        } else if (mSc.equals(SC_PIN)) {
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.badPin));
                            type = UNLOCK_PIN;
                        } else if (mSc.equals(SC_PIN2)) {
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.badPin2));
                            type = UNLOCK_PIN2;
                        }
                        // Get the No. of retries remaining to unlock PUK/PUK2
                        /**
                         * SPRD: porting ussd feature for bug475740
                         * @orig int attemptsRemaining = msg.arg1;
                         */
                        int attemptsRemaining = getRemainTimes(mContext, type, mPhone.getPhoneId());
                        if (attemptsRemaining <= 0) {
                            Rlog.d(LOG_TAG, "onSetComplete: PUK locked,"
                                    + " cancel as lock screen will handle this");
                            mState = State.CANCELLED;
                            /* SPRD: porting ussd feature for bug475740 @{ */
                            if (attemptsRemaining == 0 && type == UNLOCK_PIN2) {
                                mState = State.FAILED;
                                sb = new StringBuilder(getScString());
                                sb.append("\n");
                                sb.append(mContext.getText(
                                        com.android.internal.R.string.needPuk2));
                            }
                            /* @} */
                        } else if (attemptsRemaining > 0) {
                            Rlog.d(LOG_TAG, "onSetComplete: attemptsRemaining="
                                    + attemptsRemaining);
                            sb.append(mContext.getResources().getQuantityString(
                                    com.android.internal.R.plurals.pinpuk_attempts,
                                    attemptsRemaining, attemptsRemaining));
                        }
                    } else {
                        sb.append(mContext.getText(
                                com.android.internal.R.string.passwordIncorrect));
                    }
                } else if (err == CommandException.Error.SIM_PUK2) {
                    sb.append(mContext.getText(
                            com.android.internal.R.string.badPin));
                    sb.append("\n");
                    sb.append(mContext.getText(
                            com.android.internal.R.string.needPuk2));
                } else if (err == CommandException.Error.REQUEST_NOT_SUPPORTED) {
                    if (mSc.equals(SC_PIN)) {
                        sb.append(mContext.getText(com.android.internal.R.string.enablePin));
                    }
                } else if (err == CommandException.Error.FDN_CHECK_FAILURE) {
                    Rlog.i(LOG_TAG, "FDN_CHECK_FAILURE");
                    sb.append(mContext.getText(com.android.internal.R.string.mmiFdnError));
                } else {
                    sb.append(getErrorMessage(ar));
                }
            } else {
                sb.append(mContext.getText(
                        com.android.internal.R.string.mmiError));
            }
        } else if (isActivate()) {
            mState = State.COMPLETE;
            if (isCallFwdReg()) {
                sb.append(mContext.getText(
                        com.android.internal.R.string.serviceRegistered));
            } else {
                sb.append(mContext.getText(
                        com.android.internal.R.string.serviceEnabled));
            }
            // Record CLIR setting
            if (mSc.equals(SC_CLIR)) {
                mPhone.saveClirSetting(CommandsInterface.CLIR_INVOCATION);
            }
        } else if (isDeactivate()) {
            mState = State.COMPLETE;
            sb.append(mContext.getText(
                    com.android.internal.R.string.serviceDisabled));
            // Record CLIR setting
            if (mSc.equals(SC_CLIR)) {
                mPhone.saveClirSetting(CommandsInterface.CLIR_SUPPRESSION);
            }
        } else if (isRegister()) {
            mState = State.COMPLETE;
            sb.append(mContext.getText(
                    com.android.internal.R.string.serviceRegistered));
        } else if (isErasure()) {
            mState = State.COMPLETE;
            sb.append(mContext.getText(
                    com.android.internal.R.string.serviceErased));
        } else {
            mState = State.FAILED;
            sb.append(mContext.getText(
                    com.android.internal.R.string.mmiError));
        }

        mMessage = sb;
        mPhone.onMMIDone(this);
    }

    public static byte[] getRequestRawBytes(byte funcId, byte subFuncId) {
        byte[] rawBytes = new byte[2];
        rawBytes[0] = funcId;
        rawBytes[1] = subFuncId;
        return rawBytes;
    }

    public static byte[] getStringRequestRawBytes(String value,
            byte funcId, byte subFuncId) {
        byte[] rawData = value.getBytes();
        byte[] rawBytes = new byte[rawData.length + 9];
        rawBytes[0] = funcId;
        rawBytes[1] = subFuncId;
        byte[] len = intToByteArray(rawData.length);
        System.arraycopy(len, 0, rawBytes, 4, len.length);
        System.arraycopy(rawData, 0, rawBytes, 8, rawData.length);
        rawBytes[rawBytes.length - 1] = 0;
        return rawBytes;
    }

    public static byte[] intToByteArray(int value) {
        byte[] bytes = new byte[4];
        bytes[3] = (byte) ((value >> 24) & 0xFF);
        bytes[2] = (byte) ((value >> 16) & 0xFF);
        bytes[1] = (byte) ((value >> 8) & 0xFF);
        bytes[0] = (byte) (value & 0xFF);
        return bytes;
    }

    public static int byteArrayToInt(byte[] bytes, int offset) {
        if (bytes == null)
            return 0;
        if (offset + 4 > bytes.length)
            return 0;
        int value;
        value = (int) ((bytes[offset] & 0xFF)
                | ((bytes[offset + 1] & 0xFF) << 8)
                | ((bytes[offset + 2] & 0xFF) << 16) | ((bytes[offset + 3] & 0xFF) << 24));
        return value;
    }

    public static GsmMmiCode newNetworkInitiatedUssdError(String ussdMessage,
            boolean isUssdRequest, GsmCdmaPhone phone, UiccCardApplication app) {
        GsmMmiCode ret;
        ret = TeleFrameworkFactory.getInstance().createGsmMmiCode(phone,app);
        ret.mMessage = ret.mContext.getText(com.android.internal.R.string.mmiError);
        ret.setUssdRequest(isUssdRequest);
        ret.mState = State.FAILED;

        return ret;
    }

    /* SPRD: modify for bug641723 @{ */
    private void onQueryLiComplete(Message msg) {
        StringBuilder sb = new StringBuilder(getScString());
        sb.append("\n");
        Rlog.i(LOG_TAG, "QUERY_LI_RESPONSE msg:" + msg);
        if (QUERY_LI_RESPONSE_ERROR == msg.arg1) {
            mState = State.FAILED;
            sb.append(mContext.getText(com.android.internal.R.string.mmiError));
        } else if (QUERY_LI_FOR_SUPP_PWD_REG == msg.arg2) {
            if (QUERY_LI_SUPP_PWD_REG_SUCCESS == msg.arg1) {
                sb.append(mContext.getText(com.android.internal.R.string.serviceRegistered));
            } else if (QUERY_LI_SUPP_PWD_REG_FAIL == msg.arg1) {
                sb.append(mContext.getText(com.android.internal.R.string.passwordIncorrect));
            } else {
                sb.append(mContext.getText(com.android.internal.R.string.mmiError));
            }
            mState = State.COMPLETE;
        }else {
            if (QUERY_LI_RESPONSE_DISABLE == msg.arg1) {
                sb.append(mContext.getText(com.android.internal.R.string.serviceDisabled));
            } else if (QUERY_LI_RESPONSE_ENABLE == msg.arg1) {
                sb.append(mContext.getText(com.android.internal.R.string.serviceEnabled));
            } else {
                sb.append(mContext.getText(com.android.internal.R.string.mmiError));
            }
            mState = State.COMPLETE;
        }
        mMessage = sb;
        mPhone.onMMIDone(this);
    }
    /* @} */

    /* SPRD: Bug 597510 Read correct remain times @{ */
    private int getRemainTimes(Context context, int type, int phoneId) {
        String remainTimesProperty = null;
        int remainTimes = MIN_REMAINTIMES;

        if (type >= 0 && type < PROPERTY_REMAINTIMES.length) {
            remainTimesProperty = PROPERTY_REMAINTIMES[type];
        }

        if (!TextUtils.isEmpty(remainTimesProperty)) {
            String propertyValue = TelephonyManager.from(context)
                    .getTelephonyProperty(phoneId, remainTimesProperty, "");

            if (!TextUtils.isEmpty(propertyValue)) {
                try {
                    remainTimes = Integer.valueOf(propertyValue);
                } catch (Exception e) {}
            }
        }

        return remainTimes;
    }
    /* @} */

    /* SPRD: modify for bug627022 @{ */
    private void requestOpenNetWork() {
        Rlog.d(LOG_TAG, "request Network");
        NetworkRequest networkRequest = new NetworkRequest.Builder()
                .addTransportType(NetworkCapabilities.TRANSPORT_CELLULAR)
                .addCapability(NetworkCapabilities.NET_CAPABILITY_XCAP)
                .build();
        mConnectivityManager.requestNetwork(networkRequest,
                mNetworkRequestCallback,
                NETWORK_REQUEST_TIMEOUT_MILLIS);
    }

    public void releaseNetWorkRequest(boolean onLost) {
        Rlog.d(LOG_TAG, "releaseNetWorkRequest  mNetworkRequestCallback:" + mNetworkRequestCallback
                + "  isNetWorkRequestFirst:" + isNetWorkRequestFirst);
        if (mNetworkRequestCallback != null && !isNetWorkRequestFirst) {
            Rlog.d(LOG_TAG, "unregisterNetworkCallback");
            mConnectivityManager.unregisterNetworkCallback(mNetworkRequestCallback);
            mNetworkRequestCallback = null;
            isNetWorkRequestFirst = true;
            if (onLost){
                Rlog.d(LOG_TAG, "releaseNetWorkRequest>>onLost "+onLost);
                mState = State.FAILED;
                mMessage = mContext.getText(com.android.internal.R.string.mmiError);
                mPhone.onMMIDone(this);
            }
        }
    }

    private class NetworkRequestCallback extends ConnectivityManager.NetworkCallback {

        @Override
        public void onAvailable(Network network) {
            Rlog.d(LOG_TAG, "network:" + network
                    + "  isNetWorkRequestFirst:" + isNetWorkRequestFirst);
            if (isNetWorkRequestFirst) {
                isNetWorkRequestFirst = false;
                try {
                processCodeAfterRequest();
                } catch (CallStateException e) {
                    Rlog.d(LOG_TAG, "onAvailable>> "+e);
                }
            }
        }

        @Override
        public void onLost(Network network) {
            Rlog.d(LOG_TAG, "onLost");
            if (mNetworkRequestCallback != null) {
                isNetWorkRequestFirst = false;
                releaseNetWorkRequest(true);
            }
        }
    }
    /* @} */
}
