
package com.sprd.voicetrigger.nativeinterface;

import android.util.Log;

import com.sprd.voicetrigger.R;
import com.sprd.voicetrigger.WakeupDescribeActivity;
import com.sprd.voicetrigger.global.RuntimeStates;
import com.sprd.voicetrigger.utils.LangagePackageManager;
import com.sprd.voicetrigger.utils.VocabularyPackageManager;

import java.nio.ByteBuffer;

public class Recog implements Runnable {
    public static String res = null;
    public long trig = 0;
    public boolean resetFlag = false;
    public boolean expired = false;

    private static final String TAG = "recog";
    private boolean halt = false;
    private Audio audioInstance;
    private RecogStatusCallback mRecogStatusCallback;
    private int wakeupWordsMode;

    // Loads the JNI library
    static {
        System.loadLibrary("jni_udtsid");
    }

    public interface RecogStatusCallback {
        void onStatusChanged(int status, String msg);

        void onStatusChanged(int status, int id);
    }

    public Recog(Audio audio, int mode, RecogStatusCallback callback) {
        super();
        Log.d(TAG, "new Recog mode = " + mode);
        audioInstance = audio;
        wakeupWordsMode = mode;
        mRecogStatusCallback = callback;
    }

    /**
     * Initialize the SDK and recognition setup.
     */
    public void initSDK(LangagePackageManager.packClass lp,
                        VocabularyPackageManager.packClass tp,
                        String ftSearchFile,
                        String cacheDir,
                        Boolean requirePause) {
        trig = initSession();
        Log.i(TAG, "initSDK trig = " + trig);
        if (trig == 0) {
            expired = true;
        }
        if (!expired) {
            setPack(lp, tp, ftSearchFile, cacheDir, requirePause);
        }
    }

    private void setPack(LangagePackageManager.packClass lp,
                         VocabularyPackageManager.packClass tp,
                         String ftSearchFile,
                         String cacheDir,
                         Boolean requirePause) {
        Log.d(TAG, "initRecog" + "netFile = " + lp.netFile + "enrollnetFile = " + lp.enrollnetFile
                + "svsidFile = " + lp.svsidFile + "phonemeFile = " + lp.phonemeFile + "paramAStart ="
                + lp.paramAStart + "paramBStart" + lp.paramBStart + "checkSNR = " + lp.checkSNR);
        long result = initRecog(trig, lp.netFile, lp.enrollnetFile, lp.svsidFile, lp.phonemeFile,
                lp.paramAStart, lp.paramBStart, lp.checkSNR,
                tp.netFile, ftSearchFile, tp.antiDataFile,
                tp.paramAOffset, tp.durMinFactor, tp.durMaxFactor, tp.triggerSampPerCat,
                tp.sampPerCatWithin, tp.targetSNR, tp.learnRate, tp.learnRateWithin,
                tp.dropoutWithin, tp.epqMin, tp.useFeat, tp.checkSNR, tp.checkVowelDur,
                lp.svThreshold, lp.svThresholdStep, cacheDir,
                WakeupDescribeActivity.NUMUSERS, WakeupDescribeActivity.NUMENROLL);

        if (result == 0) {
            Log.e(TAG, "InitRecog Failed");
            // TODO : add text to String.xml
            mRecogStatusCallback.onStatusChanged(RuntimeStates.BAIL, "initRecog Failed");
            return;
        }
        configEnroll(trig, (short) (requirePause ? 1 : 0));
    }

    /**
     * Main event loop for the recognition thread. This runs continuously,
     * processing audio buffers as they are made available by the audio thread.
     * When no audio is available the thread blocks. It also cleans up the SDK
     * memory when the thread is halted.
     */
    public void run() {
        ByteBuffer buf;
        int status;
        while ((!halt) && (!expired)) {
            buf = null;
            Log.i(TAG, "run start");
            try {
                // BLOCKS UNTIL AUDIO AVAILABLE
                buf = audioInstance.getBuffer();
            } catch (InterruptedException e) {
                Log.w(TAG, "RecogThread: getBuffer interrupted");
            }
            if (buf == null)
                continue;
            if (resetFlag) {
                resetRecog(trig);
                resetFlag = false;
            }
            if (!audioInstance.isRecording())
                continue;
            // enroll phrase
            status = pipeRecogEnroll(trig, buf,
                    (short) (wakeupWordsMode + 1));
            if (status == 2) {
                mRecogStatusCallback.onStatusChanged(RuntimeStates.STOPREC, R.string.info_please_wait);
                int rcode = getResultEnroll(trig, WakeupDescribeActivity.enrollIdx,
                        (short) (wakeupWordsMode + 1));
                if (rcode == 0) {
                    mRecogStatusCallback.onStatusChanged(RuntimeStates.ADDED_WAVE, null);
                } else {
                    int result = R.string.info_default;
                    if ((rcode & 0x0003) > 0)
                        result = R.string.info_talk_louder;
                    if ((rcode & 0x0004) > 0)
                        result = R.string.info_pause_before;
                    if ((rcode & 0x0010) > 0)
                        result = R.string.info_too_noisy;
                    if ((rcode & 0x00C0) > 0)
                        result = R.string.info_talk_softer;
                    if ((rcode & 0x0100) > 0)
                        result = R.string.info_bad_recording;
                    if ((rcode & 0x0400) > 0)
                        result = R.string.info_talk_slower;
                    if ((rcode & 0x0800) > 0)
                        result = R.string.info_too_many_sound;
                    mRecogStatusCallback.onStatusChanged(RuntimeStates.CHECKED_REC_FAIL, result);
                    mRecogStatusCallback.onStatusChanged(RuntimeStates.STARTREC, null);
                }
            }
        }
        closeRecog(trig);
        Log.i(TAG, "DONE RECOG THREAD");
    }

    // Signals the recognition thread to halt.
    public synchronized void exitRecog() {
        // closeSession(trig);
        notifyAll();
        halt = true;
    }

    // ************************* native method ************************
    // never used
    public native String getExpirationDate();

    // never used
    public native long secondsUntilExpiration();

    // never used
    public native String getSDKVersion();

    public native int pipeRecogEnroll(long p, ByteBuffer b, short userID);

    // never used
    public native int pipeRecogTest(long p, ByteBuffer b);

    public native int getResultEnroll(long p, short enrollIdx, short userID);

    // never used
    public native String getResultTest(long p, int threshold, String jappDir);

    public native long initRecog(long p, String netfile_UDT, String enrollnetfile_UDT,
                                 String udtsvsidfile_UDT, String phsearchfile_UDT,
                                 float paramAStart, float paramBStart, float checkSNR_UDT,
                                 String netfile_EFT, String searchfile_EFT, String antidatafile_EFT,
                                 float paramAOffset_EFT, float durMinFactor_EFT, float durMaxFactor_EFT,
                                 float triggerSampPerCat_EFT, float sampPerCatWithin_EFT, float targetSNR_EFT,
                                 float learnRate_EFT, float learnRateWithin_EFT, float dropoutWithin_EFT,
                                 float epqMin_EFT, float useFeat_EFT, float checkSNR_EFT, float checkVowelDur_EFT,
                                 float svThresholdBase, float svThresholdStep, String savedir, short numUsers,
                                 short numEnroll);

    // public native String textNorm(long p, String text);

    public native long initSession();

    public native void closeRecog(long p);

    // TODO worring nerver call
    public native void closeSession(long p);

    public native void resetRecog(long p);

    // public native void deleteTune(long p);

    // public native int buildTrigger(long p);

    // never used
    public native int isEnrolled(long p);

    // never used
    public native void setEPQ(long p, short state);

    public native int setMode(long p, short udtMode);

    // never used
    public native int configTriggerLevel(long p, short pos, short type);

    public native int configEnroll(long p, short requireSil);

    public native int doEnroll(long p, String appDir, String FTNetFile, String FTSearchFile, String dspTarget);

    // never used
    public native int loadUsers(long arg);

    // never used
    public native int deleteUser(long arg, short uIdx);

    public native int checkNewRecordings(long p, short userID);

    public native float getPhraseQuality(long p, short userID);

    public native long[] getFeedbackDetails(long p, short userID);

}
