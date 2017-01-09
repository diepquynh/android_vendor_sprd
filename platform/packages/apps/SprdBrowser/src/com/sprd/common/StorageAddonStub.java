package com.sprd.common;

import com.android.browser.R;
import android.os.Environment;
import java.io.File;
import android.util.Log;

public class StorageAddonStub {

    public StorageAddonStub(){
    }

    public boolean useEnvironmentEx() {
        return false;
    }

    public File getInternalStoragePath() {
        return null;
    }

    public String getInternalStoragePathState(){
        return Environment.MEDIA_REMOVED;
    }

    public File getExternalStoragePath(){
        return null;
    }

    public String getExternalStoragePathState(){
        return Environment.MEDIA_REMOVED;
    }

    public File[] getUsbdiskVolumePaths(){
        return null;
    }

    public String getUsbdiskVolumeState(File path){
        return Environment.MEDIA_REMOVED;
    }
}
