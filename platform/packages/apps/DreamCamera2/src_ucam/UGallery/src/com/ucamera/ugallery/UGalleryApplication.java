/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery;

import com.ucamera.ugallery.integration.Build;
import com.ucamera.ugallery.util.ShareVideoUtils;
import com.ucamera.ugallery.util.UiUtils;

import android.app.Application;
import android.os.Handler;
import android.os.HandlerThread;

public class UGalleryApplication extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        UiUtils.initialize(this);
        catchCrashException();
    }


    private HandlerThread mHandleCrashThread;
    // CID 109191 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private Handler mCrashHandler;
    // new thread to catch force close exception and then show notification to users
    private void catchCrashException() {
        mHandleCrashThread = new HandlerThread("huilurry");
        mHandleCrashThread.start();
        // CID 109191 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mCrashHandler = new Handler(mHandleCrashThread.getLooper());
        Thread.setDefaultUncaughtExceptionHandler(new UncatchException(this));
    }

    private void uncatchCrashException() {
        if (mHandleCrashThread != null) {
            mHandleCrashThread.quit();
        }
    }
}
