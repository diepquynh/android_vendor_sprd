
package com.android.server.policy;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SprdSensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Handler;
import android.util.Log;

import java.io.PrintWriter;

/**
 * Watches for open camera gesture sensor events then invokes the listener.
 * @hide
 */
public abstract class TapEventListener {
    private static final String TAG = "TapEventListener";

    private SensorManager mSensorManager;
    private boolean mEnabled = false;
    private Sensor mSensor;
    private SensorEventListener mSensorEventListener;
    private final Handler mHandler;
    static final int TAP_REAR = 9;

    private final Object mLock = new Object();

    public TapEventListener(Context context, Handler handler) {
        mSensorManager = (SensorManager)context.getSystemService(Context.SENSOR_SERVICE);
        mHandler = handler;
        mSensor = mSensorManager.getDefaultSensor(SprdSensor.TYPE_SPRDHUB_TAP);
        if (mSensor != null) {
            // Create listener only if sensors do exist
            mSensorEventListener = new SensorEventListenerImpl();
        }
    }

    public boolean isSupported() {
        synchronized (mLock) {
            return mSensor != null;
        }
    }
    /**
     * Enables the TapEventListener so it will monitor the sensor and call
     * {@link #onTapChanged} when the device tap gesture changes.
     */
    public void enable() {
        synchronized (mLock) {
            if (mSensor == null) {
                Log.w(TAG, "Cannot detect sensors. Not enabled");
                return;
            }
            if (mEnabled == false) {
                mSensorManager.registerListener(mSensorEventListener, mSensor,
                        SensorManager.SENSOR_DELAY_NORMAL, mHandler);
                mEnabled = true;
            }
        }
    }

    /**
     * Disables the TapEventListener.
     */
    public void disable() {
        synchronized (mLock) {
            if (mSensor == null) {
                Log.w(TAG, "Cannot detect sensors. Invalid disable");
                return;
            }
            if (mEnabled == true) {
                mSensorManager.unregisterListener(mSensorEventListener);
                mEnabled = false;
            }
        }
    }

    public void dump(PrintWriter pw, String prefix) {
        synchronized (mLock) {
            pw.println(prefix + TAG);
            prefix += "  ";
            pw.println(prefix + "mEnabled=" + mEnabled);
            pw.println(prefix + "mSensor=" + mSensor);
        }
    }

    class SensorEventListenerImpl implements SensorEventListener {
        public void onSensorChanged(SensorEvent event) {
            if(PhoneWindowManager.DEBUG_INPUT)Log.d(TAG," TapEventListener onSensorChanged event = " + event.values[1]);
            if(event.values[1] == TAP_REAR){
                onTapChanged();
            }
        }

        @Override
        public void onAccuracyChanged(Sensor sensor, int accuracy) {
            // TODO Auto-generated method stub
        }
    }

    public abstract void onTapChanged();
}

