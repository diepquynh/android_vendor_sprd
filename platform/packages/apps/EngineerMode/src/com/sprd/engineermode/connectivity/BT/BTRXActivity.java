package com.sprd.engineermode.connectivity.BT;

import java.util.ArrayList;

import android.app.Activity;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.view.WindowManager;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.EditText;
import android.widget.Button;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.AdapterView.OnItemSelectedListener;
import android.view.LayoutInflater;   
import android.app.AlertDialog;   
import android.text.Editable;  
import android.text.TextWatcher; 
import android.view.View;
import android.util.Log;
import android.widget.Toast;

import java.text.NumberFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

import com.sprd.engineermode.R;
import com.sprd.engineermode.utils.SocketUtils;

public class BTRXActivity extends Activity implements OnClickListener,OnItemSelectedListener {
	
	private static final String TAG = "BTRXActivity";
	
	//cmd message
	private static final int MSG_BT_RX_START = 0;
	private static final int MSG_BT_RX_READ = 1;
	private static final int MSG_BT_RX_STOP = 2;
	private static final int MSG_BT_OFF = 3;
	
	//uimessage
	private static final int RESULT_REFRESH = 0;
	
	private EditText mChannel;
	private Spinner mPacType;
	private EditText mGain;
	private EditText mAddr;
	private ListView mResult;
	//private EditText mResult;
	private Button mStart;
	private Button mRead;
	private Button mAuto;
	private Button mClear;
	private Button mStop;
	
	private BTHelper.BTRX mBTRx;
    private BTRXHandler mBTRXHandler;
    private String mRxResult = null;
    private int mRefreshTime;
    private SimpleDateFormat mFormatter;
    
    private String mCurTime;
    private String mCurRssi;
    private String mCurPer;
    private String mCurBer;
    private String mCurPktCnt;
    private String mCurPktErrCnt;
    private String mCurBitCnt;
    private String mCurBitErrCnt;
    private boolean isNewPer = false;
    private boolean isNewBer = false;
    
    private ArrayList<BTRXResult> mBTResult;  
    private BTRXResultAdapter mResultAdapter;
    private BTRXResult mSingleRes;
    
    private Handler mUiThread = new Handler() {
        public void handleMessage(android.os.Message msg) {
            if (msg == null) {
                Log.d(TAG,"UI Message is null");
                return;
            }
            switch (msg.what) {
                case RESULT_REFRESH:                  
                    mResultAdapter.result.add((BTRXResult)msg.obj);
                    mResultAdapter.notifyDataSetChanged();
                    break;
                default:
                    break;
            }
        }
    };
	
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.bt_rx);
        getWindow().setSoftInputMode(
                WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        initUI();
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mBTRx = new BTHelper.BTRX();
        mBTRXHandler = new BTRXHandler(ht.getLooper());  
        mBTResult = new ArrayList<BTRXResult>();
        mResultAdapter = new BTRXResultAdapter(this,mBTResult);
        mResult.setAdapter(mResultAdapter);
        mFormatter = new SimpleDateFormat ("hh:mm:ss");
    }
    
    
    @Override
    public void onBackPressed() {
        Message doMessage = null;
        //if BT RX is not stop, app shoule stop when click back button
        if (mStop.isEnabled()) {
            Log.d(TAG,"Stop is Enabled when click back");
            doMessage = mBTRXHandler.obtainMessage(MSG_BT_RX_STOP);
            mBTRXHandler.sendMessage(doMessage);
        }
        
        //close BT when click back button
        if (BTHelper.isBTOn) {
            Log.d(TAG,"BT is On when click back");
            doMessage = mBTRXHandler.obtainMessage(MSG_BT_OFF);
            mBTRXHandler.sendMessage(doMessage);
        } else {
            super.onBackPressed();
        }
    }
    
    @Override
    public void onClick(View v) {
        Message doMessage = null;
        if (v == mStart) {
            if (!getSettingParam()) {
                return;
            }
            doMessage = mBTRXHandler.obtainMessage(MSG_BT_RX_START);
            mBTRXHandler.sendMessage(doMessage);
        } else if (v == mRead) {
            disableSettingPara();
            mUiThread.removeCallbacks(mRefreshHandler);
            doMessage = mBTRXHandler.obtainMessage(MSG_BT_RX_READ);
            mBTRXHandler.sendMessage(doMessage);
        } else if (v == mAuto) {
            disableSettingPara();
            LayoutInflater inflater = (LayoutInflater) BTRXActivity.this.getSystemService(LAYOUT_INFLATER_SERVICE);   
            final View view = inflater.inflate(R.layout.alert_edittext, null);
            new AlertDialog.Builder(BTRXActivity.this).setTitle(getString(R.string.rx_refresh_time))
            .setView(view)
            .setPositiveButton(getString(R.string.alertdialog_ok),new DialogInterface.OnClickListener() { 
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    EditText refreshTime = (EditText)view.findViewById(R.id.bt_rx_refresh_time);
                    if (refreshTime.getText().length() != 0) {
                        mRefreshTime = Integer.valueOf(refreshTime.getText().toString()).intValue();
                        Log.d(TAG,"Refresh time is "+mRefreshTime);
                        mBTRXHandler.removeMessages(MSG_BT_RX_READ);
                        mUiThread.post(mRefreshHandler);
                    } else {
                        Toast.makeText(BTRXActivity.this, "no refresh time, do nothing",
                                Toast.LENGTH_SHORT).show();
                    }
                }
            }).create().show();  
        } else if (v == mStop) {
            mUiThread.removeCallbacks(mRefreshHandler);
            mBTRXHandler.removeMessages(MSG_BT_RX_READ);
            if (!getSettingParam()) {
                return;
            }
            enableSettingPara();
            doMessage = mBTRXHandler.obtainMessage(MSG_BT_RX_STOP);
            mBTRXHandler.sendMessage(doMessage);
        } else if (v == mClear) {
            mUiThread.removeCallbacks(mRefreshHandler);
            mBTRXHandler.removeMessages(MSG_BT_RX_READ);
            mResultAdapter.result.clear();
            mResultAdapter.notifyDataSetChanged();
        }
  
    }
    
    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position,
            long id) {
        if (mBTRx != null) {
            if (parent == mPacType) {
                mBTRx.pactype = this.getResources().getStringArray(R.array.bt_pac_type_int)[position];
            }
        }
    }
    
    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }
    
    private void initUI(){
    	mChannel = (EditText)findViewById(R.id.bt_rx_channel);
    	mChannel.addTextChangedListener(new TextWatcher() {
            @Override  
            public void onTextChanged(CharSequence s, int start, int before,  
                    int count) {  
            }  

            @Override  
            public void beforeTextChanged(CharSequence s, int start, int count,  
                    int after) {  
            }  

            @Override  
            public void afterTextChanged(Editable s) { 
                if (mChannel.getText().length() != 0) {
                    if (Integer.parseInt(mChannel.getText().toString()) <0
                            || Integer.parseInt(mChannel.getText().toString()) > 78) {
                    	Toast.makeText(BTRXActivity.this, "number between 0 and 78",
                                Toast.LENGTH_SHORT).show();
                          return;
                      }
                }   
            }  
        });
    	
        mPacType = (Spinner) findViewById(R.id.bt_rx_pac_type);
        ArrayAdapter<String> pactype = new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, getResources()
                        .getStringArray(R.array.bt_pac_type));
        pactype
                .setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mPacType.setAdapter(pactype);
        mPacType.setOnItemSelectedListener(this);
    	
    	mGain = (EditText)findViewById(R.id.bt_rx_gain);
    	mGain.addTextChangedListener(new TextWatcher() {
            @Override  
            public void onTextChanged(CharSequence s, int start, int before,  
                    int count) {  
            }  

            @Override  
            public void beforeTextChanged(CharSequence s, int start, int count,  
                    int after) {  
            }  

            @Override  
            public void afterTextChanged(Editable s) { 
                if (mGain.getText().length() != 0) {
                    if (Integer.parseInt(mGain.getText().toString()) <0
                            || Integer.parseInt(mGain.getText().toString()) > 32) {
                    	Toast.makeText(BTRXActivity.this, "number between 0 and 32",
                                Toast.LENGTH_SHORT).show();
                          return;
                      }
                }   
            }  
        });
    	
    	mAddr = (EditText)findViewById(R.id.bt_rx_addr);
    	mResult = (ListView)findViewById(R.id.rx_result_text);
    	
    	mStart = (Button)findViewById(R.id.bt_rx_start);
    	mStart.setOnClickListener(this);
    	mRead = (Button)findViewById(R.id.bt_rx_read);
    	mRead.setOnClickListener(this);
    	mRead.setEnabled(false);
    	mAuto = (Button)findViewById(R.id.bt_rx_auto);
    	mAuto.setOnClickListener(this);
    	mAuto.setEnabled(false);
    	mClear = (Button)findViewById(R.id.bt_rx_clear);
    	mClear.setOnClickListener(this);
        mStop = (Button)findViewById(R.id.bt_rx_stop);
        mStop.setOnClickListener(this);  
        mStop.setEnabled(false);
    }
    
    private boolean getSettingParam(){
        if (mChannel.getText().length() == 0) {
            Toast.makeText(BTRXActivity.this, "please input TX Channel",
                    Toast.LENGTH_SHORT).show();
            return false;
        } else {
            mBTRx.channel = mChannel.getText().toString();  
        }
        
        if (mGain.getText().length() == 0) {
            Toast.makeText(BTRXActivity.this, "please input RX Gain",
                    Toast.LENGTH_SHORT).show();
            return false;
        } else {
            mBTRx.gain = mGain.getText().toString();  
        }
        
        if (mAddr.getText().length() == 0) {
            Toast.makeText(BTRXActivity.this, "please input RX Addr",
                    Toast.LENGTH_SHORT).show();
            return false;
        } else if (mAddr.getText().length() < 12) {
            Toast.makeText(BTRXActivity.this, "please input 12 bits RX Addr",
                    Toast.LENGTH_SHORT).show();
            return false;
        } else {
            //add : to addr
            mBTRx.addr = mAddr.getText().toString().substring(0, 2)+":"+mAddr.getText().toString().substring(2, 4)+":"
                        +mAddr.getText().toString().substring(4, 6)+":"+mAddr.getText().toString().substring(6, 8)+":"
                        +mAddr.getText().toString().substring(8, 10)+":"+mAddr.getText().toString().substring(10, 12);  
        }
        Log.d(TAG,"Now BT RX Test"+"\n"+"RX Channel is "+mBTRx.channel+"\n"+"RX Pac Type is "+mBTRx.pactype+"\n"
                +"RX Gain is "+mBTRx.gain+"\n"+"RX Addr is "+mBTRx.addr);
        return true;
    }
    
    private void disableSettingPara() {
        mChannel.setEnabled(false);
        mPacType.setEnabled(false);
        mGain.setEnabled(false);
        mAddr.setEnabled(false);
    }
    
    private void enableSettingPara() {
        mChannel.setEnabled(true);
        mPacType.setEnabled(true);
        mGain.setEnabled(true);
        mAddr.setEnabled(true);
    }
    
    class BTRXHandler extends Handler {
        public BTRXHandler(Looper looper) {
            super(looper);
        }
        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
                case MSG_BT_RX_START:
                    if (BTHelper.BTRxStart(mBTRx)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Log.d(TAG,"RX Start Success");
                                mStart.setEnabled(false);
                                mRead.setEnabled(true);
                                mAuto.setEnabled(true);
                                mStop.setEnabled(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(BTRXActivity.this, "RX Start Fail",
                                        Toast.LENGTH_SHORT).show();
                            }
                        }); 
                    }
                    break;
                case MSG_BT_RX_READ:
                    isNewPer = false;
                    isNewBer = false;
                    mRxResult = BTHelper.BTRxRead();
                    Log.d(TAG,"RX Read Result is "+mRxResult);
                    if (mRxResult != null && mRxResult.contains("OK")) {
                        Date curDate = new Date(System.currentTimeMillis());
                        mCurTime = mFormatter.format(curDate);
                        String res = mRxResult.substring(3);
                        Log.d(TAG,"res is "+res);
                        String[] res1= res.split("\\,");
                        for (int i = 0; i < res1.length; i++){
                            String[] res2 = res1[i].split("\\:");
                            if (res2[0].contains("rssi")) {
                                mCurRssi = "-"+res2[1];
                            } else if (res2[0].contains("per")) {
                               mCurPer = res2[1];
                            } else if (res2[0].contains("ber")) {
                               mCurBer = res2[1];
                            } else if (res2[0].contains("pkt_cnt")) {
                                mCurPktCnt = res2[1];                          
                            } else if (res2[0].contains("pkt_err_cnt")) {
                                mCurPktErrCnt = res2[1];
                                if (mCurPktErrCnt.equals("0")) {
                                    mCurPer = "0/"+mCurPktCnt+"=0%";
                                } else {
                                    isNewPer = true;
                                }
                            } else if (res2[0].contains("bit_cnt")) {
                                mCurBitCnt = res2[1];
                            } else if (res2[0].contains("bit_err_cnt")) {
                                mCurBitErrCnt = res2[1];
                                if(mCurBitErrCnt.contains("\0")){
                                    Log.d(TAG,"mCurBitErrCnt.contains \0");
                                    String [] value = mCurBitErrCnt.split("\0"); 
                                    Log.d(TAG,"value = "+value[0]);
                                    mCurBitErrCnt = value[0];
                                }
                                if (mCurBitErrCnt.equals("0")) {
                                    mCurBer = "0/"+mCurBitCnt+"=0%";
                                } else {
                                    isNewBer = true;
                                }
                            }
                            if (isNewPer) {
                                mCurPer = mCurPktErrCnt+"/"+mCurPktCnt+"=";
                                mCurPer += Math.floor((float)Integer.valueOf(mCurPktErrCnt).intValue()/(float)Integer.valueOf(mCurPktCnt).intValue() * 1000000)/10000 +"%";
                            }
                            if (isNewBer) {
                                mCurBer = mCurBitErrCnt+"/"+mCurBitCnt+"=";
                                mCurBer += Math.floor((float)Integer.valueOf(mCurBitErrCnt).intValue()/(float)Integer.valueOf(mCurBitCnt).intValue() * 1000000)/10000 +"%";
                            }
                        }
                        if (mResultAdapter.result.size()%2  == 0) {
                            mSingleRes = new BTRXResult(mCurTime,mCurRssi,mCurPer,mCurBer,true); 
                        } else {
                            mSingleRes = new BTRXResult(mCurTime,mCurRssi,mCurPer,mCurBer,false);  
                        }                         
                        //use message to transfer BTRXResult can avoid data cross because of MultiThread
                        Message uiMessage = mUiThread.obtainMessage(RESULT_REFRESH, mSingleRes);
                        mUiThread.sendMessage(uiMessage);
                        //use Global Variables mSingleRes to transfer BTRXResult is not good
                        //because of MultiThread using the same data
                  /*      mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mResultAdapter.result.add(mSingleRes);
                                mResultAdapter.notifyDataSetChanged();
                            }
                        });*/
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(BTRXActivity.this, "RX Rec Fail",
                                        Toast.LENGTH_SHORT).show();
                            }
                        }); 
                    }
                    break;
                case MSG_BT_RX_STOP:
                    if (BTHelper.BTRxStop(mBTRx)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mStart.setEnabled(true);
                                mRead.setEnabled(false);
                                mAuto.setEnabled(false);
                                mStop.setEnabled(false);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(BTRXActivity.this, "RX Stop Fail",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    }
                    break;
                case MSG_BT_OFF:
                    if (BTHelper.BTOff()) {
                        Toast.makeText(BTRXActivity.this, "BT Off Success",
                                Toast.LENGTH_SHORT).show();
                    } else {
                        Toast.makeText(BTRXActivity.this, "BT Off Fail",
                                Toast.LENGTH_SHORT).show();
                    }
                    BTHelper.closeSocket();
                    finish();
                    break;
                default:
                    break;
            }
        }
    }
    
    private final Runnable mRefreshHandler = new Runnable() {
        @Override
        public void run() {
            Message autoMessage = mBTRXHandler.obtainMessage(MSG_BT_RX_READ);
            mBTRXHandler.sendMessage(autoMessage);
            mUiThread.postDelayed(mRefreshHandler, mRefreshTime);
        }
    };
   
}