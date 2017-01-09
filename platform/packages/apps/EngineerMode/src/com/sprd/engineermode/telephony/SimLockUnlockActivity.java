
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

public class SimLockUnlockActivity extends Activity {
    private static final String TAG = "SimLockUnlockActivity";
    private static final int MSG_UNLOCK = 0;
    private static final int MSG_UNLOCK_DONE = 0;
    private static final int MSG_UNLOCK_FAILED = 1;

    private String mSimLockType = null;
    private EditText mEtPsw = null;
    private Handler mSimlockHandler = null;

    private Handler mUiHandler = new Handler() {

        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_UNLOCK_DONE:
                    Toast.makeText(SimLockUnlockActivity.this, "Unlocked", Toast.LENGTH_SHORT)
                            .show();
                    finish();
                    break;
                case MSG_UNLOCK_FAILED:
                    Toast.makeText(SimLockUnlockActivity.this, "Failed", Toast.LENGTH_SHORT)
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
                case MSG_UNLOCK:
                    SimLock simLock = null;
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
                        mUiHandler.sendEmptyMessage(MSG_UNLOCK_FAILED);
                        Log.e(TAG, "SimLock Init FAIL!!!");
                        return;
                    }

                    String psw = mEtPsw.getText().toString();

                    if (!simLock.lock(psw)) {
                        mUiHandler.sendEmptyMessage(MSG_UNLOCK_FAILED);
                        Log.e(TAG, "SimLock Lock FAIL!!!");
                        return;
                    }
                    mUiHandler.sendEmptyMessage(MSG_UNLOCK_DONE);
                    break;
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_simlock_unlock);
        mSimLockType = this.getIntent().getStringExtra(SimLockOpActivity.KEY_SIMLOCK_TYPE);
        mEtPsw = (EditText) findViewById(R.id.et_psw);

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mSimlockHandler = new SimLockHandler(ht.getLooper());

        Button cancelBtn = (Button) findViewById(R.id.btn_cancel);
        cancelBtn.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View arg0) {
                finish();
            }

        });
        Button unLockBtn = (Button) findViewById(R.id.btn_unlock);
        unLockBtn.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View arg0) {
                mSimlockHandler.sendEmptyMessage(MSG_UNLOCK);
            }

        });
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
