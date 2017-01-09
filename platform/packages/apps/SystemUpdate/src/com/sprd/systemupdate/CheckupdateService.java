package com.sprd.systemupdate;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.security.NoSuchAlgorithmException;

import org.apache.http.HttpHost;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.conn.ConnectTimeoutException;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.params.HttpConnectionParams;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

public class CheckupdateService extends Service {
    private Callback mCallback;
    private Context mContext;
    private Storage mStorage;
    private CheckUpdateTask mCheckUpdateTask;
    private TokenVerification mTokenVerification;
    private String seed = TokenVerification.NO_SEED;
    private int mRetryCount = 0;
    private String group = "normal";

    public static final int MAX_RETRY_COUNT = 3;
    public static final int CHECK_UPDATE_HAS_UPDATE = 0;
    public static final int CHECK_UPDATE_NO_UPDATE = 1;
    public static final int CHECK_UPDATE_CONNECT_ERROR = -1;
    public static final int CHECK_UPDATE_CONNECT_TIME_OUT = -2;
    public static final int CHECK_UPDATE_SEED_EXPRIED = -3;
    public static final int CHECK_UPDATE_VERSION_NOT_FOUND = -4;
    public static final int CHECK_UPDATE_NEED_REGISTER = -5;

    private static final String TAG = "SystemUpdate--CheckupdateService";

    public interface Callback {
        public void startcheck();

        public void endcheck(Integer i);
    }

    private CheckupdateBinder binder = new CheckupdateBinder();

    public class CheckupdateBinder extends Binder {

        public void register(Callback callback) {
            mCallback = callback;
        }

        public void unregister() {
            mCallback = null;
        }

        public void cancel() {
            mCheckUpdateTask.cancel(true);
        }

    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.i(TAG, "onBind" + binder);
        return binder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        if (Utils.DEBUG) {
            Log.i(TAG, "onCreate--mCallback");
        }
        mContext = this;
        mStorage = Storage.get(mContext);
        mTokenVerification = new TokenVerification(mContext);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        
        if (intent == null) {
            return START_NOT_STICKY;
        }
        group = (String) intent.getExtra("group","normal");
        mCheckUpdateTask = new CheckUpdateTask();
        mCheckUpdateTask.execute();

        return START_STICKY;
    }

    class CheckUpdateTask extends AsyncTask<Void, Integer, Integer> {

        public void onPreExecute() {
            if (mCallback != null) {
                mCallback.startcheck();
            }
        }

        public Integer doInBackground(Void... v) {
            String jid = mStorage.getDeviceId();
            String json = null;
            if (jid == null) {
                mStorage.setLatestVersion(null);
                return CHECK_UPDATE_NO_UPDATE;
            }

            try {
                String token = mTokenVerification.getToken(seed);
                seed = TokenVerification.NO_SEED;
                Log.i(TAG, "CheckUpdateTask--token:" + mTokenVerification
                        + token);
                if (token.equals(TokenVerification.GET_TOKEN_NO_INFO)) {
                    return CHECK_UPDATE_VERSION_NOT_FOUND;
                } else if (token.equals(TokenVerification.CONNECT_ERROR)) {
                    return CHECK_UPDATE_CONNECT_ERROR;
                }
                DefaultHttpClient client = new DefaultHttpClient();
                client.getParams().setIntParameter(
                        HttpConnectionParams.SO_TIMEOUT, mStorage.getTimeOut());
                client.getParams().setIntParameter(
                        HttpConnectionParams.CONNECTION_TIMEOUT,
                        mStorage.getTimeOut());
                Log.i(TAG, "TIMEOUT:" + mStorage.getTimeOut());
                String cmd = "/request/query_deltum/" + token
                        + "&" + jid + "&" + group;
                HttpGet get = new HttpGet(cmd);// + "&" + version
                HttpResponse response = client.execute(new HttpHost(
                        PushService.SERVER_ADDR, 3000), get);
                Log.d(TAG,"HttpGet: " + cmd);
                if (response.getStatusLine().getStatusCode() != HttpStatus.SC_OK) {
                    mStorage.setLatestVersion(null);
                    return CHECK_UPDATE_CONNECT_ERROR;
                }

                BufferedReader reader = new BufferedReader(
                        new InputStreamReader(response.getEntity().getContent()));
                json = reader.readLine();
                Log.i(TAG, "got update:" + json);
                JSONObject mJson = new JSONObject(json);
                int status = mJson.getInt("status");
                Log.i(TAG, "newVersion:" + status);

                int deal_result = ErrorStatus.DealStatus(mContext, status,
                        ErrorStatus.CHECK);
                if (deal_result == CHECK_UPDATE_HAS_UPDATE) {
                    VersionInfo olderVersion = mStorage.getLatestVersion();
                    VersionInfo newVersion = VersionInfo.fromJson(json);
                    if (!newVersion.equals(olderVersion)) {
                        mStorage.setLatestVersion(json);
                        mStorage.setState(Storage.State.NIL);
                        mStorage.setSize(0);
                    }
                }
                return deal_result;

            } catch (ConnectTimeoutException e) {
                return CHECK_UPDATE_CONNECT_TIME_OUT;
            } catch (IOException e) {
                Log.e(TAG, "CheckUpdateTask--IOException:");
                return CHECK_UPDATE_CONNECT_ERROR;
            } catch (NoSuchAlgorithmException e) {
                Log.e(TAG, "CheckUpdateTask--NoSuchAlgorithmException:");
                return CHECK_UPDATE_CONNECT_ERROR;
            } catch (JSONException e) {
                Log.e(TAG, "CheckUpdateTask--JSONException:");
                return CHECK_UPDATE_CONNECT_ERROR;
            }

        }

        public void onPostExecute(Integer i) {
            if (Utils.DEBUG) {
                Log.i(TAG, "CheckUpdateTask--onPostExecute--i:" + i);
            }
            
            if (i == CHECK_UPDATE_SEED_EXPRIED && mRetryCount < MAX_RETRY_COUNT) {
                seed = TokenVerification.SEED_EXPRIED;
                mCheckUpdateTask = new CheckUpdateTask();
                mCheckUpdateTask.execute();
                mRetryCount++;
            } else if (i == CHECK_UPDATE_NEED_REGISTER
                    && mRetryCount < MAX_RETRY_COUNT) {
                Log.i(TAG, "CheckUpdateTask--mRetryCount:" + mRetryCount);
                Intent intent = new Intent(mContext, PushService.class);
                intent.putExtra("seed", PushService.NEED_REGISTER);
                mContext.startService(intent);
                mCheckUpdateTask = new CheckUpdateTask();
                mCheckUpdateTask.execute();
                mRetryCount++;
            } else {
                mRetryCount = 0;
                if (mCallback != null) {
                    mCallback.endcheck(i);
                } else if (i == CHECK_UPDATE_HAS_UPDATE) {
                    Intent intent = new Intent(
                            "sprd.systemupdate.action.CHECK_RESULT");
                    sendBroadcast(intent);
                }
            }
        }
    }

}
