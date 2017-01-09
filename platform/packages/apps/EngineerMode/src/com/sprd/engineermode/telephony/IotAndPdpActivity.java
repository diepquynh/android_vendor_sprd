
package com.sprd.engineermode.telephony;

import java.util.HashMap;
import android.os.PowerManager.WakeLock;
import android.os.PowerManager;
import java.util.Iterator;
import java.util.Set;
import java.util.Map.Entry;

import android.app.AlertDialog.Builder;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.widget.Toast;
import android.os.Looper;
import android.content.SharedPreferences;
import android.os.HandlerThread;
import android.os.SystemProperties;
import android.preference.PreferenceManager;
import android.content.BroadcastReceiver;
import android.content.Context;

import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.telephony.SendPowerPrefActivity;
import com.sprd.engineermode.utils.IATUtils;
import android.app.AlertDialog;

import java.lang.reflect.Field;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;

public class IotAndPdpActivity extends PreferenceActivity
        implements EMSwitchPreference.OnPreferenceChangeListener {
    private static final String TAG = "IotAndPdpActivity";
    private static final String IOT = "key_iot";
    private static final String PDPONE = "key_pdp_one";
    private static final String PDPTWO = "key_pdp_two";
    private static final String PDP_NORETRY = "key_pdp_noretry";
    private static final String REUSE_DEFAULT_PDN = "key_reuse_default_pdn";
    private EMSwitchPreference mReusePdnPreference = null;
    private EMSwitchPreference mIotPreference = null;
    private EMSwitchPreference mPdpOnePreference = null;
    private EMSwitchPreference mPdpTwoPreference = null;
    private EMSwitchPreference mIotNoRetryPreference = null;
    private TelephonyManager mTm = null;
    private ConnectivityManager mCm = null;
    private ConnectivityManager.NetworkCallback mNetworkCallbackFirst = null;
    private ConnectivityManager.NetworkCallback mNetworkCallbackSecond = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        addPreferencesFromResource(R.xml.pref_iot_pdp);
        mIotPreference = (EMSwitchPreference) findPreference(IOT);
        mIotPreference.setOnPreferenceChangeListener(this);
        mPdpOnePreference = (EMSwitchPreference) findPreference(PDPONE);
        mPdpOnePreference.setOnPreferenceChangeListener(this);
        mPdpTwoPreference = (EMSwitchPreference) findPreference(PDPTWO);
        mPdpTwoPreference.setOnPreferenceChangeListener(this);
        mIotNoRetryPreference  = (EMSwitchPreference) findPreference(PDP_NORETRY);
        mIotNoRetryPreference.setOnPreferenceChangeListener(this);
        mReusePdnPreference  = (EMSwitchPreference) findPreference(REUSE_DEFAULT_PDN);
        mReusePdnPreference.setOnPreferenceChangeListener(this);
        mTm = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
        mCm = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        init();
    }

    @Override
    public void onStart() {
        super.onStart();
        if (mIotPreference != null) {
            boolean isIot = SystemProperties.getBoolean(
                    "persist.sys.volte.iot", false);
            Log.d(TAG, "Iot: " + isIot);
            if (isIot) {
                mIotPreference.setChecked(true);
            } else {
                mIotPreference.setChecked(false);
            }
        }
        if(mIotNoRetryPreference != null){
            boolean isNoretry = SystemProperties.getBoolean("persist.sys.pdp.noretry", false);
            Log.d(TAG, "isNoretry: " + isNoretry);
            if(isNoretry){
                mIotNoRetryPreference.setChecked(true);
            }else {
                mIotNoRetryPreference.setChecked(false);
            }
        
        }
        if(mReusePdnPreference != null){
            boolean ReusePdn = SystemProperties.getBoolean("persist.sys.pdp.reuse", false);
            Log.d(TAG, "ReusePdn: " + ReusePdn);
            if(ReusePdn){
               mReusePdnPreference.setChecked(true);
            }else {
               mReusePdnPreference.setChecked(false);
            }
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (preference instanceof EMSwitchPreference) {
            String key = preference.getKey();
            if (key.equals(IOT)) {
                if (!mIotPreference.isChecked()) {
                    Log.d(TAG, "Open Iot Switch");
                    SystemProperties.set("persist.sys.volte.iot", "1");
                    mIotPreference.setChecked(true);
                } else {
                    Log.d(TAG, "Close Iot Switch");
                    SystemProperties.set("persist.sys.volte.iot", "0");
                    mIotPreference.setChecked(false);
                }
            } else if(key.equals(PDP_NORETRY)){
                if (!mIotNoRetryPreference.isChecked()){
                    Log.d(TAG, "Open pdp noretry Switch");
                    SystemProperties.set("persist.sys.pdp.noretry", "1");
                    mIotNoRetryPreference.setChecked(true);
               } else {
                   Log.d(TAG, "Close pdp noretry Switch");
                   SystemProperties.set("persist.sys.pdp.noretry", "0");
                   mIotNoRetryPreference.setChecked(false);
               }
            }else if (key.equals(PDPONE)) {
                if (!mPdpOnePreference.isChecked()) {
                    Log.d(TAG, "activate 1 pdp");
                    NetworkRequest request = buildRequest(NetworkCapabilities.NET_CAPABILITY_SUPL);
                    mCm.requestNetwork(request, mNetworkCallbackFirst);
                    mPdpOnePreference.setChecked(true);
                } else {
                    try {
                        mCm.unregisterNetworkCallback(mNetworkCallbackFirst);
                        mPdpTwoPreference.setChecked(false);
                    } catch (IllegalArgumentException iea) {
                        iea.printStackTrace();
                    }
                }
            } else if (key.equals(PDPTWO)) {
                if (!mPdpTwoPreference.isChecked()) {
                    Log.d(TAG, "activate 2 pdp");
                    NetworkRequest request = buildRequest(NetworkCapabilities.NET_CAPABILITY_MMS);
                    mCm.requestNetwork(request, mNetworkCallbackSecond);
                    mPdpTwoPreference.setChecked(true);
                } else {
                    try {
                        mCm.unregisterNetworkCallback(mNetworkCallbackSecond);
                        mPdpTwoPreference.setChecked(false);
                    } catch (IllegalArgumentException iea) {
                        iea.printStackTrace();
                    }
                }
            }else if(key.equals(REUSE_DEFAULT_PDN)){
                if (!mReusePdnPreference.isChecked()){
                    Log.d(TAG, "Open reuse default pdn Switch");
                    SystemProperties.set("persist.sys.pdp.reuse", "1");
                    mReusePdnPreference.setChecked(true);
               } else {
                   Log.d(TAG, "Close reuse default pdn Switch");
                   SystemProperties.set("persist.sys.pdp.reuse", "0");
                   mReusePdnPreference.setChecked(false);
               }
            }
            return true;
        }
        return false;
    }

    private NetworkRequest buildRequest(int type) {
        NetworkRequest.Builder builder = null;
        builder = new NetworkRequest.Builder();
        builder.addCapability(type);
        builder.addTransportType(NetworkCapabilities.TRANSPORT_CELLULAR);
        builder.setNetworkSpecifier("");
        NetworkRequest request = builder.build();
        return request;
    }

    private void init() {
        /*
         * if(mPdpOnePreference!=null){ mPdpOnePreference.setChecked(false); }
         * if(mPdpTwoPreference!=null){ mPdpTwoPreference.setChecked(false); }
         */

        mNetworkCallbackSecond = new ConnectivityManager.NetworkCallback() {
            @Override
            public void onAvailable(Network network) {
                super.onAvailable(network);
                Log.d(TAG, "network2 onAvailable");
                Toast.makeText(IotAndPdpActivity.this, "success", Toast.LENGTH_SHORT)
                        .show();
            }

            @Override
            public void onLost(Network network) {
                super.onLost(network);
                Log.d(TAG, "network2 onLost");
            }
        };
        mNetworkCallbackFirst = new ConnectivityManager.NetworkCallback() {
            @Override
            public void onAvailable(Network network) {
                super.onAvailable(network);
                Log.d(TAG, "network1 onAvailable");
                Toast.makeText(IotAndPdpActivity.this, "success", Toast.LENGTH_SHORT)
                        .show();
            }

            @Override
            public void onLost(Network network) {
                super.onLost(network);
                Log.d(TAG, "network1 onLost");
            }
        };
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }
}
