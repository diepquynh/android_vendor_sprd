/** Created by Spreadst */

package com.sprd.gallery3d.gif;

import android.content.Context;
import android.graphics.Bitmap;
import android.net.Uri;
import android.util.Log;

import com.android.gallery3d.util.GalleryUtils;

public class GifPlayManager implements GifDecoder.PlayThreadController {

    private static final String TAG = "GifPlayManager";
    private static final boolean DEBUG_GIF = true;

    private DisplayGifCallback mCallback;
    private GifDecoder mDecoder;
    private PlayGifThread mPlayGifThread;

    private Uri mTempUri;

    public final Object PLAY_THREAD_LOCK = this;

    public GifPlayManager(Context context) {
        init(context);
    }

    public GifPlayManager(Context context, DisplayGifCallback callback) {
        init(context);
        setDisplayGifCallback(callback);
    }

    public void setDisplayGifCallback(DisplayGifCallback callback) {
        this.mCallback = callback;
    }

    private void init(Context context) {
        mDecoder = new GifDecoder(context, this);
        mPlayGifThread = new PlayGifThread("play_thread");
        if (GalleryUtils.SUPPORT_GIF)
            mPlayGifThread.start();
    }

    public void pausePlayFrame() {
        if (DEBUG_GIF) {
            Log.d(TAG, "GifPlayManager pause=====");
        }
        mCallback.pausePlayFrame();
    }

    public void pause() {
        mPlayGifThread.pause();
    }

    public void resume() {
        mPlayGifThread.unlock();
    }

    public boolean isSameUri(Uri uri) {
        return mTempUri != null && mTempUri.equals(uri);
    }

     public void playGif(Uri uri, boolean isGif) {
        if (!isGif) {
            if (DEBUG_GIF) {
                Log.d(TAG, "GifPlayManager is not a gif=====");
            }
            mPlayGifThread.pausePlay();
            mTempUri = null;
            return;
        }
        if (DEBUG_GIF) {
            Log.d(TAG, "GifPlayManager : uri: " + uri.toString() + " , mTempUri : "
                    + ((mTempUri == null) ? "null" : mTempUri.toString()));
        }
        mPlayGifThread.resumePlay();
        if (isSameUri(uri)) {
            if (DEBUG_GIF) {
                Log.d(TAG, "GifPlayManager: the same uri");
            }
            mPlayGifThread.playResume();
        } else {
            mTempUri = uri;
            mDecoder.startDecode(uri);
        }
    }

    public void quitManager() {
        // use pause method, will invoke mCallback.pausePlayFrame() and set
        // mPlayGifThread.lock true;
        pause();
        mPlayGifThread.quit();
    }

    @Override
    public void lock() {
        if (DEBUG_GIF) {
            Log.d(TAG, "GifPlayManager lock the play thread=====");
        }
        mPlayGifThread.waitAndReset();
    }

    @Override
    public void unlock() {
        if (DEBUG_GIF) {
            Log.d(TAG, "GifPlayManager unlock the play thread=====");
        }
        mPlayGifThread.unlock();
    }

    public interface DisplayGifCallback {
        public void displayGifFrame(Bitmap bm);

        public void pausePlayFrame();

        public void startPlayFrame();
    }

    private class PlayGifThread extends Thread {
        private boolean isRun;
        private boolean mLock;
        private boolean reset;
        private boolean mPlay;

        public GifDecoder.GifFrame temp, mFirstFrame, mCurrentFrame;

        public PlayGifThread(String name) {
            super(name);
            this.isRun = true;
            this.mLock = true;
            this.reset = false;
            this.mPlay = true;
        }

        @Override
        public void run() {
            while (true) {
                while (true) {
                    try {
                        synchronized (PLAY_THREAD_LOCK) {
                            if (!this.isRun) {
                                if (DEBUG_GIF) {
                                    Log.d(TAG, "GifPlayManager: playThread quit=====");
                                }
                                mDecoder.quitDecoder();
                                mCallback = null;
                                PLAY_THREAD_LOCK.notifyAll();
                                return;
                            }
                            if (this.mLock) {
                                if (DEBUG_GIF) {
                                    Log.d(TAG, "GifPlayManager: playThread wait=====");
                                }
                                PLAY_THREAD_LOCK.notifyAll();
                                this.mLock = false;
                                PLAY_THREAD_LOCK.wait();
                                if (this.reset) {
                                    mFirstFrame = null;
                                    mCurrentFrame = null;
                                    temp = null;
                                    this.reset = false;
                                }
                            }
                            if (mFirstFrame == null) {
                                mFirstFrame = mDecoder.getFirstFrame();
                            }
                            if (mFirstFrame != null) {
                                if (mCurrentFrame == null) {
                                    mCurrentFrame = mFirstFrame;
                                    if (DEBUG_GIF) {
                                        Log.d(TAG,
                                                "GifPlayManager: PlayThread the first frame=====");
                                    }
                                    break;
                                } else {
                                    this.temp = mCurrentFrame.next;
                                    if (this.temp != null) {
                                        mCurrentFrame = temp;
                                        break;
                                    }
                                }
                            } else {
                                if (DEBUG_GIF) {
                                    Log.d(TAG,
                                            "GifPlayManager: playThread get none first frame=====");
                                }
                                /* SPRD:for bug432196, thread leak in gallery monkey test @{ */
                                if (this.isRun) {
                                    PLAY_THREAD_LOCK.wait();
                                }
                                /* @} */
                            }
                        }
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                synchronized (PLAY_THREAD_LOCK) {
                    if (this.mLock) {
                        continue;
                    }
                }
                int dealy = mCurrentFrame.delay;
                if (dealy == 0) {
                    dealy = 50;
                }
                mCallback.displayGifFrame(mDecoder.getDisplayBitmap());
                try {
                    Thread.sleep(dealy);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                if (DEBUG_GIF) {
                    Log.d(TAG, "GifPlayManager: PlayThread played=====");
                }
            }
        }

        private void waitAndReset() {
            reset = true;
            lock();
        }

        private void pause() {
            if (mCallback != null) {
                mCallback.pausePlayFrame();
            }
            synchronized (PLAY_THREAD_LOCK) {
                this.mLock = true;
            }
        }

        private void pausePlay() {
            mPlay = false;
            pause();
        }

        private void resumePlay() {
            mPlay = true;
        }

        private void playResume() {
            if (mCallback != null) {
                mCallback.startPlayFrame();
            }
            synchronized (PLAY_THREAD_LOCK) {
                this.mLock = false;
                PLAY_THREAD_LOCK.notifyAll();
            }
        }

        private void lock() {
            if (this.getState().compareTo(State.WAITING) == 0
                    || this.getState().compareTo(State.BLOCKED) == 0) {
                return;
            }
            synchronized (PLAY_THREAD_LOCK) {
                PLAY_THREAD_LOCK.notifyAll();
                this.mLock = true;
                try {
                    PLAY_THREAD_LOCK.wait();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }

        private void unlock() {
            if (!mPlay) {
                if (DEBUG_GIF) {
                    Log.d(TAG, "unlock fail because play status is false!");
                }
                return;
            }
            if (mCallback != null) {
                mCallback.startPlayFrame();
            }
            if (!(this.getState().compareTo(State.WAITING) == 0 || this.getState().compareTo(
                    State.BLOCKED) == 0)) {
                if (DEBUG_GIF) {
                    Log.d(TAG, "is not locked!");
                }
                return;
            }
            synchronized (PLAY_THREAD_LOCK) {
                this.mLock = false;
                PLAY_THREAD_LOCK.notifyAll();
            }
        }

        private void quit() {
            synchronized (PLAY_THREAD_LOCK) {
                this.isRun = false;
                PLAY_THREAD_LOCK.notifyAll();
            }
        }
    }
}
