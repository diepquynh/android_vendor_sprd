/*This code functions as follows:
 **1, the display of the WCDMA correlation detection result
 **2, by sending AT commands to implement the query
 **3, real-time updates query results
 */
package com.sprd.engineermode.telephony.netinfo;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.view.View;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import java.lang.String;
import java.util.Timer;
import java.util.TimerTask;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;
import android.widget.LinearLayout;
import android.preference.PreferenceActivity;
import android.provider.Settings;
import android.preference.PreferenceCategory;
import android.preference.Preference;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.R;
import android.widget.ArrayAdapter;
import android.content.BroadcastReceiver;
import android.telephony.TelephonyManager;

public class WcdmaShowActivity extends Activity {

    private static final String TAG = "WcdmaShowActivity";
    private static final String PREF_INDEX = "PrefenceIndex";
    private static final String KEY_SIM_INDEX = "simindex";
    private static final String NETWORK_STAT_CHANGE = "com.sprd.network.NETWORK_STAT_CHANGE";
    private static final String NETWORK_TYPE = "NetWorkType";

    private static final int NETWORK_UNKNOW = 0;
    private static final int NETWORK_GSM = 1;
    private static final int NETWORK_TDSCDMA = 2;
    private static final int NETWORK_WCDMA = 3;
    private static final int NETWORK_LTE = 4;

    private static final int KEY_SERVING = 0;
    private static final int KEY_ADJACENT = 1;
    private static final int KEY_BETWEEN_ADJACENT_2G = 2;
    private static final int KEY_BETWEEN_ADJACENT_4G = 3;
    private static final int KEY_OUTFIELD_NETWORK = 4;

    private static final int GET_SERVING = 0;
    private static final int GET_ADJACENT = 1;
    private static final int GET_BETWEEN_ADJACENT_2G = 2;
    private static final int GET_BETWEEN_ADJACENT_4G = 3;
    private static final int GET_OUTFIELD_NETWORK = 4;

    private String SaveName = "atchannel";
    private int mCheckId;
    private int mSimIndex;
    private int mSubId;

    private Timer mTimer;
    private WcdmaHandler mWcdmaHandler;
    private Handler mUiThread = new Handler();
    private static final Object mLock = new Object();
    private ListView listView;
    private ListView mlistviewOperators;
    TextView[][] mtextView;
    private String[][] mTextValue;
    private int[][] viewID;
    public int ROW;
    public int COL;
    public int mTemp;

    public boolean isRoaming;
    public String mNetworkOperator;
    public String mNetworkOperatorName;

    private TelephonyManager mTelephonyManager;

    private int[] activityLab = new int[] { R.string.netinfo_server_cell,
            R.string.netinfo_neighbour_cell, R.string.netinfo_adjacent_cell_2g,
            R.string.netinfo_adjacent_cell_4g,
            R.string.netinfo_outfield_information };
    private String[][] Description_name = {
            { "Not Support", "Support" },
            { "Not Support", "Support" },
            { "Unknown", "GEA1", "GEA2", "GEA3" },
            { "A51", "A52", "A53", "A54", "A55", "A56", "A57" },
            { "Unknown", "UEA0", "UEA1" },
            { "Not support Hsdpa and Hsupa", "Support Hsdpa", "Support Hsupa",
                    "Support Hsdpa and Hsupa" }, { "Not Support", "Support" },
            { "Not Support", "VAMOS1", "VAMOS2" },
            { "Not Support", "Support" }, { "Not Support", "Support" },
            { "other", "R8", "R9" }, { "Not Support", "Support" },
            { "Unknown", "eea0", "eea1", "eea2" },
            { "Unknown", "eia0 ", "eia1", "eia2" } };

    private BroadcastReceiver mNetWorkReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            // TODO Auto-generated method stub
            int netWorkStat = intent.getIntExtra(NETWORK_TYPE, 0);
            int phoneId = intent.getIntExtra(KEY_SIM_INDEX, -1);
            String mIntentAction;
            Log.d(TAG, "NetWork state is " + netWorkStat);
            synchronized (mLock) {
                if ((netWorkStat != NETWORK_WCDMA) && (phoneId == mSimIndex)) {
                    switch (netWorkStat) {
                    case NETWORK_GSM:
                        mIntentAction = "android.intent.action.GSMSHOW";
                        break;
                    case NETWORK_TDSCDMA:
                        mIntentAction = "android.intent.action.TDSCDMASHOW";
                        break;
                    case NETWORK_LTE:
                        mIntentAction = "android.intent.action.LTESHOW";
                        break;
                    default:
                        return;
                    }
                    unregisterReceiver();
                    if (mTimer != null) {
                        mTimer.cancel();
                    }
                    Bundle data = new Bundle();
                    data.putInt("PrefenceIndex", mCheckId);
                    data.putInt("SubId", mSubId);
                    data.putInt(KEY_SIM_INDEX, mSimIndex);
                    Intent mIntent = new Intent(mIntentAction);
                    mIntent.putExtras(data);
                    startActivity(mIntent);
                    finish();
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Intent intent = this.getIntent();
        Bundle data = intent.getExtras();
        if(data == null){
            return;
        }
        mCheckId = data.getInt(PREF_INDEX, -1);
        mSimIndex = data.getInt("simindex", -1);
        mSubId = data.getInt("SubId", -1);
        Log.d(TAG, "mCheckId is: " + mCheckId + "mSimIndex is: " + mSimIndex
                + "SubId: " + mSubId);
        registerReceiver();
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mWcdmaHandler = new WcdmaHandler(ht.getLooper());
        if (mCheckId != -1 && mSimIndex != -1) {
            mTelephonyManager = (TelephonyManager) TelephonyManager
                    .from(WcdmaShowActivity.this);
            mNetworkOperatorName = mTelephonyManager.getNetworkOperatorName(mSubId);
            mNetworkOperator = mTelephonyManager.getNetworkOperatorForPhone(mSimIndex);
            isRoaming = mTelephonyManager.isNetworkRoaming(mSubId);
            Log.d(TAG, "NetworkOperatorName: " + mNetworkOperatorName
                    + "NetworkOperator: " + mNetworkOperator + "isRoaming: "
                    + isRoaming);
            setTitle(activityLab[mCheckId]);
            SaveName = "atchannel" + mSimIndex;
            switch (mCheckId) {
            case KEY_SERVING:
                setContentView(R.layout.netinfo_listview_show);
                listView = (ListView) findViewById(R.id.netinfo_listview);
                mlistviewOperators = (ListView) findViewById(R.id.netinfo_operators_listview);
                String[] values = this.getResources().getStringArray(R.array.operators_sering_name);
                if (mNetworkOperator != null) {
                    values[0] = values[0] + ": " + mNetworkOperator;
                } else {
                    values[0] = values[0] + ": NA";
                }
                if (mNetworkOperatorName != null) {
                    values[1] = values[1] + ": " + mNetworkOperatorName;
                } else {
                    values[1] = values[1] + ": NA";
                }
                if (isRoaming) {
                    values[2] = values[2] + ": " + getResources().getString(R.string.is_roaming);
                } else {
                    values[2] = values[2] + ": " + getResources().getString(R.string.non_roaming);
                }
                ArrayAdapter<String> operatorsAdapter = new ArrayAdapter<String>(
                        WcdmaShowActivity.this, R.layout.array_item, values);
                mlistviewOperators.setAdapter(operatorsAdapter);
                Message serving = mWcdmaHandler.obtainMessage(GET_SERVING);
                mWcdmaHandler.sendMessage(serving);
                break;
            case KEY_ADJACENT:
                setContentView(R.layout.netinfo_between_adjacent_3g);
                TextView mTextViewPsc = (TextView) (WcdmaShowActivity.this
                        .findViewById(R.id.netinfo_adjacent_3g_label02));
                mTextViewPsc.setText(R.string.netinfo_psc);
                ROW = 5;
                COL = 3;
                mtextView = new TextView[ROW][COL];
                mTextValue = new String[ROW][COL];
                viewID = new int[ROW][COL];
                mTemp = R.id.netinfo_adjacent_3g_label11;
                for (int mc = 0; mc < ROW; mc++) {
                    for (int ml = 0; ml < COL; ml++) {
                        viewID[mc][ml] = mTemp++;
                        if (ml == (COL - 1)) {
                            mTemp = mTemp + 2;
                        }
                    }
                }
                for (int i = 0; i < ROW; i++) {
                    for (int j = 0; j < COL; j++) {
                        mtextView[i][j] = (TextView) (WcdmaShowActivity.this
                                .findViewById(viewID[i][j]));
                    }
                }
                Message adjacent = mWcdmaHandler.obtainMessage(GET_ADJACENT);
                mWcdmaHandler.sendMessage(adjacent);
                break;
            case KEY_BETWEEN_ADJACENT_2G:
                setContentView(R.layout.netinfo_between_adjacent_3g);
                TextView mTextViewPsc_2g = (TextView) (WcdmaShowActivity.this
                        .findViewById(R.id.netinfo_adjacent_3g_label02));
                mTextViewPsc_2g.setText(R.string.netinfo_psc);
                ROW = 5;
                COL = 3;
                mtextView = new TextView[ROW][COL];
                mTextValue = new String[ROW][COL];
                viewID = new int[ROW][COL];
                mTemp = R.id.netinfo_adjacent_3g_label11;
                for (int mc = 0; mc < ROW; mc++) {
                    for (int ml = 0; ml < COL; ml++) {
                        viewID[mc][ml] = mTemp++;
                        if (ml == (COL - 1)) {
                            mTemp = mTemp + 2;
                        }
                    }
                }
                for (int i = 0; i < ROW; i++) {
                    for (int j = 0; j < COL; j++) {
                        mtextView[i][j] = (TextView) (WcdmaShowActivity.this
                                .findViewById(viewID[i][j]));
                    }
                }
                Message adjacent_2g = mWcdmaHandler
                        .obtainMessage(GET_BETWEEN_ADJACENT_2G);
                mWcdmaHandler.sendMessage(adjacent_2g);
                break;
            case KEY_BETWEEN_ADJACENT_4G:
                setContentView(R.layout.netinfo_between_adjacent_4g);
                ROW = 5;
                COL = 4;
                mtextView = new TextView[ROW][COL];
                mTextValue = new String[ROW][COL];
                viewID = new int[ROW][COL];
                mTemp = R.id.netinfo_adjacent_4g_label11;
                for (int mc = 0; mc < ROW; mc++) {
                    for (int ml = 0; ml < COL; ml++) {
                        viewID[mc][ml] = mTemp++;
                        if (ml == (COL - 1)) {
                            mTemp = mTemp + 2;
                        }
                    }
                }
                for (int i = 0; i < ROW; i++) {
                    for (int j = 0; j < COL; j++) {
                        mtextView[i][j] = (TextView) (WcdmaShowActivity.this
                                .findViewById(viewID[i][j]));
                    }
                }
                Message adjacent_4g = mWcdmaHandler
                        .obtainMessage(GET_BETWEEN_ADJACENT_4G);
                mWcdmaHandler.sendMessage(adjacent_4g);
                break;
            case KEY_OUTFIELD_NETWORK:
                setContentView(R.layout.netinfo_outfield_show);
                listView = (ListView) findViewById(R.id.netinfo_outfield_listview);
                Message outfieldNetwork = mWcdmaHandler
                        .obtainMessage(GET_OUTFIELD_NETWORK);
                mWcdmaHandler.sendMessage(outfieldNetwork);
                break;
            }
        } else {
            if (mWcdmaHandler != null) {
                mWcdmaHandler.getLooper().quit();
            }
            return;
        }
        mTimer = new Timer();
        mTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                switch (mCheckId) {
                case KEY_SERVING:
                    Message serving = mWcdmaHandler.obtainMessage(GET_SERVING);
                    mWcdmaHandler.sendMessage(serving);
                    break;
                case KEY_ADJACENT:
                    Message adjacent = mWcdmaHandler
                            .obtainMessage(GET_ADJACENT);
                    mWcdmaHandler.sendMessage(adjacent);
                    break;
                case KEY_BETWEEN_ADJACENT_2G:
                    Message adjacent_2g = mWcdmaHandler
                            .obtainMessage(GET_BETWEEN_ADJACENT_2G);
                    mWcdmaHandler.sendMessage(adjacent_2g);
                    break;
                case KEY_BETWEEN_ADJACENT_4G:
                    Message adjacent_4g = mWcdmaHandler
                            .obtainMessage(GET_BETWEEN_ADJACENT_4G);
                    mWcdmaHandler.sendMessage(adjacent_4g);
                    break;
                case KEY_OUTFIELD_NETWORK:
                    Message outfieldNetwork = mWcdmaHandler
                            .obtainMessage(GET_OUTFIELD_NETWORK);
                    mWcdmaHandler.sendMessage(outfieldNetwork);
                    break;
                }
            }
        }, 0, 2000);
    }

    @Override
    protected void onDestroy() {
        if (mTimer != null) {
            mTimer.cancel();
        }
        if (mWcdmaHandler != null) {
            mWcdmaHandler.getLooper().quit();
        }
        unregisterReceiver();
        super.onDestroy();
    }

    public void registerReceiver() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(NETWORK_STAT_CHANGE);
        registerReceiver(mNetWorkReceiver, filter);
    }

    public void unregisterReceiver() {
        try {
            unregisterReceiver(mNetWorkReceiver);
        } catch (IllegalArgumentException iea) {
            // Ignored.
        }
    }

    public void TextValuesDisplay(int cmdType) {
        String result = null;
        int temp;
        switch (cmdType) {
        case KEY_ADJACENT:
            result = IATUtils.sendATCmd("AT+SPENGMD=0,6,2", SaveName);
            Log.d(TAG, "AT+SPENGMD=0,6,2: " + result);
            if (result.contains(IATUtils.AT_OK)) {
                result = result.replaceAll(",-", ",+");
                result = result.replaceAll("--", "-+");
                String[] str1 = result.split("\n");
                String[] str2 = str1[0].split("-");
                for (int i = 0; i < 5; i++) {
                    if (i < str2.length) {
                        if (str2[i].contains(",")) {
                            String[] str3 = str2[i].split(",");
                            for (int j = 0; j < 3; j++) {
                                if (j == 2) {
                                    temp = Integer.parseInt(str3[j].replace("+", "-").trim());
                                    mTextValue[i][j] = String.valueOf(temp) + "dBm";
                                } else {
                                    mTextValue[i][j] = str3[j]
                                            .replace("+", "-");
                                }
                            }
                        }
                    } else {
                        for (int j = 0; j < 3; j++) {
                            mTextValue[i][j] = "NA";
                        }
                    }
                }
            } else {
                for (int i = 0; i < ROW; i++) {
                    for (int j = 0; j < COL; j++) {
                        mTextValue[i][j] = "NA";
                    }
                }
            }
            break;
        case KEY_BETWEEN_ADJACENT_2G:
            result = IATUtils.sendATCmd("AT+SPENGMD=0,6,4", SaveName);
            Log.d(TAG, "AT+SPENGMD=0,6,4: " + result);
            if (result.contains(IATUtils.AT_OK)) {
                result = result.replaceAll(",-", ",+");
                result = result.replaceAll("--", "-+");
                String[] str1 = result.split("\n");
                String[] str2 = str1[0].split("-");
                for (int i = 0; i < 5; i++) {
                    if (i < (str2.length - 3)) {
                        if (str2[i + 3].contains(",")) {
                            String[] str3 = str2[i + 3].split(",");
                            for (int j = 1; j < 4; j++) {
                                if (j == 3) {
                                    mTextValue[i][j - 1] = str3[j].replace("+",
                                            "-") + "dBm";
                                } else {
                                    mTextValue[i][j - 1] = str3[j].replace("+",
                                            "-");
                                }
                            }
                        }
                    } else {
                        for (int j = 0; j < 3; j++) {
                            mTextValue[i][j] = "NA";
                        }
                    }
                }
            } else {
                for (int i = 0; i < ROW; i++) {
                    for (int j = 0; j < COL; j++) {
                        mTextValue[i][j] = "NA";
                    }
                }
            }
            break;
        case KEY_BETWEEN_ADJACENT_4G:
            result = IATUtils.sendATCmd("AT+SPENGMD=0,0,4", SaveName);
            Log.d(TAG, "AT+SPENGMD=0,0,4: " + result);
            int num_4g = 0;
            if (result.contains(IATUtils.AT_OK)) {
                result = result.replaceAll(",-", ",+");
                result = result.replaceAll("--", "-+");
                String[] str1 = result.split("\n");
                String[] str2 = str1[0].split("-");
                for (int i = 0; i < str2.length; i++) {
                    if (num_4g < 5) {
                        if (str2[i].contains(",")) {
                            String[] str3 = str2[i].split(",");
                            for (int j = 0; j < 4; j++) {
                                if (j == 3 || j == 2) {
                                    if (j == 2) {
                                        temp = Integer.parseInt(str3[j].replace("+", "-").trim()) - 120;
                                    } else {
                                        temp = Integer.parseInt(str3[j].replace("+", "-").trim()) - 49;
                                    }
                                    mTextValue[num_4g][j] = String.valueOf(temp) + "dBm";
                                } else {
                                    mTextValue[num_4g][j] = str3[j].replace(
                                            "+", "-");
                                }
                            }
                            num_4g++;
                        }
                    } else {
                        break;
                    }
                }
                if (num_4g < 5) {
                    for (int i = num_4g; i < 5; i++) {
                        for (int j = 0; j < 4; j++) {
                            mTextValue[i][j] = "NA";
                        }
                    }
                }
            } else {
                for (int i = 0; i < ROW; i++) {
                    for (int j = 0; j < COL; j++) {
                        mTextValue[i][j] = "NA";
                    }
                }
            }
            break;
        }
        mUiThread.post(new Runnable() {
            @Override
            public void run() {
                for (int i = 0; i < ROW; i++) {
                    for (int j = 0; j < COL; j++) {
                        mtextView[i][j].setText(mTextValue[i][j]);
                    }
                }
            }
        });
    }

    public void valuesDisplay(int cmdType) {
        String result = null;
        int temp;
        final String[] values;
        if (cmdType == KEY_SERVING) {
            values = this.getResources().getStringArray(
                    R.array.Wcdma_sering_name);
            result = IATUtils.sendATCmd("AT+SPENGMD=0,6,1", SaveName);
            Log.d(TAG, "AT+SPENGMD=0,6,1: " + result);
            if (result.contains(IATUtils.AT_OK)) {
                result = result.replaceAll(",-", ",+");
                result = result.replaceAll("--", "-+");
                String[] str1 = result.split("\n");
                String[] str2 = str1[0].split("-");
                for (int i = 0; i < str2.length; i++) {
                    if (i == 0) {
                        values[2] = values[2] + ": "
                                + str2[i].replace("+", "-");
                    }
                    if (i == 1) {
                        values[3] = values[3] + ": "
                                + str2[i].replace("+", "-");
                    }
                    if (i == 2) {
                        temp = Integer.parseInt(str2[i].replace("+", "-").trim());
                        values[5] = values[5] + ": "
                                + String.valueOf(temp) + "dBm";
                    }
                    if (i == 15) {
                        values[4] = values[4] + ": "
                                + Float.parseFloat(str2[i].replace("+", "-")) * 10 / 20;
                    }
                    if (i == 16) {
                        values[0] = values[0] + ": "
                                + str2[i].replace("+", "-");
                    }
                    if (i == 17) {
                        values[1] = values[1] + ": "
                                + str2[i].replace("+", "-");
                    }
                }
            } else {
                for (int i = 0; i < 6; i++) {
                    values[i] = values[i] + ": NA";
                }
            }
            int num = 6;
            result = IATUtils.sendATCmd("AT+SPENGMD=0,0,5", SaveName);
            Log.d(TAG, "AT+SPENGMD=0,0,5: " + result);
            if (result.contains(IATUtils.AT_OK)) {
                result = result.replaceAll(",-", ",+");
                result = result.replaceAll("--", "-+");
                String[] str1 = result.split("\n");
                String[] str2 = str1[0].split("-");
                if ("".equals(str2[0]) && str2.length > 1) {
                    for (int i = 1; i < str2.length; i++) {
                        if (i == 1) {
                            if (str2[1].contains(",")) {
                                String[] str3 = str2[1].split(",");
                                for (int j = 0; j < 3 && j < str3.length; j++) {
                                    if (j==0) {
                                        values[num] = values[num] + ": "
                                                + "-" + str3[0];
                                    } else {
                                        values[num] = values[num] + ": "
                                                + str3[j].replace("+", "-");
                                    }
                                    num++;
                                }
                            } else {
                                for (int j = 0; j < 3; j++) {
                                    values[num] = values[num] + ": "
                                            + str2[1].replace("+", "-");
                                    num++;
                                }
                            }
                        } else {
                            if (num < values.length) {
                                values[num] = values[num] + ": "
                                        + str2[i].replace("+", "-");
                                num++;
                            }
                        }
                    }
                } else {
                    for (int i = 0; i < str2.length; i++) {
                        if (i == 0) {
                            if (str2[0].contains(",")) {
                                String[] str3 = str2[0].split(",");
                                for (int j = 0; j < 3; j++) {
                                    values[num] = values[num] + ": "
                                            + str3[j].replace("+", "-");
                                    num++;
                                }
                            } else {
                                for (int j = 0; j < 3; j++) {
                                    values[num] = values[num] + ": "
                                            + str2[0].replace("+", "-");
                                    num++;
                                }
                            }
                        } else {
                            if (num < values.length) {
                                values[num] = values[num] + ": "
                                        + str2[i].replace("+", "-");
                                num++;
                            }
                        }
                    }
                }
            } else {
                for (int i = 6; i < values.length; i++) {
                    values[i] = values[i] + ": NA";
                }
            }
            result = IATUtils.sendATCmd("AT+SPENGMD=0,0,8", SaveName);
            Log.d(TAG, "AT+SPENGMD=0,0,8: " + result);
            if (result.contains(IATUtils.AT_OK)){
                String[] str = result.split("-");
                if(str.length<18){
                    values[11]=values[11]+": " + "NO";
                } else {
                    if("2".equals(str[13])){
                        values[11]=values[11]+": " + "YES";
                    }else{
                        values[11]=values[11]+": " + "NO";
                    }
                }
            }else{
                values[11]=values[11]+": " + "NO";
            }

            num = 12;
            result = IATUtils.sendATCmd("AT+SPENGMD=0,5,2", SaveName);
            Log.d(TAG, "AT+SPENGMD=0,5,2: " + result);
            if (result.contains(IATUtils.AT_OK)) {
                result = result.replaceAll(",-", ",+");
                result = result.replaceAll("--", "-+");
                String[] str1 = result.split("\n");
                String[] str2 = str1[0].split("-");

                int i=0;
                for (; i < values.length - num && i<str2.length; i++) {
                    if(i == 3 || i == 7){
                        values[i + num] = values[i + num] + ": " + str2[i].replace("+", "-") + "dBm";
                    }else{
                        values[i + num] = values[i + num] + ": " + str2[i].replace("+", "-");
                    }
                }
                for(;i < values.length - num;i++){
                    values[i + num] = values[i + num] + ": " + "NA";
                }
            } else {
                for (int i = 0; i < values.length - num; i++) {
                    values[i + num] = values[i + num] + ": " + "NA";
                }
            }

            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    ArrayAdapter<String> adapter = new ArrayAdapter<String>(
                            WcdmaShowActivity.this, R.layout.array_item, values);
                    listView.setAdapter(adapter);
                }
            });
        } else if (cmdType == KEY_OUTFIELD_NETWORK) {
            values = this.getResources().getStringArray(
                    R.array.Wcdma_Outfield_Network);
            int num = 0;
            result = IATUtils.sendATCmd("AT+SPENGMD=0,0,7", SaveName);
            Log.d(TAG, "AT+SPENGMD=0,0,7: " + result);
            if (result.contains(IATUtils.AT_OK)) {
                result = result.replaceAll("--", "-+");
                String[] str1 = result.split("\n");
                String[] str2 = str1[0].split("-");
                for (int i = 0; i < str2.length; i++) {
                    if (i == 4 || i == 5 || i == 6 || i == 8 || i == 10
                            || i == 11) {
                        if (num < values.length) {
                            values[num] = values[num]
                                    + ": "
                                    + Description_name[i][Integer
                                            .valueOf(str2[i].substring(0, 1))];
                            num++;
                        }
                    }
                }
            } else {
                for (int i = 0; i < values.length; i++) {
                    values[i] = values[i] + ": NA";
                }
            }
            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    ArrayAdapter<String> adapter = new ArrayAdapter<String>(
                            WcdmaShowActivity.this, R.layout.array_item, values);
                    listView.setAdapter(adapter);
                }
            });
        }
    }

    class WcdmaHandler extends Handler {
        public WcdmaHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case GET_SERVING:
                valuesDisplay(mCheckId);
                break;
            case GET_ADJACENT:
                TextValuesDisplay(mCheckId);
                break;
            case GET_BETWEEN_ADJACENT_2G:
                TextValuesDisplay(mCheckId);
                break;
            case GET_BETWEEN_ADJACENT_4G:
                TextValuesDisplay(mCheckId);
                break;
            case GET_OUTFIELD_NETWORK:
                valuesDisplay(mCheckId);
                break;
            }
        }
    }
}
