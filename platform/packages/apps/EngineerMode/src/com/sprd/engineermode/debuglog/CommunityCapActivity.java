
package com.sprd.engineermode.debuglog;

import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.util.Log;
import android.widget.Toast;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.R.xml;
import com.sprd.engineermode.utils.IATUtils;

public class CommunityCapActivity extends PreferenceActivity {

    private static final String TAG = "CommunityCapActivity";
    private static final String KEY_MBMS = "mbms";
    private static final String KEY_HSDPA = "hsdpa";
    private static final String KEY_HSUPA = "hsupa";
    private static final String KEY_EDGE = "edge";

    private static final int GET_STATUS = 1;

    private Handler mUiThread = new Handler();
    private CCAPHandler mCCAPHandler;

    private Preference mMBMS;
    private Preference mHSDPA;
    private Preference mHSUPA;
    private Preference mEDGE;

    private String mATResponse;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_community_cap);
        mMBMS = (Preference) findPreference(KEY_MBMS);
        mHSDPA = (Preference) findPreference(KEY_HSDPA);
        mHSUPA = (Preference) findPreference(KEY_HSUPA);
        mEDGE = (Preference) findPreference(KEY_EDGE);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mCCAPHandler = new CCAPHandler(ht.getLooper());
    }

    @Override
    protected void onStart() {
        Message mStatus = mCCAPHandler.obtainMessage(GET_STATUS);
        mCCAPHandler.sendMessage(mStatus);
        super.onStart();
    }

    class CCAPHandler extends Handler {

        public CCAPHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String responValue;
            switch (msg.what) {
                case GET_STATUS: {
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_SET_SPTEST1, "atchannel0");
                    Log.d(TAG, "GET_STATUS is " + responValue);
                    if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                        String[] str = responValue.split("\\+");
                        String[] str1 = str[8].split("\\:");
                        String[] str2 = str1[1].split("\\,");
                        mATResponse = str2[1].trim();
                        if (mATResponse.equals("1")) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mMBMS.setSummary(R.string.support);
                                }
                            });
                        } else if (mATResponse.equals("2")) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mHSDPA.setSummary(R.string.support);
                                }
                            });
                        } else if (mATResponse.equals("4")) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mHSUPA.setSummary(R.string.support);
                                }
                            });
                        } else if (mATResponse.equals("8")) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mEDGE.setSummary(R.string.support);
                                }
                            });
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        if (mCCAPHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mCCAPHandler.getLooper().quit();
        }
        super.onDestroy();
    }
}
