package com.sprd.validationtools.itemstest;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.R;

import android.app.ActionBar.LayoutParams;
import android.os.Bundle;
import android.os.Environment;
import android.os.EnvironmentEx;
import android.os.Handler;
import android.os.SystemProperties;
import android.util.Log;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

public class SDCardTest extends BaseActivity {
    private String TAG = "SDCardTest";
    private static final String SPRD_SD_TESTFILE = "sprdtest.txt";
    private static final String PHONE_STORAGE_PATH = "/data/data/com.sprd.validationtools/";
    TextView mContent, mContent2;
    byte[] mounted = new byte[2];
    byte[] result = new byte[2];

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        LinearLayout ll = new LinearLayout(this);
        LayoutParams parms = new LayoutParams(LayoutParams.WRAP_CONTENT,
                LayoutParams.WRAP_CONTENT);
        ll.setLayoutParams(parms);
        ll.setOrientation(1);
        ll.setGravity(Gravity.CENTER);
        mContent = new TextView(this);
        mContent.setTextSize(35);
        ll.addView(mContent);

        mContent2 = new TextView(this);
        mContent2.setTextSize(35);
        ll.addView(mContent2);
        setContentView(ll);

        setTitle(R.string.sdcard_test);
            mContent.setText(getResources().getText(R.string.sdcard2_test));
            mContent2.setText(getResources().getText(R.string.sdcard1_test));

        super.removeButton();
    }

    public byte mSDCardTestFlag[] = new byte[1];
    public Handler mHandler = new Handler();
    public Runnable mRunnable = new Runnable() {
        public void run() {
            Log.i("SDCardTest", "=== display SDCard test info! === mounted[0] = " + mounted[0]
                    + " result[0] = " + result[0] + " mounted[1] = " + mounted[1]
                            + " result[1] = " + result[1]);
            if ((mounted[0] == 0) && (result[0] == 0)) {
                  mContent.setText(getResources().getText(
                                R.string.sdcard2_test_result_success));
            } else {
                  mContent.setText(getResources().getText(
                                     R.string.sdcard2_test_result_fail));
            }
            if ((mounted[1] == 0) && (result[1] == 0)) {
                   mContent2.setText(getResources().getText(
                                R.string.sdcard_test_result_success));
            } else {
                   mContent2.setText(getResources().getText(
                                   R.string.sdcard_test_result_fail));
            }

            if (result[0] == 0 && result[1] == 0 && mounted[0] == 0 && mounted[1] == 0) {
                mHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(SDCardTest.this, R.string.text_pass, Toast.LENGTH_SHORT).show();
                        storeRusult(true);
                        finish();
                    }
                }, 1000);
            }else {
                mHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(SDCardTest.this, R.string.text_fail, Toast.LENGTH_SHORT).show();
                        storeRusult(false);
                        finish();
                    }
                }, 1000);
            }
        }
    };

    @Override
    protected void onResume() {
        super.onResume();
        checkSDCard();
        if (mounted[0] == 1) {
                mContent.setText(getResources().getText(R.string.no_sdcard2));
        }
        if (mounted[1] == 1) {
                mContent2.setText(getResources().getText(R.string.no_sdcard));
        }
        // create thread to execute SDCard test command
        Log.i("SDCardTest",
                "=== create thread to execute SDCard test command! ===");
        Thread vtThread = new Thread() {
            public void run() {
                FileInputStream in = null;
                FileOutputStream out = null;
                try {
                    if (mounted[0] == 0) {
                        File fp = new File(EnvironmentEx.getExternalStoragePath(), SPRD_SD_TESTFILE);
                        if (fp.exists())
                            fp.delete();
                        fp.createNewFile();
                        out = new FileOutputStream(fp);
                        mSDCardTestFlag[0] = '6';
                        out.write(mSDCardTestFlag, 0, 1);
                        out.close();
                        in = new FileInputStream(fp);
                        in.read(mSDCardTestFlag, 0, 1);
                        in.close();
                        if (mSDCardTestFlag[0] == '6') {
                            result[0] = 0;
                        } else {
                            result[0] = 1;
                        }   
                    }
                    if (mounted[1] == 0) {
                        File fp = new File(PHONE_STORAGE_PATH, SPRD_SD_TESTFILE);
                        if (fp.exists())
                            fp.delete();
                        fp.createNewFile();
                        out = new FileOutputStream(fp);
                        mSDCardTestFlag[0] = 'd';
                        out.write(mSDCardTestFlag, 0, 1);
                        out.close();
                            in = new FileInputStream(fp);
                            in.read(mSDCardTestFlag, 0, 1);
                            in.close();
                            if (mSDCardTestFlag[0] == 'd'){
                                result[1] = 0;
                            }else{
                                result[1] = 1;
                            }
                    }
                    mHandler.post(mRunnable);
                } catch (Exception e) {
                    Log.i("SDCardTest", "=== error: Exception happens when sdcard I/O! ===");
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
                        Log.e("CTPTest", "close in/out err");
                        io.printStackTrace();
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
                        Log.e("CTPTest", "close in/out err");
                        io.printStackTrace();
                    }
                }
            }
        };
        vtThread.start();
    }

    private void checkSDCard() {
        if (!EnvironmentEx.getExternalStoragePathState().equals(
                Environment.MEDIA_MOUNTED)) {
            mounted[0] = 1;
        } else {
            mounted[0] = 0;
        }
        // if (!Environment.getInternalStoragePathState().equals(
        // Environment.MEDIA_MOUNTED)) {
        // mounted[1] = 1;
        // } else {
        mounted[1] = 0;
        // }
    }

}
