
package com.sprd.validationtools.itemstest;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.R;
import android.os.SystemProperties;

import android.app.Activity;
import android.graphics.Color;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.Vibrator;
import android.util.Log;
import android.view.View;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Button;
import android.widget.Toast;
import com.sprd.validationtools.IATUtils;
import android.media.AudioSystem;
import com.sprd.validationtools.Const;
import android.media.AudioManager;

public class HeadSetTest extends BaseActivity {
    private static final String TAG = "HeadSetTest";
    public byte mPLBTestFlag[] = new byte[1];
    private static final String HEADSET_UEVENT_MATCH = "DEVPATH=/devices/virtual/switch/h2w";
    private static final String HEADSET_STATE_PATH = "/sys/class/switch/h2w/state";
    private static final String HEADSET_NAME_PATH = "/sys/class/switch/h2w/name";
    public Handler mHandler = new Handler();
    private boolean isRollbackStarted = false;

    private static final int STEP_NONE = 0;
    private static final int STEP_INSERT_HEADSET = 1;
    private static final int STEP_PRESS_HEADSET_KEY = 2;
    private static final int STEP_LOOPBACK = 3;

    private TextView mInsertHeadsetNotice = null;
    private TextView mPressHeadsetKey = null;
    private TextView mLoopbackTest = null;
    private Button mEarKey = null;

    private static final int mPassColor = Color.GREEN;
    private static final int mNormalColor = Color.RED;

    private boolean mIsThirdPartHeadset = false;
    private boolean isWhaleSupport = Const.isWhale2Support();

    private AudioManager mAudioManager = null;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.headset_test);
        mAudioManager = (AudioManager) this
                .getSystemService(Context.AUDIO_SERVICE);
        mInsertHeadsetNotice = (TextView) findViewById(R.id.tx_insert_headset);
        mPressHeadsetKey = (TextView) findViewById(R.id.tx_press_earkey);
        mLoopbackTest = (TextView) findViewById(R.id.tx_start_phoneloop);
        mEarKey = (Button) findViewById(R.id.btn_earkey);
        mEarKey.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                step(STEP_PRESS_HEADSET_KEY);
                startMmiAudio();
                isRollbackStarted = true;
            }
        });

        if (isHeadsetExists()) {
            step(STEP_INSERT_HEADSET);
        } else {
            step(STEP_NONE);
        }
    }

    private void step(int step) {
        switch (step) {
            case STEP_NONE:
                mInsertHeadsetNotice.setTextColor(mNormalColor);
                mPressHeadsetKey.setTextColor(mNormalColor);
                mLoopbackTest.setTextColor(mNormalColor);
                mEarKey.setEnabled(false);
                break;
            case STEP_INSERT_HEADSET:
                mInsertHeadsetNotice.setTextColor(mPassColor);
                mPressHeadsetKey.setTextColor(mNormalColor);
                mLoopbackTest.setTextColor(mNormalColor);
                mEarKey.setEnabled(true);
                break;
            case STEP_PRESS_HEADSET_KEY:
                mInsertHeadsetNotice.setTextColor(mPassColor);
                mPressHeadsetKey.setTextColor(mPassColor);
                mLoopbackTest.setTextColor(mNormalColor);
                mEarKey.setEnabled(false);
                break;
            case STEP_LOOPBACK:
                mInsertHeadsetNotice.setTextColor(mPassColor);
                mPressHeadsetKey.setTextColor(mPassColor);
                mLoopbackTest.setTextColor(mPassColor);
                mEarKey.setEnabled(false);
                break;
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (isRollbackStarted) {
            step(STEP_PRESS_HEADSET_KEY);
            startMmiAudio();
        }
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_HEADSET_PLUG);
        registerReceiver(earphonePluginReceiver, filter);
    }

    @Override
    protected void onPause() {
        super.onPause();
        rollbackMmiAudio();
        unregisterReceiver(earphonePluginReceiver);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_HEADSETHOOK) {
            step(STEP_PRESS_HEADSET_KEY);
            startMmiAudio();
            isRollbackStarted = true;
            mEarKey.setPressed(true);
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_HEADSETHOOK) {
            mEarKey.setPressed(false);
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    private boolean isHeadsetExists() {
        char[] buffer = new char[1024];
        int newState = 0;
        FileReader file = null;
        try {
            file = new FileReader(HEADSET_STATE_PATH);
            int len = file.read(buffer, 0, 1024);
            newState = Integer.valueOf((new String(buffer, 0, len)).trim());
            if (file != null) {
                file.close();
                file = null;
            }
        } catch (FileNotFoundException e) {
            Log.e(TAG, "This kernel does not have wired headset support");
        } catch (Exception e) {
            try {
                if (file != null) {
                    file.close();
                    file = null;
                }
            } catch (IOException io) {
                Log.e(TAG, "", io);
            }
            Log.e(TAG, "", e);
        }
        return newState != 0;
    }

    private BroadcastReceiver earphonePluginReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent earphoneIntent) {
            if (earphoneIntent != null && earphoneIntent.getAction() != null) {
                Log.i(TAG,
                        "earphonePluginReceiver action : "
                                + earphoneIntent.getAction());
                if (earphoneIntent.getAction().equalsIgnoreCase(
                        Intent.ACTION_HEADSET_PLUG)) {
                    int st = 0;
                    st = earphoneIntent.getIntExtra("state", 0);
                    int deviceId = earphoneIntent.getIntExtra("microphone", 0);
                    Log.d(TAG, "microphone = " + deviceId);
                    mIsThirdPartHeadset = (deviceId == 0);
                    String nm = earphoneIntent.getStringExtra("name");
                    if (st > 0) {
                        step(STEP_INSERT_HEADSET);
                    } else if (st == 0) {
                        step(STEP_NONE);
                        rollbackMmiAudio();
                        isRollbackStarted = false;
                    }
                }
            }
        }
    };

    private void startMmiAudio() {
        Log.i("HeadSetLoopBackTest",
                "=== create thread to execute HeadSetLoopBackTest test command! ===");
        if (!isWhaleSupport) {
            new Thread() {
                public void run() {
                    String result;
                    String product = SystemProperties.get("ro.product.board");
                    Log.d(TAG, "product = " + product
                            + " mIsThirdPartHeadset = " + mIsThirdPartHeadset);
                    if (mIsThirdPartHeadset) {
                        result = IATUtils.sendATCmd("AT+SPVLOOP=1,4,8,2,3,0",
                                "atchannel0");
                    } else {
                        result = IATUtils.sendATCmd("AT+SPVLOOP=1,2,8,2,3,0",
                                "atchannel0");
                    }
                    if (!result.contains(IATUtils.AT_OK)) {
                        mHandler.post(new Runnable() {
                            public void run() {
                                step(STEP_PRESS_HEADSET_KEY);
                                Toast.makeText(HeadSetTest.this,
                                        "PhoneLoopBack Init Fail!",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    } else {
                        mHandler.post(new Runnable() {
                            public void run() {
                                step(STEP_LOOPBACK);
                            }
                        });
                    }
                }
            }.start();
        } else {
            /**
             * whale2 need to send test_out_stream_route=0x8,test_in_stream_route=0x80000010,
             * dsploop_type=1,dsp_loop=1
             **/
            new Thread() {
                public void run() {
                    if(mIsThirdPartHeadset){
                       mAudioManager.setParameter("test_out_stream_route", "0x8");
                    }else{
                       mAudioManager.setParameter("test_out_stream_route", "0x4");
                    }
                    if(mIsThirdPartHeadset){
                       mAudioManager.setParameter("test_in_stream_route",
                            "0x80000004");
                    }else{
                       mAudioManager.setParameter("test_in_stream_route",
                            "0x80000010");
                    }
                    mAudioManager.setParameter("dsp_delay","2000");
                    mAudioManager.setParameter("dsploop_type", "1");
                    mAudioManager.setParameter("dsp_loop", "1");
                    mHandler.post(new Runnable() {
                        public void run() {
                            step(STEP_LOOPBACK);
                        }
                    });
                }
            }.start();
        }

    }

    private void rollbackMmiAudio() {
        Log.i("HeadSetLoopBackTest",
                "=== create thread to execute HeadSetLoopBackTest test command! ===");
        if (!isWhaleSupport) {
            new Thread() {
                public void run() {
                    String result = IATUtils.sendATCmd(
                            "AT+SPVLOOP=0,2,8,2,3,0", "atchannel0");
                    Log.d(TAG, result);
                }
            }.start();
        } else {
            /** whale2 close the function need to send "dsp_loop=0" **/
            new Thread() {
                public void run() {
                    mAudioManager.setParameter("dsp_loop", "0");
                }
            }.start();
        }

    }

}
