
package com.sprd.validationtools.itemstest;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import android.app.ActionBar.LayoutParams;
import android.content.Context;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.SystemProperties;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.util.Log;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.TextView;
import com.sprd.validationtools.R;

import com.sprd.validationtools.BaseActivity;

public class OTGTest extends BaseActivity {
    private String TAG = "OTGTest";
    private TextView mTextView;
    private StorageManager mStorageManager = null;
    private boolean isUsb = false;
    private String usbMassStoragePath = "/storage/usbdisk";
    private static final String SPRD_OTG_TESTFILE = "otgtest.txt";
    private String otgPath = null;
    public byte mOTGTestFlag[] = new byte[1];
    byte[] result = new byte[1];
    byte[] mounted = new byte[1];

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        LinearLayout OTGLayout = new LinearLayout(this);
        LayoutParams parms = new LayoutParams(LayoutParams.WRAP_CONTENT,
                LayoutParams.WRAP_CONTENT);
        OTGLayout.setLayoutParams(parms);
        OTGLayout.setOrientation(1);
        OTGLayout.setGravity(Gravity.CENTER);
        mTextView = new TextView(this);
        mTextView.setTextSize(35);
        OTGLayout.addView(mTextView);
        setContentView(OTGLayout);
        setTitle(R.string.otg_test);
        mTextView.setText(getResources().getText(R.string.otg_begin_test));
    }

    private void checkOTGdevices() {
        if (mStorageManager == null) {
            mStorageManager = (StorageManager) getSystemService(Context.STORAGE_SERVICE);
        }
        StorageVolume[] storageVolumes = mStorageManager.getVolumeList();
        for (int i = 0; i < storageVolumes.length; i++) {
            StorageVolume storageVolume = storageVolumes[i];

            otgPath = storageVolume.getPath();

            if (usbMassStoragePath.equals(otgPath))
            {
                if (mStorageManager.getVolumeState(otgPath).equals(
                        Environment.MEDIA_MOUNTED)) {
                    mounted[0] = 0;
                    Log.i(TAG, "=== OTG mount succeed ===");
                } else {
                    mounted[0] = 1;
                    Log.i(TAG, "=== OTG mount Fail ===");
                }
            }

        }

    }

    public Handler mHandler = new Handler();
    public Runnable mRunnable = new Runnable() {
        public void run() {
            Log.i(TAG, "=== display OTG test succeed info! ===");
            if (mounted[0] == 0) {
                if (result[0] == 0) {
                    mTextView.setText(getResources().getText(R.string.otg_test_success));
                } else {
                    mTextView.setText(getResources().getText(R.string.otg_test_fail));
                }
            } else {
                mTextView.setText(getResources().getText(R.string.otg_test_fail));
            }
            showResultDialog(getString(R.string.otg_test));
        }
    };
    public Runnable mCheckRunnable = new Runnable() {
        public void run() {
            Log.i(TAG, "=== checkOTGdevices! ===");
            checkOTGdevices();
            if (mounted[0] != 0) {
                mTextView.setText(getResources().getText(R.string.otg_no_devices));
                mHandler.postDelayed(mCheckRunnable, 1000);
            } else {
                startVtThread();
            }
        }
    };

    @Override
    protected void onResume() {
        super.onResume();
        mTextView.setText(getResources().getText(R.string.otg_begin_test));
        mounted[0] = 1;
        result[0] = 1;
        checkOTGdevices();
        if (mounted[0] != 0) {
            mTextView.setText(getResources().getText(R.string.otg_no_devices));
            mHandler.postDelayed(mCheckRunnable, 1000);
        } else {
            startVtThread();
        }
    }

    @Override
    protected void onPause() {
        mHandler.removeCallbacks(mCheckRunnable);
        super.onPause();
    }

    private void startVtThread() {
        Log.i(TAG,
                "=== create thread to execute OTG test command! ===");
        Thread vtThread = new Thread() {
            public void run() {
                FileInputStream in = null;
                FileOutputStream out = null;
                try {
                    if (mounted[0] == 0) {
                        File fp = new File(usbMassStoragePath, SPRD_OTG_TESTFILE);
                        if (fp.exists())
                            fp.delete();
                        fp.createNewFile();
                        out = new FileOutputStream(fp);
                        mOTGTestFlag[0] = '7';
                        out.write(mOTGTestFlag, 0, 1);
                        out.close();
                        in = new FileInputStream(fp);
                        in.read(mOTGTestFlag, 0, 1);
                        in.close();
                        if (mOTGTestFlag[0] == '7') {
                            result[0] = 0;
                        } else {
                            result[0] = 1;
                        }
                    }
                    mHandler.post(mRunnable);
                } catch (Exception e) {
                    Log.i(TAG, "=== error: Exception happens when OTG I/O! ===");
                    e.printStackTrace();
                    try {
                        if (out != null) {
                            out.close();
                            out = null;
                        }
                        if (in != null) {
                            in.close();
                            in = null;
                        }
                    } catch (IOException io) {
                        Log.e(TAG, "close in/out err");
                    }
                } finally {
                    try {
                        if (out != null) {
                            out.close();
                            out = null;
                        }
                        if (in != null) {
                            in.close();
                            in = null;
                        }
                    } catch (IOException io) {
                        Log.e(TAG, "close in/out err");
                    }
                }
            }
        };
        vtThread.start();
    }
}
