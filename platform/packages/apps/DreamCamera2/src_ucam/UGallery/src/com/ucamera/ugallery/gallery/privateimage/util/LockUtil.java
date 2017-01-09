/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery.gallery.privateimage.util;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;

import android.app.Activity;
import android.provider.MediaStore;
import android.util.Log;

public class LockUtil {
    private static final String TAG = "LockUtil";

    public static boolean backupAlbum(String paramPath, boolean isShowLockedImage) {
        boolean sucess = false;
        String str1 = initTargetDir(paramPath, isShowLockedImage);
        File paramFile = new File(paramPath);
        File[] arrayOfFile = paramFile.listFiles();
        for (int i = 0; (arrayOfFile != null) && (i < arrayOfFile.length); i++) {
            File localFile2 = arrayOfFile[i];
            if(localFile2.isFile()) {
                String str2 = str1 + File.separator + localFile2.getName();
                String suffix =  localFile2.getName().substring(localFile2.getName().lastIndexOf(".") +1);
                if(!isImage(suffix)) {
                    continue;
                }
                if(new File(str2).exists()) {
                    int index = localFile2.getName().lastIndexOf(".");
                    if(index > 0 ) {
                        str2 = str1 + File.separator + "IMG_" + getTime(System.currentTimeMillis())+ i + "." +suffix;
                    }
                }
                 sucess = localFile2.renameTo(new File(str2));
                if(!sucess) {
                    sucess =  copyAndDeleteFile(localFile2 ,new File(str2) );
                }
            }
        }
        return sucess;
    }
    public static boolean restoreAlbum(String paramPath) {
        boolean sucess = false;
        int index = Constants.STORE_DIR_LOCKED.length();
        File paramFile = new File(paramPath);
        File[] arrayOfFile = paramFile.listFiles();
        for (int i = 0; (arrayOfFile != null) && (i < arrayOfFile.length); i++) {
            File localFile2 = arrayOfFile[i];
            if(localFile2.isFile()) {
                if(localFile2.getAbsolutePath().startsWith(Constants.STORE_DIR_LOCKED)) {
                    String str2 = localFile2.getAbsolutePath().substring(index);
                    String suffix =  localFile2.getName().substring(localFile2.getName().lastIndexOf(".") +1);
                    if(!isImage(suffix)) {
                        continue;
                    }
                    if(new File(str2).exists()) {
                        int index2 = localFile2.getName().lastIndexOf(".");
                        if(index2 > 0 ) {
                            str2 = str2.substring(0 , str2.lastIndexOf("/")) + File.separator + "IMG_" + getTime(System.currentTimeMillis()) + i + "." +suffix;
                        }
                    }
                    sucess = localFile2.renameTo(new File(str2));
                    if(!sucess) {
                        sucess = copyAndDeleteFile(localFile2 ,new File(str2) );
                    }
                }
            }
        }
        return sucess;
    }
    public static boolean copyAndDeleteFile(File oldFile, File newFile) {
        if(oldFile.getAbsolutePath().equals(newFile.getAbsolutePath())) {
            return true;
        }
        // CID 108999: Resource leak on an exceptional path (RESOURCE_LEAK)
        InputStream inStream = null;
        FileOutputStream fs = null;
        try {
            int bytesum = 0;
            int byteread = 0;
            if (oldFile.exists()) {
                if(!newFile.getParentFile().exists())
                    newFile.getParentFile().mkdirs();
                inStream = new FileInputStream(oldFile);
                fs = new FileOutputStream(newFile);
                byte[] buffer = new byte[1024];
                int length;
                while ( (byteread = inStream.read(buffer)) != -1) {
                    bytesum += byteread;
                    fs.write(buffer, 0, byteread);
                }
                // inStream.close();
                oldFile.delete();
            }
        } catch (Exception e) {
            Log.d(TAG, "copy failed or delete failed" + e);
            return false;
        }
        /* SPRD: CID 108999: Resource leak on an exceptional path (RESOURCE_LEAK)
         * CID 109002 (#1 of 1): Resource leak (RESOURCE_LEAK)
         *  @{ */
        try{
            if(inStream != null){
                inStream.close();
            }
            if(fs != null){
                fs.close();
            }
        }catch(Exception e) {
            Log.d(TAG, "copy failed or delete failed" + e);
            return false;
        }
        /* @} */
        return true;

    }
    private static String getTime (long milliseconds){
        SimpleDateFormat format = new SimpleDateFormat("yyyyMMdd_HHmmss");
        return  format.format(new Date(milliseconds));

    }
    public static String initTargetDir(String paramString, boolean isShowLockedImage) {
        File lockFile = new File(Constants.STORE_DIR_LOCKED);
        if(!lockFile.exists()) {
            lockFile.mkdirs();
        }
        try {
            File fileNoMedia = new File(Constants.STORE_DIR_LOCKED + "/" + ".nomedia");
            if (!fileNoMedia.exists() && !isShowLockedImage) {
                fileNoMedia.createNewFile();
            }
        } catch (Exception e) {
            Log.d(TAG, "create file fialed : " + e);
        }
        String str = concatPath(Constants.STORE_DIR_LOCKED,
                paramString, new String[0]);
        File localFile = new File(str);
        if (!localFile.exists())
            localFile.mkdirs();
        return str;
    }

    public static File backupSingleFile(String path, String prefixPath, boolean isShowLockedImage) {
        File lockFile = new File(prefixPath);
        if (!lockFile.exists()) {
            lockFile.mkdirs();
        }

        try {
            File fileNoMedia = new File(prefixPath + "/" + ".nomedia");
            if (!fileNoMedia.exists() && !isShowLockedImage) {
                fileNoMedia.createNewFile();
            }
        } catch (Exception e) {
            Log.d(TAG, "create file fialed : " + e);
        }
        File localFile = new File(path);
        File newFile = new File(prefixPath.concat(path));
        boolean sucess = localFile.renameTo(newFile);
        if (!sucess) {
            copyAndDeleteFile(localFile, newFile);
        }
        return newFile;
    }

    public static File backupUnlockFile(int i, String path ,String prefixPath, boolean isImage) {
        File localFile  = new File(path);
        String newPath = prefixPath.concat(path);
        String suffix =  localFile.getName().substring(localFile.getName().lastIndexOf(".") +1);
        if(new File(newPath).exists()) {
            int index = localFile.getName().lastIndexOf(".");
            if(index > 0 ) {
                if(isImage) {
                    newPath = newPath.substring(0, newPath.lastIndexOf(File.separator)) + File.separator + "IMG_" + getTime(System.currentTimeMillis())+ i + "." +suffix;
                } else {
                    newPath = newPath.substring(0, newPath.lastIndexOf(File.separator)) + File.separator + "VID_" + getTime(System.currentTimeMillis())+ i + "." +suffix;
                }
            }
        }
        File newFile = new File(newPath);
        boolean sucess = localFile.renameTo(newFile);
        if(!sucess) {
            copyAndDeleteFile(localFile , newFile);
        }
        return newFile;
    }

    public static File restoreSingleFile(String paramPath, String prefixPath) {
        int index = prefixPath.length();
        File paramFile = new File(paramPath);
        File newFile ;
        if(paramPath.startsWith(prefixPath)) {
            newFile = new File(paramPath.substring(index));
        } else {
            newFile = paramFile;
        }
        boolean sucess = paramFile.renameTo(newFile);
        if(!sucess) {
            copyAndDeleteFile(paramFile, newFile);
        }
        return newFile;
    }

    public static String concatPath(String paramString1, String paramString2, String[] paramArrayOfString)
    {
      String str = StringUtils.trimRightSlash(paramString1) + "/" + StringUtils.trimSlash(paramString2);
      for (int i = 0; i < paramArrayOfString.length; i++)
        str = str + "/" + StringUtils.trimSlash(paramArrayOfString[i]);
      return str;
    }
    private static boolean isImage(String suffix) {
        return Arrays.asList(new String[] {
                "jpeg", "jpg", "png", "gif", "bmp",
                "raw", "wbmp"
        }).contains(suffix.toLowerCase());
    }
    public static void createNoMediaFile (String prefixPath) {
        File file = new File(prefixPath);
        if(!file.exists()){
            file.mkdirs();
        }
        File fileNoMedia = new File(prefixPath + "/" + ".nomedia");
        try {
            if (!fileNoMedia.exists()) {
                fileNoMedia.createNewFile();
            }
        } catch (Exception e) {
            Log.d(TAG, "create No Media failed :" + e);
        }
    }

    public static boolean isExistHidenImages(String prefixPath) {
        File paramFile = new File(prefixPath);
        if(!paramFile.exists()) {
            return false;
        }
        try {
            File fileNoMedia = new File(prefixPath + "/" + ".nomedia");
            if (fileNoMedia.exists()) {
                fileNoMedia.delete();
            }
        } catch (Exception e) {
            Log.d(TAG, "create file fialed : " + e);
        }
        return listAllImages(new File(prefixPath), false);
    }

    private static boolean listAllImages(File paramFile, boolean isImageExist) {
        File[] arrayOfFile = paramFile.listFiles();
        for (int i = 0; (arrayOfFile != null) && (i < arrayOfFile.length); i++) {
            File localFile = arrayOfFile[i];
            if (localFile != null && localFile.isFile()) {
                isImageExist = true;
            } else {
                isImageExist = listAllImages(localFile, isImageExist);
            }
            if(isImageExist)
                break;
        }
        return isImageExist;
    }
    public static ArrayList<String> restoreAllImages(String paramPath, File paramFile, ArrayList<String> list, Activity activity) {

        File[] arrayOfFile = paramFile.listFiles();
        for (int i = 0; (arrayOfFile != null) && (i < arrayOfFile.length); i++) {
            File localFile = arrayOfFile[i];
            if(localFile != null && localFile.isFile()) {
                String localFilePath = localFile.getAbsolutePath();
                String newPath = localFilePath.substring(paramPath.length());
                File newFile = new File(newPath);
                boolean sucess = localFile.renameTo(newFile);
                if (!sucess) {
                    sucess = copyAndDeleteFile(localFile, newFile);
                }
                if (sucess) {
                    list.add(newPath);
                    String params[] = new String[]{localFilePath};
                    String suffix = null;
                    if(localFilePath.lastIndexOf(".") != -1) {
                        suffix =  localFilePath.substring(localFilePath.lastIndexOf(".") +1);
                    }
                    if(isImage(suffix)) {
                        activity.getContentResolver().delete(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, MediaStore.Images.Media.DATA + " LIKE ?", params);
                    } else {
                        activity.getContentResolver().delete(MediaStore.Video.Media.EXTERNAL_CONTENT_URI, MediaStore.Video.Media.DATA + " LIKE ?", params);
                    }

                }
            } else {
                restoreAllImages(paramPath, localFile , list, activity);
            }
        }
        return list;
    }

}
