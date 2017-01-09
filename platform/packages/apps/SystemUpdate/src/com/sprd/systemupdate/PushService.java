package com.sprd.systemupdate;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.apache.http.HttpHost;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.NameValuePair;
import org.apache.http.client.entity.UrlEncodedFormEntity;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.message.BasicNameValuePair;
import org.jivesoftware.smack.ConnectionConfiguration;
import org.jivesoftware.smack.PacketListener;
import org.jivesoftware.smack.XMPPConnection;
import org.jivesoftware.smack.filter.PacketTypeFilter;
import org.jivesoftware.smack.packet.Message;
import org.jivesoftware.smack.packet.Packet;
import org.jivesoftware.smack.packet.Presence;
import org.json.JSONObject;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.util.Log;

public class PushService extends Service {
    public static final String SERVER_ADDR = "222.66.158.131";
    public static final String KEY_MODE = "mode";
    public static final String NEED_REGISTER = "need_register";
    private static final String TAG = "PushService";

    public static final int MODE_NETWORK_DOWN = 0;
    public static final int MODE_NETWORK_UP = 1;
    public static final int TIME_SLICE = 2000;
    public static final int MAX_RETRY_COUNT = 12;

    public static final int REGISTER_SUCCEED = 100;
    public static final int REGISTER_FAILED = 101;
    public static final int REGISTER_SEED_EXPRIED = 12;

    private static final int PUSH_CODE = 1;

    private int mRetryCount = 0;

    private Handler mHandler;

    private XMPPConnection mConnection;

    private Context mContext;
    private Storage mStorage;
    private TokenVerification mTokenVerification;
    private String seed;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private boolean tryRegister() {

        if (registerIfNeeded() == REGISTER_FAILED) {
            startXmppConnection();
            Log.e(TAG, "cellular registration failed");
            return false;
        }
        
        return true;
    }

    public void onCreate() {
        mContext = this;
        mStorage = Storage.get(mContext);
        mTokenVerification = new TokenVerification(mContext);
        HandlerThread thread = new HandlerThread("push_receiver");
        thread.start();
        mHandler = new Handler(thread.getLooper()) {
            @Override
            public void handleMessage(android.os.Message msg) {
                if (msg.what == MODE_NETWORK_DOWN) {
                    removeMessages(MODE_NETWORK_UP);
                } else if (msg.what == MODE_NETWORK_UP) {
                    removeMessages(MODE_NETWORK_UP);
                    if (seed == null && !tryRegister()
                            && mRetryCount < MAX_RETRY_COUNT) {
                        // reschedule a retry
                        android.os.Message retryMsg = new android.os.Message();
                        retryMsg.what = MODE_NETWORK_UP;
                        int wakeUpTime = new Random(System.currentTimeMillis())
                                .nextInt(2 << (++mRetryCount)) * TIME_SLICE;
                        Log.i(TAG, "registration: retry in " + wakeUpTime);
                        sendMessageDelayed(retryMsg, wakeUpTime);
                    } else {
                        mRetryCount = 0;
                    }
                } else {
                    Log.e(TAG, "unknow mode:" + msg.what);
                }
            }
        };
    }

    private boolean startXmppConnection() {
        final ConnectionConfiguration connectionConfig = new ConnectionConfiguration(
                SERVER_ADDR, 5222, "ota");
        String jid = mTokenVerification.mDeviceId;
        if (jid == null) {
            return false;
        }
        mConnection = new XMPPConnection(connectionConfig);
        try {
            mConnection.connect();
            mConnection.login(jid, jid);
            Presence presence = new Presence(Presence.Type.available);
            mConnection.sendPacket(presence);
            mConnection.addPacketListener(new PacketListener() {
                public void processPacket(Packet pkt) {
                    Message msg = (Message) pkt;
                    if (Utils.DEBUG) {
                        Log.i(TAG, "got push:" + msg.getBody());
                    }
                    String json = msg.getBody();
                    VersionInfo info = VersionInfo.fromJson(json);
                    if (info != null) {
                        VersionInfo olderVersion = mStorage.getLatestVersion();

                        PendingIntent pendingIntent;
                        if (info.equals(olderVersion)) {
                            pendingIntent = sameVersionPushProcess();
                        } else {
                            pendingIntent = newVersionPushProcess(json);
                        }

                        String title = getResources().getString(
                                R.string.latest_update);

                        @SuppressWarnings("deprecation")
                        Notification notification = new Notification.Builder(
                                PushService.this).setAutoCancel(true)
                                .setContentTitle(title)
                                .setContentText(info.mVersion)
                                .setWhen(System.currentTimeMillis())
                                .setTicker(title)
                                .setSmallIcon(R.drawable.ic_push)
                                .setContentIntent(pendingIntent)
                                .setOngoing(false).getNotification();
                        notification.defaults = Notification.DEFAULT_ALL;
                        NotificationManager nm = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
                        nm.notify(1, notification);
                    }
                }
            }, new PacketTypeFilter(Message.class));
            return true;
        } catch (Exception e) {
            Log.e(TAG, "startXmppConnection:" + e.getMessage());
            return false;
        }

    }

    private int registerIfNeeded() {
        String token;
        try {
            if (seed == null) {
                token = mTokenVerification
                        .getToken(TokenVerification.FIRST_REGISTER);
            } else if (seed.equals(NEED_REGISTER)) {
                token = mTokenVerification.getToken(TokenVerification.NO_SEED);
            } else {
                token = mTokenVerification.getToken(seed);
            }
            seed = null;
            String version = mTokenVerification.mVersion;
            String product = mTokenVerification.mProduct;
            String deviceId = mTokenVerification.mDeviceId;

            List<NameValuePair> pairs = new ArrayList<NameValuePair>();
            pairs.add(new BasicNameValuePair("version", version));
            pairs.add(new BasicNameValuePair("product", product));
            pairs.add(new BasicNameValuePair("jid", deviceId));
            pairs.add(new BasicNameValuePair("token", token));
            if (token.equals(TokenVerification.GET_TOKEN_NO_INFO)
                    || token == null) {
                return REGISTER_FAILED;
            }
            DefaultHttpClient client = new DefaultHttpClient();

            HttpPost post = new HttpPost("/request/register");
            post.setEntity(new UrlEncodedFormEntity(pairs));
            HttpResponse response = client.execute(new HttpHost(SERVER_ADDR,
                    3000), post);
            if (response.getStatusLine().getStatusCode() != HttpStatus.SC_OK) {
                Log.e(TAG, "pushservice-return"
                        + response.getStatusLine().getStatusCode());
                return REGISTER_FAILED;
            } else {
                BufferedReader reader = new BufferedReader(
                        new InputStreamReader(response.getEntity().getContent()));
                String json = reader.readLine();
                JSONObject result = new JSONObject(json);
                int status = result.getInt("status");
                int deal_result = ErrorStatus.DealStatus(mContext, status,
                        ErrorStatus.REGISTER);
                if (deal_result == REGISTER_SEED_EXPRIED) {
                    seed = TokenVerification.SEED_EXPRIED;
                    return REGISTER_FAILED;
                } else {
                    return deal_result;
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "pushservice--Exception" + e.toString());
            return REGISTER_FAILED;
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flag, int startId) {
        if (intent == null) {
            return START_NOT_STICKY;
        }
        seed = intent.getStringExtra("seed");
        if (Utils.DEBUG) {
            Log.i(TAG, "PushService--seed:" + seed);
        }
        if (seed == null) {
            int mode = intent.getIntExtra(KEY_MODE, -1);
            mHandler.obtainMessage(mode).sendToTarget();
        } else {
            new Thread() {
                public void run() {
                    registerIfNeeded();
                }
            }.start();
        }
        return START_STICKY;
    }

    public PendingIntent sameVersionPushProcess() {
        PendingIntent pendingIntent;

        Intent intent;
        if (mStorage.getState() == Storage.State.PAUSE_2_DOWNLOADING
                || mStorage.getState() == Storage.State.NIL_2_DOWNLOADING
                || mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE
                || mStorage.getState() == Storage.State.PAUSE_2_PAUSE) {
            intent = new Intent(PushService.this, DownloadingActivity.class);
        } else if (mStorage.getState() == Storage.State.DOWNLOADED) {
            intent = new Intent(PushService.this, UpgradeActivity.class);
        } else if (mStorage.getState() == Storage.State.WAIT_UPDATE) {
            intent = new Intent(PushService.this, SystemUpdateActivity.class);
        } else {
            intent = new Intent(PushService.this, LatestUpdateActivity.class);
        }
        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
        intent.putExtra("from_where", Storage.fromWhere.NOTIFI_OLD);
        pendingIntent = PendingIntent.getActivity(PushService.this, PUSH_CODE,
                intent, 0);
        return pendingIntent;
    }

    public PendingIntent newVersionPushProcess(String json) {
        PendingIntent pendingIntent;
        Intent intent = new Intent(PushService.this, LatestUpdateActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
        intent.putExtra("from_where", Storage.fromWhere.NOTIFI_NEW);
        if (mStorage.getState() == Storage.State.PAUSE_2_DOWNLOADING
                || mStorage.getState() == Storage.State.NIL_2_DOWNLOADING
                || mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE
                || mStorage.getState() == Storage.State.PAUSE_2_PAUSE) {
            mStorage.setTmpLatestVersion(json);
        } else {
            mStorage.setTmpLatestVersion(json);
            mStorage.setLatestVersion(json);
        }
        pendingIntent = PendingIntent.getActivity(PushService.this, PUSH_CODE,
                intent, 0);
        return pendingIntent;
    }
}
