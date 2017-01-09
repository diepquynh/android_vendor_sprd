package com.spreadst.validator;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import android.util.Log;

public class PingActivity extends Activity{

    private static final String TAG = "PingActivity";

    private Button mPingButton = null;

    private Button mDataSwitchButton = null;

    private TextView mResultView = null;

    private ProgressDialog mProgressDialog = null;

    private final StringBuffer mResult = new StringBuffer();

    private static final String PING_CMD = "/system/bin/ping -c 4 www.baidu.com";

    private MainHandler mMainHandler = null;

    private WifiManager mWifiManager = null;

    private ConnectivityManager mConnectivityManager = null;

    private static final int PING_PAGE_CHANGE = 0;

    private static final int CLEAR_SCREEN = 1;

    private class MainHandler extends Handler {
        public MainHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            Log.d(TAG, "handle message msg.what = "+msg.what);
            switch (msg.what) {
            case PING_PAGE_CHANGE:
                mResultView.setText(mResult.toString());
                if (mProgressDialog.isShowing()) {
                    mProgressDialog.dismiss();
                }
                break;
            case CLEAR_SCREEN:
                mResult.delete(0, mResult.length());
                break;
            default :
                break;
            }
        }
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate");
        setContentView(R.layout.ping_scan_view);

        mMainHandler = new MainHandler(getMainLooper());
        mWifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        mConnectivityManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
//        mConnectivityManager.setMobileDataEnabled(true);
        mResultView = (TextView) findViewById(R.id.ping_result);
        mPingButton = (Button) findViewById(R.id.ping_btn);
        mPingButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(final View v) {
                if (!hasAvaliableMobileNetwork()) {
                    Log.d(TAG, "no mobile data connection");
                    Toast.makeText(PingActivity.this, R.string.no_mobile_data, Toast.LENGTH_SHORT).show();
                    return;
                }
                String pingMessage = getResources().getString(R.string.ping_connect);
                mProgressDialog = ProgressDialog.show(PingActivity.this, "", pingMessage);
                ping(PING_CMD);
            }
        });
        mDataSwitchButton = (Button) findViewById(R.id.data_switch_btn);
        mDataSwitchButton.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
//                mConnectivityManager.setMobileDataEnabled(true);
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");
        if (mWifiManager.isWifiEnabled()) {
            mWifiManager.setWifiEnabled(false);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "onPause");
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy");
//        mConnectivityManager.setMobileDataEnabled(false);
    }

    private void ping(final String cmdPing) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "ping run");
                cleanScreen();
                String str;
                try {
                    Process p = Runtime.getRuntime().exec(cmdPing);
                    BufferedReader bufferReader = new BufferedReader(new InputStreamReader(p.getInputStream()));
                    int status = p.waitFor();
                    if (status == 0) {
                        mResult.append("===========success==========\r\n");
                    } else {
                        mResult.append("===========failed==========\r\n");
                    }
                    while ((str = bufferReader.readLine()) != null) {
                        pingPageChange(str, status);
                    }
                    bufferReader.close();
                } catch (IOException e) {
                    e.printStackTrace();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }).start();
    }

    private void pingPageChange(String str, int status) {
        mResult.append(str).append("\r\n");
        mMainHandler.sendEmptyMessage(PING_PAGE_CHANGE);
    }

    private void cleanScreen() {
        mMainHandler.sendEmptyMessage(CLEAR_SCREEN);
    }

    private boolean hasAvaliableMobileNetwork() {
        return mConnectivityManager.getActiveNetworkInfo() != null &&
                mConnectivityManager.getActiveNetworkInfo().getType() == ConnectivityManager.TYPE_MOBILE;
    }
}
