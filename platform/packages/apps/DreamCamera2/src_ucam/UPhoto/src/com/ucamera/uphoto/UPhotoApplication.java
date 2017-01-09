/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.app.Application;
import android.os.Handler;
import android.os.HandlerThread;

public class UPhotoApplication extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        catchCrashException();
    }

    private HandlerThread mHandleCrashThread;
    /*SPRD: CID 109197 (#1 of 1): UrF: Unread field (FB.URF_UNREAD_FIELD)
    private Handler mCrashHandler;
    */
    private void catchCrashException() {
        mHandleCrashThread = new HandlerThread("crash-detector");
        mHandleCrashThread.start();
        /*SPRD: CID 109197 (#1 of 1): UrF: Unread field (FB.URF_UNREAD_FIELD)
        mCrashHandler = new Handler(mHandleCrashThread.getLooper());
        */
        Thread.setDefaultUncaughtExceptionHandler(new com.ucamera.uphoto.exception.UncatchException(this));
    }

    private void uncatchCrashException() {
        if (mHandleCrashThread != null) {
            mHandleCrashThread.quit();
        }
    }
}
