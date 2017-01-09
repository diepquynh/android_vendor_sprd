
package com.sprd.validationtools.itemstest;
/** BUG479359 zhijie.yang 2016/5/5 MMI add the magnetic sensors and the prox sensor calibration**/

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.ShellUtils;
import com.sprd.validationtools.Const;
import com.sprd.validationtools.R;

import android.content.Context;
import android.content.Intent;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import android.graphics.Color;
import android.util.Log;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnKeyListener;

public class ProxSensorCalibrationActivity extends BaseActivity {

    private static final String TAG = "ProxSensorCalibrationActivity";

    private static final int START_AUTO_TEST = 0;
    private static final int RESULT_AUTO_TEST = 1;
    private static final int SVAE_AUTO_TEST = 2;
    private static final int START_MANUAL_NEAR_TEST = 3;
    private static final int RESULT_MANUAL_NEAR_TEST = 4;
    private static final int SVAE_MANUAL_NEAR_TEST = 5;
    private static final int START_MANUAL_FAR_TEST = 6;
    private static final int RESULT_MANUAL_FAR_TEST = 7;
    private static final int SVAE_MANUAL_FAR_TEST = 8;

    private static final int AUTO_TEST = 0;
    private static final int MANUAL_TEST_NEAR = 1;
    private static final int MANUAL_TEST_FAR = 2;

    private static final String AUTO_SET_CMD = "0 8 1"; // auto calibrating
    private static final String TEST_GET_RESULT = "1 8 1";// get result of Calibration
    private static final String TEST_SAVE_RESULT = "3 8 1";// save the result
    private static final String MANUAL_NEAR_SET_CMD = "0 8 5";// manual near calibrating
    private static final String MANUAL_FAR_SET_CMD = "0 8 6";// manual far calibrating

    private static final int TWO_SECONDS = 2 * 1000;// 2 seconds;
    private static final int FOUR_SECONDS = 4 * 1000;// 4 seconds;

    private static final String PASS_NUMBER = "0";
    private static final String TEST_OK = "2";

    private Button mAutoButton;
    private Button mManualButton;
    private TextView mTestTips;
    private boolean autoResult = false;
    private boolean nearResult = false;
    private boolean farResult = false;
    private Context mContext;

    private boolean autoSaveResult = false;
    private boolean nearSaveResult = false;
    private boolean farSaveResult = false;

    private Handler mHandler = new Handler();
    private ProxHandler mProxHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.sensor_prox_calibration);
        mContext = this;
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mProxHandler = new ProxHandler(ht.getLooper());
        mAutoButton = (Button) findViewById(R.id.prox_auto_button);
        mManualButton = (Button) findViewById(R.id.prox_manual_button);
        mTestTips = (TextView) findViewById(R.id.test_tips);
        mAutoButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mAutoButton.setVisibility(View.GONE);
                mManualButton.setVisibility(View.GONE);
                mTestTips.setVisibility(View.VISIBLE);
                mTestTips.setText(getResources().getString(
                        R.string.prox_auto_test_tips));
                mProxHandler.sendMessage(mProxHandler.obtainMessage(START_AUTO_TEST));
            }
        });
        mManualButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mAutoButton.setVisibility(View.GONE);
                mManualButton.setVisibility(View.GONE);
                mTestTips.setVisibility(View.VISIBLE);
                showTipsDialog(MANUAL_TEST_NEAR);
            }
        });

    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    /**
     * the function to start calibration autotest: echo "0 8 1" > calibrator_cmd near-manual: echo
     * "0 8 5" > calibrator_cmd far-manual: echo "0 8 6" > calibrator_cmd
     **/
    private void startCalibration(int type) {
        Log.d(TAG, "startCalibration: " + type);
        if (type == AUTO_TEST) {
            ShellUtils.writeFile(Const.CALIBRATOR_CMD, AUTO_SET_CMD);
            return;
        } else if (type == MANUAL_TEST_NEAR) {
            ShellUtils.writeFile(Const.CALIBRATOR_CMD, MANUAL_NEAR_SET_CMD);
            return;
        } else if (type == MANUAL_TEST_FAR) {
            ShellUtils.writeFile(Const.CALIBRATOR_CMD, MANUAL_FAR_SET_CMD);
            return;
        }
    }

    /**
     ** the function to get the result of calibration echo "2 [SENSOR_ID] 1" > calibrator_cmd cat
     * calibrator_data if the get value is 2 ,the test is ok ,or test is fail
     **/
    private boolean getResult() {
        boolean isOk = false;
        ShellUtils.writeFile(Const.CALIBRATOR_CMD, TEST_GET_RESULT);
        String getResult = ShellUtils.readFile(Const.CALIBRATOR_DATA);
        Log.d(TAG, "the result of Prox sensor calibration: " + getResult);
        if (getResult != null && TEST_OK.equals(getResult.trim())) {
            isOk = true;
        }
        return isOk;
    }

    /**
     ** the functon to save result echo "3 [SENSOR_ID] 1" > calibrator_cmd cat calibrator_data to
     * save test result
     **/
	private boolean saveResult() {
		boolean isOk = false;
		Log.d(TAG, "saveResult...");
		ShellUtils.writeFile(Const.CALIBRATOR_CMD, TEST_SAVE_RESULT);
		String saveResult = ShellUtils.readFile(Const.CALIBRATOR_DATA);
		Log.d(TAG, "save result: " + saveResult);
		if (saveResult != null && PASS_NUMBER.equals(saveResult.trim())) {
			isOk = true;
			Log.d(TAG, "save result isOk: " + isOk);
		}
		return isOk;
	}

    private void showTipsDialog(int type) {
        Log.d(TAG, "showTipsDialog: " + type);
        final int step = type;
        String tips = null;
        if (step == MANUAL_TEST_NEAR) {
            tips = mContext.getString(R.string.prox_manual_tips_near);
        } else {
            tips = mContext.getString(R.string.prox_manual_tips_far);
        }
        AlertDialog alertDialog = new AlertDialog.Builder(mContext)
                .setTitle(getString(R.string.prox_calibration_button_manual))
                .setMessage(tips)
                .setNegativeButton(getString(R.string.alertdialog_ok),
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                if (step == MANUAL_TEST_NEAR) {
                                    mProxHandler.sendMessage(mProxHandler
                                            .obtainMessage(START_MANUAL_NEAR_TEST));
                                } else {
                                    mProxHandler.sendMessage(mProxHandler
                                            .obtainMessage(START_MANUAL_FAR_TEST));
                                }
                            }
                        }).create();
        alertDialog.show();
    }

    @Override
    public void onDestroy() {
        if (mProxHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mProxHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    private void ThreadSleep(int sleep) {
        try {
            Thread.sleep(sleep);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    class ProxHandler extends Handler {

        public ProxHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case START_AUTO_TEST:
                    startCalibration(AUTO_TEST);
                    mProxHandler.sendMessageDelayed(mProxHandler.obtainMessage(RESULT_AUTO_TEST),
                            FOUR_SECONDS);
                    break;
                case RESULT_AUTO_TEST:
                    autoResult = getResult();
                    mProxHandler.sendMessage(mProxHandler.obtainMessage(SVAE_AUTO_TEST));
                    break;
                case SVAE_AUTO_TEST:
                    autoSaveResult = saveResult();
                    if (autoResult && autoSaveResult) {
                        mHandler.post(new Runnable() {
                            public void run() {
                                Toast.makeText(mContext, R.string.text_pass,
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                        storeRusult(true);
                        finish();
                    } else {
                        mHandler.post(new Runnable() {
                            public void run() {
                                mTestTips.setText(getResources().getString(
                                        R.string.prox_auto_test_fail));
                            }
                        });
                    }
                    break;
                case START_MANUAL_NEAR_TEST:
                    mHandler.post(new Runnable() {
                        public void run() {
                            mTestTips.setText(getResources().getString(
                                    R.string.prox_manual_tips_near_testing));
                        }
                    });
                    startCalibration(MANUAL_TEST_NEAR);
                    mProxHandler.sendMessageDelayed(
                            mProxHandler.obtainMessage(RESULT_MANUAL_NEAR_TEST), FOUR_SECONDS);
                    break;
                case RESULT_MANUAL_NEAR_TEST:
                    nearResult = getResult();
                    mProxHandler.sendMessage(mProxHandler.obtainMessage(SVAE_MANUAL_NEAR_TEST));
                    break;
                case SVAE_MANUAL_NEAR_TEST:
                    nearSaveResult = saveResult();
                    if (nearResult && nearSaveResult) {
                        mHandler.post(new Runnable() {
                            public void run() {
                                mTestTips.setText(getResources().getString(
                                        R.string.prox_manual_tips_near_pass));
                            }
                        });
                    } else {
                        mHandler.post(new Runnable() {
                            public void run() {
                                mTestTips.setText(getResources().getString(
                                        R.string.prox_manual_tips_near_fail));
                            }
                        });
                    }
                    ThreadSleep(TWO_SECONDS);
                    showTipsDialog(MANUAL_TEST_FAR);
                    break;
                case START_MANUAL_FAR_TEST:
                    mHandler.post(new Runnable() {
                        public void run() {
                            mTestTips.setText(getResources().getString(
                                    R.string.prox_manual_tips_far_testing));
                        }
                    });
                    startCalibration(MANUAL_TEST_FAR);
                    mProxHandler.sendMessageDelayed(
                            mProxHandler.obtainMessage(RESULT_MANUAL_FAR_TEST), FOUR_SECONDS);
                    break;
                case RESULT_MANUAL_FAR_TEST:
                    farResult = getResult();
                    mProxHandler.sendMessage(mProxHandler.obtainMessage(SVAE_MANUAL_FAR_TEST));
                    break;
                case SVAE_MANUAL_FAR_TEST:
                    farSaveResult = saveResult();
                    if (nearResult && farResult && farSaveResult && nearSaveResult) {
                        mHandler.post(new Runnable() {
                            public void run() {
                                Toast.makeText(mContext, R.string.text_pass,
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                        storeRusult(true);
                        finish();
                    } else {
                        final String str = (nearResult ? getResources().getString(
                                R.string.prox_manual_tips_near_pass) : getResources().getString(
                                R.string.prox_manual_tips_near_fail))
                                + "\n"
                                + (farResult ? getResources().getString(
                                        R.string.prox_manual_tips_far_pass) : getResources()
                                        .getString(
                                                R.string.prox_manual_tips_far_fail));
                        mHandler.post(new Runnable() {
                            public void run() {
                                mTestTips.setText(str);
                            }
                        });
                    }
                    break;
            }
        }
    }
}
