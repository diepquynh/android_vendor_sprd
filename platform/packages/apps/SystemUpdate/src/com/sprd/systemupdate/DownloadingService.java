package com.sprd.systemupdate;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.RandomAccessFile;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.List;

import org.apache.http.HttpHost;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.NameValuePair;
import org.apache.http.client.entity.UrlEncodedFormEntity;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.message.BasicNameValuePair;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.params.HttpParams;
import org.json.JSONObject;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.util.Log;

public class DownloadingService extends Service {
    public Callback mCallback;
    private Context mContext;
    private Storage mStorage;
    private VersionInfo mInfo;
    private DownloadTask mDownloadTask;
    private TokenVerification mTokenVerification;
    private NotificationManager mNotificationManager;
    private Intent mNotifyIntent;
    public static final int MAX_RETRY_COUNT = 3;
    public static final int BUFFER_SIZE = 10240;
    public static final int PUBLISH_STEPS = 100;

    public static final String UPDATE_FILE_NAME = "update.zip";

    public static final String BEGIN_DOWNLOAD_URL = "begin_download_notify";
    public static final String FULL_DOWNLOAD_URL = "full_download_notify";
    public static final String TAG = "SystemUpdate-DownloadingService";

    public static final int SET_TWO_STATE_BUTTON_ENABLED = 0x01;
    public static final int SET_TWO_STATE_BUTTON_PAUSE = 0x02;
    public static final int SET_TWO_STATE_BUTTON_RESUMED = 0x03;
    public static final int UPDATE_FILE_BEING_DELETED = 0x04;

    private static final int ONE_TENTH_MB = 1024 * 1024 / 10;

    private boolean updateFileState = true;

    public interface Callback {

        public void updateProgress(int progress);

        public void endDownload(boolean succ);

        public void setTwoStateButtonEnabled();

        public void setTwoStateButtonPaused();

        public void setTwoStateButtonResumed();

        public void showUpdateFileDeletedDialog();
    }

    private DownloadingBinder binder = new DownloadingBinder();

    public class DownloadingBinder extends Binder {

        public void register(Callback callback) {
            mCallback = callback;
        }

        public void unregister() {
            if (mCallback != null) {
                mCallback = null;
            }
        }

        public void cancel() {
            if (mDownloadTask != null) {
                mDownloadTask.cancel(true);
            }
        }

    }

    public Handler DownloadHandler = new Handler() {
        public void handleMessage(Message msg) {

            Log.i(TAG, "InhandlerMessage+ msg.what" + msg.what);

            Log.i(TAG, "mCallback:" + mCallback);
            if (mCallback != null) {
                switch (msg.what) {
                case SET_TWO_STATE_BUTTON_ENABLED:
                    mCallback.setTwoStateButtonEnabled();
                    break;
                case SET_TWO_STATE_BUTTON_PAUSE:
                    mCallback.setTwoStateButtonPaused();
                    break;
                case SET_TWO_STATE_BUTTON_RESUMED:
                    mCallback.setTwoStateButtonResumed();
                    break;
                case UPDATE_FILE_BEING_DELETED:
                    mCallback.showUpdateFileDeletedDialog();
                default:
                    break;
                }
            }

            super.handleMessage(msg);
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        if (Utils.DEBUG) {
            Log.i(TAG, "DownloadingService--onBind" + binder);
        }
        return binder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mNotifyIntent = new Intent(DownloadingService.this,
                DownloadingActivity.class);
        mNotifyIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
        mContext = this;
        mStorage = Storage.get(mContext);
        mTokenVerification = new TokenVerification(mContext);
        mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);

    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        
        if (Utils.DEBUG) {
            Log.i(TAG, "DownloadingService--onStartCommand");
        }

        mInfo = mStorage.getLatestVersion();
        mDownloadTask = new DownloadTask();
        mDownloadTask.execute();

        return START_STICKY;
    }

    class DownloadTask extends AsyncTask<Object, Integer, Boolean> {

        private PendingIntent mPendingIntent = PendingIntent.getActivity(
                DownloadingService.this, 0, mNotifyIntent, 0);

        public void onPreExecute() {
            if (mInfo == null) {
                return;
            }
            String title = getResources().getString(R.string.downloading);
            String titleAndPercentage = getTitleAndPercentage(title);
            @SuppressWarnings("deprecation")
            Notification notification = new Notification.Builder(
                    DownloadingService.this).setAutoCancel(false)
                    .setContentTitle(titleAndPercentage)
                    .setContentText(mInfo.mVersion)
                    .setWhen(System.currentTimeMillis()).setTicker(title)
                    .setSmallIcon(android.R.drawable.stat_sys_download)
                    .setProgress( mInfo.mSize, 0, false)
                    .setContentIntent(mPendingIntent).setOngoing(true)
                    .getNotification();
            mNotificationManager.notify(0, notification);
            try {
                noticeServer(BEGIN_DOWNLOAD_URL);
            } catch (NoSuchAlgorithmException e) {
                Log.e("DOWNLOAD", "noticeServer" + e.toString());
            }
        }

        public Boolean doInBackground(Object... v) {
            return getDelta();
        }

        public void onPostExecute(Boolean success) {
            
            if (updateFileState == false) {
                if (mCallback == null) {
                    mStorage.setSize(0);
                    final NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
                    notificationManager.cancelAll();
                    mStorage.setState(Storage.State.NIL);
                    new File(mStorage.getStorageFilePath()).delete();

                    Intent intent = new Intent(
                            "sprd.systemupdate.action.UPDATE_FILE_DELETED");
                    sendBroadcast(intent);
                } else {
                    mCallback.showUpdateFileDeletedDialog();
                }
                updateFileState = true;
                return;
            }

            Message message = new Message();
            mNotificationManager.cancel(0);
            if (success) {
                mStorage.setState(Storage.State.DOWNLOADED);
                try {
                    noticeServer(FULL_DOWNLOAD_URL);
                } catch (NoSuchAlgorithmException e) {
                    Log.e(TAG, "noticeServer " + e.toString());
                }
            } else {
                if (mStorage.getState() == Storage.State.NIL_2_DOWNLOADING
                        || mStorage.getState() == Storage.State.PAUSE_2_DOWNLOADING) {
                    mStorage.setState(Storage.State.DOWNLOADING_2_PAUSE);

                    message.what = SET_TWO_STATE_BUTTON_RESUMED;
                    if (DownloadHandler != null) {
                        DownloadHandler.sendMessage(message);
                    }
                } else if (mStorage.getState() == Storage.State.NIL) {
                    // mStorage.setState(Storage.State.NIL);

                } else if (mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE) {
                    message.what = SET_TWO_STATE_BUTTON_RESUMED;
                    if (DownloadHandler != null) {
                        DownloadHandler.sendMessage(message);
                    }
                }
            }

            if (mCallback != null) {
                mCallback.endDownload(success);
            } else {
                Intent intent = new Intent(
                        "sprd.systemupdate.action.DOWNLOAD_RESULT");
                intent.putExtra("result", success);
                sendBroadcast(intent);
            }

        }

        @Override
        protected void onCancelled() {
            Log.i(TAG,
                    "DownloadingService--onCancelled()" + mStorage.getState());
            if (mInfo == null) {
                return;
            }
            if (mStorage.getSize() == mInfo.mSize) {
                mNotificationManager.cancel(0);
                if (mCallback != null) {
                    mCallback.endDownload(true);
                }
                return;
            }

            if (mStorage.getState() == Storage.State.PAUSE_2_DOWNLOADING
                    || mStorage.getState() == Storage.State.NIL_2_DOWNLOADING) {
                mStorage.setState(Storage.State.DOWNLOADING_2_PAUSE);
                Log.i(TAG,
                        "DownloadingService--onCancelled()_again"
                                + mStorage.getState());

            }
            String title = getResources().getString(R.string.pause);
            String titleAndPercentage = getTitleAndPercentage(title);

            @SuppressWarnings("deprecation")
            Notification notification = new Notification.Builder(
                    DownloadingService.this).setAutoCancel(false)
                    .setContentTitle(titleAndPercentage)
                    .setContentText(mInfo.mVersion)
                    .setWhen(System.currentTimeMillis()).setTicker(title)
                    .setSmallIcon(R.drawable.stat_download_pause)
                    .setProgress(mInfo.mSize, mStorage.getSize(), false)
                    .setContentIntent(mPendingIntent).setOngoing(true)
                    .getNotification();
            mNotificationManager.notify(0, notification);
            if (Utils.DEBUG) {
                Log.d(TAG, "cancelled_set_button_enabled");
            }
            Message message = new Message();
            message.what = SET_TWO_STATE_BUTTON_ENABLED;
            if (DownloadHandler != null) {
                DownloadHandler.sendMessage(message);
            }

        }

        @Override
        public void onProgressUpdate(Integer... values) {
            if (mInfo == null) {
                return;
            }
            int progress = values[0];

            String title = getResources().getString(R.string.downloading);
            String titleAndPercentage = getTitleAndPercentage(title);

            @SuppressWarnings("deprecation")
            Notification notification = new Notification.Builder(
                    DownloadingService.this).setAutoCancel(false)
                    .setContentTitle(titleAndPercentage)
                    .setContentText(mInfo.mVersion)
                    .setWhen(System.currentTimeMillis()).setTicker(title)
                    .setSmallIcon(android.R.drawable.stat_sys_download)
                    .setProgress(mInfo.mSize, progress, false)
                    .setContentIntent(mPendingIntent).setOngoing(true)
                    .getNotification();
            mNotificationManager.notify(0, notification);
            if (mCallback != null) {
                mCallback.updateProgress(progress);
            }
        }

        private Boolean getDelta() {
            if (mInfo == null) {
                return false;
            }
            int publishStep = mInfo.mSize / PUBLISH_STEPS;
            int nextThreshHold = publishStep;
            if (new File(mStorage.getStorageFilePath()).exists()) {
                mStorage.setSize((int) new File(mStorage.getStorageFilePath())
                        .length());
            }

            DefaultHttpClient client = new DefaultHttpClient();
            HttpParams httpParams = client.getParams();
            if (httpParams != null) {
                httpParams.setIntParameter(HttpConnectionParams.SO_TIMEOUT,
                        mStorage.getTimeOut());
                httpParams.setIntParameter(
                        HttpConnectionParams.CONNECTION_TIMEOUT,
                        mStorage.getTimeOut());
            } else {
                if (Utils.DEBUG) {
                    Log.i(TAG, "httpParams: " + httpParams);
                }
                return false;
            }

            int total = 0;
            int pre_download = 0;
            InputStream is = null;
            RandomAccessFile raf = null;

            File file = new File(mStorage.getStorageFilePath());
            try {
                String url = mInfo.mUrl.replace(" ", "%20");
                HttpGet get = new HttpGet(url);
                if (mStorage.getSize() != 0) {
                    pre_download = total = mStorage.getSize();
                    get.addHeader("Range", "bytes=" + total + "-" + mInfo.mSize);
                    publishProgress(total);
                }

                if (Utils.DEBUG) {
                    Log.i(TAG, "about to get:" + mInfo.mUrl);
                    Log.i(TAG, "about to get:" + url);
                }
                
                HttpResponse response = client.execute(new HttpHost(
                        PushService.SERVER_ADDR, 3000), get);

                if (response.getStatusLine().getStatusCode() != HttpStatus.SC_OK
                        && response.getStatusLine().getStatusCode() != HttpStatus.SC_PARTIAL_CONTENT) {
                    Log.e(TAG, "DownloadTask--StatusCode"
                            + response.getStatusLine().getStatusCode());
                    return false;
                }

                if (mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE) {
                    mStorage.setState(Storage.State.PAUSE_2_DOWNLOADING);
                } else if (mStorage.getState() == Storage.State.NIL) {
                    mStorage.setState(Storage.State.NIL_2_DOWNLOADING);
                }

                Message message = new Message();
                message.what = SET_TWO_STATE_BUTTON_ENABLED;
                if (DownloadHandler != null) {
                    DownloadHandler.sendMessage(message);
                }

                is = response.getEntity().getContent();
                raf = new RandomAccessFile(file, "rw");
                raf.seek(total);
                byte[] buffer = new byte[BUFFER_SIZE];

                int count;
                try {
                    count = is.read(buffer, 0, BUFFER_SIZE);
                } catch (Exception e) {
                    return false;
                }

                while (count != -1) {

                    if (isUpdateFileEffective(total) == false) {
                        updateFileState = false;
                        return false;
                    }
                    raf.write(buffer, 0, count);
                    total += count;
                    mStorage.setSize(total);
                    if (total > nextThreshHold
                            || (total - pre_download) >= ONE_TENTH_MB) {
                        pre_download = total;
                        publishProgress(total);
                        nextThreshHold += publishStep;
                    }
                    if (isCancelled()) {

                        pre_download = total;
                        publishProgress(mStorage.getSize());
                        nextThreshHold += publishStep;

                        if (raf != null) {
                            raf.close();
                            raf = null;
                        }
                        if (mInfo.mSize == total) {
                            return true;
                        } else {
                            return false;
                        }
                    }

                    try {
                        count = is.read(buffer, 0, BUFFER_SIZE);
                        if (count == -1 && total < mInfo.mSize) {
                            Log.e(TAG, "network is unreached!!!");
                            return false;
                        }

                    } catch (Exception e) {
                        Log.e(TAG, "getDelta_inputstream.read" + e.toString());
                        return false;
                    }

                }

                if (total == mInfo.mSize) {
                    return true;
                } else {
                    return false;
                }

            } catch (Exception e) {
                Log.e(TAG, "DownloadTask" + e.toString());
                return false;
            } finally {
                try {
                    client.getConnectionManager().shutdown();
                    if (raf != null) {
                        raf.close();
                        raf = null;
                    }

                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private void noticeServer(String notice) throws NoSuchAlgorithmException {
        if (mInfo == null) {
            return;
        }
        final String url = notice;
        String token = mTokenVerification.getToken(TokenVerification.NO_SEED);
        String deviceId = mTokenVerification.mDeviceId;
        String delta_name = mInfo.mDelta_name;
        Log.e(TAG, "noticeServer--token:" + token);
        final List<NameValuePair> pairs = new ArrayList<NameValuePair>();
        pairs.add(new BasicNameValuePair("token", token));
        pairs.add(new BasicNameValuePair("jid", deviceId));
        pairs.add(new BasicNameValuePair("delta_name", delta_name));

        new Thread() {

            @Override
            public void run() {
                try {
                    DefaultHttpClient client = new DefaultHttpClient();
                    HttpPost post = new HttpPost("/request/" + url);
                    post.setEntity(new UrlEncodedFormEntity(pairs));
                    HttpResponse response = client.execute(new HttpHost(
                            PushService.SERVER_ADDR, 3000), post);

                    if (response.getStatusLine().getStatusCode() != HttpStatus.SC_OK) {
                        Log.e(TAG, "HTTPStatusCode"
                                + response.getStatusLine().getStatusCode());
                    }
                    BufferedReader reader = new BufferedReader(
                            new InputStreamReader(response.getEntity()
                                    .getContent()));
                    String json = reader.readLine();
                    JSONObject result = new JSONObject(json);
                    int status = result.getInt("status");
                    ErrorStatus
                            .DealStatus(mContext, status, ErrorStatus.NOTICE);
                    Log.e(TAG, "noticeServer--status:" + status);
                } catch (Exception e) {
                    Log.e(TAG, "noticeServer--Exception:" + e.toString());
                    e.printStackTrace();
                }

            }

        }.start();

    }

    private String getTitleAndPercentage(String title) {
        String percentage = mContext.getString(R.string.percentage);
        String downloadPercentage = null;
        if (percentage != null && mStorage.getLatestVersion() != null) {
            downloadPercentage = String.format(percentage,
                    mStorage.getSize() / (mStorage.getLatestVersion().mSize / 100));
        }
        String titleAndPercentage = title + " " + downloadPercentage;
        return titleAndPercentage;
    }

    private boolean isUpdateFileEffective(int size) {
        File file = new File(mStorage.getStorageFilePath());

        if (!file.exists()) {
            return false;
        }

        if (file.length() != size) {
            return false;
        }
        return true;
    }

}
