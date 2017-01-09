
package com.sprd.engineermode.debuglog.slogui;

/* BUG540993 2016/05/11 sprd:EngineerMode add the parameter of CAP log packet length feature */
import android.util.Log;
import android.widget.Toast;
import android.content.Intent;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.preference.Preference;
import android.preference.EditTextPreference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.DialogInterface;
import com.sprd.engineermode.R;

public class CapPacketSettingActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener {

    private static final String TAG = "CapPacketSettingActivity";
    private static final String KEY_CAP_LOG_PACKET = "cap_log_packet";
    private static final int MAX_CAP_LOG_LENGTH = 800;
    private static final int MIN_CAP_LOG_LENGTH = 0;

    private static final int GET_CAP_LOG_PACKET = 0;
    private static final int SET_CAP_LOG_PACKET = 1;

    private EditTextPreference mCapLogPacket;

    private Handler mUiThread = new Handler();
    private CapSettingHandler capSettingHandler;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_cap_packet);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        capSettingHandler = new CapSettingHandler(ht.getLooper());
        mCapLogPacket = (EditTextPreference) findPreference(KEY_CAP_LOG_PACKET);
        mCapLogPacket.setOnPreferenceChangeListener(this);
    }

    @Override
    public void onStart() {
        super.onStart();
        if (mCapLogPacket != null) {
            Message caploglength = capSettingHandler.obtainMessage(GET_CAP_LOG_PACKET);
            capSettingHandler.sendMessage(caploglength);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object objValue) {
        final String re = String.valueOf(objValue);
        if (pref == mCapLogPacket) {
            Message seCapLogPacket = capSettingHandler.obtainMessage(
                    SET_CAP_LOG_PACKET, re);
            capSettingHandler.sendMessage(seCapLogPacket);
            return true;
        }
        return false;
    }

    class CapSettingHandler extends Handler {

        public CapSettingHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String atResponse;
            String responValue;
            switch (msg.what) {
                case GET_CAP_LOG_PACKET: {
                    atResponse = IATUtils.sendATCmd(engconstents.ENG_AT_GETCAPLOGLENGTH,
                            "atchannel0");
                    Log.d(TAG, "GET_CAP_LOG_PACKET atResponse:" + atResponse);
                    if (atResponse != null && atResponse.contains(IATUtils.AT_OK)) {
                        String[] str = atResponse.split("\n");
                        String[] str1 = str[0].split(":");
                        if (str1.length < 2) {
                            return;
                        }
                        final String str2 = str1[1].trim();
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mCapLogPacket.setSummary(str2);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mCapLogPacket.setEnabled(false);
                                mCapLogPacket.setSummary(R.string.feature_not_support);
                            }
                        });
                    }
                }
                    break;
                case SET_CAP_LOG_PACKET: {
                    final String setCapLogSize = (String) msg.obj;
                    Log.d(TAG, "setCapLogSize: " + setCapLogSize);
                    if ("".equals(setCapLogSize)
                            || Integer.valueOf(setCapLogSize) > MAX_CAP_LOG_LENGTH
                            || Integer.valueOf(setCapLogSize) < MIN_CAP_LOG_LENGTH) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(CapPacketSettingActivity.this,
                                        "Please input between " + MIN_CAP_LOG_LENGTH + "~"
                                                + MAX_CAP_LOG_LENGTH,
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    } else {
                        atResponse = IATUtils.sendATCmd(engconstents.ENG_AT_SETCAPLOGLENGTH
                                + setCapLogSize,
                                "atchannel0");
                        if (atResponse != null && atResponse.contains(IATUtils.AT_OK)) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mCapLogPacket.setSummary(setCapLogSize);
                                    Toast.makeText(CapPacketSettingActivity.this, "Set Success",
                                            Toast.LENGTH_SHORT).show();
                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    Toast.makeText(CapPacketSettingActivity.this, "Set Fail",
                                            Toast.LENGTH_SHORT).show();
                                }
                            });
                        }
                    }

                }
                    break;
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (capSettingHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            capSettingHandler.getLooper().quit();
        }

    }

}
