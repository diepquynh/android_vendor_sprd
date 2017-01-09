package plugin.sprd.pickUpToAnswerIncomingCall;

import android.app.AddonManager;
import android.content.Context;
import android.provider.Settings;
import android.util.Log;

import com.android.server.telecom.CallsManager;
import com.android.server.telecom.Call;
import com.android.server.telecom.CallState;

import android.telecom.VideoProfile;

import com.android.server.telecom.PickUpToAnswerIncomingCallHelper;

import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;

public class PickUpToAnswerIncomingCallPlugin extends PickUpToAnswerIncomingCallHelper
        implements AddonManager.InitialCallback, SensorEventListener {

    private static final String TAG = "PickUpToAnswerIncomingCallPlugin";
    private Context mContext;
    private CallsManager mCallsManager;
    private SensorManager mSensorManager;
    private Sensor mSensor;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public PickUpToAnswerIncomingCallPlugin() {

    }

    public void init(Context context, CallsManager callsManager) {
        mContext = context;
        mCallsManager = callsManager;

        mSensorManager = (SensorManager) mContext.getSystemService(Context.SENSOR_SERVICE);
        mSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_SPRDHUB_HAND_UP);

        mCallsManager.addListener(this);
    }

    private boolean isPickUpToAnswerIncomingCallOn() {
        boolean isPickUpToAnswerIncomingCallOn = Settings.Global.getInt(
                mContext.getContentResolver(), Settings.Global.EASY_ANSWER, 0) != 0;
        return isPickUpToAnswerIncomingCallOn;
    }

    @Override
    public void onCallAdded(Call call) {
        Call ringingCall = mCallsManager.getRingingCall();

        if (isPickUpToAnswerIncomingCallOn() && mSensor != null && ringingCall != null) {
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
        Log.d(TAG, "onSensorChanged.");
        Call ringingCall = mCallsManager.getRingingCall();
        Call activeOrBackgroundCall = mCallsManager.getActiveOrBackgroundCall();

        if (activeOrBackgroundCall == null && ringingCall != null) {
            mCallsManager.answerCall(ringingCall, VideoProfile.STATE_AUDIO_ONLY);
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }

}