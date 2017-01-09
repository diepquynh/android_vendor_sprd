package com.android.server.power;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.OutputStream;
import java.io.InputStream;
import java.io.PrintWriter;

import android.content.ContentResolver;
import android.content.Context;
import android.database.ContentObserver;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.SystemClock;
import android.os.UserHandle;
import android.provider.Settings;
import android.util.Log;

public class PowerManagerServiceUtils extends AbsPowerManagerServiceUtils{

    private static final String TAG = "PowerManagerServiceUtils";
    private static PowerManagerServiceUtils sInstance;

    private Context mContext;
    private boolean mBootCompleted;

    private static final String SOCKET_NAME = "thermald";
    private LocalSocket client;
    private LocalSocketAddress address;

    // SPRD: Default mButtonOffTimeoutSetting 0.
    private int mButtonOffTimeoutSetting = 0;
    // SPRD: Default and minimum button light off timeout in milliseconds.
    private static final int DEFAULT_BUTTON_OFF_TIMEOUT = 1500;

    public static PowerManagerServiceUtils getInstance(Context context){

        synchronized(PowerManagerServiceUtils.class){
            if (sInstance == null ){
                sInstance = new PowerManagerServiceUtils(context);
            }
        }

        return sInstance;
    }

    private PowerManagerServiceUtils(Context context){
        mContext = context;
    }


    public void registerButtonLightOffTimeOut(ContentResolver resolver, ContentObserver settingsObserver){
        // SPRD: Register for settings changes update BUTTON_LIGHT_OFF_TIMEOUT.
        resolver.registerContentObserver(Settings.System.getUriFor(
                        Settings.System.BUTTON_LIGHT_OFF_TIMEOUT),
                false, settingsObserver, UserHandle.USER_ALL);
    }

    public void updateButtonLightOffTimeOut(ContentResolver resolver){
        final int buttonOffTimeoutSetting = Settings.System.getInt(resolver,
                Settings.System.BUTTON_LIGHT_OFF_TIMEOUT, DEFAULT_BUTTON_OFF_TIMEOUT);
        Log.d(TAG, "buttonOffTimeoutSetting = " + buttonOffTimeoutSetting +
                " mButtonOffTimeoutSetting = " + mButtonOffTimeoutSetting);
        if (buttonOffTimeoutSetting != mButtonOffTimeoutSetting) {
            mButtonOffTimeoutSetting = buttonOffTimeoutSetting;
            PowerManagerServiceEx.getInstance(mContext).scheduleButtonLightTimeout(SystemClock.uptimeMillis());
        }
    }

    @Override
    public int getButtonOffTimeoutSetting() {
        return mButtonOffTimeoutSetting;
    }

    @Override
    public void setBootCompleted(boolean bootCompleted) {
        PowerManagerServiceEx.getInstance(mContext).setBootCompleted(bootCompleted);
    }

    private boolean clientConnect(){
        int connetTime = 1;
        client = new LocalSocket();
        address = new LocalSocketAddress(SOCKET_NAME, LocalSocketAddress.Namespace.ABSTRACT);
        while (connetTime <= 10) {
            try {
                Log.d(TAG, "SocketClient Try to connect socket;ConnectTime:"+connetTime);
                client.connect(address);
                return true;
            } catch (Exception e) {
                e.printStackTrace();
                connetTime++;
                Log.e(TAG, "SocketClient Connect fail");
            }
        }
        return false;
    }

    public void thermalEnabled(boolean en){
        if (!clientConnect()) {
            return;
        }
        try {
            Log.d(TAG, "thermalEnabled: " + en);
            OutputStream out = client.getOutputStream();
            String msg = new String();
            if (en){
                msg = "SetThmEn";
            }else{
                msg = "SetThmDis";
            }
            out.write(msg.getBytes());
            out.flush();

            out.close();
            client.close();
        } catch (IOException e) {
            Log.e(TAG, "send message error.");
        }
        return;
    }

    public boolean isThermalEnabled(){
        boolean en = true;
        if (!clientConnect()) {
            return en;
        }
        try {
            OutputStream out = client.getOutputStream();
            String msg = new String("GetThmEn");
            out.write(msg.getBytes());
            out.flush();

            Log.e(TAG, "GetThmEn ...");
            InputStream in = client.getInputStream();
            byte[] buf = new byte[128];
            int len = in.read(buf, 0, 128);
            String res = new String(buf, 0, len);
            en = res.equals("true");

            out.close();
            in.close();
            client.close();
        } catch (IOException e) {
            Log.e(TAG, "send/receive message error.");
        }

        Log.d(TAG, "isThermalEnabled: " + en);
        return en;
    }

}
