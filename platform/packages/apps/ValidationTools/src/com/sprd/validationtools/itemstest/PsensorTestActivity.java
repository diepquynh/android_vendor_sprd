
package com.sprd.validationtools.itemstest;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.R;

import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.view.WindowManager;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

public class PsensorTestActivity extends BaseActivity {
    /** the value of change color */
    private static final float VALUE_OF_CHANGE_COLOR = 0.5f;

    /** the default value */
    private static final int LSENSOR_DEFAULT_VALUE = 0;
    private static final float PSENSOR_DEFAULT_VALUE = 1.0f;

    private static final String VALUE_FAR = "Distant";

    private static final String VALUE_CLOSE = "Closer";

    /** sensor manager object */
    private SensorManager pManager = null;

    /** sensor object */
    private Sensor pSensor = null;

    /** sensor listener object */
    private SensorEventListener pListener = null;

    /** the status of p-sensor */
    private TextView psensorTextView;

    private static final float MAXIMUM_BACKLIGHT = 1.0f;

    /** Screen backlight when the value of the darkest */
    private static final float MINIMUM_BACKLIGHT = 0.1f;

    /** sensor manager object */
    private SensorManager lManager = null;

    /** sensor object */
    private Sensor lSensor = null;

    /** sensor listener object */
    private SensorEventListener lListener = null;

    /** the progressBar object */
    private ProgressBar lsensorProgressBar;

    /** the textview object */
    private TextView valueIllumination;

    /** the max value of progressBar */
    private static final int MAX_VALUE_PROGRESSBAR = 300;

    /** System backlight value */
    private int mCurrentValue;

    /** Integer into a floating-point type */
    private float mBrightnessValue;

    /** Brightness current value */
    private static final int BRIGHTNESS_CURRENT_VALUE = 180;

    /** Brightness max value */
    private static final float BRIGHTNESS_MAX_VALUE = 255.0f;

    private Context mContext;

    private int mSensorMin = -1;
    private int mSensorMax = -1;

    private boolean mIsCloseDone = false;
    private boolean mIsDistantDone = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;
        setContentView(R.layout.sensor_proximity);
        setTitle(R.string.proximity_sensor_test);
        psensorTextView = (TextView) findViewById(R.id.txt_psensor);
        initSensor();
        setPsensorDisplay(VALUE_FAR, PSENSOR_DEFAULT_VALUE, Color.WHITE);

        try {
            mCurrentValue = Settings.System.getInt(getContentResolver(),
                    Settings.System.SCREEN_BRIGHTNESS);
        } catch (SettingNotFoundException e) {
            mCurrentValue = BRIGHTNESS_CURRENT_VALUE;
        }
        mBrightnessValue = mCurrentValue / BRIGHTNESS_MAX_VALUE;

        valueIllumination = (TextView) findViewById(R.id.txt_value_lsensor);

        lsensorProgressBar = (ProgressBar) findViewById(R.id.progressbar_lsensor);
        lsensorProgressBar.setMax(MAX_VALUE_PROGRESSBAR);

        lManager = (SensorManager) this.getSystemService(Context.SENSOR_SERVICE);
        lSensor = lManager.getDefaultSensor(Sensor.TYPE_LIGHT);

        lListener = new SensorEventListener() {
            public void onAccuracyChanged(Sensor s, int accuracy) {
            }

            public void onSensorChanged(SensorEvent event) {
                float x = event.values[SensorManager.DATA_X];

                int processInt = (int) x;
                if (mSensorMin == -1
                        || mSensorMin > x) {
                    mSensorMin = processInt;
                }

                if (mSensorMax == -1
                        || mSensorMax < x) {
                    mSensorMax = processInt;
                }

                showStatus(x);

                if ((mSensorMax - mSensorMin) > 0
                        && mIsCloseDone && mIsDistantDone) {
                    Toast.makeText(mContext, R.string.text_pass, Toast.LENGTH_SHORT).show();
                    storeRusult(true);
                    finish();
                }
            }
        };
    }

    @Override
    protected void onResume() {
        super.onResume();
        setPsensorDisplay(VALUE_FAR, PSENSOR_DEFAULT_VALUE, Color.WHITE);
        psensorTextView.setBackgroundColor(Color.WHITE);
        pManager.registerListener(pListener, pSensor, SensorManager.SENSOR_DELAY_UI);

        valueIllumination.setText(getString(R.string.lsensor_value) + LSENSOR_DEFAULT_VALUE);
        lsensorProgressBar.setProgress(LSENSOR_DEFAULT_VALUE);
        lManager.registerListener(lListener, lSensor, SensorManager.SENSOR_DELAY_UI);
    }

    @Override
    protected void onPause() {
        if (pManager != null) {
            pManager.unregisterListener(pListener);
        }

        lManager.unregisterListener(lListener);
        setBrightness(mBrightnessValue);
        super.onPause();
    }

    private void showStatus(float x) {
        valueIllumination.setText(getString(R.string.lsensor_value) + x);
        int valueProgress = (int) x;
        lsensorProgressBar.setProgress(valueProgress);
        float mCurrentBrightnessValue = x / BRIGHTNESS_MAX_VALUE;
        if (mCurrentBrightnessValue > MAXIMUM_BACKLIGHT) {
            setBrightness(MAXIMUM_BACKLIGHT);
        } else if (mCurrentBrightnessValue < MINIMUM_BACKLIGHT) {
            setBrightness(MINIMUM_BACKLIGHT);
        } else {
            setBrightness(mCurrentBrightnessValue);
        }
    }

    private void setBrightness(float brightness) {
        WindowManager.LayoutParams lp = getWindow().getAttributes();
        lp.screenBrightness = brightness;
        getWindow().setAttributes(lp);
    }

    private void setPsensorDisplay(String dis, float data, int color) {
        psensorTextView.setText("");
        if (pSensor != null) {
            psensorTextView.append("Chip id: " + pSensor.getName() + "\n");
        }

        psensorTextView.append(getString(R.string.psensor_msg_data) + " " + data + "\n");
        psensorTextView.append(getString(R.string.psensor_msg_value) + " " + dis);
        psensorTextView.setBackgroundColor(color);
    }

    private void initSensor() {
        pManager = (SensorManager) this.getSystemService(Context.SENSOR_SERVICE);
        pSensor = pManager.getDefaultSensor(Sensor.TYPE_PROXIMITY);
        pListener = new SensorEventListener() {
            public void onAccuracyChanged(Sensor s, int accuracy) {
            }

            public void onSensorChanged(SensorEvent event) {
                float x = event.values[SensorManager.DATA_X];

                if (x <= VALUE_OF_CHANGE_COLOR) {
                    setPsensorDisplay(VALUE_CLOSE, x, Color.RED);
                    mIsCloseDone = true;
                } else {
                    setPsensorDisplay(VALUE_FAR, x, Color.WHITE);
                    mIsDistantDone = true;
                }

                if ((mSensorMax - mSensorMin) > 0
                        && mIsCloseDone && mIsDistantDone) {
                    Toast.makeText(mContext, R.string.text_pass, Toast.LENGTH_SHORT).show();
                    storeRusult(true);
                    finish();
                }
            }
        };
    }
}
