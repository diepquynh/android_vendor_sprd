
package com.sprd.engineermode.telephony;

import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;

import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.telephony.SimLockOpActivity.SimLockHandler;
import com.sprd.engineermode.utils.IATUtils;

import android.app.Activity;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

public class SimLockLockActivity extends Activity implements OnItemSelectedListener {

    private static final String TAG = "SimLockLockActivity";
    private static final int POSITION_HAND_INPUT = 0;
    private static final int POSITION_READ_FROM_SIM = 1;
    private static final int MSG_LOCK = 0;
    private static final int MSG_LOCK_DONE = 0;
    private static final int MSG_LOCK_FAILED = 1;

    private Spinner mSpInputType = null;
    private EditText mEtIMSI = null;
    private EditText mEtGid1 = null;
    private EditText mEtGid2 = null;
    private EditText mEtPsw = null;
    private String mCurrOpType = null;
    private String mSimLockType = null;

    private SimLockHandler mSimlockHandler;
    private Handler mUiHandler = new Handler() {

        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_LOCK_DONE:
                    Toast.makeText(SimLockLockActivity.this, "Locked", Toast.LENGTH_SHORT)
                            .show();
                    finish();
                    break;
                case MSG_LOCK_FAILED:
                    Toast.makeText(SimLockLockActivity.this, "Failed", Toast.LENGTH_SHORT)
                            .show();
                    break;
            }
        }
    };

    class SimLockHandler extends Handler {

        public SimLockHandler(Looper looper) {
            super(looper);
        }

        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_LOCK:
                    SimLock simLock = null;
                    Log.d(TAG, "MSG_LOCK");
                    if (0 == mSimLockType.compareTo(SimLockOpActivity.KEY_SIMLOCK_NETWORK)) {
                        simLock = new SimLock(SimLock.TYPE_NETWORK);
                    } else if (0 == mSimLockType
                            .compareTo(SimLockOpActivity.KEY_SIMLOCK_NETWORK_SUBSET)) {
                        simLock = new SimLock(SimLock.TYPE_NETWORK_SUBSET);
                    } else if (0 == mSimLockType.compareTo(SimLockOpActivity.KEY_SIMLOCK_SERVICE)) {
                        simLock = new SimLock(SimLock.TYPE_SERVICE);
                    } else if (0 == mSimLockType.compareTo(SimLockOpActivity.KEY_SIMLOCK_CORPORATE)) {
                        simLock = new SimLock(SimLock.TYPE_CORPORATE);
                    } else if (0 == mSimLockType.compareTo(SimLockOpActivity.KEY_SIMLOCK_SIM)) {
                        simLock = new SimLock(SimLock.TYPE_SIM);
                    } else {
                        mUiHandler.sendEmptyMessage(MSG_LOCK_FAILED);
                        Log.e(TAG, "SimLock Init FAIL!!!");
                        return;
                    }

                    if (0 == mCurrOpType.compareTo(SimLockOpActivity.KEY_ADD)
                            || 0 == mCurrOpType.compareTo(SimLockOpActivity.KEY_LOCK)) {
                        String psw = mEtPsw.getText().toString();
                        if (0 == mCurrOpType.compareTo(SimLockOpActivity.KEY_ADD)) {
                            String imsi = null;
                            if (mEtIMSI.getVisibility() == View.VISIBLE) {
                                imsi = mEtIMSI.getText().toString();
                            }

                            String gid1 = "";
                            if (mEtGid1.getVisibility() == View.VISIBLE) {
                                gid1 = mEtGid1.getText().toString();
                            }
                            String gid2 = "";
                            if (mEtGid2.getVisibility() == View.VISIBLE) {
                                gid2 = mEtGid2.getText().toString();
                            }

                            if (!simLock.add(imsi, gid1, gid2, psw)) {
                                mUiHandler.sendEmptyMessage(MSG_LOCK_FAILED);
                                Log.e(TAG, "SimLock Add FAIL!!!");
                                return;
                            }
                        }

                        if (!simLock.lock(psw)) {
                            mUiHandler.sendEmptyMessage(MSG_LOCK_FAILED);
                            Log.e(TAG, "SimLock Lock FAIL!!!");
                            return;
                        }
                        mUiHandler.sendEmptyMessage(MSG_LOCK_DONE);
                    } else {
                        mUiHandler.sendEmptyMessage(MSG_LOCK_FAILED);
                        Log.e(TAG, "SimLock OpType Error");
                        return;
                    }

                    // String mATResponse = IATUtils.sendATCmd("AT+CLCK?",
                    // "atchannel0");
                    // Log.d(TAG, mATResponse);
                    // mATResponse = IATUtils.sendATCmd("AT+CLCK=?",
                    // "atchannel0");
                    // Log.d(TAG, mATResponse);
                    // mATResponse = IATUtils.sendATCmd("AT+CLCK=" + "\"PS\"" +
                    // ",1," + "\"1234\"",
                    // "atchannel0");
                    // Log.d(TAG, mATResponse);
                    // mATResponse = IATUtils.sendATCmd("AT+CLCK=" + "\"PS\"" +
                    // ",2," + "\"1234\"",
                    // "atchannel0");
                    // Log.d(TAG, mATResponse);
                    // mATResponse = IATUtils.sendATCmd("AT+CLCK=" + "\"PS\"" +
                    // ",0," + "\"1234\"",
                    // "atchannel0");
                    // Log.d(TAG, mATResponse);
                    // mATResponse = IATUtils.sendATCmd("AT+CLCK=" + "\"PS\"" +
                    // ",2," + "\"1234\"",
                    // "atchannel0");
                    // Log.d(TAG, mATResponse);
                    // mATResponse = IATUtils.sendATCmd("AT+SPACTCARD=0",
                    // "atchannel0");
                    // Log.d(TAG, mATResponse);
                    // mATResponse =
                    // IATUtils.sendATCmd("AT+CRSM=176,28478,0,0,0",
                    // "atchannel0");
                    // Log.d(TAG, mATResponse);
                    // mATResponse =
                    // IATUtils.sendATCmd("AT+CRSM=176,28479,0,0,0",
                    // "atchannel0");
                    // Log.d(TAG, mATResponse);
                    // mATResponse = IATUtils.sendATCmd("AT+SPSMNW=1,123456,2",
                    // "atchannel0");
                    // Log.d(TAG, mATResponse);
                    // mATResponse = IATUtils.sendATCmd("AT+COPS=?",
                    // "atchannel0");
                    // Log.d(TAG, mATResponse);
                    // mATResponse = IATUtils.sendATCmd("AT*SPENDI?",
                    // "atchannel0");
                    // Log.d(TAG, mATResponse);
                    // mATResponse = IATUtils.sendATCmd("AT+CSCB?",
                    // "atchannel0");
                    // Log.d(TAG, mATResponse);
                    // mUiHandler.sendEmptyMessage(msg.what);
                    break;
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_simlock_lock);
        mCurrOpType = this.getIntent().getStringExtra(SimLockOpActivity.KEY_ADD_LOCK);
        mSimLockType = this.getIntent().getStringExtra(SimLockOpActivity.KEY_SIMLOCK_TYPE);
        initViews(mCurrOpType);

        mSpInputType = (Spinner) findViewById(R.id.sp_imsi_input);
        mSpInputType.setOnItemSelectedListener(this);
        mEtIMSI = (EditText) findViewById(R.id.imsi_nums);
        mEtGid1 = (EditText) findViewById(R.id.et_gid1);
        mEtGid2 = (EditText) findViewById(R.id.et_gid2);
        mEtPsw = (EditText) findViewById(R.id.et_psw);

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mSimlockHandler = new SimLockHandler(ht.getLooper());

        Button cancelBtn = (Button) findViewById(R.id.simlockadd_btn_cancel);
        cancelBtn.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View arg0) {
                finish();
            }

        });
        Button lockBtn = (Button) findViewById(R.id.simlockadd_btn_lock);
        lockBtn.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View arg0) {
                Log.d(TAG, "onClick MSG_LOCK");
                mSimlockHandler.sendEmptyMessage(MSG_LOCK);
            }

        });
    }

    private void initViews(String opType) {
        if(opType == null){
            return;
        }
        if (0 == opType.compareTo(SimLockOpActivity.KEY_ADD)) {
            TextView imsiInputNotice = (TextView) findViewById(R.id.imsi_input_notice);
            imsiInputNotice.setVisibility(View.VISIBLE);
            LinearLayout imsiInput = (LinearLayout) findViewById(R.id.imsi_input);
            imsiInput.setVisibility(View.VISIBLE);

            if (mSimLockType == SimLockOpActivity.KEY_SIMLOCK_NETWORK_SUBSET) {

            } else if (mSimLockType == SimLockOpActivity.KEY_SIMLOCK_NETWORK_SUBSET) {

            }
        }
    }

    @Override
    public void onItemSelected(AdapterView<?> arg0, View arg1, int position, long arg3) {
        if (position == POSITION_HAND_INPUT) {
            mEtIMSI.setVisibility(View.VISIBLE);
            mEtIMSI.requestFocus();
        } else if (position == POSITION_READ_FROM_SIM) {
            mEtIMSI.setVisibility(View.GONE);
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> arg0) {
    }

    @Override
    protected void onDestroy() {
        if (mSimlockHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mSimlockHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        finish();
        super.onBackPressed();
    }

}
