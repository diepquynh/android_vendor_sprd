
package com.sprd.engineermode.connectivity.wifi;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import android.text.TextUtils;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Message;
import android.os.SystemProperties;
import android.util.Log;
import com.sprd.engineermode.utils.SocketUtils;

import android.widget.Toast;

public class WifiEUTHelperMarlin extends WifiEUTHelper {

    private static final String TAG = "WifiEUTHelperMarlin";
    private static final String SOCKET_NAME = "wcnd_eng";
    private static final String CMD_SUCCESS = "OK";
    private static final String CMD_FAIL = "Fail";

    // iwnpi wlan0 ifaceup is sended by iwnpi, so equal to ifconfig wlan0 up,
    // iwnpi wlan0 ifacedown is sended by iwnpi, so equal to ifconfig wlan0 down
    private static final String WIFI_DRIVER_INSMOD = "iwnpi wlan0 insmod";
    private static final String WIFI_DRIVER_RMMOD = "iwnpi wlan0 rmmod";

    private static final String SET_EUT_UP = "iwnpi wlan0 ifaceup";
    private static final String SET_EUT_DOWN = "iwnpi wlan0 ifacedown";
    private static final String SET_EUT_START = "iwnpi wlan0 start";
    private static final String SET_EUT_STOP = "iwnpi wlan0 stop";
    private static final String SET_EUT_SET_CHANNEL = "iwnpi wlan0 set_channel ";
    private static final String SET_EUT_SET_RATE = "iwnpi wlan0 set_rate ";

    // for tx
    private static final String SET_EUT_TX_START = "iwnpi wlan0 tx_start";
    private static final String SET_EUT_TX_STOP = "iwnpi wlan0 tx_stop";
    private static final String SET_EUT_CW_START = "iwnpi wlan0 sin_wave";
    private static final String SET_EUT_SET_POWER = "iwnpi wlan0 set_tx_power ";
    private static final String SET_EUT_SET_LENGTH = "iwnpi wlan0 set_pkt_length ";
    private static final String SET_EUT_SET_COUNT = "iwnpi wlan0 set_tx_count ";
    private static final String SET_EUT_SET_PREAMBLE = "iwnpi wlan0 set_preamble ";
    private static final String SET_EUT_BANDWIDTH = "iwnpi wlan0 set_bandwidth ";
    private static final String SET_EUT_GUARDINTERVAL = "iwnpi wlan0 set_guard_interval ";

    // for rx
    private static final String SET_EUT_RX_START = "iwnpi wlan0 rx_start";
    private static final String SET_EUT_RX_STOP = "iwnpi wlan0 rx_stop";
    private static final String SET_EUT_GET_RXOK = "iwnpi wlan0 get_rx_ok";

    // for reg_wr
    private static final String SET_EUT_READ = "iwnpi wlan0 get_reg ";
    private static final String SET_EUT_WRITE = "iwnpi wlan0 set_reg ";

    private static final String CMD_ENABLED_POWER_SAVE = "iwnpi wlan0 lna_on";
    private static final String CMD_DISABLED_POWER_SAVE = "iwnpi wlan0 lna_off";
    private static final String CMD_GET_POWER_SAVE_STATUS = "iwnpi wlan0 lna_status";

    private static final int WIFI_UP = 0;
    private static final int WIFI_START = 1;
    private static final int WIFI_STOP = 2;
    private static final int WIFI_DOWN = 3;
    private static final int WIFI_SET_CHANNEL = 4;
    private static final int WIFI_SET_RATE = 5;
    private static final int WIFI_TX_START = 6;
    private static final int WIFI_TX_STOP = 7;
    private static final int WIFI_TX_POWER = 8;
    private static final int WIFI_TX_COUNT = 9;
    private static final int WIFI_TX_LENGTH = 10;
    private static final int WIFI_TX_PREAMBLE = 11;
    private static final int WIFI_TX_SINWAVE = 12;
    private static final int WIFI_TX_BANDWIDTH = 13;
    private static final int WIFI_TX_GUARDINTERVAL = 14;
    private static final int WIFI_RX_START = 15;
    private static final int WIFI_RX_STOP = 16;
    private static final int WIFI_RX_OK = 17;
    private static final int WIFI_REG_READ = 18;
    private static final int WIFI_REG_WRITE = 19;
    private static final int WIFI_GET_STATUS = 20;
    private static final int WIFI_SET_STATUS_ON = 21;
    private static final int WIFI_SET_STATUS_OFF = 22;
    private static final int WIFI_INSMOD_DRIVER = 23;
    private static final int WIFI_RMMOD_DRIVER = 24;

    private static LocalSocket mSocketClient;
    private static LocalSocketAddress mSocketAddress;
    private static OutputStream mOutputStream;
    private static InputStream mInputStream;

    private static LocalSocket mInsmodeSocketClient;
    private static LocalSocketAddress mInsmodeSocketAddress;
    private static OutputStream mInsmodeOutputStream;
    private static InputStream mInsmodeInputStream;

    /**
     * creat wcnd socket and send iwnpi wlan0 ifaceup because this cmd is the first cmd, so creat
     * socket here but the socket is not closed here, and close it when wifiDown(), cause the
     * "iwnpi wlan0 ifacedown" cmd is the last cmd sended by the wcnd socket
     * 
     * @return true if success
     */
    @Override
    public boolean wifiUp() {
        byte[] buf = new byte[255];
        String result = null;
        result=SocketUtils.sendCmdAndRecResult(SOCKET_NAME,LocalSocketAddress.Namespace.ABSTRACT,"eng " + SET_EUT_UP);

        return analysisResult(WIFI_UP, result);
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
        byte[] buf = new byte[255];
        String result = null;
        result=SocketUtils.sendCmdAndRecResult(SOCKET_NAME,LocalSocketAddress.Namespace.ABSTRACT,"eng " + SET_EUT_STOP);

        return analysisResult(WIFI_DOWN, result);
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
        int temp_power = 0;
        int temp_rate = 0;
        if (TextUtils.isEmpty(tx.powerlevel)) {
            tx.powerlevel = "0";
        }
        /** SPRD:Bug 579966  @{ **/
        if (tx.mode.equals("802.11 pkt")) {
            temp_rate = Integer.parseInt(tx.rate, 10);
            if ("1".equals(tx.chip)) {
                if (temp_rate == 6 || temp_rate == 9 || temp_rate == 12 || temp_rate == 18 ||
                        temp_rate == 24 || temp_rate == 36 || temp_rate == 48 || temp_rate == 54 ||
                        temp_rate == 7 || temp_rate == 13 || temp_rate == 19 || temp_rate == 26 ||
                        temp_rate == 39 || temp_rate == 52 || temp_rate == 58 || temp_rate == 65) {
                    temp_power = Integer.parseInt(tx.powerlevel, 10) + 1000;
                } else {
                    temp_power = Integer.parseInt(tx.powerlevel, 10);
                }
                Log.d(TAG, "2351 eut iwnpi temp_power =" + temp_power);
            } else {
                temp_power = Integer.parseInt(tx.powerlevel, 10);
            }
            /*** @} **/
            if (sendCmd(SET_EUT_SET_CHANNEL + tx.channel, WIFI_SET_CHANNEL)
                    && sendCmd(SET_EUT_SET_LENGTH + tx.pktlength,
                            WIFI_TX_LENGTH)
                    && sendCmd(SET_EUT_SET_COUNT + tx.pktcnt, WIFI_TX_COUNT)
                    && sendCmd(SET_EUT_SET_POWER + String.valueOf(temp_power), WIFI_TX_POWER)
                    && sendCmd(SET_EUT_SET_RATE + tx.rate, WIFI_SET_RATE)
                    && sendCmd(SET_EUT_SET_PREAMBLE + tx.preamble,
                            WIFI_TX_PREAMBLE)
                    && sendCmd(SET_EUT_BANDWIDTH + tx.bandwidth,
                            WIFI_TX_BANDWIDTH)
                    && sendCmd(SET_EUT_GUARDINTERVAL + tx.guardinterval,
                            WIFI_TX_GUARDINTERVAL)
                    && sendCmd(SET_EUT_TX_START, WIFI_TX_START)) {
                return true;
            }
        } else if (tx.mode.equals("Sin Wave")) {
            if (sendCmd(SET_EUT_SET_CHANNEL + tx.channel, WIFI_SET_CHANNEL)
                    && sendCmd(SET_EUT_SET_LENGTH + tx.pktlength,
                            WIFI_TX_LENGTH)
                    && sendCmd(SET_EUT_SET_COUNT + tx.pktcnt, WIFI_TX_COUNT)
                    && sendCmd(SET_EUT_SET_POWER + tx.powerlevel, WIFI_TX_POWER)
                    && sendCmd(SET_EUT_SET_RATE + tx.rate, WIFI_SET_RATE)
                    && sendCmd(SET_EUT_SET_PREAMBLE + tx.preamble,
                            WIFI_TX_PREAMBLE)
                    && sendCmd(SET_EUT_BANDWIDTH + tx.bandwidth,
                            WIFI_TX_BANDWIDTH)
                    && sendCmd(SET_EUT_GUARDINTERVAL + tx.guardinterval,
                            WIFI_TX_GUARDINTERVAL)
                    && sendCmd(SET_EUT_CW_START, WIFI_TX_SINWAVE)) {
                return true;
            }
        }
        return false;
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
        if (sendCmd(SET_EUT_SET_CHANNEL + tx.channel, WIFI_SET_CHANNEL)
                && sendCmd(SET_EUT_TX_START, WIFI_TX_START)) {
            return true;
        }
        return false;
    }

    /**
     * This is for RX EX: iwnpi wlan0 set_channel xx iwnpi wlan0 rx_start
     * 
     * @return
     */
    @Override
    public boolean wifiRXStart(WifiRX rx) {
        if (sendCmd(SET_EUT_SET_CHANNEL + rx.channel, WIFI_SET_CHANNEL)
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
        return sendCmdStr(SET_EUT_READ + reg.type + " " + reg.addr + " " + reg.length,
                WIFI_REG_READ);
    }

    /**
     * This is for REG_W EX: iwnpi wlan0 set_reg %s(type) %x(Addr) %x(Vlaue)
     * 
     * @param reg
     * @return
     */
    @Override
    public String wifiREGW(WifiREG reg) {
        return sendCmdStr(SET_EUT_WRITE + reg.type + " " + reg.addr + " " + reg.value,
                WIFI_REG_WRITE);
    }

    @Override
    public String wifiGetStatus() {
        return sendCmdStr(CMD_GET_POWER_SAVE_STATUS, WIFI_GET_STATUS);
    }

    @Override
    public String wifiSetStatusOn() {
        return sendCmdStr(CMD_ENABLED_POWER_SAVE, WIFI_SET_STATUS_ON);
    }

    @Override
    public String wifiSetStatusOff() {
        return sendCmdStr(CMD_DISABLED_POWER_SAVE, WIFI_SET_STATUS_OFF);
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
            case WIFI_INSMOD_DRIVER:
            case WIFI_RMMOD_DRIVER:
            case WIFI_UP:
            case WIFI_START:
            case WIFI_STOP:
            case WIFI_DOWN:
            case WIFI_SET_CHANNEL:
            case WIFI_SET_RATE:
            case WIFI_TX_START:
            case WIFI_TX_STOP:
            case WIFI_TX_POWER:
            case WIFI_TX_COUNT:
            case WIFI_TX_LENGTH:
            case WIFI_TX_PREAMBLE:
            case WIFI_TX_SINWAVE:
            case WIFI_TX_BANDWIDTH:
            case WIFI_TX_GUARDINTERVAL:
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
     * we should load wifi driver and start wifi before testing TX/RX
     * 
     * @return true if loading driver success, false if fail
     */
    @Override
    public boolean insmodeWifi() {
        byte[] buf = new byte[255];
        String result = null;
        result=SocketUtils.sendCmdAndRecResult(SOCKET_NAME,LocalSocketAddress.Namespace.ABSTRACT,"eng " + WIFI_DRIVER_INSMOD);

        return analysisResult(WIFI_INSMOD_DRIVER, result);
    }

    /**
     * we shoule reload the wifi driver when finish WifiTXActivity/WifiRXActivity
     * 
     * @return true if reload success, false if fail
     */
    @Override
    public boolean remodeWifi() {
        byte[] buf = new byte[255];
        String result = null;
        result=SocketUtils.sendCmdAndRecResult(SOCKET_NAME,LocalSocketAddress.Namespace.ABSTRACT,"eng " + WIFI_DRIVER_RMMOD);

        if(null == result){
            return false;
        }
        return true;
    }
}
