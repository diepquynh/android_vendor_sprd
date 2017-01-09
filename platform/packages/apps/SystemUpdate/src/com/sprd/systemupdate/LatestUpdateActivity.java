package com.sprd.systemupdate;

import java.io.File;

import android.app.AlarmManager;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;
import android.os.SystemProperties;

public class LatestUpdateActivity extends PreferenceActivity {

    private Preference mVersionPref;
    private Preference mOriginalVersionPref;
    private Preference mDatePref;
    private Preference mSizePref;
    private Preference mReleaseNotePref;
    private Storage mStorage;
    private Context mContext;

    @SuppressWarnings("deprecation")
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mContext = this;
        mStorage = Storage.get(this);

        setContentView(R.layout.list_item_one_button);
        addPreferencesFromResource(R.xml.latest_update);
        getListView().setItemsCanFocus(true);

        mVersionPref = findPreference("lastest_update_version");
        mOriginalVersionPref = findPreference("original_version");
        mDatePref = findPreference("lastest_update_date");
        mSizePref = findPreference("lastest_update_size");
        mReleaseNotePref = findPreference("lastest_update_release_note");

        Button download = (Button) findViewById(R.id.download);
        download.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {

                if (getIntent().getIntExtra("from_where", -1) == Storage.fromWhere.NOTIFI_NEW) {
                    if (mStorage.getState() == Storage.State.PAUSE_2_DOWNLOADING
                            || mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE
                            || mStorage.getState() == Storage.State.NIL_2_DOWNLOADING
                            || mStorage.getState() == Storage.State.PAUSE_2_PAUSE) {
                        Toast.makeText(mContext,
                                R.string.please_cancel_current_download,
                                Toast.LENGTH_LONG).show();
                        return;
                    } else {
                        if (mStorage.getTmpLatestVersion() != null) {
                            mStorage.setLatestVersion(mStorage
                                    .getTmpLatestVersionString());
                        } else {
                            Toast.makeText(mContext, R.string.download_failed,
                                    Toast.LENGTH_LONG).show();
                            return;
                        }
                    }
                }
                if (mStorage.checkSdCardState()) {
                    if (mStorage.getState() == Storage.State.DOWNLOADED
                            || mStorage.getState() == Storage.State.DOWNLOADING_2_PAUSE
                            || mStorage.getState() == Storage.State.PAUSE_2_PAUSE
                            || mStorage.getState() == Storage.State.WAIT_UPDATE) {
                        showReDownloadDialog();
                    } else {
                        startDownloadUpdateFile();
                    }
                }

            }

        });
        bindData();
    }

    private void bindData() {
        VersionInfo info = mStorage.getLatestVersion();

        if (getIntent().getIntExtra("from_where", -1) == Storage.fromWhere.NOTIFI_NEW) {
            info = mStorage.getTmpLatestVersion();
        }

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
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {

            Intent intent = new Intent(LatestUpdateActivity.this,
                    SystemUpdateActivity.class);
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
            startActivity(intent);
            finish();
        }
        return super.onKeyDown(keyCode, event);
    }

    public Dialog onCreateDialog(int id) {
        switch (id) {
        case Storage.SDCARD_LACK_SPACE:
            sdCardLackSpaceDialog(id);
            break;
        case Storage.SDCARD_NOT_MOUNTED:
            sdCardUnmountedDialog(id);
            break;
        default:
            break;
        }

        return null;
    }

    public void sdCardUnmountedDialog(int id) {
        Dialog dialog = new AlertDialog.Builder(this)
                .setMessage(R.string.save_in_internal_storage_due_to_no_sdcard)
                .setPositiveButton(R.string.ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                dialog.dismiss();
                                if (mStorage.getStorageState() == Storage.SDCARD_AVALIABLE) {
                                    startDownloadUpdateFile();
                                }
                            }
                        })
                .setNegativeButton(R.string.cancel,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                dialog.dismiss();
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

    public void sdCardLackSpaceDialog(int id) {
        Dialog dialog = new AlertDialog.Builder(this)
                .setMessage(R.string.save_in_internal_storage_due_to_lack_space)
                .setPositiveButton(R.string.ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                dialog.dismiss();
                                if (mStorage.getStorageState() == Storage.SDCARD_AVALIABLE) {
                                    startDownloadUpdateFile();
                                }
                            }
                        })
                .setNegativeButton(R.string.cancel,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                dialog.dismiss();
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

    public void startDownloadUpdateFile() {

        if (mStorage.getState() == Storage.State.WAIT_UPDATE) {
            AlarmManager mAlarmManager = (AlarmManager) mContext
                    .getSystemService(Context.ALARM_SERVICE);
            Intent intent = new Intent("sprd.systemupdate.action.ASK_UPGRADE");
            PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext,
                    0, intent, 0);
            mAlarmManager.cancel(pendingIntent);
        }

        new File(mStorage.getStorageFilePath()).delete();
        File updateFile = new File(mStorage.getStoragePath(),
                Storage.UPDATE_FILE_NAME);
        if (updateFile != null) {
            mStorage.setStorageFilePath(updateFile.getPath().toString());
        }
        mStorage.setState(Storage.State.NIL);
        mStorage.setSize(0);
        startActivity(new Intent(LatestUpdateActivity.this,
                DownloadingActivity.class));
        finish();
    }

    public void showReDownloadDialog() {
        Dialog dialog = new AlertDialog.Builder(this)
                .setMessage(R.string.update_file_will_be_covered)
                .setPositiveButton(R.string.ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                dialog.dismiss();
                                startDownloadUpdateFile();
                            }
                        })
                .setNegativeButton(R.string.cancel,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                dialog.dismiss();
                            }
                        }).create();
        dialog.setCanceledOnTouchOutside(false);
        dialog.show();
    }

    @Override
    public void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        bindData();
    }

}
