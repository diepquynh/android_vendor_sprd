package com.sprd.engineermode.debuglog;

import android.app.ProgressDialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.telephony.TelephonyManager;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;
import android.util.Log;
import android.widget.Toast;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceGroup;
import android.preference.PreferenceManager;
import android.preference.Preference.OnPreferenceClickListener;
import com.sprd.engineermode.R;
import com.sprd.engineermode.R.xml;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class LockFrequencyActivity extends PreferenceActivity implements
Preference.OnPreferenceClickListener{

    private static final String TAG = "LockFrequencyActivity";
    private static final String KEY_TW_QUERY = "tw_frequency_query";
    private static final String KEY_G_QUERY = "g_frequency_query";
    private static final String KEY_LTE_QUERY = "lte_frequency_query";
    private static final String KEY_TD_BAND_QUERY = "t_band_query";
    private static final String KEY_W_BAND_QUERY ="w_band_query";
    private static final String KEY_G_BAND_QUERY = "g_band_query";
    private static final String KEY_LTE_BAND_QUERY = "lte_band_query";

    private static final String KEY_TW_FREQUENCY = "tw_frequency";
    private static final String KEY_G_FREQUENCY = "g_frequency";
    private static final String KEY_TW_QUERY_SIM = "tw_frequency_sim";
    private static final String KEY_G_QUERY_SIM = "g_frequency_sim";
    private static final String TW_FREQUENCY_RESULT = "com.sprd.engineermode.action.TWFREQUENCY";
    private static final String G_FREQUENCY_RESULT = "com.sprd.engineermode.action.GFREQUENCY";
    private static final String LOCKED_BAND = "locked_band";
    private static final String MODEM_TYPE = "modemtype";
    private static final String ISDUALMODE = "dual_mode";
    private static final String NET_MODE = "net_mode";
    private static final String G_LOCKED_BAND = "g_locked_band";

    private static final int TW_QUERY = 1;
    private static final int G_QUERY = 2;
    private static final int TD_BAND_QUERY = 3;
    private static final int W_BAND_QUERY = 4;
    private static final int G_BAND_QUERY = 5;
    private static final int LTE_BAND_QUERY = 6;

    private Handler mUiThread = new Handler();
    private LockFHandler mLockFHandler;
    PreferenceGroup mPreGroup = null;
    private int mPhoneCount;
    private int mModemType;
    private PreferenceCategory mPreferenceCategory;
    private Preference[] mFrePreference;
//    private Preference[] mBandPreference;
    private int mSIM = 0;
    private String mResult = null;
    private String mServerName = "atchannel0";
    private String mLockBand;
    private String mIntentAction;
    private ProgressDialog mProgressDialog;  
    private boolean mIsDualMode = false;
    private String mNetMode;

    @Override
    protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        mPhoneCount = TelephonyManager.from(this).getPhoneCount();
        mFrePreference = new Preference[mPhoneCount];
//        mBandPreference = new Preference[mPhoneCount];
        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
        mPreGroup = getPreferenceScreen();
        for (int i=0;i<mPhoneCount;i++) {
            mModemType = TelephonyManagerSprd.getModemType();
            if (mModemType == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA) {
                mPreferenceCategory = new PreferenceCategory(this);
                mPreferenceCategory.setTitle("SIM"+i);
                mPreGroup.addPreference(mPreferenceCategory);
                mFrePreference[i] = new Preference(this);
                mFrePreference[i].setKey(KEY_TW_QUERY+i);
                mFrePreference[i].setTitle(R.string.frequency_set);
                mFrePreference[i].setSummary(R.string.click_query_set_frequency);
                mFrePreference[i].setOnPreferenceClickListener(this);               
                mPreGroup.addPreference(mFrePreference[i]);
                mFrePreference[i].setEnabled(false);
//                mBandPreference[i] = new Preference(this);
//                mBandPreference[i].setTitle(R.string.frequency_band_set);
//                mBandPreference[i].setSummary(R.string.click_set_band);
//                mBandPreference[i].setKey(KEY_TD_BAND_QUERY+i);
//                mBandPreference[i].setOnPreferenceClickListener(this);
//                mPreGroup.addPreference(mBandPreference[i]);
            } else if (mModemType == TelephonyManagerSprd.MODEM_TYPE_WCDMA) {
                mPreferenceCategory = new PreferenceCategory(this);
                mPreferenceCategory.setTitle("SIM"+i);
                mPreGroup.addPreference(mPreferenceCategory);
                mFrePreference[i] = new Preference(this);
                mFrePreference[i].setKey(KEY_TW_QUERY+i);
                mFrePreference[i].setTitle(R.string.frequency_set);
                mFrePreference[i].setSummary(R.string.click_query_set_frequency);
                mFrePreference[i].setOnPreferenceClickListener(this);
                mPreGroup.addPreference(mFrePreference[i]);
                mFrePreference[i].setEnabled(false);
//                mBandPreference[i] = new Preference(this);
//                mBandPreference[i].setTitle(R.string.frequency_band_set);
//                mBandPreference[i].setSummary(R.string.click_set_band);
//                mBandPreference[i].setKey(KEY_W_BAND_QUERY+i);
//                mBandPreference[i].setOnPreferenceClickListener(this);
//                mPreGroup.addPreference(mBandPreference[i]);
            } else if (mModemType == TelephonyManagerSprd.MODEM_TYPE_GSM) {
                mPreferenceCategory = new PreferenceCategory(this);
                mPreferenceCategory.setTitle("SIM"+i);
                mPreGroup.addPreference(mPreferenceCategory);
                mFrePreference[i] = new Preference(this);
                mFrePreference[i].setKey(KEY_G_QUERY+i);
                mFrePreference[i].setTitle(R.string.frequency_set);
                mFrePreference[i].setSummary(R.string.click_query_set_frequency);
                mFrePreference[i].setOnPreferenceClickListener(this);
                mPreGroup.addPreference(mFrePreference[i]);
                mFrePreference[i].setEnabled(false);
//                mBandPreference[i] = new Preference(this);
//                mBandPreference[i].setTitle(R.string.frequency_band_set);
//                mBandPreference[i].setSummary(R.string.click_set_band);
//                mBandPreference[i].setKey(KEY_G_BAND_QUERY+i);
//                mBandPreference[i].setOnPreferenceClickListener(this);
//                mPreGroup.addPreference(mBandPreference[i]);
            } else if (mModemType == TelephonyManagerSprd.MODEM_TYPE_LTE) {
                mPreferenceCategory = new PreferenceCategory(this);
                mPreferenceCategory.setTitle("SIM"+i);
                mPreGroup.addPreference(mPreferenceCategory);
                mFrePreference[i] = new Preference(this);
                mFrePreference[i].setKey(KEY_G_QUERY+i);
                mFrePreference[i].setTitle(R.string.frequency_set);
                mFrePreference[i].setSummary(R.string.click_query_set_frequency);
                mFrePreference[i].setOnPreferenceClickListener(this);
                mPreGroup.addPreference(mFrePreference[i]);
                mFrePreference[i].setEnabled(false);
//                mBandPreference[i] = new Preference(this);
//                mBandPreference[i].setTitle(R.string.frequency_band_set);
//                mBandPreference[i].setSummary(R.string.click_set_band);
//                mBandPreference[i].setKey(KEY_LTE_BAND_QUERY+i);
//                mBandPreference[i].setOnPreferenceClickListener(this);
//                mPreGroup.addPreference(mBandPreference[i]);
            }
        }

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mLockFHandler = new LockFHandler(ht.getLooper()); 

    }

    @Override
    protected void onDestroy() {
        if(mLockFHandler != null){
            Log.d(TAG,"HandlerThread has quit");
            mLockFHandler.getLooper().quit();
        } 
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        // TODO Auto-generated method stub
        finish();
        super.onBackPressed();
    }

    @Override
    public boolean  onPreferenceClick(Preference pref){
        String key = pref.getKey();
        Log.d(TAG,"key is "+key);
        for (int i=0;i<mPhoneCount;i++) {
            if (key.equals(KEY_TW_QUERY+i)) {        
                Message twQuery = mLockFHandler.obtainMessage(TW_QUERY,i);
                mLockFHandler.sendMessage(twQuery);
            } else if(key.equals(KEY_G_QUERY+i)) {
                Message gQuery = mLockFHandler.obtainMessage(G_QUERY,i);
                mLockFHandler.sendMessage(gQuery);
            } else if(key.equals(KEY_TD_BAND_QUERY+i)) {
                Message tBand = mLockFHandler.obtainMessage(TD_BAND_QUERY,i);
                mLockFHandler.sendMessage(tBand);
            } else if(key.equals(KEY_W_BAND_QUERY+i)) {
                Message wBand = mLockFHandler.obtainMessage(W_BAND_QUERY,i);
                mLockFHandler.sendMessage(wBand);
            } else if(key.equals(KEY_G_BAND_QUERY+i)) {
                Message gBand = mLockFHandler.obtainMessage(G_BAND_QUERY,i);
                mLockFHandler.sendMessage(gBand);
            } else if (key.equals(KEY_LTE_BAND_QUERY+i)) {
                Message lteBand = mLockFHandler.obtainMessage(LTE_BAND_QUERY,i);
                mLockFHandler.sendMessage(lteBand);
            }
        }
        return true;
    }

    private boolean isLocked(String resp){
        if(resp != null){
            String[] str = resp.split("\\:");
            String[] str1 = str[1].split("\\,");
            String[] str2 = str1[2].split("\n");
            String result = str2[0].trim();
            if(result.contains("1")){
                return true;
            }else{
                return false;
            }
        }else{
            return false;
        }
    }

    private String queryWCDMABandLocked(String serverName){
        String result = "";
        String atCmd = null;
        String resp = "";
        for(int i=0;i<4;i++){
            if(i == 0){
                atCmd = engconstents.ENG_AT_W_LOCKED_BAND+"1";
                resp = IATUtils.sendATCmd(atCmd,serverName);
                if(resp.contains(IATUtils.AT_OK)){
                    if(isLocked(resp)){
                        resp="1";
                        result = resp;
                    }else{
                        resp = "";
                    }
                    continue;
                }else{
                    resp ="";
                    continue;
                }
            }else if(i == 1){
                atCmd = engconstents.ENG_AT_W_LOCKED_BAND+"2";
                resp = IATUtils.sendATCmd(atCmd,serverName);
                if(resp.contains(IATUtils.AT_OK)){
                    if(isLocked(resp)){
                        resp="2";
                        result = result+","+resp;
                    }else{
                        resp = "";
                    }
                    continue;
                }else{
                    resp ="";
                    continue;
                }
            }else if(i == 2){
                atCmd = engconstents.ENG_AT_W_LOCKED_BAND+"5";
                resp = IATUtils.sendATCmd(atCmd,serverName);
                if(resp.contains(IATUtils.AT_OK)){
                    if(isLocked(resp)){
                        resp="5";
                        result = result+","+resp;
                    }else{
                        resp = "";
                    }
                    continue;
                }else{
                    resp ="";
                    continue;
                }
            }else if(i == 3){
                atCmd = engconstents.ENG_AT_W_LOCKED_BAND+"8";
                resp = IATUtils.sendATCmd(atCmd,serverName);
                if(resp.contains(IATUtils.AT_OK)){
                    if(isLocked(resp)){
                        resp="8";
                        result = result+","+resp;
                    }else{
                        resp = "";
                    }
                    continue;
                }else{
                    resp ="";
                    continue;
                }
            }
        }
        Log.d(TAG,"ATChannel is "+serverName+",queryWCDMABandLocked is "+result);
        return result;
    }

    class LockFHandler extends Handler {

        public LockFHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg){
            String responValue = null;
            String gRespLock = null;
            int sim = 0;
            switch(msg.what){
                case TW_QUERY:{
                    sim = (Integer)msg.obj;
                    mServerName = "atchannel"+sim; 
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_SPFRQ1,mServerName);
                    Log.d(TAG,"<"+sim+">Channel is "+mServerName+",TW_QUERY is "+responValue);
                    Intent intent = new Intent(TW_FREQUENCY_RESULT);
                    Bundle bundle = new Bundle();
                    bundle.putString(KEY_TW_FREQUENCY, responValue);
                    bundle.putInt(KEY_TW_QUERY_SIM, sim);
                    intent.putExtras(bundle);
                    startActivity(intent);
                    break;
                }
                case G_QUERY:{
                    sim = (Integer)msg.obj;
                    mServerName = "atchannel"+sim; 
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_SPFRQ1,mServerName);
                    Log.d(TAG,"<"+sim+">Channel is "+mServerName+",G_QUERY is "+responValue);
                    Intent intent = new Intent(G_FREQUENCY_RESULT);
                    Bundle bundle = new Bundle();
                    bundle.putString(KEY_G_FREQUENCY, responValue);
                    bundle.putInt(KEY_G_QUERY_SIM, sim);
                    intent.putExtras(bundle);
                    startActivity(intent);
                    break;
                }
                case TD_BAND_QUERY:{
                    mSIM = (Integer) msg.obj;
                    if (mSIM == 0) {
                        mServerName = "atchannel0";
                        mIntentAction = "com.sprd.engineermode.action.FIRBANDMODESET";
                    } else if (mSIM == 1) {
                        mServerName = "atchannel1";
                        mIntentAction = "com.sprd.engineermode.action.SECBANDMODESET";
                    }else if(mSIM == 2){
                        mServerName = "atchannel2";
                        mIntentAction = "com.sprd.engineermode.action.TRIBANDMODESET";
                    } 
                    showProgressDialog();
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_NETMODE1, mServerName);
                    Log.d(TAG, "Dual-mode Result is " + responValue);
                    if (responValue.contains(IATUtils.AT_OK)) {
                        String[] str = responValue.split("\\:");
                        String[] str1 = str[1].split("\\,");
                        String result = str1[0].trim();
                        if (result.contains("2")) {
                            mIsDualMode = true;
                            mNetMode = "2";
                            gRespLock = IATUtils.sendATCmd(engconstents.ENG_AT_CURRENT_GSMBAND,
                                    mServerName);
                            mLockBand = IATUtils.sendATCmd(engconstents.ENG_AT_TD_LOCKED_BAND, mServerName);
                        } else if (result.contains("13")) {
                            mIsDualMode = false;
                            mNetMode = "13";
                            gRespLock = IATUtils.sendATCmd(engconstents.ENG_AT_CURRENT_GSMBAND,
                                    mServerName);
                        } else if (result.contains("15")) {
                            mIsDualMode = false;
                            mNetMode = "15";
                            mLockBand = IATUtils.sendATCmd(engconstents.ENG_AT_TD_LOCKED_BAND, mServerName);
                        }
                    }                    
                    dismissProgressDialog();
                    Intent intent = new Intent(mIntentAction);
                    Bundle bundle = new Bundle();
                    bundle.putBoolean(ISDUALMODE, mIsDualMode);
                    bundle.putInt(MODEM_TYPE, TelephonyManagerSprd.MODEM_TYPE_TDSCDMA);
                    bundle.putString(NET_MODE, mNetMode);
                    bundle.putString(G_LOCKED_BAND, gRespLock);
                    bundle.putString(LOCKED_BAND, mLockBand);
                    intent.putExtras(bundle);
                    startActivity(intent);
                    mIsDualMode = false;
                    Log.d(TAG, "<" + mSIM + ">Channel is " + mServerName + ",TD_LOCK_BAND is "
                            + mLockBand);
                    break;
                }
                case W_BAND_QUERY:{
                    mSIM = (Integer) msg.obj;
                    if (mSIM == 0) {
                        mServerName = "atchannel0";
                        mIntentAction = "com.sprd.engineermode.action.FIRBANDMODESET";
                    } else if (mSIM == 1) {
                        mServerName = "atchannel1";
                        mIntentAction = "com.sprd.engineermode.action.SECBANDMODESET";
                    }else if(mSIM == 2){
                        mServerName = "atchannel2";
                        mIntentAction = "com.sprd.engineermode.action.TRIBANDMODESET";
                    } 
                    showProgressDialog();
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_NETMODE1, mServerName);
                    Log.d(TAG, "<" + mSIM + ">Dual-mode Result is " + responValue);
                    if (responValue.contains(IATUtils.AT_OK)) {
                        String[] str = responValue.split("\\:");
                        String[] str1 = str[1].split("\\,");
                        String result = str1[0].trim();
                        if (result.contains("2")) {
                            mIsDualMode = true;
                            mNetMode = "2";
                            gRespLock = IATUtils.sendATCmd(engconstents.ENG_AT_CURRENT_GSMBAND,
                                    mServerName);
                            mLockBand = queryWCDMABandLocked(mServerName); 
                        } else if (result.contains("13")) {
                            mIsDualMode = false;
                            mNetMode = "13";
                            gRespLock = IATUtils.sendATCmd(engconstents.ENG_AT_CURRENT_GSMBAND,
                                    mServerName);
                        } else if (result.contains("14")) {
                            mIsDualMode = false;
                            mNetMode = "14";
                            mLockBand = queryWCDMABandLocked(mServerName); 
                        }
                    }                    
                    dismissProgressDialog();
                    Intent intent = new Intent(mIntentAction);
                    Bundle bundle = new Bundle();
                    bundle.putBoolean(ISDUALMODE, mIsDualMode);
                    bundle.putInt(MODEM_TYPE, TelephonyManagerSprd.MODEM_TYPE_WCDMA);
                    bundle.putString(NET_MODE, mNetMode);
                    bundle.putString(G_LOCKED_BAND, gRespLock);
                    bundle.putString(LOCKED_BAND, mLockBand);                
                    intent.putExtras(bundle);
                    startActivity(intent);
                    mIsDualMode = false;
                    Log.d(TAG, "<" + mSIM + ">Channel is " + mServerName + ",W_LOCK_BAND is "
                            + mLockBand);
                    break;
                }
                case G_BAND_QUERY:{       
                    mSIM = (Integer)msg.obj;
                    if(mSIM == 0){
                        mServerName = "atchannel0";
                        mIntentAction = "com.sprd.engineermode.action.FIRBANDMODESET";
                    }else if(mSIM == 1){
                        mServerName = "atchannel1";
                        mIntentAction = "com.sprd.engineermode.action.SECBANDMODESET";
                    }else if(mSIM == 2){
                        mServerName = "atchannel2";
                        mIntentAction = "com.sprd.engineermode.action.TRIBANDMODESET";
                    } 
                    showProgressDialog();
                    mLockBand = IATUtils.sendATCmd(engconstents.ENG_AT_CURRENT_GSMBAND,mServerName); 
                    dismissProgressDialog();
                    Intent intent = new Intent(mIntentAction);
                    Bundle bundle = new Bundle();
                    bundle.putBoolean(ISDUALMODE, mIsDualMode);                        
                    bundle.putInt(MODEM_TYPE, TelephonyManagerSprd.MODEM_TYPE_GSM);
                    bundle.putString(NET_MODE, "13");
                    bundle.putString(G_LOCKED_BAND, mLockBand);
                    intent.putExtras(bundle);
                    startActivity(intent);                        
                    Log.d(TAG,"<"+mSIM+">Channel is "+mServerName+",G_LOCK_BAND is "+mLockBand);       
                    break;
                }
                case LTE_BAND_QUERY:{
                    mSIM = (Integer) msg.obj;
                    if (mSIM == 0) {
                        mServerName = "atchannel0";
                        mIntentAction = "com.sprd.engineermode.action.FIRBANDMODESET";
                    } else if (mSIM == 1) {
                        mServerName = "atchannel1";
                        mIntentAction = "com.sprd.engineermode.action.SECBANDMODESET";
                    } else if (mSIM == 2) {
                        mServerName = "atchannel2";
                        mIntentAction = "com.sprd.engineermode.action.TRIBANDMODESET";
                    }
                    showProgressDialog();
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_NETMODE1, mServerName);
                    Log.d(TAG, "Dual-mode Result is " + responValue);
                    mNetMode = "";
                    if (responValue.contains(IATUtils.AT_OK)) {
                        String[] str = responValue.split("\\:");
                        String[] str1 = str[1].split("\\,");
                        String result = str1[0].trim();
                        /*SPRD: fix bug344931 Perfect BandSelect @{*/
                        if (result.contains("2")) {
                            mIsDualMode = true;
                            mNetMode = "2";
                            gRespLock = IATUtils.sendATCmd(engconstents.ENG_AT_CURRENT_GSMBAND,
                                    mServerName);
                            mLockBand = IATUtils.sendATCmd(engconstents.ENG_AT_TD_LOCKED_BAND,
                                    mServerName);
                            Log.d(TAG, "AT+SPLOCKBAND?" + mLockBand);
                            if (!gRespLock.contains(IATUtils.AT_OK)
                                    || !mLockBand.contains(IATUtils.AT_OK)) {
                                dismissProgressDialog();
                                return;
                            }
                        } else if (result.contains("13")) {
                            mIsDualMode = false;
                            mNetMode = "13";
                            gRespLock = IATUtils.sendATCmd(engconstents.ENG_AT_CURRENT_GSMBAND,
                                    mServerName);
                            if (!gRespLock.contains(IATUtils.AT_OK)) {
                                dismissProgressDialog();
                                return;
                            }
                        } else if (result.contains("15")) {
                            mIsDualMode = false;
                            mNetMode = "15";
                            mLockBand = IATUtils.sendATCmd(engconstents.ENG_AT_TD_LOCKED_BAND,
                                    mServerName);
                            if (!mLockBand.contains(IATUtils.AT_OK)) {
                                dismissProgressDialog();
                                return;
                            }
                        }
                        dismissProgressDialog();
                    }
                    dismissProgressDialog();
                    Intent intent = new Intent(mIntentAction);
                    Bundle bundle = new Bundle();
                    bundle.putBoolean(ISDUALMODE, mIsDualMode);
                    //bundle.putBoolean(ISSUPPORTLTE, true);
                    bundle.putInt(MODEM_TYPE, TelephonyManagerSprd.MODEM_TYPE_TDSCDMA);
                    bundle.putString(NET_MODE, mNetMode);
                    bundle.putString(G_LOCKED_BAND, gRespLock);
                    bundle.putString(LOCKED_BAND, mLockBand);
                    intent.putExtras(bundle);
                    startActivity(intent);
                    mIsDualMode = false;
                    Log.d(TAG, "<" + mSIM + ">Channel is " + mServerName + ",TD_LOCK_BAND is "
                            + mLockBand);
                }
                break;
                default:
                    break;
            }
        }
    }

    private void showProgressDialog() {
        mUiThread.post(new Runnable() {
            @Override
            public void run() {
                mProgressDialog = ProgressDialog.show(LockFrequencyActivity.this, "Query...", "Please wait...", true,
                        false);
            }
        });
    }

    private void dismissProgressDialog() {
        mUiThread.post(new Runnable() {
            @Override
            public void run() {
                if (mProgressDialog != null) {
                    mProgressDialog.dismiss();
                }
            }
        });
    }
}