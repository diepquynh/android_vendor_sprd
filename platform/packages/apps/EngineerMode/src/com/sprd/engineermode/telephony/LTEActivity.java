package com.sprd.engineermode.telephony;

import android.app.AlertDialog;
import android.app.PendingIntent;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.telephony.TelephonyManager;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;
import com.sprd.engineermode.telephony.TelephonyManagerSprd.RadioCapbility;
import android.util.Log;
import android.widget.Toast;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.content.SharedPreferences.Editor;
import android.content.Context;
import android.os.PowerManager;
import com.sprd.engineermode.R;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.PhoneFactory;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.utils.SocketUtils;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;

import com.android.internal.telephony.ITelephony;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.telephony.TelephonyManager;
import android.telephony.SubscriptionManager;
import android.app.NotificationManager;
import android.app.Notification;
import android.widget.RemoteViews;

public class LTEActivity extends PreferenceActivity implements
        OnPreferenceChangeListener {

    private static final String TAG = "LTEActivity";
    private static final String KEY_SVLTE = "svlte_key";
    private static final String KEY_FDD = "fdd_csfb_key";
    private static final String KEY_TDD = "tdd_csfb_key";
    private static final String KEY_CSFB = "csfb_key";
    private static final String KEY_TDD_SVLTE = "TDD_SVLTE";
    private static final String KEY_FDD_CSFB = "FDD_CSFB";
    private static final String KEY_TDD_CSFB = "TDD_CSFB";
    private static final String KEY_LTE_CSFB = "CSFB";
    private static final String KEY_SIM_INDEX = "simindex";
    private static final String CHANGE_NETMODE_BY_EM = "persist.sys.cmccpolicy.disable";
    private static final String NEW_MESSAGE = "Notice From EngineerMode!";
    private static final String CLOSE_NOTICE ="After test,please close this notificaiton";
    private final int NOTIFICATION_ID = 0x1123;
    private boolean isSupportTDD = TelephonyManagerSprd.RadioCapbility.TDD_CSFB
            .equals(TelephonyManagerSprd.getRadioCapbility());
    private ListPreference mListPreferenceSVLTE;
    private ListPreference mListPreferenceFDD;
    private ListPreference mListPreferenceTDD;
    private ListPreference mListPreferenceCSFB;
    private RadioCapbility mCurrentRadioCapbility;
    private int mCurrentRadioFeatures;
    private SharedPreferences mSharePref;
    private TelephonyManagerSprd mTelephonyManager;
    private SubscriptionManager mSubscriptionManager;
    private ProgressDialog mProgressDialog;
    private Context mContext;
    private int mPhoneId = -1;
    private Handler mUiThread = new Handler();
    private NotificationManager mNotifiCation;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;
        addPreferencesFromResource(R.xml.pref_lte);

        mListPreferenceSVLTE = (ListPreference) findPreference(KEY_SVLTE);
        mListPreferenceFDD = (ListPreference) findPreference(KEY_FDD);
        mListPreferenceTDD = (ListPreference) findPreference(KEY_TDD);
        mListPreferenceCSFB = (ListPreference) findPreference(KEY_CSFB);

        PreferenceScreen prefSet = getPreferenceScreen();

        if(!SystemProperties.get("persist.radio.engtest.enable").contains("true")){
            SystemProperties.set("persist.radio.engtest.enable", "true");
            Log.d(TAG, "get vSystemProperties is " +SystemProperties.get("persist.radio.engtest.enable"));
            initNotifiCation();
        }
        mListPreferenceSVLTE.setOnPreferenceChangeListener(this);
        mListPreferenceFDD.setOnPreferenceChangeListener(this);
        mListPreferenceTDD.setOnPreferenceChangeListener(this);
        mListPreferenceCSFB.setOnPreferenceChangeListener(this);

        mSharePref = PreferenceManager.getDefaultSharedPreferences(this);

        mCurrentRadioCapbility = TelephonyManagerSprd.getRadioCapbility();
        mTelephonyManager = new TelephonyManagerSprd(LTEActivity.this);
        mCurrentRadioFeatures = mTelephonyManager.getPreferredNetworkType();
        Log.d(TAG, "mCurrentRadioCapbility is " + mCurrentRadioCapbility.toString()
                + ", mCurrentRadioFeatures is " + mCurrentRadioFeatures);

        if (mCurrentRadioCapbility.toString().equals(KEY_TDD_SVLTE)) {
            prefSet.removePreference(mListPreferenceFDD);
            prefSet.removePreference(mListPreferenceTDD);
            prefSet.removePreference(mListPreferenceCSFB);
            mListPreferenceSVLTE.setValueIndex(changeValueToIndex(KEY_SVLTE));
            mListPreferenceSVLTE.setSummary(mListPreferenceSVLTE.getEntry());
            mListPreferenceFDD.setValue(null);
            mListPreferenceTDD.setValue(null);
            mListPreferenceCSFB.setValue(null);
        } else if (mCurrentRadioCapbility.toString().equals(KEY_FDD_CSFB)) {
            prefSet.removePreference(mListPreferenceSVLTE);
            prefSet.removePreference(mListPreferenceTDD);
            prefSet.removePreference(mListPreferenceCSFB);
            mListPreferenceFDD.setValueIndex(changeValueToIndex(KEY_FDD));
            mListPreferenceFDD.setSummary(mListPreferenceFDD.getEntry());
            mListPreferenceSVLTE.setValue(null);
            mListPreferenceTDD.setValue(null);
            mListPreferenceCSFB.setValue(null);
        } else if (mCurrentRadioCapbility.toString().equals(KEY_TDD_CSFB)) {
            prefSet.removePreference(mListPreferenceSVLTE);
            prefSet.removePreference(mListPreferenceFDD);
            prefSet.removePreference(mListPreferenceCSFB);
            mListPreferenceTDD.setEntries(R.array.list_entries_tdd_csfb_change);
            mListPreferenceTDD
                    .setEntryValues(R.array.list_entriesvalues_tdd_csfb_change);
            mListPreferenceTDD.setValueIndex(changeValueToIndex(KEY_TDD));
            mListPreferenceTDD.setSummary(mListPreferenceTDD.getEntry());
            mListPreferenceSVLTE.setValue(null);
            mListPreferenceFDD.setValue(null);
            mListPreferenceCSFB.setValue(null);
        } else if (mCurrentRadioCapbility.toString().equals(KEY_LTE_CSFB)) {
            prefSet.removePreference(mListPreferenceSVLTE);
            prefSet.removePreference(mListPreferenceTDD);
            prefSet.removePreference(mListPreferenceFDD);
            mListPreferenceCSFB.setValueIndex(changeValueToIndex(KEY_CSFB));
            mListPreferenceCSFB.setSummary(mListPreferenceCSFB.getEntry());
            mListPreferenceSVLTE.setValue(null);
            mListPreferenceTDD.setValue(null);
            mListPreferenceFDD.setValue(null);
        } else {
            mListPreferenceSVLTE.setValue(null);
            mListPreferenceFDD.setValue(null);
            mListPreferenceTDD.setValue(null);
        }
    }

    private void initNotifiCation() {
        mNotifiCation = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
        RemoteViews mRemoteViews = new RemoteViews(this.getPackageName(), R.layout.notification);
        mRemoteViews.setTextViewText(R.id.mt_notification,NEW_MESSAGE+"\n"+"    "+CLOSE_NOTICE);
        Intent clickIntent = new Intent(this, NotificationBroadcastReceiver.class);
        clickIntent.putExtra("notificationId", NOTIFICATION_ID);
        PendingIntent pendingIntent= PendingIntent.getBroadcast(this, 0, clickIntent, PendingIntent.FLAG_UPDATE_CURRENT);
        mRemoteViews.setOnClickPendingIntent(R.id.bt_notification, pendingIntent);
        Notification notify = new Notification.Builder(this)
            .setTicker(NEW_MESSAGE)
            .setContent(mRemoteViews)
            .setAutoCancel(true)
            .setContentTitle(NEW_MESSAGE)
            .setPriority(Notification.PRIORITY_DEFAULT)
            .setOngoing(true)
            .setSmallIcon(R.drawable.cg)
            .build();
        mNotifiCation.notify(NOTIFICATION_ID,notify);
    }
    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    /** BEGIN BUG:543427 zhijie.yang 2016/04/06 modify the method of the switch network **/
    private int changeValueToIndex(String PrefKey) {
        int valueIndex = 0;
        if (PrefKey.equals(KEY_FDD)) {
            if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_LTE_FDD_WCDMA_GSM) {
                valueIndex = 0;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_TD_LTE_WCDMA_GSM) {
                valueIndex = 1;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_TD_LTE) {
                valueIndex = 2;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_LTE_FDD) {
                valueIndex = 3;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_LTE_FDD_TD_LTE) {
                valueIndex = 4;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_LTE_FDD_TD_LTE_WCDMA_GSM) {
                valueIndex = 5;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_GSM) {
                valueIndex = 6;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_WCDMA) {
                valueIndex = 7;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_WCDMA_GSM) {
                valueIndex = 8;
            }
            return valueIndex;
        } else if (PrefKey.equals(KEY_TDD)) {
            if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_TD_LTE_TDSCDMA_GSM) {
                valueIndex = 0;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_TD_LTE) {
                valueIndex = 1;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_LTE_FDD) {
                valueIndex = 2;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_LTE_FDD_TD_LTE) {
                valueIndex = 3;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_LTE_FDD_TD_LTE_TDSCDMA_GSM) {
                valueIndex = 4;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_GSM) {
                if (isSupportTDD) {
                    valueIndex = 2;
                } else {
                    valueIndex = 5;
                }
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_TDSCDMA) {
                if (isSupportTDD) {
                    valueIndex = 3;
                } else {
                    valueIndex = 6;
                }
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_TDSCDMA_GSM) {
                if (isSupportTDD) {
                    valueIndex = 4;
                } else {
                    valueIndex = 7;
                }
            }
            return valueIndex;
        } else if (PrefKey.equals(KEY_CSFB)) {
            if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_LTE_FDD_TD_LTE_WCDMA_TDSCDMA_GSM) {
                valueIndex = 0;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_LTE_FDD_TD_LTE_WCDMA_GSM) {
                valueIndex = 1;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_LTE_FDD_WCDMA_GSM) {
                valueIndex = 2;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_TD_LTE_WCDMA_GSM) {
                valueIndex = 3;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_TD_LTE) {
                valueIndex = 4;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_LTE_FDD) {
                valueIndex = 5;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_LTE_FDD_TD_LTE) {
                valueIndex = 6;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_TD_LTE_TDSCDMA_GSM) {
                valueIndex = 7;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_LTE_FDD_TD_LTE_TDSCDMA_GSM) {
                valueIndex = 8;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_GSM) {
                valueIndex = 9;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_WCDMA) {
                valueIndex = 10;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_WCDMA_GSM) {
                valueIndex = 11;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_TDSCDMA) {
                valueIndex = 12;
            } else if (mCurrentRadioFeatures == TelephonyManagerSprd.NT_TDSCDMA_GSM) {
                valueIndex = 13;
            }
        }
        return valueIndex;
    }

    private int changeIndexToValue(RadioCapbility radio,
            int setValueIndex) {
        int setRadioFeature = -1;
        if (radio.equals(TelephonyManagerSprd.RadioCapbility.FDD_CSFB)) {
            switch (setValueIndex) {
                case 0:
                    setRadioFeature = TelephonyManagerSprd.NT_LTE_FDD_WCDMA_GSM;
                    break;
                case 1:
                    setRadioFeature = TelephonyManagerSprd.NT_TD_LTE_WCDMA_GSM;
                    break;
                case 2:
                    setRadioFeature = TelephonyManagerSprd.NT_TD_LTE;
                    break;
                case 3:
                    setRadioFeature = TelephonyManagerSprd.NT_LTE_FDD;
                    break;
                case 4:
                    setRadioFeature = TelephonyManagerSprd.NT_LTE_FDD_TD_LTE;
                    break;
                case 5:
                    setRadioFeature = TelephonyManagerSprd.NT_LTE_FDD_TD_LTE_WCDMA_GSM;
                    break;
                case 6:
                    setRadioFeature = TelephonyManagerSprd.NT_GSM;
                    break;
                case 7:
                    setRadioFeature = TelephonyManagerSprd.NT_WCDMA;
                    break;
                case 8:
                    setRadioFeature = TelephonyManagerSprd.NT_WCDMA_GSM;
                    break;
            }
            return setRadioFeature;
        } else if (radio.equals(TelephonyManagerSprd.RadioCapbility.TDD_CSFB)) {
            switch (setValueIndex) {
                case 0:
                    setRadioFeature = TelephonyManagerSprd.NT_TD_LTE_TDSCDMA_GSM;
                    break;
                case 1:
                    setRadioFeature = TelephonyManagerSprd.NT_TD_LTE;
                    break;
                case 2:
                    if (isSupportTDD) {
                        setRadioFeature = TelephonyManagerSprd.NT_GSM;
                    } else {
                        setRadioFeature = TelephonyManagerSprd.NT_LTE_FDD;
                    }
                    break;
                case 3:
                    if (isSupportTDD) {
                        setRadioFeature = TelephonyManagerSprd.NT_TDSCDMA;
                    } else {
                        setRadioFeature = TelephonyManagerSprd.NT_LTE_FDD_TD_LTE;
                    }
                    break;
                case 4:
                    if (isSupportTDD) {
                        setRadioFeature = TelephonyManagerSprd.NT_TDSCDMA_GSM;
                    } else {
                        setRadioFeature = TelephonyManagerSprd.NT_LTE_FDD_TD_LTE_TDSCDMA_GSM;
                    }
                    break;
                case 5:
                    setRadioFeature = TelephonyManagerSprd.NT_GSM;
                    break;
                case 6:
                    setRadioFeature = TelephonyManagerSprd.NT_TDSCDMA;
                    break;
                case 7:
                    setRadioFeature = TelephonyManagerSprd.NT_TDSCDMA_GSM;
                    break;
            }
            return setRadioFeature;
        } else if (radio.equals(TelephonyManagerSprd.RadioCapbility.CSFB)) {
            switch (setValueIndex) {
                case 0:
                    setRadioFeature = TelephonyManagerSprd.NT_LTE_FDD_TD_LTE_WCDMA_TDSCDMA_GSM;
                    break;
                case 1:
                    setRadioFeature = TelephonyManagerSprd.NT_LTE_FDD_TD_LTE_WCDMA_GSM;
                    break;
                case 2:
                    setRadioFeature = TelephonyManagerSprd.NT_LTE_FDD_WCDMA_GSM;
                    break;
                case 3:
                    setRadioFeature = TelephonyManagerSprd.NT_TD_LTE_WCDMA_GSM;
                    break;
                case 4:
                    setRadioFeature = TelephonyManagerSprd.NT_TD_LTE;
                    break;
                case 5:
                    setRadioFeature = TelephonyManagerSprd.NT_LTE_FDD;
                    break;
                case 6:
                    setRadioFeature = TelephonyManagerSprd.NT_LTE_FDD_TD_LTE;
                    break;
                case 7:
                    setRadioFeature = TelephonyManagerSprd.NT_TD_LTE_TDSCDMA_GSM;
                    break;
                case 8:
                    setRadioFeature = TelephonyManagerSprd.NT_LTE_FDD_TD_LTE_TDSCDMA_GSM;
                    break;
                case 9:
                    setRadioFeature = TelephonyManagerSprd.NT_GSM;
                    break;
                case 10:
                    setRadioFeature = TelephonyManagerSprd.NT_WCDMA;
                    break;
                case 11:
                    setRadioFeature = TelephonyManagerSprd.NT_WCDMA_GSM;
                    break;
                case 12:
                    setRadioFeature = TelephonyManagerSprd.NT_TDSCDMA;
                    break;
                case 13:
                    setRadioFeature = TelephonyManagerSprd.NT_TDSCDMA_GSM;
                    break;
            }
            return setRadioFeature;
        }
        return setRadioFeature;
    }

    /** END BUG:543427 zhijie.yang 2016/04/06 modify the method of the switch network **/

    private void setSummary(RadioCapbility radio) {
        if (radio == null) {
            return;
        } else {
            if (radio.equals(TelephonyManagerSprd.RadioCapbility.TDD_SVLTE)) {
                mListPreferenceSVLTE
                        .setSummary(mListPreferenceSVLTE.getEntry());
            } else if (radio
                    .equals(TelephonyManagerSprd.RadioCapbility.FDD_CSFB)) {
                mListPreferenceFDD.setSummary(mListPreferenceFDD.getEntry());
            } else if (radio
                    .equals(TelephonyManagerSprd.RadioCapbility.TDD_CSFB)) {
                mListPreferenceTDD.setSummary(mListPreferenceTDD.getEntry());
            } else if (radio.equals(TelephonyManagerSprd.RadioCapbility.CSFB)) {
                mListPreferenceCSFB.setSummary(mListPreferenceCSFB.getEntry());
            }
        }
    }

    private void retrievePrefIndex(RadioCapbility radio, int lastValueIndex) {
        if (radio == null) {
            return;
        } else {
            if (radio.equals(TelephonyManagerSprd.RadioCapbility.FDD_CSFB)) {
                mListPreferenceFDD.setValueIndex(lastValueIndex);
                SharedPreferences.Editor edit = mSharePref.edit();
                edit.putString(KEY_FDD, mListPreferenceFDD.getValue());
                edit.commit();
            } else if (radio.equals(TelephonyManagerSprd.RadioCapbility.TDD_CSFB)) {
                mListPreferenceTDD.setValueIndex(lastValueIndex);
                SharedPreferences.Editor edit = mSharePref.edit();
                edit.putString(KEY_TDD, mListPreferenceTDD.getValue());
                edit.commit();
            } else if (radio.equals(TelephonyManagerSprd.RadioCapbility.CSFB)) {
                mListPreferenceCSFB.setValueIndex(lastValueIndex);
                SharedPreferences.Editor edit = mSharePref.edit();
                edit.putString(KEY_CSFB, mListPreferenceCSFB.getValue());
                edit.commit();
            }
        }
    }

    private void retrieveCapbility(RadioCapbility radio) {
        if (radio == null) {
            return;
        } else {
            if (radio.equals(TelephonyManagerSprd.RadioCapbility.TDD_SVLTE)) {
                mListPreferenceSVLTE.setValue(null);
            } else if (radio
                    .equals(TelephonyManagerSprd.RadioCapbility.FDD_CSFB)) {
                mListPreferenceFDD.setValue(null);
            } else if (radio
                    .equals(TelephonyManagerSprd.RadioCapbility.TDD_CSFB)) {
                mListPreferenceTDD.setValue(null);
            } else if (radio.equals(TelephonyManagerSprd.RadioCapbility.CSFB)) {
                mListPreferenceCSFB.setValue(null);
            }
        }
    }

    /** BEGIN BUG:543427 zhijie.yang 2016/04/06 modify the method of the switch network **/
    public AlertDialog showAlertDialog(final RadioCapbility mRadio,
            final int setValueIndex, final int valueIndex) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        final int setRadioFeature = changeIndexToValue(mRadio,
                setValueIndex);
        Log.d(TAG, "setRadioFeature is " + setRadioFeature);
        builder.setMessage(getString(R.string.setting_dialog_con1))
                .setCancelable(false)
                .setPositiveButton(android.R.string.yes,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                setRadioFeatures(setRadioFeature);
                            }
                        })
                .setNegativeButton(android.R.string.no,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                retrievePrefIndex(mRadio, valueIndex);
                            }
                        });
        AlertDialog alert = builder.create();
        alert.show();
        return alert;
    }

    public void setRadioFeatures(int setRadioFeature) {
        Log.d(TAG, "now setRadioFeatures: " + setRadioFeature);
        mTelephonyManager.setPreferredNetworkType(setRadioFeature);
        if (mCurrentRadioCapbility == TelephonyManagerSprd.RadioCapbility.TDD_CSFB) {
            if (setRadioFeature == TelephonyManagerSprd.NT_TD_LTE_TDSCDMA_GSM) {
                SystemProperties.set(CHANGE_NETMODE_BY_EM, "false");
            } else {
                SystemProperties.set(CHANGE_NETMODE_BY_EM, "true");
            }
        } else if (mCurrentRadioCapbility == TelephonyManagerSprd.RadioCapbility.FDD_CSFB) {
            if (setRadioFeature == TelephonyManagerSprd.NT_LTE_FDD_TD_LTE_WCDMA_GSM) {
                SystemProperties.set(CHANGE_NETMODE_BY_EM, "false");
            } else {
                SystemProperties.set(CHANGE_NETMODE_BY_EM, "true");
            }
        } else if (mCurrentRadioCapbility == TelephonyManagerSprd.RadioCapbility.CSFB) {
            if (setRadioFeature == TelephonyManagerSprd.NT_LTE_FDD_TD_LTE_WCDMA_TDSCDMA_GSM) {
                SystemProperties.set(CHANGE_NETMODE_BY_EM, "false");
            } else {
                SystemProperties.set(CHANGE_NETMODE_BY_EM, "true");
            }
        }
        setSummary(mCurrentRadioCapbility);
    }

    /** END BUG:543427 zhijie.yang 2016/04/06 modify the method of the switch network **/

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        int setValueIndex = 0;
        int valueIndex = 0;
        Log.d(TAG, "get vSystemProperties is " +SystemProperties.get("persist.radio.engtest.enable"));
        mCurrentRadioCapbility = TelephonyManagerSprd.getRadioCapbility();
        mCurrentRadioFeatures = mTelephonyManager.getPreferredNetworkType();
        Log.d(TAG, "onPreferenceChange" + "\n" + "mCurrentRadioCapbility is "
                + mCurrentRadioCapbility + ", mCurrentRadioFeatures is "
                + mCurrentRadioFeatures);
        setValueIndex = Integer.valueOf(newValue.toString());
        if (mCurrentRadioCapbility
                .equals(TelephonyManagerSprd.RadioCapbility.FDD_CSFB)) {
            valueIndex = changeValueToIndex(KEY_FDD);
        } else if (mCurrentRadioCapbility
                .equals(TelephonyManagerSprd.RadioCapbility.TDD_CSFB)) {
            valueIndex = changeValueToIndex(KEY_TDD);
        } else if (mCurrentRadioCapbility
                .equals(TelephonyManagerSprd.RadioCapbility.CSFB)) {
            valueIndex = changeValueToIndex(KEY_CSFB);
        }
        if (preference instanceof ListPreference) {
            RadioCapbility radioCapbility = null;
            ListPreference listPreference = (ListPreference) preference;
            String key = listPreference.getKey();
            if (key.equals(KEY_FDD)) {
                radioCapbility = TelephonyManagerSprd.RadioCapbility.FDD_CSFB;
            } else if (key.equals(KEY_CSFB)) {
                radioCapbility = TelephonyManagerSprd.RadioCapbility.CSFB;
            } else {
                radioCapbility = TelephonyManagerSprd.RadioCapbility.TDD_CSFB;
            }
            if (setValueIndex != valueIndex) {
                showAlertDialog(radioCapbility, setValueIndex, valueIndex);
            }
            Log.d(TAG, "mCurrentRadioCapbility is " + mCurrentRadioCapbility
                    + ", changeradioCapbility is " + radioCapbility
                    + ", setValueIndex is " + setValueIndex
                    + ", valueIndex is " + valueIndex + ", key is " + key);
        }
        return true;
    }

}
