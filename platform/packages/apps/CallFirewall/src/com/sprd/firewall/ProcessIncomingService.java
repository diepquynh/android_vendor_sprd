
package com.sprd.firewall;

import android.app.Service;
import android.content.Intent;
import android.database.Cursor;
import android.os.Binder;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.provider.CallLog;
import android.util.Log;

public class ProcessIncomingService extends Service {
    private static final String TAG = "ProcessIncomingService";

    private Looper mlooper;

    private ServiceHandler mServiceHandler;

    static final Object mStartingServiceSync = new Object();

    @Override
    public void onCreate() {
        super.onCreate();
        Log.v(TAG, "onCreate");
        HandlerThread mHandlerThread = new HandlerThread("process_incoming_service_thread",
                Process.THREAD_PRIORITY_BACKGROUND);
        mHandlerThread.start();
        mlooper = mHandlerThread.getLooper();
        mServiceHandler = new ServiceHandler(mlooper);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        synchronized (mStartingServiceSync) {
            Log.v(TAG, "onStartCommand");
            Message msg = mServiceHandler.obtainMessage();
            msg.arg1 = startId;
            msg.obj = intent;
            mServiceHandler.sendMessageDelayed(msg, 2 * 1000);

            return super.onStartCommand(intent, flags, startId);
        }

    }

    private final class ServiceHandler extends Handler {

        public ServiceHandler(Looper mlooper) {
            super(mlooper);
        }

        @Override
        public void handleMessage(Message msg) {
            int serviceId = msg.arg1;
            Intent intent = (Intent) msg.obj;
            if (intent != null) {
                String action = intent.getAction();
                Log.v(TAG, "handleMessage - action is " + action);
                if (intent.getExtras() != null) {
                    String incomingNumber = intent.getStringExtra("incomingNumber");
                    Log.v(TAG, "incomingNumber = " + incomingNumber);
                    Cursor cursor = ProcessIncomingService.this.getContentResolver().query(
                            CallLog.Calls.CONTENT_URI, null, null, null,
                            CallLog.Calls.DEFAULT_SORT_ORDER);
                    try {
                        if (cursor.moveToFirst()) {
                            do {
                                int _id = cursor.getInt(cursor.getColumnIndex(CallLog.Calls._ID));
                                String number = cursor.getString(cursor
                                        .getColumnIndex(CallLog.Calls.NUMBER));
                                Log.v(TAG, "_id = " + _id);
                                Log.v(TAG, "number = " + number);
                                if (number.equals(incomingNumber)) {
                                    Log.v(TAG, "incomingNumber = number " + incomingNumber);
                                    if (ProcessIncomingService.this.getContentResolver().delete(
                                            CallLog.Calls.CONTENT_URI,
                                            CallLog.Calls._ID + "=" + _id, null) != 0)
                                        return;
                                }
                            } while (cursor.moveToNext());
                        }
                    } finally {
                        if (cursor != null) {
                            cursor.close();
                        }
                        synchronized (mStartingServiceSync) {
                            FireWallApp.finishStartingService(ProcessIncomingService.this,
                                    serviceId);
                        }
                    }
                }
            }

        }

    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    private final IBinder mBinder = new BlockServiceBinder();

    public class BlockServiceBinder extends Binder {
        ProcessIncomingService getService() {
            return ProcessIncomingService.this;
        }
    }
}
