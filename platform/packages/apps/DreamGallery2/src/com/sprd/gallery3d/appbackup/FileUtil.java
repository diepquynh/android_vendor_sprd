package com.sprd.gallery3d.appbackup;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

import android.util.Log;

public class FileUtil {
    private static final String TAG = "FileUtil";

    public static final String UNPACK_RETURN_EXCEPTION = "Exception";
    public static final String INCOMPLETE_EXCEPTION = "Incomplete";

    private static final String CONVERT_PREFIX = "BACKTAG_";
    private static final String CONVERT_SUBFIX = "_GATKCAB";
    private static final String CONVERT_SPERATOR = "_-SPERATOR-_";

    public static String fixFileName(String path) {
        return CONVERT_PREFIX + path + CONVERT_SUBFIX;
    }

    public static String convertPath(String path) {
        String result = "";
        if (path == null) {
            Log.e(TAG, "Can't convert null");
            return "";
        }
        if (path.startsWith(CONVERT_PREFIX) && path.endsWith(CONVERT_SUBFIX)) {
            String temp = path.replace(CONVERT_PREFIX, "");
            result = temp.replace(CONVERT_SUBFIX, "");
            temp = result;
            result = temp.replace(CONVERT_SPERATOR, File.separator);
        } else if (path.contains(File.separator)) {
            String temp = path.replace(File.separator, CONVERT_SPERATOR);
            result = CONVERT_PREFIX + temp + CONVERT_SUBFIX;
        }
        if (result == null) {
            result = "";
        }
        // TODO REMOVE AFTER DEBUG.
        Log.d(TAG, "result = " + result + "and path = " + path);
        return result;
    }

    public static boolean checkDirAutoCreate(File file) {
        if (file == null) {
            Log.d(TAG, "file == null");
            return false;
        }

        if (file.getAbsolutePath().equals("/")) {
            Log.d(TAG, "file equals /");
            return false;
        }

        if (!file
                .getAbsolutePath()
                .trim()
                .startsWith(
                        StorageUtil.getExternalStorage().getAbsolutePath()
                                .trim())
                && !file.getAbsolutePath()
                        .trim()
                        .startsWith(
                                StorageUtil.getInternalStorage()
                                        .getAbsolutePath().trim())) {
            Log.d(TAG, "Failed compare");
            return false;
        }

        file.delete();
        file.getParentFile().mkdirs();

        try {
            Log.d(TAG, "create " + file.getAbsolutePath());
            file.createNewFile();

            return file.exists();
        } catch (IOException e) {
            Log.e(TAG, e.getMessage());
            e.printStackTrace();
            return false;
        }

    }

    public static boolean writeToZip(File file, ZipOutputStream zos) {
        boolean result = false;
        if (file.exists()) {
            if (file.isDirectory()) {
                return false;
            } else {
                FileInputStream fis = null;
                DataInputStream dis = null;
                try {
                    fis = new FileInputStream(file);
                    dis = new DataInputStream(new BufferedInputStream(fis));
                    ZipEntry ze = new ZipEntry(
                            convertPath(file.getAbsolutePath()));
                    zos.putNextEntry(ze);
                    byte[] content = new byte[1024];
                    int len;
                    while ((len = fis.read(content)) != -1) {
                        zos.write(content, 0, len);
                        zos.flush();
                    }
                    result = true;
                } catch (FileNotFoundException e) {
                    e.printStackTrace();
                    result = false;
                } catch (IOException e) {
                    e.printStackTrace();
                    result = false;
                } finally {
                    try {
                        if (dis != null) {
                            dis.close();
                        }
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
        return result;
    }

    public static String unpackZipEntry(ZipEntry entry, ZipInputStream zis) {
        BufferedOutputStream bos = null;
        String fileName;
        String filePath;
        byte[] buf = null;
        int bufLen = 2048;
        int count = -1;
        Log.d(TAG, "unpackZipEntry");
        try {
            buf = new byte[bufLen];
            fileName = entry.getName().toString().trim();
            filePath = convertPath(fileName);
            if (filePath == null || "".equals(filePath)) {
                return UNPACK_RETURN_EXCEPTION;
            }
            Log.d(TAG, "converted path = " + filePath);
            File targetFile = new File(filePath);

            if (!checkDirAutoCreate(targetFile)) {
                Log.e(TAG, "Create file failed!");
                return UNPACK_RETURN_EXCEPTION;
            }
            Log.d(TAG, "Prepare BufferedOutputStream");
            bos = new BufferedOutputStream(new FileOutputStream(targetFile),
                    bufLen);
            while ((count = zis.read(buf, 0, bufLen)) != -1) {
                if (count == 0) {
                    Log.d(TAG, "read is not normally finished");
                    return INCOMPLETE_EXCEPTION;
                }
                bos.write(buf, 0, count);
            }
            Log.d(TAG, "write finished.");
            bos.flush();
        } catch (Exception e) {
            Log.e(TAG, "Catch exception, " + e.getMessage());
            e.printStackTrace();
            return UNPACK_RETURN_EXCEPTION;
        } finally {
            if (bos != null) {
                try {
                    bos.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        return filePath;
    }

}
