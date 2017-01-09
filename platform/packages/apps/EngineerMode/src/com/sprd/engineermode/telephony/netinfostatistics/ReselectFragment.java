
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

import java.text.NumberFormat;

public class ReselectFragment extends Fragment {

    private String TAG = "ReselectFragment";
    private Handler mUiThread = new Handler();

    private HandlerThread mht;
    private NetinfoHandler netinfoHandler;
    public int ROW;
    public int COL;
    private Context mContext;
    TextView[][] mtextView;
    private String[][] mTextValue;
    private int[][] viewID;
    NumberFormat format = NumberFormat.getPercentInstance();
    private String[] firstcol;
    public int mTemp;

    private boolean isSupportLTE = !(TelephonyManagerSprd.getRadioCapbility()
            .equals(TelephonyManagerSprd.RadioCapbility.NONE));
    private boolean mFragmentDestroyed;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        Log.d(TAG, "onCreateView");
        if (isSupportLTE) {
            return inflater.inflate(R.layout.cell_reselect_lte, container,
                    false);
        } else {
            return inflater.inflate(R.layout.cell_reselect, container, false);
        }
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        Log.d(TAG, "onAttach");
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate");
        mContext = getActivity();
        if (mht == null) {
            mht = new HandlerThread(TAG);
            mht.start();
        }
        netinfoHandler = new NetinfoHandler(mht.getLooper());
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
                if (isSupportLTE) {
                    parseNetStatisticlte(simid);
                } else {
                    parseNetStatistic(simid);
                }
            }
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        Log.d(TAG, "onActivityCreated");

        if (isSupportLTE) {
            ROW = 10;
            COL = 5;
            mTemp = R.id.reselect_lte_label00;
        } else {
            ROW = 5;
            COL = 5;
            mTemp = R.id.reselet_label00;
        }
        mtextView = new TextView[ROW][COL];
        mTextValue = new String[ROW][COL];
        viewID = new int[ROW][COL];

        for (int mc = 0; mc < ROW; mc++) {
            for (int ml = 0; ml < COL; ml++) {
                viewID[mc][ml] = mTemp++;
                if (ml == (COL - 1)) {
                    mTemp++;
                }
            }
        }

        for (int i = 0; i < ROW; i++) {
            for (int j = 0; j < COL; j++) {
                mtextView[i][j] = (TextView) (getActivity()
                        .findViewById(viewID[i][j]));
            }
        }
    }

    @Override
    public void onStart() {
        super.onStart();
        Log.d(TAG, "onStart");
    }

    @Override
    public void onResume() {
        super.onResume();
        Intent intent = getActivity().getIntent();
        int simindex = intent.getIntExtra("simindex", -1);
        Log.d(TAG, "onResume simindex=" + simindex);
        Message m = netinfoHandler.obtainMessage(simindex);
        netinfoHandler.sendMessage(m);
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
        if (netinfoHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            netinfoHandler.getLooper().quit();
        }
        if (mht != null) {
            ThreadUtils.stopThread(mht);
        }
        mFragmentDestroyed = true;
        Log.d(TAG, "onDestroy");
    }

    @Override
    public void onDetach() {
        super.onDetach();
        Log.d(TAG, "onDetach");
    }

    private void parseNetStatisticlte(int simindex) {
        String atRSP = IATUtils.sendATCmd("AT+SPENGMD=0,7,1", "atchannel"
                + simindex);

        if (mFragmentDestroyed) {
            Log.d(TAG, "mFragmentDestroyed ==" + mFragmentDestroyed);
            return;
        }

        if (atRSP != null && atRSP.contains(IATUtils.AT_OK)) {
            String[] str = atRSP.split("-");
            int array_len = str.length;
            for (int i = 0; i < array_len; i++) {
                str[i] = str[i].replaceAll("\r|\n", "");
            }
            str[array_len - 1] = str[array_len - 1].substring(0,
                    str[array_len - 1].length() - 2);
            int temp = 0;
            for (int j = 1; j < ROW; j++) {
                for (int k = 1; k < COL - 1; k++) {
                    mTextValue[j][k] = str[temp++];
                    if (temp == 12) {
                        temp = temp + 2;
                    } // str[12],str[13] is useless,need to delete
                }
            }
            for (int z = 1; z < ROW; z++) {
                try {
                    if (Integer.parseInt(mTextValue[z][3]) == 0) {
                        format.setMinimumFractionDigits(2);
                        mTextValue[z][3] = format.format(1.0);
                    } else {
                        format.setMinimumFractionDigits(2);
                        mTextValue[z][3] = format.format((double) Integer
                                .parseInt(mTextValue[z][1])
                                / Integer.parseInt(mTextValue[z][3]));
                    }
                } catch (RuntimeException ex) {
                    Log.d(TAG, "shengyi parse at response fail");
                }
            }
            firstcol = mContext.getResources().getStringArray(
                    R.array.Reselect_LTE);
            for (int index = 0; index < firstcol.length; index++) {
                mTextValue[index][0] = firstcol[index];
            }
            /* reselect delay according to "at+spengmd =0,7,1" response */
            mTextValue[1][COL - 1] = str[32];
            mTextValue[2][COL - 1] = str[33];
            mTextValue[3][COL - 1] = str[29];
            mTextValue[4][COL - 1] = str[30];
            mTextValue[5][COL - 1] = str[34];
            mTextValue[6][COL - 1] = str[31];
            mTextValue[7][COL - 1] = str[35];
            mTextValue[8][COL - 1] = str[36];
            mTextValue[9][COL - 1] = str[37];

            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    for (int i = 1; i < ROW; i++) {
                        for (int j = 0; j < COL; j++) {
                            mtextView[i][j].setText(mTextValue[i][j]);
                        }
                    }
                }
            });
        } else {
            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    for (int i = 1; i < ROW; i++) {
                        for (int j = 1; j < COL; j++) {
                            mtextView[i][j].setText("NA");
                        }
                    }
                }
            });
        }
    }

    private void parseNetStatistic(int simindex) {
        String atRSP = IATUtils.sendATCmd("AT+SPENGMD=0,7,1", "atchannel"
                + simindex);
        if (mFragmentDestroyed) {
            Log.d(TAG, "mFragmentDestroyed ==" + mFragmentDestroyed);
            return;
        }

        if (atRSP != null && atRSP.contains(IATUtils.AT_OK)) {
            String[] str = atRSP.split("\n");
            String[] str1 = str[0].split("-");
            for (int i = 0; i < str1.length; i++) {
                str1[i] = str1[i].replaceAll("\r|\n", "");
            }
            int temp = 0;
            for (int j = 1; j < 5; j++) {
                for (int k = 1; k < 4; k++) {
                    mTextValue[j][k] = str1[temp++];
                }
            }
            mTextValue[1][COL - 1] = str1[32];
            mTextValue[2][COL - 1] = str1[33];
            mTextValue[3][COL - 1] = str1[29];
            mTextValue[4][COL - 1] = str1[30];
            for (int z = 1; z < 5; z++) {
                try {
                    if (Integer.parseInt(mTextValue[z][3]) == 0) {
                        format.setMinimumFractionDigits(2);
                        mTextValue[z][3] = format.format(1.0);
                    } else {
                        format.setMinimumFractionDigits(2);
                        mTextValue[z][3] = format.format((double) Integer
                                .parseInt(mTextValue[z][1])
                                / Integer.parseInt(mTextValue[z][3]));
                    }
                } catch (RuntimeException ex) {
                    Log.d(TAG, "parse at response fail");
                }
            }
            int modemType = TelephonyManagerSprd.getModemType();
            Log.d(TAG, "modemType==" + modemType);
            if (modemType == TelephonyManagerSprd.MODEM_TYPE_WCDMA) {
                mTextValue[1][0] = "W-W";
                mTextValue[2][0] = "W-G";
                mTextValue[3][0] = "G-G";
                mTextValue[4][0] = "G-W";
            } else if (modemType == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA) {
                mTextValue[1][0] = "TD-TD";
                mTextValue[2][0] = "TD-G";
                mTextValue[3][0] = "G-G";
                mTextValue[4][0] = "G-TD";
            }
            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    for (int i = 1; i < ROW; i++) {
                        for (int j = 0; j < COL; j++) {
                            mtextView[i][j].setText(mTextValue[i][j]);
                        }
                    }
                }
            });
        } else {
            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    for (int i = 1; i < ROW; i++) {
                        for (int j = 1; j < COL; j++) {
                            mtextView[i][j].setText("NA");
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
