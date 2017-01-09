package plugin.sprd.flipToMute;

import android.app.AddonManager;
import android.content.Context;
import android.provider.Settings;
import android.util.Log;

import com.android.server.telecom.CallsManager;
import com.android.server.telecom.Ringer;
import com.android.server.telecom.Call;
import com.android.server.telecom.CallState;
import com.android.server.telecom.FlipToMuteHelper;

import android.telephony.TelephonyManager;

import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;

public class FlipToMutePlugin extends FlipToMuteHelper
        implements AddonManager.InitialCallback, SensorEventListener {

    private static final String TAG = "FlipToMutePlugin";
    private Context mContext;
    private CallsManager mCallsManager;
    private Ringer mRinger;
    private SensorManager mSensorManager;
    private Sensor mSensor;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public FlipToMutePlugin() {

    }

    public void init(Context context, CallsManager callsManager, Ringer ringer) {
        mContext = context;
        mCallsManager = callsManager;
        mRinger = ringer;

        mSensorManager = (SensorManager) mContext.getSystemService(Context.SENSOR_SERVICE);
        mSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_SPRDHUB_FACE_UP_DOWN);

        callsManager.addListener(this);

    }

    private boolean isFlipToMuteOn() {
        boolean isFlipToMuteOn = Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.MUTE_INCOMING_CALLS, 0) != 0;
        return isFlipToMuteOn;
    }

    @Override
    public void onCallAdded(Call call) {
        Call ringingCall = mCallsManager.getRingingCall();

        if (isFlipToMuteOn() && mSensor != null && ringingCall != null) {
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
        if (isFlipToMuteOn()) {
            if (mCallsManager.getCallState() == TelephonyManager.CALL_STATE_RINGING) {
                mRinger.silence();
                if (mSensor != null) {
                    mSensorManager.unregisterListener(this, mSensor);
                }
            }
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }
}
