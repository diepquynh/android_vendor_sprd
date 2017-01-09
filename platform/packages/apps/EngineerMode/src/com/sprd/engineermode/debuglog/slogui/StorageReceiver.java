
package com.sprd.engineermode.debuglog.slogui;

import java.io.File;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.preference.PreferenceManager;
import android.util.Log;

import com.sprd.engineermode.debuglog.slogui.StorageUtil;

public class StorageReceiver extends BroadcastReceiver {

    private static final String TAG = "SlogStorageReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "StorageReceiver intent: " + intent);
        /**
         * internal mount/unmount not care
         */
		if (intent.getData() == null) {
			return;
        }
        if (StorageUtil.isInInternalStorage(intent.getData().getPath())) {
            return;
        }
        File storageFile = whichStorage(intent.getData());
        if (storageFile == null) {
            return;
        }
        String path = storageFile.getAbsolutePath();
        String action = intent.getAction();
        SharedPreferences settings = context.getSharedPreferences("settings", Context.MODE_PRIVATE);
        boolean preAvailable = settings.getBoolean(path, false);
        if (Intent.ACTION_MEDIA_MOUNTED.equals(action)) {
            if (!preAvailable) {
                Log.d(TAG, "Sdcard has mounted");
                settings.edit().putBoolean(path, true).commit();
                StorageUtil.notifyStorageChanged(path, true);
            }
        } else if (Intent.ACTION_MEDIA_EJECT.equals(action)
                || Intent.ACTION_MEDIA_UNMOUNTED.equals(action)
                || Intent.ACTION_MEDIA_BAD_REMOVAL.equals(action)) {
            if (preAvailable && Intent.ACTION_MEDIA_UNMOUNTED.equals(action)) {
                Log.d(TAG, "Sdcard has unmounted");
                settings.edit().putBoolean(path, false).commit();
                StorageUtil.notifyStorageChanged(path, false);
            }
        }
    }

    private File whichStorage(Uri uri) {
        if (uri == null || uri.getPath() == null) {
            Log.e(TAG, "onReceive, intent.data.path is null");
            return null;
        }
        String path = uri.getPath();
        if (StorageUtil.isInExternalStorage(path)) {
            return StorageUtil.getExternalStorage();
        } else if (StorageUtil.isInInternalStorage(path)) {
            return StorageUtil.getInternalStorage();
        } else {
            Log.e(TAG, "can not found the uri: " + uri + " in which storage");
            return null;
        }
    }

}
