
package com.sprd.engineermode.debuglog;

import java.util.ArrayList;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceGroup;
import android.telephony.TelephonyManager;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;
import android.telephony.SubscriptionManager;
import android.widget.Toast;
import android.content.ContentValues;
import android.net.Uri;
import android.content.ContentResolver;
import android.provider.Settings;
import android.provider.Telephony;
//import android.provider.TelephonySprd;
import com.android.internal.telephony.TelephonyProperties;

import android.content.Context;
import android.util.Log;
import android.content.ComponentName;
import android.database.Cursor;
import com.sprd.engineermode.R;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionInfo;

public class APNSettingActivity extends PreferenceActivity implements
        Preference.OnPreferenceClickListener {

    private static final String TAG = "APNSettingActivity";
    public static final String KEY_SELECT_SIM = "select_sim";
    public static final String SUB_ID = "sub_id";
    private static final String KEY_SIM_PREFERENCE = "sim_";
    private static final String KEY_CUSTOM_SET = "custom_set";
    private static final String DEFAULT_APN = "test.rohde-schwarz";
    public static final String PREFERRED_APN_URI =
            "content://telephony/carriers/preferapn";
    private final String APN_ID = "apn_id";
    private final String APN_ID_SIM2 = "apn_id_sim2";
    private final String APN_ID_SIM3 = "apn_id_sim3";
    private ContentResolver mResolver;

    private static final int ID_INDEX = 0;
    private static final int NAME_INDEX = 1;
    private static final int APN_INDEX = 2;
    private static final int TYPES_INDEX = 3;
    private static final Uri PREFERAPN_URI = Uri.parse(PREFERRED_APN_URI);
    public static int mPhoneId = 0;
    private String mSelectedKey;
    public static int mSubId = 0;
    private SubscriptionManager mSubscriptionManager;

    PreferenceGroup mPreGroup = null;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mResolver = getContentResolver();
        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
        mSubscriptionManager = (SubscriptionManager) SubscriptionManager
                .from(APNSettingActivity.this);
        mPreGroup = getPreferenceScreen();
        getListView().setItemsCanFocus(true);
        mPhoneId = this.getIntent().getIntExtra(KEY_SELECT_SIM, 0);
        mSubId = slotIdToSubId(mPhoneId);
        Preference pref = new Preference(this);
        pref.setTitle(R.string.custom_set);
        pref.setKey(KEY_CUSTOM_SET);
        pref.setOnPreferenceClickListener(this);
        mPreGroup.addPreference(pref);
    }

    public int slotIdToSubId(int phoneId) {
        int subId;
        SubscriptionInfo mSubscriptionInfo = mSubscriptionManager.getActiveSubscriptionInfoForSimSlotIndex(phoneId);
        if (mSubscriptionInfo != null) {
            subId = mSubscriptionInfo.getSubscriptionId();
        } else {
            subId = SubscriptionManager.getDefaultSubscriptionId();
        }
        return subId;
     }

    @Override
    public boolean onPreferenceClick(Preference pref) {
        String key = null;
        key = pref.getKey();
        if (key == null) {
            return false;
        }
        if (key.equals(KEY_CUSTOM_SET)) {
            AlertDialog alertDialog = new AlertDialog.Builder(this)
                    .setTitle(getString(R.string.custom_set))
                    .setMessage(getString(R.string.custom_apn_set))
                    .setPositiveButton(R.string.alertdialog_ok,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                }
                            }).create();
            alertDialog.show();
        }
        return true;
    }
}
