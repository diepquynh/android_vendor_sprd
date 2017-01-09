/*
 *
 * Copyright (C) 2010,2011 Thundersoft Corporation
 *
 */
/*
 * catch camera exception and save the log
 */
package com.ucamera.uphoto.exception;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintWriter;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Environment;
import android.util.Log;

import com.ucamera.uphoto.R;

public class UncatchException implements Thread.UncaughtExceptionHandler {
    private Thread.UncaughtExceptionHandler mException;
    private Context mContext;
    public static final String LOG_PATH = Environment.getExternalStorageDirectory().toString() + "/UCam/logs/"; // exception log path.
    public static final String LOG_NAME = "camera_log.txt";     // excetion log name.

    public UncatchException(Context context) {
        this.mContext = context;
        this.mException = Thread.getDefaultUncaughtExceptionHandler();
    }

    @Override
    public void uncaughtException(Thread thread, Throwable ex) {
        savelogtofile();
        try {
            Runnable r = new Runnable() {
                public void run() {
                    try {
                        showNotification(mContext);
                    } catch (Exception e) {
                    }
                }
            };
            new Thread(r).start();
        } catch (Exception e) {
            e.printStackTrace();
        }

        if (mException != null)
            mException.uncaughtException(thread, ex);
    }

    private void showNotification(Context context) {
        NotificationManager nm = (NotificationManager) mContext
                .getSystemService("notification");
        CharSequence from = context.getString(R.string.text_uphoto_label);
        CharSequence message = context.getString(R.string.str_camera_crash);

        PendingIntent contentIntent = PendingIntent.getActivity(context, 0,
                new Intent(context, MailSenderActivity.class),
                Intent.FLAG_ACTIVITY_NEW_TASK);
        String tickerText = context.getString(
                R.string.application_stopped, message);

        Notification notif = new Notification(R.drawable.icon_u_photo, tickerText,
                System.currentTimeMillis());

        notif.setLatestEventInfo(context, from, message, contentIntent);

        notif.vibrate = new long[] { 100, 250, 100, 500 };

        nm.notify(R.string.application_stopped, notif);
    }

    public static void savelogtofile() {
        try {
            /**
             * FIX BUG: 155
             * BUG CAUSE: the exception log file is at "sdcard/".
             * FIX COMMENT: put the exception log file to "sdcard/ucam/logs"
             * Date: 2011-12-08
             */
            File oldLogFile = new File(LOG_PATH + LOG_NAME);
            if (oldLogFile.exists()) {          // delete old log file
                /* SPRD: CID 109209 (#2 of 2): RV: Bad use of return value (FB.RV_RETURN_VALUE_IGNORED_BAD_PRACTICE) @{
                oldLogFile.delete();
                */
                boolean ds = oldLogFile.delete();
                if (!ds) {
                    throw new Exception();
                }
                /* @} */
            }
            File logFile = new File(LOG_PATH);
            if (!logFile.exists()) {            // careate log folder
                /* SPRD: CID 109209 (#1 of 2): RV: Bad use of return value (FB.RV_RETURN_VALUE_IGNORED_BAD_PRACTICE) @{
                logFile.mkdirs();
                */
                boolean ms = logFile.mkdirs();
                if (!ms) {
                    throw new Exception();
                }
                /* @} */
            }
            String cmd = "logcat " + "-t 2000  -v threadtime  -f" + LOG_PATH + LOG_NAME +"\n";
            Process process = Runtime.getRuntime().exec("sh");
            OutputStream os = process.getOutputStream();
            /* SPRD: CID 109164 (#1 of 1): Dm: Dubious method used (FB.DM_DEFAULT_ENCODING) @{
            os.write(cmd.getBytes());
            */
            os.write(cmd.getBytes("utf-8"));
            /* @} */
            os.close();
        } catch (Exception e) {
            throw new RuntimeException("Write log file error!");
        }

    }

}