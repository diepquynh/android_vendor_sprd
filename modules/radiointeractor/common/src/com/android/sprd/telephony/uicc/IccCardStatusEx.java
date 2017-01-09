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

package com.android.sprd.telephony.uicc;

/**
 * See also RIL_CardStatus in include/telephony/ril.h
 *
 * {@hide}
 */
public class IccCardStatusEx {
    public static final int CARD_MAX_APPS = 8;

    public static final int SIM_STATE_EX_BASE = 8;
    public static final int SIM_STATE_NETWORKSUBSET_LOCKED = SIM_STATE_EX_BASE + 1;
    public static final int SIM_STATE_SERVICEPROVIDER_LOCKED = SIM_STATE_EX_BASE + 2;
    public static final int SIM_STATE_CORPORATE_LOCKED = SIM_STATE_EX_BASE + 3;
    public static final int SIM_STATE_SIM_LOCKED = SIM_STATE_EX_BASE + 4;
    public static final int SIM_STATE_NETWORK_LOCKED_PUK = SIM_STATE_EX_BASE + 5;
    public static final int SIM_STATE_NETWORK_SUBSET_LOCKED_PUK = SIM_STATE_EX_BASE + 6;
    public static final int SIM_STATE_SERVICE_PROVIDER_LOCKED_PUK = SIM_STATE_EX_BASE + 7;
    public static final int SIM_STATE_CORPORATE_LOCKED_PUK = SIM_STATE_EX_BASE + 8;
    public static final int SIM_STATE_SIM_LOCKED_PUK = SIM_STATE_EX_BASE + 9;
    public static final int SIM_STATE_SIM_LOCKED_FOREVER = SIM_STATE_EX_BASE + 10;

    private static final int SPRD_BASE = 0;
    public static final int UNLOCK_SIM = SPRD_BASE + 1;
    public static final int UNLOCK_NETWORK = SPRD_BASE + 2;
    public static final int UNLOCK_NETWORK_SUBSET = SPRD_BASE + 3;
    public static final int UNLOCK_SERVICE_PORIVDER = SPRD_BASE + 4;
    public static final int UNLOCK_CORPORATE = SPRD_BASE + 5;
    public static final int UNLOCK_SIM_PUK = SPRD_BASE + 6;
    public static final int UNLOCK_NETWORK_PUK = SPRD_BASE + 7;
    public static final int UNLOCK_NETWORK_SUBSET_PUK = SPRD_BASE + 8;
    public static final int UNLOCK_SERVICE_PORIVDER_PUK = SPRD_BASE + 9;
    public static final int UNLOCK_CORPORATE_PUK = SPRD_BASE + 10;

    public static final String CB_FACILITY_BA_PS = "PS";
    public static final String CB_FACILITY_BA_PN = "PN";
    public static final String CB_FACILITY_BA_PU = "PU";
    public static final String CB_FACILITY_BA_PP = "PP";
    public static final String CB_FACILITY_BA_PC = "PC";
    public static final String CB_FACILITY_BA_PS_PUK = "PSP";
    public static final String CB_FACILITY_BA_PN_PUK = "PNP";
    public static final String CB_FACILITY_BA_PU_PUK = "PUP";
    public static final String CB_FACILITY_BA_PP_PUK = "PPP";
    public static final String CB_FACILITY_BA_PC_PUK = "PCP";

    public enum CardState {
        CARDSTATE_ABSENT,
        CARDSTATE_PRESENT,
        CARDSTATE_ERROR;

        boolean isCardPresent() {
            return this == CARDSTATE_PRESENT;
        }
    }

    public enum PinState {
        PINSTATE_UNKNOWN,
        PINSTATE_ENABLED_NOT_VERIFIED,
        PINSTATE_ENABLED_VERIFIED,
        PINSTATE_DISABLED,
        PINSTATE_ENABLED_BLOCKED,
        PINSTATE_ENABLED_PERM_BLOCKED;

        boolean isPermBlocked() {
            return this == PINSTATE_ENABLED_PERM_BLOCKED;
        }

        boolean isPinRequired() {
            return this == PINSTATE_ENABLED_NOT_VERIFIED;
        }

        boolean isPukRequired() {
            return this == PINSTATE_ENABLED_BLOCKED;
        }
    }

    public CardState  mCardState;
    public PinState   mUniversalPinState;
    public int        mGsmUmtsSubscriptionAppIndex;
    public int        mCdmaSubscriptionAppIndex;
    public int        mImsSubscriptionAppIndex;

    public IccCardApplicationStatusEx[] mApplications;

    public void setCardState(int state) {
        switch(state) {
        case 0:
            mCardState = CardState.CARDSTATE_ABSENT;
            break;
        case 1:
            mCardState = CardState.CARDSTATE_PRESENT;
            break;
        case 2:
            mCardState = CardState.CARDSTATE_ERROR;
            break;
        default:
            throw new RuntimeException("Unrecognized RIL_CardState: " + state);
        }
    }

    public void setUniversalPinState(int state) {
        switch(state) {
        case 0:
            mUniversalPinState = PinState.PINSTATE_UNKNOWN;
            break;
        case 1:
            mUniversalPinState = PinState.PINSTATE_ENABLED_NOT_VERIFIED;
            break;
        case 2:
            mUniversalPinState = PinState.PINSTATE_ENABLED_VERIFIED;
            break;
        case 3:
            mUniversalPinState = PinState.PINSTATE_DISABLED;
            break;
        case 4:
            mUniversalPinState = PinState.PINSTATE_ENABLED_BLOCKED;
            break;
        case 5:
            mUniversalPinState = PinState.PINSTATE_ENABLED_PERM_BLOCKED;
            break;
        default:
            throw new RuntimeException("Unrecognized RIL_PinState: " + state);
        }
    }

    @Override
    public String toString() {
        IccCardApplicationStatusEx app;

        StringBuilder sb = new StringBuilder();
        sb.append("IccCardState {").append(mCardState).append(",")
        .append(mUniversalPinState)
        .append(",num_apps=").append(mApplications.length)
        .append(",gsm_id=").append(mGsmUmtsSubscriptionAppIndex);
        if (mGsmUmtsSubscriptionAppIndex >=0
                && mGsmUmtsSubscriptionAppIndex <CARD_MAX_APPS) {
            app = mApplications[mGsmUmtsSubscriptionAppIndex];
            sb.append(app == null ? "null" : app);
        }

        sb.append(",cdma_id=").append(mCdmaSubscriptionAppIndex);
        if (mCdmaSubscriptionAppIndex >=0
                && mCdmaSubscriptionAppIndex <CARD_MAX_APPS) {
            app = mApplications[mCdmaSubscriptionAppIndex];
            sb.append(app == null ? "null" : app);
        }

        sb.append(",ims_id=").append(mImsSubscriptionAppIndex);
        if (mImsSubscriptionAppIndex >=0
                && mImsSubscriptionAppIndex <CARD_MAX_APPS) {
            app = mApplications[mImsSubscriptionAppIndex];
            sb.append(app == null ? "null" : app);
        }

        sb.append("}");

        return sb.toString();
    }

}
