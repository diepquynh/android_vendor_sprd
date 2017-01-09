
package com.android.fmradio;

import com.android.fmradio.FmConstants.*;
import com.android.fmradio.FmConstantsForBrcm.*;

import android.util.Log;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Message;
import android.os.SystemProperties;
import android.content.Context;
import android.media.AudioSystem;
import com.broadcom.fm.fmreceiver.FmProxy;

public class FmManagerSelect {
    private final Context mContext;
    private FmManagerForBrcm mFmManagerForBrcm = null;

    private static final String LOGTAG = "FmManagerSelect";
    // add for universe_ui_support
    private static boolean mIsUseBrcmFmChip = SystemProperties.getBoolean("ro.fm.chip.port.UART.androidm", false);

    public FmManagerSelect(Context context) {
        mContext = context;
        Log.d(LOGTAG, "FmManagerSelect mIsUseBrcmFmChip = " + mIsUseBrcmFmChip);
        if (mIsUseBrcmFmChip) {
            mFmManagerForBrcm = new FmManagerForBrcm(mContext);
        }
    }

    public boolean powerUp(float frequency) {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.powerUp();
        } else {
            return FmNative.powerUp(frequency);
        }
    }

    public boolean powerDown() {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.powerDown();
        } else {
            return FmNative.powerDown(0);
        }
    }

    public boolean openDev() {
        if (mIsUseBrcmFmChip) {
            return true;
        } else {
            return FmNative.openDev();
        }
    }

    public boolean closeDev() {
        if (mIsUseBrcmFmChip) {
            return true;
        } else {
            return FmNative.closeDev();
        }
    }

    public int[] autoScan(int start_freq) {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.autoScanStation(start_freq);
        } else {
            short[] stationsInShort = null;
            int[] stations = null;
            stationsInShort = FmNative.autoScan(start_freq);
            if (null != stationsInShort) {
                int size = stationsInShort.length;
                stations = new int[size];
                for (int i = 0; i < size; i++) {
                    stations[i] = stationsInShort[i];
                }
            }
            return stations;
        }
    }

    public boolean tuneRadio(float frequency) {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.setFreq(frequency);
        } else {
            return FmNative.tune(frequency);
        }
    }

    public boolean tuneRadioAgain(float frequency) {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.setFreq(frequency);
        } else {
            return true;
        }
    }

    public int getFreq() {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.getFreq();
        } else {
            return -1;//mFmManager.getFreq();
        }
    }

    public boolean setVolume(int volume) {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.setVolume(volume);
        } else {
            return false;
        }
    }

    public int setMute(boolean isMute) {
        if (mIsUseBrcmFmChip) {
            mFmManagerForBrcm.muteAudio(isMute);
            return 1;
        } else {
            return FmNative.setMute(isMute);
        }
    }

    public boolean setAudioPathEnable(AudioPath path, boolean enable) {
        if (mIsUseBrcmFmChip) {
            if (enable) {
                mFmManagerForBrcm.setAudioMode(FmAudioModeForBrcm.FM_AUDIO_MODE_BLEND);
                AudioSystem.setDeviceConnectionState(FmConstants.DEVICE_OUT_FM_HEADSET, AudioSystem.DEVICE_STATE_AVAILABLE, "" ,"");
            } else {
                AudioSystem.setDeviceConnectionState(FmConstants.DEVICE_OUT_FM_HEADSET, AudioSystem.DEVICE_STATE_UNAVAILABLE, "" ,"");
            }
            return mFmManagerForBrcm.setAudioPath(convertAudioPathForBrcm(path));
        } else {
            if (enable) {
                Log.d("FmManagerSelect", "setAudioPathEnable setDeviceConnectionState + FmConstants.DEVICE_OUT_FM_HEADSET=" + FmConstants.DEVICE_OUT_FM_HEADSET);
                AudioSystem.setDeviceConnectionState(FmConstants.DEVICE_OUT_FM_HEADSET, AudioSystem.DEVICE_STATE_AVAILABLE, "" ,"");
                return true;
            } else {
                AudioSystem.setDeviceConnectionState(FmConstants.DEVICE_OUT_FM_HEADSET, AudioSystem.DEVICE_STATE_UNAVAILABLE, "" ,"");
                return true;
            }

        }
    }

    public boolean setSpeakerEnable(AudioPath path, boolean isSpeaker) {
        if (mIsUseBrcmFmChip) {
            if (isSpeaker) {
                AudioSystem.setForceUse(FmConstants.FOR_FM, AudioSystem.FORCE_SPEAKER);
            } else {
                AudioSystem.setForceUse(FmConstants.FOR_FM, AudioSystem.FORCE_NONE);
            }
            return mFmManagerForBrcm.setAudioPath(convertAudioPathForBrcm(path));
        } else {
            if (isSpeaker) {
                AudioSystem.setForceUse(FmConstants.FOR_FM, AudioSystem.FORCE_SPEAKER);
            } else {
                AudioSystem.setForceUse(FmConstants.FOR_FM, AudioSystem.FORCE_NONE);
            }
            return true;
        }
    }

    // add begain for new feature RDS bug-448080
    public int setRdsMode(boolean rdsMode, boolean enableAf) {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.setRdsMode(rdsMode, enableAf);
        } else {
            return FmNative.setRds(rdsMode);
        }
    }

    public int isRdsSupported() {
        if (mIsUseBrcmFmChip) {
            return 1;
        } else {
            return FmNative.isRdsSupport();
        }
    }
    // add end;

    //begin 562936 add by suyan.yang 2016.05.14
    public String getFmBler(){
        if (mIsUseBrcmFmChip) {
            Log.d(LOGTAG,"rds bler value is:"+FmBrcmUtil.bler);
            if(FmBrcmUtil.bler == -1){
                return "getting...";
            }
            return String.valueOf(FmBrcmUtil.bler)+"%";
        } else {
            return String.valueOf(FmNative.getBler())+"%";
        }
    }
    //end 562936 add by suyan.yang 2016.05.14

    private int convertAudioPathForBrcm(AudioPath path) {
        if (path == AudioPath.FM_AUDIO_PATH_HEADSET) {
            return FmProxy.AUDIO_PATH_WIRE_HEADSET;
        } else if (path == AudioPath.FM_AUDIO_PATH_SPEAKER) {
            return FmProxy.AUDIO_PATH_SPEAKER;
        } else if (path == AudioPath.FM_AUDIO_PATH_NONE) {
            return FmProxy.AUDIO_PATH_NONE;
        } else {
            return FmProxy.AUDIO_PATH_NONE;
        }
    }

    private int convertAudioPathForSprd(AudioPath path) {
        if (path == AudioPath.FM_AUDIO_PATH_HEADSET) {
            return FmConstants.DEVICE_OUT_FM_HEADSET;
        } else if (path == AudioPath.FM_AUDIO_PATH_NONE) {
            return FmConstants.DEVICE_OUT_FM_HEADSET;
        } else {
            return FmConstants.DEVICE_OUT_FM_HEADSET;
        }
    }

    public boolean isFmOn() {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.isFmOn();
        } else {
            return false;//mFmManager.isFmOn();
        }
    }

    public boolean setStepType(StepType type) {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.setStepType(convertStepTypeForBrcm(type));
        } else {
            return false;//mFmManager.setStepType(convertStepTypeForSprd(type));
        }
    }

    private FmStepTypeForBrcm convertStepTypeForBrcm(StepType type) {
        if (type == StepType.FM_STEP_50KHZ) {
            return FmStepTypeForBrcm.FM_STEP_50KHZ;
        } else if (type == StepType.FM_STEP_100KHZ) {
            return FmStepTypeForBrcm.FM_STEP_100KHZ;
        } else {
            return FmStepTypeForBrcm.FM_STEP_UNKNOWN;
        }
    }

    public boolean isUseBrcmFmChip() {
        return mIsUseBrcmFmChip;
    }

    /*private FmStepType convertStepTypeForSprd(StepType type) {
        if (type == StepType.FM_STEP_50KHZ) {
            return FmStepType.FM_STEP_50KHZ;
        } else if (type == StepType.FM_STEP_100KHZ) {
            return FmStepType.FM_STEP_100KHZ;
        } else {
            return FmStepType.FM_STEP_UNKNOWN;
        }
    }*/
}
