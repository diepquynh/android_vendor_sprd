package com.sprd.appbackup.utils;

import java.io.File;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.FileObserver;
import android.util.Log;

public class StorageReceiver extends BroadcastReceiver {
    private static final String TAG = "StorageReceiver";
    private StorageObserver mExternalObserver;
    private StorageObserver mInternalObserver;

    /*
     *
     * <receiver android:name="test.spreadtrum.storagemodule.StorageReceiver" >
     * <intent-filter> <action android:name="android.intent.action.MEDIA_EJECT"
     * /> <action android:name="android.intent.action.MEDIA_MOUNTED" /> <action
     * android:name="android.intent.action.MEDIA_BAD_REMOVAL" /> <action
     * android:name="android.intent.action.MEDIA_UNMOUNTED" />
     *
     * <data android:scheme="file" /> </intent-filter> </receiver>
     */

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.e(TAG, "Recevie storage intent: " + intent.getAction());
        Log.e(TAG, "data = " + intent.getDataString());
        Log.e(TAG, "scheme = " + intent.getScheme());
        SharedPreferences settings = context.getSharedPreferences("storage_status", 0);
        if (Intent.ACTION_MEDIA_MOUNTED.equals(intent.getAction())) {
            if (intent.getDataString().endsWith(
                    StorageUtil.getExternalStorage().getAbsolutePath())) {
                mExternalObserver = new StorageObserver(
                        StorageUtilImpl.getExternalStorage(), settings);
                mExternalObserver.startWatching();
            } else if (intent.getDataString().endsWith(
                    StorageUtil.getInternalStorage().getAbsolutePath())) {
                try {
                    mInternalObserver = new StorageObserver(
                            StorageUtilImpl.getInternalStorage(), settings);
                    mInternalObserver.startWatching();
                } catch (Exception e) {
                    Log.e(TAG, "init ");
                }
            }
        } else if (Intent.ACTION_MEDIA_EJECT.equals(intent.getAction())
                || Intent.ACTION_MEDIA_UNMOUNTED.equals(intent.getAction())
                || Intent.ACTION_MEDIA_BAD_REMOVAL.equals(intent.getAction())) {
            if (intent.getDataString().endsWith(
                    StorageUtil.getExternalStorage().getAbsolutePath())) {
                if (settings.getBoolean(StorageUtil.getExternalStorage()
                        .getAbsolutePath(), false)) {
                    StorageUtilImpl.notifyStorageChanged(
                            StorageUtilImpl.getExternalStorage(), false,
                            settings);
                }
            } else if (intent.getDataString().endsWith(
                    StorageUtil.getInternalStorage().getAbsolutePath())) {
                if (settings.getBoolean(StorageUtil.getInternalStorage()
                        .getAbsolutePath(), false)) {

                    StorageUtilImpl.notifyStorageChanged(
                            StorageUtilImpl.getInternalStorage(), false,
                            settings);
                }
            }
        }

    }

    private class StorageObserver extends FileObserver {
        private File mStorage;
        private SharedPreferences mSettings;
        private boolean mLastExternalState;
        private boolean mLastInternalState;

        public StorageObserver(File storage, SharedPreferences settings) {
            super(storage.getParentFile().getAbsolutePath());
            boolean isNotify;
            boolean available;
            mStorage = storage;
            mSettings = settings;
            mLastExternalState = settings.getBoolean(
                    mStorage.getAbsolutePath(), false);
            mLastInternalState = settings.getBoolean(
                    mStorage.getAbsolutePath(), false);
            if (mStorage.equals(StorageUtilImpl.getExternalStorage())) {
                available = StorageUtilImpl.getExternalStorageState();
                isNotify = available || mLastExternalState;
            } else {
                available = StorageUtilImpl.getInternalStorageState();
                isNotify = available || mLastInternalState;
            }
            if (isNotify) {
                StorageUtilImpl.notifyStorageChanged(mStorage, available, mSettings);
            }
        }

        @Override
        public void onEvent(int event, String path) {
            boolean available;
            boolean isNotify;
            if (mStorage.equals(StorageUtilImpl.getExternalStorage())) {
                available = StorageUtilImpl.getExternalStorageState();
                isNotify = available || mLastExternalState;
            } else {
                available = StorageUtilImpl.getInternalStorageState();
                isNotify = available || mLastInternalState;
            }
            if (isNotify) {
                StorageUtilImpl.notifyStorageChanged(mStorage, available,
                        mSettings);
            }
            Log.e(TAG, "state = " + StorageUtil.getExternalStorageState()
                    + " path = " + path);
            stopWatching();

        }
    }

}
