package com.sprd.appbackup;

import java.io.File;

import android.app.Application;
import android.app.NotificationManager;
import android.content.Context;
import android.os.Environment;
import android.os.FileUtils;
import android.util.Log;

public class SprdAppBackupApplication extends Application {

    private static final String TAG = "SprdAppBackupApplication";

    private File mThirdAppBackupFolder;
    
    @Override
    public void onCreate() {
        super.onCreate();
        NotificationManager notifyManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        if (notifyManager != null) {
            notifyManager.cancelAll();
            Log.i(TAG, "notifyManager.cancelAll()");
        }
        mThirdAppBackupFolder = new File(Environment.getDataDirectory(), "data" + File.separatorChar + 
        		getPackageName() + File.separatorChar + "thirdapp");
        if(!createFolder(mThirdAppBackupFolder)) {
        	mThirdAppBackupFolder = null;
        }
    }
    
    private boolean createFolder(File file) {
        if(!file.exists()) {
        	boolean isOk = file.mkdir();
        	if(isOk) {
        		if(FileUtils.setPermissions(file.getAbsolutePath(), FileUtils.S_IRWXU | FileUtils.S_IRWXG
                        | FileUtils.S_IRWXO, -1, -1) != 0) {
        			file.delete();
        			isOk = false;
        		}
        	}
        	Log.d(TAG, "mkdir file " + (isOk ? "succeed" : "failed") + ": " + file);
        	return isOk;
        }
        return true;
    }
    
    public File getBackupUserAppFolder(String pkgName) {
    	if(mThirdAppBackupFolder == null) {
    		return null;
    	}
    	File ret = new File(mThirdAppBackupFolder, pkgName);
    	if(createFolder(ret)) {
    		return ret;
    	} else {
    		return null;
    	}
    }
    
    public File getBackupUserAppFolder() {
    	return mThirdAppBackupFolder;
    }
    
}
