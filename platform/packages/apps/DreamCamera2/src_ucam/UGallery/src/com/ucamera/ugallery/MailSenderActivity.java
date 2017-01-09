/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.NotificationManager;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;

import com.ucamera.ugallery.util.HostUtil;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;

public class MailSenderActivity extends Activity {

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        NotificationManager nm = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        nm.cancel(R.string.camera_application_stopped);
        showDialog();
    }

    private void showDialog() {
        new AlertDialog.Builder(this)
            .setMessage(R.string.str_receive)
            .setIcon(android.R.drawable.ic_dialog_alert)
            .setTitle(android.R.string.dialog_alert_title)
            .setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    finish();
                }
            })
            .setPositiveButton(android.R.string.ok,new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    backgroundSendMail();
                    finish();
                }
            })
            .show();
    }

    private void backgroundSendMail() {
        new Thread(new Runnable() {
            public void run() {
                String prefFile = null;
                try {
                    prefFile = HostUtil.zipCameraPreference(MailSenderActivity.this);
                    new MailSender(MailSenderActivity.this).addAttachment(prefFile)
                                    .sendMail(buildMessageSubject(), buildMessageBody());
                } catch (Throwable e) {
                    Log.e("SendMail", e.getMessage(), e);
                } finally {
                    if (prefFile != null) {
                        try {
                            new File(prefFile).delete();
                        }catch (Throwable e) {
                            // IGNORE
                        }
                    }
                }
            }
        }).start();
    }

    private static String readFile(String path){
        try {
            FileInputStream stream = new FileInputStream(new File(path));
            DataInputStream inStream = new DataInputStream(stream);
            StringBuilder fileContent = new StringBuilder();

            try {
                String lineString;
                do {
                    lineString = inStream.readLine();
                    if (lineString != null) {
                        fileContent.append('\r');
                        fileContent.append(lineString);
                    }
                }
                while (lineString != null);
            }
            finally {
                inStream.close();
                stream.close();
            }
            return fileContent.toString();
        }catch (Exception e){
            // do nothing
        }
        return null;
      }

    private String buildMessageSubject() {
        return new StringBuilder()
            .append("【UGallery:CRASH】")
            .append(HostUtil.makeTitleBase(this))
            .toString();
    }
    private String buildMessageBody() {
        return new StringBuilder()
            .append("\n\n====  SOFTWARE INFO\n")
            .append(HostUtil.getSoftwareInfo(this))
            .append("\n\n====  CPU INFO\n")
            .append(HostUtil.getCpuInfo(this))
            .append("\n\n====  MEMORY INFO\n")
            .append(HostUtil.getMemoryInfo(this))
            .append("\n\n====  SYSTEM PROPS\n")
            .append(HostUtil.getSystemProps(this))
            .append("\n\n====  CAMERA PARAMETERS\n")
            .append(HostUtil.getCameraParameters(this))
            .append("\n\n====  LOG FILE\n")
            .append(readFile(UncatchException.LOG_PATH+UncatchException.LOG_NAME))
            .toString();
    }

    @Override
    protected void onResume() {
        finish();
        super.onResume();
    }
}
