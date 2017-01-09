package com.sprd.systemupdate;

import java.io.File;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.NotificationManager;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.SystemProperties;
import android.os.IBinder;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;

public class DownloadingActivity extends PreferenceActivity implements
        DownloadingService.Callback {
    private ProgressPreference mProgressPref;
    private Preference mVersionPref;
    private Preference mOriginalVersionPref;
    private Preference mDatePref;
    private Preference mSizePref;
    private Preference mReleaseNotePref;
    private Button mTwoState;
    private Context mContext;
    private Storage mStorage;
    private DownloadingService.DownloadingBinder mBinder;

    private static final String TAG = "DownloadingActivity";
    public static final int DOWNLOAD_FAILED = 100;
    public static final int IS_CANCEL_DOWNLOAD = 101;
    public static final int DOWNLOAD_SUCCESS = 102;
    public static final int UPDATE_FILE_BEING_DELETED = 103;

    private ServiceConnection conn = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            if (Utils.DEBUG) {
                Log.i(TAG, "onServiceConnected");
            }
            
            mBinder = (DownloadingService.DownloadingBinder) service;
            if (mBinder != null) {
                mBinder.register(DownloadingActivity.this);
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            mBinder = null;
        }

    };

    @SuppressWarnings("deprecation")
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.list_item_two_button);
        addPreferencesFromResource(R.xml.downloading);
        getListView().setItemsCanFocus(true);

        mContext = this;
        mStorage = Storage.get(mContext);
        if (Utils.DEBUG) {
            Log.i(TAG, "DownloadingActivity onCreate() ");
            Log.i(TAG, "mStorage State:" + mStorage.getState());
        }
        
        if (mStorage.getState() != Storage.State.NIL_2_DOWNLOADING
                && mStorage.getState() != Storage.State.PAUSE_2_DOWNLOADING
                && mStorage.getState() != Storage.State.DOWNLOADED
                && mStorage.getState() != Storage.State.PAUSE_2_PAUSE) {
            if (mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE
                    && getIntent().getBooleanExtra("fromMainActivity", false) != true) {

            } else {
                Intent mIntent = new Intent();
                mIntent.setAction("sprd.systemupdate.action.DOWNLOADING");
                mIntent.setPackage(getPackageName());
                startService(mIntent);
            }
        }

        mProgressPref = (ProgressPreference) findPreference("download_progress");
        mVersionPref = findPreference("lastest_update_version");
        mOriginalVersionPref = findPreference("original_version");
        mDatePref = findPreference("lastest_update_date");
        mSizePref = findPreference("lastest_update_size");
        mReleaseNotePref = findPreference("lastest_update_release_note");

        mTwoState = (Button) findViewById(R.id.two_state);
        mTwoState.setText(R.string.pause);

        Button cancel = (Button) findViewById(R.id.cancel);
        cancel.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {

                if (mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE) {
                    mStorage.setState(Storage.State.PAUSE_2_PAUSE);
                }

                setTwoStateButtonResumed();

                if (mBinder != null) {
                    mBinder.cancel();
                }

                DownloadingActivity.this.showDialog(IS_CANCEL_DOWNLOAD);

            }
        });

        if ((mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE || mStorage
                .getState() == Storage.State.PAUSE_2_PAUSE)) {
            if (getIntent().getBooleanExtra("fromMainActivity", false) != true) {
                mTwoState.setText(R.string.resume);
            }
        }

        mTwoState.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {

                mTwoState.setEnabled(false);

                if (mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE
                        || mStorage.getState() == Storage.State.PAUSE_2_PAUSE) {
                    Log.i(TAG, "TwoState.PAUSE-->DOWNLOADING");
                    mStorage.setState(Storage.State.DOWNLOADING_2_PAUSE);
                    mTwoState.setText(R.string.pause);
                    Intent mIntent = new Intent();
                    mIntent.setAction("sprd.systemupdate.action.DOWNLOADING");
                    mIntent.setPackage(getPackageName());
                    startService(mIntent);
                } else if (mStorage.getState() == Storage.State.PAUSE_2_DOWNLOADING
                        || mStorage.getState() == Storage.State.NIL_2_DOWNLOADING) {
                    mTwoState.setText(R.string.resume);
                    if (mBinder != null) {
                        mBinder.cancel();
                    }
                }
            }
        });

        bindData();
    }

    public void setTwoStateButtonEnabled() {
        if (mStorage.getState() == Storage.State.PAUSE_2_DOWNLOADING) {
            Toast.makeText(mContext, R.string.download_recovery,
                    Toast.LENGTH_SHORT).show();
        } else if (mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE) {
            Toast.makeText(mContext, R.string.download_paused,
                    Toast.LENGTH_SHORT).show();
        }

        mTwoState.setEnabled(true);
        updateProgress(mStorage.getSize());
    }

    public void setTwoStateButtonResumed() {
        mTwoState.setEnabled(true);
        mTwoState.setText(R.string.resume);
    }

    public void setTwoStateButtonPaused() {
        mTwoState.setEnabled(true);
        mTwoState.setText(R.string.pause);
    }

    private void bindData() {
        VersionInfo info = mStorage.getLatestVersion();
        if (info == null) {
            finish();
            return;
        }
        mVersionPref.setSummary(info.mVersion);
        mDatePref.setSummary(info.mDate);
        mOriginalVersionPref.setSummary(SystemProperties.get("ro.build.description"));
        if (info.mSize < 1024) {
            mSizePref.setSummary(Integer.toString(info.mSize) + " bytes");
        } else if (info.mSize < 1024 * 1024) {
            double delta_KB = info.mSize / 1024.0;
            mSizePref.setSummary(String.format("%1$.1f", delta_KB) + " KB");
        } else if (info.mSize >= 1024 * 1024) {
            double delta_MB = info.mSize / 1048576.0;
            mSizePref.setSummary(String.format("%1$.1f", delta_MB) + " MB");
        }

        mReleaseNotePref.setSummary(info.mReleaseNote);
    }

    @Override
    protected void onRestart() {
        super.onRestart();

        File file = new File(mStorage.getStorageFilePath());
        if ((int) file.length() == mStorage.getLatestVersion().mSize) {
            startActivity(new Intent(DownloadingActivity.this,
                    UpgradeActivity.class));
            finish();
        } else if (mStorage.getSize() == 0
                && (mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE || mStorage
                        .getState() == Storage.State.PAUSE_2_PAUSE)) {
            startActivity(new Intent(DownloadingActivity.this,
                    SystemUpdateActivity.class));
            finish();
        }
    }

    @Override
    protected void onResume() {
        if (getIntent().getIntExtra("from_where", 0) == Storage.fromWhere.NOTIFI_OLD) {
            getIntent().putExtra("from_where", Storage.fromWhere.NIL);
            Toast toast = Toast.makeText(mContext, R.string.downloading_the_push_version, Toast.LENGTH_LONG);
            toast.setGravity(Gravity.CENTER, 0, 0);
            toast.show();
        }
        Intent mIntent = new Intent();
        mIntent.setAction("sprd.systemupdate.action.DOWNLOADING");
        mIntent.setPackage("com.sprd.systemupdate");
        bindService(mIntent, conn,Service.BIND_AUTO_CREATE);
        mProgressPref.setProgress(mStorage.getSize());

        super.onResume();
    }

    @Override
    protected void onPause() {
        if (mBinder != null) {
            mBinder.unregister();
        }
        unbindService(conn);
        super.onPause();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    public void updateProgress(int progress) {
        mProgressPref.setProgress(progress);
    }

    @SuppressWarnings("deprecation")
    public void endDownload(boolean succ) {
        if (succ) {
            mStorage.setSize(0);

            mStorage.setState(Storage.State.NIL);
            startActivity(new Intent(DownloadingActivity.this,
                    UpgradeActivity.class));
            finish();

        } else {
            setTwoStateButtonResumed();
            showDialog(DOWNLOAD_FAILED);
        }

    }

    public Dialog onCreateDialog(int id) {
        switch (id) {
        case DOWNLOAD_FAILED:
            downloadFailedDialog();
            break;
        case IS_CANCEL_DOWNLOAD:
            cancelDownloadDialog();
            break;
        case DOWNLOAD_SUCCESS:
            downloadSuccessDialog();
            break;
        case UPDATE_FILE_BEING_DELETED:
            updateFileDeletedDialog();
            break;
        default:
            break;
        }

        return null;
    }

    private void downloadFailedDialog() {
        Dialog dialog = new AlertDialog.Builder(this)
                .setMessage(R.string.download_failed)
                .setPositiveButton(R.string.reload,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                mProgressPref.setProgress(mStorage.getSize());
                                dialog.dismiss();
                                Intent mIntent = new Intent();
                                mIntent.setAction("sprd.systemupdate.action.DOWNLOADING");
                                mIntent.setPackage(getPackageName());
                                startService(mIntent);
                                setTwoStateButtonPaused();
                            }
                        })
                .setNegativeButton(R.string.cancel,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                dialog.dismiss();
                                startActivity(new Intent(
                                        DownloadingActivity.this,
                                        SystemUpdateActivity.class));
                                finish();
                            }
                        }).create();
        dialog.setCancelable(true);
        dialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            public void onCancel(DialogInterface dialog) {
                startActivity(new Intent(DownloadingActivity.this,
                        SystemUpdateActivity.class));
                finish();
            }
        });
        dialog.setCanceledOnTouchOutside(false);
        dialog.show();
    }

    private void cancelDownloadDialog() {
        Dialog dialog = new AlertDialog.Builder(this)
                .setMessage(R.string.is_cancel_download)
                .setPositiveButton(R.string.ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                mStorage.setSize(0);
                                dialog.dismiss();
                                final NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
                                notificationManager.cancelAll();
                                mStorage.setState(Storage.State.NIL);
                                new File(mStorage.getStorageFilePath())
                                        .delete();
                                Toast.makeText(mContext,
                                        R.string.download_canceled,
                                        Toast.LENGTH_SHORT).show();
                                startActivity(new Intent(
                                        DownloadingActivity.this,
                                        SystemUpdateActivity.class));
                                finish();
                            }
                        })
                .setNegativeButton(R.string.cancel,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                dialog.dismiss();
                                if (mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE) {
                                    setTwoStateButtonPaused();
                                    mStorage.setState(Storage.State.PAUSE_2_DOWNLOADING);
                                    Intent mIntent = new Intent();
                                    mIntent.setAction("sprd.systemupdate.action.DOWNLOADING");
                                    mIntent.setPackage(getPackageName());
                                    startService(mIntent);
                                } else if (mStorage.getState() == Storage.State.PAUSE_2_PAUSE) {
                                    setTwoStateButtonResumed();
                                    mStorage.setState(Storage.State.DOWNLOADING_2_PAUSE);
                                }

                            }
                        }).create();
        dialog.setCancelable(true);
        dialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            public void onCancel(DialogInterface dialog) {
                dialog.dismiss();

                if (mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE) {
                    mStorage.setState(Storage.State.PAUSE_2_DOWNLOADING);
                    Intent mIntent = new Intent();
                    mIntent.setAction("sprd.systemupdate.action.DOWNLOADING");
                    mIntent.setPackage(getPackageName());
                    startService(mIntent);
                } else if (mStorage.getState() == Storage.State.PAUSE_2_PAUSE) {
                    mStorage.setState(Storage.State.DOWNLOADING_2_PAUSE);
                }
            }
        });
        dialog.setCanceledOnTouchOutside(false);
        dialog.show();
    }

    private void downloadSuccessDialog() {
        Dialog dialog = new AlertDialog.Builder(this)
                .setMessage(R.string.is_upgrade)
                .setPositiveButton(R.string.ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                dialog.dismiss();
                                mStorage.setState(Storage.State.NIL);
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

        dialog.setCancelable(true);
        dialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            public void onCancel(DialogInterface dialog) {
                dialog.dismiss();

                startActivity(new Intent(DownloadingActivity.this,
                        SystemUpdateActivity.class));
                finish();
            }
        });
        dialog.setCanceledOnTouchOutside(false);
        dialog.show();
    }

    @SuppressWarnings("deprecation")
    public void showUpdateFileDeletedDialog() {
        mStorage.setSize(0);
        final NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancelAll();
        mStorage.setState(Storage.State.NIL);
        new File(mStorage.getStorageFilePath()).delete();

        showDialog(UPDATE_FILE_BEING_DELETED);
    }

    public void updateFileDeletedDialog() {
        Dialog dialog = new AlertDialog.Builder(this)
                .setMessage(R.string.update_file_being_deleted)
                .setPositiveButton(R.string.ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                dialog.dismiss();
                                if (mStorage.checkSdCardState()) {
                                    Intent mIntent = new Intent();
                                    mIntent.setAction("sprd.systemupdate.action.DOWNLOADING");
                                    mIntent.setPackage(getPackageName());
                                    startService(mIntent);
                                } else {
                                    mStorage.setState(Storage.State.NIL);
                                    startActivity(new Intent(
                                            DownloadingActivity.this,
                                            SystemUpdateActivity.class));
                                    finish();
                                }
                            }
                        })
                .setNegativeButton(R.string.cancel,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                dialog.dismiss();
                                mStorage.setState(Storage.State.NIL);
                                startActivity(new Intent(
                                        DownloadingActivity.this,
                                        SystemUpdateActivity.class));
                                finish();
                            }
                        }).create();

        dialog.setCancelable(true);
        dialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            public void onCancel(DialogInterface dialog) {
                dialog.dismiss();
            }
        });
        dialog.setCanceledOnTouchOutside(false);
        dialog.show();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {
            if (mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE) {
                mStorage.setState(Storage.State.PAUSE_2_PAUSE);
            }
            startActivity(new Intent(DownloadingActivity.this,
                    SystemUpdateActivity.class));
            finish();
        } else if (keyCode == KeyEvent.KEYCODE_HOME
                && event.getRepeatCount() == 0) {
            if (mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE) {
                mStorage.setState(Storage.State.PAUSE_2_PAUSE);
            }
            finish();
        }

        return super.onKeyDown(keyCode, event);
    }
    
    public void onNewIntent(Intent intent) {
        super.onNewIntent(intent);       
        setIntent(intent);       
    }
    

}
