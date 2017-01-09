/** Created by Spreadst */

package plugin.sprd.supportcmcc;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import android.app.Activity;
import android.content.Context;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.Toast;

public class WifiTrustedAPList extends Activity implements OnClickListener,
        OnItemClickListener {

    private static final String TAG = "WifiTrustedAPList";

    private static final int WIFI_TRUST_LIST_UPDATE = 0;
    private static final int WIFI_TRUST_LIST_MOVED = 1;

    private Button mSave;
    private Button mMoveUp;
    private Button mMoveDown;

    private ListView mList;
    private int mIndex;
    private String[] mSsids;
    private List<TrustedAccessPoint> mTrustedAccessPoints;

    private WifiManager mWifiManager;

    private boolean mUpdate;

    private Handler mUpdateHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.wifi_trusted_list);

        mWifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);

        mList = (ListView) findViewById(R.id.trusted_list);
        mList.setOnItemClickListener(this);

        mSave = (Button) findViewById(R.id.save);
        mSave.setOnClickListener(this);
        mMoveDown = (Button) findViewById(R.id.move_down);
        mMoveDown.setOnClickListener(this);
        mMoveUp = (Button) findViewById(R.id.move_up);
        mMoveUp.setOnClickListener(this);

        mUpdateHandler = new updateTrustedListHandler();
        initializeTrustedList();
    }

    @Override
    protected void onStart() {
        super.onStart();
        updateTrustedList();
    }

    private void initializeTrustedList() {

        final List<WifiConfiguration> SortedConfigs = sortConfigedAPs(mWifiManager
                .getConfiguredNetworks());
        mTrustedAccessPoints = new ArrayList<TrustedAccessPoint>();

        if (SortedConfigs != null) {
            for (WifiConfiguration config : SortedConfigs) {
                if (config.SSID != null) {
                    TrustedAccessPoint mTrustedAp = new TrustedAccessPoint(config.networkId,
                            config.SSID);
                    mTrustedAccessPoints.add(mTrustedAp);
                }
            }
        }
    }

    private List<WifiConfiguration> sortConfigedAPs(List<WifiConfiguration> mconfig) {
        // mconfig is null when SCAN_ONLY_WITH_WIFI_OFF_MODE
        if (mconfig == null) {
            return null;
        }
        /* ascending sort by priority */
        Collections.sort(mconfig, new Comparator<WifiConfiguration>() {
            public int compare(WifiConfiguration a, WifiConfiguration b) {
                if (a.priority > b.priority) {
                    return -1;
                } else if (a.priority < b.priority) {
                    return 1;
                } else {
                    return -1;
                }
            }
        });
        return mconfig;
    }

    /**
     * Send sticky message
     * 
     * @param action
     */
    private void sendActionMessage(String action) {
        int msgWhat;
        Bundle data = null;
        if (action != null) {
            msgWhat = WIFI_TRUST_LIST_MOVED;
            data = new Bundle();
            data.putString("MOVE_ACTION", action);
        } else {
            msgWhat = WIFI_TRUST_LIST_UPDATE;
        }
        mUpdateHandler.removeMessages(msgWhat);
        Message msg = Message.obtain(mUpdateHandler, msgWhat);
        if (data != null) {
            msg.setData(data);
        }
        msg.sendToTarget();
    }

    private void updateButton() {
        if (mIndex == 0) {
            mMoveUp.setEnabled(false);
        } else {
            mMoveUp.setEnabled(true);
        }
        if (mIndex == mTrustedAccessPoints.size() - 1) {
            mMoveDown.setEnabled(false);
        } else {
            mMoveDown.setEnabled(true);
        }
        mSave.setEnabled(mUpdate);
    }

    private void setButtonEnable(boolean enable) {
        mMoveUp.setEnabled(enable);
        mMoveDown.setEnabled(enable);
    }

    private class updateTrustedListHandler extends Handler {

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case WIFI_TRUST_LIST_MOVED:
                    Bundle data = msg.getData();
                    String moveAction = data.getString("MOVE_ACTION");
                    setButtonEnable(false);
                    // exchange priority
                    TrustedAccessPoint mCurAp = mTrustedAccessPoints.get(mIndex);
                    if (moveAction.equals("UP") && mIndex > 0) {
                        mTrustedAccessPoints.set(mIndex, mTrustedAccessPoints.get(mIndex - 1));
                        mIndex = mIndex - 1;
                        mTrustedAccessPoints.set(mIndex, mCurAp);
                    } else if (moveAction.equals("DOWN")
                            && mIndex < mTrustedAccessPoints.size() - 1) {
                        mTrustedAccessPoints.set(mIndex, mTrustedAccessPoints.get(mIndex + 1));
                        mIndex = mIndex + 1;
                        mTrustedAccessPoints.set(mIndex, mCurAp);
                    }

                    mUpdate = true;
                    updateTrustedList();
                    break;

                case WIFI_TRUST_LIST_UPDATE:
                    int i = mTrustedAccessPoints.size();
                    for (TrustedAccessPoint ap : mTrustedAccessPoints) {
                        WifiConfiguration config = new WifiConfiguration();
                        config.networkId = ap.networkId;
                        config.priority = i;
                        mWifiManager.save(config, null);
                        i--;
                    }
                    mUpdate = false;
                    finish();
                    break;

                default:
                    // TODO:do other things
                    break;
            }
        }
    }

    private void updateTrustedList() {

        int size = mTrustedAccessPoints.size();
        if (size == 0) {
            Toast.makeText(this, R.string.trusted_ap_list_null, Toast.LENGTH_SHORT).show();
            finish();
        }
        mSsids = new String[size];
        int i = 0;
        for (TrustedAccessPoint ap : mTrustedAccessPoints) {
            mSsids[i] = removeDoubleQuotes(ap.ssid);
            i++;
        }
        ArrayAdapter<?> mArrayAdapter = new ArrayAdapter<Object>(this,
                android.R.layout.simple_list_item_single_choice,
                android.R.id.text1, mSsids);
        mList.setAdapter(mArrayAdapter);
        mList.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
        mList.setItemChecked(mIndex, true);
        mList.setSelection(mIndex);
        updateButton();
    }

    private String removeDoubleQuotes(String string) {
        int length = string.length();
        if ((length > 1) && (string.charAt(0) == '"')
                && (string.charAt(length - 1) == '"')) {
            return string.substring(1, length - 1);
        }
        return string;
    }

    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        mIndex = position;
        updateButton();
    }

    public void onClick(View v) {
        if (v == mMoveUp && mIndex > 0) {
            sendActionMessage("UP");
        } else if (v == mMoveDown && mIndex < mTrustedAccessPoints.size() - 1) {
            sendActionMessage("DOWN");
        } else if (v == mSave) {
            sendActionMessage(null);
        }
    }

    private class TrustedAccessPoint {
        public int networkId;
        public String ssid;

        public TrustedAccessPoint(int _networkId, String _ssid) {
            networkId = _networkId;
            ssid = _ssid;
        }
    }
}
