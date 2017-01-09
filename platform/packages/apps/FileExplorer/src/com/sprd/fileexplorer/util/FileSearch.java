package com.sprd.fileexplorer.util;

import java.io.File;
import java.io.FileFilter;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import java.util.Stack;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

public class FileSearch extends Thread {
    private Handler mHandler;
    private FileFilter filter;
    private ArrayList<String> ret;
    private Set<String> mContainType;
    public static final String FILE_SEARCH_OK_KEY = "search_ok";
    public static final String FILE_SEARCH_RET_KEY = "search_result";
    public static final int CANCEL_SEARCH_KEY = -1;
    
    public static final String SEARCH_KEY = "search_key";
    public static final String SEARCH_ATTACH = "search_attach";
    public static final String SEARCH_LOCATION = "search_location";
    public static final String SEARCH_TYPE="search_type";
    
    public static final int SEARCH_EXTERNAL = 0;
    public static final int SEARCH_INTERNAL = 1;
    public static final int SEARCH_ALL = 2;
    // SPRD: Add for bug607772.
    public static final int SEARCH_USB = 3;

    public FileSearch(Handler handler, final Set<String> containType) {
        mHandler = handler;
        ret = new ArrayList<String>();
        if (containType == null || containType.isEmpty()) {
            sendMessage(false);
            return;
        }
        containType.remove(null);
        mContainType = new HashSet<String>();
        for (String s : containType) {
            mContainType.add(s.toLowerCase(Locale.US));
        }
        filter = new FileFilter() {

            @Override
            public boolean accept(File pathname) {
                if (pathname.isDirectory()) {
                    return true;
                } else {
                    if (endsWith(pathname.getName())) {
                        return true;
                    } else {
                        return false;
                    }
                }
            }

            public boolean endsWith(String fileName) {
                if (fileName == null) {
                    return false;
                }
                if (mContainType.isEmpty()) {
                    return true;
                }
                fileName = fileName.toLowerCase(Locale.US);
                for (String s : mContainType) {
                    if (fileName.endsWith(s)) {
                        return true;
                    }
                }
                return false;
            }

        };
    }

    @Override
    public void run() {
        Stack<File> stack = new Stack<File>();
        File file = StorageUtil.getExternalStorage();
        if (file != null) {
            stack.push(file);
        }
        file = StorageUtil.getInternalStorage();
        if (file != null) {
            stack.push(file);
        }
        while (!stack.empty()) {
            file = stack.pop();
            if (file.canRead() && file.isDirectory()) {
                for (File f : file.listFiles(filter)) {
                    if (f.isDirectory()) {
                        stack.push(f);
                    } else {
                        ret.add(f.getPath());
                    }
                }
            }
        }
        sendMessage(true);
    }

    private void sendMessage(boolean isSucceed) {
        Message msg = mHandler.obtainMessage();
        Bundle data = new Bundle();
        data.putBoolean(FILE_SEARCH_OK_KEY, isSucceed);
        data.putStringArrayList(FILE_SEARCH_RET_KEY, ret);
        msg.setData(data);
        msg.what = CANCEL_SEARCH_KEY;
        mHandler.sendMessage(msg);
    }

}
