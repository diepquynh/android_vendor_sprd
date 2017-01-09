package com.android.messaging.smilplayer.drm;

import android.app.Activity;
//import android.app.AddonManager;
import android.content.ContentUris;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.net.Uri;
//import android.os.SystemProperties;
import android.provider.Telephony.Mms;

import com.google.android.mms.pdu.PduBody;
import com.google.android.mms.pdu.PduPart;

import com.android.messaging.smilplayer.R;
import com.android.messaging.smilplayer.model.SlideshowModel;
import com.android.messaging.smilplayer.model.AudioModel;
import com.android.messaging.smilplayer.model.VideoModel;
import com.android.messaging.smilplayer.ui.UriImage;
import com.android.messaging.smilplayer.util.ThumbnailManager;

public class DrmModelUtils {
    static DrmModelUtils sInstance;

    public static DrmModelUtils getInstance(Context context) {
        if (sInstance != null)
            return sInstance;
        /*sInstance = (DrmModelUtils) AddonManager.getDefault().getAddon(
                R.string.mms_model, DrmModelUtils.class);*/
        sInstance = new DrmModelUtils();
        return sInstance;

    }

    public DrmModelUtils() {
    }

    public void setImageType(UriImage uriImage, Uri uri, String extension) {
    }

    public void setImageTypeBySrc(UriImage uriImage, String src) {
    }

    public void setImageTypeAndSrc(UriImage uriImage, String src) {

    }

    public void setAudioTypeBySrc(AudioModel model, String src) {
    }

    public void setAudioTypeByExt(AudioModel model, String extension) {

    }

    public void setAudioTypeAndSrc(AudioModel model, String src) {

    }

    public void setVideoTypeBySrc(VideoModel model, String src) {
    }

    public void setVideoTypeByExt(VideoModel model, String src) {
    }

    public void setVideoSrc(VideoModel model, Cursor c) {
    }

}
