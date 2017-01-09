
package com.sprd.voicetrigger.utils;

import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;

import android.content.Context;
import android.os.Environment;
import android.util.Log;

public class EnvironmentUtils {
    private static final String TAG = "EnvironmentUtils";

    /**
     * Description: return whether the external storage is available
     *
     * @return true : available</p>
     * false : unavailable
     */
    public static boolean isExternalStorageAvailable() {
        boolean state = false;
        String extStorageState = Environment.getExternalStorageState();
        if (Environment.MEDIA_MOUNTED.equals(extStorageState)) {
            state = true;
        }
        return state;
    }

    public static Boolean writeStorage(Context context, byte[] data, String filename) {
        try {
            FileOutputStream fos = context.openFileOutput(filename, Context.MODE_PRIVATE);
            fos.write(data);
            fos.close();
        } catch (Exception e) {
            Log.e(TAG, "writeStorage: " + e.getMessage());
            e.printStackTrace();
            return false;
        }
        return true;
    }

    public byte[] readStorage(Context context, String filename) {
        int len = 1024;
        byte[] buffer = new byte[len];
        try {
            FileInputStream fis = context.openFileInput(filename);
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            int nrb = fis.read(buffer, 0, len); // read up to len bytes
            while (nrb != -1) {
                baos.write(buffer, 0, nrb);
                nrb = fis.read(buffer, 0, len);
            }
            buffer = baos.toByteArray();
            fis.close();
        } catch (Exception e) {
            Log.e(TAG, "readStorage: " + e.getMessage());
            e.printStackTrace();
        }
        return buffer;
    }

}
