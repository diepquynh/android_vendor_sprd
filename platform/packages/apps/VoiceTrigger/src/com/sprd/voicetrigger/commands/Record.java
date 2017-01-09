
package com.sprd.voicetrigger.commands;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.os.Build;
import android.util.Log;

import com.sprd.voicetrigger.controller.RecogniseController;

import java.nio.ByteBuffer;
import java.nio.ShortBuffer;
import java.util.Vector;

public class Record implements Runnable {
    AudioTrack oTrack = null;
    byte[] playData = null;
    AudioRecord aud = null;
    private static final int encoding = AudioFormat.ENCODING_PCM_16BIT;
    // private static final int encoding = AudioFormat.ENCODING_PCM_8BIT;
    private static final int chan = AudioFormat.CHANNEL_IN_MONO;
    // private static final int chan = AudioFormat.CHANNEL_IN_STEREO;
    private static int samplerate = 0;
    private int bufferSize = 0;
    // private static final int recsrc = MediaRecorder.AudioSource.DEFAULT;
    // private static final int recsrc = MediaRecorder.AudioSource.MIC;
    private static final int recsrc = MediaRecorder.AudioSource.VOICE_RECOGNITION;
    private int MAXQUEUE = 50;
    private int numSamples = 0;
    static Vector<ByteBuffer> buffers = new Vector<ByteBuffer>();
    private volatile boolean isRecording = false;
    private volatile boolean isStopped = true;
    private final Object mutex = new Object();
    public int maxamp = 0;
    private boolean done = false;
    private static String TAG = "SlefAudio";

    private long mStartTime;
    private boolean mIsCommandMode = false;
    private static long TIME_OUT_COMMAND = 8000;

    public Record(RecogniseController controller) {
        super();
        if (Build.MODEL.equals("google_sdk") || Build.MODEL.equals("sdk")) { // emulator
            samplerate = 8000;
            bufferSize = 8000; // NOTE: CANNOT BE SMALLER THAN 4096 on emulator
        } else { // real device
            samplerate = 16000;
            bufferSize = AudioRecord.getMinBufferSize((int) samplerate, chan, encoding);
        }
        mRecongManager = controller;
    }

    public void setCommandStart() {
        mIsCommandMode = true;
        mStartTime = System.currentTimeMillis();
    }

    public void stopCommand() {
        mIsCommandMode = false;
        mStartTime = 0;
    }

    public long getLeftTime() {
        return TIME_OUT_COMMAND - (System.currentTimeMillis() - mStartTime);
    }

    public synchronized void discardAudio() {
        if (buffers.size() > 0)
            buffers.removeAllElements();
    }

    public synchronized int getSize() {
        return buffers.size();
    }

    public synchronized ByteBuffer getBuffer() throws InterruptedException {
        while (buffers.size() == 0) {
            // Log.i(THF.TAG,"getBuffer: wait1");
            wait();
            // Log.i(THF.TAG,"getBuffer: wait2");

        }
        ByteBuffer buf = buffers.firstElement();
        buffers.removeElement(buf);
        return buf;
    }

    private synchronized void addBuffer(ByteBuffer buf) {
        Log.d(TAG, "addBuffer");
        ShortBuffer sb = buf.asShortBuffer();
        maxamp = 0;
        for (int i = 0; i < sb.capacity(); i++) {
            int val = (int) Short.reverseBytes(sb.get(i));
            if (val > maxamp)
                maxamp = val;
        }
        // THF.sendMsg(THF.MSG_ADDBUFF);
        if (mIsCommandMode && (getLeftTime() <= 0)) {
            mRecongManager.updateState(RecogniseController.RECOG_COMMAND_SUCCESS, null);
            mIsCommandMode = false;
        }
        if (buffers.size() == MAXQUEUE) {
            // Log.w(THF.TAG,"Warning: audio buffer overflow; Resetting!");
            buffers.removeAllElements();
            setRecording(false);
            // MSG_OVERFLOW
            // THF.sendMsg(THF.MSG_OVERFLOW);
            // throw new IllegalStateException("addBuffer: BUFFER OVERFLOW");
        }
        buffers.addElement(buf);
        // Log.i(THF.TAG,"ADD BUFFER: sz="+buffers.size());
        notify(); // notify waiting getBuffer
    }

    private RecogniseController mRecongManager = null;

    void initAudio() {
        aud = null;
        boolean success = false;
        int i = 0;
        while (!success && i < 5) {
            i++;
            try {
                aud = new AudioRecord(recsrc, samplerate, chan, encoding, bufferSize);
                aud.startRecording();
                success = true;
            } catch (IllegalStateException e) {
                aud = null;
                e.printStackTrace();
                // Log.i(THF.TAG,"Audio init failed: attempt "+i);
                synchronized (this) {
                    try {
                        wait(250);
                    } catch (InterruptedException e1) {
                        e1.printStackTrace();
                    }
                }
            }
        }
        Log.d("SelfRecord", "MSG_STARTREC", new Throwable());
        // THF.sendMsg(THF.MSG_STARTREC);
        mRecongManager.updateState(RecogniseController.RECOG_START, null);
    }

    public void run() {
        android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
        while (!done) {
            initAudio();
            setStopped(false);
            while (isRecording) {
                ByteBuffer buffer = ByteBuffer.allocateDirect(bufferSize * 2);
                numSamples = aud.read(buffer, bufferSize * 2) / 2;
                if (numSamples == AudioRecord.ERROR_INVALID_OPERATION) {
                    throw new IllegalStateException(
                            "read() returned AudioRecord.ERROR_INVALID_OPERATION");
                } else if (numSamples == AudioRecord.ERROR_BAD_VALUE) {
                    throw new IllegalStateException("read() returned AudioRecord.ERROR_BAD_VALUE");
                } else if (numSamples == AudioRecord.ERROR_INVALID_OPERATION) {
                    throw new IllegalStateException(
                            "read() returned AudioRecord.ERROR_INVALID_OPERATION");
                }
                addBuffer(buffer);
            }
            // THF.sendMsg(THF.MSG_STOPREC);
            aud.stop();
            aud.release();
            aud = null;
            setStopped(true);
            synchronized (mutex) {
                try {
                    // Log.i(THF.TAG,"AUDIO THREAD WAIT.....");
                    mutex.wait();
                    // Log.i(THF.TAG,"AUDIO THREAD RESUME.....");
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public void setRecording(boolean val) {
        synchronized (mutex) {
            this.isRecording = val;
            if (this.isRecording) {
                // Log.i(THF.TAG,"setRecord: sending NOTIFY");
                mutex.notify();
            }
        }
    }

    public boolean isRecording() {
        synchronized (mutex) {
            return isRecording;
        }
    }

    public boolean isStopped() {
        synchronized (mutex) {
            return isStopped;
        }
    }

    public void setStopped(boolean val) {
        synchronized (mutex) {
            this.isStopped = val;
        }
    }

    public synchronized void exitAudio() {
        done = true;
        notifyAll();
    }
}
