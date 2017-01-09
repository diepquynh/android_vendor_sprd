package com.sprd.voicetrigger.services;

import android.app.Service;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.hardware.soundtrigger.SoundTrigger.RecognitionEvent;
import android.hardware.soundtrigger.SoundTrigger.SoundModelEvent;
import android.hardware.soundtrigger.SoundTrigger.StatusListener;
import android.os.Handler;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.widget.Toast;

import com.sprd.voicetrigger.HelloTriggerActivity;
import com.sprd.voicetrigger.SensibilityActivity;
import com.sprd.voicetrigger.aidl.IVoiceTriggerUnlock;
import com.sprd.voicetrigger.aidl.VoiceTriggerAidlService.Stub;
import com.sprd.voicetrigger.controller.VoiceTriggerController;
import com.sprd.voicetrigger.R;
import com.sprd.voicetrigger.provider.ContentProviderHelper;
import android.os.UserManager;
import android.content.Context;
import android.telephony.TelephonyManager;

import java.io.File;

public class VoiceTriggerService extends Service {
    private final String TAG = "VoiceTriggerService";
    private TelephonyManager mTelephonyManager;
    private VoiceTriggerAidlInterfaceBinder mBinder;
    private IVoiceTriggerUnlock mVoiceTriggerUnlockService;
    private ServiceConnection mIVoiceTriggerUnlockServiceConn = new ServiceConnection() {
        @Override
        public void onServiceDisconnected(ComponentName arg0) {
            mVoiceTriggerUnlockService = null;
            Log.d(TAG, " screen unlock service disconnected!");
        }

        @Override
        public void onServiceConnected(ComponentName arg0, IBinder arg1) {
            mVoiceTriggerUnlockService = IVoiceTriggerUnlock.Stub.asInterface(arg1);
            Log.d(TAG, " screen unlock service connected!");
        }
    };

    public VoiceTriggerService() {
        // do nothing
    }

    @Override
    public void onCreate() {
        super.onCreate();
        UserManager userManager = (UserManager) getSystemService(Context.USER_SERVICE);
        Log.i(TAG, "VoiceTriggerService onCreate !");
        mBinder = new VoiceTriggerAidlInterfaceBinder();
        Log.i(TAG, "VoiceTriggerService  userManager.isGuestUser() ="+userManager.isGuestUser()+",isSystemUser="+userManager.isSystemUser()
                +",isAdminUser="+userManager.isAdminUser()+",isLinkedUser="+userManager.isLinkedUser()
                +",getUserName ="+userManager.getUserName()+",getUserHandle="+userManager.getUserHandle());
        if (userManager.isSystemUser()){
            Log.i(TAG, "isSystemUser bind SystemUI");
            bindUnlockService();
        } else{
            Log.i(TAG, "VoiceTriggerService is other user,cann't binder SystemUI");
        }
        mTelephonyManager = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
    }

    private boolean bindUnlockService() {
        Intent voiceTriggerUnlockIntent = new Intent();
        voiceTriggerUnlockIntent.setAction("android.intent.action.VoiceTriggerUnlockAIDLService");
        voiceTriggerUnlockIntent.setPackage("com.android.systemui");
        return bindService(voiceTriggerUnlockIntent, mIVoiceTriggerUnlockServiceConn, BIND_AUTO_CREATE);
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.i(TAG, "VoiceTriggerService onBind !");
        return (IBinder) mBinder;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "VoiceTriggerService onStartCommand !");
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "VoiceTriggerService onDestroy !");
        if (mVoiceTriggerUnlockService != null) {
            unbindService(mIVoiceTriggerUnlockServiceConn);
        }
        super.onDestroy();
    }

    public class VoiceTriggerAidlInterfaceBinder extends Stub {
        private final String TAG = "mBinder";
        public VoiceTriggerController mVoiceTriggerController;
        private int mEventStatus;
        private boolean isOpenTrigger = false;
        private boolean isStopedRecognition = false;
        private StatusListener mStatusListener = new StatusListener() {
            @Override
            public void onRecognition(RecognitionEvent event) {
                Log.d(TAG, "onRecognition: envent = " + event.status);
                switch (event.status) {
                    case 0:
                        mEventStatus = 0;

                        try {
                            stopRecognition();
                            startRecognition();
                        } catch (RemoteException e) {
                            e.printStackTrace();
                        }

                        int callState = mTelephonyManager.getCallState();
                        Log.d(TAG, "onRecognition: callState = "+callState);
                        if(TelephonyManager.CALL_STATE_IDLE != mTelephonyManager.getCallState()){
                            Log.d(TAG, "onRecognition: callState isn't in idle");
                            return;
                         }
                        try {
                            if (mVoiceTriggerUnlockService != null) {
                                mVoiceTriggerUnlockService.voiceTriggerUnlock();
                                Intent recog = new Intent(getApplicationContext(), HelloTriggerActivity.class);
                                recog.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                                startActivity(recog);
                            } else {
                                Log.d(TAG, "mVoiceTriggerUnlockService is not bind");
                            }
                        } catch (RemoteException e) {
                            e.printStackTrace();
                        }
                        break;
                    case 1:
                        mEventStatus = 1;
                        break;
                    default:
                        break;
                }
            }

            @Override
            public void onSoundModelUpdate(SoundModelEvent event) {
                Log.d(TAG, "onSoundModelUpdate");
                // firmware loaded then start recognition again
                //SPRD：modified for bug559272 When update or setup the phrass,it notices English Toast when the system language is chinese
                Toast.makeText(VoiceTriggerService.this, R.string.update_phrass_success, Toast.LENGTH_SHORT).show();
                try {
                    startRecognition();
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
                isOpenTrigger = true;
            }

            @Override
            public void onServiceStateChange(int state) {
                Log.d(TAG, "onServiceStateChange");
                if (state == 0) {
                    //SERVICE_STATE_ENABLED
                    Log.d(TAG, "SERVICE_STATE_ENABLED isStopedRecognition = "+isStopedRecognition);
                    if (isOpenTrigger && mEventStatus == 1) {
                        try {
                            if (!isStopedRecognition){
                                startRecognition();
                            }
                        } catch (RemoteException e) {
                            e.printStackTrace();
                        }
                    }
                } else {
                    Log.d(TAG, "SERVICE_STATE_DISABLED");
                }
            }

            @Override
            public void onServiceDied() {
                Log.e(TAG, "onServiceDied");
                // TODO we should restart the device services when this method is called
            }
        };

        public VoiceTriggerAidlInterfaceBinder() {
            int moduleId = 1;
            mVoiceTriggerController = new VoiceTriggerController(moduleId, mStatusListener, new Handler());
            try {
                loadSoundModel(true, getFilesDir().getPath() + File.separator + "SMicTD1.dat");
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }

        @Override
        public boolean isFunctionEnable() throws RemoteException {
            return ContentProviderHelper.isOpenSwitch(getBaseContext());
        }

        @Override
        public void startRecognition() throws RemoteException {
            /**
             * TODO: there is two level to give device, now it is same, if you need different level,
             * you can set them independently
             * @triggerLevel the sensibility of trigger you say, for example , say "hello blue genie"
             * or say "hello blue genie yeah~~",of level high.it will not wakeup
             * @verificationLevel the sensibility to verification of trigger saying form different people
             */

            isStopedRecognition = false;
            byte triggerLevel, verificationLevel;
            if (ContentProviderHelper.isDefaultMode(getBaseContext())) {
                triggerLevel = 100;
                verificationLevel = (byte) ContentProviderHelper.getSensibilityValue(getBaseContext());
            } else {
                triggerLevel = (byte) ContentProviderHelper.getSensibilityValue(getBaseContext());
                verificationLevel = triggerLevel;
            }

            byte data[] = {triggerLevel, verificationLevel};
            mVoiceTriggerController.startRecognition(data);
            Log.i(TAG, "startRecognition: set sensibilityValue = " + data[0]);
        }

        @Override
        public void stopRecognition() throws RemoteException {
            isStopedRecognition = true;
            mVoiceTriggerController.stopRecognition();
        }

        @Override
        public void loadSoundModel(boolean isFirstLoad, String dataDir) throws RemoteException {
            mVoiceTriggerController.loadSoundModel(isFirstLoad, dataDir);
        }
    }
}
