package com.sprd.cmccvideoplugin;

import com.android.gallery3d.app.MoviePlayer;

public class CmccPhoneUtils {
    private static final String TAG = "CmccPhoneUtils";
    private static CmccPhoneUtils mInstance;
    long currentTime;
    public static CmccPhoneUtils getInstance(){
        if(mInstance == null){
            mInstance = new CmccPhoneUtils();
        }
        return mInstance;
    }
   public boolean ifIsPhoneTimeout(long current) {
        currentTime =current;
        return System.currentTimeMillis() - currentTime > 60 * 1000;
    }
}
