
package com.sprd.voicetrigger.commands;

import android.os.Bundle;
import android.util.Log;

import com.sprd.voicetrigger.controller.RecogniseController;

import java.nio.ByteBuffer;

public class Recog implements Runnable {
    static {
        System.loadLibrary("jni_udtsid");
    }

    public native long secondsUntilExpiration();

    public native String getExpirationDate();

    public native String getSDKVersion();

    public native long initSDK(String logFile);

    public native long initSession();

    public native long setupRecog(long p, String trignetfile, String trigsearchfile, String commnetfile, String commsearchfile);

    public native long initRecog(long p, long mode);

    public native String pipeRecog(long p, ByteBuffer b, long mode);

    public native void closeRecog(long p);

    public static String res = null;
    boolean halt = false;
    boolean swap = false;
    long trig;

    private RecogniseController mRecongManager = null;

    public Recog(RecogniseController controller) {
        super();
        //mTHFActivity = (THF) activity;
        mRecongManager = controller;
    }

    public void swapSearch() {
        swap = true;
    }


    public void run() {
        //android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_MORE_FAVORABLE);
        trig = initSDK(mRecongManager.logFile);
        if (trig == -1) {
            //Log.i(THF.TAG,"LICENSE EXPIRED");
            //THF.sendMsg(THF.MSG_LICENSE_EXPIRED);
            return;
        }
        //initRecog(trig,THF.trignetFile,THF.searchArray.get(THF.sensitivity),THF.commnetFile,THF.commsearchFile);
        /*Log.i(THF.TAG, "THF.trignetFile='"    + THF.trignetFile    + "'");
        Log.i(THF.TAG, "THF.trigsearchFile='" + THF.trigsearchFile + "'");
        Log.i(THF.TAG, "THF.commnetFile='"    + THF.commnetFile    + "'");
        Log.i(THF.TAG, "THF.commsearchFile='" + THF.commsearchFile + "'");*/
        setupRecog(trig, mRecongManager.mTrignetFile, mRecongManager.trigsearchFile,
                mRecongManager.commnetFile, mRecongManager.commsearchFile);
        initRecog(trig, mRecongManager.recogMode);
        //THF.sendMsg(THF.MSG_RECOG_READY);
        mRecongManager.updateState(RecogniseController.RECOG_INIT, null);
        while (!halt) {
            ByteBuffer buf = null;
            try {
                buf = mRecongManager.mAudioInstance.getBuffer();
            } catch (InterruptedException e) {
                throw new IllegalStateException("RecogThread: getBuffer failed");
            }
            if (swap) {
                //Log.i(THF.TAG,"SWAP SEARCH: mode="+THF.recogMode);
                //initRecog(trig,THF.recogMode);
                swap = false;
                //THF.startTime = System.currentTimeMillis();
                //THF.audioInstance.discardAudio();
                //Log.i(THF.TAG,"DONE SWAP");
            }
            res = pipeRecog(trig, buf, mRecongManager.recogMode);
            Log.i("SelfRecog", "Result=" + res);
            if (res != null && mRecongManager.recogMode == 0) {
                mRecongManager.updateState(RecogniseController.RECOG_SUCCESS, null);
                mRecongManager.setRecogMode((short) 1);
            } else if ((res != null) && (mRecongManager.recogMode == 1)) {
                Log.d("SelfRecog", "update command");
                mRecongManager.setRecogMode((short) 0);
                Bundle bundle = new Bundle();
                bundle.putString("command_result", res);
                mRecongManager.updateState(RecogniseController.RECOG_COMMAND_SUCCESS, bundle);
            }

            //THF.sendMsg(THF.MSG_TRIGGER);
        }
        //Log.i(THF.TAG,"recogThread: Halted!");

        //closeRecog(trig);
    }

    public synchronized void exitRecog() {
        halt = true;
    }
}



