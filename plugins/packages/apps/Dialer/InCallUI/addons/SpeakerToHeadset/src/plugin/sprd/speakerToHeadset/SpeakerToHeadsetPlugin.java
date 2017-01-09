package plugin.sprd.speakerToHeadset;

import android.app.AddonManager;
import android.content.Context;
import android.provider.Settings;
import android.telecom.CallAudioState;
import android.util.Log;

import com.android.incallui.SpeakerToHeadsetHelper;
import com.android.incallui.CallButtonFragment;

import android.hardware.Sensor;
import android.hardware.SprdSensor;
import android.hardware.SensorManager;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;

public class SpeakerToHeadsetPlugin extends SpeakerToHeadsetHelper
        implements AddonManager.InitialCallback, SensorEventListener {

    private static final String TAG = "SpeakerToHeadsetPlugin";
    private Context mContext;
    private CallButtonFragment mCallButtonFragment;
    private SensorManager mSensorManager;
    private Sensor mSensor;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public SpeakerToHeadsetPlugin() {

    }

    public void init(Context context, CallButtonFragment callButtonFragment) {
        mContext = context;
        mCallButtonFragment = callButtonFragment;

        mSensorManager = (SensorManager) mContext.getSystemService(Context.SENSOR_SERVICE);
        mSensor = mSensorManager.getDefaultSensor(SprdSensor.TYPE_SPRDHUB_HAND_UP);

        registerSpeakerTriggerListener();
    }

    private boolean isHandsfreeSwitchToHeadsetOn() {
        boolean isHandsfreeSwitchToHeadsetOn = Settings.Global.getInt(
                mContext.getContentResolver(), Settings.Global.HANDSFREE_SWITCH, 0) != 0;
        return isHandsfreeSwitchToHeadsetOn;
    }

    void registerSpeakerTriggerListener() {

        if (isHandsfreeSwitchToHeadsetOn() && mSensor != null) {
            Log.d(TAG, "registerListener.");
            mSensorManager.registerListener(this, mSensor, SensorManager.SENSOR_DELAY_NORMAL);
        }
    }

    public void unRegisterSpeakerTriggerListener() {
        if (mSensor != null) {
            Log.d(TAG, "unregisterListener.");
            mSensorManager.unregisterListener(this, mSensor);
        }
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        Log.d(TAG, "onSensorChanged");
        if (mCallButtonFragment.getPresenter().getAudioMode() == CallAudioState.ROUTE_SPEAKER) {
            mCallButtonFragment.getPresenter().toggleSpeakerphone();
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }

}