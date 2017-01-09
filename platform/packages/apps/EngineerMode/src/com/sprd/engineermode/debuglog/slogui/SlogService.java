/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sprd.engineermode.debuglog.slogui;

import static com.sprd.engineermode.debuglog.slogui.SlogService.SERVICE_SLOG_KEY;
import static com.sprd.engineermode.debuglog.slogui.SlogService.SERVICE_SNAP_KEY;
import com.sprd.engineermode.R;
import com.sprd.engineermode.activity.slog.SlogActivity;
import com.sprd.engineermode.activity.slog.SlogInfo;

import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.WindowManager;

import com.android.internal.app.IMediaContainerService;

public class SlogService extends Service {
    private static final String TAG = "SlogService";
    public static final String ACTION_SCREEN_SHOT = "slogui.intent.action.SCREEN_SHOT";
    public static final String ACTION_LOW_STORAGE_ALERT = "slogui.intent.action.LOW_VOLUME";

    public static final String EXTRA_FREE_SPACE = "freespace";

    public static final String SERVICES_SETTINGS_KEY = "settings";
    public static final String SERVICE_SLOG_KEY = "slog";
    public static final String SERVICE_SNAP_KEY = "snap";
    public static final int NOTIFICATION_SLOG = 1;
    public static final int NOTIFICATION_SNAP = 2;
    public static final int NOTIFICATION_LOW_STORAGE = 3;

    private static final String LOW_STORAGE_ALERT = "low_storage";

    private IMediaContainerService mMediaContainer;
    private GlobalAlertDialog mGlobalAlert;
    private ServiceConnection mMediaContainerConnection = new ServiceConnection() {
        @Override
        public void onServiceDisconnected(ComponentName name) {
            mMediaContainer = null;
        }
        
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mMediaContainer = IMediaContainerService.Stub.asInterface(service);
        }
    };

    private NotificationManager mNotificationManager;
    static Notification sSlogNotification;
    static Notification sSnapNotification;
    static Notification sLowStorageNotification;
    private ISlogService.Stub mBinder = new ISlogService.Stub() {

        @Override
        public void setNotification(int which, boolean show)
                throws RemoteException {
            Log.d(TAG, "remote:setNotification");
            switch (which) {
                case NOTIFICATION_SLOG:
                    if (sSlogNotification == null) {
                        sSlogNotification = createSlogNotification(NOTIFICATION_SLOG);
                    }
                    if (show) {
                        mNotificationManager.notify(R.string.service_slog_title,
                                sSlogNotification);
                    } else {
                        mNotificationManager.cancel(R.string.service_slog_title);
                    }
                    break;
                case NOTIFICATION_SNAP:
                    if (sSnapNotification == null) {
                        sSnapNotification = createSlogNotification(NOTIFICATION_SNAP);
                    }
                    if (show) {
                        mNotificationManager.notify(R.string.service_snap_title,
                                sSnapNotification);
                    } else {
                        mNotificationManager.cancel(R.string.service_snap_title);
                    }
                    break;
                case NOTIFICATION_LOW_STORAGE:
                    if (sLowStorageNotification == null) {
                        sLowStorageNotification =
                                createSlogNotification(NOTIFICATION_LOW_STORAGE);
                    }
                    if (show) {
                        mNotificationManager.notify(R.string.notification_low_storage_title,
                                sLowStorageNotification);
                    } else {
                        mNotificationManager.cancel(R.string.notification_low_storage_title);
                    }
                break;
                default:
                    Log.w(TAG, "Unknown notification:" + which);
                    break;
            }
        }
    };

    /**
     * package
     * Compute free space of current using storage, when it is full or low,
     * show the alert.
     */
    static void computeAndShowAlertAsync(final Context context,
            final Notification notification,
            final AlertDialog alertdialog,
            final IMediaContainerService imcs, long limited) {
        if (context == null) {
            return;
        }
        /* compute free space and slog files usage, and determine whether
         * the dialog should show */
        int computeResult = 0;
        if (limited > computeResult && alertdialog != null) {
            alertdialog.setMessage(context.getString(
                    R.string.low_space_dialog_message, computeResult));
            alertdialog.show();
        }

    }

    Notification createSlogNotification(int which) {
        Notification notification = null;
        if (which == NOTIFICATION_SLOG) {
            notification = new Notification(
                    android.R.drawable.ic_dialog_alert,
                    getString(R.string.service_slog_statusbar_prompt), 0);
            notification.setLatestEventInfo(getApplicationContext(),
                    getString(R.string.service_slog_title),
                    getString(R.string.service_slog_prompt), PendingIntent
                            .getActivity(
                                    this,
                                    NOTIFICATION_SLOG,
                                    new Intent().setClass(getApplicationContext(),
                                            SlogActivity.class).setFlags(
                                            Intent.FLAG_ACTIVITY_CLEAR_TOP),
                                    PendingIntent.FLAG_CANCEL_CURRENT));
            notification.flags |= Notification.FLAG_ONGOING_EVENT;
        } else if (which == NOTIFICATION_SNAP) {
            notification = new Notification(
                    android.R.drawable.ic_dialog_alert,
                    getString(R.string.service_snap_statusbar_prompt), 0);
            notification.setLatestEventInfo(getApplicationContext(),
                    getString(R.string.service_snap_title),
                    getString(R.string.service_snap_prompt), PendingIntent
                            .getBroadcast(getApplicationContext(),
                                    NOTIFICATION_SNAP, new Intent(
                                    ACTION_SCREEN_SHOT),
                                    PendingIntent.FLAG_CANCEL_CURRENT));
            notification.flags |= Notification.FLAG_ONGOING_EVENT;
        } else if (which == NOTIFICATION_LOW_STORAGE){

            notification = new Notification(
                    android.R.drawable.ic_dialog_alert,
                    getString(R.string.notification_low_storage_statusbar_prompt), 0);

            /* Solution A, open SlogUICommonControl */
            notification.setLatestEventInfo(getApplicationContext(),
                    getString(R.string.notification_low_storage_title),
                    getString(R.string.notification_low_storage_prompt), PendingIntent
                            .getActivity(
                                    this,
                                    NOTIFICATION_LOW_STORAGE,
                                    new Intent().setClass(getApplicationContext(),
                                    		SlogActivity.class).setFlags(
                                            Intent.FLAG_ACTIVITY_CLEAR_TOP),
                                    PendingIntent.FLAG_CANCEL_CURRENT));

            notification.flags |= Notification.FLAG_ONGOING_EVENT;
        } else {
            throw new IllegalArgumentException("Which notification is " + which + "?");
        }
        return notification;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null) {
            if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) {
                stopSelf();
            }
            if (ACTION_LOW_STORAGE_ALERT.equals(intent.getAction())) {
                try {
                    if (sLowStorageNotification == null) {
                        sLowStorageNotification = createSlogNotification(
                                NOTIFICATION_LOW_STORAGE);
                    }
                    mNotificationManager.notify(R.string.notification_low_storage_title,
                            sLowStorageNotification);
                    stopSelf();
                } catch (Exception e) {} // Ignore
            }
        } else {
            // no data, won't know what to do.
            stopSelf();
        }
        return super.onStartCommand(intent, flags, startId);
    }

    

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "Service of SloguI begin to work");

        mNotificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);

        if (sSlogNotification == null) {
            sSlogNotification = createSlogNotification(NOTIFICATION_SLOG);
        }
        if (sSnapNotification == null) {
            sSnapNotification = createSlogNotification(NOTIFICATION_SNAP);
        }

        if (sLowStorageNotification == null) {
            sLowStorageNotification = createSlogNotification(NOTIFICATION_LOW_STORAGE);
        }

        SharedPreferences settings = getApplicationContext()
                .getSharedPreferences(SERVICES_SETTINGS_KEY,
                        MODE_PRIVATE);
        if ((settings.getBoolean(SERVICE_SLOG_KEY, false)||
                getApplicationContext().getSharedPreferences("test", MODE_PRIVATE).getBoolean(SERVICE_SLOG_KEY,false))) {
            mNotificationManager.notify(R.string.service_slog_title, sSlogNotification);
        }

    }

    @Override
    public IBinder onBind(Intent data) {
        Log.d(TAG, "onBind");
        return mBinder;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "Service of SlogUI has destroyed");
    }

    static class GlobalAlertDialog extends AlertDialog {
        public GlobalAlertDialog(Context context) {
            super(context);
            getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
            getWindow().setFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM,
                    WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
        }
    }

}
