
package com.sprd.engineermode.debuglog.slogui;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;

import android.content.Intent;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.DialogInterface;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.PowerManager;
import android.content.Context;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.preference.TwoStatePreference;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.widget.Toast;
import android.content.BroadcastReceiver;
import android.preference.EditTextPreference;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.debuglog.slogui.SlogAction;
import com.sprd.engineermode.debuglog.slogui.SlogConfListener;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.utils.SocketUtils;
import android.widget.EditText;
import android.text.method.NumberKeyListener;
import com.android.internal.app.IMediaContainerService;
import android.content.ServiceConnection;
import android.content.ComponentName;
import android.os.IBinder;
import android.text.InputFilter;
import android.text.InputFilter.LengthFilter;

public class SdSettingsActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener {

    private static final String TAG = "SdSettingsActivity";
    private static final String KEY_LOG_SIZE_SET = "log_size_set";
    private static final String KEY_SD_SIZE_SET = "sd_log_size_set";
    private static final String KEY_DATA_SIZE_SET = "data_log_size_set";
    private static final String KEY_LOG_OVERWRITE = "log_overwrite_enable";

    private static final int GET_LOG_OVERWRITE_STATUE = 0;
    private static final int SET_LOG_OVERWRITE_OPEN = 1;
    private static final int SET_LOG_OVERWRITE_CLOSE = 2;
    private static final int GET_LOG_SIZE = 3;
    private static final int SET_LOG_SIZE = 4;
    private static final int GET_SD_SIZE = 5;
    private static final int SET_SD_SIZE = 6;
    private static final int GET_DATA_SIZE = 7;
    private static final int SET_DATA_SIZE = 8;
    private static final int SET_MAX_LOGSIZE = 1024;
    private static final int SET_MIN_LOGSIZE = 1;
    private static final int SET_SD_FREESPACE = 12000;
    private static final int SET_MAX_DATA_LOGSIZE = 1024;

    private EditTextPreference mLogSizeSet;
    private ListPreference mSdLogSizeSet;
    private EditTextPreference mDataLogSizeSet;
    private TwoStatePreference mLogOverwrite;

    private int mSigleLogSize;
    private int mTotalSdLogSize;
    private int mTotalDataLogSize;

    private SharedPreferences mSharePref;

    IMediaContainerService mMediaContainerService;
    protected static final ComponentName DEFAULT_CONTAINER_COMPONENT = new ComponentName(
            "com.android.defcontainer",
            "com.android.defcontainer.DefaultContainerService");
    private Handler mUiThread = new Handler();
    private SdSettingsActivityHandler mSdSettingsActivityHandler;

    private SlogConfListener mSlogConfListener = null;
    protected boolean mMCSEnable;
    private ServiceConnection mMCSConnection = new ServiceConnection() {

        @Override
        public void onServiceDisconnected(ComponentName name) {
            mMCSEnable = false;
        }

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mMediaContainerService = IMediaContainerService.Stub
                    .asInterface(service);
            mMCSEnable = true;
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_sd_settings);
        mSharePref = PreferenceManager.getDefaultSharedPreferences(this);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mSdSettingsActivityHandler = new SdSettingsActivityHandler(ht.getLooper());
        mLogSizeSet = (EditTextPreference) findPreference(KEY_LOG_SIZE_SET);
        mLogSizeSet.setOnPreferenceChangeListener(this);
        mSdLogSizeSet = (ListPreference) findPreference(KEY_SD_SIZE_SET);
        mSdLogSizeSet.setOnPreferenceChangeListener(this);
        mDataLogSizeSet = (EditTextPreference) findPreference(KEY_DATA_SIZE_SET);
        mDataLogSizeSet.setOnPreferenceChangeListener(this);
        mLogOverwrite = (TwoStatePreference) findPreference(KEY_LOG_OVERWRITE);
        mLogOverwrite.setOnPreferenceChangeListener(this);
        bindService(new Intent().setComponent(DEFAULT_CONTAINER_COMPONENT),
                mMCSConnection, BIND_AUTO_CREATE);
    }

    @Override
    public void onStart() {
        super.onStart();
        if (mLogOverwrite != null) {
            Message getLogOverwriteStatue = mSdSettingsActivityHandler
                    .obtainMessage(GET_LOG_OVERWRITE_STATUE);
            mSdSettingsActivityHandler.sendMessage(getLogOverwriteStatue);
        }
        if (mLogSizeSet != null) {
            Message getLogSize = mSdSettingsActivityHandler
                    .obtainMessage(GET_LOG_SIZE);
            mSdSettingsActivityHandler.sendMessage(getLogSize);
        }
        if (mSdLogSizeSet != null) {
            Message getSdLogSize = mSdSettingsActivityHandler
                    .obtainMessage(GET_SD_SIZE);
            mSdSettingsActivityHandler.sendMessage(getSdLogSize);
        }
        if (mDataLogSizeSet != null) {
            Message getDataLogSize = mSdSettingsActivityHandler
                    .obtainMessage(GET_DATA_SIZE);
            mSdSettingsActivityHandler.sendMessage(getDataLogSize);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mSdSettingsActivityHandler != null) {
            mSdSettingsActivityHandler.getLooper().quit();
        }
        unbindService(mMCSConnection);
    }

    public void setSlogConfigChangedListener(SlogConfListener listener) {
        mSlogConfListener = listener;
    }

    private String SendSlogModemAt(String cmd) {
        Log.d(TAG, "SendSlogModemAt " + cmd);
        String strTmp = SocketUtils.sendCmdAndRecResult("slogmodem",
                LocalSocketAddress.Namespace.ABSTRACT, cmd);
        return strTmp;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        final String re = String.valueOf(newValue);
        if (preference == mLogOverwrite) {
            if (!mLogOverwrite.isChecked()) {
                Message openLogOverwrite = mSdSettingsActivityHandler
                        .obtainMessage(SET_LOG_OVERWRITE_OPEN);
                mSdSettingsActivityHandler.sendMessage(openLogOverwrite);
            } else {
                Message closeLogOverwrite = mSdSettingsActivityHandler
                        .obtainMessage(SET_LOG_OVERWRITE_CLOSE);
                mSdSettingsActivityHandler.sendMessage(closeLogOverwrite);
            }
            return true;
        } else if (preference == mLogSizeSet) {
            Message setLogSize = mSdSettingsActivityHandler
                    .obtainMessage(SET_LOG_SIZE, re);
            mSdSettingsActivityHandler.sendMessage(setLogSize);
            return true;
        } else if (preference == mSdLogSizeSet) {
            if (Integer.parseInt(re) == 1) {
                final EditText inputServer = new EditText(SdSettingsActivity.this);
                inputServer.setKeyListener(new NumberKeyListener() {

                    private char[] numberChars = {
                            '0', '1', '2', '3', '4',
                            '5', '6', '7', '8', '9'
                    };

                    @Override
                    public int getInputType() {
                        return android.text.InputType.TYPE_CLASS_PHONE;
                    }

                    @Override
                    protected char[] getAcceptedChars() {
                        return numberChars;
                    }
                });
                inputServer.setHint(getString(R.string.sd_log_size_unit));
                LengthFilter lengthFilter = new LengthFilter(5);
                InputFilter[] inputFilter = {
                    lengthFilter
                };
                inputServer.setFilters(inputFilter);
                AlertDialog.Builder builder = new AlertDialog.Builder(
                        SdSettingsActivity.this);
                builder.setTitle(getString(R.string.sd_set_all_log_size));
                builder.setView(inputServer);
                builder.setCancelable(false);
                builder.setNegativeButton(getString(R.string.alertdialog_cancel),
                        new DialogInterface.OnClickListener() {

                            public void onClick(DialogInterface dialog,
                                    int which) {
                                Message getSdLogSize = mSdSettingsActivityHandler
                                        .obtainMessage(GET_SD_SIZE);
                                mSdSettingsActivityHandler.sendMessage(getSdLogSize);
                            }
                        });
                builder.setPositiveButton(getString(R.string.alertdialog_ok),
                        new DialogInterface.OnClickListener() {

                            public void onClick(DialogInterface dialog,
                                    int which) {
                                /**BEGIN BUG542676 zhijie.yang 2016/04/07 After set the sigle log size, UE couldn't change the sigle log size to the original value*/
                                if (inputServer.getText().toString() != null
                                        && inputServer.getText().toString()
                                                .length() != 0 && (Integer.valueOf(inputServer
                                                        .getText().toString()) >= 1)) {
                                    int value = Integer.valueOf(inputServer
                                            .getText().toString());
                                    Message setSdLogSize = mSdSettingsActivityHandler
                                            .obtainMessage(SET_SD_SIZE, inputServer.getText()
                                                    .toString());
                                    mSdSettingsActivityHandler.sendMessage(setSdLogSize);
                                } else {
                                    Toast.makeText(
                                            SdSettingsActivity.this,
                                            "Input is wrong, please check!",
                                            Toast.LENGTH_SHORT).show();
                                    Message getSdLogSize = mSdSettingsActivityHandler
                                            .obtainMessage(GET_SD_SIZE);
                                    mSdSettingsActivityHandler.sendMessage(getSdLogSize);
                                    return;
                                }
                                /**END BUG542676 zhijie.yang 2016/04/07 After set the sigle log size, UE couldn't change the sigle log size to the original value*/
                            }
                        });
                builder.show();
            } else {
                Message setSdLogSize = mSdSettingsActivityHandler
                        .obtainMessage(SET_SD_SIZE, "0");
                mSdSettingsActivityHandler.sendMessage(setSdLogSize);

            }
            return true;
        } else if (preference == mDataLogSizeSet) {
            Message setDataLogSize = mSdSettingsActivityHandler
                    .obtainMessage(SET_DATA_SIZE, re);
            mSdSettingsActivityHandler.sendMessage(setDataLogSize);
            return true;
        }
        return false;
    }

    class SdSettingsActivityHandler extends Handler {
        public SdSettingsActivityHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String atResponse = null;
            String atCmd = null;
            switch (msg.what) {
                case GET_LOG_OVERWRITE_STATUE:
                    atCmd = "GET_LOG_OVERWRITE\n";
                    atResponse = SendSlogModemAt(atCmd);
                    if (atResponse != null && atResponse.contains(SocketUtils.OK)) {
                        String str1[] = atResponse.split("\\n");
                        String str2[] = str1[0].split("\\s+");
                        if (str2[1].contains("ENABLE")) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mLogOverwrite.setChecked(true);
                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mLogOverwrite.setChecked(false);
                                }
                            });
                        }
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mLogOverwrite.setChecked(false);
                            }
                        });
                    }
                    break;
                case SET_LOG_OVERWRITE_OPEN:
                    atResponse = SendSlogModemAt("ENABLE_LOG_OVERWRITE\n");
                    Log.d(TAG, "ENABLE_LOG_OVERWRITE: " + atResponse);
                    if (atResponse != null && atResponse.contains(SocketUtils.OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mLogOverwrite.setChecked(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mLogOverwrite.setChecked(false);
                            }
                        });
                    }
                    break;
                case SET_LOG_OVERWRITE_CLOSE:
                    atResponse = SendSlogModemAt("DISABLE_LOG_OVERWRITE\n");
                    Log.d(TAG, "DISABLE_LOG_OVERWRITE: " + atResponse);
                    if (atResponse != null && atResponse.contains(SocketUtils.OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mLogOverwrite.setChecked(false);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mLogOverwrite.setChecked(true);
                            }
                        });
                    }
                    break;
                case GET_LOG_SIZE:
                    atResponse = SendSlogModemAt("GET_LOG_FILE_SIZE\n");
                    Log.d(TAG, "GET_LOG_FILE_SIZE: " + atResponse);
                    if (atResponse != null && atResponse.contains(SocketUtils.OK)) {
                        String str1[] = atResponse.split("\\n");
                        String str2[] = str1[0].split("\\s+");
                        final String logSize = str2[1];
                        mSigleLogSize = Integer.parseInt(logSize.trim());
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mLogSizeSet.setSummary(logSize + "M");
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mLogSizeSet.setSummary("error");
                            }
                        });
                    }
                    break;
                case SET_LOG_SIZE:
                    final String setLogSize = (String) msg.obj;
                    Log.d(TAG, "Set Log size value: " + setLogSize);
                    /**BEGIN BUG542676 zhijie.yang 2016/04/07 After set the sigle log size, UE couldn't change the sigle log size to the original value*/
                    if ("".equals(setLogSize) || Integer.parseInt(setLogSize) < 1) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(
                                        SdSettingsActivity.this,
                                        "Input is wrong, please check!", Toast.LENGTH_SHORT).show();
                            }
                        });
                    } else {
                        atCmd = "SET_LOG_FILE_SIZE " + setLogSize + "\n";
                        atResponse = SendSlogModemAt(atCmd);
                        Log.d(TAG, atCmd + ": " + atResponse);
                        if (atResponse != null && atResponse.contains(SocketUtils.OK)) {
                            mSigleLogSize = Integer.parseInt(setLogSize.trim());
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mLogSizeSet.setSummary(setLogSize + "M");
                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    Toast.makeText(SdSettingsActivity.this, "Set Fail",
                                            Toast.LENGTH_SHORT).show();
                                }
                            });
                        }
                    }
                    /**END BUG542676 zhijie.yang 2016/04/07 After set the sigle log size, UE couldn't change the sigle log size to the original value*/
                    break;
                case GET_SD_SIZE:
                    atCmd = "GET_SD_MAX_SIZE\n";
                    atResponse = SendSlogModemAt(atCmd);
                    Log.d(TAG, atCmd + ": " + atResponse);
                    if (atResponse != null && atResponse.contains(SocketUtils.OK)) {
                        String str1[] = atResponse.split("\\n");
                        String str2[] = str1[0].split("\\s+");
                        final String sdMaxSize = str2[1];
                        if ("0".equals(sdMaxSize.trim())) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mSdLogSizeSet.setValueIndex(0);
                                    mSdLogSizeSet.setSummary(mSdLogSizeSet.getEntry());
                                }
                            });
                        } else {
                            mTotalSdLogSize = Integer.parseInt(sdMaxSize.trim());
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mSdLogSizeSet.setValueIndex(1);
                                    mSdLogSizeSet.setSummary(sdMaxSize + "M");

                                }
                            });
                        }
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSdLogSizeSet.setSummary("error");
                            }
                        });
                    }
                    break;
                case SET_SD_SIZE:
                    final String setSdMaxSize = (String) msg.obj;
                    atCmd = "SET_SD_MAX_SIZE " + setSdMaxSize + "\n";
                    atResponse = SendSlogModemAt(atCmd);
                    Log.d(TAG, atCmd + ": " + atResponse);
                    /**BEGIN BUG542676 zhijie.yang 2016/04/07 After set the sigle log size, UE couldn't change the sigle log size to the original value*/
                    if (atResponse != null && atResponse.contains(SocketUtils.OK)) {
                        mTotalSdLogSize = Integer.parseInt(setSdMaxSize.trim());
                        if (mTotalSdLogSize == 0) {
                            Message getSdLogSize = mSdSettingsActivityHandler
                                    .obtainMessage(GET_SD_SIZE);
                            mSdSettingsActivityHandler.sendMessage(getSdLogSize);
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mSdLogSizeSet.setSummary(setSdMaxSize + "M");
                                }
                            });
                        }
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(SdSettingsActivity.this, "Set Fail",
                                        Toast.LENGTH_SHORT).show();
                                Message getSdLogSize = mSdSettingsActivityHandler
                                        .obtainMessage(GET_SD_SIZE);
                                mSdSettingsActivityHandler.sendMessage(getSdLogSize);
                            }
                        });
                    }
                    /**END BUG542676 zhijie.yang 2016/04/07 After set the sigle log size, UE couldn't change the sigle log size to the original value*/
                    break;
                case GET_DATA_SIZE:
                    atCmd = "GET_DATA_MAX_SIZE\n";
                    atResponse = SendSlogModemAt(atCmd);
                    Log.d(TAG, atCmd + ": " + atResponse);
                    if (atResponse != null && atResponse.contains(SocketUtils.OK)) {
                        String str1[] = atResponse.split("\\n");
                        String str2[] = str1[0].split("\\s+");
                        final String dataMaxSize = str2[1];
                        mTotalDataLogSize = Integer.parseInt(dataMaxSize.trim());
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDataLogSizeSet.setSummary(dataMaxSize + "M");
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDataLogSizeSet.setSummary("error");
                            }
                        });
                    }
                    break;
                case SET_DATA_SIZE:
                    final String setDataMaxSize = (String) msg.obj;
                    /**BEGIN BUG542676 zhijie.yang 2016/04/07 After set the sigle log size, UE couldn't change the sigle log size to the original value*/
                    if ("".equals(setDataMaxSize)
                            || Integer.parseInt(setDataMaxSize) < 1) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(
                                        SdSettingsActivity.this,
                                        "Input is wrong, please check!", Toast.LENGTH_SHORT).show();
                            }
                        });
                    } else {
                        atCmd = "SET_DATA_MAX_SIZE " + setDataMaxSize + "\n";
                        atResponse = SendSlogModemAt(atCmd);
                        Log.d(TAG, atCmd + ": " + atResponse);
                        if (atResponse != null && atResponse.contains(SocketUtils.OK)) {
                            mTotalDataLogSize = Integer.parseInt(setDataMaxSize.trim());
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mDataLogSizeSet.setSummary(setDataMaxSize + "M");
                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    Toast.makeText(SdSettingsActivity.this, "Set Fail",
                                            Toast.LENGTH_SHORT).show();
                                }
                            });
                        }
                    }
                    /**END BUG542676 zhijie.yang 2016/04/07 After set the sigle log size, UE couldn't change the sigle log size to the original value*/
                    break;
                default:
                    break;
            }
        }
    }
}
