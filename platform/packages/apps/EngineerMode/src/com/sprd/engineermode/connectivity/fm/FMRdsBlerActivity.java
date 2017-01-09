package com.sprd.engineermode.connectivity.fm;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;
import android.widget.EditText;
import android.widget.Button;
import android.widget.Toast;
import android.os.Handler;
import android.os.Message;
import android.content.Context;
import android.util.Log;

import com.android.fmradio.FmManagerSelect;
import com.android.fmradio.FmNative;
import com.sprd.engineermode.R;

public class FMRdsBlerActivity extends Activity{
    private static final String TAG = "FMRdsBlerActivity";

    private TextView mRdsBler;
    private Context mContext;
    private FmManagerSelect mFmManager = null;

    private static final int GET_BLER = 1;
    private static final int TIME=200;
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == GET_BLER) {
                Log.d(TAG,"handleMessage");
                mRdsBler.setText(getResources().getString(R.string.bler)+":"+mFmManager.getFmBler());
                Message m = obtainMessage(GET_BLER);
                sendMessageDelayed(m, TIME);
            }
        }
    };
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.fm_rds_bler);
        mContext=this;

        mFmManager = new FmManagerSelect(mContext);
        mRdsBler=(TextView)findViewById(R.id.rds_bler);
        mHandler.sendEmptyMessage(GET_BLER);
    }
    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mHandler != null) {
            mHandler.removeMessages(GET_BLER);
            Log.d(TAG, "HandlerThread has quit");
        }
    }
}
