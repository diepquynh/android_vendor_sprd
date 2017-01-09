
package com.spreadst.validator;

import android.content.Intent;
import android.net.Uri;
import android.provider.Contacts.Intents.UI;
import android.util.Log;

public class Properties {

    //public static final String SDCARD_PATH = "/sdcard/vld/";
//    public static final String SDCARD_PATH = "/storage/sdcard0/";
    
    public static final String SDCARD_PATH = "/storage/emulated/0/";
    
    public static final int APP_END = 109;
    public static Uri uri;

    public static Intent getBrowserIntent() {
        Intent intent = new Intent();
        intent.setClassName("com.android.browser",
                "com.android.browser.BrowserActivity");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    public static Intent getContactsIntent() {
        Intent intent = new Intent();
        intent.setClassName("com.android.contacts",
                "com.android.contacts.DialtactsActivity");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    public static Intent getCameraIntent() {
        Intent intent = new Intent();
        intent.setClassName("com.android.cameraswitch",
                "com.android.cameraswitch.CameraSwitch");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    public static Intent getCameraIntenta() {
        Intent intent = new Intent();
        intent.setClassName("com.android.camera2",
                "com.android.camera.CameraLauncher");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    public static Intent getMusicIntent() {
        Intent intent = new Intent();
        intent.setClassName("com.android.music",
                "com.android.music.MusicBrowserActivity");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    public static Intent getGallery3DIntent() {
        Intent intent = new Intent();
        intent.setClassName("com.cooliris.media",
                "com.cooliris.media.Gallery");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    public static Intent getGalleryIntent() {
        Intent intent = new Intent();
        intent.setClassName("com.android.gallery",
                "com.android.camera.GalleryPicker");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    public static Intent getDialorIntent() {
        Intent intent = new Intent();
        intent.setClassName("com.android.dialer",
                "com.android.dialer.DialtactsActivity");
        intent.setAction(UI.FILTER_CONTACTS_ACTION);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    public static Intent getVedioIntent() {
        Intent intent = new Intent();

        intent.setAction("android.intent.action.VIEW");
        intent.setClassName("com.android.gallery3d", "com.android.gallery3d.app.MovieActivity");
        intent.setDataAndType(Uri.parse("file://" + SDCARD_PATH + "test.mp4"), "video/*");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        Log.d("wsx", "intent = " + intent);
        return intent;
    }

    public static Intent getNeoCoreIntent() {
        Intent intent = new Intent();
        intent.setClassName("com.qualcomm.qx.neocore", "com.qualcomm.qx.neocore.Neocore");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    public static Intent getVoiceCircleIntent() {
        Intent intent = new Intent();
        intent.setClassName("com.spreadst.validdate", "com.spreadst.validdate.AudioCircleActivity");
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;

    }
}
