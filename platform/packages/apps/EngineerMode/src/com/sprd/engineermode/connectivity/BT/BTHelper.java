package com.sprd.engineermode.connectivity.BT;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Message;
import android.os.SystemProperties;
import android.util.Log;
import android.widget.Toast;

public class BTHelper {

    private static final String TAG = "BTHelper";
    private static final String SOCKET_NAME = "wcnd_eng";
    private static final String CMD_SUCCESS = "OK";
    private static final String CMD_FAIL = "Fail";

    //eng bt cmd
    private static final String CMD_BT_ON = "eng bt bt_on";
    private static final String CMD_BT_OFF = "eng bt bt_off";
    private static final String CMD_BT_TX = "eng bt set_nosig_tx_testmode ";
    private static final String CMD_BT_RX_START = "eng bt set_nosig_rx_testmode ";
    private static final String CMD_BT_RX_READ = "eng bt set_nosig_rx_recv_data";
    private static final String CMD_BT_BLE_TX = "eng bt set_nosig_tx_testmode ";
    private static final String CMD_BT_BLE_RX_START = "eng bt set_nosig_rx_testmode ";
    private static final String CMD_BT_BLE_RX_READ = "eng bt set_nosig_rx_recv_data_le";

    private static LocalSocket mSocketClient;
    private static LocalSocketAddress mSocketAddress;
    private static OutputStream mOutputStream;
    private static InputStream mInputStream;
    private static String mCmd;
    public static boolean isBTOn = false;

    static class BTTX {
        public String pattern;
        public String channel;
        public String pactype;
        public String paclen;
        public String powertype;
        public String powervalue;
        public String paccnt;
    }

    static class BTRX {
        public String channel;
        public String pactype;
        public String gain;
        public String addr;
    }

    public static boolean BTTxStart(BTTX tx) {
        boolean isSuccess = false;
        if (connectSocket(SOCKET_NAME)) {
            String bt_on = sendCmd(CMD_BT_ON);
            if (bt_on != null && bt_on.contains("bt_status=1")) {
                isBTOn = true;
                mCmd = CMD_BT_TX + "1 0 " + tx.pattern + " " + tx.channel + " " + tx.pactype + " "
                        + tx.paclen + " " + tx.powertype + " " + tx.powervalue + " " + tx.paccnt;
                Log.d(TAG, "BTTxStart cmd is " + mCmd);
                String bt_tx = sendCmd(mCmd);
                if (bt_tx != null && bt_tx.contains("ok")) {
                    isSuccess = true;
                }
            }
        }
        return isSuccess;
    }

    public static boolean BTBLETxStart(BTTX tx) {
        boolean isSuccess = false;
        if (connectSocket(SOCKET_NAME)) {
            String bt_on = sendCmd(CMD_BT_ON);
            if (bt_on != null && bt_on.contains("bt_status=1")) {
                isBTOn = true;
                mCmd = CMD_BT_BLE_TX + "1 1 " + tx.pattern + " " + tx.channel + " " + tx.pactype
                        + " " + tx.paclen + " " + tx.powertype + " " + tx.powervalue + " "
                        + tx.paccnt;
                Log.d(TAG, "BTBLETxStart cmd is " + mCmd);
                String bt_ble_tx = sendCmd(mCmd);
                if (bt_ble_tx != null && bt_ble_tx.contains("ok")) {
                    isSuccess = true;
                }
            }
        }
        return isSuccess;
    }

    public static boolean BTTxStop(BTTX tx) {
        mCmd = CMD_BT_TX + "0 0 " + tx.pattern + " " + tx.channel + " " + tx.pactype + " "
                + tx.paclen + " " + tx.powertype + " " + tx.powervalue + " " + tx.paccnt;
        Log.d(TAG, "BTTxStop cmd is " + mCmd);
        String tx_stop = sendCmd(mCmd);
        if (tx_stop != null && tx_stop.contains("ok")) {
            return true;
        } else {
            return false;
        }
    }

    public static boolean BTBLETxStop(BTTX tx) {
        mCmd = CMD_BT_TX + "0 1 " + tx.pattern + " " + tx.channel + " " + tx.pactype + " "
                + tx.paclen + " " + tx.powertype + " " + tx.powervalue + " " + tx.paccnt;
        Log.d(TAG, "BTBLETxStop cmd is " + mCmd);
        String tx_stop = sendCmd(mCmd);
        if (tx_stop != null && tx_stop.contains("ok")) {
            return true;
        } else {
            return false;
        }
    }

    public static boolean BTOff() {
        String bt_off = sendCmd(CMD_BT_OFF);
        if (bt_off != null && bt_off.contains("bt_status=0")) {
            isBTOn = false;
            return true;
        } else {
            return false;
        }
    }

    public static boolean BTRxStart(BTRX rx) {
        boolean isSuccess = false;
        if (connectSocket(SOCKET_NAME)) {
            String bt_on = sendCmd(CMD_BT_ON);
            if (bt_on != null && bt_on.contains("bt_status=1")) {
                isBTOn = true;
                mCmd = CMD_BT_RX_START + "1 0 7 " + rx.channel + " " + rx.pactype + " " + rx.gain
                        + " " + rx.addr;
                Log.d(TAG, "BTRxStart cmd is " + mCmd);
                String bt_rx = sendCmd(mCmd);
                if (bt_rx != null && bt_rx.contains("ok")) {
                    isSuccess = true;
                }
            }
        }
        return isSuccess;
    }

    public static boolean BTBLERxStart(BTRX rx) {
        boolean isSuccess = false;
        if (connectSocket(SOCKET_NAME)) {
            String bt_on = sendCmd(CMD_BT_ON);
            if (bt_on != null && bt_on.contains("bt_status=1")) {
                isBTOn = true;
                mCmd = CMD_BT_BLE_RX_START + "1 1 7 " + rx.channel + " " + rx.pactype + " "
                        + rx.gain + " " + rx.addr;
                Log.d(TAG, "BTBLERxStart cmd is " + mCmd);
                String bt_ble_rx = sendCmd(mCmd);
                if (bt_ble_rx != null && bt_ble_rx.contains("ok")) {
                    isSuccess = true;
                }
            }
        }
        return isSuccess;
    }

    public static String BTRxRead() {
        return sendCmd(CMD_BT_RX_READ);
        //return "OK rssi:9, pkt_cnt:3, pkt_err_cnt:3, bit_cnt:4672, bit_err_cnt:2351";
    }

    public static String BTBLERxRead() {
        return sendCmd(CMD_BT_BLE_RX_READ);
    }

    public static boolean BTRxStop(BTRX rx) {
        mCmd = CMD_BT_RX_START + "0 0 7 " + rx.channel + " " + rx.pactype + " " + rx.gain + " "
                + rx.addr;
        Log.d(TAG, "BTRXStop cmd is " + mCmd);
        String bt_stop = sendCmd(mCmd);
        if (bt_stop != null && bt_stop.contains("ok")) {
            return true;
        } else {
            return false;
        }
    }

    public static boolean BTBLERxStop(BTRX rx) {
        mCmd = CMD_BT_RX_START + "0 1 7 " + rx.channel + " " + rx.pactype + " " + rx.gain + " "
                + rx.addr;
        Log.d(TAG, "BTBLERXStop cmd is " + mCmd);
        String bt_stop = sendCmd(mCmd);
        if (bt_stop != null && bt_stop.contains("ok")) {
            return true;
        } else {
            return false;
        }
    }

    public static boolean connectSocket(String socketName) {
        try {
            if (mSocketClient == null) {
                mSocketClient = new LocalSocket();
                mSocketAddress = new LocalSocketAddress(SOCKET_NAME,
                        LocalSocketAddress.Namespace.ABSTRACT);
                mSocketClient.connect(mSocketAddress);
                Log.d(TAG, "mSocketClient connect is " + mSocketClient.isConnected());
            }

            // mOutputStream = mSocketClient.getOutputStream();
        } catch (IOException e) {
            Log.e(TAG, "connectSocket fail" + e);
            return false;
        }
        return mSocketClient.isConnected();
    }

    public static String sendCmd(String comand) {
        byte[] buf = new byte[255];
        String result = null;
        try {
            if (mSocketClient != null) {
                mOutputStream = mSocketClient.getOutputStream();
                if (mOutputStream != null) {
                    final StringBuilder cmdBuilder = new StringBuilder(comand).append('\0');
                    final String cmd = cmdBuilder.toString();
                    mOutputStream.write(cmd.getBytes(StandardCharsets.UTF_8));
                    mOutputStream.flush();
                }
                mInputStream = mSocketClient.getInputStream();
                int count = mInputStream.read(buf, 0, 255);
                result = new String(buf, "utf-8");
                Log.d(TAG, "send " + comand + ", result is " + result);
            }
        } catch (IOException e) {
            Log.e(TAG, "Failed get output stream: " + e);
        }
        return result;
    }

    public static void closeSocket() {
        try {
            if (mOutputStream != null) {
                mOutputStream.close();
            }
            if (mInputStream != null) {
                mInputStream.close();
            }
            if (mSocketClient.isConnected()) {
                mSocketClient.close();
                mSocketClient = null;
            }
        } catch (Exception e) {
            Log.d(TAG, "catch exception is " + e);
        }
    }
}
