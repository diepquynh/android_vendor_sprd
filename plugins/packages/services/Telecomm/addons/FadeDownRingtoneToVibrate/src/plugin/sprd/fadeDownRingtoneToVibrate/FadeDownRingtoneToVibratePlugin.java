package plugin.sprd.fadeDownRingtoneToVibrate;

import android.app.AddonManager;
import android.content.Context;
import android.provider.Settings;
import android.util.Log;

import com.android.server.telecom.CallsManager;
import com.android.server.telecom.Ringer;
import com.android.server.telecom.Call;
import com.android.server.telecom.CallState;
import com.android.server.telecom.FadeDownRingtoneToVibrateHelper;

import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;

public class FadeDownRingtoneToVibratePlugin extends FadeDownRingtoneToVibrateHelper
        implements AddonManager.InitialCallback, SensorEventListener {

    private static final String TAG = "FadeDownRingtoneToVibratePlugin";
    private Context mContext;
    private CallsManager mCallsManager;
    private Ringer mRinger;
    private SensorManager mSensorManager;
    private Sensor mSensor;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public FadeDownRingtoneToVibratePlugin() {

    }

    public void init(Context context, CallsManager callsManager, Ringer ringer) {
        mContext = context;
        mCallsManager = callsManager;
        mRinger = ringer;

        mSensorManager = (SensorManager) mContext.getSystemService(Context.SENSOR_SERVICE);
        mSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_SPRDHUB_HAND_UP);

        callsManager.addListener(this);
    }

    private boolean isFadedownRingtoneToVibrateOn() {
        boolean isFadedownRingtoneToVibrateOn = Settings.Global.getInt(
                mContext.getContentResolver(), Settings.Global.EASY_BELL, 0) != 0;
        return isFadedownRingtoneToVibrateOn;
    }

    @Override
    public void onCallAdded(Call call) {
        Call ringingCall = mCallsManager.getRingingCall();

        if (isFadedownRingtoneToVibrateOn() && mSensor != null && ringingCall != null) {
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
        mRinger.fadeDownRingtone();
        if (mSensor != null) {
            mSensorManager.unregisterListener(this, mSensor);
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }

}