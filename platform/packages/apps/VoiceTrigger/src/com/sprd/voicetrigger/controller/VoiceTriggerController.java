
package com.sprd.voicetrigger.controller;

import java.io.IOException;
import java.util.UUID;

import com.sprd.voicetrigger.utils.DataCreaterUtil;

import android.hardware.soundtrigger.SoundTrigger.StatusListener;
import android.hardware.soundtrigger.SoundTrigger;
import android.hardware.soundtrigger.SoundTrigger.RecognitionConfig;
import android.hardware.soundtrigger.SoundTriggerModule;
import android.hardware.soundtrigger.SoundTrigger.RecognitionEvent;
import android.hardware.soundtrigger.SoundTrigger.SoundModelEvent;
import android.hardware.soundtrigger.SoundTrigger.SoundModel;
import android.hardware.soundtrigger.SoundTrigger.ConfidenceLevel;
import android.hardware.soundtrigger.SoundTrigger.KeyphraseRecognitionEvent;
import android.hardware.soundtrigger.SoundTrigger.KeyphraseRecognitionExtra;
import android.hardware.soundtrigger.SoundTrigger.KeyphraseSoundModel;
import android.os.Handler;
import android.util.Log;

public class VoiceTriggerController {
    private static final String TAG = "VoiceTriggerController";
    private SoundTriggerModule mSoundTriggerModule;
    public final UUID mUUIDInstance1 = UUID.fromString("119341a0-8469-11df-81f9-0012a5d5c51b");
    public final UUID mUUIDInstance2 = UUID.fromString("119341a0-8469-11df-81f9-0002a5d5c51b");
    private SoundTrigger.RecognitionConfig mRecognitionConfig;
    private SoundTrigger.KeyphraseRecognitionExtra[] recognitionExtra;
    private static int soundModelHandle[] = {
            1
    };

    public VoiceTriggerController(int moduleId, SoundTrigger.StatusListener mStatusListener,
                                   Handler mHandler) {
        Log.d(TAG, "create VoiceTriggerController");
        mSoundTriggerModule = SoundTrigger.attachModule(moduleId, mStatusListener, mHandler);
    }

    public void stopRecognition() {
        Log.d(TAG, "stopRecognition()");
        mSoundTriggerModule.stopRecognition(soundModelHandle[0]);
    }

    public void startRecognition(byte[] data) {
        Log.d(TAG, "startRecognition()");
        recognitionExtra = new SoundTrigger.KeyphraseRecognitionExtra[1];
        recognitionExtra[0] = new SoundTrigger.KeyphraseRecognitionExtra(1, 2, 0,
                new ConfidenceLevel[0]);
        mRecognitionConfig = new SoundTrigger.RecognitionConfig(true, true, recognitionExtra, data);
        mSoundTriggerModule.startRecognition(soundModelHandle[0], mRecognitionConfig);
    }

    public boolean loadSoundModel(boolean isFirstLoad, String dataDir) {
        Log.d(TAG, "loadSoundModel start");
        if(!isFirstLoad){
            stopRecognition();
            unloadSoundModel();
        }
        try {
            byte[] mGeneratedDataByte = DataCreaterUtil.getByteArrayFromFileDir(dataDir);
            SoundTrigger.SoundModel mSoundModel = new SoundTrigger.SoundModel(mUUIDInstance1,
                    mUUIDInstance2, SoundTrigger.SoundModel.TYPE_KEYPHRASE, mGeneratedDataByte);
            mSoundTriggerModule.loadSoundModel(mSoundModel, soundModelHandle);
            Log.d(TAG, "loadSoundModel successful");
            return true;
        }catch (IOException e){
            e.printStackTrace();
            Log.d(TAG, "loadSoundModel failed");
            return false;
        }
    }

    public void unloadSoundModel() {
        Log.d(TAG, "unloadSoundModel()");
        mSoundTriggerModule.unloadSoundModel(soundModelHandle[0]);
    }
}
