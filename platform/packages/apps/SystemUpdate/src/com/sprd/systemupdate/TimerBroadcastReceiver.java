package com.sprd.systemupdate;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

import android.app.AlarmManager;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Build;
import android.os.PowerManager;
import android.util.Log;
import android.view.WindowManager;
import android.os.EnvironmentEx;
import android.os.RecoverySystem;

public class TimerBroadcastReceiver extends BroadcastReceiver {

    private File RECOVERY_DIR = new File("/cache/recovery");
    private File COMMAND_FILE = new File(RECOVERY_DIR, "command");
    private static final String TAG = "TimeBroadcastReceiver";

    public void onReceive(Context context, Intent intent) {
        Log.i(TAG,
                "onReceive:" + System.currentTimeMillis() + intent.getAction());
		if (intent.getAction() == null) {
			return;
		}
        final Storage mStorage = Storage.get(context);
        long mInterval = mStorage.getInterval();
        final Context mContext = context;
        final Intent mIntentDownloadSucceed = new Intent(mContext,
                UpgradeActivity.class);
        final Intent mIntentCheckSucceed = new Intent(mContext,
                LatestUpdateActivity.class);

        PendingIntent mPendingIntent = PendingIntent.getBroadcast(context, 0,
                new Intent("sprd.systemupdate.action.TIMER"), 0);
        AlarmManager mAlarmManager = (AlarmManager) context
                .getSystemService(Context.ALARM_SERVICE);

        if (intent.getAction().equals("sprd.systemupdate.action.TIMER")) {
            Intent mIntent = new Intent();
            mIntent.setAction("sprd.systemupdate.action.CHECKUPDATE");
            mIntent.setPackage("com.sprd.systemupdate");
            context.startService(mIntent);
            mStorage.setPretime(System.currentTimeMillis());
        } else if (intent.getAction().equals(
                "android.intent.action.BOOT_COMPLETED")) {
            if (mStorage.getState() == Storage.State.NIL_2_DOWNLOADING
                    || mStorage.getState() == Storage.State.PAUSE_2_DOWNLOADING) {
                mStorage.setState(Storage.State.DOWNLOADING_2_PAUSE);
            } else if (intent.getAction().equals(
                    "android.intent.action.ACTION_SHUTDOWN")) {

            }
            Log.i(TAG, "onReceive--BOOT_COMPLETED");
            if (mInterval != 0) {
                long start_time = mStorage.getPretime() + mInterval;
                mAlarmManager.setRepeating(AlarmManager.RTC_WAKEUP, start_time,
                        mInterval, mPendingIntent);
            }

            if (mStorage.getUpgrade()==true) {
                mStorage.setUpgrade(false);
                mStorage.setState(Storage.State.NIL);
                if (mStorage.getLatestVersion() != null) {
                    if (Utils.DEBUG) {
                        Log.d(TAG,"mVersion="+mStorage.getLatestVersion().mVersion);
                        Log.d(TAG,"bendiversion="+Build.DISPLAY);
                    }
                    if (mStorage.getLatestVersion().mVersion
                            .equals(Build.DISPLAY)) {
                        if (Utils.DEBUG) {
                            Log.i(TAG,"mVersion="+mStorage.getLatestVersion().mVersion);
                            Log.i(TAG,"bendiversion="+Build.DISPLAY);
                        }
                        Dialog dialog = new AlertDialog.Builder(context)
                                .setTitle(R.string.app_name)
                                .setMessage(R.string.upgrade_succeed)
                                .setPositiveButton(R.string.ok,
                                        new DialogInterface.OnClickListener() {
                                            public void onClick(
                                                    DialogInterface dialog,
                                                    int which) {
                                                dialog.dismiss();
                                            }
                                        }).create();
                        dialog.getWindow().setType(
                                WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
                        dialog.setCanceledOnTouchOutside(false);
                        dialog.show();
                    } else {
                        Dialog dialog = new AlertDialog.Builder(context)
                                .setTitle(R.string.app_name)
                                .setMessage(R.string.upgrade_failed)
                                .setPositiveButton(R.string.ok,
                                        new DialogInterface.OnClickListener() {
                                            public void onClick(
                                                    DialogInterface dialog,
                                                    int which) {
                                                dialog.dismiss();
                                            }
                                        }).create();
                        dialog.getWindow().setType(
                                WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
                        dialog.setCanceledOnTouchOutside(false);
                        dialog.show();
                    }
                }

            }

        } else if (intent.getAction().equals(
                "sprd.systemupdate.action.DOWNLOAD_RESULT")) {
            boolean result = intent.getBooleanExtra("result", false);
            if (result) {
                mStorage.setState(Storage.State.DOWNLOADED);
                Dialog dialog = new AlertDialog.Builder(context)
                        .setTitle(R.string.app_name)
                        .setMessage(R.string.newVersion_download_succeed)
                        .setPositiveButton(R.string.ok,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog,
                                            int which) {
                                        mStorage.setSize(0);
                                        dialog.dismiss();
                                        mIntentDownloadSucceed
                                                .setFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK|Intent.FLAG_ACTIVITY_NEW_TASK);
                                        mContext.startActivity(mIntentDownloadSucceed);
                                    }
                                }).create();
                dialog.getWindow().setType(
                        WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
                dialog.setCanceledOnTouchOutside(false);
                dialog.show();
            } else {
                Dialog dialog = new AlertDialog.Builder(context)
                        .setTitle(R.string.app_name)
                        .setMessage(R.string.download_failed)
                        .setPositiveButton(R.string.reload,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog,
                                            int which) {
                                        dialog.dismiss();
                                        Intent mIntent = new Intent();
                                        mIntent.setAction("sprd.systemupdate.action.DOWNLOADING");
                                        mIntent.setPackage("com.sprd.systemupdate");
                                        mContext.startService(mIntent);
                                        if (Utils.DEBUG) {
                                            Log.d("DOWNLOAD",
                                                    "DOWNLOAD_FAILED--reload");
                                        }
                                    }
                                })
                        .setNegativeButton(R.string.cancel,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog,
                                            int which) {
                                        dialog.dismiss();
                                        mStorage.setSize(0);
                                        if (Utils.DEBUG) {
                                            Log.e("DOWNLOAD",
                                                    "DOWNLOAD_FAILED--cancel");
                                        }
                                    }
                                }).create();

                dialog.getWindow().setType(
                        WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
                dialog.setCanceledOnTouchOutside(false);
                dialog.setCancelable(true);
                dialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
                    public void onCancel(DialogInterface dialog) {
                        dialog.dismiss();
                        mStorage.setSize(0);
                    }
                });
                dialog.show();
            }
        } else if (intent.getAction().equals(
                "sprd.systemupdate.action.ASK_UPGRADE")) {
            Dialog dialog = new AlertDialog.Builder(context)
                    .setTitle(R.string.app_name)
                    .setMessage(R.string.is_now_upgrade)
                    .setPositiveButton(R.string.ok,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    dialog.dismiss();
                                    mStorage.setUpgrade(true);

                                    RECOVERY_DIR.mkdirs(); // In case we need it
                                    COMMAND_FILE.delete(); // In case it's not
                                                           // writable

                                   /* try {
                                        FileWriter command = new FileWriter(
                                                COMMAND_FILE);
                                        command.write("--update_package=/sdcard0/update.zip");
                                        command.write("\n");
                                        command.close();
                                    } catch (IOException e) {
                                        e.printStackTrace();

                                    }*/
                                    String storageDirectory = EnvironmentEx.getExternalStoragePath().getAbsolutePath();
                                    File file=new File(storageDirectory+"/update.zip");
                                    if(file.exists()) {
                                        try {
                                            RecoverySystem.installPackage(mContext, file);
                                        } catch (IOException e) {
                                            e.printStackTrace();
                                        }
                                    }
                                   /* PowerManager pm = (PowerManager) mContext
                                            .getSystemService(Context.POWER_SERVICE);
                                    pm.reboot("recovery");*/
                                }
                            })
                    .setNegativeButton(R.string.cancel,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    mStorage.setState(Storage.State.DOWNLOADED);
                                    dialog.dismiss();
                                }
                            }).create();
            dialog.getWindow().setType(
                    WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
            dialog.setCanceledOnTouchOutside(false);
            dialog.show();

        } else if (intent.getAction().equals(
                "sprd.systemupdate.action.CHECK_RESULT")) {
            Dialog dialog = new AlertDialog.Builder(context)
                    .setTitle(R.string.app_name)
                    .setMessage(R.string.has_new_version)
                    .setPositiveButton(R.string.ok,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    dialog.dismiss();
                                    mIntentCheckSucceed
                                            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_CLEAR_TASK);
                                    mContext.startActivity(mIntentCheckSucceed);
                                }
                            })
                    .setNegativeButton(R.string.cancel,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    dialog.dismiss();
                                }
                            }).create();

            dialog.getWindow().setType(
                    WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
            dialog.setCanceledOnTouchOutside(false);
            dialog.show();

        } else if (intent.getAction().equals(
                "sprd.systemupdate.action.UPDATE_FILE_DELETED")) {
            Dialog dialog = new AlertDialog.Builder(context)
                    .setTitle(R.string.app_name)
                    .setMessage(R.string.update_file_being_deleted)
                    .setPositiveButton(R.string.ok,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    dialog.dismiss();
                                    if (mStorage.checkSdCardState()) {
                                        Intent mIntent = new Intent();
                                        mIntent.setAction("sprd.systemupdate.action.DOWNLOADING");
                                        mIntent.setPackage("com.sprd.systemupdate");
                                        mContext.startService(mIntent);
                                    } else {
                                        dialog.dismiss();
                                        mStorage.setState(Storage.State.NIL);
                                    }

                                }
                            })
                    .setNegativeButton(R.string.cancel,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    dialog.dismiss();
                                    mStorage.setState(Storage.State.NIL);
                                }
                            }).create();

            dialog.getWindow().setType(
                    WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
            dialog.setCanceledOnTouchOutside(false);
            dialog.show();
        }
    }

}
