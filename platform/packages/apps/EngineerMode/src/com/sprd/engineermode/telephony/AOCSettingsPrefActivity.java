
package com.sprd.engineermode.telephony;

import android.os.Bundle;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.text.InputFilter;
import android.widget.EditText;
import android.widget.TextView;
import android.text.InputFilter.LengthFilter;
import android.text.method.NumberKeyListener;
import android.text.InputType;
import android.util.Log;
import android.widget.Toast;

import com.sprd.engineermode.telephony.SimSelectHelperActivity;
import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class AOCSettingsPrefActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener {
    private static final String TAG = "AOCSettingsPrefActivity";
    private static final String KEY_AOCACT = "aoc_active";
    private static final String KEY_AOCSET = "aoc_setting";
    private static final String MAX_NUM = "AOC MAX";

    private static final int STATUS_ON = 1;
    private static final int STATUS_OFF = 0;

    private static final int GETAOCACTIVE = 1;
    private static final int SETAOCACTIVE = 2;
    private static final int AOCSET = 3;

    private static final String PIN2 = "2345";
    private EMSwitchPreference mPreference01;
    private EditTextPreference mPreference02;
    private Handler mUiThread = new Handler();
    private String mATResponse;
    private String mAnalysisResponse;
    private static String mMaxNum;
    private AOCSettingHandler aocSettingHandler;
    private int state;
    private int mSetNum;
    private int mSimIndex;

    private NumberKeyListener numberKeyListener = new NumberKeyListener() {

        private char[] numberChars = {
                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd',
                'e', 'f'
        };

        @Override
        public int getInputType() {
            return android.text.InputType.TYPE_CLASS_TEXT;
        }

        @Override
        protected char[] getAcceptedChars() {
            return numberChars;
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_aocsettings);

        mSimIndex = getIntent().getIntExtra(SimSelectHelperActivity.INTENT_SIM_INDEX,0);

        mPreference01 = (EMSwitchPreference) findPreference(KEY_AOCACT);
        mPreference01.setOnPreferenceChangeListener(this);
        mPreference02 = (EditTextPreference) findPreference(KEY_AOCSET);
        mPreference02.setOnPreferenceChangeListener(this);
        mPreference02.getEditText().setKeyListener(numberKeyListener);
        if (savedInstanceState != null) {
            mMaxNum = savedInstanceState.getString(MAX_NUM, null);
        }
        mPreference02.setSummary(mMaxNum);
        LengthFilter lengthFilter = new LengthFilter(6);

        InputFilter[] inputFilter = {
                lengthFilter
        };
        mPreference02.getEditText().setFilters(inputFilter);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        aocSettingHandler = new AOCSettingHandler(ht.getLooper());

        Message aocActiveMessage = aocSettingHandler.obtainMessage(GETAOCACTIVE);
        aocSettingHandler.sendMessage(aocActiveMessage);
    }

    public boolean onPreferenceChange(Preference preference, Object newValue) {
        final String key = preference.getKey();
        if (KEY_AOCSET.equals(key)) {
            final String numToSet = (String) newValue;
            if (numToSet.length() != 6) {
                Toast.makeText(AOCSettingsPrefActivity.this,
                        "number between 000000 and ffffff",
                        Toast.LENGTH_SHORT).show();
                return false;
            } else {
                final EditText inputServer = new EditText(this);
                inputServer.setKeyListener(new NumberKeyListener() {

                    private char[] numberChars = {
                            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
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

                LengthFilter lengthFilter = new LengthFilter(8);
                InputFilter[] inputFilter = {
                        lengthFilter
                };
                inputServer.setFilters(inputFilter);
                inputServer.setInputType(InputType.TYPE_NUMBER_VARIATION_PASSWORD);

                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle("Input PIN2");
                builder.setView(inputServer);
                builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {

                    public void onClick(DialogInterface dialog, int which) {
                    }
                });
                builder.setPositiveButton("Ok", new DialogInterface.OnClickListener() {

                    public void onClick(DialogInterface dialog, int which) {
                        if (inputServer.getText().toString() != null
                                && inputServer.getText().toString().length() >= 4) {
                            Message m = aocSettingHandler.obtainMessage(AOCSET, 
                                    Integer.parseInt(numToSet, 16), 0, inputServer.getText().toString());
                            aocSettingHandler.sendMessage(m);
                        }
                        else {
                            Toast.makeText(AOCSettingsPrefActivity.this, "Please input 4-8 pos",
                                    Toast.LENGTH_SHORT).show();
                            return;
                        }
                    }
                });
                builder.show();
                return false;
            }
        } else if (KEY_AOCACT.equals(key)) {
            int statusWant = STATUS_ON;
            if (mPreference01.isChecked()) {
                statusWant = STATUS_OFF;
            }
            Message m = aocSettingHandler.obtainMessage(SETAOCACTIVE, statusWant, 0, mPreference01);
            aocSettingHandler.sendMessage(m);

            return true;
        } else
            return false;
    }

    private String analysisResponse(String response, int type) {
        if (response.contains(IATUtils.AT_OK)) {
            if (type == GETAOCACTIVE) {
                String[] str = response.split("\n");
                String[] str1 = str[0].split(":");
                Log.d(TAG, type + "  " + str1[1]);
                return str1[1].trim();
            } else if (type == SETAOCACTIVE || type == AOCSET) {
                return IATUtils.AT_OK;
            }
        }
        return IATUtils.AT_FAIL;

    }

    private class AOCSettingHandler extends Handler {
        public AOCSettingHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {

            switch (msg.what) {
                case GETAOCACTIVE: {
                    mATResponse = IATUtils.sendATCmd(engconstents.ENG_AT_CAOCQ1, "atchannel"+mSimIndex);
                    mAnalysisResponse = analysisResponse(mATResponse, GETAOCACTIVE);
                    if (!IATUtils.AT_FAIL.equals(mAnalysisResponse)) {
                        if (mAnalysisResponse.equals("0")) {
                            mUiThread.post(new Runnable() {

                                @Override
                                public void run() {
                                    mPreference01.setSummary("the state is query");
                                }

                            });
                        } else if (mAnalysisResponse.equals("2")) {
                            mUiThread.post(new Runnable() {

                                @Override
                                public void run() {
                                    mPreference01.setChecked(true);
                                }

                            });
                        } else if (mAnalysisResponse.equals("1")) {
                            mUiThread.post(new Runnable() {

                                @Override
                                public void run() {
                                    mPreference01.setChecked(false);
                                }

                            });
                        }
                    }
                }
                    break;
                case SETAOCACTIVE: {
                    if (msg.arg1 == STATUS_ON) {
                        state = 1;
                    } else {
                        state = 0;
                    }
                    mATResponse = IATUtils.sendATCmd(engconstents.ENG_AT_CAOCSET1 + state,
                            "atchannel"+mSimIndex);
                    mAnalysisResponse = analysisResponse(mATResponse, SETAOCACTIVE);
                    if (IATUtils.AT_OK.equals(mAnalysisResponse)) {
                        mUiThread.post(new AOCATSuccessRunnable());
                    } else {
                        mUiThread.post(new AOCATFailRunnable(msg));
                    }
                }
                    break;
                case AOCSET: {
                    mSetNum = msg.arg1;
                    final String mSetNumStr = Integer.toHexString(mSetNum);;
                    String pin2 = (String) msg.obj;
                    mATResponse = IATUtils.sendATCmd(engconstents.ENG_AT_CAMM1 + "\"" + mSetNumStr
                            + "\"" + ","
                            + "\"" + pin2 + "\"", "atchannel"+mSimIndex);
                    Log.d(TAG, engconstents.ENG_AT_CAMM1 + "\"" + mSetNumStr + "\"" + ","
                            + "\"" + pin2 + "\"");
                    mAnalysisResponse = analysisResponse(mATResponse, AOCSET);
                    Log.d(TAG, mAnalysisResponse);
                    if (IATUtils.AT_OK.equals(mAnalysisResponse)) {
                        mUiThread.post(new AOCATSuccessRunnable());
                        mUiThread.post(new Runnable() {

                            @Override
                            public void run() {
                                mPreference02.setSummary(mSetNumStr);
                                mMaxNum = mSetNumStr;
                            }

                        });
                    } else {
                        mUiThread.post(new AOCATFailRunnable(msg));
                    }
                }
            }
        }
    }

    class AOCATFailRunnable implements Runnable {
        private int arg1 = 0;
        private Object obj = null;

        public AOCATFailRunnable(Message msg) {
            arg1 = msg.arg1;
            obj = msg.obj;
        }

        @Override
        public void run() {
            Toast.makeText(AOCSettingsPrefActivity.this, "Fail", Toast.LENGTH_SHORT).show();
            if (obj == mPreference01) {
                mPreference01.setChecked(arg1 == STATUS_OFF ? true : false);
                mPreference01.setEnabled(true);
            }
        }

    }

    class AOCATSuccessRunnable implements Runnable {

        @Override
        public void run() {
            Toast.makeText(AOCSettingsPrefActivity.this, "Success", Toast.LENGTH_SHORT).show();
        }

    }

    @Override
    protected void onDestroy() {
        if (aocSettingHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            aocSettingHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        finish();
        super.onBackPressed();
    }
    
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(
                outState);
        outState.putString(MAX_NUM, mMaxNum);
    }
}
