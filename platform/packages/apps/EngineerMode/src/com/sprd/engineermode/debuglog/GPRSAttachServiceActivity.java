
package com.sprd.engineermode.debuglog;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.TwoStatePreference;
import com.sprd.engineermode.EMSwitchPreference;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;
import android.content.Context;

import android.telephony.TelephonyManager;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.R.xml;
import com.sprd.engineermode.utils.IATUtils;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.SystemProperties;
import android.telephony.SubscriptionManager;
import android.view.KeyEvent;

public class GPRSAttachServiceActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener {

    private static final String TAG = "GPRSAttachServiceActivity";

    private static final int GET_GPRS = 1;
    private static final int SET_GPRSATTACH = 2;
    private static final int SET_GPRSDETACH = 3;

    private Handler mUiThread = new Handler();
    private GPRSHandler mGPRSHandler;

    private int mPhoneCount;
    private int mSetSim;
    PreferenceGroup mPreGroup = null;
    private TwoStatePreference mGPRSSetSwitch;
    private TelephonyManager[] mTelephonyManager;
    private Context mContext;
    private Toast toast = null;
    private static final String PROP_ATTACH_ENABLE = "persist.sys.attach.enable";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mContext=this;
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mGPRSHandler = new GPRSHandler(ht.getLooper());

        mPhoneCount = TelephonyManager.from(this).getPhoneCount();
        Log.d(TAG, "mPhoneCount is " + mPhoneCount);
        mTelephonyManager = new TelephonyManager[mPhoneCount];

        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
        mPreGroup = getPreferenceScreen();

        //mGPRSSetSwitch = new TwoStatePreference;
        /*for (int i = 0; i < mPhoneCount; i++) {
            mGPRSSetSwitch[i] = new EMSwitchPreference(this, null);
            mGPRSSetSwitch[i].setTitle("SIM" + i);
            mGPRSSetSwitch[i].setKey("SIM" + i);
            mGPRSSetSwitch[i].setOnPreferenceChangeListener(this);
            mPreGroup.addPreference(mGPRSSetSwitch[i]);
        }*/
        mGPRSSetSwitch = new EMSwitchPreference(this, null);
        mGPRSSetSwitch.setTitle("SIM");
        mGPRSSetSwitch.setKey("SIM");
        mGPRSSetSwitch.setOnPreferenceChangeListener(this);
        mPreGroup.addPreference(mGPRSSetSwitch);

        /*for (int i = 0; i < mPhoneCount; i++) {
            mTelephonyManager[i] = TelephonyManager.from(this);
            if (mTelephonyManager[i] != null
                    && mTelephonyManager[i].getSimState(i) == TelephonyManager.SIM_STATE_READY) {
                mGPRSSetSwitch[i].setEnabled(true);
            } else {
                mGPRSSetSwitch[i].setEnabled(false);
                mGPRSSetSwitch[i].setSummary(R.string.input_card_to_test);
            }
        }*/
    }

    @Override
    protected void onStart() {
        final SubscriptionManager subscriptionManager = SubscriptionManager.from(this);
        mSetSim=subscriptionManager.getDefaultDataPhoneId();
        Message mGPRSGetMessage = mGPRSHandler.obtainMessage(GET_GPRS, mSetSim, 0);
        mGPRSHandler.sendMessage(mGPRSGetMessage);
        /*for (int i = 0; i < mPhoneCount; i++) {
            Message mGPRSGetMessage = mGPRSHandler.obtainMessage(GET_GPRS, i, 0);
            mGPRSHandler.sendMessage(mGPRSGetMessage);
        }*/
        super.onStart();
    }

    @Override
    protected void onPause() {
        if(toast != null){
           toast.cancel();
        }
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        if (mGPRSHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mGPRSHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    /*private int checkAttachCount() {
        int attachCount = 0;
        for (int i = 0; i < mPhoneCount; i++) {
            if (mGPRSSetSwitch[i].isChecked()) {
                attachCount++;
            }
        }
        return attachCount;
    }*/

    private void alertUnattachDialog() {
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setMessage("Only one sim is attached");
        alert.setCancelable(false);
        alert.setNeutralButton("Confirm",
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                    }
                });
        alert.create();
        alert.show();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (preference == null) {
            return false;
        } else {
            String prefKey = preference.getKey();
            int messageType;
            /*for (int i = 0; i < mPhoneCount; i++) {
                if (prefKey.equals("SIM" + i)) {
                    if (mGPRSSetSwitch[i].isChecked()) {
                        messageType = SET_GPRSDETACH;
                        Message mGPRSSetMessage = mGPRSHandler.obtainMessage(messageType, i, 0);
                        mGPRSHandler.sendMessage(mGPRSSetMessage);
                    } else {
                        if (checkAttachCount() < 1) {
                            messageType = SET_GPRSATTACH;
                            Message mGPRSSetMessage = mGPRSHandler.obtainMessage(messageType, i, 0);
                            mGPRSHandler.sendMessage(mGPRSSetMessage);
                        } else {
                            mGPRSSetSwitch[i].setChecked(false);
                            alertUnattachDialog();
                        }
                    }
                }
            }*/
            if (prefKey.equals("SIM")) {
                if (mGPRSSetSwitch.isChecked()) {
                    messageType = SET_GPRSDETACH;
                    Message mGPRSSetMessage = mGPRSHandler.obtainMessage(messageType, mSetSim, 0);
                    mGPRSHandler.sendMessage(mGPRSSetMessage);
                } else {
                    messageType = SET_GPRSATTACH;
                    Message mGPRSSetMessage = mGPRSHandler.obtainMessage(messageType, mSetSim, 0);
                    mGPRSHandler.sendMessage(mGPRSSetMessage);
                }
           }
        }
        return true;
    }

    class GPRSHandler extends Handler {

        public GPRSHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String responValue = null;
            String serverName = null;
            final int mSetSim = (int) msg.arg1;
            switch (msg.what) {
                case GET_GPRS:
                    //final int mSetSim = (int) msg.arg1;
                    serverName = "atchannel" + mSetSim;
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_GPRS1, serverName);
                    Log.d(TAG, "ATChannel is " + serverName + ", GET_GPRS is " + responValue);
                    if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                        String[] str = responValue.split("\n");
                        String str1 = str[0].split(":")[1].trim();
                        if ("1".equals(str1)) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mGPRSSetSwitch.setChecked(true);
                                    mGPRSSetSwitch.setSummary("attach");
                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                 @Override
                                 public void run() {
                                     mGPRSSetSwitch.setChecked(false);
                                     mGPRSSetSwitch.setSummary("detach");
                                 }
                             });
                       }
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mGPRSSetSwitch.setChecked(false);
                                mGPRSSetSwitch.setSummary("detach");
                            }
                        });
                    }
                    break;
                case SET_GPRSDETACH:
                   // mSetSim = (int) msg.arg1;
                    SystemProperties.set(PROP_ATTACH_ENABLE, "false");
                    toast = Toast.makeText(mContext, "please wait 10s", Toast.LENGTH_SHORT);
                    toast.show();

                    serverName = "atchannel" + mSetSim;
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_GPRS + "0", serverName);
                    Log.d(TAG, "ATChannel is " + serverName + ", SET_GPRSDETACH is " + responValue);
                    if (responValue.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mGPRSSetSwitch.setChecked(false);
                                mGPRSSetSwitch.setSummary("detach");
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mGPRSSetSwitch.setChecked(true);
                                mGPRSSetSwitch.setSummary("attach");
                            }
                        });
                    }

                    break;
                case SET_GPRSATTACH:
                   // mSetSim = (int) msg.arg1;
                        SystemProperties.set(PROP_ATTACH_ENABLE, "true");
                    serverName = "atchannel" + mSetSim;
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_GPRS + "1", serverName);
                    Log.d(TAG, "ATChannel is " + serverName + ", SET_GPRSATTACH is " + responValue);
                    if (responValue.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mGPRSSetSwitch.setChecked(true);
                                mGPRSSetSwitch.setSummary("attach");
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mGPRSSetSwitch.setChecked(false);
                                mGPRSSetSwitch.setSummary("detach");
                            }
                        });
                    }
                    break;
                default:
                    break;
            }
        }
    }
}
