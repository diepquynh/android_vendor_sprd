/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery.util;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.util.Log;
import android.webkit.URLUtil;
import android.widget.RemoteViews;
import android.widget.Toast;

import com.ucamera.ugallery.ImageGallery;
import com.ucamera.ugallery.R;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;

public class UpdateService extends Service {
    private static final String TAG = "UpdateService";
    private Context mContext;
    private NotificationManager mNotificationManager;
    private DownloadThread mDownloadThread;
    private Notification mNotification;
    private static final int NOTIFY_ID = 0;
    private RemoteViews mNotifView;  // the specified notification view
    private String mServerAddress;   // the server of the Ucam
    public static final String DIRECTORY = Environment.getExternalStorageDirectory()
            .toString() + "/UCam/download/";
    private File mCameraApkFile;    // the file of ucam.apk downloaded
    public static boolean mIsDownloading = false;

    public void onCreate() {
        mContext = UpdateService.this;
        mNotificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
    }

    /**
     * called by startService in Camera. show the Notification and delete the
     * exist apk in directory sdcard/UCam/download/
     */
    public int onStartCommand(Intent intent, int flags, int startId) {
        mServerAddress = intent.getStringExtra("serverAddress");
        Log.i(TAG, "start download service id " + startId + ": " + intent);
        if (startId == 1) {
            Toast.makeText(UpdateService.this,
                    R.string.text_service_started_notification,
                    Toast.LENGTH_SHORT).show();
            showNotification();
            deleteInvalidFile();
            mDownloadThread = new DownloadThread();
            mDownloadThread.start();
            mIsDownloading = true;
        }else if(startId > 1) {
            Toast.makeText(UpdateService.this, R.string.text_service_starting_notification, Toast.LENGTH_SHORT).show();
        }
        return START_STICKY;
    }

    public void onDestroy() {
    }

    public IBinder onBind(Intent intent) {
        return null;
    }
/**
 * initial some views and show the notification
 */
    private void showNotification() {
        CharSequence text = getText(R.string.text_service_started_notification);
        CharSequence title = getText(R.string.text_ugallery_name);
        mNotification = new Notification(R.drawable.download_icon, text,
                System.currentTimeMillis());
        mNotification.flags = Notification.FLAG_ONGOING_EVENT;
        mNotifView = new RemoteViews(mContext.getPackageName(),
                R.layout.download_notification_layout);
        mNotifView.setTextViewText(R.id.fileName, title);

        mNotification.contentView = mNotifView;
        PendingIntent contentIntent = PendingIntent.getActivity(this, 0,
                new Intent(this, ImageGallery.class), 0);
        mNotification.contentIntent = contentIntent;
        mNotificationManager.notify(NOTIFY_ID, mNotification);
    }
/**
 * receive the message form download thread and then update the progress bar
 * finally set the install event for notification.
 */
    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            int what = msg.what;
            switch (what) {
            case 1:
                int rate = msg.arg1;
                if (rate < 100) {
                    mNotifView.setTextViewText(R.id.rate, rate + "%");
                    mNotifView.setProgressBar(R.id.progress, 100, rate, false);
                } else {
                    mNotifView.setTextViewText(R.id.rate, rate + "%");
                    mNotifView.setProgressBar(R.id.progress, 100, rate, false);
                    mNotification.flags = Notification.FLAG_AUTO_CANCEL;
                    mNotification.contentView = null;

                    Intent intent = new Intent();
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.setAction(android.content.Intent.ACTION_VIEW);
                    intent.setDataAndType(Uri.fromFile(mCameraApkFile),
                            "application/vnd.android.package-archive");
                    PendingIntent contentIntent = PendingIntent.getActivity(
                            mContext, 0, intent,
                            PendingIntent.FLAG_UPDATE_CURRENT);
                    CharSequence completeString = getText(R.string.text_download_apk_success);
                    CharSequence title = getText(R.string.text_ugallery_name);
                    mNotification.setLatestEventInfo(mContext, title,
                            completeString, contentIntent);
                    mIsDownloading = false;
                    Toast.makeText(UpdateService.this, R.string.text_download_apk_success, Toast.LENGTH_SHORT).show();
                    UpdateService.this.stopSelf();
                }
                mNotificationManager.notify(NOTIFY_ID, mNotification);
                break;
            }
        }
    };
/**
 * the thread connect to the server and save the apk file
 *
 */
    private class DownloadThread extends Thread {
        public void run() {
            try {
                int rate = 0;
                if (!URLUtil.isNetworkUrl(mServerAddress)) {
                    Toast.makeText(mContext,
                            R.string.text_no_network_available,
                            Toast.LENGTH_SHORT).show();
                } else {
                    URL myURL = new URL(mServerAddress);
                    URLConnection conn = myURL.openConnection();
                    conn.connect();
                    int totalSize = conn.getContentLength();
                    int currentSize = 0;
                    InputStream is = conn.getInputStream();
                    if (is == null) {
                        throw new RuntimeException("stream is null");
                    }
                    String fileExtention = mServerAddress.substring(
                            mServerAddress.lastIndexOf(".") + 1, mServerAddress.length())
                            .toLowerCase();
                    String fileName = mServerAddress.substring(
                            mServerAddress.lastIndexOf("/") + 1, mServerAddress.lastIndexOf("."));
                    File dir = new File(DIRECTORY);
                    if (!dir.exists())              // fix bug 6083 check the
                        dir.mkdirs();               // path to save file
                    String filePath = DIRECTORY + fileName + "."
                            + fileExtention;
                    FileOutputStream fos = null;
                    try {
                        mCameraApkFile = new File(filePath);
                        fos = new FileOutputStream(mCameraApkFile);
                    } catch(Exception e) {
                        filePath = DIRECTORY + mServerAddress.substring(mServerAddress.lastIndexOf("=") + 1, mServerAddress.length());
                        mCameraApkFile = new File(filePath);
                        fos = new FileOutputStream(mCameraApkFile);
                    }
                    byte buf[] = new byte[128];
                    long currentTime = System.currentTimeMillis();
                    do {
                        int numread = is.read(buf);
                        if (numread <= 0) {
                            break;
                        }
                        fos.write(buf, 0, numread);
                        currentSize += numread;
                        Message msg = mHandler.obtainMessage();
                        msg.what = 1;
                        if (System.currentTimeMillis() - currentTime > 1000) {
                            currentTime = System.currentTimeMillis();
                            rate = currentSize * 100 / totalSize;
                            msg.arg1 = rate;
                            mHandler.sendMessage(msg);
                        } else if (currentSize == totalSize) {
                            fos.close();
                            msg.arg1 = 100;
                            mHandler.sendMessage(msg);
                        }
                    } while (true);

                    try {
                        is.close();
                        /* SPRD: CID 109011 : Resource leak (RESOURCE_LEAK) @{ */
                        if(fos != null){
                            fos.close();
                        }
                        /* @} */
                    } catch (Exception ex) {
                    }
                }
            } catch (Exception e) {
                mIsDownloading = false;
                Log.e(TAG, "Download Ucam error "+ e.getMessage());
            }
        }
    }
/**
 * delete the old exist apk file
 */
    private void deleteInvalidFile() {
        File downloadDirectory=new File(DIRECTORY);
        File[] fileList = downloadDirectory.listFiles();
        for (int i = 0; i < fileList.length; i++) {
            String path = fileList[i].getAbsolutePath();
            if (path.endsWith(".apk") && !fileList[i].isDirectory()) {
                String fileName = path.substring(
                        path.lastIndexOf("/") + 1, path.lastIndexOf("."));
                if (fileName.startsWith("UCam")) {
                    fileList[i].delete();
                }
            }
        }
    }
}
