/** Copyright 2009 - 2013 Broadcom Corporation
**
** This program is the proprietary software of Broadcom Corporation and/or its
** licensors, and may only be used, duplicated, modified or distributed
** pursuant to the terms and conditions of a separate, written license
** agreement executed between you and Broadcom (an "Authorized License").
** Except as set forth in an Authorized License, Broadcom grants no license
** (express or implied), right to use, or waiver of any kind with respect to
** the Software, and Broadcom expressly reserves all rights in and to the
** Software and all intellectual property rights therein.
** IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
** SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
** ALL USE OF THE SOFTWARE.
**
** Except as expressly set forth in the Authorized License,
**
** 1.     This program, including its structure, sequence and organization,
**        constitutes the valuable trade secrets of Broadcom, and you shall
**        use all reasonable efforts to protect the confidentiality thereof,
**        and to use this information only in connection with your use of
**        Broadcom integrated circuit products.
**
** 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
**        "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
**        REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
**        OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
**        DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
**        NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
**        ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
**        CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
**        OF USE OR PERFORMANCE OF THE SOFTWARE.
**
** 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
**        ITS LICENSORS BE LIABLE FOR
**        (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
**              DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
**              YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
**              HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
**        (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
**              SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
**              LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
**              ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
*/

package com.broadcom.bt.app.fm.rx;

import com.broadcom.bt.app.fm.R;
import android.util.Log;
import java.lang.Integer;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceGroup;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;

public class FmRadioSettings extends PreferenceActivity implements OnPreferenceClickListener,
        OnSharedPreferenceChangeListener {

    private static final String TAG = "RADIO SETTINGS";

    /* Public strings for enabling preference listening elsewhere. */
    public static final String FM_PREF_ENABLE = "fm_pref_enabled";
    public static final String FM_PREF_AUDIO_MODE = "fm_pref_audio_mode";
    public static final String FM_PREF_AUDIO_PATH = "fm_pref_audio_path";
    public static final String FM_PREF_SCAN_STEP = "fm_pref_scan_step";
    public static final String FM_PREF_RDS_ENABLED = "fm_pref_rds_enabled_2";
    public static final String FM_PREF_AF_ENABLED = "fm_pref_af_enabled";
    public static final String FM_PREF_WORLD_REGION = "fm_pref_world_region";
    public static final String FM_PREF_DEEMPHASIS = "fm_pref_deemphasis";
    public static final String FM_PREF_LIVE_POLLING = "fm_pref_live_polling";
    public static final String FM_PREF_LIVE_POLL_INT = "fm_pref_live_polling_interval";
    public static final String FM_PREF_NFL_MODE = "fm_pref_nfl_mode";
    public static final String FM_PREF_SNR_THRESHOLD = "fm_pref_snr_thres";

    public static final String FM_RDS_MODE_OFF_VALUE = "0";
    public static final String FM_RDS_MODE_ON_VALUE = "2";

    Preference AFPreference, RDSModePreference;

    public FmRadioSettings() {
    }

    /** Called with the activity is first created. */

    public void onCreate(Bundle b) {
        super.onCreate(b);

        Log.v(TAG, "onCreate()");

        /* Initialize local references to all preferences for GUI purposes. */
        addPreferencesFromResource(R.xml.fm_radio_preferences);

        Preference p = getPreferenceScreen().findPreference(FM_PREF_AUDIO_MODE);
        p.setOnPreferenceClickListener(new CyclePreferenceClickListener());
        p = getPreferenceScreen().findPreference(FM_PREF_AUDIO_PATH);
        p.setOnPreferenceClickListener(new CyclePreferenceClickListener());

        RDSModePreference = getPreferenceScreen().findPreference(FM_PREF_RDS_ENABLED);
        RDSModePreference.setOnPreferenceClickListener(this);
        AFPreference = getPreferenceScreen().findPreference(FM_PREF_AF_ENABLED);
        // Log.d(TAG, ">>"+((ListPreference)RDSModePreference).getValue());
        if (((ListPreference) RDSModePreference).getValue().equals(FM_RDS_MODE_OFF_VALUE)) {
            AFPreference.setEnabled(false);
        } else {
            AFPreference.setEnabled(true);
        }
    }

    // @Override
    protected void onResume() {
        super.onResume();
        // Set up a listener whenever a key changes
        getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
    }

    // @Override
    protected void onPause() {
        super.onPause();
        // Unregister the listener the PreferenceActivity goes to background
        getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(
                this);
    }

    // @Override
    public boolean onPreferenceClick(Preference preference) {
        if (preference.getKey().equals(FM_PREF_RDS_ENABLED)) {
            Log.i(TAG, "key:" + preference.getKey());
            Log.i(TAG, "value:" + (String) ((ListPreference) preference).getValue());
            return true;
        }
        return false;
    }

    // @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        if (key.equals(FM_PREF_RDS_ENABLED)) {
            String result = sharedPreferences.getString(FM_PREF_RDS_ENABLED, "NULL");
            if (result != "NULL") {
                if (result.equals(FM_RDS_MODE_OFF_VALUE))
                    AFPreference.setEnabled(false);
                else
                    AFPreference.setEnabled(true);
            }
        }
    }

    private class CyclePreferenceClickListener implements OnPreferenceClickListener {

        // @Override
        public boolean onPreferenceClick(Preference p) {
            ListPreference lp = (ListPreference) p;
            int index = lp.findIndexOfValue(lp.getValue());
            Log.v(TAG, "onPreferenceClick index:" + index);
            // lp.setValueIndex((index + 1) % lp.getEntryValues().length);
            return true;
        }

    }
}
