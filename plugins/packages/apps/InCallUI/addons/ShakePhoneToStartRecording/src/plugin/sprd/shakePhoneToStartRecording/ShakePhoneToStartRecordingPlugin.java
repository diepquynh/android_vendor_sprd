package plugin.sprd.shakePhoneToStartRecording;

import android.app.AddonManager;
import android.content.Context;
import android.provider.Settings;
import android.util.Log;

import com.android.dialer.DialerApplication;
import com.android.incallui.ShakePhoneToStartRecordingHelper;
import com.android.incallui.CallButtonFragment;
import com.android.incallui.CallList;
import com.android.incallui.Call;

import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;

public class ShakePhoneToStartRecordingPlugin extends ShakePhoneToStartRecordingHelper
        implements AddonManager.InitialCallback, SensorEventListener {

    private static final String TAG = "ShakePhoneToStartRecordingPlugin";
    private Context mContext;
    private CallButtonFragment mCallButtonFragment;
    private SensorManager mSensorManager;
    private Sensor mSensor;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public ShakePhoneToStartRecordingPlugin() {

    }

    public void init(Context context, CallButtonFragment callButtonFragment) {
        mContext = context;
        mCallButtonFragment = callButtonFragment;

        registerTriggerRecorderListener();
    }

    private boolean isShakePhoneToStartRecordingOn() {
        boolean isShakePhoneToStartRecordingOn = Settings.Global.getInt(mContext
                .getContentResolver(), Settings.Global.SMART_CALL_RECORDER, 0) != 0;
        return isShakePhoneToStartRecordingOn;
    }

    void registerTriggerRecorderListener() {
        Log.d(TAG, "registerTriggerListener");
        if (isShakePhoneToStartRecordingOn()) {
            mSensorManager = (SensorManager) mContext.getSystemService(Context.SENSOR_SERVICE);
            mSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_SPRDHUB_SHAKE);
            if (mSensor != null) {
                mSensorManager.registerListener(this, mSensor, SensorManager.SENSOR_DELAY_NORMAL);
            }
        }
    }

    public void unRegisterTriggerRecorderListener() {
        Log.d(TAG, "unRegisterTriggerListener");
        if (mSensor != null) {
            mSensorManager.unregisterListener(this, mSensor);
        }
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        Log.d(TAG, "onSensorChanged");
        Call mCall = CallList.getInstance().getActiveCall();
        Context context = mContext.getApplicationContext();
        DialerApplication dialerApplication = (DialerApplication) context;
        boolean isRecordingStart = dialerApplication.getIsRecordingStart();

        if (!isRecordingStart && mCall != null
                && mCallButtonFragment.getActivity() != null
                && mCallButtonFragment.getActivity().isResumed()) {
            mCallButtonFragment.toggleRecord();
            unRegisterTriggerRecorderListener();
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }

}
