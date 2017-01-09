package com.sprd.voicetrigger;

import android.app.Application;
import android.app.Service;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.util.Log;
import android.widget.Toast;

import com.sprd.voicetrigger.aidl.VoiceTriggerAidlService;
import com.sprd.voicetrigger.global.SharedPreferencesField;
import com.sprd.voicetrigger.provider.ContentProviderHelper;

public class VoiceTriggerApp extends Application {
    private final String TAG = "VoiceTriggerApp";
    private final String START_SERVICE_ACTION = "com.sprd.voicetrigger.VoiceTriggerService";
    private VoiceTriggerAidlService mVoiceTriggerAidlService;
    private boolean bindResult = false;
    private boolean isEnableRecognition;
    private ServiceConnection mServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName arg0, IBinder service) {
            Log.d(TAG, "<onServiceConnected> Service connected");
            mVoiceTriggerAidlService = VoiceTriggerAidlService.Stub.asInterface(service);
            if (isEnableRecognition) {
                try {
                    Log.i(TAG, "startRecognition when Service connected");
                    mVoiceTriggerAidlService.startRecognition();
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            } else {
                try {
                    Log.i(TAG, "stopRecognition when Service connected");
                    mVoiceTriggerAidlService.stopRecognition();
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0) {
            Log.d(TAG, "<onServiceDisconnected> Service disconnected");
            mVoiceTriggerAidlService = null;
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        isEnableRecognition = ContentProviderHelper.isOpenSwitch(this);
        if (!bindService()) {
            Log.e(TAG, "<onStart> fail to bind service");
        }
    }

    public boolean bindService() {
        Intent intent = new Intent();
        intent.setAction(START_SERVICE_ACTION);
        intent.setPackage(getPackageName());
        bindResult = bindService(intent, mServiceConnection, Service.BIND_AUTO_CREATE);
        return bindResult;
    }

    public void unbindService() {
        unbindService(mServiceConnection);
        bindResult = false;
    }

    public boolean isServiceBinded() {
        return bindResult;
    }

    public void startRecognition() {
        isEnableRecognition = true;
        if (mVoiceTriggerAidlService == null) {
            return;
        }
        try {
            mVoiceTriggerAidlService.startRecognition();
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void stopRecognition() {
        isEnableRecognition = false;
        if (mVoiceTriggerAidlService == null) {
            return;
        }
        try {
            mVoiceTriggerAidlService.stopRecognition();
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }
    public void loadSoundModel(boolean isFirstLoad, String dataDir) {
        if (mVoiceTriggerAidlService == null) {
            return;
        }
        try {
            mVoiceTriggerAidlService.loadSoundModel(isFirstLoad,dataDir);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }
}
