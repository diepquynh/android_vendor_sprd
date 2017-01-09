package com.sprd.hz.selfportrait.util;
import java.util.HashMap;

import com.android.camera2.R;

import android.content.Context;
import android.media.AudioManager;
import android.media.SoundPool;
import com.sprd.hz.selfportrait.util.ContextUtil;
public class SoundUtil {
    public static final int SOUND_TIMER = 0x00000001;
    private static SoundUtil sInstance = null;
    public static Context mContext;
    public static SoundUtil getInstance() {
        if(sInstance==null) {
            sInstance = new SoundUtil();
            sInstance.loadSound(SOUND_TIMER,ContextUtil.getInstance().getContext(), R.raw.cam_timer);
        }
        return sInstance;
    }
    private SoundPool mSoundPool = null;;
    private HashMap<Integer, Integer> mSoundPoolMap = null;;
    private static final int STREAM_TYPE = AudioManager.STREAM_SYSTEM;
    /**
     * Default constructor associates this manager with an instance of SoundPool to manage
     * sounds.
     */
    private SoundUtil() {
        mSoundPool = new SoundPool(10, STREAM_TYPE, 0);
        mSoundPoolMap = new HashMap<Integer, Integer>();
    }
    /**
     * Load the sound resource which needs to be played.<br>
     * Note: This is an asynchronous function. Calling playSound
     * right after loadSound may cause an error.
     *
       * @param key The key information of the sound file.
        * @param context The Context of a application.
       * @param resId The identity information of the sound file.
     */
    private void loadSound(int key, Context context, int resId) {
        if(mSoundPoolMap != null && mSoundPool != null)
            mSoundPoolMap.put(key, mSoundPool.load(context, resId, 1));
    }
    /**
     * Play the sound resource which had been loaded.
     *
       * @param key  The key information of the sound file.
     */
    public int playSound(int key){
        if(mSoundPoolMap != null && mSoundPool != null)
            return mSoundPool.play(mSoundPoolMap.get(key), 0.5f, 0.5f, 1, 0, 1f);
        return 0;
    }
    public void setLoop(int streamId, int loop) {
        if(mSoundPoolMap != null && mSoundPool != null) {
            mSoundPool.setLoop(streamId, loop);
        }
    }
    /**
     * Play the sound resource which had been loaded.
     *
       * @param key      The key information of the sound file.
       * @param volume The volume which the sound play at(range = 0.0 to 1.0).
       * @param loop      Loop mode (0 = no loop, -1 = loop forever).
       * @param rate     Playback rate(1.0 = normal playback, range 0.5 to 2.0).
     */
    public void playSound(int key, float volume, int loop, float rate){
        if(mSoundPoolMap != null && mSoundPool != null)
            mSoundPool.play(mSoundPoolMap.get(key), volume, volume, 1, loop, rate);
    }
    /**
     * Release sound resources used in the SoundPool.
     */
    public void release(){
        if(mSoundPool != null){
            mSoundPool.release();
            mSoundPool = null;
        }
        if(mSoundPoolMap != null){
            mSoundPoolMap.clear();
            mSoundPoolMap = null;
        }
    }
}
