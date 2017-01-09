/** Created by Spreadst */
package com.sprd.gallery3d.app;

import java.lang.reflect.Method;
import java.net.UnknownHostException;

import android.app.AlertDialog;
import android.app.SearchManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Bundle;

import android.preference.CheckBoxPreference;

import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;

import android.util.Log;
import android.util.Patterns;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;
import android.telephony.TelephonyManager;
import android.text.Editable;
import android.text.InputFilter;
import android.text.Selection;
import android.telephony.SubscriptionManager;

import android.os.RemoteException;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.preference.EditTextPreference;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.ContentObserver;
import android.os.Handler;
import android.app.ActionBar;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.UserHandle;//SPRD:Bug534607 add
import com.android.gallery3d.R;
import com.android.gallery3d.util.GalleryUtils;

public class MovieViewProxySet extends PreferenceActivity {
    private String LOG_TAG = "MovieViewProxySet";
   // max port value by default
    private static final int RTP_RTCP_MAX_PORT_VALUE = 65535;
    // min port value by default
    private static final int RTP_RTCP_MIN_PORT_VALUE = 1024;


    private Preference mApnSet;
    private EditTextPreference mProxyIpAdress;
    private EditTextPreference mProxyPortSet;
    private EditTextPreference mRtcpMaxPort;
    private EditTextPreference mRtcpMinPort;
    private CheckBoxPreference mProxyCheck;

    private String ProxyIpAdress;
    private String ProxyPortSet;
    public String MaxPort;
    public String MinPort;
    public String ApnSet;

    public boolean ProxyEnble;
    private ContentObserver mContentObserver;
    public static final int SIM_STATE_READY = 5;
    public int CardOne = 0;
    public int CardTwo = 1;

    final String[] CONTENT_PROJECTION = new String[] { "max_port", "min_port",
            "http_proxy", "http_port", "rtsp_proxy", "rtsp_port",
            "proxy_enable", "conn_prof" };

    BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            // TODO Auto-generated method stub
            Log.d(LOG_TAG, "intent.getAction() " + intent.getAction());
            if (intent.getAction().equals(
                    "android.intent.action.SIM_STATE_CHANGED")) {
                TelephonyManager telephonyManager = (TelephonyManager) getSystemService(TELEPHONY_SERVICE);
                if (telephonyManager.getSimState(CardOne) != SIM_STATE_READY  && telephonyManager.getSimState(CardTwo) != SIM_STATE_READY ) {
                    if (mApnSet != null) {
                        mApnSet.setEnabled(false);
                    }
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        // Load the preferences from an XML resource
        addPreferencesFromResource(R.layout.movie_view_proxy_set);

        IntentFilter mFilter = new IntentFilter();
        mFilter.addAction("android.intent.action.SIM_STATE_CHANGED");
        registerReceiver(mReceiver, mFilter);
        TelephonyManager mTelephonyManager = (TelephonyManager) getSystemService(TELEPHONY_SERVICE);
        mApnSet = findPreference("ApnEntrySet");
        /**SPRD:Bug534607 add  @{*/
        if (!mTelephonyManager.hasIccCard()
                || (UserHandle.getCallingUserId() > 0)
                || ((mTelephonyManager.getSimState(CardOne) != SIM_STATE_READY)
                 && (mTelephonyManager.getSimState(CardTwo) != SIM_STATE_READY))) {
            mApnSet.setEnabled(false);
        }
        /**@}*/

        mProxyIpAdress = (EditTextPreference) findPreference("proxy_ip_set");
        mProxyPortSet = (EditTextPreference) findPreference("proxy_port_set");
        mRtcpMaxPort = (EditTextPreference) findPreference("max_port_set");
        mRtcpMinPort = (EditTextPreference) findPreference("min_port_set");
        mProxyCheck = (CheckBoxPreference) findPreference("ProxyEnable");

        loadMovieViewContentProvider();

        mProxyIpAdress.getEditText().setFilters(new InputFilter[]{
                    new InputFilter.LengthFilter(64)
        });
        mProxyPortSet.getEditText().setFilters(
                new InputFilter[] { new InputFilter.LengthFilter(5) });

        mRtcpMaxPort.getEditText().setFilters(new InputFilter[]{
                new InputFilter.LengthFilter(5)
                });

        mRtcpMinPort.getEditText().setFilters(new InputFilter[]{
                new InputFilter.LengthFilter(5)
                });

        setProxyIpPreferenceChangeListener(mProxyIpAdress, "http_proxy",
                ProxyIpAdress);
        setProxyPortPreferenceChangeListener(mProxyPortSet, "http_port",
                ProxyPortSet);
        setMaxMinPortPreferenceChangeListener(mRtcpMaxPort, "max_port", MaxPort);
        setMaxMinPortPreferenceChangeListener(mRtcpMinPort, "min_port", MinPort);

        mContentObserver = new MovieViewProxySetChangeObserver();
        getActionBar().setBackgroundDrawable(new ColorDrawable(Color.GRAY));
        this.getContentResolver().registerContentObserver(
                MovieViewContentProvider.CONTENT_URI, true, mContentObserver);
    }

    @Override
    public void onMultiWindowModeChanged(boolean isInMultiWindowMode) {
        if (isInMultiWindowMode){
            android.util.Log.d(LOG_TAG, "onMultiWindowModeChanged: " + isInMultiWindowMode);
            Toast.makeText(this,R.string.exit_multiwindow_video_tips, Toast.LENGTH_SHORT).show();
            finish();
        }
    }

    @Override
    protected void onResume() {
        // TODO Auto-generated method stub
        GalleryUtils.killActivityInMultiWindow(this, GalleryUtils.DONT_SUPPORT_VIEW_VIDEO);
        super.onResume();
        updateMovieViewContentProvider();
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {
        String key = preference.getKey();
        if ("ApnEntrySet".equals(key)) {
            Intent startIntent = new Intent();
            startIntent.setClassName("com.android.settings",
                    "com.android.settings.Settings$ApnSettingsActivity");
            /* SPRD:add for bug538671
             * when playing videos,go to apn settings through the menu,we found that we have no access to modify the apn settings@{
             */
            int subId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
            final int[] subIdList =
                    SubscriptionManager.from(this).getActiveSubscriptionIdList();
            if (subIdList.length > 1) {
                subId = SubscriptionManager.getDefaultDataSubscriptionId();
            } else if (subIdList.length == 1) {
                subId = subIdList[0];
            }
            startIntent.putExtra("sub_id", subId);
            /*@}*/
            startActivity(startIntent);
            return super.onPreferenceTreeClick(preferenceScreen, preference);
        } else if ("ProxyEnable".equals(key)) {
            boolean enabled = mProxyCheck.isChecked();
            ContentValues values = new ContentValues();
            if (enabled) {
                values.put("proxy_enable", "1");
                mProxyIpAdress.setEnabled(true);
                mProxyPortSet.setEnabled(true);
            } else {
                values.put("proxy_enable", "0");
                mProxyIpAdress.setEnabled(false);
                mProxyPortSet.setEnabled(false);
            }
            getContentResolver().update(MovieViewContentProvider.CONTENT_URI,
                    values, null, null);
        // change the cursor position
        } else if (preference instanceof EditTextPreference) {
            Editable ed = ((EditTextPreference) preference).getEditText().getText();
            Selection.setSelection(ed, ed.length());
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        super.onDestroy();
        unregisterReceiver(mReceiver);
        if (mContentObserver != null) {
            this.getContentResolver().unregisterContentObserver(
                    mContentObserver);
        }
    }

    private String checkNull(String value) {
        if (value == null || value.length() == 0) {
            return "";
        } else {
            return value;
        }
    }

    final DialogInterface.OnClickListener AlertDialogPositive = new DialogInterface.OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) {
            mProxyIpAdress.setText(ProxyIpAdress);
            mProxyPortSet.setText(ProxyPortSet);

            mRtcpMaxPort.setText(MaxPort);
            mRtcpMinPort.setText(MinPort);
        }
    };

    public void setProxyIpPreferenceChangeListener(
            final EditTextPreference ProxyTextPreference,
            final String Proxytype, final String Proxyin) {
        ProxyTextPreference
                .setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
                    public boolean onPreferenceChange(Preference p,
                            Object newValue) {
                        String Proxyout = (String) newValue;
                        if (isIpAddress(Proxyout)
                                || (Proxyout == null || Proxyout.equals(""))) {
                            if ((newValue == null || newValue.equals(""))) {
                                Proxyout = "";
                                ProxyIpAdress = "";
                            }
                            p.setSummary(Proxyout);
                            ContentValues values = new ContentValues();
                            values.put(Proxytype, Proxyout);
                            getContentResolver().update(
                                    MovieViewContentProvider.CONTENT_URI,
                                    values, null, null);
                        } else {
                            new AlertDialog.Builder(MovieViewProxySet.this)
                                    .setTitle(null)
                                    .setMessage(
                                            R.string.movie_view_proxy_ip_seterror_info)
                                    .setPositiveButton(
                                            R.string.movie_view_proxy_ok,
                                            AlertDialogPositive).show();
                            ProxyTextPreference.setText(Proxyin);
                        }
                        return true;
                    }
                });
    }

    public void setProxyPortPreferenceChangeListener(
            final EditTextPreference ProxyTextPreference,
            final String Proxytype, final String Proxyin) {
        ProxyTextPreference
                .setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
                    public boolean onPreferenceChange(Preference p,
                            Object newValue) {
                        String Proxyout = (String) newValue;
                        if (isValuedPort(Proxyout)
                                || (Proxyout == null || Proxyout.equals(""))) // clear
                        {
                            if ((newValue == null || newValue.equals(""))) {
                                Proxyout = "";
                                ProxyPortSet = "";
                            }
                            p.setSummary(Proxyout);
                            ContentValues values = new ContentValues();
                            values.put(Proxytype, Proxyout);
                            getContentResolver().update(
                                    MovieViewContentProvider.CONTENT_URI,
                                    values, null, null);
                        } else {
                            String seterror_info;
                            if ((MaxPort == null || MaxPort.equals(""))
                                    || (MinPort == null || MinPort.equals(""))) {
                                seterror_info = (getResources()
                                        .getString(R.string.movie_view_rtp_rtcp_port_set_first_error_info));
                            } else {
                                seterror_info = (getResources()
                                        .getString(R.string.movie_view_proxy_port_seterror_info))
                                        + MinPort + "~" + MaxPort;
                            }
                            new AlertDialog.Builder(MovieViewProxySet.this)
                                    .setTitle(null).setMessage(seterror_info)
                                    .setPositiveButton(
                                            R.string.movie_view_proxy_ok,
                                            AlertDialogPositive).show();
                            ProxyTextPreference.setText(Proxyin);
                        }
                        return true;
                    }
                });
    }

    public void setMaxMinPortPreferenceChangeListener(
            final EditTextPreference ProxyTextPreference,
            final String Proxytype, final String Proxyin) {
        ProxyTextPreference
                .setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
                    public boolean onPreferenceChange(Preference p,
                            Object newValue) {
                        String Proxyout = (String) newValue;
                        if (isValuedPort(Proxyout, Proxytype)
                                || (newValue == null || newValue.equals(""))) {
                            if ((newValue == null || newValue.equals(""))) {
                                if (Proxytype.equals("max_port")) {
                                    MaxPort = RTP_RTCP_MAX_PORT_VALUE + "";
                                    // set RTP default max value to 65535 when user input null
                                    Proxyout = MaxPort;
                                } else {
                                    MinPort = RTP_RTCP_MIN_PORT_VALUE + "";
                                    // set RTP default min value to 1024 when user input null 
                                    Proxyout = MinPort;
                                }
                            }
                            p.setSummary(Proxyout);
                            ContentValues values = new ContentValues();
                            values.put(Proxytype, Proxyout);
                            getContentResolver().update(
                                    MovieViewContentProvider.CONTENT_URI,
                                    values, null, null);
                        } else {
                            new AlertDialog.Builder(MovieViewProxySet.this)
                                    .setTitle(null)
                                    .setMessage(
                                            R.string.movie_view_rtp_rtcp_port_set_seterror_info)
                                    .setPositiveButton(
                                            R.string.movie_view_proxy_ok,
                                            AlertDialogPositive).show();
                            ProxyTextPreference.setText(Proxyin);
                        }
                        return true;
                    }
                });
    }

    public void loadMovieViewContentProvider() {
        Cursor cursor = getContentResolver().query(
                MovieViewContentProvider.CONTENT_URI, CONTENT_PROJECTION, null,
                null, null);
        if (cursor != null) {
            try {
                int colidx, proxy_enable;
                cursor.moveToFirst();
                colidx = cursor.getColumnIndex("max_port");
                MaxPort = cursor.getString(colidx);
                colidx = cursor.getColumnIndex("min_port");
                MinPort = cursor.getString(colidx);
                colidx = cursor.getColumnIndex("http_proxy");
                ProxyIpAdress = cursor.getString(colidx);
                colidx = cursor.getColumnIndex("http_port");
                ProxyPortSet = cursor.getString(colidx);
                colidx = cursor.getColumnIndex("proxy_enable");
                Log.i(LOG_TAG, "loadMovieViewContentProvider proxy_enable"
                        + cursor.getString(colidx));
                proxy_enable = Integer.parseInt(cursor.getString(colidx));
                if (proxy_enable != 0) {
                    ProxyEnble = true;
                } else {
                    ProxyEnble = false;
                }
            } finally {
                cursor.close();
            }
        }
    }

    public void updateMovieViewContentProvider() {
        loadMovieViewContentProvider();
        mProxyIpAdress.setText(ProxyIpAdress);
        mProxyIpAdress.setSummary(checkNull(mProxyIpAdress.getText()));
        mProxyPortSet.setText(ProxyPortSet);
        mProxyPortSet.setSummary(checkNull(mProxyPortSet.getText()));
        mRtcpMaxPort.setText(MaxPort);
        mRtcpMaxPort.setSummary(checkNull(mRtcpMaxPort.getText()));
        mRtcpMinPort.setText(MinPort);
        mRtcpMinPort.setSummary(checkNull(mRtcpMinPort.getText()));
        if (ProxyEnble) {
            mProxyCheck.setChecked(true);
            mProxyIpAdress.setEnabled(true);
            mProxyPortSet.setEnabled(true);
        } else {
            mProxyCheck.setChecked(false);
            mProxyIpAdress.setEnabled(false);
            mProxyPortSet.setEnabled(false);
        }
    }

    private boolean isIpAddress(String address) {
        if (address == null || address.equals(""))
            return false;
        if (Patterns.IP_ADDRESS.matcher(address).matches()) {
            ProxyIpAdress = address;
            return true;
        }
        return false;
    }

    private boolean isValuedPort(String port) {
        if (port == null || port.equals("") || port.length() > 5)
            return false;
        int a = Integer.parseInt(port);
        Log.i(LOG_TAG, "isValuedPort" + a);
        int min_port = Integer
                .parseInt((MinPort == null || MinPort.equals("")) ? "0"
                        : MinPort);
        int max_port = Integer
                .parseInt((MaxPort == null || MaxPort.equals("")) ? "0"
                        : MaxPort);
        if (a >= min_port && a <= max_port) {
            ProxyPortSet = port;
            return true;
        }
        return false;
    }
    /**
     * Proxy port Value verify
     * @param port the int value between 1024~65535
     * @param type the String value "max_port" or "min_port"
     * @return boolean whether the port value is right
     */
    private boolean isValuedPort(String port, String type) {
        if (port == null || port.equals("") || port.length() > 5)
            return false;
        int portNum = Integer.parseInt(port);
        Log.i(LOG_TAG, "isValuedPort" + portNum);
        // current MinPort Value
        int min_port = Integer
                .parseInt((MinPort == null || MinPort.equals("")) ? RTP_RTCP_MIN_PORT_VALUE + ""
                        : MinPort);
        // current MaxPort value
        int max_port = Integer
                .parseInt((MaxPort == null || MaxPort.equals("")) ? RTP_RTCP_MAX_PORT_VALUE + ""
                        : MaxPort);

        if (type.equals("max_port")) {
         // SPRD: add for bug 605227 verify maxport number to 1024~65535
            if (portNum >= RTP_RTCP_MIN_PORT_VALUE && portNum <= RTP_RTCP_MAX_PORT_VALUE
                    && portNum >= min_port) {
                MaxPort = port;
                return true;
            }
        } else if (type.equals("min_port")) {
         // SPRD: add for bug 605227 verify maxport number to 1024~65535  461
            if (portNum >= RTP_RTCP_MIN_PORT_VALUE && portNum <= RTP_RTCP_MAX_PORT_VALUE
                    && portNum <= max_port) {
                    MinPort = port;
                    return true;
            }
        }
        return false;
    }

    private class MovieViewProxySetChangeObserver extends ContentObserver {
        public MovieViewProxySetChangeObserver() {
            super(new Handler());
            Log.i(LOG_TAG, "MovieViewProxySetChangeObserver");
        }
        @Override
        public void onChange(boolean selfChange) {
            Log.i(LOG_TAG, "MovieViewProxySetChangeObserver " + selfChange);
            updateMovieViewContentProvider();
        }
    }

}
