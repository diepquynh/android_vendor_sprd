
package com.sprd.validationtools.itemstest;

import android.graphics.Color;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Parcel;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.util.Log;
import android.view.Gravity;
import android.widget.TextView;
import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.R;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.FileNotFoundException;

import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.nio.charset.Charset;

public class RedLightTest extends BaseActivity {
    private static final String TAG = "RedLightTest";
    private static String redLight_high_time_path = "/sys/class/leds/red_bl/high_time";
    private static String redLight_low_time_path = "/sys/class/leds/red_bl/low_time";
    private static String redLight_rising_time_path = "/sys/class/leds/red_bl/rising_time";
    private static String redLight_falling_time_path = "/sys/class/leds/red_bl/falling_time";
    private static String redLight_on_off_path = "/sys/class/leds/red_bl/on_off";
    private TextView mContent;
    private IBinder mBinder;
    private static boolean DEBUG = false;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContent = new TextView(this);
        setBackground();
        setContentView(mContent);
        mContent.setGravity(Gravity.CENTER);
        mContent.setTextSize(35);
        mBinder = ServiceManager.getService("phasechecknative");
        if (mBinder != null)
            Log.e(TAG, "Get The service connect!");
        else
            Log.e(TAG, "connect Error!!");
        writeLedlightSwitch(1);
    }

    private void setBackground() {
        mContent.setBackgroundColor(Color.RED);
        mContent.setText(getString(R.string.status_indicator_red));
        if (DEBUG) {
            startTest();
        }
    }

    @Override
    protected void onDestroy() {
        writeLedlightSwitch(0);
        super.onDestroy();
    }

    public boolean writeLedlightSwitch(int value) {
        try {
            Parcel data = Parcel.obtain();
            Parcel reply = Parcel.obtain();
            data.writeInt(value);
            mBinder.transact(7, data, reply, 0);
            Log.e(TAG, "writeChargeSwitch red light data = " + reply.readString() + " SUCESS!!");
            data.recycle();
            return true;
        } catch (Exception ex) {
            Log.e(TAG, "Exception ", ex);
            return false;
        }
    }

    private void startTest() {
        new Thread() {
            @Override
            public void run() {
                int data1 = getIntFromFile(redLight_high_time_path);
                int data2 = getIntFromFile(redLight_low_time_path);
                int data3 = getIntFromFile(redLight_rising_time_path);
                int data4 = getIntFromFile(redLight_falling_time_path);
                int data5 = getIntFromFile(redLight_on_off_path);
                Log.d(TAG, "data1:" + data1 + ",data2:" + data2 + ",data3:" + data3 + ",data4:"
                        + data4 + ",data5:" + data5);
            }
        }.start();
    }

    private static int getIntFromFile(String filename) {
        File file = new File(filename);
        InputStream fIn = null;
        try {
            fIn = new FileInputStream(file);
            InputStreamReader isr = new InputStreamReader(fIn,Charset.defaultCharset());
            char[] inputBuffer = new char[1024];
            int q = -1;

            q = isr.read(inputBuffer);
            isr.close();
            fIn.close();

            if (q > 0)
                return Integer.parseInt(String.valueOf(inputBuffer, 0, q).trim());
        } catch (Exception e) {
           // TODO Auto-generated catch block
            e.printStackTrace();
            return -1;
        }
        return -1;
    }
}
