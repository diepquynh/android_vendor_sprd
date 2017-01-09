
package com.sprd.engineermode.core;

import static com.sprd.engineermode.debuglog.slogui.SlogService.NOTIFICATION_SLOG;
import com.sprd.engineermode.debuglog.slogui.SlogUIAlert.AlertCallBack;
import static com.sprd.engineermode.debuglog.slogui.SlogService.NOTIFICATION_SNAP;
import static com.sprd.engineermode.debuglog.slogui.SlogService.SERVICE_SNAP_KEY;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import android.os.SystemProperties;

import org.apache.http.NameValuePair;
import org.apache.http.message.BasicNameValuePair;

import com.sprd.engineermode.activity.slog.SlogInfo;
import com.sprd.engineermode.debuglog.slogui.ISlogService;
import com.sprd.engineermode.debuglog.slogui.SlogAction;
import com.sprd.engineermode.debuglog.slogui.SlogService;
import com.sprd.engineermode.debuglog.slogui.StorageUtil;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.telephony.ParaSetPrefActivity;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.R;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Environment;
import android.os.IBinder;
import android.os.RemoteException;

import android.util.Log;
import com.sprd.engineermode.utils.SocketUtils;
import java.util.Calendar;

/**
 * Created by SPREADTRUM\zhengxu.zhang on 9/8/15.
 */
public class SlogCore {

    private static final String TAG = "SlogCore";

    public static final String SWITCH_ANDROID_LOG = "";
    public static final String LOG_USER_CMD = "1,0,"
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\""
            + ","
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\"";
    public static final String LOG_DEBUG_CMD = "1,3,"
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\""
            + ","
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\"";
    public static final String LOG_FULL_CMD = "1,5,"
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\""
            + ","
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\"";
    public static final int GET_DSP_LOG = 4;
    public static List<NameValuePair> DIC_LOG_NAME = new ArrayList<NameValuePair>();

    public static void buildDic() {
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_MainLogController", "main log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_EventLogController", "event log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_SystemLogController", "system log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_RadioLogController", "radio log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_KernelLogController", "kernel log "));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_BtHciLogController", "bt hci log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_APCapLogController", "apcap log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_CrashLogController", "crash log"));
        // DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_ModemLogController",""));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_DspLogController", "dsp log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_WcnLogController", "wcn  log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_GpsLogController", "gnss log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_CpCapLogController", "cpcap log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_ArmLogController", "modem log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_AGDspLogController", "ag-dsp log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_Cm4LogController", "cm4 log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_ArmPcmDataController", "arm pcm log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_DspPcmDataController", "dsp pcm log"));
        DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_AGDspPcmDumpLogController", "ag-dsp pcmdump log"));
        // DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_DspPcmDataController",""));
        // DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_ArmPcmDataController",""));
        // DIC_LOG_NAME.add(new BasicNameValuePair("SlogCore_SimLogController","sim log"));
    }

    public static boolean isYlogOpen() {
        String result = SystemProperties.get("init.svc.ylog", "stopped");
        Log.d(TAG, "isYlogOpen result = " + result);
        if (result.equals("running")) {
            return true;
        }
        return false;
    }

    public static void startYLog() {
        if (SystemProperties.getBoolean("persist.ylog.enabled", false))
            return;
        SystemProperties.set("persist.ylog.enabled", "1");
    }

    public static void stopYLog() {
        if (!SystemProperties.getBoolean("persist.ylog.enabled", true))
            return;
        SystemProperties.set("persist.ylog.enabled", "0");
    }

    /*BEGIN BUG575763 zhijie.yang 2016/06/27 */
    public static int SlogCore_ClearYLogController() {
        Log.d(TAG, "clear ylog");
        try {
            String strTmp = SocketUtils
                    .sendCmdAndRecResult("ylog_cli",
                            LocalSocketAddress.Namespace.ABSTRACT,
                            "rylogr\n");
            if (strTmp != null) {
                return 0;
            } else {
                return 1;
            }
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return 1;
        }
    }


    public static String SlogCore_YlogdController(String arg){
        Log.d(TAG, String.format("ylogd_mode want to %s", arg.equals("1") ? "open" : "close"));
        try {
            SocketUtils
                    .sendCmdAndRecResult("ylog_cli",
                            LocalSocketAddress.Namespace.ABSTRACT,
                            String.format("ylog ylogd %s\n", arg.equals("1") ? "start"
                                    : "stop"), 20);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            EngineerModeProtocol.resultBuilder(false);
        }
        return EngineerModeProtocol.resultBuilder(true);
    }

    public static String SlogCore_GetYlogdStatus(){
        try {
            String strTmp = SocketUtils
                    .sendCmdAndRecResult("ylog_cli",
                            LocalSocketAddress.Namespace.ABSTRACT,
                            String.format("ylog ylogd get started\n"), 20);
            return EngineerModeProtocol.resultBuilder(strTmp.contains("1"));
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            EngineerModeProtocol.resultBuilder(false);
        }
        return EngineerModeProtocol.resultBuilder(true);
    }


    public static int SlogCore_ClearModemLogController() {
        Log.d(TAG, "clear modemlog");
        try {
            String strTmp = SocketUtils
            .sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT,
                    "slogctl clear\n");
            if (strTmp != null && strTmp.contains(IATUtils.AT_OK)) {
                Log.d(TAG,"slogmodem clear successful");
                return 0;
            } else {
                return 1;
            }
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return 1;
        }
    }
    /*END BUG575763 zhijie.yang 2016/06/27 */

    /**
     * open main log
     * 
     * @param arg 0 or 1
     * @return
     */
    // pass
    public String SlogCore_MainLogController(String arg) {
        Log.d(TAG, String.format("mainlog want to %s", arg.equals("1") ? "open" : "close"));
        try {
            SocketUtils
                    .sendCmdAndRecResult("ylog_cli",
                            LocalSocketAddress.Namespace.ABSTRACT,
                            String.format("ylog android_main %s\n", arg.equals("1") ? "start"
                                    : "stop"), 20);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            EngineerModeProtocol.resultBuilder(false);
        }
        return EngineerModeProtocol.resultBuilder(true);
    }

    // pass
    public String SlogCore_EventLogController(String arg) {
        Log.d(TAG, String.format("eventlog want to %s", arg.equals("1") ? "open" : "close"));
        try {
            SocketUtils.sendCmdAndRecResult("ylog_cli",
                    LocalSocketAddress.Namespace.ABSTRACT,
                    String.format("ylog android_events %s\n", arg.equals("1") ? "start" : "stop"),
                    20);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            EngineerModeProtocol.resultBuilder(false);
        }
        return EngineerModeProtocol.resultBuilder(true);
    }

    // pass
    public String SlogCore_SystemLogController(String arg) {
        Log.d(TAG, String.format("systemlog want to %s", arg.equals("1") ? "open" : "close"));
        try {
            SocketUtils.sendCmdAndRecResult("ylog_cli",
                    LocalSocketAddress.Namespace.ABSTRACT,
                    String.format("ylog android_system %s\n", arg.equals("1") ? "start" : "stop"),
                    20);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            EngineerModeProtocol.resultBuilder(false);
        }
        return EngineerModeProtocol.resultBuilder(true);
    }

    // pass
    public String SlogCore_CrashLogController(String arg) {
        Log.d(TAG, String.format("crashlog want to %s", arg.equals("1") ? "open" : "close"));
        try {
            SocketUtils.sendCmdAndRecResult("ylog_cli",
                    LocalSocketAddress.Namespace.ABSTRACT,
                    String.format("ylog android_crash %s\n", arg.equals("1") ? "start" : "stop"),
                    20);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            EngineerModeProtocol.resultBuilder(false);
        }
        return EngineerModeProtocol.resultBuilder(true);
    }

    // pass
    public String SlogCore_RadioLogController(String arg) {
        Log.d(TAG, String.format("radiolog want to %s", arg.equals("1") ? "open" : "close"));
        try {
            SocketUtils.sendCmdAndRecResult("ylog_cli",
                    LocalSocketAddress.Namespace.ABSTRACT,
                    String.format("ylog android_radio %s\n", arg.equals("1") ? "start" : "stop"),
                    20);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            EngineerModeProtocol.resultBuilder(false);
        }
        return EngineerModeProtocol.resultBuilder(true);
    }

    // pass
    public String SlogCore_KernelLogController(String arg) {
        Log.d(TAG, String.format("kernellog want to %s", arg.equals("1") ? "open" : "close"));
        try {
            SocketUtils.sendCmdAndRecResult("ylog_cli",
                    LocalSocketAddress.Namespace.ABSTRACT,
                    String.format("ylog kernel %s\n", arg.equals("1") ? "start" : "stop"), 20);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            EngineerModeProtocol.resultBuilder(false);
        }
        return EngineerModeProtocol.resultBuilder(true);
    }

    public String SlogCore_AndroidLogController(String arg) {
        if (arg.equals("1")) {
            Log.d(TAG, "AndroidLog open");
        } else if (arg.equals("0")) {
            Log.d(TAG, "AndroidLog close");
        }
        return EngineerModeProtocol.resultBuilder(true);
    }

    // pass
    public String SlogCore_BtHciLogController(String arg) {
        Log.d(TAG, String.format("bthcilog want to %s", arg.equals("1") ? "open" : "close"));
        try {
            SocketUtils.sendCmdAndRecResult("ylog_cli",
                    LocalSocketAddress.Namespace.ABSTRACT,
                    String.format("ylog hcidump %s\n", arg.equals("1") ? "start" : "stop"), 20);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            EngineerModeProtocol.resultBuilder(false);
        }
        return EngineerModeProtocol.resultBuilder(true);
    }

    // pass
    public String SlogCore_APCapLogController(String arg) {
        Log.d(TAG, String.format("apcaplog want to %s", arg.equals("1") ? "open" : "close"));
        try {
            SocketUtils.sendCmdAndRecResult("ylog_cli",
                    LocalSocketAddress.Namespace.ABSTRACT,
                    String.format("ylog tcpdump %s\n", arg.equals("1") ? "start" : "stop"), 20);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            EngineerModeProtocol.resultBuilder(false);
        }
        return EngineerModeProtocol.resultBuilder(true);
    }

    // pass
    public static String SlogCore_ModemLogController(String arg) {
        Log.d(TAG, String.format("modemlog want to %s", arg.equals("1") ? "open" : "close"));
        boolean state = arg.equals("1");

        String atResponse = EngineerModeProtocol.sendAt(engconstents.ENG_AT_SETARMLOG1 + arg,
                "atchannel0");

        boolean pcDisable = SystemProperties.getBoolean("persist.sys.engpc.disable", false);
        boolean isModeToSd = (pcDisable && state);

        if (SlogAction.CP0_ENABLE) {
            SocketUtils.sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT, isModeToSd ? "ENABLE_LOG WCDMA"
                            : "DISABLE_LOG WCDMA");
        }
        if (SlogAction.CP1_ENABLE) {
            SocketUtils.sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT, isModeToSd ? "ENABLE_LOG TD"
                            : "DISABLE_LOG TD");
        }
        if (SlogAction.CP2_ENABLE) {
            SocketUtils.sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT, isModeToSd ? "ENABLE_LOG WCN"
                            : "DISABLE_LOG WCN");
        }
        if (SlogAction.CP3_ENABLE) {
            SocketUtils.sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT, isModeToSd ? "ENABLE_LOG TDD-LTE"
                            : "DISABLE_LOG TDD-LTE");
        }
        if (SlogAction.CP4_ENABLE) {
            SocketUtils.sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT, isModeToSd ? "ENABLE_LOG FDD-LTE"
                            : "DISABLE_LOG FDD-LTE");
        }
        if (SlogAction.CP5_ENABLE) {
            SocketUtils.sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT, isModeToSd ? "ENABLE_LOG 5MODE"
                            : "DISABLE_LOG 5MODE");
        }

        return EngineerModeProtocol.resultBuilder(true);
    }

    // ?

    public static void setModemLogToPC() {
        // SlogAction.writeSlogConfig();
        // SlogCore_ModemLogController("0");
        SystemProperties.set("persist.sys.engpc.disable", "0");
        /*BEGIN BUG563151 zhijie.yang 2016/05/16 */
        sendMsgToModemd("persist.sys.engpc.disable");
        /*END BUG563151 zhijie.yang 2016/05/16 */
        String strTmp = SocketUtils.sendCmdAndRecResult("wcnd",
                LocalSocketAddress.Namespace.ABSTRACT, "wcn startengpc");
        setAgDspLogClose();
        if (SlogAction.CP0_ENABLE) {
            SocketUtils.sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT, "DISABLE_LOG WCDMA");
        }
        if (SlogAction.CP1_ENABLE) {
            SocketUtils.sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT, "DISABLE_LOG TD");
        }
        if (SlogAction.CP2_ENABLE) {
            SocketUtils.sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT, "DISABLE_LOG WCN");
        }
        if (SlogAction.CP3_ENABLE) {
            SocketUtils.sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT, "DISABLE_LOG TDD-LTE");
        }
        if (SlogAction.CP4_ENABLE) {
            SocketUtils.sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT, "DISABLE_LOG FDD-LTE");
        }
        if (SlogAction.CP5_ENABLE) {
            SocketUtils.sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT, "DISABLE_LOG 5MODE");
        }
    }

    public static void setModemLogToPhone() {
        SystemProperties.set("persist.sys.engpc.disable", "1");
        /*BEGIN BUG563151 zhijie.yang 2016/05/16 */
        sendMsgToModemd("persist.sys.engpc.disable");
        /*EDN BUG563151 zhijie.yang 2016/05/16 */
        String strTmp = SocketUtils.sendCmdAndRecResult("wcnd",
                LocalSocketAddress.Namespace.ABSTRACT, "wcn stopengpc");
        if ("1".equals(SlogInfo.self().getCustomerDefined("SlogCore_AGDspLogController"))) {
            setAgDspLogOpen();
        } else {
            setAgDspLogClose();
        }
        SlogCore_ModemLogController("1");
    }

    public static boolean getModemLogType() {
        if (SystemProperties.getInt("persist.sys.engpc.disable", 1) == 0)
            return true;// PC
        return false;// SD | data
    }

    public static boolean getGnssStatus() {
        String strTmp = SocketUtils.sendCmdAndRecResult("slogmodem",
                LocalSocketAddress.Namespace.ABSTRACT, "GET_LOG_STATE GNSS");
        Log.d(TAG, "the result of GET_LOG_STATE GNSS: " + strTmp);
        if (strTmp != null && strTmp.contains(IATUtils.AT_OK)) {
            if (strTmp.contains("ON"))
                return true;
        }
        return false;
    }

    public static void setGnssOpen() {
        String strTmp = SocketUtils.sendCmdAndRecResult("slogmodem",
                LocalSocketAddress.Namespace.ABSTRACT, "ENABLE_LOG GNSS");
        Log.d(TAG, "the result of ENABLE_LOG GNSS: " + strTmp);
    }

    public static void setGnssClose() {
        String strTmp = SocketUtils.sendCmdAndRecResult("slogmodem",
                LocalSocketAddress.Namespace.ABSTRACT, "DISABLE_LOG GNSS");
        Log.d(TAG, "the result of DISABLE_LOG GNSS: " + strTmp);
    }

    public static boolean getAgDspLogStatus() {
        String strTmp = SocketUtils.sendCmdAndRecResult("slogmodem",
                LocalSocketAddress.Namespace.ABSTRACT, "GET_LOG_STATE AG-DSP");
        Log.d(TAG, "the result of GET_LOG_STATE AG-DSP: " + strTmp);
        if (strTmp != null && strTmp.contains(IATUtils.AT_OK)) {
            if (strTmp.contains("ON"))
                return true;
        }
        return false;
    }

    public static void setAgDspLogOpen() {
        String strTmp = SocketUtils.sendCmdAndRecResult("slogmodem",
                LocalSocketAddress.Namespace.ABSTRACT, "ENABLE_LOG AG-DSP");
        Log.d(TAG, "the result of ENABLE_LOG AG-DSP: " + strTmp);
    }

    public static void setAgDspLogClose() {
        String strTmp = SocketUtils.sendCmdAndRecResult("slogmodem",
                LocalSocketAddress.Namespace.ABSTRACT, "DISABLE_LOG AG-DSP");
        Log.d(TAG, "the result of DISABLE_LOG AG-DSP: " + strTmp);
    }

    /* SPRD: Bug 568186 add CM4 log in EngineerMode @{ */
    public static boolean getCm4LogStatus() {
        String strTmp = SocketUtils.sendCmdAndRecResult("slogmodem",
                LocalSocketAddress.Namespace.ABSTRACT, "GET_LOG_STATE PM_SH\n");
        Log.d(TAG, "the result of GET_LOG_STATE PM_SH: " + strTmp);
        if (strTmp != null && strTmp.contains(IATUtils.AT_OK)) {
            if (strTmp.contains("ON"))
                return true;
        }
        return false;
    }

    public static void setCm4LogOpen() {
        String strTmp = SocketUtils.sendCmdAndRecResult("slogmodem",
                LocalSocketAddress.Namespace.ABSTRACT, "ENABLE_LOG PM_SH\n");
        Log.d(TAG, "the result of ENABLE_LOG PM_SH: " + strTmp);
    }

    public static void setCm4LogClose() {
        String strTmp = SocketUtils.sendCmdAndRecResult("slogmodem",
                LocalSocketAddress.Namespace.ABSTRACT, "DISABLE_LOG PM_SH\n");
        Log.d(TAG, "the result of DISABLE_LOG PM_SH: " + strTmp);
    }
    /* @} */

    public static void sendMsgToModemd(String msg) {
        /*BEGIN BUG563151 zhijie.yang 2016/05/16 */
        Log.d(TAG,"send cmd: " + msg);
        /*END BUG563151 zhijie.yang 2016/05/16 */
        OutputStream mOut = null;
        LocalSocket mClient = null;
        try {
            mClient = new LocalSocket();
            LocalSocketAddress serverAddress = new LocalSocketAddress(
                    "modemd", LocalSocketAddress.Namespace.ABSTRACT);
            mClient.connect(serverAddress);
            mOut = mClient.getOutputStream();
            mOut.write(msg.getBytes());
            mOut.flush();
        } catch (Exception e) {
            Log.w(TAG, "client connet exception " + e.getMessage());
        }

        if (mClient.isConnected()) {
            try {
                mOut.close();
                mClient.close();
            } catch (Exception e) {
                Log.w(TAG, "close client exception " + e.getMessage());
            }
        }
    }

    // ?
    public static int dspOption = 2;

    public String SlogCore_DspLogController(String arg) {
        Log.d(TAG, String.format("dsplog want to %s", arg.equals("0") ? "close" : "open"));
        String responValue = IATUtils.AT_FAIL;
        String atResponse = EngineerModeProtocol.sendAt(engconstents.ENG_AT_SETDSPLOG1 +
                (arg.equals("0") ? "0" : String.format("%d", dspOption)), "atchannel0");
        responValue = EngineerModeProtocol.analysisResponse(atResponse,
                EngineerModeProtocol.SET_DSP_LOG);

        return EngineerModeProtocol.resultBuilder(responValue.contains(IATUtils.AT_OK));
    }

    /* SPRD: add AG-DSP log in EngineerMode @{ */
    public String SlogCore_AGDspLogController(String arg) {
        Log.d(TAG, String.format("ag_dsplog want to %s", arg.equals("0") ? "close" : "open"));
        boolean isPhone = SystemProperties.get("persist.sys.engpc.disable", "0").equals("1");
        if (isPhone && arg.equals("1")) {
            setAgDspLogOpen();
        } else {
            setAgDspLogClose();
        }
        return EngineerModeProtocol.resultBuilder(true);
    }
    /* @} */

    /* SPRD: Bug 568186 add CM4 log in EngineerMode @{ */
    public String SlogCore_Cm4LogController(String arg) {
        Log.d(TAG, String.format("cm4 log want to %s", arg.equals("0") ? "close" : "open"));
        if (arg.equals("1")) {
            if (!getCm4LogStatus()) {
                setCm4LogOpen();
            }
        } else if (arg.equals("0")) {
            if (getCm4LogStatus()) {
                setCm4LogClose();
            }
        }
        return EngineerModeProtocol.resultBuilder(true);
    }
    /* @} */

    /* SPRD: Bug 565553 add ag-dsp pcm dump log in EngineerMode @{ */
    public String SlogCore_AGDspPcmDumpLogController(String arg) {
        Log.d(TAG, String.format("AG-DSP pcm dump want to %s", arg.equals("0") ? "close" : "open"));
        SocketUtils.sendCmdAndRecResult("slogmodem",
                LocalSocketAddress.Namespace.ABSTRACT, arg.equals("0") ? "SET_AGDSP_PCM_OUTPUT OFF"
                        : "SET_AGDSP_PCM_OUTPUT ON");
        return EngineerModeProtocol.resultBuilder(true);
    }

    public String SlogCore_AGDspOutputController(String arg) {
        String cmd = null;
        if (arg.equals("0")) {
            cmd = "SET_AGDSP_LOG_OUTPUT OFF";
        } else if (arg.equals("1")) {
            cmd = "SET_AGDSP_LOG_OUTPUT UART";
        } else if (arg.equals("2")) {
            cmd = "SET_AGDSP_LOG_OUTPUT USB";
        }
        Log.d(TAG, String.format("set ag-dsp output to %s", cmd));
        if (cmd != null) {
            SocketUtils.sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT, cmd);
        }
        return EngineerModeProtocol.resultBuilder(true);
    }
    /* @} */
    public static int SlogCore_GetDspLogStatus() {
        String atResponse = EngineerModeProtocol.sendAt(engconstents.ENG_AT_GETDSPLOG1,
                "atchannel0");
        String responValue = EngineerModeProtocol.analysisResponse(atResponse,
                EngineerModeProtocol.GET_DSP_LOG);
        if (atResponse.contains(IATUtils.AT_OK)) {
            if (responValue.trim().equals("1")) {
                dspOption = 1;
            } else if (responValue.trim().equals("2")) {
                dspOption = 2;
            }
        }
        return dspOption;
    }

    // pass
    public String SlogCore_WcnLogController(String arg) {

        Log.d(TAG, String.format("btlog want to %s", arg.equals("1") ? "open" : "close"));
        // return "";
        // EngineerMode need to record cp2 switching state
        SystemProperties.set("persist.sys.cp2log", arg.trim());
        String strTmp = SocketUtils.sendCmdAndRecResult("wcnd",
                LocalSocketAddress.Namespace.ABSTRACT, "wcn " + "poweron");
        Log.d(TAG, "wcn power on");
        strTmp = SocketUtils.sendCmdAndRecResult("wcnd",
                LocalSocketAddress.Namespace.ABSTRACT, "wcn " + "at+armlog=" + arg);
        Log.d(TAG, "at+armlog=" + arg);
        if (arg.equals("1")) {
            Calendar c = Calendar.getInstance();
            int month = c.get(Calendar.MONTH) + 1;
            int day = c.get(Calendar.DAY_OF_MONTH);
            int hour = c.get(Calendar.HOUR);
            int minute = c.get(Calendar.MINUTE);
            int second = c.get(Calendar.SECOND);
            int millisecond = c.get(Calendar.MILLISECOND);
            String cmd = "wcn " + "at+aptime=" + month + "," + day + "," + hour + "," + minute + "," + second + "," + millisecond + "\r";
            strTmp = SocketUtils.sendCmdAndRecResult("wcnd",
                    LocalSocketAddress.Namespace.ABSTRACT, cmd);
            Log.d(TAG, "set wcn log time");
        }
        strTmp = SocketUtils.sendCmdAndRecResult("wcnd",
                LocalSocketAddress.Namespace.ABSTRACT, "wcn " + "poweroff");
        Log.d(TAG, "wcn power off");

        Log.d(TAG, String.format("%s", strTmp));

        return EngineerModeProtocol.resultBuilder(false);

    }

    public static boolean isSupportGPS() {
        File gpsFile = new File("/data/gnss/config/config.xml");
        File gpsFileAtSystem = new File("/system/etc/config.xml");

        if (!gpsFile.exists()) {
            Log.d(TAG, "/data/gnss/config/config.xml also not existed");
            if (gpsFileAtSystem.exists()) {
                copyFile("/system/etc/config.xml", "/data/gnss/config/config.xml");
            } else {
                Log.d(TAG, "/system/etc/config.xml also not existed");
                return false;
            }
        }
        return true;
    }

    // new add
    public String SlogCore_GpsLogController(String arg) {
        Log.d(TAG, String.format("gpslog want to %s", arg.equals("1") ? "open" : "close"));

        if (!isSupportGPS())
            return EngineerModeProtocol.resultBuilder(false);

        if (arg.equals("1")) {
            setGpsStatus(true);
        } else if (arg.equals("0")) {
            setGpsStatus(false);
        }

        return EngineerModeProtocol.resultBuilder(true);
    }

    // ?
    public String SlogCore_CpCapLogController(String arg) {
        Log.d(TAG, String.format("caplog want to %s", arg.equals("1") ? "open" : "close"));
        String responValue = IATUtils.AT_FAIL;
        String atResponse = EngineerModeProtocol.sendAt(engconstents.ENG_AT_SETCAPLOG1 + arg,
                "atchannel0");

        if (arg.equals("1"))
        {
            responValue = EngineerModeProtocol.analysisResponse(atResponse,
                    EngineerModeProtocol.SET_CAP_LOG_OPEN);
        } else if (arg.equals("0")) {
            responValue = EngineerModeProtocol.analysisResponse(atResponse,
                    EngineerModeProtocol.SET_CAP_LOG_CLOSE);
        }

        return EngineerModeProtocol.resultBuilder(responValue.contains(IATUtils.AT_OK));
    }

    // pass
    public String SlogCore_ArmLogController(String arg) {
        Log.d(TAG, String.format("armlog want to %s", arg.equals("1") ? "open" : "close"));
        String responValue = IATUtils.AT_FAIL;
        String atResponse = EngineerModeProtocol.sendAt(engconstents.ENG_AT_SETARMLOG1 + arg,
                "atchannel0");
        /* SPRD: bug596031 modem loglevel setiing@{ */
        if (arg.equals("1"))
        {
            if (SystemProperties.get("ro.build.type").equalsIgnoreCase("user")) {
                IATUtils.sendATCmd(engconstents.ENG_SET_LOG_LEVEL + LOG_DEBUG_CMD, "atchannel0");
            }
            responValue = EngineerModeProtocol.analysisResponse(atResponse,
                    EngineerModeProtocol.SET_ARM_LOG_OPEN);
        } else if (arg.equals("0")) {
            if (SystemProperties.get("ro.build.type").equalsIgnoreCase("user")) {
                IATUtils.sendATCmd(engconstents.ENG_SET_LOG_LEVEL + LOG_USER_CMD, "atchannel0");
            }
            responValue = EngineerModeProtocol.analysisResponse(atResponse,
                    EngineerModeProtocol.SET_ARM_LOG_CLOSE);
        }
        /* @} */
        return EngineerModeProtocol.resultBuilder(responValue.contains(IATUtils.AT_OK));
    }

    // todo cmd open and close not know
    public String SlogCore_DspPcmDataController(String arg) {
        Log.d(TAG, String.format("dsppcmdata want to %s", arg.equals("1") ? "open" : "close"));
        String responValue = IATUtils.AT_FAIL;
        String atResponse;
        if (arg.equals("1"))
            atResponse = EngineerModeProtocol.sendAt("AT+SPDSP=65535,0,0,4096",
                    "atchannel0");
        else
            atResponse = EngineerModeProtocol.sendAt("AT+SPDSP=65535,0,0,0",
                    "atchannel0");
        return EngineerModeProtocol.resultBuilder(true);
    }

    public String SlogCore_ArmPcmDataController(String arg) {
        Log.d(TAG, String.format("armpcmdata want to %s", arg.equals("1") ? "open" : "close"));
        String responValue = IATUtils.AT_FAIL;
        String atResponse = EngineerModeProtocol.sendAt("AT+SPPCMDUMP=" + arg + ",1,1,255",
                "atchannel0");

        return EngineerModeProtocol.resultBuilder(true);
    }

    public String SlogCore_SimLogController(String arg) {
        Log.d(TAG, String.format("simlog want to %s", arg.equals("1") ? "open" : "close"));
        String responValue = IATUtils.AT_FAIL;
        String atResponse = EngineerModeProtocol.sendAt("AT+SPUSIMDRVLS=" + arg,
                "atchannel0");

        return EngineerModeProtocol.resultBuilder(responValue.contains(IATUtils.AT_OK));
    }

    public void sendModemAssert() {
        Log.d(TAG, "dump modem assert");
        IATUtils.sendATCmd(engconstents.ENG_AT_SET_MANUAL_ASSERT, "atchannel0");
    }

    public static void dumpwcnMem() {
        Log.d(TAG, "dumpwcnMem");
        boolean isUser = SystemProperties.get("ro.build.type").equalsIgnoreCase("user");
        if (isUser) {
            SocketUtils.sendCmdAndRecResult("wcnd",
                    LocalSocketAddress.Namespace.ABSTRACT, "wcn dump_enable");
        }
        String strTmp = SocketUtils.sendCmdAndRecResult("wcnd",
                LocalSocketAddress.Namespace.ABSTRACT, "wcn dumpmem");
    }

    public static String SendCP2At(String cmd) {
        Log.d(TAG, "SendCP2At is " + cmd);
        String strTmp = SocketUtils.sendCmdAndRecResult("wcnd",
                LocalSocketAddress.Namespace.ABSTRACT, cmd);
        return strTmp;
    }

    public static int saveSleepLog() {
        String mSupportMode = "";
        if (SlogAction.CP0_ENABLE) {
            mSupportMode = "WCDMA";
        } else if (SlogAction.CP1_ENABLE) {
            mSupportMode = "TD";
        } else if (SlogAction.CP3_ENABLE) {
            mSupportMode = "TDD-LTE";
        } else if (SlogAction.CP4_ENABLE) {
            mSupportMode = "FDD-LTE";
        } else if (SlogAction.CP5_ENABLE) {
            mSupportMode = "5MODE";
        }

        Log.d(TAG, "save sleeplog  supportmode:" + mSupportMode);
        String strTmp = SocketUtils.sendCmdAndRecResult("slogmodem",
                LocalSocketAddress.Namespace.ABSTRACT, "SAVE_SLEEP_LOG " + mSupportMode);
        if (strTmp.contains("ERROR")) {
            String[] str = strTmp.split("\n");
            String[] str1 = str[0].trim().split("\\s+");
            String str2 = str1[1];
            Log.d(TAG, "Error is " + str2);
            return Integer.parseInt(str2);
        }
        return 0;
    }

    public static int saveLogBuf() {
        String mSupportMode = "";
        if (SlogAction.CP0_ENABLE) {
            mSupportMode = "WCDMA";
        } else if (SlogAction.CP1_ENABLE) {
            mSupportMode = "TD";
        } else if (SlogAction.CP3_ENABLE) {
            mSupportMode = "TDD-LTE";
        } else if (SlogAction.CP4_ENABLE) {
            mSupportMode = "FDD-LTE";
        } else if (SlogAction.CP5_ENABLE) {
            mSupportMode = "5MODE";
        }
        Log.d(TAG, "save logbuf supportmode:" + mSupportMode);
        String strTmp = SocketUtils.sendCmdAndRecResult("slogmodem",
                LocalSocketAddress.Namespace.ABSTRACT, "SAVE_RINGBUF " + mSupportMode);
        if (strTmp.contains("ERROR")) {
            String[] str = strTmp.split("\n");
            String[] str1 = str[0].trim().split("\\s+");
            String str2 = str1[1];
            Log.d(TAG, "Error is " + str2);
            return Integer.parseInt(str2);
        }
        return 0;
    }

    public static String SlogCore_AlwaysShowSlogInNotification(String arg) {

        return EngineerModeProtocol.resultBuilder(true);
    }

    public static String SlogCore_SnapDevice(String arg) {

        return EngineerModeProtocol.resultBuilder(true);
    }

    public static String setMemoryLeak(String arg) {
        String atResponse = EngineerModeProtocol.sendAt(engconstents.ENG_AT_MEMORY_LEAK,
                "atchannel0");
        if (atResponse != null && atResponse.contains(IATUtils.AT_OK)) {
            Log.d(TAG, "dump memory leak log success");
            return EngineerModeProtocol.resultBuilder(true);
        } else {
            Log.d(TAG, "dump memory leak log fail");
            return EngineerModeProtocol.resultBuilder(false);
        }

    }

    public String SlogCore_CpTcpIpLogController(String arg) {
        // CP2
        return EngineerModeProtocol.resultBuilder(true);
    }

    public static void copyFile(String oldPath, String newPath) {
        try {
            int bytesum = 0;
            int byteread = 0;
            File oldfile = new File(oldPath);
            File newFile = new File(newPath);

            if (oldfile.exists()) {
                InputStream inStream = new FileInputStream(oldPath);

                if (!newFile.exists()) {
                    String dir[] = newPath.split("/");
                    String dirPath = "";
                    for (int i = 0; i < dir.length - 1; ++i) {
                        dirPath += "/" + dir[i];
                    }

                    (new File(dirPath)).mkdirs();
                    Log.d(TAG, "make dir " + dirPath);
                }
                FileOutputStream fs = new FileOutputStream(newPath);
                byte[] buffer = new byte[1444];
                int length;
                while ((byteread = inStream.read(buffer)) != -1) {
                    bytesum += byteread;
                    System.out.println(bytesum);
                    fs.write(buffer, 0, byteread);
                }
                inStream.close();
            }
        } catch (Exception e) {
            Log.d(TAG, String.format("copy failed from %s to %s", oldPath, newPath));
        }
    }

    private void setGpsStatus(boolean isOpen) {
        String result = null;
        String gpsInfo = readFileToString("/data/gnss/config/config.xml");
        Log.d(TAG, "gpsInfo: " + gpsInfo);
        String info[] = gpsInfo.split("\n");

        for (int i = 0; i < info.length; ++i) {
            if (info[i].contains("LOG-ENABLE")) {
                Log.d(TAG, "gpsInfoKey: " + info[i]);
                if (isOpen) {
                    result = info[i].replace("FALSE", "TRUE");
                } else {
                    result = info[i].replace("TRUE", "FALSE");
                }
                if (result != null)
                    info[i] = result;
            }
        }

        if (result != null) {
            result = "";
            for (int i = 0; i < info.length; ++i) {
                result += info[i] + "\n";
            }
            Log.d(TAG, "gpsInfo: " + result);
            writeFile("/data/gnss/config/config.xml", result);
        }
    }

    private String readFileToString(String filePath) {
        String result = "";
        try {

            String encoding = "UTF-8";
            File file = new File(filePath);

            if (file.isFile() && file.exists()) {
                InputStreamReader read = new InputStreamReader(
                        new FileInputStream(file), encoding);
                BufferedReader bufferedReader = new BufferedReader(read);
                String lineTxt = null;
                while ((lineTxt = bufferedReader.readLine()) != null) {
                    result += lineTxt + "\n";
                }
                Log.d(TAG, result);
                read.close();
            }
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        return result;
    }

    private void writeFile(String filePath, String result) {
        FileOutputStream outputStream;
        try {
            File file = new File(filePath);
            // outputStream = SlogInfo.x.openFileOutput(filePath, Context.MODE_PRIVATE);
            outputStream = new FileOutputStream(file);
            outputStream.write(result.getBytes());
            outputStream.close();
        } catch (Exception e) {
            // TODO Auto-generated catch block
            Log.d(TAG, String.format("wrtie %s Error Exception:%s", filePath, e));
        }

    }

    public static final int NOTIFICATION_SLOG = 1;

    public static String SlogCore_GetLogSavePath() {
        String strTmp = "data";
        try {
            strTmp = SocketUtils
                    .sendCmdAndRecResult("ylog_cli",
                            LocalSocketAddress.Namespace.ABSTRACT,
                            "rootdir\n");
            String[] str = strTmp.split("\n");
            strTmp = str[0].trim();
        } catch (Exception e) {
            Log.e(TAG, e.toString());
        }

        return strTmp + File.separator + "ylog";
    }

    public static int SlogCore_GetAndroidLogLevel() {
        return SlogAction.getLevel(SlogAction.MAINKEY);
    }

}
