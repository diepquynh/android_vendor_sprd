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

package com.sprd.phone.plugin;

import android.telephony.PhoneNumberUtilsPlugin;

import android.app.AddonManager;
import android.content.Context;
import android.os.SystemProperties;
import android.telephony.PhoneNumberUtils;
import android.telephony.PhoneNumberUtilsSprd;
import android.telephony.TelephonyManagerSprd;
import android.text.TextUtils;
import android.util.Log;

import com.android.internal.telephony.gsm.GSMPhoneSprd;

/**
 * Various utilities for dealing with phone number strings.
 */
public class AddonPhoneNumberUtils extends PhoneNumberUtilsPlugin implements AddonManager.InitialCallback
{
    private Context mAddonContext;
    private static final String TAG = "[CMCC---AddonPhoneNumberUtils]";
    private static final String CUSTOM_EMERGENCY_NUMBER = "110,119,120";
    private static final String CMCC_SUPPORT_EMERGENCY_NUMBER = "000,08,999,118";

    public AddonPhoneNumberUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        log("AddonPhoneNumberUtils clazz: " + clazz);
        mAddonContext = context;
        return clazz;
    }

    public boolean isCustomEmergencyNumber(String number) {
        log("isCustomEmergencyNumber(number: "+number+")");
        return (PhoneNumberUtilsSprd.isEmergencyNumber(number) && (CUSTOM_EMERGENCY_NUMBER.indexOf(number) != -1
                || CMCC_SUPPORT_EMERGENCY_NUMBER.indexOf(number) != -1));
    }

    public static boolean isCustomEmergencyNumber2(String number) {
        log("isCustomEmergencyNumber2(number: "+number+")");
        return (CUSTOM_EMERGENCY_NUMBER.indexOf(number) != -1) || (CMCC_SUPPORT_EMERGENCY_NUMBER.indexOf(number) != -1);
    }

    /**
     * SPRD:
     * @hide
     */
    public boolean isSimEmergencyNumber(String number, int phoneId) {
        log("isSimEmergencyNumber(number: "+number+", phoneId: "+phoneId+")");
        if (number == null || phoneId < 0) return false;
        int phoneCount = TelephonyManagerSprd.getDefault().getPhoneCount();
        if (phoneId >= phoneCount) return false;
        // Strip the separators from the number before comparing it
        // to the list.
        number = PhoneNumberUtils.extractNetworkPortionAlt(number);
        /* SPRD: modify for bug 440410 {@ */
        if (TextUtils.isEmpty(number)) return false;
        /* @} */
        // modified emergency number logic 2011-9-30 start
        StringBuffer numbers = new StringBuffer();

        boolean hasSimCard = false;
        for (int j = 0; j < phoneCount; j++ ) {
            hasSimCard = hasSimCard || TelephonyManagerSprd.getDefault().hasIccCard(j);
            if (hasSimCard) {
                break;
            }
        }

        if (!hasSimCard) {
            numbers.append(CMCC_SUPPORT_EMERGENCY_NUMBER);
            numbers.append(",");
            numbers.append(CUSTOM_EMERGENCY_NUMBER);
        } else if (TelephonyManagerSprd.getDefault().hasIccCard(phoneId)) {
            // retrieve the list of emergency numbers
            // check read-write ecclist property first
            String tmpnumbers = SystemProperties.get("ril.ecclist");
            if (TextUtils.isEmpty(tmpnumbers)) {
                // then read-only ecclist property since old RIL only uses this
                tmpnumbers = SystemProperties.get("ro.ril.ecclist");
            }
            if (!TextUtils.isEmpty(tmpnumbers)){
                numbers.append(",");
                numbers.append(tmpnumbers);
            }

            // retrieve the list of ecc in sim card
            String eccList = SystemProperties.get(
                    GSMPhoneSprd.getSetting("ril.sim.ecclist",phoneId));
            if (!TextUtils.isEmpty(eccList)){
                numbers.append(",");
                numbers.append(eccList);
            }
            // cmcc
            numbers.append(",");
            numbers.append(CMCC_SUPPORT_EMERGENCY_NUMBER);
        }

        log("sim" + phoneId + " emergency numbers: " + numbers);

        if (!TextUtils.isEmpty(numbers)) {
            // searches through the comma-separated list for a match,
            // return true if one is found.
            for (String emergencyNum : numbers.toString().split(",")) {
                if (number.equals(emergencyNum)) {
                    return true;
                }
            }
        }
        // No ecclist system property, so use our own list.
        return (number.equals("112") || number.equals("911"));
    }

    private static void log(String msg) {
        Log.d(TAG, msg);
    }
}
