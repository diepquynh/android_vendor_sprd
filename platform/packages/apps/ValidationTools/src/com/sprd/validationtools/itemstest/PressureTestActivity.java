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

public class PressureTestActivity extends BaseActivity {

    private static final String TAG = "PressureTestActivity";
    private static final int P_MAX_VALUE = 1100;
    private static final int P_MIN_VALUE = 900;
    private static final int P_PASS_TIMES = 5;

    private SensorManager manager = null;
    private Sensor sensor = null;
    private SensorEventListener listener = null;

    private Context mContext;

    private int cnt = 0;
    private boolean mPass = false;

    private float pressureValue = 0;
    private float dataPressure = 0;

    private TextView mDisplayText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.sensor_pressure);
        mContext = this;
        mDisplayText = (TextView) findViewById(R.id.txt_pressure_value);
        mDisplayText.setTextColor(Color.GREEN);
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

    private void displayValue(float x) {
        mDisplayText.setText(mContext.getResources().getString(
                R.string.pressure_value)
                + x + " hPa");
    }

    private void initSensor() {
        Log.d(TAG, "pressure test, init...");
        manager = (SensorManager) this.getSystemService(Context.SENSOR_SERVICE);
        sensor = manager.getDefaultSensor(Sensor.TYPE_PRESSURE);
        listener = new SensorEventListener() {
            public void onAccuracyChanged(Sensor s, int accuracy) {
            }

            public void onSensorChanged(SensorEvent event) {
                float x = event.values[0];
                displayValue(x);
                Log.d(TAG, "mmitest get value of pressure sensor: " + x);
                dataPressure = x;
                if (pressureValue != dataPressure
                        && (P_MIN_VALUE <= dataPressure)
                        && (P_MAX_VALUE > dataPressure)) {
                    cnt = cnt + 1;
                    if (cnt > P_PASS_TIMES) {
                        mPass = true;
                    }
                    pressureValue = dataPressure;
                }
                if (mPass) {
                    Toast.makeText(mContext, R.string.text_pass,
                            Toast.LENGTH_SHORT).show();
                    storeRusult(true);
                    finish();
                }
            }
        };
    }

}
