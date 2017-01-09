// VoiceTriggerAidlInterface.aidl
package com.sprd.voicetrigger.aidl;

interface VoiceTriggerAidlService {
    boolean isFunctionEnable();
    void startRecognition();
    void stopRecognition();
    void loadSoundModel(boolean isFirstLoad, String dataDir);
}
