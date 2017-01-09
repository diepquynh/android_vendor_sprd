
package com.android.server.policy;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.hardware.SprdSensor;
import android.os.Handler;
import android.util.Log;

import java.io.PrintWriter;

/**
 * Watches for open camera gesture sensor events then invokes the listener.
 * @hide
 */
public abstract class PocketEventListener {
    private static final String TAG = "PocketEventListener";

    private SensorManager mSensorManager;
    private boolean mEnabled = false;
    private Sensor mSensor;
    private SensorEventListener mSensorEventListener;
    private final Handler mHandler;

    private final Object mLock = new Object();

    public PocketEventListener(Context context, Handler handler) {
        mSensorManager = (SensorManager)context.getSystemService(Context.SENSOR_SERVICE);
        mHandler = handler;

        mSensor = mSensorManager.getDefaultSensor(SprdSensor.TYPE_SPRDHUB_POCKET_MODE, true);
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
     * Enables the PocketEventListener so it will monitor the sensor and call
     * {@link #onPocketChanged} when the device pocket mode changes.
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
     * Disables the PocketEventListener.
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
            if(PhoneWindowManager.DEBUG_INPUT)Log.d(TAG," PocketEventListener onPocketChanged event = " + event.values[0]);
            int mode = (int)event.values[0];
            if(mode ==1){
                onStartPocketMode();
            }else if(mode == 0){
                onStopPocketMode();
            }
        }

        @Override
        public void onAccuracyChanged(Sensor sensor, int accuracy) {
            // TODO Auto-generated method stub
        }
    }

    public abstract void onStartPocketMode();
    public abstract void onStopPocketMode();
}


