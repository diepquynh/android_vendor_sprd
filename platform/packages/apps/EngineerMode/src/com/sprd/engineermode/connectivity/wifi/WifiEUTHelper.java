
package com.sprd.engineermode.connectivity.wifi;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

import android.content.Context;

import android.text.TextUtils;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Message;
import android.os.SystemProperties;
import android.util.Log;

import android.widget.Toast;

public class WifiEUTHelper {
    static boolean isMarlin = SystemProperties.get("ro.modem.wcn.enable").equals("1");
    private static WifiEUTHelper helper;
    protected static Context mContext;

    public static synchronized WifiEUTHelper getHelper() {
        if (isMarlin) {
            helper = new WifiEUTHelperMarlin();
        } else {
            helper = new WifiEUTHelperBrcm();
        }
        return helper;
    }

    public static synchronized WifiEUTHelper getHelper(Context context) {
        mContext = context;
        return getHelper();
    }

    static class WifiTX {
        public String band;
        public String channel;
        public String pktlength;
        public String pktcnt;
        public String powerlevel;
        public String rate;
        public String mode;
        public String chip;
        public String preamble;
        public String bandwidth;
        public String guardinterval;
    }

    static class WifiRX {
        public String band;
        public String bandwidth;
        public String channel;
        public String rxtestnum;
    }

    static class WifiREG {
        public String type;
        public String addr;
        public String length;
        public String value;
    }

    /**
     * creat wcnd socket and send iwnpi wlan0 ifaceup because this cmd is the first cmd, so creat
     * socket here but the socket is not closed here, and close it when wifiDown(), cause the
     * "iwnpi wlan0 ifacedown" cmd is the last cmd sended by the wcnd socket
     * 
     * @return true if success
     */
    public boolean wifiUp() {
        return true;
    }

    /**
     * send iwnpi wlan0 start
     * 
     * @return the cmd result, true if success
     */
    public boolean wifiStart() {
        return true;
    }

    /**
     * send cmd(iwnpi wlan0 ifacedown) and close wcnd socket
     * 
     * @return true if success
     */
    public boolean wifiDown() {
        return true;
    }

    /**
     * send iwnpi wlan0 stop
     * 
     * @return true if success
     */
    public boolean wifiStop() {
        return true;
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

    public boolean wifiTXGo(WifiTX tx) {
        return true;
    }

    /**
     * This is for TX EX: iwnpi wlan0 tx_stop
     * 
     * @return
     */
    public boolean wifiTXStop() {
        return true;
    }

    /**
     * This is for TX EX: iwnpi wlan0 set_channel xx iwnpi wlan0 tx_start
     * 
     * @param tx
     * @return
     */
    public boolean wifiTXCw(WifiTX tx) {
        return true;
    }

    /**
     * This is for RX EX: iwnpi wlan0 set_channel xx iwnpi wlan0 rx_start
     * 
     * @return
     */

    public boolean wifiRXStart(WifiRX rx) {
        return true;
    }

    /**
     * This is for RX EX: iwnpi wlan0 get_rx_ok
     * 
     * @return
     */
    public String wifiRXResult() {
        return "test";
    }

    /**
     * This is for RX EX: iwnpi wlan0 rx_stop
     * 
     * @return
     */
    public boolean wifiRXStop() {
        return true;
    }

    /**
     * This is for REG_R EX: iwnpi wlan0 get_reg %s(type) %x(Addr) %x(Length)
     * 
     * @param reg
     * @return
     */
    public String wifiREGR(WifiREG reg) {
        return "test";
    }

    /**
     * This is for REG_W EX: iwnpi wlan0 set_reg %s(type) %x(Addr) %x(Vlaue)
     * 
     * @param reg
     * @return
     */
    public String wifiREGW(WifiREG reg) {
        return "test";
    }

    public String wifiGetStatus() {
        return "test";
    }

    public String wifiSetStatusOn() {
        return "test";
    }

    public String wifiSetStatusOff() {
        return "test";
    }

    /**
     * analysis the socket result
     * 
     * @param cmd send cmd
     * @param res the socket return result
     * @return EM analysis Result
     */
    public boolean analysisResult(int cmd, String result) {
        return true;
    }

    public boolean sendCmd(String cmd, int anaycmd) {
        return true;
    }

    public String sendCmdStr(String cmd, int anaycmd) {
        return "test";
    }

    /**
     * we should load wifi driver and start wifi before testing TX/RX
     * 
     * @return true if loading driver success, false if fail
     */
    public boolean insmodeWifi() {
        return true;
    }

    /**
     * we shoule reload the wifi driver when finish WifiTXActivity/WifiRXActivity
     * 
     * @return true if reload success, false if fail
     */
    public boolean remodeWifi() {
        return true;
    }

    public boolean wifiEUTHelperRelease() {
        return true;
    }
}
