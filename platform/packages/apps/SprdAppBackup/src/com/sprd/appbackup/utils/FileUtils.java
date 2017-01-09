package com.sprd.appbackup.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Collections;
import java.util.Comparator;
import java.util.LinkedList;
import java.util.List;
import java.util.Stack;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

import android.util.Log;


public class FileUtils {
    
    private static final String TAG = "BackupFileUtils";
    private final static int READ_BYTES = 1024;
    
    private FileUtils(){ }
    
    public static boolean erase(File file) {
        return erase(file, false);
    }
    
    public static boolean erase(File file, boolean isClear) {
        List<File> allFile = getAllFiles(file, true);
        if(allFile.isEmpty()) {
            return true;
        }
        Collections.sort(allFile, new Comparator<File>() {

            @Override
            public int compare(File o1, File o2) {
                return -o1.getPath().compareTo(o2.getPath());
            }
            
        });
        boolean ret = true;
        if(isClear) {
            allFile.remove(allFile.size()-1);
        }
        for(File f: allFile) {
            ret = f.delete();
        }
        return ret;
    }
    
    public static List<File> getAllFiles(File sourceFile, boolean containFolder) {
        Stack<File> dirStack = new Stack<File>();
        List<File> allFile = new LinkedList<File>();
        if(!sourceFile.exists()) {
            Log.e(TAG, "file: " + sourceFile + " does not exist");
            return allFile;
        }
        if(sourceFile.isDirectory()) {
            dirStack.add(sourceFile);
        } else {
            allFile.add(sourceFile);
        }
        while(!dirStack.empty()) {
            File file = dirStack.pop();
            /* SPRD: fix bug 594254 @{ */
            if(file == null || file.listFiles() == null) {
                Log.e(TAG, "file.listFiles() is null and may attempt to get length of null array");
                return allFile;
            }
            /* @} */
            if(containFolder) {
                allFile.add(file);
            }
            for(File f : file.listFiles()) {
                if(f.isDirectory()) {
                    dirStack.push(f);
                } else {
                    allFile.add(f);
                }
            }
        }
        return allFile;
    }
    
    public static boolean compression(File srcFile, File thirdAppBackupFolder, File targetFile) {
        List<File> allFiles = getAllFiles(srcFile, false);
        if(allFiles.isEmpty()) {
            Log.w(TAG, "don't have file to compression");
            return true;
        }
        boolean ret = true;
        InputStream in = null;
        ZipOutputStream zipOut = null;
        try {
            zipOut = new ZipOutputStream(new FileOutputStream(targetFile));
            for(File f : allFiles) {
                int index = thirdAppBackupFolder.getPath().length() + 1;
                if(index < 0) {
                    continue;
                }
                zipOut.putNextEntry(new ZipEntry(f.getPath().substring(index)));
                try {
                    in = new FileInputStream(f);
                    int len;
                    byte[] content = new byte[READ_BYTES];
                    while ((len = in.read(content)) != -1 ) {
                        zipOut.write(content, 0, len);
                    }
                    zipOut.flush();
                } catch (IOException e) {
                    Log.e(TAG, "compression, in.close(); have exception", e);
                    ret = false;
                } finally {
                    if(in != null) {
                        in.close();
                        in = null;
                    }
                }
            }
        } catch (IOException e) {
            Log.d(TAG, "compression srcFile: " + srcFile + " to " + targetFile + " have exception" , e);
            ret = false;
        } finally {
            try {
                if(zipOut != null)
                    zipOut.close();
                if(in != null)
                    in.close();
            } catch (IOException e) {
                Log.e(TAG, "decompression, close stream have exception", e);
                ret = false;
            }
        }
        return ret;
    }
    
    public static boolean decompression(File srcFile, File targetFolder) {
        if(!targetFolder.exists() && !targetFolder.mkdirs()) {
            Log.e(TAG, "targetFolder " + targetFolder + " is not exist, and create failed");
            return false;
        } else if(!targetFolder.isDirectory()) {
            Log.e(TAG, "targetFolder " + targetFolder + " is not a folder");
            return false;
        } else if(!srcFile.exists()){
            Log.e(TAG, "zip file: " + srcFile + " is not exist");
            return false;
        }
        boolean ret = true;
        ZipInputStream zipIn = null;
        OutputStream out = null;
        InputStream in = null;
        ZipFile zipFile = null;
        try {
            zipFile = new ZipFile(srcFile);
            zipIn = new ZipInputStream(new FileInputStream(srcFile));
            ZipEntry zipEntry = null;
            while((zipEntry = zipIn.getNextEntry()) != null) {
                File tmp = new File(targetFolder, zipEntry.getName());
                Log.d(TAG, "decompression file: " + tmp);
                if (!tmp.getParentFile().exists()) {
                    tmp.getParentFile().mkdirs();
                }
                out = new FileOutputStream(tmp);
                in = zipFile.getInputStream(zipEntry);
                byte[] content = new byte[READ_BYTES];
                int len;
                while ((len=in.read(content)) != -1 ) {
                    out.write(content, 0, len);
                }
                out.flush();
            }
        } catch (IOException e) {
            Log.e(TAG, "decompression srcFile: " + srcFile + " to " + targetFolder + " have exception", e);
            ret = false;
        } finally {
            try {
                if(zipIn != null)
                    zipIn.close();
                if(out != null)
                    out.close();
                if(in != null)
                    in.close();
                if(zipFile != null) 
                    zipFile.close();
            } catch (IOException e) {
                Log.e(TAG, "decompression, close stream have exception", e);
                ret = false;
            }
        }
        return ret;
    }
    
    public static boolean setPermissions(File file, int mode) {
        List<File> allFiles = getAllFiles(file, true);
        boolean ret = true;
        for(File f: allFiles) {
            if(android.os.FileUtils.setPermissions(f.getAbsolutePath(), mode, -1, -1) != 0) {
                ret = false;
            }
        }
        return ret;
    }
    
}
