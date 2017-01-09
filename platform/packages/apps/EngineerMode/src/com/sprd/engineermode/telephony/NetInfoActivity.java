
package com.sprd.engineermode.telephony;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;

import android.app.Activity;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo.State;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.widget.TextView;

import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class NetInfoActivity extends Activity {
    private static final String TAG = "NetInfoActivity";
    private Handler mUiThread = new Handler();
    private TextView mSCELL;
    private TextView mNCELL;
    private TextView mCCED1;
    private TextView mCCED2;
    private String mStrTmp = null;
    private String mStrCCED1 = null;
    private String mStrCCED2 = null;
    private static final int SCELL = 1;
    private static final int NCELL = 2;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.netinfo);
        mNCELL = (TextView) findViewById(R.id.ncell_title);
        mSCELL = (TextView) findViewById(R.id.scell_title);
        mCCED1 = (TextView) findViewById(R.id.ncell_value);
        mCCED2 = (TextView) findViewById(R.id.scell_value);

        new Thread(new Runnable() {
            @Override
            public void run() {
                if (checkCurrentNetworkState()) {
                    requestEvent(engconstents.ENG_AT_CCED + "0,1", "atchannel0");
                    requestEvent(engconstents.ENG_AT_CCED + "0,2", "atchannel0");
                } else {
                    mSCELL.setText("network is unavailable");
                    Log.d(TAG, "network is unavailable");
                }
            }
        }).start();
        mSCELL.setText("SCELL");
        mNCELL.setText("NCELL");
        mSCELL.setTextSize(20);
        mCCED1.setTextSize(20);
        mNCELL.setTextSize(20);
        mCCED2.setTextSize(20);
    }

    void myParseAtVersion(engconstents mAtResponse, String AT, Runnable runner) {
        mAtResponse.ParseAtResponse(AT);
        mUiThread.post(runner);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    private void requestEvent(final String cmd, String serverName) {
        mStrTmp = IATUtils.sendATCmd(cmd, serverName);

        myParseAtVersion(new engconstents() {
            @Override
            public void ParseAtResponse(String response) {
                // TODO Auto-generated method stub
                Log.d(TAG, "reponse:" + response);
                if (0 == cmd.compareTo(engconstents.ENG_AT_CCED + "0,1")) {
                    if (response.indexOf(",") != -1) {
                        String[] strs = response.split(",");
                        String[] strs1 = strs[0].split(":");
                        mStrCCED1 = new StringBuilder(strs1[1]).append(",")
                                .append(strs[1]).append(",").append(strs[5]).append(",")
                                .append(strs[6]).toString();
                        Log.d(TAG, mStrCCED1);
                    } else {
                        mStrCCED1 = "No Value";
                    }
                } else {
                    if (response.indexOf(",") != -1) {
                        String[] str = response.split("\n");
                        String[] strs = str[0].split(",");
                        String[] strs1 = strs[0].split(":");
                        strs[0] = strs1[1];
                        int nums = strs.length / 7;
                        mStrCCED2 = "";
                        for (int i = 0; i < nums; i++) {
                            mStrCCED2 += new StringBuilder(strs[i * 7])
                                    .append(",").append(strs[i * 7 + 1]).append(",")
                                    .append(strs[i * 7 + 5]).append(",").append(strs[i * 7 + 6]);
                            mStrCCED2 +="\n";
                            Log.d(TAG, mStrCCED2);
                        }
                    } else {
                        mStrCCED2 = "No Value";
                    }
                }
            }
        }, mStrTmp, new Runnable() {
            public void run() {
                if (0 == cmd.compareTo(engconstents.ENG_AT_CCED + "0,1")) {
                    mCCED1.setText(mStrCCED1);
                } else {
                    mCCED2.setText(mStrCCED2);
                }
            }
        });
    }

    private void updateUi(final TextView tv, final String msg) {
        mUiThread.post(new Runnable() {
            public void run() {
                mSCELL.setText("SCELL");
                mNCELL.setText("NCELL");
                mSCELL.setTextSize(20);
                mCCED1.setTextSize(20);
                mNCELL.setTextSize(20);
                mCCED2.setTextSize(20);
                tv.setText(msg);
            }
        });
    }

    private boolean checkCurrentNetworkState() {
        ConnectivityManager conMan = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        if (conMan.getNetworkInfo(ConnectivityManager.TYPE_MOBILE).getState() == State.DISCONNECTED)
            return false;
        else
            return true;
    }

}
