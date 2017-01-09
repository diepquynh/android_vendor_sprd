/*
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */
package com.ucamera.ucomm.downloadcenter;

import java.io.Closeable;
import java.io.File;
import java.io.FileNotFoundException;
import java.util.List;
import android.app.Activity;
import android.content.SharedPreferences;
import android.util.Log;

public class DownLoadUtil {
    private static final String TAG = "DownLoadUtil";
    // CID 109346 : UuF: Unused field (FB.UUF_UNUSED_PUBLIC_OR_PROTECTED_FIELD)
    // public static String[] mFrameImage ;
    /**
     * update the view status by selecting item
     * @param activity current activity
     * @param multiSelect current be selected item object.
     */
    public static void setViewStatus(Activity activity, List<?> multiSelect) {
        int[] viewIds = {R.id.btn_multi_confirm, R.id.btn_multi_cancel};
        if(multiSelect != null) {
            int currentSize = multiSelect.size();
            boolean hasFile = currentSize > 0;
            for(int viewId : viewIds) {
                activity.findViewById(viewId).setEnabled(hasFile);
            }
        } else {
            for(int viewId : viewIds) {
                activity.findViewById(viewId).setEnabled(false);
            }
        }
    }

    public static void closeSilently(Closeable c) {
        if (c == null) return;
        try {
            c.close();
        } catch (Throwable t) {
            // do nothing
        }
    }

    public static void createDefaultDirectory(String defaultDirectory) {
        File file = new File(defaultDirectory);
        if(!file.exists()) {
            file.mkdirs();
        }
    }

    public static boolean upgradeDownloadCenterThumbnail(SharedPreferences pref) {
        int version;
        boolean upgrade = false;
        try {
            version = pref.getInt(Constants.KEY_LOCAL_VERSION, 0);
        } catch (Exception ex) {
            version = 0;
        }
        if (version == Constants.CURRENT_LOCAL_VERSION)
            return false;

        SharedPreferences.Editor editor = pref.edit();
        if (version < Constants.CURRENT_LOCAL_VERSION) {
            upgrade = true;
        }
        editor.putInt(Constants.KEY_LOCAL_VERSION, Constants.CURRENT_LOCAL_VERSION);
        editor.commit();
        return upgrade;
    }

    public static boolean deletefile(String delpath) throws Exception {
        try {
            File file = new File(delpath);
            if (!file.isDirectory()) {
                file.delete();
            } else if (file.isDirectory()) {
                String[] filelist = file.list();
                for (int i = 0; filelist != null && i < filelist.length; i++) {
                    File delfile = new File(delpath + "/" + filelist[i]);
                    deletefile(delfile.getAbsolutePath());
                }
                file.delete();
            }

        } catch (FileNotFoundException e) {
            Log.d(TAG, "deletefile() Exception:" + e.getMessage());
        }
        return true;
    }
}
