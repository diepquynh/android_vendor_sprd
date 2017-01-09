/** Created by Spreadst */

package plugin.sprd.supportcmcc;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Debug;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings.Global;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ListView;
import android.widget.ListAdapter;
import android.widget.TextView;

public class WifiConnectionPolicyDialogActivity extends Activity implements
        OnCheckedChangeListener, OnItemClickListener {

    private static final String TAG = "WifiConnectionPolicyDialogActivity";
    private static final boolean DBG = true;
    private static final int INVALID_VALUE = -1;
    private static final int MESSAGE_DIALOG_TIME_OUT = 0;
    private static final int MESSAGE_DIALOG_TIME_OUT_VALUE = 5 * 1000;

    private TextView mMessage;
    private ListView mListView;
    private CheckBox mCheckBox;
    private Button mOK;
    private Button mCancel;

    private Handler mHandler;

    private WifiManager mWifiManager = null;
    private static TelephonyManager mTelephonyManager = null;

    private int mDialogType = INVALID_VALUE;
    private String mSsidName = null;
    private int mSsidNetworkId = INVALID_VALUE;
    private String[] mSsidNameArrary = null;
    private int[] mSsidNetworkIdArrary = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.wifi_connection_policy_dialog);

        getWindow().setCloseOnTouchOutside(false);

        mMessage = (TextView) findViewById(R.id.message);
        mListView = (ListView) findViewById(R.id.list);
        mListView.setOnItemClickListener(this);
        mCheckBox = (CheckBox) findViewById(R.id.do_not_prompt);
        mCheckBox.setOnCheckedChangeListener(this);

        mOK = (Button) findViewById(R.id.yes);
        mOK.setOnClickListener(okListener);
        mCancel = (Button) findViewById(R.id.no);
        mCancel.setOnClickListener(cancelListener);

        mWifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        mTelephonyManager = TelephonyManager.from(this);

        mHandler = new UpdateHandler();

        Intent getIntent = getIntent();
        if (getIntent.getExtras().containsKey(
                WifiConnectionPolicy.INTENT_EXTRA_DIALOG_TYPE)) {
            int dialogType = getIntent.getExtras().getInt(
                    WifiConnectionPolicy.INTENT_EXTRA_DIALOG_TYPE,
                    INVALID_VALUE);
            logd("onCreate dialogType is  " + dialogType);
            if (dialogType != INVALID_VALUE) {
                switch (dialogType) {
                    case WifiConnectionPolicy.DIALOG_TYPE_MOBILE_TO_WLAN_ALWAYS_ASK:
                        mSsidName = getIntent
                                .getStringExtra(WifiConnectionPolicy.INTENT_EXTRA_SSID_NAME);
                        mSsidNetworkId = getIntent.getIntExtra(
                                WifiConnectionPolicy.INTENT_EXTRA_SSID_ID,
                                INVALID_VALUE);
                        logd("mSsidName is " + mSsidName + ", mSsidNetworkId is "
                                + mSsidNetworkId);
                        break;
                    case WifiConnectionPolicy.DIALOG_TYPE_WLAN_TO_WLAN:
                    case WifiConnectionPolicy.DIALOG_TYPE_MOBILE_TO_WLAN_MANUAL:
                        mSsidNameArrary = getIntent
                                .getStringArrayExtra(WifiConnectionPolicy.INTENT_EXTRA_SSIDS_NAME);
                        mSsidNetworkIdArrary = getIntent
                                .getIntArrayExtra(WifiConnectionPolicy.INTENT_EXTRA_SSIDS_ID);
                        break;
                    case WifiConnectionPolicy.DIALOG_TYPE_WLAN_TO_MOBILE:
                        mTelephonyManager.setDataEnabled(false);
                        break;
                }
                updateDialogView(dialogType);
                mDialogType = dialogType;
            } else {
                finish();
            }
        }
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        if (mDialogType == WifiConnectionPolicy.DIALOG_TYPE_CONNECT_TO_CMCC) {
            Global.putInt(getContentResolver(),
                    WifiConnectionPolicy.DIALOG_CONNECT_TO_CMCC, buttonView.isChecked() ? 0 : 1);
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position,
            long id) {
        logd( "position is " + position + ", name is " + mSsidNameArrary[position]
                + " , networkId is " + mSsidNetworkIdArrary[position]);
        if (mSsidNetworkIdArrary != null) {
            mSsidNetworkId = mSsidNetworkIdArrary[position];
            mListView.setItemChecked(position, true);
        }
    }

    View.OnClickListener okListener = new View.OnClickListener() {

        @Override
        public void onClick(View arg0) {
            logd( "okListener mDialogType is " + mDialogType);
            switch (mDialogType) {
                case WifiConnectionPolicy.DIALOG_TYPE_WLAN_TO_WLAN:
                    if (mSsidNetworkId != INVALID_VALUE) {
                        mWifiManager.connect(mSsidNetworkId, null);
                        WifiConnectionPolicy.setManulConnectFlags(true);
                    }
                    if (mCheckBox.isChecked()) {
                        Global.putInt(getContentResolver(),
                                WifiConnectionPolicy.DIALOG_WLAN_TO_WLAN,
                                WifiConnectionPolicy.YES_AND_REMEMBERED);
                    }
                    WifiConnectionPolicy.setDialogShowing(false);
                    break;
                case WifiConnectionPolicy.DIALOG_TYPE_MOBILE_TO_WLAN_MANUAL:
                    if (mSsidNetworkId != INVALID_VALUE) {
                        mWifiManager.connect(mSsidNetworkId, null);
                        WifiConnectionPolicy.setManulConnectFlags(true);
                    }
                    if (mCheckBox.isChecked()) {
                        Global.putInt(getContentResolver(),
                                WifiConnectionPolicy.DIALOG_MOBILE_TO_WLAN_MANUAL,
                                WifiConnectionPolicy.YES_AND_REMEMBERED);
                    }
                    WifiConnectionPolicy.setDialogShowing(false);
                    break;
                case WifiConnectionPolicy.DIALOG_TYPE_MOBILE_TO_WLAN_AUTO:
                    // do nothings
                    break;
                case WifiConnectionPolicy.DIALOG_TYPE_MOBILE_TO_WLAN_ALWAYS_ASK:
                    if (mSsidNetworkId != INVALID_VALUE) {
                        mWifiManager.connect(mSsidNetworkId, null);
                        WifiConnectionPolicy.setManulConnectFlags(true);
                    }
                    if (mCheckBox.isChecked()) {
                        Global.putInt(getContentResolver(),
                                WifiConnectionPolicy.DIALOG_MOBILE_TO_WLAN_ALWAYS_ASK,
                                WifiConnectionPolicy.YES_AND_REMEMBERED);
                    }
                    WifiConnectionPolicy.setDialogShowing(false);
                    break;
                case WifiConnectionPolicy.DIALOG_TYPE_WLAN_TO_MOBILE:
                    if (mCheckBox.isChecked()) {
                        Global.putInt(getContentResolver(),
                                WifiConnectionPolicy.DIALOG_WLAN_TO_MOBILE,
                                WifiConnectionPolicy.YES_AND_REMEMBERED);
                    }
                    mTelephonyManager.setDataEnabled(true);
                    WifiConnectionPolicy.setDialogShowing(false);
                    break;
                case WifiConnectionPolicy.DIALOG_TYPE_CONNECT_TO_CMCC:
                    // do nothings
                    break;
            }
            finish();
        }
    };

    View.OnClickListener cancelListener = new View.OnClickListener() {

        @Override
        public void onClick(View arg0) {
            logd( "cancelListener mDialogType is " + mDialogType);
            switch (mDialogType) {
                case WifiConnectionPolicy.DIALOG_TYPE_WLAN_TO_WLAN:
                    if (mCheckBox.isChecked()) {
                        Global.putInt(getContentResolver(),
                                WifiConnectionPolicy.DIALOG_WLAN_TO_WLAN,
                                WifiConnectionPolicy.NO_AND_REMEMBERED);
                    }
                    // WifiConnectionPolicy.setTimer(System.currentTimeMillis());
                    WifiConnectionPolicy.setWlanToWLanDialogCancleFlag(true);
                    WifiConnectionPolicy.setDialogShowing(false);
                    break;
                case WifiConnectionPolicy.DIALOG_TYPE_MOBILE_TO_WLAN_MANUAL:
                    if (mCheckBox.isChecked()) {
                        Global.putInt(getContentResolver(),
                                WifiConnectionPolicy.DIALOG_MOBILE_TO_WLAN_MANUAL,
                                WifiConnectionPolicy.NO_AND_REMEMBERED);
                    }
                    WifiConnectionPolicy.setTimer(System.currentTimeMillis());
                    // WifiConnectionPolicy.setManualDialogCancleFlag(true);
                    WifiConnectionPolicy.setDialogShowing(false);
                    break;
                case WifiConnectionPolicy.DIALOG_TYPE_MOBILE_TO_WLAN_AUTO:
                    // do nothings
                    break;
                case WifiConnectionPolicy.DIALOG_TYPE_MOBILE_TO_WLAN_ALWAYS_ASK:
                    if (mCheckBox.isChecked()) {
                        Global.putInt(getContentResolver(),
                                WifiConnectionPolicy.DIALOG_MOBILE_TO_WLAN_ALWAYS_ASK,
                                WifiConnectionPolicy.NO_AND_REMEMBERED);
                    }
                    WifiConnectionPolicy.setTimer(System.currentTimeMillis());
                    WifiConnectionPolicy.setDialogShowing(false);
                    break;
                case WifiConnectionPolicy.DIALOG_TYPE_WLAN_TO_MOBILE:
                    if (mCheckBox.isChecked()) {
                        Global.putInt(getContentResolver(),
                                WifiConnectionPolicy.DIALOG_WLAN_TO_MOBILE,
                                WifiConnectionPolicy.NO_AND_REMEMBERED);
                    }
                    // mConnectivityManager.setMobileDataEnabled(false);
                    WifiConnectionPolicy.setDialogShowing(false);
                    break;
                case WifiConnectionPolicy.DIALOG_TYPE_CONNECT_TO_CMCC:
                    // do nothings
                    break;
            }
            finish();
        }
    };

    private class UpdateHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MESSAGE_DIALOG_TIME_OUT:
                    finish();
                    break;
                default:
                    break;
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        logd("onResume");
        setDialogDisplayStatus(true);
    }

    @Override
    protected void onPause() {
        super.onPause();
        logd("onPause");
        setDialogDisplayStatus(false);
    }

    private void updateDialogView(int dialogType) {
        switch (dialogType) {
            case WifiConnectionPolicy.DIALOG_TYPE_WLAN_TO_WLAN:
                // for wlanToWlan case
                setTitle(R.string.weak_wifi_signal_title);
                mMessage.setText(R.string.weak_wifi_signal_message);
                mListView.setVisibility(View.VISIBLE);
                if (mSsidNameArrary != null && mSsidNameArrary.length > 0
                        && mSsidNetworkIdArrary != null && mSsidNetworkIdArrary.length > 0
                        && mSsidNameArrary.length == mSsidNetworkIdArrary.length) {
                    ArrayAdapter<?> mArrayAdapter = new ArrayAdapter<Object>(this,
                            android.R.layout.simple_list_item_checked,
                            android.R.id.text1, mSsidNameArrary);
                    mListView.setAdapter(mArrayAdapter);
                    setListViewHight(mListView);
                    mListView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
                    mListView.setItemChecked(0, true);
                    mSsidNetworkId = mSsidNetworkIdArrary[0];
                }
                break;
            case WifiConnectionPolicy.DIALOG_TYPE_MOBILE_TO_WLAN_MANUAL:
                // for cmcc case 4.6 and case 4.8
                setTitle(R.string.select_trusted_ap_access);
                mMessage.setVisibility(View.GONE);
                mListView.setVisibility(View.VISIBLE);
                if (mSsidNameArrary != null && mSsidNameArrary.length > 0
                        && mSsidNetworkIdArrary != null && mSsidNetworkIdArrary.length > 0
                        && mSsidNameArrary.length == mSsidNetworkIdArrary.length) {
                    ArrayAdapter<?> mArrayAdapter = new ArrayAdapter<Object>(this,
                            android.R.layout.simple_list_item_checked,
                            android.R.id.text1, mSsidNameArrary);
                    mListView.setAdapter(mArrayAdapter);
                    setListViewHight(mListView);
                    mListView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
                    mListView.setItemChecked(0, true);
                    mSsidNetworkId = mSsidNetworkIdArrary[0];
                }
                break;
            case WifiConnectionPolicy.DIALOG_TYPE_MOBILE_TO_WLAN_AUTO:
                // do nothings
                break;
            case WifiConnectionPolicy.DIALOG_TYPE_MOBILE_TO_WLAN_ALWAYS_ASK:
                // for cmcc case 4.9
                setTitle(R.string.network_disconnect_title);
                if (!TextUtils.isEmpty(mSsidName)) {
                    mMessage.setText(String.format(this.getString(R.string.mobile_to_wlan_popup), mSsidName));
                }
                break;
            case WifiConnectionPolicy.DIALOG_TYPE_WLAN_TO_MOBILE:
                // for cmcc case 4.10
                setTitle(R.string.network_disconnect_title);
                mMessage.setText(R.string.network_disconnect_message);
                break;
            case WifiConnectionPolicy.DIALOG_TYPE_CONNECT_TO_CMCC:
                // for cmcc case
                setTitle(R.string.network_disconnect_title);
                mOK.setVisibility(View.GONE);
                mCancel.setVisibility(View.GONE);
                mMessage.setText(R.string.connect_to_cmcc_ap_message);
                mHandler.sendEmptyMessageDelayed(MESSAGE_DIALOG_TIME_OUT,
                        MESSAGE_DIALOG_TIME_OUT_VALUE);
                break;
        }
    }

    private void setDialogDisplayStatus(boolean show) {
        logd( "setDialogDisplayStatus mDialogType is " + mDialogType + ", show is " + show);
        switch (mDialogType) {
        case WifiConnectionPolicy.DIALOG_TYPE_WLAN_TO_WLAN:
        case WifiConnectionPolicy.DIALOG_TYPE_MOBILE_TO_WLAN_MANUAL:
        case WifiConnectionPolicy.DIALOG_TYPE_MOBILE_TO_WLAN_ALWAYS_ASK:
        case WifiConnectionPolicy.DIALOG_TYPE_WLAN_TO_MOBILE:
            WifiConnectionPolicy.setDialogShowing(show);
            break;
        case WifiConnectionPolicy.DIALOG_TYPE_MOBILE_TO_WLAN_AUTO:
            // do nothings
            break;
        case WifiConnectionPolicy.DIALOG_TYPE_CONNECT_TO_CMCC:
            // do nothings
            break;
        }
    }

    private void setListViewHight(ListView listView) {
        ListAdapter listAdapter = listView.getAdapter();
        if (listAdapter == null) {
            return;
        }

        View listItem = listAdapter.getView(0, null, listView);
        listItem.measure(0, 0);
        int listItemHeight = listItem.getMeasuredHeight() + listView.getDividerHeight();
        int listItemCount = listAdapter.getCount();

        ViewGroup.LayoutParams params = listView.getLayoutParams();
        params.height = listItemHeight * (listItemCount > 3 ? 3:listItemCount);
        listView.setLayoutParams(params);
    }

    private void logd(String logString) {
        if (DBG) {
            Log.d(TAG, logString);
        }
    }
}
