
package com.android.fmradio;

import java.util.ArrayList;

import com.broadcom.fm.fmreceiver.FmProxy;
import com.broadcom.fm.fmreceiver.IFmReceiverEventHandler;
import com.broadcom.fm.fmreceiver.IFmProxyCallback;
import android.content.Context;
import android.util.Log;

import com.android.fmradio.FmConstantsForBrcm.*;

public class FmManagerForBrcm implements IFmProxyCallback {
    private Object mLock = new Object();
    private Object mSearchLock = new Object();
    private Object mVolumeSetLock = new Object();
    private Object mFmProxyInitLock = new Object();
    private Object mRdsModeLock = new Object();

    private static final String TAG = "FmManagerForBrcm";
    private static final boolean DBG = true;
    private static int[] mVolumeTbl = {
            0, 1, 2, 4, 8, 16, 32, 48, 64, 80, 108, 128, 150, 180, 210, 255
    };
    private boolean mPowerOnPending = false;
    private boolean mPowerOffPending = false;
    private boolean mSetFreqPending = false;
    private boolean mSearchStationPending = false;
    private boolean mCancelSearchPending = false;
    private boolean mSetVolumePending = false;
    private boolean mValidStation = false;
    private boolean mFmReceiverIsReady = false;
    private boolean mMuteAudioPending = false;
    private boolean mSetAudioPathPending = false;
    private boolean mIsRadioAfJump = false;

    private FmProxy mFmReceiver;
    private FmReceiverEventHandler mFmReceiverEventHandler;

    private final Context mContext;
    FmBrcmUtil mFmUtil = null;

    private int mNfl = -1;

    private FmBandForBrcm mWorldRegion = FmBandForBrcm.FM_BAND_UNKNOWN;
    private FmStepTypeForBrcm mStepType = FmStepTypeForBrcm.FM_STEP_100KHZ;
    private FmAudioModeForBrcm mModeType = FmAudioModeForBrcm.FM_AUDIO_MODE_UNKNOWN;

    private int mMinFreq = 87500;
    private int mMaxFreq = 108000;
    private long mLastOffTime = 0;

    private float mSeekFreqClbk = (float) 8750 / 100;

    public FmManagerForBrcm(Context context) {
        mContext = context;
    }

    public void init() {
        if (mFmReceiver == null) {
            synchronized (mFmProxyInitLock) {
                if (FmProxy.getProxy(mContext, this)) {
                    try {
                        mFmProxyInitLock.wait(5000);
                    } catch (InterruptedException e) {
                        Log.e(TAG, "Interrupted Exception in init()");
                    }
                }
            }
            mFmUtil = FmBrcmUtil.getUtil();
        }
    }

    @Override
    public void onProxyAvailable(Object ProxyObject) {
        if (mFmReceiver == null) {
            mFmReceiver = (FmProxy) ProxyObject;
        }
        if (mFmReceiver == null) {
            Log.e(TAG, "Get Proxy Error");
        }
       if(mFmReceiverEventHandler == null){
           mFmReceiverEventHandler = new FmReceiverEventHandler();
       }
        if (null != mFmReceiver) {
            mFmReceiver.registerEventHandler(mFmReceiverEventHandler);
            mFmReceiverIsReady = true;
        }
        synchronized (mFmProxyInitLock) {
            mFmProxyInitLock.notify();
        }
    }

    /**
     * proxy is Unavailable. mostly likely because FM service crashed in this
     * case we need to force the call to onDestroy() to avoid sending any Fm
     * commands to the proxy.
     */
    public void onProxyUnAvailable() {
        Log.e(TAG, "onProxyUnAvailable");
    }

    public void finish() {
        if (mFmReceiver != null && mFmReceiverIsReady) {
            if (DBG) {
                Log.d(TAG, "Finishing FmProxy proxy...");
            }
            mFmReceiverIsReady = false;
            mFmReceiver.finish();
            mFmReceiver = null;
            mFmReceiverEventHandler = null;
        }
    }

    protected class FmReceiverEventHandler implements IFmReceiverEventHandler {
        @Override
        public void onAudioModeEvent(int audioMode) {
            if (DBG) {
                Log.d(TAG, "onAudioModeEvent(" + audioMode + ")");
            }
            synchronized (mLock) {
                mLock.notify();
            }
        }

        @Override
        public void onAudioPathEvent(int audioPath) {
            if (DBG) {
                Log.d(TAG, "onAudioPathEvent(" + audioPath + ")");
            }
            synchronized (mLock) {
                mSetAudioPathPending = false;
                mLock.notify();
            }
        }

        @Override
        public void onEstimateNoiseFloorLevelEvent(int nfl) {
            if (DBG) {
                Log.d(TAG, "onEstimateNoiseFloorLevelEvent(" + nfl + ")");
            }
            mNfl = nfl;
            synchronized (mLock) {
                mLock.notify();
            }
        }

        @Override
        public void onLiveAudioQualityEvent(int rssi, int snr) {
            if (DBG) {
                Log.d(TAG, "onLiveAudioQualityEvent(" + rssi + ", " + snr + " )");
            }
            synchronized (mLock) {
                mLock.notify();
            }
        }

        @Override
        public void onRdsDataEvent(int rdsDataType, int rdsIndex,
                String rdsText) {
            if (DBG) {
                Log.d(TAG, "onRdsDataEvent(" + rdsDataType + ", " + rdsIndex + ", " + rdsText
                        + ")");
            }

            if (rdsDataType == FmBrcmUtil.FM_RDS_DATA_PTY_UPDATE) {
                if (rdsIndex < 0 || rdsIndex >= FmConstantsForBrcm.PTY_LIST.length) {
                    Log.e(TAG, "onRdsDataEvent illegal param!");
                    return;
                }
                //rdsText = FmConstantsForBrcm.PTY_LIST[rdsIndex];
            }
            mFmUtil.setRdsDataType(rdsDataType);
            mFmUtil.fmSignal2Info(FmBrcmUtil.FM_NOTIFY_RDS_DATA, rdsIndex, rdsText);

        }

        @Override
        public void onRdsModeEvent(int rdsMode, int alternateFreqHopEnabled) {
            if (DBG) {
                Log.d(TAG, "onRdsModeEvent(" + rdsMode + ", " + alternateFreqHopEnabled + ")");
            }

            synchronized (mRdsModeLock) {
                mRdsModeLock.notify();
            }
            String afStateMode = "" + alternateFreqHopEnabled;
            mFmUtil.fmSignal2Info(FmBrcmUtil.FM_NOTIFY_RDS_AF_STATE, rdsMode, afStateMode);
        }

        @Override
        public void onSeekCompleteEvent(int freq, int rssi,
                int snr, boolean seeksuccess) {
            if (DBG) {
                Log.d(TAG, "onSeekCompleteEvent(" + freq + ", " + rssi + ", " + snr + ", "
                        + seeksuccess + ")");
            }
            // add begain for new feature RDS bug-448080
            if (mSearchStationPending || mCancelSearchPending) {
                mIsRadioAfJump = false;
            } else {
                Log.d(TAG, "onSeekCompleteEvent(" + freq + ", " + rssi + ", " + snr + ", "
                        + seeksuccess + ")" + "RadioAfJump");
                mIsRadioAfJump = true;
                //mFmUtil.fmSignal2Info(FmBrcmUtil.FM_NOTIFY_AF_JUMP, freq, "");
            }
            // add end;
            synchronized (mSearchLock) {
                mValidStation = seeksuccess;
                if (seeksuccess) {
                    mSeekFreqClbk = (float) freq / 100;
                }
                mSearchStationPending = false;
                mCancelSearchPending = false;
                mSearchLock.notify();
            }
        }

        @Override
        public void onStatusEvent(int freq, int rssi, int snr, boolean radioIsOn,
                int rdsProgramType, String rdsProgramService,
                String rdsRadioText, String rdsProgramTypeName, boolean isMute) {
            if (DBG) {
                Log.d(TAG, "onStatusEvent(" + freq + ", " + rssi + ", " + snr + ", "
                        + radioIsOn + ", " + rdsProgramType + ", " + rdsProgramService
                        + ", " + rdsRadioText + ", " + rdsProgramTypeName + ", " + isMute + ")");
            }
            synchronized (mLock) {
                if (radioIsOn) {
                    mPowerOnPending = false;
                } else {
                    mPowerOffPending = false;
                    mLastOffTime = System.currentTimeMillis();
                }
                mSetFreqPending = false;
                mMuteAudioPending = false;

                //begin 562936 add by suyan.yang 2016.05.19
                Log.d(TAG,"the bler value is :"+(snr-1000));
                if(snr>=1000){
                    mFmUtil.setRdsBler(snr-1000);
                }
                //end 562936 add by suyan.yang 2016.05.19
                mLock.notify();
            }
            synchronized (mVolumeSetLock) {
                if (mSetVolumePending) {
                    mVolumeSetLock.notify();
                }
            }
        }

        @Override
        public void onWorldRegionEvent(int worldRegion) {
            if (DBG) {
                Log.d(TAG, "onWorldRegionEvent(" + worldRegion + ")");
            }
            mWorldRegion = FmBandForBrcm.values()[worldRegion];
            updateMinMaxFrequencies();
            synchronized (mLock) {
                mLock.notify();
            }
        }

        @Override
        public void onVolumeEvent(int status, int volume) {
            if (DBG) {
                Log.d(TAG, "onVolumeEvent(" + status + ", " + volume + ")");
            }
            synchronized (mVolumeSetLock) {
                mSetVolumePending = false;
                mVolumeSetLock.notify();
            }
        }
    }

    public boolean powerUp() {
        boolean result = false;
        int status = 1;
        long now = System.currentTimeMillis();

        if ((now -mLastOffTime) < 300) {
            Log.d(TAG, "300 powerup_time diff:" + (now -mLastOffTime) );
            try {
                Thread.sleep(500);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        init();
        if (mFmReceiver != null && mFmReceiverIsReady) {
            synchronized (mLock) {
                mPowerOnPending = true;
                if (FmConstantsForBrcm.FM_SOFTMUTE_FEATURE_ENABLED) {
                    status = mFmReceiver.turnOnRadio(FmProxy.FUNC_REGION_NA | FmProxy.FUNC_RBDS
                            | FmProxy.FUNC_AF | FmProxy.FUNC_SOFTMUTE, mContext.getPackageName());
                } else {
                    status = mFmReceiver.turnOnRadio(FmProxy.FUNC_REGION_NA | FmProxy.FUNC_RBDS
                            | FmProxy.FUNC_AF, mContext.getPackageName());
                }
                if (DBG) {
                    Log.d(TAG, "turnOnRadio status = " + status);
                }
                if (status == FmProxy.STATUS_OK) {
                    Log.d(TAG, "turnOnRadio1");
                    try {
                        if (mPowerOnPending) {
                            Log.d(TAG, "turnOnRadio2");
                            mLock.wait(5000);
                        }
                        Log.d(TAG, "turnOnRadio after status = " + status);
                        if (!mPowerOnPending) {
                            setStepType(mStepType);
                            return true;
                        }
                    } catch (InterruptedException e) {
                        Log.e(TAG, "Interrupted Exception in powerUp()");
                    }
                }
            }
            if (!result) {
                Log.d(TAG, "powerUp fail");
                finish();
            }
        }
        return result;
    }

    public boolean powerDown() {
        boolean result = false;
        int status = 1;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            synchronized (mLock) {
                mPowerOffPending = true;
                status = mFmReceiver.turnOffRadio();
                if (DBG) {
                    Log.d(TAG, "powerDown result = " + status);
                }
                if (status == FmProxy.STATUS_OK) {
                    try {
                        if (mPowerOffPending) {
                            mLock.wait(5000);
                        }
                        if (!mPowerOffPending) {
                            finish();
                            return true;
                        }
                    } catch (InterruptedException e) {
                        Log.e(TAG, "Interrupted Exception in powerDown()");
                    }
                }
            }
        }
        finish();
        return result;
    }

    public int[] autoScanStation(int start_freq) {

        boolean mStopSearching = false;
        int[] stations= null;
        float freq = FmUtils.computeFrequency(start_freq);
        float prev = 0.0f;
        ArrayList<Integer> intStation = new ArrayList<Integer>();
        while (!mStopSearching && freq <= FmUtils.getHighestFrequency()) {
            freq = searchStation(freq, true);
            if (freq != prev  && prev < freq) {
                Log.d(TAG, "result freq=" + freq);
                prev = freq;
                intStation.add(FmUtils.computeStation(freq));
            }
            if (freq < 0 || prev > freq) {
                break;
            }
        }
        short[] stationsInShort = null;
        stations = new int[intStation.size()];
        for (int i = 0; i < intStation.size(); i++) {
            stations[i] = intStation.get(i);
        }
        if (stations.length == 0) {
            stations = new int[] {
                    -100
            };
        }
        return stations;
    }

    public float searchStation(float frequence, boolean isUp) {
        int status = 1;
        int freq = (int)(frequence * 1000);
        int endFrequency = freq;
        int mFrequencyStep = getScanFreqStep();
        int searchDirection = FmProxy.SCAN_MODE_DOWN;
        if (isUp) {
            searchDirection = FmProxy.SCAN_MODE_UP;
            /* Increase the current listening frequency by one step. */
            if ((mMinFreq <= freq) && (freq <= mMaxFreq)) {
                freq = (freq + mFrequencyStep);
                if (freq > mMaxFreq) {
                    freq = mMinFreq;
                }
            } else {
                freq = mMinFreq;
                endFrequency = mMaxFreq;
            }
        } else {
            searchDirection = FmProxy.SCAN_MODE_DOWN;
            if ((mMinFreq <= freq) && (freq <= mMaxFreq)) {
                freq = (freq - mFrequencyStep);
                if (freq < mMinFreq) {
                    freq = mMaxFreq;
                }
            } else {
                freq = mMaxFreq;
                endFrequency = mMinFreq;
            }
        }
        // covert freq to meet brcm 87500->8750
        freq = freq / 10;
        endFrequency = endFrequency / 10;
        if (DBG) {
            Log.d(TAG, "seekStationCombo freq = " + freq + " endFrequency = " + endFrequency);
        }
        if (mFmReceiver != null && mFmReceiverIsReady) {
            synchronized (mSearchLock) {
                mSearchStationPending = true;
                mValidStation = false;
                status = mFmReceiver.seekStationCombo(
                        freq,
                        endFrequency,
                        FmProxy.MIN_SIGNAL_STRENGTH_DEFAULT,
                        searchDirection,
                        FmProxy.SCAN_MODE_NORMAL,
                        FmConstantsForBrcm.COMBO_SEARCH_MULTI_CHANNEL_DEFAULT,
                        FmProxy.RDS_COND_NONE,
                        FmProxy.RDS_COND_PTY_VAL);
                if (DBG) {
                    Log.d(TAG, "seekStationCombo status = " + status);
                }
                if (status == FmProxy.STATUS_OK) {
                    try {
                        if (mSearchStationPending) {
                            mSearchLock.wait(5000);
                        }
                        if (!mSearchStationPending) {
                            if (mValidStation) {
                                Log.d(TAG, "mSeekFreqClbk freq = " + mSeekFreqClbk);
                                return mSeekFreqClbk;
                            } else {
                                return -mSeekFreqClbk;
                            }
                        } else {
                            return -mSeekFreqClbk;
                        }
                    } catch (InterruptedException e) {
                        Log.e(TAG, "Interrupted Exception in searchStation()");
                    }
                }
            }
        }
        return -mSeekFreqClbk;
    }

    public boolean cancelSearch() {
        boolean result = false;
        int status = 1;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            synchronized (mSearchLock) {
                mCancelSearchPending = true;
                status = mFmReceiver.seekStationAbort();
                if (DBG) {
                    Log.d(TAG, "seekStationAbort status = " + status);
                }
                if (status == FmProxy.STATUS_OK) {
                    try {
                        if (mCancelSearchPending) {
                            mSearchLock.wait(3000);
                        }
                        if (!mCancelSearchPending) {
                            return true;
                        }
                    } catch (InterruptedException e) {
                        Log.e(TAG, "Interrupted Exception in seekStationAbort()");
                    }
                }
            }
        }
        return result;
    }

    public boolean setFreq(float freq) {
        boolean result = false;
        int status = 1;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            synchronized (mLock) {
                mSetFreqPending = true;
                status = mFmReceiver.tuneRadio((int)(freq * 1000) / 10);
                if (DBG) {
                    Log.d(TAG, "tuneRadio status = " + status);
                }
                if (status == FmProxy.STATUS_OK) {
                    try {
                        if (mSetFreqPending) {
                            mLock.wait(3000);
                        }
                        if (!mSetFreqPending) {
                            return true;
                        }
                    } catch (InterruptedException e) {
                        Log.e(TAG, "Interrupted Exception in setFreq()");
                    }
                }
            }
        }
        return result;
    }

    public int getFreq() {
        int result = -1;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            result = mFmReceiver.getTunedFrequency();
            result = result * 10; // covert freq 8750->87500
            if (DBG) {
                Log.d(TAG, "getFreq result = " + result);
            }
        }
        return result;
    }

    private int convertVolume(int volume) {
        if ((volume >= FmConstantsForBrcm.MIN_VOLUME && volume <= FmConstantsForBrcm.MAX_VOLUME)) {
            volume = mVolumeTbl[volume];
        } else {
            volume = -1;
        }
        return volume;
    }

    public boolean setVolume(int volume) {
        boolean result = false;
        int status = 1;
        volume = convertVolume(volume);
        if (volume < 0) {
            Log.e(TAG, "setVolume illegal parameter");
            return result;
        }
        if (mFmReceiver != null && mFmReceiverIsReady) {
            synchronized (mVolumeSetLock) {
                mSetVolumePending = true;
                status = mFmReceiver.setFMVolume(volume);
                if (DBG) {
                    Log.d(TAG, "setVolume status = " + status + " volume = " + volume);
                }
                if (status == FmProxy.STATUS_OK) {
                    try {
                        if (mSetVolumePending) {
                            mVolumeSetLock.wait(3000);
                        }
                        if (!mSetVolumePending) {
                            mSetVolumePending = false;
                            return true;
                        }
                    } catch (InterruptedException e) {
                        Log.e(TAG, "Interrupted Exception in setVolume()");
                    }
                }
            }
        }
        mSetVolumePending = false;
        return result;
    }

    public int muteAudio(boolean mute) {
        int status = 0;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            synchronized (mLock) {
                mMuteAudioPending = true;
                status = mFmReceiver.muteAudio(mute);
                if (DBG) {
                    Log.d(TAG, "muteAudio status = " + status + " mute = " + mute);
                }
                if (status == FmProxy.STATUS_OK) {
                    try {
                        if(mMuteAudioPending){
                        mLock.wait(100);
                        }
                        if (!mMuteAudioPending) {
                            return status;
                        }
                    } catch (InterruptedException e) {
                        Log.e(TAG, "Interrupted Exception in muteAudio()");
                    }
                }
            }
        }
        return status;
    }

    public boolean setAudioPath(int path) {
        boolean result = false;
        int status = 1;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            synchronized (mLock) {
                mSetAudioPathPending = true;
                status = mFmReceiver.setAudioPath(path);
                if (DBG) {
                    Log.d(TAG, "setAudioPath status = " + status + " path = " + path);
                }
                if (status == FmProxy.STATUS_OK) {
                    try {
                        if (mSetAudioPathPending) {
                            mLock.wait(2000);
                        }
                        if (!mSetAudioPathPending) {
                            return true;
                        }
                    } catch (InterruptedException e) {
                        Log.e(TAG, "Interrupted Exception in setAudioPath");
                    }

                }
            }
        }
        return result;
    }

    public boolean isFmOn() {
        boolean result = false;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            result = mFmReceiver.getRadioIsOn();
            if (DBG) {
                Log.d(TAG, "isFmOn result = " + result);
            }
        }
        return result;
    }

    /********************* not use in current FM App *********************/

    public boolean setAudioMode(FmAudioModeForBrcm mode) {
        boolean result = false;
        int status = 1;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            status = mFmReceiver.setAudioMode(mode.ordinal());
            if (DBG) {
                Log.d(TAG, "setAudioMode status = " + status + " mode = " + mode);
            }
            if (status == FmProxy.STATUS_OK) {
                mModeType = mode;
                return true;
            }
        }
        return result;
    }

    public FmAudioModeForBrcm getAudioMode() {
        return mModeType;
    }

    public boolean setStepType(FmStepTypeForBrcm type) {
        boolean result = false;
        int status = 1;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            status = mFmReceiver.setStepSize(type.ordinal());
            if (DBG) {
                Log.d(TAG, "setStepType status = " + status + " type = " + type);
            }
            if (status == FmProxy.STATUS_OK) {
                mStepType = type;
                return true;
            }
        }
        return result;
    }

    public FmStepTypeForBrcm getStepType() {
        return mStepType;
    }

    public int getScanFreqStep() {
        int scanStep = 0;
        if (mStepType == FmStepTypeForBrcm.FM_STEP_50KHZ) {
            scanStep = 50;
        } else if (mStepType == FmStepTypeForBrcm.FM_STEP_100KHZ) {
            scanStep = 100;
        } else {
            Log.e(TAG, "mStepType is error StepType!");
        }
        return scanStep;
    }

    public boolean setWorldRegion(FmBandForBrcm band) {
        boolean result = false;
        int status = 1;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            synchronized (mLock) {
                if (band == FmBandForBrcm.FM_BAND_NA) {
                    status = mFmReceiver.setWorldRegion(band.ordinal(), FmProxy.DEEMPHASIS_75U);
                } else if (band != FmBandForBrcm.FM_BAND_UNKNOWN) {
                    status = mFmReceiver.setWorldRegion(band.ordinal(), FmProxy.DEEMPHASIS_50U);
                }
                if (DBG) {
                    Log.d(TAG, "setWorldRegion status = " + status + " band = " + band);
                }
                if (status == FmProxy.STATUS_OK) {
                    try {
                        mLock.wait(3000);
                        return true;
                    } catch (InterruptedException e) {
                        Log.e(TAG, "Interrupted Exception in setWorldRegion()");
                    }
                }
            }
        }
        return result;
    }

    public FmBandForBrcm getWorldRegion() {
        return mWorldRegion;
    }

    private void updateMinMaxFrequencies() {
        switch (mWorldRegion) {
            case FM_BAND_NA: {
                mMinFreq = 87500;
                mMaxFreq = 108000;
            }
                break;
            case FM_BAND_EU: {
                mMinFreq = 87500;
                mMaxFreq = 108000;
            }
                break;
            case FM_BAND_JP_STD: {
                mMinFreq = 76000;
                mMaxFreq = 90000;
            }
                break;
            case FM_BAND_JP_WIDE: {
                mMinFreq = 76000;
                mMaxFreq = 90000;
            }
                break;
            default: {
                mMinFreq = 87500;
                mMaxFreq = 108000;
            }
        }
    }

    public boolean isMuted() {
        boolean result = false;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            result = mFmReceiver.getIsMute();
        }
        if (DBG) {
            Log.d(TAG, "isMuted result = " + result);
        }
        return result;
    }

    /*************************** AF RDS function *****************************/
    public boolean setSnrThreshold(int snrThreshold) {
        boolean result = false;
        int status = 1;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            status = mFmReceiver.setSnrThreshold(snrThreshold);
            if (DBG) {
                Log.d(TAG, "setSnrThreshold status = " + status + " snrThreshold = "
                        + snrThreshold);
            }
            if (status == FmProxy.STATUS_OK) {
                return true;
            }
        }
        return result;
    }

    public boolean getStatus() {
        boolean result = false;
        int status = 1;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            synchronized (mLock) {
                status = mFmReceiver.getStatus();
                if (DBG) {
                    Log.d(TAG, "getStatus status = " + status);
                }
                if (status == FmProxy.STATUS_OK) {
                    try {
                        mLock.wait(3000);
                        return true;
                    } catch (InterruptedException e) {
                        Log.e(TAG, "Interrupted Exception in getStatus()");
                    }
                }
            }
        }
        return result;
    }

    public boolean setLiveAudioPolling(boolean liveAudioPolling, int signalPollInterval) {
        boolean result = false;
        int status = 1;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            status = mFmReceiver.setLiveAudioPolling(liveAudioPolling, signalPollInterval);
            if (DBG) {
                Log.d(TAG, "setLiveAudioPolling status = " + status + " liveAudioPolling = "
                        + liveAudioPolling + " signalPollInterval = " + signalPollInterval);
            }
            if (status == FmProxy.STATUS_OK) {
                return true;
            }
        }
        return result;
    }

    public boolean estimateNoiseFloorLevel(int nflLevel) {
        boolean result = false;
        int status = 1;
        if (mFmReceiver != null && mFmReceiverIsReady) {
            synchronized (mLock) {
                status = mFmReceiver.estimateNoiseFloorLevel(nflLevel);
                if (DBG) {
                    Log.d(TAG, "estimateNoiseFloorLevel status = " + status + " nflLevel = "
                            + nflLevel);
                }
                if (status == FmProxy.STATUS_OK) {
                    try {
                        mLock.wait(3000);
                        return true;
                    } catch (InterruptedException e) {
                        Log.e(TAG, "Interrupted Exception in estimateNoiseFloorLevel()");
                    }
                }
            }
        }
        return result;
    }

    public int getNoiseFloorLevelEvent() {
        if (DBG) {
            Log.d(TAG, "getNoiseFloorLevelEvent mNfl = " + mNfl);
        }
        return mNfl;
    }

    public int setRdsMode(boolean rdsMode, boolean enableAf){
        int status = -1;
        int afMode = 0;
        int rdsModeInt = 2;
        if (enableAf) {
            afMode = 1;
        }
        if (rdsMode) {
            rdsModeInt = 2;
        } else {
            rdsModeInt = 0;
        }
        if (mFmReceiver != null && mFmReceiverIsReady) {
            synchronized (mRdsModeLock) {
                status = mFmReceiver.setRdsMode(rdsModeInt, FmProxy.RDS_FEATURE_PS
                        | FmProxy.RDS_FEATURE_RT | FmProxy.RDS_FEATURE_TP
                        | FmProxy.RDS_FEATURE_PTY | FmProxy.RDS_FEATURE_PTYN, afMode,
                        FmProxy.MIN_SIGNAL_STRENGTH_DEFAULT);
                if (DBG) {
                    Log.d(TAG, "setRdsMode status = " + status + " rdsMode = " + rdsMode
                            + " afMode = " + afMode);
                }
                if (status == FmProxy.STATUS_OK) {
                    try {
                        mRdsModeLock.wait(3000);
                        return status;
                    } catch (InterruptedException e) {
                        Log.e(TAG, "Interrupted Exception in setRdsMode()");
                    }
                }
            }
        }
        return status;
    }

}
