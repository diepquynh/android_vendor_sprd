package plugin.sprd.maxRingingVolumeAndVibrate;

import android.app.AddonManager;
import android.content.Context;
import android.provider.Settings;
import android.media.AudioManager;
import android.util.Log;

import com.android.server.telecom.CallsManager;
import com.android.server.telecom.Ringer;
import com.android.server.telecom.Call;
import com.android.server.telecom.CallState;
import com.android.server.telecom.MaxRingingVolumeAndVibrateHelper;

import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;

public class MaxRingingVolumeAndVibratePlugin extends MaxRingingVolumeAndVibrateHelper
        implements AddonManager.InitialCallback, SensorEventListener {

    private static final String TAG = "MaxRingingVolumeAndVibratePlugin";
    private Context mContext;
    private CallsManager mCallsManager;
    private Ringer mRinger;
    private SensorManager mSensorManager;
    private Sensor mSensor;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public MaxRingingVolumeAndVibratePlugin() {

    }

    public void init(Context context, CallsManager callsManager, Ringer ringer) {
        mContext = context;
        mCallsManager = callsManager;
        mRinger = ringer;

        mSensorManager = (SensorManager) mContext.getSystemService(Context.SENSOR_SERVICE);
        mSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_SPRDHUB_POCKET_MODE);

        callsManager.addListener(this);
    }

    private boolean isMaxRingingVolumeAndVibrateOn() {

        boolean isMaxRingingVolumeAndVibrateOn = Settings.Global.getInt(
                mContext.getContentResolver(), Settings.Global.SMART_BELL, 0) != 0;
        return isMaxRingingVolumeAndVibrateOn;
    }

    @Override
    public void onCallAdded(Call call) {
        Call ringingCall = mCallsManager.getRingingCall();

        if (isMaxRingingVolumeAndVibrateOn() && mSensor != null && ringingCall != null) {
            Log.d(TAG, "registerListener.");
            mSensorManager.registerListener(this, mSensor, SensorManager.SENSOR_DELAY_NORMAL);
        }
    }

    @Override
    public void onCallStateChanged(Call call, int oldState, int newState) {

        if (mSensor != null && oldState == CallState.RINGING && newState != CallState.RINGING) {
            Log.d(TAG, "unregisterListener.");
            mSensorManager.unregisterListener(this, mSensor);
        }
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        Log.d(TAG, "onSensorChanged");
        if (event != null && event.values[0] == 1) {
            AudioManager audioManager =
                    (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
            if (audioManager.getStreamVolume(AudioManager.STREAM_RING) > 0) {
                mRinger.maxRingingVolume();
            }
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }

}