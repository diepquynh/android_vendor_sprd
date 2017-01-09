
package com.sprd.voicetrigger.nativeinterface;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Build;
import android.util.Log;

import com.sprd.voicetrigger.WakeupDescribeActivity;
import com.sprd.voicetrigger.global.RuntimeStates;

import java.nio.ByteBuffer;
import java.util.Vector;

public class Audio implements Runnable {

    // NOTE: VOICE_RECOGNITION mode seems to make recordings and playback very
    // quiet so using MIC mode instead
    public static final int recsrcVR = MediaRecorder.AudioSource.VOICE_RECOGNITION;
    public static final int recsrcMIC = MediaRecorder.AudioSource.MIC;
    public int recsrc = recsrcVR; // current audio source

    AudioRecord aud = null;
    private static final int encoding = AudioFormat.ENCODING_PCM_16BIT;
    private static final int chan = AudioFormat.CHANNEL_IN_MONO;
    public static long samplerate;
    private int bufferSize;
    private static Vector<ByteBuffer> buffers = new Vector<>();
    private volatile boolean isRecording = false;
    private volatile boolean isStopped = true;
    private final Object mutex = new Object();
    private final Object recordingStopped = new Object();
    private boolean done = false;
    private static final String TAG = "audioRunnable";
    private AudioStatusCallback mAudioStatusCallback;

    public interface AudioStatusCallback {
        void onStatusChanged(int status, String msg);
    }

    public Audio(AudioStatusCallback callback) {
        super();
        mAudioStatusCallback = callback;
        Log.i(TAG, "MODEL=" + Build.MODEL);
        // emulator
        if (Build.MODEL.equals("google_sdk") || Build.MODEL.equals("sdk")) {
            samplerate = 8000;
            bufferSize = 8000; // Cannot be smaller than 4096 on emulator
        } else { // Real device
            samplerate = 16000;
            bufferSize = AudioRecord.getMinBufferSize((int) samplerate, chan, encoding);
        }
    }

    // Wipes the audio queue
    public synchronized void flushAudio() {
        Log.i(TAG, "flush audio");
        buffers.removeAllElements();
    }

    // Returns the audio queue size
    public synchronized int getSize() {
        return buffers.size();
    }

    // Returns audio buffer from queue. Blocking call while no audio is
    // available.
    public synchronized ByteBuffer getBuffer() throws InterruptedException {
        while (buffers.size() == 0) {
            Log.d(TAG, "buffers.size() == 0 ---- waiting");
            wait();
        }
        ByteBuffer buf = buffers.firstElement();
        buffers.removeElement(buf);
        Log.d(TAG, "getBuffer buffers.size() = " + buffers.size());
        return buf;
    }

    /**
     * Adds audio buffer to queue. </p> - Computes maximum amplitude and
     * FFT.</p> - Sends FFT data to GUI for updating VU meter. </p> - Notifies
     * waiting functions that audio is available.</p>
     *
     * @param buf
     */
    private synchronized void addBuffer(ByteBuffer buf) {
        int MAXQUEUE = 100;
        if (buffers.size() == MAXQUEUE) {
            // TODO : add text to String.xml
            mAudioStatusCallback.onStatusChanged(RuntimeStates.LONG_TOAST, "Buffer Overflow...resetting");
            Log.d(TAG, "Buffer Overflow...resetting");
            buffers.removeAllElements();
        }
        buffers.addElement(buf);
        Log.d(TAG, "add buffer... buffers size = " + buffers.size());
        notify(); // notify waiting getBuffer() that audio is available
        mAudioStatusCallback.onStatusChanged(RuntimeStates.ADDBUFF, null);
    }

    // Initializes the audio interface using the AudioRecord class.
    private boolean initAudio() {
        aud = null;
        boolean success = false;
        int i = 0;
        while (!success && i < 5) {
            i++;
            try {
                recsrc = WakeupDescribeActivity.recsrc;
                aud = new AudioRecord(recsrc, (int) samplerate, chan,
                        encoding, bufferSize);
                aud.startRecording();
                success = true;
            } catch (IllegalStateException e) {
                aud = null;
                e.printStackTrace();
                Log.i(TAG, "Audio init failed: attempt " + i);
                synchronized (this) {
                    try {
                        wait(250);
                    } catch (InterruptedException e1) {
                        // just do wait ignore error
                    }
                }
            }
        }
        return success;
    }

    /**
     * Main event loop for audio thread.
     * - Initializes audio (if needed) - Waits for signal to start recording
     * - Retrieves audio buffer from audio device and places into audio queue
     * - Notifies the GUI when recording is stopped
     */
    public void run() {
        android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
        while (!done) {
            synchronized (mutex) {
                try {
                    Log.i(TAG, "audio run: wait");
                    mutex.wait();
                    Log.i(TAG, "audio run: unwait");
                } catch (InterruptedException e) {
                    Log.w(TAG, "AudioThread: interrupted");
                }
            }
            if (!initAudio()) {
                // TODO : add text to String.xml
                mAudioStatusCallback.onStatusChanged(RuntimeStates.BAIL,
                        "Failed to initialize audio device");
                return;
            }
            setStopped(false);
            while (isRecording) {
                ByteBuffer buffer = ByteBuffer.allocateDirect(bufferSize * 2);
                int numSamples = aud.read(buffer, bufferSize * 2);
                if (numSamples == AudioRecord.ERROR_INVALID_OPERATION) {
                    throw new IllegalStateException("read() returned ERROR_INVALID_OPERATION");
                } else if (numSamples == AudioRecord.ERROR_BAD_VALUE) {
                    throw new IllegalStateException("read() returned ERROR_BAD_VALUE");
                }
                numSamples /= 2;
                addBuffer(buffer);
            }
            aud.stop();
            aud.release();
            aud = null;
            setStopped(true);
            synchronized (recordingStopped) {
                Log.i(TAG, "audio run: notify recordStopped");
                recordingStopped.notifyAll();
            }
        }
    }

    /**
     * Signal start/stop recording
     *
     * @param val
     */
    public void setRecording(boolean val) {

        this.isRecording = val;
        // Log.i(TAG, "setRecording=" + val, new Throwable());
        Log.i(TAG, "setRecording=" + val);
        if (this.isRecording) {
            Log.i(TAG, "setRecording: notify");
            synchronized (mutex) {
                mutex.notify();
            }
        } else if (!isStopped) { // synchronize stop record
            synchronized (recordingStopped) {
                try {
                    Log.i(TAG, "setRecording: wait on recordStop");
                    recordingStopped.wait();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    // Returns isRecording state
    public boolean isRecording() {
        synchronized (mutex) {
            return isRecording;
        }
    }

    // Returns isStopped state Called from shutdown code
    public boolean isStopped() {
        synchronized (mutex) {
            return isStopped;
        }
    }

    // Sets isStopped state
    public void setStopped(boolean val) {
        synchronized (mutex) {
            Log.i(TAG, "setStopped: notify");
            this.isStopped = val;
        }
    }

    // Signal audio event loop to halt
    public synchronized void exitAudio() {
        notifyAll();
        done = true;
    }

}
