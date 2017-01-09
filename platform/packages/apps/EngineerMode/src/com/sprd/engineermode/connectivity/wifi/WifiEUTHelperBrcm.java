
package com.sprd.engineermode.connectivity.wifi;

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
import android.os.Handler;
import com.sprd.engineermode.utils.SocketUtils;

public class WifiEUTHelperBrcm extends WifiEUTHelper {

    private static final String TAG = "WifiEUTHelperBrcm";
    private static final String SOCKET_NAME = "hardwaretest";
    private static final String CMD_SUCCESS = "OK";
    private static final String CMD_FAIL = "Fail";

    // iwnpi wlan0 ifaceup is sended by iwnpi, so equal to ifconfig wlan0 up,
    // iwnpi wlan0 ifacedown is sended by iwnpi, so equal to ifconfig wlan0 down

    private static final String SET_EUT_START = "wl START";
    private static final String SET_EUT_STOP = "wl STOP";
    private static final String SET_EUT_SET_CHANNEL = "wl SET_CHANNEL ";
    private static final String SET_EUT_SET_RATE = "wl SET_RATE ";

    // for tx
    private static final String SET_EUT_TX_START = "wl TX_START";
    private static final String SET_EUT_TX_STOP = "wl TX_STOP";
    private static final String SET_EUT_CW_START = "wl SIN_WAVE";
    private static final String SET_EUT_SET_POWER = "wl TX_POWER ";
    // add by alisa.li
    private static final String SET_EUT_SET_BAND = "wl SET_BAND ";
    private static final String SET_EUT_SET_BANDWITCH = "wl SET_BW ";
    private static final String SET_EUT_SET_PREAMBLE = "wl SET_PREAMBLE ";
    private static final String SET_EUT_SET_MODE = "wl SET_MODE ";

    // for rx
    private static final String SET_EUT_RX_START = "wl RX_START";
    private static final String SET_EUT_RX_STOP = "wl RX_STOP";
    private static final String SET_EUT_GET_RXOK = "wl GET_RXOK";
    private static final String SET_EUT_POWERSAVE_MODE = "wl POWERSAVE_MODE ";

    private static final int WIFI_UP = 0;
    private static final int WIFI_START = 1;
    private static final int WIFI_STOP = 2;
    private static final int WIFI_DOWN = 3;
    private static final int WIFI_SET_BAND = 4;
    private static final int WIFI_TX_LENGTH = 5;
    private static final int WIFI_TX_COUNT = 6;
    private static final int WIFI_TX_POWER = 7;
    private static final int WIFI_TX_PREAMBLE = 8;
    private static final int WIFI_TX_BANDWIDTH = 9;
    private static final int WIFI_SET_CHANNEL = 10;
    private static final int WIFI_SET_RATE = 11;
    private static final int WIFI_SET_MODE = 12;
    private static final int WIFI_TX_SINWAVE = 13;
    private static final int WIFI_TX_GUARDINTERVAL = 14;
    private static final int WIFI_TX_START = 15;
    private static final int WIFI_TX_STOP = 16;
    private static final int WIFI_RX_START = 17;
    private static final int WIFI_RX_STOP = 18;
    private static final int WIFI_RX_OK = 19;
    private static final int WIFI_REG_READ = 20;
    private static final int WIFI_REG_WRITE = 21;
    private static final int WIFI_GET_STATUS = 22;
    private static final int WIFI_SET_STATUS_ON = 23;
    private static final int WIFI_SET_STATUS_OFF = 24;

    private static LocalSocket mSocketClient = null;
    private static LocalSocketAddress mSocketAddress;
    private static OutputStream mOutputStream;
    private static InputStream mInputStream;

    private static LocalSocket mInsmodeSocketClient;
    private static LocalSocketAddress mInsmodeSocketAddress;
    private static OutputStream mInsmodeOutputStream;
    private static InputStream mInsmodeInputStream;

    private Handler mUiThread = new Handler();

    /**
     * creat wcnd socket and send iwnpi wlan0 ifaceup because this cmd is the first cmd, so creat
     * socket here but the socket is not closed here, and close it when wifiDown(), cause the
     * "iwnpi wlan0 ifacedown" cmd is the last cmd sended by the wcnd socket
     * 
     * @return true if success
     */
    @Override
    public boolean wifiUp() {
        return true;
    }

    /**
     * send iwnpi wlan0 start
     * 
     * @return the cmd result, true if success
     */
    @Override
    public boolean wifiStart() {
        byte[] buf = new byte[255];
        String result = null;
        result=SocketUtils.sendCmdAndRecResult(SOCKET_NAME,LocalSocketAddress.Namespace.ABSTRACT,"eng " + SET_EUT_START);

        return analysisResult(WIFI_START, result);
    }

    /**
     * send cmd(iwnpi wlan0 ifacedown) and close wcnd socket
     * 
     * @return true if success
     */
    @Override
    public boolean wifiDown() {
        return true;
    }

    /**
     * send iwnpi wlan0 stop
     * 
     * @return true if success
     */
    @Override
    public boolean wifiStop() {
        byte[] buf = new byte[255];
        String result = null;
        result=SocketUtils.sendCmdAndRecResult(SOCKET_NAME,LocalSocketAddress.Namespace.ABSTRACT,"eng " + SET_EUT_STOP);

        return analysisResult(WIFI_STOP, result);
    }

    /**
     * This is for TX EX: mode 802.11 pkt iwnpi wlan0 set_channel xx iwnpi wlan0 set_pkt_length xx
     * iwnpi wlan0 set_tx_count xx iwnpi wlan0 set_tx_power xx iwnpi wlan0 set_rate xx iwnpi wlan0
     * set_preamble xx iwnpi wlan0 set_bandwidth xx iwnpi wlan0 set_guard_interval iwnpi wlan0
     * tx_start mode Sin Wave iwnpi wlan0 set_channel xx iwnpi wlan0 set_pkt_length xx iwnpi wlan0
     * set_tx_count xx iwnpi wlan0 set_tx_power xx iwnpi wlan0 set_rate xx iwnpi wlan0 set_preamble
     * xx iwnpi wlan0 set_bandwidth xx iwnpi wlan0 set_guard_interval iwnpi wlan0 sin_wave
     * 
     * @param tx
     * @return
     */

    @Override
    public boolean wifiTXGo(WifiTX tx) {
        // if (tx.mode.equals("802.11 pkt")) {
        // if (sendCmd(SET_EUT_SET_CHANNEL + tx.channel, WIFI_SET_CHANNEL)
        // && sendCmd(SET_EUT_SET_POWER + tx.powerlevel, WIFI_TX_POWER)
        // && sendCmd(SET_EUT_SET_RATE + tx.rate, WIFI_SET_RATE)
        // && sendCmd(SET_EUT_TX_START, WIFI_TX_START)) {
        // return true;
        // }
        // } else if (tx.mode.equals("Sin Wave")) {
        // if (sendCmd(SET_EUT_SET_CHANNEL + tx.channel, WIFI_SET_CHANNEL)
        // && sendCmd(SET_EUT_CW_START, WIFI_TX_SINWAVE)) {
        // return true;
        // }
        // }
        Log.d(TAG, "tx.band=" + "" + tx.band);
        Log.d(TAG, "tx.powerlevel=" + tx.powerlevel);
        Log.d(TAG, "tx.preamble=" + tx.preamble);
        Log.d(TAG, "tx.bandwidth=" + tx.bandwidth);
        Log.d(TAG, "tx.channel=" + tx.channel);
        Log.d(TAG, "tx.rate=" + tx.rate);
        Log.d(TAG, "tx.mode=" + tx.mode);
        if (tx.mode.equals("1")) {
            if (sendCmd(SET_EUT_SET_BAND + tx.band, WIFI_SET_BAND)
                    && sendCmd(SET_EUT_SET_POWER + tx.powerlevel, WIFI_TX_POWER)
                    && sendCmd(SET_EUT_SET_PREAMBLE + tx.preamble, WIFI_TX_PREAMBLE)
                    && sendCmd(SET_EUT_SET_BANDWITCH + tx.bandwidth, WIFI_TX_BANDWIDTH)
                    && sendCmd(SET_EUT_SET_CHANNEL + tx.channel, WIFI_SET_CHANNEL)
                    && sendCmd(SET_EUT_SET_RATE + tx.rate, WIFI_SET_RATE)
                    && sendCmd(SET_EUT_SET_MODE + tx.mode, WIFI_SET_MODE)
                    && sendCmd(SET_EUT_TX_START, WIFI_TX_START)) {
                return true;
            }
            return false;
        } else {
            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(mContext, "SET MODE error, mode should be 802.11pkt",
                            Toast.LENGTH_SHORT).show();
                }
            });
            return false;
        }
    }

    /**
     * This is for TX EX: iwnpi wlan0 tx_stop
     * 
     * @return
     */
    @Override
    public boolean wifiTXStop() {
        if (sendCmd(SET_EUT_TX_STOP, WIFI_TX_STOP)) {
            return true;
        }
        return false;
    }

    /**
     * This is for TX EX: iwnpi wlan0 set_channel xx iwnpi wlan0 tx_start
     * 
     * @param tx
     * @return
     */
    @Override
    public boolean wifiTXCw(WifiTX tx) {
        Log.d(TAG, "tx.band=" + "" + tx.band);
        Log.d(TAG, "tx.powerlevel=" + tx.powerlevel);
        Log.d(TAG, "tx.preamble=" + tx.preamble);
        Log.d(TAG, "tx.bandwidth=" + tx.bandwidth);
        Log.d(TAG, "tx.channel=" + tx.channel);
        Log.d(TAG, "tx.rate=" + tx.rate);
        Log.d(TAG, "tx.mode=" + tx.mode);
        if (tx.mode.equals("0")) {
            Log.d(TAG, "mode=sinWave if ");
            if (sendCmd(SET_EUT_SET_BAND + tx.band, WIFI_SET_BAND)
                    && sendCmd(SET_EUT_SET_POWER + tx.powerlevel, WIFI_TX_POWER)
                    && sendCmd(SET_EUT_SET_PREAMBLE + tx.preamble, WIFI_TX_PREAMBLE)
                    && sendCmd(SET_EUT_SET_BANDWITCH + tx.bandwidth, WIFI_TX_BANDWIDTH)
                    && sendCmd(SET_EUT_SET_CHANNEL + tx.channel, WIFI_SET_CHANNEL)
                    && sendCmd(SET_EUT_SET_RATE + tx.rate, WIFI_SET_RATE)
                    && sendCmd(SET_EUT_SET_MODE + tx.mode, WIFI_SET_MODE)
                    && sendCmd(SET_EUT_CW_START, WIFI_TX_SINWAVE)) {
                return true;
            }
            return false;
        } else {
            Log.d(TAG, "mode=sinWave else ");
            mUiThread.post(new Runnable() {
                @Override
                public void run() {

                    Toast.makeText(mContext, "SET MODE error, mode should be Sin Wave",
                            Toast.LENGTH_SHORT).show();
                }
            });
            return false;
        }
    }

    /**
     * This is for RX EX: iwnpi wlan0 set_channel xx iwnpi wlan0 rx_start
     * 
     * @return
     */

    @Override
    public boolean wifiRXStart(WifiRX rx) {
        Log.d(TAG, "tx.band=" + "" + rx.band);
        Log.d(TAG, "tx.bandwidth=" + rx.bandwidth);
        Log.d(TAG, "tx.channel=" + rx.channel);
        if (sendCmd(SET_EUT_SET_BAND + rx.band, WIFI_SET_BAND)
                && sendCmd(SET_EUT_SET_BANDWITCH + rx.bandwidth, WIFI_TX_BANDWIDTH)
                && sendCmd(SET_EUT_SET_CHANNEL + rx.channel, WIFI_SET_CHANNEL)
                && sendCmd(SET_EUT_RX_START, WIFI_RX_START)) {
            return true;
        }
        return false;
    }

    /**
     * This is for RX EX: iwnpi wlan0 get_rx_ok
     * 
     * @return
     */
    @Override
    public String wifiRXResult() {
        return sendCmdStr(SET_EUT_GET_RXOK, WIFI_RX_OK);
    }

    /**
     * This is for RX EX: iwnpi wlan0 rx_stop
     * 
     * @return
     */
    @Override
    public boolean wifiRXStop() {
        if (sendCmd(SET_EUT_RX_STOP, WIFI_RX_STOP)) {
            return true;
        }
        return false;
    }

    /**
     * This is for REG_R EX: iwnpi wlan0 get_reg %s(type) %x(Addr) %x(Length)
     * 
     * @param reg
     * @return
     */
    @Override
    public String wifiREGR(WifiREG reg) {
        return "test";
    }

    /**
     * This is for REG_W EX: iwnpi wlan0 set_reg %s(type) %x(Addr) %x(Vlaue)
     * 
     * @param reg
     * @return
     */
    @Override
    public String wifiREGW(WifiREG reg) {
        return "test";
    }

    @Override
    public String wifiGetStatus() {
        return "test";
    }

    @Override
    public String wifiSetStatusOn() {
        if (sendCmd(SET_EUT_POWERSAVE_MODE + "1", WIFI_SET_CHANNEL)) {
            return "OK";
        }
        return "FAIL";
    }

    @Override
    public String wifiSetStatusOff() {
        if (sendCmd(SET_EUT_POWERSAVE_MODE + "0", WIFI_SET_CHANNEL)) {
            return "OK";
        }
        return "FAIL";
    }

    /**
     * analysis the socket result
     * 
     * @param cmd send cmd
     * @param res the socket return result
     * @return EM analysis Result
     */
    @Override
    public boolean analysisResult(int cmd, String result) {
        Log.d(TAG, "analysisResult cmd is " + cmd + ", socket return result is " + result);
        switch (cmd) {
            case WIFI_UP:
            case WIFI_START:
            case WIFI_STOP:
            case WIFI_DOWN:
            case WIFI_SET_BAND:
            case WIFI_TX_LENGTH:
            case WIFI_TX_COUNT:
            case WIFI_TX_POWER:
            case WIFI_TX_PREAMBLE:
            case WIFI_TX_BANDWIDTH:
            case WIFI_SET_CHANNEL:
            case WIFI_SET_RATE:
            case WIFI_SET_MODE:
            case WIFI_TX_SINWAVE:
            case WIFI_TX_GUARDINTERVAL:
            case WIFI_TX_START:
            case WIFI_TX_STOP:
            case WIFI_RX_START:
            case WIFI_RX_STOP:
                if (result != null && result.startsWith(CMD_SUCCESS)) {
                    return true;
                }
                break;
            default:
                break;
        }
        return false;
    }

    @Override
    public boolean sendCmd(String cmd, int anaycmd) {
        byte[] buf = new byte[255];
        String result = null;
        result=SocketUtils.sendCmdAndRecResult(SOCKET_NAME,LocalSocketAddress.Namespace.ABSTRACT,"eng " + cmd);

        return analysisResult(anaycmd, result);
    }

    @Override
    public String sendCmdStr(String cmd, int anaycmd) {
        byte[] buf = new byte[255];
        String result = null;
        result=SocketUtils.sendCmdAndRecResult(SOCKET_NAME,LocalSocketAddress.Namespace.ABSTRACT,"eng " + cmd);

        return result;
    }

    /**
     * release socket and mOutputStream
     * 
     * @return true
     */
    @Override
    public boolean wifiEUTHelperRelease() {
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
            mSocketClient = null;
        }

        return true;
    }

    /**
     * we should load wifi driver and start wifi before testing TX/RX
     * 
     * @return true if loading driver success, false if fail
     */
    @Override
    public boolean insmodeWifi() {
        return true;
    }

    /**
     * we shoule reload the wifi driver when finish WifiTXActivity/WifiRXActivity
     * 
     * @return true if reload success, false if fail
     */
    @Override
    public boolean remodeWifi() {
        return true;
    }

}
