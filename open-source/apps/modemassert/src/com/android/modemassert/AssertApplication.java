package com.android.modemassert;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.text.TextUtils;
import android.util.Log;
import android.app.Application;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemClock;
import android.os.SystemProperties;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.nio.charset.StandardCharsets;

public class AssertApplication extends Application {
    private final String MTAG = "AssertApplication";

    //Action for assert broadcast
    private static final String MODEM_STAT_CHANGE = "com.android.modemassert.MODEM_STAT_CHANGE";
    //extra data key in intent for assert broadcast
    private static final String MODEM_STAT = "modem_stat";
    //SPRD: add modem assert reason for PhoneInfo feature
    private static final String MODEM_INFO= "modem_info";
    //values of extra data in intent.
    private static final String MODEM_ALIVE = "modem_alive";
    private static final String MODEM_ASSERT = "modem_assert";
    //name of socket to listen to modem state
    private static final String MODEM_SOCKET_NAME = "modemd";

    //name of socket to listen to slog modem message
    private static final String SLOG_SOCKET_NAME = "slogmodem";

    //name of socket to listen to wcn modem state
    private static final String WCN_MODEM_SOCKET_NAME = "wcnd";
    private static String PROP_MODEM_TYPE = "ro.radio.modemtype";

    //notification id to cancel
    private static final int MODEM_ASSERT_ID = 1;
    private static final int WCND_ASSERT_ID = 2;
    private static final int MODEM_BLOCK_ID = 3;

    private static final int BUF_SIZE = 128;

    private final Object mObjectLock = new Object();
    private Handler mAssertHandler;
    private HandlerThread mAssertThread;

    private final Object mObjectSlogLock = new Object();
    private Handler mSlogHandler;
    private HandlerThread mSlogThread;

    private final Object mObjectWcnLock = new Object();
    private Handler mWcnAssertHandler;
    private HandlerThread mWcnAssertThread;

    private static final boolean IS_DEBUGGABLE = SystemProperties.getInt(
            "ro.debuggable", 0) == 1;
    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(MTAG, "onCreate()...");
        mAssertThread = new HandlerThread("assertHandlerThread");
        mAssertThread.start();
        mAssertHandler = new assertHandler(mAssertThread.getLooper());
        mAssertHandler.sendEmptyMessage(0);

        mSlogThread = new HandlerThread("slogHandlerThread");
        mSlogThread.start();
        mSlogHandler = new slogHandler(mSlogThread.getLooper());
        mSlogHandler.sendEmptyMessage(0);

        mWcnAssertThread = new HandlerThread("WcnAssertHandlerThread");
        mWcnAssertThread.start();
        mWcnAssertHandler = new wcnassertHandler(mWcnAssertThread.getLooper());
        mWcnAssertHandler.sendEmptyMessage(0);
    }

    private class assertHandler extends Handler {
        public assertHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            try {
                LocalSocket socket = new LocalSocket();
                LocalSocketAddress socketAddr = new LocalSocketAddress(MODEM_SOCKET_NAME,
                        LocalSocketAddress.Namespace.ABSTRACT);
                runSocket(socket, socketAddr);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    private void connectToSocket(LocalSocket socket, LocalSocketAddress socketAddr) {
        for (;;) {
            try {
                socket.connect(socketAddr);
                break;
            } catch (IOException ioe) {
                ioe.printStackTrace();
                SystemClock.sleep(10000);
                continue;
            }
        }
    }

    private void runSocket(LocalSocket socket, LocalSocketAddress socketAddr) {
        byte[] buf = new byte[BUF_SIZE];
        connectToSocket(socket, socketAddr);

        Log.d(MTAG, " -runSocket");
        synchronized (mObjectLock) {
            for (;;) {
                int cnt = 0;
                InputStream is = null;
                try {
                    // mBinder.wait(endtime - System.currentTimeMillis());
                    is = socket.getInputStream();
                    cnt = is.read(buf, 0, BUF_SIZE);
                    Log.d(MTAG, "read " + cnt + " bytes from modemdSocket: \n" );
                } catch (IOException e) {
                    Log.w(MTAG, "read exception\n");
                }
                if (cnt > 0) {
                    String info = "";
                    try {
                        info = new String(buf, 0, cnt, "US-ASCII");
                    } catch (UnsupportedEncodingException e) {
                        e.printStackTrace();
                    } catch (StringIndexOutOfBoundsException e) {
                        e.printStackTrace();
                    }
                    Log.d(MTAG, "read something: "+ info);
                    if (TextUtils.isEmpty(info)) {
                        continue;
                    }
                    if (info.contains("Modem Alive")) {
                        //SPRD: add modem assert reason for PhoneInfo feature 
                        sendModemStatBroadcast(MODEM_ALIVE,info);
                        hideNotification(MODEM_ASSERT_ID);
                        hideNotification(MODEM_BLOCK_ID);
                    } else if (info.contains("Modem Assert")) {
                        String value = SystemProperties.get("persist.sys.sprd.modemreset", "default");
                        Log.d(MTAG, " modemreset ? : " + value);
                        if(!value.equals("1")){
                            showNotification(MODEM_ASSERT_ID,"modem assert",info);
                        }
                        /* SPRD:Bug#311987 After modem assert,the lte signal still displayed@{ */
                        Intent intent = new Intent("android.intent.action.ACTION_LTE_READY");
                        intent.putExtra("lte", false);
                        Log.i(MTAG,"modem assert Send ACTION_LTE_READY  false");
                        sendBroadcast(intent);
                        /* @} */
	                //SPRD: add modem assert reason for PhoneInfo feature
                        sendModemStatBroadcast(MODEM_ASSERT,info);
                    } else if (info.contains("Modem Blocked")) {
                        showNotification(MODEM_BLOCK_ID,"modem block",info);
                    } else {
                        Log.d(MTAG, "do nothing with info :" + info);
                    }
                    continue;
                } else if (cnt < 0) {
                    try {
                        is.close();
                        socket.close();
                    } catch (IOException e) {
                        Log.w(MTAG, "close exception\n");
                    }
                    socket = new LocalSocket();
                    connectToSocket(socket, socketAddr);
                }
            }
        }
    }

    private void showNotification(int notificationId, String title, String info) {
        Log.v(MTAG, "show assert Notification.");
        if(IS_DEBUGGABLE){
            int icon = R.drawable.modem_assert;
            NotificationManager manager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
            long when = System.currentTimeMillis();
            Notification notification = new Notification(icon, info, when);

            Context context = getApplicationContext();
            CharSequence contentTitle = title;
            CharSequence contentText = info;
            /** modify 145779 add show assertinfo page **/
            Intent notificationIntent = new Intent(this, AssertInfoActivity.class);
            notificationIntent.putExtra("assertInfo", info);
            PendingIntent contentIntent =  PendingIntent.getActivity(context, 0, notificationIntent, 0);

            //notification.defaults |= Notification.DEFAULT_VIBRATE;
            long[] vibrate = {0, 10000};
            notification.vibrate = vibrate;
            notification.flags |= Notification.FLAG_NO_CLEAR;// no clear
            /** modify 145779 add show assertinfo page **/
            notification.defaults |= Notification.DEFAULT_SOUND;
            //notification.sound = Uri.parse("file:///sdcard/assert.mp3");

            notification.setLatestEventInfo(context, contentTitle, contentText, contentIntent);
            manager.notify(notificationId, notification);
        }
    }

    private void hideNotification(int notificationId) {
        Log.v(MTAG, "hideNotification");
        if(IS_DEBUGGABLE){
            NotificationManager manager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
            manager.cancel(notificationId);
        }
    }

    private void sendModemStatBroadcast(String modemStat,String info) {
        Log.d(MTAG, "sendModemStatBroadcast : " + modemStat);
        Intent intent = new Intent(MODEM_STAT_CHANGE);
        intent.putExtra(MODEM_STAT, modemStat);
        //SPRD: add modem assert reason for PhoneInfo feature 
        intent.putExtra(MODEM_INFO, info);
        sendBroadcast(intent);
    }
    /* after modem assert, show progressDialog */
    private void showProgressDialog() {
       Intent intent = new Intent(this, SystemInfoDumpingActivity.class);
       intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
       intent.putExtra("closeFlag", false);
       startActivity(intent);
    }

    private class slogHandler extends Handler {
        public slogHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            try {
                LocalSocket socket = new LocalSocket();
                LocalSocketAddress socketAddr = new LocalSocketAddress(SLOG_SOCKET_NAME,
                        LocalSocketAddress.Namespace.ABSTRACT);
                runSlogSocket(socket, socketAddr);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }
    private void runSlogSocket(LocalSocket socket, LocalSocketAddress socketAddr) {
        byte[] buf = new byte[BUF_SIZE];
        connectToSlogSocket(socket, socketAddr);

        Log.d(MTAG, " -runSlogSocket");
        synchronized (mObjectSlogLock) {
            for (;;) {
                int cnt = 0;
                InputStream is = null;
                try {
                    is = socket.getInputStream();
                    cnt = is.read(buf, 0, BUF_SIZE);
                    Log.d(MTAG, "read " + cnt + " bytes from slogSocket: \n" );
                } catch (IOException e) {
                    Log.w(MTAG, "read exception\n");
                }

                if (cnt > 0) {
                    String info = "";
                    try {
                        info = new String(buf, 0, cnt, "US-ASCII");
                    } catch (UnsupportedEncodingException e) {
                        e.printStackTrace();
                    } catch (StringIndexOutOfBoundsException e) {
                        e.printStackTrace();
                    }
                    Log.d(MTAG, "read something: "+ info);
                    if (TextUtils.isEmpty(info)) {
                        continue;
                    }
                    if (info.contains("CP_DUMP_START")) {
                        showProgressDialog();
                    } else if (info.contains("CP_DUMP_END")) {
                        Log.d(MTAG, "SLOG_DUMP_END_ACTION");
                        Intent closeIntent = new Intent(this, SystemInfoDumpingActivity.class);
                        closeIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
                        closeIntent.putExtra("closeFlag", true);
                        startActivity(closeIntent);
                    } else {
                        Log.d(MTAG, "do nothing with info: " + info);
                    }
                    continue;
                } else if (cnt < 0) {
                    try {
                        is.close();
                        socket.close();
                    } catch (IOException e) {
                        Log.w(MTAG, "close exception\n");
                    }
                    socket = new LocalSocket();
                    connectToSlogSocket(socket, socketAddr);
                }
            }
        }
    }

    private void connectToSlogSocket(LocalSocket socket, LocalSocketAddress socketAddr) {
        String ssdaMode = "";
        String modemTypeInfo = SystemProperties.get(PROP_MODEM_TYPE);
        if ("w".equals(modemTypeInfo)) {
            ssdaMode = "WCDMA";
        } else if ("t".equals(modemTypeInfo)) {
            ssdaMode = "TD";
        } else if ("l".equals(modemTypeInfo)) {
            ssdaMode = "5MODE";
        } else if ("tl".equals(modemTypeInfo)) {
            ssdaMode = "TDD-LTE";
        } else if ("lf".equals(modemTypeInfo)) {
            ssdaMode = "FDD-LTE";
        }
        Log.d(MTAG, "ssdaMode: "+ ssdaMode);
        for (;;) {
            OutputStream os = null;
            try {
                socket.connect(socketAddr);
                os = socket.getOutputStream();
                if(os != null) {
                    String strcmd = "SUBSCRIBE " + ssdaMode + " DUMP";
                    final StringBuilder cmdBuilder =  new StringBuilder(strcmd).append('\n');
                    final String cmd = cmdBuilder.toString();
                    os.write(cmd.getBytes(StandardCharsets.UTF_8));
                    os.flush();
                }
                break;
            } catch (IOException ioe) {
                ioe.printStackTrace();
                SystemClock.sleep(10000);
                continue;
            }
        }
    }

    private class wcnassertHandler extends Handler {
        public wcnassertHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            try {
                LocalSocket socket = new LocalSocket();
                LocalSocketAddress socketAddr = new LocalSocketAddress(WCN_MODEM_SOCKET_NAME,
                        LocalSocketAddress.Namespace.ABSTRACT);
                runWcnSocket(socket, socketAddr);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    private void runWcnSocket(LocalSocket socket, LocalSocketAddress socketAddr) {
        byte[] buf = new byte[BUF_SIZE];
        connectToSocket(socket, socketAddr);

        Log.d(MTAG, " -runWcndSocket");
        synchronized(mObjectWcnLock) {
            for (;;) {
                int cnt = 0;
                InputStream is = null;
                try {
                    is = socket.getInputStream();
                    cnt = is.read(buf, 0, BUF_SIZE);
                    Log.d(MTAG, "read " + cnt + " bytes from wcndSocket: \n" );
                } catch (IOException e) {
                    Log.w(MTAG, "read exception\n");
                }
                if (cnt > 0) {
                    String info = "";
                    try {
                        info = new String(buf, 0, cnt, "US-ASCII");
                    } catch (UnsupportedEncodingException e) {
                        e.printStackTrace();
                    } catch (StringIndexOutOfBoundsException e) {
                        e.printStackTrace();
                    }
                    Log.d(MTAG, "read something: " + info);
                    if (TextUtils.isEmpty(info)) {
                        continue;
                    }
                    if (info.contains("WCN-CP2-ALIVE")) {
                        //SPRD: add modem assert reason for PhoneInfo feature
                        sendModemStatBroadcast(MODEM_ALIVE, info);
                        hideNotification(WCND_ASSERT_ID);
                        hideNotification(MODEM_BLOCK_ID);
                    } else if(info.contains("WCN-CP2-EXCEPTION")) {
                        showNotification(WCND_ASSERT_ID, "wcnd assert", info);
                        //SPRD: add modem assert reason for PhoneInfo feature
                        sendModemStatBroadcast(MODEM_ASSERT,info);
                    } else {
                        Log.d(MTAG, "do nothing with info :" + info);
                    }
                    continue;
                } else if (cnt < 0) {
                    try {
                        is.close();
                        socket.close();
                    } catch (IOException e) {
                        Log.w(MTAG, "close exception\n");
                    }
                    socket = new LocalSocket();
                    connectToSocket(socket, socketAddr);
                }
            }
        }
    }
}
