/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.sound;
import java.io.IOException;
import java.util.Arrays;
import android.content.Context;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.SoundPool;
import android.util.Log;
import com.ucamera.ucam.modules.utils.LogUtils;
import com.ucamera.ucam.modules.utils.Utils;;

public class ShutterSound {
    private final static String TAG = "ShutterSound";
    private static final int STREAM_TYPE_SYSTEM_ENFORCED;
    private static int mSound = -1;
    private SoundPool sSoundPool = null;
    private boolean mSetMicroPhoneMute = false;
    private boolean mMute;
    static {
        STREAM_TYPE_SYSTEM_ENFORCED = Utils.getIntFieldIfExists(
                AudioManager.class, "STREAM_SYSTEM_ENFORCED", null, 7);
    }

    private static final int DEFAULT_STREAM_VOLUME = 7;

    private MediaPlayer mShutterPlayer = null;

    private AudioManager mAudioManager;

    public ShutterSound(Context ctx) {
        mAudioManager = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);
    }
    public void loadSounds(Context context){
        if (sSoundPool == null){
            sSoundPool = new SoundPool(5, STREAM_TYPE_SYSTEM_ENFORCED , 0);
                try {
                    mSound = sSoundPool.load("/system/media/audio/ui/camera_click.ogg",1);
                    /**
                     *  BUG 4833
                     *  BUG CAUSE :  this file "/system/media/audio/ui/camera_click.ogg" is not exists in meizu devices ;
                     *  DATE : 2013-09-09
                     */
                    if(mSound <= 0 ) {
                        mSound = sSoundPool.load("/system/media/audio/ui/camera_click.wav",1);
                    }
                }catch(Exception e){
                    mSound = -1;
                    LogUtils.debug(TAG, "mSoundPool.load error!");
                    e.printStackTrace();
            }
        }
    }
    public final void setupShutterSound(boolean mute) {
        mMute = mute;
        if(needAdjustSystemVolume()) {
            setSystemShutterMute(mute);
        }
    }

    public boolean isMute(){
        return mMute;
    }
    public final void restoreSystemShutterSound() {
        if(needAdjustSystemVolume()) {
            setSystemShutterMute(false);
        }
    }

    private void setSystemShutterMute(boolean mute) {
        int volume = mute ? 0: DEFAULT_STREAM_VOLUME;
        LogUtils.debug(TAG, "set StreamENFORCED STREAM_SYSTEM  STREAM_MUSIC [%d]=>%d",STREAM_TYPE_SYSTEM_ENFORCED, volume);
        mAudioManager.setStreamVolume(STREAM_TYPE_SYSTEM_ENFORCED, volume, 0);
    }

    public void playNormal() {
       if(!mMute && isSystemDefaultNoSound()) {
           playCameraSound();
       }
    }

    public void playBurst() {
        if(mMute) return;
        playCameraSound();
    }

    public void playGif(){
        if(mMute) return;
        playCameraSound();
    }

    public void playMagiclens(){
        if(mMute) return;
        playCameraSound();
    }

    public void playUgifVideoSound() {
        if(mMute) return;
        playVideoSound();
    }
    /**
     *  Add the function to play the sound when take the panorama.
     */
    public void playPanorama(){
        if(mMute) return;
        playVideoSound();
    }

    private void playVideoSound(){
        if (mShutterPlayer == null) {
            mShutterPlayer = new MediaPlayer();
        }
        try {
            String strPath = "/system/media/audio/ui/VideoRecord.ogg";
            mShutterPlayer.reset();
            mShutterPlayer.setDataSource(strPath);
            mShutterPlayer.setAudioStreamType(STREAM_TYPE_SYSTEM_ENFORCED);
            mShutterPlayer.prepare();
            mShutterPlayer.start();
        } catch (IOException ex) {
            mShutterPlayer.release();
            mShutterPlayer = null;
        }

    }
    public void playCameraSound() {
        if (sSoundPool != null && mSound !=-1){
            sSoundPool.play(mSound, 1, 1, 1, 0, 1);
        }
    }

    public boolean setMuteRecord(){
        if(!mAudioManager.isMicrophoneMute()){
            mSetMicroPhoneMute = true;
            mAudioManager.setMicrophoneMute(true);
        }
        return mAudioManager.isMicrophoneMute();
    }

    public void resetMuteRecord(){
        if(mAudioManager.isMicrophoneMute() && mSetMicroPhoneMute){
            mSetMicroPhoneMute = false;
            mAudioManager.setMicrophoneMute(false);
        }
    }

    private boolean isSystemDefaultNoSound(){
        return false;
    }

    private boolean needAdjustSystemVolume(){
        return false;
    }
    // release the resources occupied by soundPool
    /* SPRD: Bug540238 SoundPool leaks. @{ */
    public void unloadSounds(){
        if (sSoundPool != null){
            if (mSound != -1){
                sSoundPool.unload(mSound);
            }
            sSoundPool.release();
            sSoundPool = null;
        }
    }
    /* @} */

    private final AudioManager.OnAudioFocusChangeListener mOnAudioFocusChangeListener
        = new AudioManager.OnAudioFocusChangeListener() {
        @Override
        public void onAudioFocusChange(int focusChange) {}
    };

    public void pauseMusic() {
        mAudioManager.requestAudioFocus(mOnAudioFocusChangeListener,
                AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
    }

    public void resumeMusic() {
        mAudioManager.abandonAudioFocus(mOnAudioFocusChangeListener);
    }
}