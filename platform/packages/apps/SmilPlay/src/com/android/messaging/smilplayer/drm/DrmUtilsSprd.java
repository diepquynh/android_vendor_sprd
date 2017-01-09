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
import android.util.Log;

import com.google.android.mms.pdu.PduBody;
import com.google.android.mms.pdu.PduPart;

import com.android.messaging.smilplayer.R;
import com.android.messaging.smilplayer.model.SlideshowModel;
import com.android.messaging.smilplayer.model.AudioModel;
import com.android.messaging.smilplayer.model.VideoModel;
import com.android.messaging.smilplayer.ui.UriImage;
import com.android.messaging.smilplayer.util.ThumbnailManager;

public class DrmUtilsSprd {

    static DrmUtilsSprd sInstance;
    public static final int TYPE_IMAGE = 1;
    public static final int TYPE_AUDIO = 2;
    public static final int TYPE_VIDEO = 3;

    public static DrmUtilsSprd getInstance(Context context) {
        if (sInstance != null)
            return sInstance;
        /*sInstance = (DrmUtilsSprd) AddonManager.getDefault().getAddon(
                R.string.mms_drm, DrmUtilsSprd.class);*/
        sInstance = new DrmUtilsSprd();
        return sInstance;

    }

    public DrmUtilsSprd() {
    }

    public boolean isDrmSrc(String src) {
        return false;
    }

    public boolean isDrmExtension(String src) {
        return false;
    }

    public boolean isDrmType(String mimeType) {
        return false;
    }

    public boolean isDrmType(String path, String mimeType) {
        return false;
    }

    public boolean isDrmType(Uri uri, String mimeType) {
        return false;
    }

    public boolean haveRightsForAction(Uri uri, int action) {
        return false;
    }

    public boolean isAttachHasRight(Uri uri) {
        return false;
    }

    public boolean isAttachHasRight(Uri uri, int type) {
        return true;
    }

    public boolean isForwardable(long messageId) {
        return false;
    }

    public boolean isDrmEnabled() {
        return false;
    }

    public Bitmap createEmptyImageBitmapForDrm() {
        return null;
    }

    public String getExtensionByName(String name) {
        return null;
    }
}
