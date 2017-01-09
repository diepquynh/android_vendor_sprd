
package com.dream.camera;

public class DreamModule {

    public final static int UNDEFINED = -1;
    public final static int INTERVAL_MODULE = 0;
    public final static int AUDIOPICTURE_MODULE = 1;
    public final static int VGESTURE_MODULE = 2;
    public final static int GIF_MODULE = 3;
    public final static int FILTER_MODULE = 4;
    public final static int QR_MODULE = 5;
    public final static int CONTINUE_MODULE = 6;
    public final static int WIDEANGLE_MODULE = 7;

    public int getModuleTpye() {
        return UNDEFINED;
    }

    public void onGifCancel() {
    }

    public void onGifFinish() {
    }

    public boolean isSetFreezeFrameDisplay() {
        return false;
    }

    public boolean isFreezeFrameDisplayShow() {
        return false;
    }

    public boolean canShutter() {
        return true;
    }

    public boolean isBurstThumbnailNotInvalid() {
        return false;
    }

    public void restoreCancelBurstTag() {
    }

    public boolean isGifCapture() {
        return false;
    }

    public boolean isAudioRecording() {
        return false;
    }
}
