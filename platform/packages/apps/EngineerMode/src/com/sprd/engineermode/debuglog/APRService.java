
package com.sprd.engineermode.debuglog;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.Date;

import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;
import android.preference.PreferenceManager;

import org.apache.http.entity.mime.*;
import org.apache.http.entity.mime.content.*;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;

import com.sprd.engineermode.R;

import android.net.ConnectivityManager;
import android.net.NetworkInfo;

public class APRService extends Service {

    private static final String TAG = "APRService";
    public static final String ACTION_START = "android.intent.action.ACTION_START";
    public static final String DATA_OCCUR_ERROR = "data apr.xml file not exists!";
    public static final String ACTION_APR_SERVER_START = "com.sprd.engineermode.debuglog.aprservice_start";
    public static final String ACTION_APR_SERVER_END = "com.sprd.engineermode.debuglog.aprservice_end";
    public static final String ACTION_APR_SERVICE_RESTART = "com.sprd.engineermode.debuglog.aprservice_restart";
    public static final String SHARED_PREFS_APR_FILE = "shared_prefs_apr_file";
    private static final String KEY_DATEFORMAT = "yyyy.MM.dd HH:mm:ss";

    public static final String KEY_INTERNAL = "internal";
    private static int dataIndex = 0;
    public static ServiceHandler mServiceHandler;
    private Looper mServiceLooper;
    private SharedPreferences mPreference;

    public static final int EVENT_REQUEST_UPLOAD = 1;
    private int mInterval = 0;
    private int mStartID;
    private int mUploadIndex = 0;
    private static SimpleDateFormat mDateFormat = new SimpleDateFormat(KEY_DATEFORMAT);

    @Override
    public IBinder onBind(Intent intent) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public void onCreate() {
        Log.d(TAG, "onCreate");

        mPreference = PreferenceManager.getDefaultSharedPreferences(this);
        HandlerThread thread = new HandlerThread("APRService");
        thread.start();
        mServiceLooper = thread.getLooper();
        mServiceHandler = new ServiceHandler(mServiceLooper);
        sendBroadcast(new Intent(ACTION_APR_SERVER_START));

    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "onStartCommand startId:" + startId);
        mStartID = startId;
        if (intent != null) {
            mServiceHandler.sendEmptyMessage(EVENT_REQUEST_UPLOAD);
        }
        return Service.START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        sendBroadcast(new Intent(ACTION_APR_SERVER_END));
        super.onDestroy();
    }

    private void nextAlarm() {

        Intent intent = new Intent(ACTION_APR_SERVICE_RESTART);
        PendingIntent operation = PendingIntent.getBroadcast(this, 0, intent,
                PendingIntent.FLAG_ONE_SHOT);
        AlarmManager am = (AlarmManager) this.getSystemService(Context.ALARM_SERVICE);
        String intervalTime = SystemProperties.get(DebugLogFragment.KEY_APR_CUSTOMER_TIME, "1");
        int i = Integer.parseInt(intervalTime);
        long dutime = System.currentTimeMillis() + (i * 60 * 60 * 1000);
        Log.d(TAG, " nextAlarm at:" + dutime);
        am.cancel(operation);
        am.set(AlarmManager.RTC, dutime, operation);
    }

    private void uploadFile() {
        if (!isConnected()) {
            //Add for bug498082, Autoupload APR failure pop toast when power affect customer experience.
            //Toast.makeText(this, R.string.aler_upload_aprfile, Toast.LENGTH_SHORT).show();
            nextAlarm();
            saveStringToFile(this, "fail  no network");
            return;
        }
        String softwore = android.os.Build.VERSION.INCREMENTAL;
        String phoneName = android.os.Build.DEVICE;
        try {
            MultipartEntity mEntity = new MultipartEntity();
            String phoneModel = SystemProperties.get("ro.product.model");
            StringBody sbody = new StringBody(phoneModel);
            StringBody sbody1 = new StringBody(phoneName);
            StringBody sbody2 = new StringBody(softwore);
            String phoneSN = SystemProperties.get("ro.serialno");
            StringBody sbody3 = new StringBody(phoneSN);
            String testGroup = SystemProperties.get("persist.sys.apr.testgroup", "CSSLAB");
            StringBody sbody4 = new StringBody(testGroup);
            String projectInfo = SystemProperties.get("ro.product.name") + "_"
                    + SystemProperties.get("ro.build.type") + "_"
                    + SystemProperties.get("ro.build.version.release");
            StringBody sbody5 = new StringBody(projectInfo);
            mEntity.addPart("phoneModel", sbody);
            mEntity.addPart("phoneName", sbody1);
            mEntity.addPart("phoneSoftwore", sbody2);
            mEntity.addPart("phoneSN", sbody3);
            mEntity.addPart("group", sbody4);
            mEntity.addPart("projectInfo", sbody5);
            File aprFile = new File("/data/sprdinfo/apr.xml");
            if (aprFile.exists()) {
                FileBody file = new FileBody(aprFile);
                mEntity.addPart("apr", file);
                HttpPost post = new HttpPost("http://222.66.158.137:8080/sendfile1/sendfile.do");
                post.setEntity(mEntity);
                DefaultHttpClient dhc = new DefaultHttpClient();
                HttpResponse response = dhc.execute(post);
                int status = response.getStatusLine().getStatusCode();
                if (status == 200) {
                    saveStringToFile(this, "success");
                    Log.e(TAG, "response success! , upload data phoneModel: " + phoneModel
                            + ",phoneName:" + phoneName + ",phoneSN:" + phoneSN + ",testGroup:"
                            + testGroup);
                    nextAlarm();
                } else {
                    Log.e(TAG, "response " + status);
                    saveStringToFile(this, "upload failed!! retry that.");
                    /*
                     * Toast.makeText(this, "upload failed!! retry that. ",
                     * Toast.LENGTH_SHORT).show();
                     */
                    mServiceHandler.sendEmptyMessageDelayed(EVENT_REQUEST_UPLOAD, 60 * 1000);
                }
            } else {
                //Add for bug498082, Autoupload APR failure pop toast when power affect customer experience.
                //Toast.makeText(this, DATA_OCCUR_ERROR, Toast.LENGTH_LONG).show();
                Log.e(TAG, DATA_OCCUR_ERROR);
                saveStringToFile(this, DATA_OCCUR_ERROR);
                nextAlarm();
            }
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            nextAlarm();
            saveStringToFile(this, "upload failed!");
            //Add for bug498082, Autoupload APR failure pop toast when power affect customer experience.
            //Toast.makeText(this, "upload failed!", Toast.LENGTH_SHORT).show();
        }
    }

    private boolean isConnected() {
        boolean isConnected = false;
        ConnectivityManager connectManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo netinfo = connectManager.getActiveNetworkInfo();
        if (netinfo != null) {
            if (netinfo.getType() == ConnectivityManager.TYPE_MOBILE
                    || netinfo.getType() == ConnectivityManager.TYPE_WIFI) {
                isConnected = true;
            }
        }
        return isConnected;
    }

    public class ServiceHandler extends Handler {
        public ServiceHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case EVENT_REQUEST_UPLOAD:
                    String aprSwitch = SystemProperties.get("init.svc.apr", "stopped");
                    if (aprSwitch.equals("running")) {
                        uploadFile();
                    } else {
                        stopSelf(mStartID);
                    }
                    break;
            }
        }
    }

    public static void saveStringToFile(Context context, String logString) {
        if (context == null) {
            return;
        }
        SharedPreferences prefs = context.getSharedPreferences(SHARED_PREFS_APR_FILE,
                Context.MODE_PRIVATE);
        long count = prefs.getLong("apr_history_count", 0);
        StringBuilder build = new StringBuilder();
        build.append((count + 1) + ".  ");
        build.append(mDateFormat.format(new Date()));
        build.append("  " + logString + "\n");

        String data = build.toString();
        FileOutputStream fos = null;
        try {
            File file = new File(DebugLogFragment.APR_UPLOAD_HISTORY_PATH);
            if (!file.exists()) {
                file.createNewFile();
            }

            fos = new FileOutputStream(file, true);
            fos.write(data.getBytes());
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }

        SharedPreferences.Editor editor = prefs.edit();
        editor.putLong("apr_history_count", count + 1);
        editor.apply();
    }

    public static ServiceHandler getServiceHandler() {
        return mServiceHandler;
    }

}
