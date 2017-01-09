
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
    private static boolean mIsUseBrcmFmChip = SystemProperties.getBoolean(
            "ro.fm.chip.port.UART.androidm", false);

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

    /*
     * public int searchStation(int freq, SearchDirection direction, int timeout) { if
     * (mIsUseBrcmFmChip) { return mFmManagerForBrcm.searchStation(freq,
     * convertDirectionForBrcm(direction), timeout); } else { return
     * -1;//mFmManager.searchStation(freq, convertDirectionForSprd(direction), timeout); } } private
     * FmSearchDirectionForBrcm convertDirectionForBrcm(SearchDirection direction) { if (direction
     * == SearchDirection.FM_SEARCH_UP) { return FmSearchDirectionForBrcm.FM_SEARCH_UP; } else {
     * return FmSearchDirectionForBrcm.FM_SEARCH_DOWN; } }
     */
    /*
     * private FmSearchDirection convertDirectionForSprd(SearchDirection direction) { if (direction
     * == SearchDirection.FM_SEARCH_UP) { return FmSearchDirection.FM_SEARCH_UP; } else { return
     * FmSearchDirection.FM_SEARCH_DOWN; } }
     */

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

    public boolean stopScan() {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.cancelSearch();
        } else {
            return FmNative.stopScan();
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

    public float seekStation(float frequency, boolean isUp) {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.searchStation(frequency, isUp);
        } else {
            return FmNative.seek(frequency, isUp);
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
            return -1;// mFmManager.getFreq();
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
                AudioSystem.setDeviceConnectionState(AudioManager.DEVICE_OUT_FM_HEADSET,
                        AudioSystem.DEVICE_STATE_AVAILABLE, "", "");
            } else {
                AudioSystem.setDeviceConnectionState(AudioManager.DEVICE_OUT_FM_HEADSET,
                        AudioSystem.DEVICE_STATE_UNAVAILABLE, "", "");
            }
            return mFmManagerForBrcm.setAudioPath(convertAudioPathForBrcm(path));
        } else {
            if (enable) {
                AudioSystem.setDeviceConnectionState(AudioManager.DEVICE_OUT_FM_HEADSET,
                        AudioSystem.DEVICE_STATE_AVAILABLE, "", "");
                return true;
            } else {
                AudioSystem.setDeviceConnectionState(AudioManager.DEVICE_OUT_FM_HEADSET,
                        AudioSystem.DEVICE_STATE_UNAVAILABLE, "", "");
                return true;
            }

        }
    }

    public boolean setSpeakerEnable(AudioPath path, boolean isSpeaker) {
        if (mIsUseBrcmFmChip) {
            if (isSpeaker) {
                AudioSystem.setForceUse(AudioSystem.FOR_FM, AudioSystem.FORCE_SPEAKER);
            } else {
                AudioSystem.setForceUse(AudioSystem.FOR_FM, AudioSystem.FORCE_NONE);
            }
            return mFmManagerForBrcm.setAudioPath(convertAudioPathForBrcm(path));
        } else {
            if (isSpeaker) {
                AudioSystem.setForceUse(AudioSystem.FOR_FM, AudioSystem.FORCE_SPEAKER);
            } else {
                AudioSystem.setForceUse(AudioSystem.FOR_FM, AudioSystem.FORCE_NONE);
            }
            return true;
        }
    }

    // add begain for new feature RDS bug-448080
    public int setRdsMode(boolean rdsMode, boolean enableAf) {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.setRdsMode(rdsMode, FmUtils.isAFOpen(mContext));
        } else {
            return FmNative.setRds(rdsMode);
        }
    }

    public int setAfMode(boolean enableAf) {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.setRdsMode(FmUtils.isRDSOpen(mContext), enableAf);
        } else {
            return FmNative.setRds(enableAf);
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
            return AudioManager.DEVICE_OUT_FM_HEADSET;
        } else if (path == AudioPath.FM_AUDIO_PATH_NONE) {
            return AudioManager.DEVICE_OUT_FM_HEADSET;
        } else {
            return AudioManager.DEVICE_OUT_FM_HEADSET;
        }
    }

    public boolean isFmOn() {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.isFmOn();
        } else {
            return false;// mFmManager.isFmOn();
        }
    }

    public boolean setStepType(StepType type) {
        if (mIsUseBrcmFmChip) {
            return mFmManagerForBrcm.setStepType(convertStepTypeForBrcm(type));
        } else {
            return false;// mFmManager.setStepType(convertStepTypeForSprd(type));
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

    /*
     * private FmStepType convertStepTypeForSprd(StepType type) { if (type ==
     * StepType.FM_STEP_50KHZ) { return FmStepType.FM_STEP_50KHZ; } else if (type ==
     * StepType.FM_STEP_100KHZ) { return FmStepType.FM_STEP_100KHZ; } else { return
     * FmStepType.FM_STEP_UNKNOWN; } }
     */
}
