/*This code functions as follows:
 **1, the display of the GSM correlation detection result
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

public class GsmShowActivity extends Activity {

    private static final String TAG = "GsmShowActivity";
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
    private static final int KEY_BETWEEN_ADJACENT_3G = 2;
    private static final int KEY_BETWEEN_ADJACENT_4G = 3;
    private static final int KEY_OUTFIELD_NETWORK = 4;

    private static final int GET_SERVING = 0;
    private static final int GET_ADJACENT = 1;
    private static final int GET_BETWEEN_ADJACENT_3G = 2;
    private static final int GET_BETWEEN_ADJACENT_4G = 3;
    private static final int GET_OUTFIELD_NETWORK = 4;

    private String SaveName = "atchannel";
    private int mCheckId;
    private int mSimIndex;
    private int mSubId;

    private Timer mTimer;
    private GsmHandler mGsmHandler;
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
            R.string.netinfo_neighbour_cell, R.string.netinfo_adjacent_cell_3g,
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
                if ((netWorkStat != NETWORK_GSM) && (phoneId == mSimIndex)) {
                    switch (netWorkStat) {
                    case NETWORK_TDSCDMA:
                        mIntentAction = "android.intent.action.TDSCDMASHOW";
                        break;
                    case NETWORK_WCDMA:
                        mIntentAction = "android.intent.action.WCDMASHOW";
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
        mGsmHandler = new GsmHandler(ht.getLooper());
        if ((mCheckId != -1) && (mSimIndex != -1)) {
            mTelephonyManager = (TelephonyManager) TelephonyManager
                    .from(GsmShowActivity.this);
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
                        GsmShowActivity.this, R.layout.array_item, values);
                mlistviewOperators.setAdapter(operatorsAdapter);
                Message serving = mGsmHandler.obtainMessage(GET_SERVING);
                mGsmHandler.sendMessage(serving);
                break;
            case KEY_ADJACENT:
                setContentView(R.layout.netinfo_adjacent_show);
                ROW = 5;
                COL = 5;
                mtextView = new TextView[ROW][COL];
                mTextValue = new String[ROW][COL];
                viewID = new int[ROW][COL];
                mTemp = R.id.netinfo_adjacent_label11;
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
                        mtextView[i][j] = (TextView) (GsmShowActivity.this
                                .findViewById(viewID[i][j]));
                    }
                }
                Message adjacent = mGsmHandler.obtainMessage(GET_ADJACENT);
                mGsmHandler.sendMessage(adjacent);
                break;
            case KEY_BETWEEN_ADJACENT_3G:
                setContentView(R.layout.netinfo_between_adjacent_3g);
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
                        mtextView[i][j] = (TextView) (GsmShowActivity.this
                                .findViewById(viewID[i][j]));
                    }
                }
                Message adjacent_3g = mGsmHandler
                        .obtainMessage(GET_BETWEEN_ADJACENT_3G);
                mGsmHandler.sendMessage(adjacent_3g);
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
                        mtextView[i][j] = (TextView) (GsmShowActivity.this
                                .findViewById(viewID[i][j]));
                    }
                }
                Message adjacent_4g = mGsmHandler
                        .obtainMessage(GET_BETWEEN_ADJACENT_4G);
                mGsmHandler.sendMessage(adjacent_4g);
                break;
            case KEY_OUTFIELD_NETWORK:
                setContentView(R.layout.netinfo_outfield_show);
                listView = (ListView) findViewById(R.id.netinfo_outfield_listview);
                Message outfieldNetwork = mGsmHandler
                        .obtainMessage(GET_OUTFIELD_NETWORK);
                mGsmHandler.sendMessage(outfieldNetwork);
                break;
            }
        } else {
            if (mGsmHandler != null) {
                mGsmHandler.getLooper().quit();
            }
            return;
        }
        mTimer = new Timer();
        mTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                switch (mCheckId) {
                case KEY_SERVING:
                    Message serving = mGsmHandler.obtainMessage(GET_SERVING);
                    mGsmHandler.sendMessage(serving);
                    break;
                case KEY_ADJACENT:
                    Message adjacent = mGsmHandler.obtainMessage(GET_ADJACENT);
                    mGsmHandler.sendMessage(adjacent);
                    break;
                case KEY_BETWEEN_ADJACENT_3G:
                    Message adjacent_3g = mGsmHandler
                            .obtainMessage(GET_BETWEEN_ADJACENT_3G);
                    mGsmHandler.sendMessage(adjacent_3g);
                    break;
                case KEY_BETWEEN_ADJACENT_4G:
                    Message adjacent_4g = mGsmHandler
                            .obtainMessage(GET_BETWEEN_ADJACENT_4G);
                    mGsmHandler.sendMessage(adjacent_4g);
                    break;
                case KEY_OUTFIELD_NETWORK:
                    Message outfieldNetwork = mGsmHandler
                            .obtainMessage(GET_OUTFIELD_NETWORK);
                    mGsmHandler.sendMessage(outfieldNetwork);
                    break;
                }
            }
        }, 0, 200);
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

    @Override
    protected void onDestroy() {
        if (mTimer != null) {
            mTimer.cancel();
        }
        if (mGsmHandler != null) {
            mGsmHandler.getLooper().quit();
        }
        unregisterReceiver();
        super.onDestroy();
    }

    public void TextValuesDisplay(int cmdType) {
        String result = null;
        int temp;
        switch (cmdType) {
        case KEY_ADJACENT:
            result = IATUtils.sendATCmd("AT+SPENGMD=0,0,1", SaveName);
            Log.d(TAG, "AT+SPENGMD=0,0,1: " + result);
            if (result.contains(IATUtils.AT_OK)) {
                result = result.replaceAll(",-", ",+");
                result = result.replaceAll("--", "-+");
                String[] str1 = result.split("\n");
                String[] str2 = str1[0].split("-");
                for (int i = 0; i < str2.length; i++) {
                    if (str2[i].contains(",")) {
                        String[] str3 = str2[i].split(",");
                        for (int j = 0; j < 5; j++) {
                            if (i == 0) {
                                if (j >= str3.length) {
                                    mTextValue[j][1] = "NA";
                                } else {
                                    mTextValue[j][1] = str3[j]
                                            .replace("+", "-");
                                }
                            }
                            if (i == 1) {
                                if (j >= str3.length) {
                                    mTextValue[j][2] = "NA";
                                } else {
                                    mTextValue[j][2] = str3[j]
                                            .replace("+", "-");
                                }
                            }
                            if (i == 2) {
                                if (j >= str3.length) {
                                    mTextValue[j][0] = "NA";
                                } else {
                                    mTextValue[j][0] = str3[j]
                                            .replace("+", "-");
                                }
                            }
                            if (i == 3) {
                                if (j >= str3.length) {
                                    mTextValue[j][3] = "NA";
                                } else {
                                    mTextValue[j][3] = str3[j]
                                            .replace("+", "-");
                                }
                            }
                            if (i == 4) {
                                if (j >= str3.length) {
                                    mTextValue[j][4] = "NA";
                                } else {
                                    temp = Integer.parseInt(str3[j].replace("+", "-").trim()) - 110;
                                    mTextValue[j][4] = String.valueOf(temp) + "dBm";
                                }
                            }
                        }
                    } else {
                        for (int j = 0; j < 5; j++) {
                            if (i == 0) {
                                if (j > 0) {
                                    mTextValue[j][1] = "NA";
                                } else {
                                    mTextValue[j][1] = str2[i]
                                            .replace("+", "-");
                                }
                            }
                            if (i == 1) {
                                if (j > 0) {
                                    mTextValue[j][2] = "NA";
                                } else {
                                    mTextValue[j][2] = str2[i]
                                            .replace("+", "-");
                                }
                            }
                            if (i == 2) {
                                if (j > 0) {
                                    mTextValue[j][0] = "NA";
                                } else {
                                    mTextValue[j][0] = str2[i]
                                            .replace("+", "-");
                                }
                            }
                            if (i == 3) {
                                if (j > 0) {
                                    mTextValue[j][3] = "NA";
                                } else {
                                    mTextValue[j][3] = str2[i]
                                            .replace("+", "-");
                                }
                            }
                            if (i == 4) {
                                if (j > 0) {
                                    mTextValue[j][4] = "NA";
                                } else {
                                    temp = Integer.parseInt(str2[i].replace("+", "-").trim()) - 110;
                                    mTextValue[j][4] = String.valueOf(temp) + "dBm";
                                }
                            }
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
        case KEY_BETWEEN_ADJACENT_3G:
            result = IATUtils.sendATCmd("AT+SPENGMD=0,0,2", SaveName);
            Log.d(TAG, "AT+SPENGMD=0,0,2: " + result);
            if (result.contains(IATUtils.AT_OK)) {
                result = result.replaceAll(",-", ",+");
                result = result.replaceAll("--", "-+");
                String[] str1 = result.split("\n");
                String[] str2 = str1[0].split("-");
                for (int i = 0; i < str2.length; i++) {
                    if (str2[i].contains(",")) {
                        String[] str3 = str2[i].split(",");
                        for (int j = 0; j < 5; j++) {
                            if (i < 3) {
                                if (j >= str3.length) {
                                    if (i == 2) {
                                        mTextValue[j][i] = "NA";
                                    } else {
                                        mTextValue[j][i] = "NA";
                                    }
                                } else {
                                    if (i == 2) {
                                        temp = Integer.parseInt(str3[j].replace("+","-").trim()) - 116;
                                        mTextValue[j][i] = String.valueOf(temp) + "dBm";
                                    } else {
                                        mTextValue[j][i] = str3[j].replace("+",
                                                "-");
                                    }
                                }
                            }
                        }
                    } else {
                        for (int j = 0; j < 5; j++) {
                            if (i < 3) {
                                if (j > 0) {
                                    if (i == 2) {
                                        mTextValue[j][i] = "NA";
                                    } else {
                                        mTextValue[j][i] = "NA";
                                    }
                                } else {
                                    if (i == 2) {
                                        temp = Integer.parseInt(str2[i].replace("+","-").trim()) - 116;
                                        mTextValue[j][i] = String.valueOf(temp) + "dBm";
                                    } else {
                                        mTextValue[j][i] = str2[i].replace("+",
                                                "-");
                                    }
                                }
                            }
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
            result = IATUtils.sendATCmd("AT+SPENGMD=0,0,3", SaveName);
            Log.d(TAG, "AT+SPENGMD=0,0,3: " + result);
            if (result.contains(IATUtils.AT_OK)) {
                result = result.replaceAll(",-", ",+");
                result = result.replaceAll("--", "-+");
                String[] str1 = result.split("\n");
                String[] str2 = str1[0].split("-");
                for (int i = 0; i < 4; i++) {
                    if (str2[i].contains(",")) {
                        String[] str3 = str2[i].split(",");
                        for (int j = 0; j < 5; j++) {
                            if (i < 5) {
                                if (j >= str3.length) {
                                    if (i == 2 || i == 3) {
                                        mTextValue[j][i] = "NA";
                                    } else {
                                        mTextValue[j][i] = "NA";
                                    }
                                } else {
                                    if (i == 2 || i == 3) {
                                        temp = Integer.parseInt(str3[j].replace("+","-").trim())/100;
                                        mTextValue[j][i] = String.valueOf(temp) + "dBm";
                                    } else {
                                        mTextValue[j][i] = str3[j].replace("+",
                                                "-");
                                    }
                                }
                            }
                        }
                    } else {
                        for (int j = 0; j < 5; j++) {
                            if (i < 5) {
                                if (j > 0) {
                                    if (i == 2 || i == 3) {
                                        mTextValue[j][i] = "NA";
                                    } else {
                                        mTextValue[j][i] = "NA";
                                    }
                                } else {
                                    if (i == 2 || i == 3) {
                                        temp = Integer.parseInt(str2[i].replace("+","-").trim())/100;
                                        mTextValue[j][i] = String.valueOf(temp) + "dBm";
                                    } else {
                                        mTextValue[j][i] = str2[i].replace("+",
                                                "-");
                                    }
                                }
                            }
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
            values = this.getResources()
                    .getStringArray(R.array.Gsm_sering_name);
            result = IATUtils.sendATCmd("AT+SPENGMD=0,0,0", SaveName);
            Log.d(TAG, "AT+SPENGMD=0,0,0: " + result);
            if (result.contains(IATUtils.AT_OK)) {
                result = result.replaceAll("--", "-+");
                String[] str1 = result.split("\n");
                String[] str2 = str1[0].split("-");
                for (int i = 0; i < str2.length; i++) {
                    if (i == 0) {
                        values[1] = values[1] + ": "
                                + str2[i].replace("+", "-");
                    }
                    if (i == 1) {
                        values[2] = values[2] + ": "
                                + str2[i].replace("+", "-");
                    }
                    if (i == 2) {
                        values[0] = values[0] + ": "
                                + str2[i].replace("+", "-");
                    }
                    if (i == 3) {
                        values[3] = values[3] + ": "
                                + str2[i].replace("+", "-");
                    }
                    if (i == 4) {
                        temp = Integer.parseInt(str2[i].replace("+", "-").trim()) - 110;
                        values[4] = values[4] + ": "
                                + String.valueOf(temp) + "dBm";
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
                            GsmShowActivity.this, R.layout.array_item, values);
                    listView.setAdapter(adapter);
                }
            });
        } else if (cmdType == KEY_OUTFIELD_NETWORK) {
            values = this.getResources().getStringArray(
                    R.array.Gsm_Outfield_Network);
            int num = 0;
            result = IATUtils.sendATCmd("AT+SPENGMD=0,0,7", SaveName);
            Log.d(TAG, "AT+SPENGMD=0,0,7: " + result);
            if (result.contains(IATUtils.AT_OK)) {
                result = result.replaceAll("--", "-+");
                String[] str1 = result.split("\n");
                String[] str2 = str1[0].split("-");
                for (int i = 0; i < str2.length; i++) {
                    if (i == 0 || i == 1 || i == 2 || i == 3 || i == 6
                            || i == 7 || i == 9) {
                        if (i == 3) {
                            String string = "";
                            int response = Integer
                                    .valueOf(str2[i].substring(0));
                            int[] bite = new int[7];
                            bite[6] = response % 128 / 64;
                            bite[5] = response % 64 / 32;
                            bite[4] = response % 32 / 16;
                            bite[3] = response % 16 / 8;
                            bite[2] = response % 8 / 4;
                            bite[1] = response % 4 / 2;
                            bite[0] = response % 2;
                            for (int j = 0; j < 6; j++) {
                                if (bite[j] == 1) {
                                    string = string + Description_name[3][j]
                                            + " ";
                                }
                            }
                            values[num] = values[num] + ": " + string;
                        } else {
                            values[num] = values[num]
                                    + ": "
                                    + Description_name[i][Integer
                                            .valueOf(str2[i].substring(0, 1))];
                        }
                        num++;
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
                            GsmShowActivity.this, R.layout.array_item, values);
                    listView.setAdapter(adapter);
                }
            });
        }
    }

    class GsmHandler extends Handler {
        public GsmHandler(Looper looper) {
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
            case GET_BETWEEN_ADJACENT_3G:
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
