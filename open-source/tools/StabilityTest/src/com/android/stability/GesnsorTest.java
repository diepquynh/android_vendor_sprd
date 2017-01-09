package com.android.stability;

import android.app.Activity;
import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

public class GesnsorTest extends Activity implements SensorEventListener{
    /** Called when the activity is first created. */
    private TextView xValueTextView;
    private TextView yValueTextView;
    private TextView zValueTextView;
    private SensorManager sm;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.gsensor_layout);
        xValueTextView = (TextView) findViewById(R.id.textXValue);
        yValueTextView = (TextView) findViewById(R.id.textYValue);
        zValueTextView = (TextView) findViewById(R.id.textZValue);
        
        sm = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        int sensorType = Sensor.TYPE_ACCELEROMETER;
        
        sm.registerListener(this,sm.getDefaultSensor(sensorType),SensorManager.SENSOR_DELAY_UI);
        
    }
    

    @Override
    public void onSensorChanged(SensorEvent event) {
        if (Sensor.TYPE_ACCELEROMETER == event.sensor.getType()) {
            Log.i("GesnsorTest", "onSensorChanged");
            float xValue = event.values[0];
            float yValue = event.values[1];
            float zValue = event.values[2];
            
            xValueTextView.setText(String.format("%1.6f",xValue).replaceAll("0+$", "").replaceAll("\\.$", ""));
            yValueTextView.setText(String.format("%1.6f",yValue).replaceAll("0+$", "").replaceAll("\\.$", ""));
            zValueTextView.setText(String.format("%1.6f",zValue).replaceAll("0+$", "").replaceAll("\\.$", ""));
            showArrow(xValue,yValue);
        }
    }


    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        Log.i("GesnsorTest", "onAccuracyChanged");
        
    }
    
    private void showArrow(float x, float y) {
        int arrowId = 0;

        if (Math.abs(x) <= Math.abs(y)) {
            if (y < 0) {
                // up is low
                arrowId = R.drawable.arrow_up;
            } else if (y > 0) {
                // down is low
                arrowId = R.drawable.arrow_down;
            } else if (y == 0) {
                // do nothing
            }
        } else {
            if (x < 0) {
                // right is low
                arrowId = R.drawable.arrow_right;
            } else {
                // left is low
                arrowId = R.drawable.arrow_left;
            }
        }

        TextView txtArrow = (TextView) findViewById(R.id.txt_sensor_arrow);
        if (arrowId == 0) {
            txtArrow.setBackgroundDrawable(null);
        } else {
            txtArrow.setBackgroundResource(arrowId);
        }
    }
}
