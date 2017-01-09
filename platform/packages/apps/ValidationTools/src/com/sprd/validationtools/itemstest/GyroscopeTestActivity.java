package com.sprd.validationtools.itemstest;

import com.sprd.validationtools.BaseActivity;
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

public class GyroscopeTestActivity extends BaseActivity {

    private static final String TAG = "GyroscopeTestActivity";
    private static final int M_COMPAR_VALUE = 3;

    private SensorManager manager = null;
    private Sensor sensor = null;
    private SensorEventListener listener = null;

    private Context mContext;

    private boolean onetime = true;

    private boolean xPass = false;
    private boolean yPass = false;
    private boolean zPass = false;

    private float[] data_last = { 0, 0, 0 };

    private float[] data_current = { 0, 0, 0 };

    private TextView mDisplayText;
    private TextView mXsensorText;
    private TextView mYsensorText;
    private TextView mZsensorText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.sensor_gyroscope);
        mContext = this;
        mDisplayText = (TextView) findViewById(R.id.txt_msg_gyroscope);
        mXsensorText = (TextView) findViewById(R.id.txt_sensor_x);
        mYsensorText = (TextView) findViewById(R.id.txt_sensor_y);
        mZsensorText = (TextView) findViewById(R.id.txt_sensor_z);
        diplayXYZ(0, 0, 0);
        showPass(0, 0);
        showPass(0, 1);
        showPass(0, 2);
        initSensor();
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume...");
        //SPRD: Add for bug546426, test pressure or gyroscope frequently, ValidationTools no response.
        manager.registerListener(listener, sensor,
                SensorManager.SENSOR_DELAY_UI);
    }

    @Override
    protected void onPause() {
        Log.d(TAG, "onPause...");
        if (manager != null) {
            manager.unregisterListener(listener);
        }
        super.onPause();
    }

    private void showPass(int colorId, int postionId) {
        if (postionId == SensorManager.DATA_X) {
            if (colorId == 0) {
                mXsensorText.setTextColor(Color.WHITE);
                mXsensorText.setText("X-axis: ");
            } else {
                mXsensorText.setTextColor(Color.GREEN);
                mXsensorText.setText("X-axis: Pass!");
            }
        }
        if (postionId == SensorManager.DATA_Y) {
            if (colorId == 0) {
                mYsensorText.setTextColor(Color.WHITE);
                mYsensorText.setText("Y-axis: ");
            } else {
                mYsensorText.setTextColor(Color.GREEN);
                mYsensorText.setText("Y-axis: Pass!");
            }
        }
        if (postionId == SensorManager.DATA_Z) {
            if (colorId == 0) {
                mZsensorText.setTextColor(Color.WHITE);
                mZsensorText.setText("Z-axis: ");
            } else {
                mZsensorText.setTextColor(Color.GREEN);
                mZsensorText.setText("Z-axis: Pass!");
            }
        }
    }

    private boolean gsensorCheck(float data) {
        if (Math.abs(data) > M_COMPAR_VALUE) {
            return true;
        }
        return false;
    }

    private void diplayXYZ(float x, float y, float z) {
        mDisplayText.setText("\nX: " + x + "\nY: " + y + "\nZ: " + z);
    }

    private void initSensor() {
        Log.d(TAG, "Gyroscope test, init...");
        manager = (SensorManager) this.getSystemService(Context.SENSOR_SERVICE);
        sensor = manager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
        listener = new SensorEventListener() {
            public void onAccuracyChanged(Sensor s, int accuracy) {
            }

            public void onSensorChanged(SensorEvent event) {
                float x = event.values[0];
                float y = event.values[1];
                float z = event.values[2];
                Log.d(TAG, "mmi test values: " + x + "," + y + "," + z);
                diplayXYZ(x, y, z);
                data_current[0] = x;
                data_current[1] = y;
                data_current[2] = z;
                if (onetime && (x != 0) && (y != 0) && (z != 0)) {
                    onetime = false;
                    data_last[0] = x;
                    data_last[1] = y;
                    data_last[2] = z;
                }
                if ((!xPass) && gsensorCheck(data_current[0])
                        && (data_current[0] != data_last[0])) {
                    xPass = true;
                    showPass(1, 0);
                }
                if (!yPass && gsensorCheck(data_current[1])
                        && (data_current[1] != data_last[1])) {
                    yPass = true;
                    showPass(1, 1);
                }
                if (!zPass && gsensorCheck(data_current[2])
                        && (data_current[2] != data_last[2])) {
                    zPass = true;
                    showPass(1, 2);
                }
                if (xPass && yPass && zPass) {
                    Toast.makeText(mContext, R.string.text_pass,
                            Toast.LENGTH_SHORT).show();
                    storeRusult(true);
                    finish();
                }
            }
        };
    }
}
