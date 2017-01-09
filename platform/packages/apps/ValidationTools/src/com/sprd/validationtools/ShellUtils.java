
package com.sprd.validationtools;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import android.util.Log;
import java.io.File;
import java.io.FileReader;
import java.io.FileOutputStream;
import java.io.BufferedOutputStream;

public class ShellUtils {

    static final String TAG = "ShellUtils";

    public static synchronized String execShellStr(String cmd) {
        Log.d(TAG, "execShellStr: " + cmd);
        String[] cmdStrings = new String[] {
                "sh", "-c", cmd
        };
        String retString = "";
        try {
            Process process = Runtime.getRuntime().exec(cmdStrings);
            BufferedReader stdout = new BufferedReader(new InputStreamReader(
                    process.getInputStream()), 7777);
            BufferedReader stderr = new BufferedReader(new InputStreamReader(
                    process.getErrorStream()), 7777);
            String line = null;
            while ((null != (line = stdout.readLine()))
                    || (null != (line = stderr.readLine()))) {
                if ("" != line) {
                    retString += line + "\n";
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return retString;

    }

    public static synchronized String readFile(String path) {
        File file = new File(path);
        String str = new String("");
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(file));
            String line = null;
            while ((line = reader.readLine()) != null) {
                str = str + line;
            }
        } catch (Exception e) {
            Log.d(TAG, "Read file error!!!");
            str = "readError";
            e.printStackTrace();
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (Exception e2) {
                    e2.printStackTrace();
                }
            }
        }
        Log.d(TAG, "read " + path + " value is " + str.trim());
        return str.trim();
    }

    public static synchronized void writeFile(String path, String cmd) {
        Log.d(TAG, "path: " + path + "; cmd: " + cmd);
        File file = new File(path);
        if (!file.exists()) {
            Log.d(TAG, "the file is not exists");
            return;
        }
        FileOutputStream fos = null;
        BufferedOutputStream bos = null;
        try {
            fos = new FileOutputStream(file);
            bos = new BufferedOutputStream(fos);
            byte[] bytes = cmd.getBytes();
            bos.write(bytes);
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (bos != null) {
                try {
                    bos.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            if (fos != null) {
                try {
                    fos.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    }

}
