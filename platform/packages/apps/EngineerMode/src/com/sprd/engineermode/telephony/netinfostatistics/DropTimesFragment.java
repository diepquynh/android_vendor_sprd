package com.sprd.engineermode.telephony.netinfostatistics;

import android.os.SystemProperties;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.app.Fragment;
import android.view.ViewGroup;
import android.telephony.TelephonyManager;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;
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

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.utils.ThreadUtils;

public class DropTimesFragment extends Fragment {

    private String TAG = "DropTimesFragment";
    private Handler mUiThread = new Handler();
    private HandlerThread mHT;
    private NetinfoHandler mNetinfoHandler;
    public final int ROW = 3;
    public final int COL = 2;
    private TextView[][] mTextView = new TextView[ROW][COL];
    private String[][] mTextValue = new String[ROW][COL];
    private int[][] mViewID = new int[ROW][COL];
    private boolean isSupportLTE = !(TelephonyManagerSprd.getRadioCapbility()
            .equals(TelephonyManagerSprd.RadioCapbility.NONE));
    public int mTemp;
    private boolean mFragmentDestroyed;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        // Inflate the layout for this fragment

        return inflater.inflate(R.layout.droptimes, container, false);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate");
        if (mHT == null) {
            mHT = new HandlerThread(TAG);
            mHT.start();
        }
        mNetinfoHandler = new NetinfoHandler(mHT.getLooper());
        mFragmentDestroyed = false;
    }

    class NetinfoHandler extends Handler {

        public NetinfoHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            int simid = msg.what;
            if (isSimExist(simid)) {
                parseNetStatistic(simid);
            }
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        Log.d(TAG, "onActivityCreated");
        mTemp = R.id.droptimes_label00;
        for (int mc = 0; mc < ROW; mc++) {
            for (int ml = 0; ml < COL; ml++) {
                mViewID[mc][ml] = mTemp++;
                if (ml == (COL - 1)) {
                    mTemp++;
                }
            }
        }
        for (int i = 0; i < ROW; i++) {
            for (int j = 0; j < COL; j++) {
                mTextView[i][j] = (TextView) (getActivity()
                        .findViewById(mViewID[i][j]));
            }
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        Intent intent = getActivity().getIntent();
        int simindex = intent.getIntExtra("simindex", -1);
        Log.d(TAG, "onResume simindex=" + simindex);
        Message m = mNetinfoHandler.obtainMessage(simindex);
        mNetinfoHandler.sendMessage(m);
    }

    @Override
    public void onPause() {
        super.onPause();
        Log.d(TAG, "onPause");
    }

    @Override
    public void onStop() {
        super.onStop();
        Log.d(TAG, "onStop");
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        Log.d(TAG, "onDestroyView");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mNetinfoHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mNetinfoHandler.getLooper().quit();
        }
        if (mHT != null) {
            ThreadUtils.stopThread(mHT);
        }
        mFragmentDestroyed = true;
        Log.d(TAG, "onDestroy");
    }

    private void parseNetStatistic(int simindex) {

        String atRSP = IATUtils.sendATCmd("AT+SPENGMD=0,7,0", "atchannel"
                    + simindex);
        if (mFragmentDestroyed) {
            Log.d(TAG, "mFragmentDestroyed ==" + mFragmentDestroyed);
            return;
        }
        if (atRSP != null && atRSP.contains(IATUtils.AT_OK)) {
            String[] str = atRSP.split("-");
            int array_len = str.length;
            for (int i = 0; i < str.length; i++) {
                str[i] = str[i].replaceAll("\r|\n", "");
            }
            str[array_len - 1] = str[array_len - 1].substring(0,
                    str[array_len - 1].length() - 2);
            mTextValue[2][1] = str[2];
            if (!isSupportLTE) {
                mTextValue[2][1] = "NA";
            }
            mTextValue[0][1] = str[0];
            mTextValue[1][1] = str[1];
            int modemType = TelephonyManagerSprd.getModemType();
            Log.d(TAG, "modemType==" + modemType);
            if (modemType == TelephonyManagerSprd.MODEM_TYPE_WCDMA) {
                mTextValue[0][0] = "Drop times on GSM";
                mTextValue[1][0] = "Drop times on WCDMA";
                mTextValue[2][0] = "Drop times on FDD-LTE";
            } else if (modemType == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA) {
                mTextValue[0][0] = "Drop times on GSM";
                mTextValue[1][0] = "Drop times on TD";
                mTextValue[2][0] = "Drop times on TD-LTE";
            } else if (isSupportLTE) {
                mTextValue[0][0] = "Drop times on 2G";
                mTextValue[1][0] = "Drop times on 3G";
                mTextValue[2][0] = "Drop times on 4G";
            }

            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    for (int i = 0; i < ROW; i++) {
                        for (int j = 0; j < COL; j++) {
                            mTextView[i][j].setText(mTextValue[i][j]);
                        }
                    }
                }
            });
        } else {
            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    for (int i = 0; i < ROW; i++) {
                        for (int j = 1; j < COL; j++) {
                            mTextView[i][j].setText("NA");
                        }
                    }
                }
            });
        }
    }

    private boolean isSimExist(int simIndex) {
        if (TelephonyManager.from(getActivity()).getSimState(simIndex) == TelephonyManager.SIM_STATE_READY) {
            return true;
        }
        return false;
    }

}
