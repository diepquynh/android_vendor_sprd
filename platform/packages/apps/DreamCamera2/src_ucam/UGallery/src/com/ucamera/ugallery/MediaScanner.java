/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ugallery;

import java.io.File;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicInteger;
import android.content.Context;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.util.Log;

public class MediaScanner {

    private MediaScannerConnection mMediaScanConn = null;

    private LockerSannerClient mClient = null;

    private String filePath = null;

    private String fileType = null;

    private ArrayList<String> filePaths = null;
    private ScannerFinishCallBack mCallBack;

    public MediaScanner(Context context, ScannerFinishCallBack callBack) {
        if (mClient == null) {
            mClient = new LockerSannerClient(callBack);
        }

        if (mMediaScanConn == null) {
            mMediaScanConn = new MediaScannerConnection(context, mClient);
        }
    }

    public interface ScannerFinishCallBack {
        public void scannerComplete();
    }

    class LockerSannerClient implements
            MediaScannerConnection.MediaScannerConnectionClient {

        public LockerSannerClient(ScannerFinishCallBack callBack) {
            mCallBack = callBack;
        }

        public void onMediaScannerConnected() {
            mHasFileScanning = false;
            if(filePath != null) {
                scannerFile(filePath);
            }
            if(filePaths != null) {
                scannerFile(filePaths);
            }
            if (!mHasFileScanning) {
                if (mCallBack != null) {
                    mCallBack.scannerComplete();
                }
            }
        }

        public void onScanCompleted(String path, Uri uri) {
            if (sScanCounter.decrementAndGet() <= 0) {
                mMediaScanConn.disconnect();
                if (mCallBack != null) {
                    mCallBack.scannerComplete();
                }
            }
        }

    }

    private final AtomicInteger sScanCounter = new AtomicInteger(0);
    boolean mHasFileScanning = false;

    private void scannerFile(String filePath) {
        File file = new File(filePath);
        File[] files = file.listFiles();

        for (int i = 0; files != null && i < files.length; i++) {
            if (files[i].isFile()) {
                mHasFileScanning = true;
                sScanCounter.incrementAndGet();
                mMediaScanConn.scanFile(files[i].getAbsolutePath(), null);
            } else {
                scannerFile(files[i].getAbsolutePath());
            }
        }

    }
    private void scannerFile(ArrayList<String> filePaths) {

        for (int i = 0; filePaths != null && i < filePaths.size(); i++) {
                mHasFileScanning = true;
                sScanCounter.incrementAndGet();
                mMediaScanConn.scanFile(filePaths.get(i), null);
        }

    }
    public void scanFile(String filepath, String fileType) {

        this.filePath = filepath;

        this.fileType = fileType;
        mMediaScanConn.connect();
    }

    public void scanFile(ArrayList<String> filePaths, String fileType) {

        this.filePaths = filePaths;

        this.fileType = fileType;

        mMediaScanConn.connect();

    }

    public String getFilePath() {

        return filePath;
    }

    public void setFilePath(String filePath) {

        this.filePath = filePath;
    }

    public String getFileType() {

        return fileType;
    }

    public void setFileType(String fileType) {

        this.fileType = fileType;
    }
}