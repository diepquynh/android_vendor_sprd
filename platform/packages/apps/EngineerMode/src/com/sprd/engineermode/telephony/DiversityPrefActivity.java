// SPRD: add Diversity switch by alisa.li 20160517

package com.sprd.engineermode.telephony;

import java.util.Arrays;

import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.ListPreference;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.HandlerThread;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import android.content.Context;
import android.util.Log;
import android.widget.Toast;

public class DiversityPrefActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener {

    private static final String TAG = "DiversityPrefActivity";
    private static final String WDIVERSITY = "w_Div";
    private static final String LTESCCTX = "lte_SCC_TX";
    private static final String LTEDIVERSITYRX = "lte_Div_RX";
    private static final String LTEPRIMARYTX = "lte_Pri_TX";
    private static final String LTEPRIMARYRX = "lte_Pri_RX";

    private static final int GET_STATUS = 0;
    private static final int SET_WDiv_STATUS = 1;
    private static final int SET_LTESCCTX_STATUS = 2;
    private static final int SET_LTEDivRX_STATUS = 3;
    private static final int SET_LTEPrimTX_STATUS = 4;
    private static final int SET_LTEPrimRX_STATUS = 5;

    private ListPreference mWDiversity;
    private ListPreference mLTESCCTX;
    private ListPreference mLTEDiversityRX;
    private ListPreference mLTEPrimTX;
    private ListPreference mLTEPrimRX;

    private String mATCmd;
    private String mATResponse;
    private int PARA1;
    private int PARA2;
    private int type;

    private DivHandler mDivHandler;
    private Context mContext;
    private Handler mUiThread = new Handler();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mDivHandler = new DivHandler(ht.getLooper());

        addPreferencesFromResource(R.xml.pref_diversity);
        mWDiversity = (ListPreference) findPreference(WDIVERSITY);
        mWDiversity.setOnPreferenceChangeListener(this);
        mLTESCCTX = (ListPreference) findPreference(LTESCCTX);
        mLTESCCTX.setOnPreferenceChangeListener(this);
        mLTEDiversityRX = (ListPreference) findPreference(LTEDIVERSITYRX);
        mLTEDiversityRX.setOnPreferenceChangeListener(this);
        mLTEPrimTX = (ListPreference) findPreference(LTEPRIMARYTX);
        mLTEPrimTX.setOnPreferenceChangeListener(this);
        mLTEPrimRX = (ListPreference) findPreference(LTEPRIMARYRX);
        mLTEPrimRX.setOnPreferenceChangeListener(this);

    }

    @Override
    protected void onStart() {
        super.onStart();
        Message getStatus = mDivHandler
                .obtainMessage(GET_STATUS);
        mDivHandler.sendMessage(getStatus);
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object newValue) {
        if (pref == mWDiversity) {
            Message setmDiversitystatus = mDivHandler.obtainMessage(SET_WDiv_STATUS,
                    Integer.parseInt(String.valueOf(newValue)), 0);
            mDivHandler.sendMessage(setmDiversitystatus);
        } else if (pref == mLTESCCTX) {
            Message setmSCCTXstatus = mDivHandler.obtainMessage(SET_LTESCCTX_STATUS,
                    Integer.parseInt(String.valueOf(newValue)), 0);
            mDivHandler.sendMessage(setmSCCTXstatus);
        } else if (pref == mLTEDiversityRX) {
            Message setmDiversityRXstatus = mDivHandler.obtainMessage(SET_LTEDivRX_STATUS,
                    Integer.parseInt(String.valueOf(newValue)), 0);
            mDivHandler.sendMessage(setmDiversityRXstatus);
        } else if (pref == mLTEPrimTX) {
            Message setmPrimaryTXstatus = mDivHandler.obtainMessage(SET_LTEPrimTX_STATUS,
                    Integer.parseInt(String.valueOf(newValue)), 0);
            mDivHandler.sendMessage(setmPrimaryTXstatus);
        } else if (pref == mLTEPrimRX) {
            Log.d(TAG, "start KEY_mLTEPrimRX" + newValue);
            Message setmPrimaryRXstatus = mDivHandler.obtainMessage(SET_LTEPrimRX_STATUS,
                    Integer.parseInt(String.valueOf(newValue)), 0);
            mDivHandler.sendMessage(setmPrimaryRXstatus);
        }
        return true;
    }

    @Override
    protected void onDestroy() {
        if (mDivHandler != null) {
            mDivHandler.getLooper().quit();
            Log.d(TAG, "HandlerThread has quit");
        }
        super.onDestroy();
    }

    class DivHandler extends Handler {
        public DivHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case GET_STATUS:
                    mATCmd = "AT+SPDUALRFSEL?";
                    mATResponse = IATUtils.sendATCmd(mATCmd, "atchannel0");
                    Log.d(TAG, "mATResponse :" + mATResponse);
                    //modify by alisa.li for bug574839 begin
                    if(mATResponse != null && mATResponse.contains(IATUtils.AT_OK)){
                        String[] tempArray = mATResponse.split(" ");
                        String[] paraArray = tempArray[1].split(",");
                        String[] paraArray2 = paraArray[1].trim().split("\n");
                        PARA1 = Integer.valueOf(paraArray[0].trim()).intValue();
                        PARA2 = Integer.valueOf(paraArray2[0].trim()).intValue();
                        Log.d(TAG, "GET_STATUS PARA1=" + PARA1);
                        Log.d(TAG, "GET_STATUS PARA2=" + PARA2);
                        initUI();
                    }else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(mContext, "AT cmd send fail",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    }
                  //modify by alisa.li for bug574839 end
                    break;
                case SET_WDiv_STATUS:
                    type = (int) msg.arg1;
                    Log.d(TAG, "SET_WDiv_STATUS type=" + type);
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Log.d(TAG, "Run Set wDiv Status");
                            mWDiversity.setSummary(mWDiversity.getEntry());
                        }
                    });
                    if (type == 0)
                    {
                        PARA2 = 0;
                        mATCmd = "AT+SPDUALRFSEL=" + PARA1 + "," + PARA2;
                    } else if (type == 1) {
                        PARA2 = 1;
                        mATCmd = "AT+SPDUALRFSEL=" + PARA1 + "," + PARA2;
                    } else {
                        PARA2 = 17;
                        mATCmd = "AT+SPDUALRFSEL=" + PARA1 + "," + PARA2;
                    }
                    Log.d(TAG, "WDiv mATCmd = " + mATCmd);
                    mATResponse = IATUtils.sendATCmd(mATCmd, "atchannel0");
                    Log.d(TAG, "WDIVERSITY mATResponse :" + mATResponse);
                    if (mATResponse != null && mATResponse.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(mContext, "AT cmd send fail",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    }
                    break;
                case SET_LTESCCTX_STATUS:
                    type = (int) msg.arg1;
                    Log.d(TAG, "SET_LTESCCTX_STATUS type=" + type);
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Log.d(TAG, "Run Set mLTESCCTX Status");
                            mLTESCCTX.setSummary(mLTESCCTX.getEntry());
                        }
                    });
                    if (type == 0) {
                        PARA1 = PARA1 | 1 << 3;
                        mATCmd = "AT+SPDUALRFSEL=" + PARA1 + "," + PARA2;
                        Log.d(TAG, "mLTESCCTX mATCmd=" + mATCmd);
                        mATResponse = IATUtils.sendATCmd(mATCmd, "atchannel0");
                        Log.d(TAG, "open LTESCCTX mATResponse :" + mATResponse);
                        if (mATResponse != null && mATResponse.contains(IATUtils.AT_OK)) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    Toast.makeText(mContext, "AT cmd send fail",
                                            Toast.LENGTH_SHORT).show();
                                }
                            });
                        }
                    } else if (type == 1) {
                        PARA1 = PARA1 & (~(1 << 3));
                        mATCmd = "AT+SPDUALRFSEL=" + PARA1 + "," + PARA2;
                        Log.d(TAG, "close mLTESCCTX mATCmd=" + mATCmd);
                        mATResponse = IATUtils.sendATCmd(mATCmd, "atchannel0");
                        Log.d(TAG, "close LTESCCTX mATResponse :" + mATResponse);
                        if (mATResponse != null && mATResponse.contains(IATUtils.AT_OK)) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {

                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    Toast.makeText(mContext, "AT cmd send fail",
                                            Toast.LENGTH_SHORT).show();
                                }
                            });
                        }
                    }
                    break;
                case SET_LTEDivRX_STATUS:
                    type = (int) msg.arg1;
                    Log.d(TAG, "SET_LTEDivRX_STATUS type=" + type);
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Log.d(TAG, "Run Set mLTEDiversityRX Status");
                            mLTEDiversityRX.setSummary(mLTEDiversityRX.getEntry());
                        }
                    });
                    if (type == 0) {
                        PARA1 = PARA1 | 1 << 4;
                        mATCmd = "AT+SPDUALRFSEL=" + PARA1 + "," + PARA2;
                        Log.d(TAG, "open mLTEDiversityRX mATCmd=" + mATCmd);
                        mATResponse = IATUtils.sendATCmd(mATCmd, "atchannel0");
                        Log.d(TAG, "open LTEDivRX mATResponse :" + mATResponse);
                        if (mATResponse != null && mATResponse.contains(IATUtils.AT_OK)) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    Toast.makeText(mContext, "AT cmd send fail",
                                            Toast.LENGTH_SHORT).show();
                                }
                            });
                        }
                    } else if (type == 1) {
                        PARA1 = PARA1 & (~(1 << 4));
                        mATCmd = "AT+SPDUALRFSEL=" + PARA1 + "," + PARA2;
                        Log.d(TAG, "close mLTEDiversityRX mATCmd=" + mATCmd);
                        mATResponse = IATUtils.sendATCmd(mATCmd, "atchannel0");
                        Log.d(TAG, "close LTEDivRX mATResponse :" + mATResponse);
                        if (mATResponse != null && mATResponse.contains(IATUtils.AT_OK)) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {

                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    Toast.makeText(mContext, "AT cmd send fail",
                                            Toast.LENGTH_SHORT).show();
                                }
                            });
                        }
                    }
                    break;
                case SET_LTEPrimTX_STATUS:
                    type = (int) msg.arg1;
                    Log.d(TAG, "SET_LTEPrimTX_STATUS type=" + type);
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Log.d(TAG, "Run Set mLTEPrimTX Status");
                            mLTEPrimTX.setSummary(mLTEPrimTX.getEntry());
                        }
                    });
                    if (type == 0) {
                        PARA1 = PARA1 | 1 << 5;
                        mATCmd = "AT+SPDUALRFSEL=" + PARA1 + "," + PARA2;
                        Log.d(TAG, "open mLTEPrimTX mATCmd=" + mATCmd);
                        mATResponse = IATUtils.sendATCmd(mATCmd, "atchannel0");
                        Log.d(TAG, "open LTEPrimTX mATResponse :" + mATResponse);
                        if (mATResponse != null && mATResponse.contains(IATUtils.AT_OK)) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    Toast.makeText(mContext, "AT cmd send fail",
                                            Toast.LENGTH_SHORT).show();
                                }
                            });
                        }
                    } else if (type == 1) {
                        PARA1 = PARA1 & (~(1 << 5));
                        mATCmd = "AT+SPDUALRFSEL=" + PARA1 + "," + PARA2;
                        Log.d(TAG, "close mLTEPrimTX mATCmd=" + mATCmd);
                        mATResponse = IATUtils.sendATCmd(mATCmd, "atchannel0");
                        Log.d(TAG, "close LTEPrimTX mATResponse :" + mATResponse);
                        if (mATResponse != null && mATResponse.contains(IATUtils.AT_OK)) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {

                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    Toast.makeText(mContext, "AT cmd send fail",
                                            Toast.LENGTH_SHORT).show();
                                }
                            });
                        }
                    }
                    break;
                case SET_LTEPrimRX_STATUS:
                    type = (int) msg.arg1;
                    Log.d(TAG, "SET_LTEPrimRX_STATUS type=" + type);
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Log.d(TAG, "Run Set mLTEPrimRX Status");
                            mLTEPrimRX.setSummary(mLTEPrimRX.getEntry());
                        }
                    });
                    if (type == 0) {
                        PARA1 = PARA1 | 1 << 6;
                        mATCmd = "AT+SPDUALRFSEL=" + PARA1 + "," + PARA2;
                        Log.d(TAG, "open mLTEPrimRX mATCmd=" + mATCmd);
                        mATResponse = IATUtils.sendATCmd(mATCmd, "atchannel0");
                        Log.d(TAG, "Reset and open LTEPrimRX mATResponse :" + mATResponse);
                        if (mATResponse != null && mATResponse.contains(IATUtils.AT_OK)) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    Toast.makeText(mContext, "AT cmd send fail",
                                            Toast.LENGTH_SHORT).show();
                                }
                            });
                        }
                    } else if (type == 1) {
                        PARA1 = PARA1 & (~(1 << 6));
                        mATCmd = "AT+SPDUALRFSEL=" + PARA1 + "," + PARA2;
                        Log.d(TAG, "close mLTEPrimRX mATCmd=" + mATCmd);
                        mATResponse = IATUtils.sendATCmd(mATCmd, "atchannel0");
                        Log.d(TAG, "close LTEPrimRX mATResponse :" + mATResponse);
                        if (mATResponse != null && mATResponse.contains(IATUtils.AT_OK)) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {

                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    Toast.makeText(mContext, "AT cmd send fail",
                                            Toast.LENGTH_SHORT).show();
                                }
                            });
                        }
                    }
                    break;
            }
        }
    }

    private void initUI() {
        PARA1 = PARA1 & ~(1 << 2);
        Log.d(TAG, "initUI() PARA1 = " + PARA1);
        if (mWDiversity != null) {
            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    if (PARA2 == 0) {
                        mWDiversity.setValueIndex(0);
                        mWDiversity.setSummary(mWDiversity.getEntry());
                        Log.d(TAG, "mWDiversity= " + mWDiversity.getEntry());
                    } else if (PARA2 == 1) {
                        mWDiversity.setValueIndex(1);
                        mWDiversity.setSummary(mWDiversity.getEntry());
                    } else if (PARA2 == 17) {
                        mWDiversity.setValueIndex(2);
                        mWDiversity.setSummary(mWDiversity.getEntry());
                    } else {
                        Toast.makeText(mContext, "WCDMA Diversity value error, Please check",
                                Toast.LENGTH_SHORT).show();
                    }
                }
            });
        }
        if (mLTESCCTX != null) {
            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    if ((PARA1 & 1 << 3) == 0) {
                        mLTESCCTX.setValueIndex(1);
                        mLTESCCTX.setSummary(mLTESCCTX.getEntry());
                    } else {
                        mLTESCCTX.setValueIndex(0);
                        mLTESCCTX.setSummary(mLTESCCTX.getEntry());
                    }
                }
            });
        }
        if (mLTEDiversityRX != null) {
            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    if ((PARA1 & 1 << 4) == 0) {
                        mLTEDiversityRX.setValueIndex(1);
                        mLTEDiversityRX.setSummary(mLTEDiversityRX.getEntry());
                    } else {
                        mLTEDiversityRX.setValueIndex(0);
                        mLTEDiversityRX.setSummary(mLTEDiversityRX.getEntry());
                    }
                }
            });
        }
        if (mLTEPrimTX != null) {
            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    if ((PARA1 & 1 << 5) == 0) {
                        mLTEPrimTX.setValueIndex(1);
                        mLTEPrimTX.setSummary(mLTEPrimTX.getEntry());
                    } else {
                        mLTEPrimTX.setValueIndex(0);
                        mLTEPrimTX.setSummary(mLTEPrimTX.getEntry());
                    }
                }
            });
        }
        if (mLTEPrimRX != null) {
            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    if ((PARA1 & 1 << 6) == 0) {
                        mLTEPrimRX.setValueIndex(1);
                        mLTEPrimRX.setSummary(mLTEPrimRX.getEntry());
                    } else {
                        mLTEPrimRX.setValueIndex(0);
                        mLTEPrimRX.setSummary(mLTEPrimRX.getEntry());
                    }
                }
            });
        }
    }
}
