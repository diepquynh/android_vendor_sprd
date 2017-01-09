
package com.sprd.engineermode.telephony;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.OutputStream;
import java.io.InputStream;
import java.io.PrintWriter;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.util.Log;

public class ThermalInterface {
    private static final String TAG = "ThermalInterface";
    private static final String SOCKET_NAME = "thermald";
    private LocalSocket client;
    private LocalSocketAddress address;

    public ThermalInterface() {
    };

    private boolean clientConnect() {
        int connetTime = 1;
        client = new LocalSocket();
        address = new LocalSocketAddress(SOCKET_NAME, LocalSocketAddress.Namespace.ABSTRACT);
        while (connetTime <= 10) {
            try {
                Log.d(TAG, "SocketClient Try to connect socket;ConnectTime:" + connetTime);
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

    public void thermalEnabled(boolean en) {
        if (!clientConnect()) {
            return;
        }
        try {
            Log.d(TAG, "thermalEnabled: " + en);
            OutputStream out = client.getOutputStream();
            String msg = new String();
            if (en) {
                msg = "SetThmEn";
            } else {
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

    public boolean isThermalEnabled() {
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
            /** BEGIN BUG:589610  Ansel.li 2016/08/16 EngineerMode crash **/
            Log.e(TAG, "the len is..."+len);
            if (len > 0){
                String res = new String(buf, 0, len);
                en = res.equals("true");
            }else{
                en = false;
            }
            /** END BUG:589610  Ansel.li 2016/08/16 EngineerMode crash **/
            out.close();
            in.close();
            client.close();
        } catch (IOException e) {
            Log.e(TAG, "send/receive message error.");
        }
        Log.d(TAG, "isThermalEnabled: " + en);
        return en;
    }
    public void thermalPaEnabled(boolean en) {
        if (!clientConnect()) {
            return;
        }
        try {
            Log.d(TAG, "thermalPaEnabled: " + en);
            OutputStream out = client.getOutputStream();
            String msg = new String();
            if (en) {
                msg = "SetPaEn";
            } else {
                msg = "SetPaDis";
            }
            out.write(msg.getBytes());
            out.flush();
            out.close();
            client.close();
        } catch (IOException e) {
            Log.e(TAG, "send Pa_sensor message error.");
        }
        return;
    }
    public boolean isThermalPaEnabled() {
        boolean en = true;
        if (!clientConnect()) {
            return en;
        }
        try {
            OutputStream out = client.getOutputStream();
            String msg = new String("GetPaEn");
            out.write(msg.getBytes());
            out.flush();
            Log.e(TAG, "GetPaEn ...");
            InputStream in = client.getInputStream();
            byte[] buf = new byte[128];
            int len = in.read(buf, 0, 128);
            if(len>0){
                String res = new String(buf, 0, len);
                en = res.equals("true");
            }else{
                en=false;
            }
            out.close();
            in.close();
            client.close();
        } catch (IOException e) {
            Log.e(TAG, "send/receive Pa_sensor message error.");
        }
        Log.d(TAG, "isThermalPaEnabled: " + en);
        return en;
    }
}
