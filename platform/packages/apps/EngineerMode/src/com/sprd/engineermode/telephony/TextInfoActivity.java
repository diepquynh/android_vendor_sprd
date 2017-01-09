
package com.sprd.engineermode.telephony;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import android.content.Context;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.widget.TextView;
import android.telephony.TelephonyManager;

//import com.android.internal.telephony.IccCardApplication.AppType;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.R.id;
import com.sprd.engineermode.R.layout;
import com.sprd.engineermode.R.string;
import com.sprd.engineermode.utils.IATUtils;

public class TextInfoActivity extends Activity {

    static final String TAG = "TextInfoActivity";

    private TextView mTextView;
    private static final int FPLMN = 1;
    private static final int NETINFOSTATIS = 3;
    private static final int APN_QUERY = 4;
    private static final int SIM_INFO = 5;

    private String mATline;
    private int mSocketID;
    private int mStartN;
    private String mStrTmp = null;
    private String mStrDisplay = null;
    private Handler mUiThread = new Handler();
    private TextinfoHandler textinfoHandler;
    private String mATResponse;
    private String mATResponse1;
    private String mAnalysisResponse;
    private String mInfo = null;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.textinfo);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        textinfoHandler = new TextinfoHandler(ht.getLooper());
        mTextView = (TextView) findViewById(R.id.text_view);
        Intent intent = this.getIntent();
        mStartN = intent.getIntExtra("text_info", 0);

        switch (mStartN) {
            case FPLMN: {
                setTitle(R.string.sim_forbid_plmn);
                Message m = textinfoHandler.obtainMessage(FPLMN);
                textinfoHandler.sendMessage(m);
            }
                break;
            case 2:
                setTitle(R.string.sim_equal_plmn);
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        requestEvent(engconstents.ENG_AT_COPS, "atchannel0");
                    }
                }).start();
                break;
            case NETINFOSTATIS: {
                setTitle("NetInfo Statistics");
                Message m = textinfoHandler.obtainMessage(NETINFOSTATIS);
                textinfoHandler.sendMessage(m);
            }
                break;
            case APN_QUERY: {
                setTitle("APN Query");
                Message m = textinfoHandler.obtainMessage(APN_QUERY);
                textinfoHandler.sendMessage(m);
            }
                break;
            case SIM_INFO: {
                setTitle("SIM Info");
                Message m = textinfoHandler.obtainMessage(SIM_INFO);
                textinfoHandler.sendMessage(m);
            }
                break;
            default:
                Log.e(TAG, "mStartN:" + mStartN);
        }
    }

    void myParseAtVersion(engconstents mAtResponse, String AT, Runnable runner) {
        mAtResponse.ParseAtResponse(AT);
        mUiThread.post(runner);
    }

    private void requestEvent(String cmd, String serverName) {
        mStrTmp = IATUtils.sendATCmd(cmd, serverName);

        myParseAtVersion(new engconstents() {
            @Override
            public void ParseAtResponse(String response) {
                // TODO Auto-generated method stub
                Log.d("TAG", "reponse:" + response);

                if (response.contains(IATUtils.AT_OK)) {
                    String[] strs = response.split("\n");
                    mStrDisplay = strs[0];
                } else {
                    mStrDisplay = response;
                }

            }
        }, mStrTmp, new Runnable() {
            public void run() {
                mTextView.setText(mStrDisplay);
            }
        });
    }

    private int phoneCount() {
        return TelephonyManager.from(TextInfoActivity.this).getPhoneCount();
    }

    private String paserSimInfo(String atRSP) {
        if (atRSP.contains(IATUtils.AT_OK)) {
            String infoPasered = "IMSI:";

            String[] imsi = atRSP.split("\n");
            infoPasered += imsi[0] + "\n";

            String tempStrmcc = "";
            String tempStrmnc = "";
            tempStrmcc = atRSP.substring(0, 3);
            tempStrmnc = atRSP.substring(3, 5);
            infoPasered += "MCC:";
            infoPasered += tempStrmcc + "\n";
            infoPasered += "MNC:";
            infoPasered += tempStrmnc + "\n";
            return infoPasered;
        } else {
            return "";
        }
    }

    private String paserRPLMN(String atRSP) {
        if (atRSP.contains(IATUtils.AT_OK)) {
            String infoPasered = "RPLMN:";

            String[] tmpStr = atRSP.split("\n");
            String[] tmpStr1 = tmpStr[0].split(":");
            infoPasered += tmpStr1[1];
            infoPasered += "\n";

            return infoPasered;
        } else {
            return "\n";
        }
    }

    private String paserFPLMN(String atRSP) {
        final int FPLMN_MAX = 4;
        String infoPasered = "";
        if (atRSP.contains(IATUtils.AT_OK)) {
            String[] str = atRSP.split(",");
            for (int i = 0; i < FPLMN_MAX; i++) {
                int start = i * 6;
                if (0 != str[2].substring(start + 1, start + 2).compareTo("F")) {
                    infoPasered += "MCC:";
                    infoPasered += str[2].substring(start + 1, start + 2);
                    infoPasered += str[2].substring(start + 0, start + 1);
                    infoPasered += str[2].substring(start + 3, start + 4);
                    infoPasered += " ";
                    infoPasered += "MNC:";
                    if (0 != str[2].substring(start + 2, start + 3).compareTo(
                            "F")) {
                        infoPasered += str[2].substring(start + 2, start + 3);
                    }
                    infoPasered += str[2].substring(start + 5, start + 6);
                    infoPasered += str[2].substring(start + 4, start + 5);
                    infoPasered += "\n";
                }
            }

            return infoPasered;
        } else {
            return "Error";
        }
    }

    private boolean isUsim(int simIndex) {
        mATResponse = IATUtils.sendATCmd(
                "AT+CRSM=192,28539,0,0,15,0,\"3F007FFF\"", "atchannel"
                        + simIndex);

        Log.d(TAG, mATResponse);
        if (mATResponse.contains(IATUtils.AT_OK)) {
            String[] str = mATResponse.split(",");
            if (0 == str[2].substring(0, 1).compareTo("0")
                    && 0 == str[2].substring(1, 2).compareTo("0")
                    && 0 == str[2].substring(2, 3).compareTo("0")) {
                return true;
            }
        }

        return false;
    }

    private boolean isSimExist(int simIndex) {
        if (TelephonyManager.from(TextInfoActivity.this).getSimState(simIndex) == TelephonyManager.SIM_STATE_READY) {
            return true;
        }
        return false;
    }

    class TextinfoHandler extends Handler {

        public TextinfoHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case FPLMN: {
                    String cmd = null;
                    mInfo = "";
                    for (int simIndex = 0; simIndex < phoneCount(); simIndex++) {
                        if (isSimExist(simIndex)) {
                            if (isUsim(simIndex)) {
                                cmd = engconstents.ENG_RPLMN_USIM;
                            } else {
                                cmd = engconstents.ENG_RPLMN_SIM;
                            }
                            mATResponse = IATUtils.sendATCmd(cmd, "atchannel"
                                    + simIndex);
                            Log.d(TAG, mATResponse);
                            int simNumber = simIndex + 1;
                            mInfo += "SIM" + simNumber + ":\n";
                            mInfo += paserFPLMN(mATResponse);
                        }
                    }

                    if (mInfo.length() == 0) {
                        mInfo = "No sim";
                    }

                    mUiThread.post(new Runnable() {

                        @Override
                        public void run() {
                            mTextView.setText(mInfo);
                        }

                    });
                }
                    break;
                case NETINFOSTATIS: {
                    mATResponse = IATUtils.sendATCmd(
                            engconstents.ENG_AT_NETINFO_STATI, "atchannel0");
                    mUiThread.post(new Runnable() {

                        @Override
                        public void run() {
                            mTextView.setText(mATResponse);
                        }

                    });
                }
                    break;
                case APN_QUERY: {
                    mATResponse = IATUtils.sendATCmd(engconstents.ENG_AT_APNQUERY,
                            "atchannel0");
                    mUiThread.post(new Runnable() {

                        @Override
                        public void run() {
                            mTextView.setText(mATResponse);
                        }

                    });
                }
                    break;
                case SIM_INFO: {
                    mInfo = "";
                    for (int i = 0, simIndex = 1; i < phoneCount(); i++, simIndex++) {
                        mInfo += "SIM" + simIndex + " Infos:\n";
                        mATResponse = IATUtils.sendATCmd(
                                engconstents.ENG_AT_GETHPLMN, "atchannel" + i);
                        mInfo += paserSimInfo(mATResponse);
                        mATResponse1 = IATUtils.sendATCmd(
                                engconstents.ENG_AT_GETRPLMN, "atchannel" + i);
                        mInfo += paserRPLMN(mATResponse1);
                        mInfo += "\n";
                    }
                    mUiThread.post(new Runnable() {

                        @Override
                        public void run() {
                            mTextView.setText(mInfo);
                        }

                    });
                }
                    break;
            }

        }
    }

    @Override
    protected void onDestroy() {
        if (textinfoHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            textinfoHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        finish();
        super.onBackPressed();
    }

}
