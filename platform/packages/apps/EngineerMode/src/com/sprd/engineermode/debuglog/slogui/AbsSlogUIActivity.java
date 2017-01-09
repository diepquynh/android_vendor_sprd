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

import com.sprd.engineermode.R;
import com.sprd.engineermode.debuglog.slogui.SlogUIAlert.AlertCallBack;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.FileObserver;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;
import android.widget.Toast;
import java.io.File;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.DialogInterface;

import static com.sprd.engineermode.debuglog.slogui.SlogAction.SLOG_CONFIG_PATH;
import com.sprd.engineermode.debuglog.slogui.StorageUtil.StorageChangedListener;

/**
 * All activities except Dialogs show in SlogUI should extends from this one. <br>
 * <b>Overrides</b> {@link AbsSlogUIActivity#syncState()}
 */
public abstract class AbsSlogUIActivity extends Activity{
    public static final String TAG = "AbsSlogUIActivity";
    public static final int WAITING_FOR_START_ACTIVITY = 2;
    /**LOGPATHCHANGE mark sdcard state has changed*/
    public static final String LOGPATHCHANGE = "log_path_chage_to_internel";
    /** The package name of Slog */
    private static final String APPLICATION_PACKAGE_NAME = "com.spreadtrum.android.eng.slogui";
    private static final String SLOG_SERVICE_CLASS = ".SlogService";
    public static final ComponentName SLOG_SERVICE_COMPONENT_NAME = new ComponentName(
            APPLICATION_PACKAGE_NAME, SLOG_SERVICE_CLASS);
    LayoutInflater mInflater;
    Context mContext = this;
    protected boolean mServiceEnable = false;
    public boolean mLoadStateEnable = false;
    protected Handler mMainThreadHandler;
    protected HandlerThread mHandlerThread;
    protected Handler mHandler;
    private volatile boolean mEnableReload = true;

    abstract void commit();

    private SlogConfListener mSlogConfListener = null;
    private SharedPreferences mSettings;

    // /**
    // * The runnable of making configuration take effect.
    // */
    // private Runnable mReloadRunnable = new Runnable() {
    // @Override
    // public void run() {
    // SlogAction.reload();
    // }
    // };

    private BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            if (SlogAction.ACTION_DUMPLOG_COMPLETED.equals(intent.getAction())) {
                onDumpLogEnded();
                mEnableReload = true;
            } else if (SlogAction.ACTION_CLEARLOG_COMPLETED.equals(intent
                    .getAction())) {
                onClearLogEnded();
                mEnableReload = true;
            }

        }
    };
    
    private StorageChangedListener mStorageChangedListener = new StorageChangedListener() {
        @Override
        public void onStorageChanged(String path, boolean available) {
            // when mCache is null, SlogAction.getState(SlogAction.STORAGEKEY)
            // return internel
            if (!mSettings.getBoolean(LOGPATHCHANGE, false) && !available) {
                mSettings.edit().putBoolean(LOGPATHCHANGE, true).commit();
                AlertDialog.Builder alert = new AlertDialog.Builder(mContext);
                alert.setMessage(R.string.log_path_chage_to_internel);
                alert.setCancelable(false);
                alert.setNeutralButton(getString(R.string.alertdialog_ok),
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                            }
                        });
                alert.create();
                alert.show();
                updateSlogConfig();
            }
            /**mount proc*/
            if (mSettings.getBoolean(LOGPATHCHANGE, false) && available) {
                mSettings.edit().putBoolean(LOGPATHCHANGE, false).commit();
                AlertDialog.Builder alert = new AlertDialog.Builder(mContext);
                alert.setMessage(R.string.log_path_chage_to_externel);
                alert.setCancelable(false);
                alert.setNeutralButton(getString(R.string.alertdialog_ok),
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                syncState();
                            }
                        });
                alert.create();
                alert.show();
                updateSlogConfig();
            }
        }
    };

    protected static final void setViewVisibility(View v, boolean visible) {
        if (v != null) {
            v.setVisibility(visible ? View.VISIBLE : View.GONE);
        }
    }

    /**
     * The runnable of the setting slog configuration, and make configuration
     * take effect
     */
    protected Runnable mCommitConfigRunnable = new Runnable() {
        @Override
        public void run() {
            if (mEnableReload) {
                SlogAction.writeSlogConfig();
                SlogAction.reloadCache(mSlogConfListener);
            }
        }
    };

    /*private FileObserver mSlogConfigCreateObserver = new FileObserver(
            SLOG_CONFIG_PATH, FileObserver.CREATE) {

        @Override
        public void onEvent(int event, String path) {
            if (mSlogConfListener != null) {
                mSlogConfListener.onSlogConfigChanged();

            }

        }
    };

    private FileObserver mSlogConfigChangeObserver = new FileObserver(
            SLOG_CONFIG_PATH, FileObserver.CLOSE_WRITE) {

        @Override
        public void onEvent(int event, String path) {
            if (mSlogConfListener != null) {
                mSlogConfListener.onSlogConfigChanged();
            }

        }
    };*/

    abstract void onSlogServiceConnected();

    abstract void onSlogServiceDisconnected();

    /**
     * The listener of service connection, you should can call
     * {@code AbsSlogUIActivity.setSlogServiceConnectListener(SlogServiceConnectListener)}
     */
    interface SlogServiceConnectListener {

        void onSlogServiceConnected();

        void onSlogServiceDisconnected();
    }

    protected ISlogService mService;
    private ServiceConnection mSlogConnection = new ServiceConnection() {

        @Override
        public void onServiceDisconnected(ComponentName name) {
            onSlogServiceDisconnected();
        }

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mService = ISlogService.Stub.asInterface(service);
            mServiceEnable = true;
            onSlogServiceConnected();
        }
    };

    public void dumpLog() {
        if (mHandler == null
                || mHandler.hasMessages(WAITING_FOR_START_ACTIVITY)) {
            return;
        }
        AlertCallBack callback = new AlertCallBack() {

            @Override
            public void onTextAccept(int which, String text) {
                if (null == text) {
                    return;
                }
                String fileName = text;
                java.util.regex.Pattern pattern = java.util.regex.Pattern
                        .compile("[0-9a-zA-Z-_]*");
                if ((pattern.matcher(fileName).matches() && !""
                        .equals(fileName)) && fileName.length() <= 40) {
                    onDumpLogStarted(SlogAction.dump(fileName,
                            AbsSlogUIActivity.this));
                    mEnableReload = false;
                } else {
                    Toast.makeText(
                            AbsSlogUIActivity.this,
                            fileName.length() > 40 ? getText(R.string.toast_dump_input_limited)
                                    : getText(R.string.toast_dump_filename_error),
                            Toast.LENGTH_LONG).show();
                }
            }

            @Override
            public void onClick(int which) {
                // do nothing
            }
        };

        Intent intent = SlogUIAlert.prepareIntent()
                .setTitle(R.string.alert_dump_title)
                .setEditTextAccepter(R.string.alert_dump_title, callback)
                .setPositiveButton(R.string.alert_dump_dialog_ok, callback)
                .setNegativeButton(R.string.alert_dump_dialog_cancel, callback)
                .generateIntent();
        intent.setClass(this, SlogUIAlert.class);
        startActivity(intent);
        mHandler.sendEmptyMessageDelayed(WAITING_FOR_START_ACTIVITY, 5000);
    }

    public void clearLog() {
        if (mHandler == null
                || mHandler.hasMessages(WAITING_FOR_START_ACTIVITY)) {
            return;
        }
        AlertCallBack callBack = new AlertCallBack() {

            @Override
            public void onTextAccept(int which, String text) {
                // empty

            }

            @Override
            public void onClick(int which) {
                if (which == R.id.positive_button) {
                    onClearLogStarted(SlogAction.clear(AbsSlogUIActivity.this));
                    mEnableReload = false;
                }

            }
        };
        Intent intent = SlogUIAlert.prepareIntent()
                .setTitle(R.string.alert_clear_title)
                .setMessage(R.string.alert_clear_string, "")
                .setPositiveButton(R.string.alert_clear_dialog_ok, callBack)
                .setNegativeButton(R.string.alert_clear_dialog_cancel, null)
                .generateIntent();
        intent.setClass(this, SlogUIAlert.class);
        startActivity(intent);
        mHandler.sendEmptyMessageDelayed(WAITING_FOR_START_ACTIVITY, 5000);
    }

    protected void onDumpLogStarted(boolean enabled) {

    }

    protected void onDumpLogEnded() {

    }

    protected void onClearLogStarted(boolean enabled) {

    }

    protected void onClearLogEnded() {

    }

    /**
     * You can refresh your switches states with syncState(), call them in
     * SlogConfListener#onSlogconfigChanged or Activity#onResume
     */
    public abstract void syncState();

    public boolean isServiceEnable() {
        return mServiceEnable;
    }

    public void bindSlogService() {
        Intent intent = new Intent();
        // intent.setComponent(SLOG_SERVICE_COMPONENT_NAME);
        intent.setClass(getApplicationContext(), SlogService.class);
        bindService(intent, mSlogConnection, BIND_AUTO_CREATE);
    }

    public void unbindSlogUIService() {
        unbindService(mSlogConnection);
    }

    public ISlogService getSlogService() {
        return mService;
    }

    public void setSlogConfigChangedListener(SlogConfListener listener) {
        mSlogConfListener = listener;
    }
    



    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(SlogAction.ACTION_CLEARLOG_COMPLETED);
        intentFilter.addAction(SlogAction.ACTION_DUMPLOG_COMPLETED);
        registerReceiver(mBroadcastReceiver, intentFilter);
        StorageUtil.setStorageChangeListener(mStorageChangedListener);
        mHandlerThread = new HandlerThread("SlogUIInternalWorkingThread");
        mHandlerThread.start();
        mHandler = new Handler(mHandlerThread.getLooper());
        mMainThreadHandler = new Handler(getMainLooper());
        mInflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        //mSlogConfigCreateObserver.startWatching();
       // mSlogConfigChangeObserver.startWatching();

        mLoadStateEnable = SlogAction.reloadCacheIfInvalid(mSlogConfListener);
        mSettings = getApplicationContext().getSharedPreferences("settings",
                Context.MODE_PRIVATE);
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        unregisterReceiver(mBroadcastReceiver);
        mEnableReload = true;
        new Thread(mCommitConfigRunnable).start();
        //mSlogConfigCreateObserver.stopWatching();
       // mSlogConfigChangeObserver.stopWatching();
        mHandlerThread.quit();
        StorageUtil.removeListener(mStorageChangedListener);
        super.onDestroy();
    }

    public static final void setCheckBoxState(CheckBox tempCheckBox, boolean tempHost,
            boolean tempBranch) {
        if (tempCheckBox == null) {
            Log.e(TAG, "Do NOT give checkbox null");
            return;
        }

        if (tempHost) {
            tempCheckBox.setEnabled(tempHost);
            tempCheckBox.setChecked(tempBranch);
        } else {
            tempCheckBox.setEnabled(tempHost);
        }
    }

    @Override
    protected void onResume() {
        if (SlogAction.isCacheInvalid()) {
            syncState();
        }
        super.onResume();
    }
    @Override
    protected void onPause() {
        super.onPause();
    }
    public void updateSlogConfig() {
        mHandler.removeCallbacks(mCommitConfigRunnable);
        mHandler.postDelayed(mCommitConfigRunnable, 1000);
    }
}
