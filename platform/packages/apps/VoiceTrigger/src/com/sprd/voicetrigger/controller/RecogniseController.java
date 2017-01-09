
package com.sprd.voicetrigger.controller;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.text.SpannableStringBuilder;
import android.util.Log;

import com.sprd.voicetrigger.R;
import com.sprd.voicetrigger.commands.Recog;
import com.sprd.voicetrigger.commands.Record;
import com.sprd.voicetrigger.utils.VocabularyPackageManager;

import java.io.File;
import java.util.ArrayList;
import java.util.Locale;

public class RecogniseController {
    // Recognise status
    public static final int RECOG_INIT = 0;
    public static final int RECOG_START = 1;
    public static final int RECOG_WAIT = 2;
    public static final int RECOG_STOP = 3;
    public static final int RECOG_SUCCESS = 4;
    public static final int RECOG_FAIL = 5;
    public static final int RECOG_UNKOWN = 6;
    public static final int RECOG_ERROR = 7;
    public static final int RECOG_TRIG_SUCCESS = 8;
    public static final int RECOG_COMMAND_SUCCESS = 9;

    private OnRecogniseStatusChangedListener mOnRecogniseStatusChangedListener;

    private Handler mainHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case RECOG_INIT:
                    if (mOnRecogniseStatusChangedListener != null) {
                        mOnRecogniseStatusChangedListener.onRecogniseInit();
                    }
                    break;
                case RECOG_START:
                    if (mOnRecogniseStatusChangedListener != null) {
                        mOnRecogniseStatusChangedListener.onRecogniseStart();
                    }
                    break;
                case RECOG_WAIT:
                    if (mOnRecogniseStatusChangedListener != null) {
                        mOnRecogniseStatusChangedListener.onRecogniseWait();
                    }
                    break;
                case RECOG_STOP:
                    if (mOnRecogniseStatusChangedListener != null) {
                        mOnRecogniseStatusChangedListener.onRecogniseStop();
                    }
                    break;

                case RECOG_SUCCESS:
                    if (mOnRecogniseStatusChangedListener != null) {
                        mOnRecogniseStatusChangedListener.onRecogniseSuccess();
                    }
                    break;
                case RECOG_COMMAND_SUCCESS:
                    if (mOnRecogniseStatusChangedListener != null) {
                        mOnRecogniseStatusChangedListener.onCommandSuccess(msg.getData());
                    }
                    break;
                case RECOG_FAIL:
                    if (mOnRecogniseStatusChangedListener != null) {
                        mOnRecogniseStatusChangedListener.onRecogniseFailed();
                    }
                    break;
                case RECOG_UNKOWN:
                    if (mOnRecogniseStatusChangedListener != null) {
                        mOnRecogniseStatusChangedListener.onRecogniseUnkown();
                    }
                    break;
                case RECOG_ERROR:
                    if (mOnRecogniseStatusChangedListener != null) {
                        mOnRecogniseStatusChangedListener.onRecogniseError("错误");
                    }
                    break;
                default:

                    break;
            }
            super.handleMessage(msg);
        }

        @Override
        public boolean sendMessageAtTime(Message msg, long uptimeMillis) {
            return super.sendMessageAtTime(msg, uptimeMillis);
        }

    };

    public Record mAudioInstance = null;
    public Recog mRecogInstance = null;


    protected Thread mAudioThread = null;
    protected Thread mRecogThread = null;

    /**
     * construct method
     */
    public RecogniseController(Context context) {
        mContext = context;
        loadConfig();

    }

    public Context getContext() {
        return mContext;
    }

    public void start() {
        recogMode = 0;
        mRecogInstance = new Recog(this);
        mAudioInstance = new Record(this);
        mAudioThread = new Thread(mAudioInstance);
        mRecogThread = new Thread(mRecogInstance);
        mAudioThread.start();
        mRecogThread.start();
    }

    private Context mContext = null;
    public VocabularyPackageManager.packClass mTrigPack = null;
    public String mTrigName = null;
    public String mTrigSens = null;
    public VocabularyPackageManager.packClass mCommPack = null;
    public String mCommName = null;
    public String mCommSens = null;

    public String mTrignetFile = null;
    public String trigsearchFile = null;
    public String commnetFile = null;
    public String commsearchFile = null;
    public String appDirFull = null;
    public static String logFile = null;
    public static short recogMode = 0;

    private static final String TAG = "RecogniseController";

    private void loadConfig() {
        AssetManager amgr = mContext.getAssets();

        VocabularyPackageManager.getInstance().initCommandPacks(amgr, mContext.getFilesDir().toString() + "/");


        File myDir = null;
        String root = Environment.getExternalStorageDirectory().toString();
        myDir = new File(root + "/sensory");
        myDir.mkdirs();
        myDir = new File(root + "/sensory/thf");
        myDir.mkdirs();
        logFile = root + "/sensory/thf/log.txt";

        mTrigName = mContext.getResources().getString(R.string.default_trigger);
        mTrigSens = mContext.getResources().getString(R.string.default_trigger_sen);

        mCommName = mContext.getResources().getString(R.string.default_command);
        mCommSens = mContext.getResources().getString(R.string.default_command_sen);
        mTrigPack = VocabularyPackageManager.getInstance().getPack(mTrigName);
        mCommPack = VocabularyPackageManager.getInstance().getPack(mCommName);

        mTrignetFile = mTrigPack.netFile;
        trigsearchFile = mTrigPack.search.get(mTrigPack.sensitivity.indexOf(mTrigSens));
        commnetFile = mCommPack.netFile;
        commsearchFile = mCommPack.search.get(mCommPack.sensitivity.indexOf(mCommSens));
    }

    public void setRecordingState(boolean state) {
        mAudioInstance.setRecording(state);
    }

    public void startCommand() {
        mAudioInstance.setCommandStart();
    }

    //for listener test 
    public void listenerTest(int status) {
        Message msg = new Message();
        msg.what = status;
        mainHandler.sendMessage(msg);
    }

    public void updateState(int status, Bundle bundle) {
        Message msg = new Message();
        msg.what = status;
        msg.setData(bundle);
        mainHandler.sendMessage(msg);
    }

    public void setRecogMode(short val) {
        recogMode = val;
    }

    public void stop() {
        mRecogInstance.exitRecog();
        setRecordingState(false);
        mAudioInstance.exitAudio();
    }

    public void setDisplay(String lastResult) {
        Locale locale = new Locale("en_US");
        Log.i(TAG, "locale = " + locale);
        Locale.setDefault(locale);

        ArrayList<SpannableStringBuilder> ssbCommands = new ArrayList<SpannableStringBuilder>();
        Log.i(TAG, "numCommands = " + VocabularyPackageManager.getInstance().getCommandsNum(mCommPack));
        for (int iDx = 0; iDx < VocabularyPackageManager.getInstance().getCommandsNum(mCommPack); iDx++) {
            if (lastResult.equals(VocabularyPackageManager.getInstance().getCommandPhrase(mCommPack, iDx))) {

            }
        }
    }

    public void setOnRecogniseStatusChangedListener(OnRecogniseStatusChangedListener orscl) {
        mOnRecogniseStatusChangedListener = orscl;
    }

    public void unRegistRecogniseStatusChangedListener() {
        mOnRecogniseStatusChangedListener = null;
    }

    public interface OnRecogniseStatusChangedListener {
        void onRecogniseInit();

        void onRecogniseStart();

        void onRecogniseWait();

        void onRecogniseStop();

        void onRecogniseSuccess();

        void onCommandSuccess(Bundle bundle);

        void onRecogniseFailed();

        void onRecogniseUnkown();

        void onRecogniseError(String errorStr);
    }
}
