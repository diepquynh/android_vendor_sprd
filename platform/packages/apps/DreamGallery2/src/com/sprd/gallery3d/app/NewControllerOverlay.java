/**
 * Created By Spreadst
 * */

package com.sprd.gallery3d.app;

public interface NewControllerOverlay {
    public interface NewListener{
        void onVideoSliding(float speed);
        void adjustVolume(float index);
        void adjustLightness(float index);
        void lockControl();
        void lockOrintation();
        void printScreen();
        int getVideoWidth();
        void endSliding();
        void doubleClickPlayPause();
        void adjustVideoSize();
        /* SPRD:Add for new feature 568552 Delete the methods in TrimVideo.java@{*/
        void onStopVideo();
        void onNext();
        void onPrev();
        void onStartFloatPlay();
        void onShown();
        void onHidden();
        /* @} */
        // SPRD:Add for bug597099 When clear the cache,enter the videoplayer and lock control,the controller will disappear
        void setSystemUiVisibility(boolean isUiVisible);
        boolean isNewStreamUri();
        boolean isDisableFloatWindow();
    }
    void setNewListener(NewListener listener);
    void showSpeed(int time);
    void showBackward(int time);
    void showVolume(String vol);
    void showLightness(String light);
}
