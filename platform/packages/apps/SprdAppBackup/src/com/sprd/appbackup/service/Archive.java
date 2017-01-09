package com.sprd.appbackup.service;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

public class Archive implements Parcelable {
    String mPath;
    String mTimeStamp;
    protected static final String TAG = "Archive";

    public static Archive get(String rootPath, String fileName,boolean fileNameWithPath) {
        if (fileNameWithPath) {
            rootPath = fileName.substring(0, fileName.lastIndexOf("/") + 1);
            Log.i(TAG, "fileName=" + fileName);
            Log.i(TAG, "rootPath=" + rootPath);
        }
        if (null == rootPath) {
            Log.e(TAG, "Archive get null == rootPath ");
            return null;
        }
        if (!rootPath.endsWith("/backup/Data/") && !rootPath.endsWith("/.backup/")) {
            Log.e(TAG, "Archive get null !rootPath.endsWith");
            return null;
        }
        return new Archive(rootPath, fileName, fileNameWithPath);
    }

    public static Archive get(String rootPath) {
        if (null == rootPath) {
            return null;
        }
        if (!rootPath.endsWith("/backup/Data/")) {
            return null;
        }
        return new Archive(rootPath);
    }

    public static void delete(Archive archive) {
        Log.d("Archive", "Archive.delete()");
        Log.d("Archive", "path = "+ archive.getPath());
        File file = new File(archive.getPath());
        if(file != null && file.exists()){
            deleteDir(file);
            Log.d("Archive", "Archive.delete() is over");
        }
    }
    public static void deleteDir(File dir) {
        if (dir != null && dir.exists() && dir.getAbsolutePath().contains("/.backup/")) {
            File originFile = new File(dir.getAbsolutePath()+".zip");
            File targetFile = new File(dir.getAbsolutePath()+".delete");

            if (originFile.exists() && originFile.isFile()) {
                originFile.renameTo(targetFile);
                if (targetFile.exists() && targetFile.isFile()) {
                    targetFile.delete();
                }
            }
        }
        if (dir == null || !dir.exists() || !dir.isDirectory() || null == dir.list())
            return;

        if (dir.exists() && dir.isDirectory()) {
            if (dir.list().length == 0) {
                return;
            }
        }
        Log.d("Archive", "dir path = "+ dir.getPath());
        for (File file : dir.listFiles()) {
            if (file.isFile()){
                Log.d("Archive", "file path = "+ file.getPath());
                boolean d = file.delete();
                Log.d("Archive", "file path delete = "+ d);
            }else if (file.isDirectory()){
                deleteDir(file);
            }
        }
        Log.d("Archive", "root dir path = "+ dir.getPath());
        boolean del = dir.delete();
        Log.d("Archive", "delete root dir path = "+ del);
    }

    public static void deleteDir(Archive archive, String agentFolder){
        if(archive == null || agentFolder == null){
            Log.d("Archive", "delete agentfolder archive == null || agentFolder == null");
            return;
        }
        Log.d("Archive", "delete dir = " + archive.getPath() + "/" + agentFolder);
        File file = new File(archive.getPath() + "/" + agentFolder);
        if(file != null && file.exists()){
            deleteDir(file);
            Log.d("Archive", "delete agentfolder is over");
        }
    }
    public static void deleteEmptyDirs(Archive archive) {
        Log.d("Archive", "deleteEmptyDir");
        Log.d("Archive", "path = " + archive.getPath());
        File dir = new File(archive.getPath());
        if(dir != null && dir.exists()){
            deleteEmptyDir(dir);
            deleteEmptyDir(dir);
            Log.d("Archive", "Archive.deleteEmptyDir is end");
        }
    }

    public static void deleteEmptyDir(File dir) {
        if (dir == null || !dir.exists() || !dir.isDirectory() || null == dir.list()){
            return;
        }
        int length = dir.list().length;
        if(length > 0){
            for (File file : dir.listFiles()) {
                if (file.isFile())
                    continue;
                else if (file.isDirectory())
                    deleteEmptyDir(file);
            }
        } else {
            dir.delete();
        }
    }
    
    private Archive(String rootPath) {
        // mTimeStamp=(int)(System.currentTimeMillis()/1000);
        SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMddHHmmss", Locale.getDefault());
        Date date = new Date(System.currentTimeMillis());
        mTimeStamp = sdf.format(date);
        mPath = rootPath + mTimeStamp;
        File f = new File(mPath);
        f.mkdirs();
    }

    private Archive(String rootPath,String timeStamp,boolean timeStampWithPath) {
        if (!timeStampWithPath) {
            mTimeStamp = timeStamp;
            mPath = rootPath + timeStamp;            
        } else {
            mTimeStamp = timeStamp.substring(timeStamp.lastIndexOf("/") + 1);
            mPath = timeStamp;
        }
        File f = new File(mPath);
        f.mkdirs();
    }

    public String getPath() {
        return mPath;
    }
    public String toString() {
        return mTimeStamp;
    }

    public int describeContents() {
        return 0;
    }

    public void writeToParcel(Parcel out, int flags) {
        out.writeString(mTimeStamp);
        out.writeString(mPath);
    }

    public static final Parcelable.Creator<Archive> CREATOR = new Parcelable.Creator<Archive>() {
        public Archive createFromParcel(Parcel in) {
            return new Archive(in);
        }

        public Archive[] newArray(int size) {
            return new Archive[size];
        }
    };

    private Archive(Parcel in) {
        mTimeStamp = in.readString();
        mPath = in.readString();
    }

    @Override
    public int hashCode() {
        return mPath.hashCode();
    }

    @Override
    public boolean equals(Object other) {
        if (this == other) {
            return true;
        }

        if (!(other instanceof Archive)) {
            return false;
        }

        Archive archive = (Archive) other;
        return mTimeStamp.equals(archive.mTimeStamp);
    }
}
