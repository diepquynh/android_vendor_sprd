/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sprd.engineermode.debuglog.slogui;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;
import java.io.OutputStream;

import org.apache.http.util.EncodingUtils;

import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.media.MediaScannerConnection;
import android.os.Bundle;
import android.os.Environment;
import android.os.FileObserver;
import android.os.SystemProperties;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.widget.Toast;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import java.nio.charset.StandardCharsets;
import java.io.OutputStream;

import com.android.internal.app.IMediaContainerService;
import com.sprd.engineermode.R;
import com.sprd.engineermode.utils.SocketUtils;
import com.sprd.engineermode.utils.IATUtils;
//import com.spreadst.android.eng.engfetch;

public class SlogAction {
    public static final boolean DBG = true;
    private static volatile String mCache;
    private static boolean sDumpLog = false;
    private static boolean sClearLog = false;
    private static final Object mLock = new Object();
    private static LocalSocket mClient;
    private static OutputStream mOut;

    // <action android:name="slogui.intent.action.CLEARLOG_COMPLETED" />
    // <action android:name="slogui.intent.action.DUMPLOG_COMPLETED" />
    public static final String ACTION_CLEARLOG_COMPLETED = "slogui.intent.action.CLEARLOG_COMPLETED";
    public static final String ACTION_DUMPLOG_COMPLETED = "slogui.intent.action.DUMPLOG_COMPLETED";
    public static final String EXTRA_CLEAR_RESULT = "clear_result";
    public static final String EXTRA_DUMP_RESULT = "dump_result";

    public static final boolean CP0_ENABLE = SystemProperties.getBoolean(
            "ro.modem.w.enable", false)
            || SystemProperties.getBoolean("persist.modem.w.enable", false);

    public static final boolean CP1_ENABLE = SystemProperties.getBoolean(
            "persist.modem.t.enable", false)
            || SystemProperties.getBoolean("ro.modem.t.enable", false);

    public static final boolean CP2_ENABLE = SystemProperties.getBoolean(
            "ro.modem.wcn.enable", false);

    public static final boolean CP3_ENABLE = SystemProperties.getBoolean(
            "persist.modem.tl.enable", false);

    public static final boolean CP4_ENABLE = SystemProperties.getBoolean(
            "persist.modem.lf.enable", false);

    public static final boolean CP5_ENABLE = SystemProperties.getBoolean(
            "persist.modem.l.enable", false);

    public static boolean BLUETHOOTH_ENABLE = false;

    public static boolean CAP_ENABLE = false;

    public interface Contracts {
        // /**Query the working state of slog, you can see them through log. See
        // more {@see runSlogCommand} **/
        // public static final int SLOG_COMMAND_QUERY = "slogctl query";
        // Run slog control after set States
        public static final int SLOG_COMMAND_RELOAD = 11;
        // Command : Clear Log
        public static final int SLOG_COMMAND_CLEAR = 12;
        // Command : Export Logs
        public static final int SLOG_COMMAND_DUMP = 13;

        public static final int SLOG_COMMAND_SCREENSHOT = 14;

        public static final int SLOG_COMMAND_FETCH = 15;

        public static final int SLOG_COMMAND_CHECK = 16;

        public static final int SLOG_COMMAND_UNLINK = 17;

        public static final int SLOG_COMMAND_RESET = 18;

        public static final int SLOG_BT_FASLE = 19;

    }

    //public static final String SLOG_CONFIG_PATH = "/data/local/tmp/slog/slog.conf";
    public static final String SLOG_CONFIG_PATH = "/data/local/tmp/slog";
    // The name of package.
    // private static final String PACKAGE_NAME = "com.spreadtrum.android.eng";
    // The tag for slog
    private static final String TAG = "SlogAction";
    // slog.conf StorageLocation
    private static final String SLOG_CONF_LOCATION = "/data/local/tmp/slog/slog.conf";
    // The common-control of logs and etc.
    // private static final boolean DBG = true;

    // A tester to confirm slog is running.
    // private static final String SLOG_COMMAND_QUERY = "slogctl query";
    // Run slog control after set States
    private static final String SLOG_COMMAND_RELOAD = "slogctl reload";
    // Command : Clear Log
    private static final String SLOG_COMMAND_CLEAR = "slogctl clear";
    // Command : Export Logs
    private static final String SLOG_COMMAND_DUMP = "slogctl dump ";

    private static final String SLOG_COMMAND_SCREENSHOT = "slogctl screen";

    private static final String SLOG_COMMAND_FETCH = "synchronism download";

    private static final String SLOG_COMMAND_CHECK = "synchronism check";

    private static final String SLOG_COMMAND_UNLINK = "synchronism unlink";

    private static final String SLOG_COMMAND_RESET = "rm " + SLOG_CONF_LOCATION;

    private static final String SLOG_BT_FALSE = "slogctl bt false";
    // New Feature
    // public static String Options[] = null;

    /** Tags,which used to differ Stream States On->true **/
    public static final String ON = "on";
    /** Tags,which used to differ Stream States Off->false **/
    public static final String OFF = "off";

    /** Log's StorageLocation SDCard->external or NAND->internal **/
    public static final String STORAGEKEY = "logpath\t\t";
    public static final String STORAGENAND = "internal";
    public static final String STORAGESDCARD = "external";

    /** keyName->General **/
    // TODO:Bad reading method, need improve.
    public static final String GENERALKEY = "\n";
    public static final String GENERALON = "enable";
    public static final String GENERALOFF = "disable";
    public static final String GENERALLOWPOWER = "low_power";

    // keyName->Options
    public static final String KERNELKEY = "stream\tkernel\t";
    public static final String SYSTEMKEY = "stream\tsystem\t";
    public static final String RADIOKEY = "stream\tradio\t";
    public static final String MODEMKEY = "stream\tmodem\t";
    public static final String CP0KEY = "stream\tcp_wcdma\t";
    public static final String CP1KEY = "stream\tcp_td-scdma\t";
    public static final String CP2KEY = "stream\tcp_wcn\t\t";
    public static final String CP3KEY = "stream\tcp_td-lte\t";
    public static final String CP4KEY = "stream\tcp_tdd-lte\t";
    public static final String CP5KEY = "stream\tcp_fdd-lte\t";

    public static final String MAINKEY = "stream\tmain\t";
    public static final String EVENTKEY = "stream\tevents\t";
    public static final String TCPKEY = "stream\ttcp\t";
    public static final String BLUETOOTHKEY = "stream\tbt\t";
    public static final String MISCKEY = "misc\tmisc\t";
    public static final String SAVEALLKEY = "var\tslogsaveall\t";

    public static final String HWWATCHDOGKEY = "var\tslogsaveall\t";
    public static final String SYSDUMPKEY = "var\tsysdump\t\t";
    public static final String COREDUMPKEY = "var\tcoredump\t";
    public static final String HPROFSKEY = "var\thprofs\t\t";
    public static final String SPRDDEBUGKEY = "var\tsprd_debug\t";
    public static final String LOGSIZE = "logsize\t";
    public static final String LOGOVRE = "log_overwrite\t";

    /** Android keyName **/
    public static final int ANDROIDKEY = 101;
    public static final int MODEMLOGKEY = 102;
    public static final String ANDROIDSPEC = "android";

    public static final String DECODE_ERROR = "decode error";
    // new feature
    public static final int LENGTHOPTIONSTREAM = 3;

    public static final int OptionStreamState = 0;
    public static final int OptionStreamSize = 1;
    public static final int OptionStreamLevel = 2;

    // public static final String SIZE[] = new
    // String[]{"0","50","100","150","200"};

    // public static int position;
    // private static final int SLOG_COMMAND_RETURN_CHECK_FAILED = 255;
    private static final String STREAM = "stream";
    private static final String MISC = "misc";
    public static final int SLOG_COMMAND_RETURN_FAILED = -1;
    public static final int SLOG_COMMAND_RETURN_OK = 0;
    public static final int SLOG_COMMAND_RETURN_EXCEPTION = -126;

    private static HandlerThread sHandlerThread;
    private static Handler sHandler;
    private static HandlerThread sTimeoutThread;
    private static Handler sTimeoutHandler;
    private static HandlerThread sDumpOrClearThread;
    private static Handler SDumpOrClearHandler;
    private static boolean sDirty;

    private static final Object sLock = new Object();

    static class TimeoutRunnable extends ToastRunnable {
        TimeoutRunnable(Context context) {
            // TODO remove ""
            super(context, context.getString(R.string.screenshot_timeout));
        }
    }

    static class ToastRunnable implements Runnable {
        Context mContext;
        String mPrompt;

        ToastRunnable(Context context, String prompt) {
            mContext = context;
            mPrompt = prompt;
        }

        @Override
        public void run() {
            // TODO remove ""
            // Log.d(TAG, "show toast now, the prompt is " + mPrompt);
            Toast.makeText(mContext, mPrompt, Toast.LENGTH_SHORT).show();
        }
    }

    static class ScreenshotRunnable implements Runnable {
        Context mContext;
        TimeoutRunnable mTimeout;

        ScreenshotRunnable(Context context, TimeoutRunnable timeout) {
            mContext = context;
            mTimeout = timeout;
        }

        @Override
        public void run() {
            synchronized (sLock) {
                // Log.d(TAG, "start screenshot now");
                screenShot(mContext, mTimeout);
            }
        }
    }
    private static Object sReadLock;

    static {
        sHandlerThread = new HandlerThread("SlogActionHandler");
        sHandlerThread.start();
        sHandler = new Handler(sHandlerThread.getLooper());
        sTimeoutThread = new HandlerThread("TimeoutWatchingHandler");
        sTimeoutThread.start();
        sTimeoutHandler = new Handler(sTimeoutThread.getLooper());
        sDumpOrClearThread = new HandlerThread("SlogDump/ClearWorkThread");
        sDumpOrClearThread.start();
        SDumpOrClearHandler = new Handler(sDumpOrClearThread.getLooper());
        sReadLock = new Object();
    }

    static FileObserver sSlogObserver = new FileObserver(SLOG_CONFIG_PATH) {
        public void onEvent(int event, String path) {
            Log.v("duke", "observer "+ path +" event = 0x" + Integer.toHexString(event));
            if (event == FileObserver.DELETE_SELF) {
                stopWatching();
                return;
            }
            synchronized(sReadLock) {
                sReadLock.notifyAll();
            }
        }
    };
    /**
     * Security confirm, the engineering model has system authority, so it can
     * run "everything" by running {@code SlogAction#runSlogCommand(int)}, and
     * all of them could take effect, if runSlogCommand function invoked by some
     * one, that would take huge damage to the platform.
     * 
     * @param convert , the command/keyName/otherCase you want to convert to
     *            string
     * @return the effective string return back
     */
    private static String resolveCommandToString(int convert) {
        switch (convert) {
            case Contracts.SLOG_COMMAND_CHECK:
                return SLOG_COMMAND_CHECK;
            case Contracts.SLOG_COMMAND_FETCH:
                return SLOG_COMMAND_FETCH;
            case Contracts.SLOG_COMMAND_UNLINK:
                return SLOG_COMMAND_UNLINK;
            case Contracts.SLOG_COMMAND_RELOAD:
                return SLOG_COMMAND_RELOAD;
            case Contracts.SLOG_COMMAND_CLEAR:
                return SLOG_COMMAND_CLEAR;
            case Contracts.SLOG_COMMAND_DUMP:
                return SLOG_COMMAND_DUMP;
            case Contracts.SLOG_COMMAND_SCREENSHOT:
                return SLOG_COMMAND_SCREENSHOT;
            case Contracts.SLOG_COMMAND_RESET:
                return SLOG_COMMAND_RESET;
            case Contracts.SLOG_BT_FASLE:
                return SLOG_BT_FALSE;
        }

        return "";
    }

    /**
     * This function can change InputStream to String, if catching exception,
     * I'll return {@code SlogAction.Contracts#DECODE_ERROR}.
     */
    public static String decodeInputStream(InputStream input) {
        if (input == null) {
            Log.e(TAG, DECODE_ERROR + ", InputStream is null.");
            return DECODE_ERROR;
        }

        byte[] buffer = null;
        try {
            buffer = new byte[input.available()];
            input.read(buffer);
            input.close();
        } catch (IOException ioe) {
            try {
                if (input != null) {
                    input.close();
                }
            } catch (Exception e) {
                Log.e(TAG, "Close file failed, \n" + e.toString());
            }
            Log.e(TAG, DECODE_ERROR + ", see log:" + ioe.toString());
            return DECODE_ERROR;
        }

        String result = EncodingUtils.getString(buffer, "UTF-8");
        if (result != null) {
            return result;
        } else {
            Log.e(TAG, DECODE_ERROR + ", result == null.");
            return DECODE_ERROR;
        }
    }

    public static int runSlogCommand(int command) {
        return runSlogCommand(command, null);
    }

    /**
     * Run the command under java runtime. This a time casting function, put
     * this in thread better.<br>
     * 
     * @param command The command start with <b>SLOG_COMMAND_</b> in
     *            {@code SlogAction.Contracts}
     * @return The result of the command
     */
    private static int runSlogCommand(int command, String addition) {
        java.util.regex.Pattern pattern = java.util.regex.Pattern
                .compile("[0-9a-zA-Z-]*");
        if (addition != null && !pattern.matcher(addition).matches()) {
            return SLOG_COMMAND_RETURN_EXCEPTION;
        }

        String commandString = resolveCommandToString(command)
                + (addition == null ? "" : addition);
        Log.i(TAG, "runSlogCommand command=" + commandString
                + " ,and additional is " + addition);
        try {
            Process proc = Runtime.getRuntime().exec(commandString);
            proc.waitFor();
            Log.d(TAG, decodeInputStream(proc.getInputStream()));
            /*if (commandString.contains(SLOG_COMMAND_DUMP)) {
                Thread.sleep(500);
                runSlogCommand(Contracts.SLOG_COMMAND_CLEAR);
            }*/
            Log.i(TAG, "after proc.waitFor proc.exitValue = " + proc.exitValue());
            return proc.exitValue();
        } catch (IOException ioException) {
            Log.e(TAG, "Catch IOException.\n", ioException.getCause());
            return SLOG_COMMAND_RETURN_EXCEPTION;
        } catch (InterruptedException interruptException) {
            Log.e(TAG, "Catch InterruptedException.\n", interruptException.getCause());
            return SLOG_COMMAND_RETURN_EXCEPTION;
        } catch (Exception other) {
            Log.e(TAG, "Catch Exception.\n", other.getCause());
            return SLOG_COMMAND_RETURN_EXCEPTION;
        }
    }
    private static final void assumeAndEnsureDaemonAlive(String keyName, String status) {
        if (keyName == null || status == null) return;
        if (GENERALKEY.equals(keyName)) {
            if (GENERALON.equals(status) || GENERALLOWPOWER.equals(status)) {
                startSlogDaemon();
            }else if (GENERALOFF.equals(status)) {
                stopSlogDaemon();
            }
        }
    }
    /** Start the slog daemon, and then slog can work. */
    private static final void startSlogDaemon() {
     // If the daemon started, no need to set it again.
        if (SystemProperties.getBoolean("persist.sys.slog.enabled", false)) return;
        Log.v("duke", "start daemon now");
        new File(SLOG_CONFIG_PATH).mkdir();
        SystemProperties.set("persist.sys.slog.enabled", "1");
    }

    /** Stop the slog daemon, all related trasactions will hangup and failed. */
    private static final void stopSlogDaemon() {
        if (!SystemProperties.getBoolean("persist.sys.slog.enabled", false)) return;
        SystemProperties.set("persist.sys.slog.enabled", "0");
    }
    private static int sReloadTryTime = 0;

    /**
     * Remove slog.conf, and reload the cache
     */
    public static void resetSlogConfig(SlogConfListener l) {
        if (sReloadTryTime++ > 5) {
            return;
        }
        startSlogDaemon();
        runSlogCommand(Contracts.SLOG_COMMAND_RESET);
        runSlogCommand(Contracts.SLOG_COMMAND_RELOAD);
        try {
            Thread.sleep(1000);
        } catch (Exception e) {
            Log.e(TAG, "Can't sleep when reset slog.conf");
        }
        reloadCache(l);
        sReloadTryTime = 0;
    }

    public static boolean isCacheInvalid() {
        // Log.d(TAG, "isCacheInvalid cache is " + mCache);
        // Log.d(TAG, "isCacheInvalid !mCache.contains(GENERALKEY) "
        // +!mCache.contains(GENERALKEY));
        // Log.d(TAG, "isCacheInvalid mCache.contains(STORAGEKEY) " +
        // !mCache.contains(STORAGEKEY));
        return mCache == null || "".equals(mCache) || mCache.length() < 2
                || !mCache.contains(GENERALKEY) || !mCache.contains(STORAGEKEY);
    }

    public static boolean reloadCacheIfInvalid(SlogConfListener l) {
        boolean invalid = isCacheInvalid();
        if (invalid) {
            reloadCache(l);
        } else {
            if (l != null) {
                l.onLoadFinished();
            }
        }
        return invalid;
    }

    /**
     * Reload slog.conf into cache
     */
    public static synchronized void reloadCache(final SlogConfListener l) {
        // Log.d(TAG, "reload cache");
        Log.d(TAG, "reload cache , last cache=" + mCache);
        // Thread.dumpStack();
        sHandler.post(new Runnable() {

            @Override
            public void run() {
                try {
                    mCache = "";
                    FileInputStream freader = new FileInputStream(SLOG_CONF_LOCATION);
                    mCache = decodeInputStream(freader);
                    // Log.d(TAG, "reload cache complete, mCache = " + mCache);
                    if (isCacheInvalid()) {
                        Log.e(TAG, "The size is too small, reset slog.conf");
                        return;
                    }
                    if (!getState(GENERALKEY)) stopSlogDaemon();
                    if (l != null) {
                        l.onLoadFinished();
                    }
                    sReloadTryTime = 0;
                } catch (Exception e) {
                    e.printStackTrace();
                    Log.e(TAG, "Reading cache catch exception", e.getCause());
                }
            }
        });

    }

    private static Runnable sReloadRunnable = new Runnable() {
        @Override
        public void run() {
            sReloadTryTime = 0;
            assumeAndEnsureDaemonAlive(GENERALKEY, getState(GENERALKEY, true));
            runSlogCommand(Contracts.SLOG_COMMAND_RELOAD);
        }
    };

    public static void reload() {
        sHandler.removeCallbacks(sReloadRunnable);
        sHandler.postDelayed(sReloadRunnable, 1000);
    }

    /**
     * Reload GetState Function, get states and compared, finally return the
     * result. PS:SDCard isChecked =>true
     **/
    public static boolean getState(String keyName) {
        // null pointer handler
        if (keyName == null) {
            Log.e("slog",
                    "You have give a null to GetState():boolean,please check");
            return false;
        }

        try {
            if (keyName.equals(GENERALKEY)
                    && getState(keyName, true).equals(GENERALON)) {
                return true;
            }
            if (keyName.equals(STORAGEKEY)
                    && getState(keyName, true).equals(STORAGESDCARD)) {
                return true;
            }
            if (keyName.equals(SAVEALLKEY)
                    && getState(keyName, true).equals(ON)) {
                return true;
            } else if (keyName.equals(HWWATCHDOGKEY)
                    && getState(HWWATCHDOGKEY, true).equals(ON)) {
                return true;
            } else if (keyName.equals(SYSDUMPKEY)
                    && getState(SYSDUMPKEY, true).equals(ON)) {
                return true;
            } else if (keyName.equals(COREDUMPKEY)
                    && getState(COREDUMPKEY, true).equals(ON)) {
                return true;
            } else if (keyName.equals(HPROFSKEY)
                    && getState(HPROFSKEY, true).equals(ON)) {
                return true;
            } else if (keyName.equals(SPRDDEBUGKEY)
                    && getState(SPRDDEBUGKEY, true).equals(ON)) {
                return true;
            } else if (getState(keyName, false).equals(ON)) {

                return true;
            } else {
                return false;
            }
        } catch (NullPointerException nullPointer) {
            Log.e("GetState",
                    "Maybe you change GetState(),but don't return null.Log:\n"
                            + nullPointer);
            return false;
        }
    }

    /** Reload GetState function, for other condition **/
    public static boolean getState(int otherCase) {
        // TODO if you have another Conditions, please use it,add the code under
        // switch with case
        // !
        try {
            switch (otherCase) {
                case ANDROIDKEY:
                    if (getState(KERNELKEY, false).equals(ON))
                        return true;
                    else if (getState(SYSTEMKEY, false).equals(ON))
                        return true;
                    else if (getState(RADIOKEY, false).equals(ON))
                        return true;
                    else if (getState(MAINKEY, false).equals(ON))
                        return true;
                    else if (isKeyValid(EVENTKEY) && getState(EVENTKEY, false).equals(ON))
                        return true;
                    break;
                case MODEMLOGKEY:
                    if (CP0_ENABLE && getState(CP0KEY, false).equals(ON))
                        return true;
                    else if (CP1_ENABLE && getState(CP1KEY, false).equals(ON))
                        return true;
                    else if (CP2_ENABLE && getState(CP2KEY, false).equals(ON))
                        return true;
                    else if (CP3_ENABLE && getState(CP3KEY, false).equals(ON))
                        return true;
                    else if (CP4_ENABLE && getState(CP4KEY, false).equals(ON))
                        return true;
                    else if (CP5_ENABLE && getState(CP5KEY, false).equals(ON))
                        return true;
                    break;
                default:
                    Log.e("GetState(int)", "You have given a invalid case");
                    break;
            }
        } catch (NullPointerException nullPointer) {
            Log.e("GetState(int)",
                    "Maybe you change GetState(),but don't return null.Log:\n"
                            + nullPointer);
            return false;
        }
        return false;
    }

    /**
     * final function
     * 
     * @param keyName
     * @param isLastOption
     * @return
     */
    public static String getState(String keyName, boolean isLastOption) {
        if (keyName == null) {
            Log.e(TAG, "keyName is null, return");
            return DECODE_ERROR;
        }
        if (mCache == null) {
            Log.e(TAG, "mCache is null");
            if (keyName.equals(GENERALKEY)) {
                return GENERALOFF;
            } else if (keyName.equals(STORAGEKEY)) {
                return STORAGENAND;
            } else {
                return OFF;
            }
        }

        if (!keyName.equals(GENERALKEY) || !keyName.equals(STORAGEKEY)) {
            if (!isKeyValid(keyName)) {
                Log.w(TAG, "no such key named " + keyName);
                return DECODE_ERROR;
            }
        }
        String result;
        try {
            synchronized (mLock) {
                int start = -1;
                int end = -1;
                if (keyName.equals(GENERALKEY)) {
                    if (mCache.contains("\n" + GENERALON + "\n")) {
                        return GENERALON;
                    } else if (mCache.contains("\n" + GENERALOFF + "\n")) {
                        return GENERALOFF;
                    } else if (mCache.contains("\n" + GENERALLOWPOWER + "\n")) {
                        return GENERALLOWPOWER;
                    } else {
                        return DECODE_ERROR;
                    }
                }
                start = mCache.indexOf(keyName) + keyName.length();
                end = mCache.indexOf(isLastOption ? "\n" : "\t",
                        mCache.indexOf(keyName) + keyName.length() + 1);
                if (start < 0 || end < 0) {
                    return DECODE_ERROR;
                }
                result = mCache.substring(start, end);

            }
        } catch (Exception e) {
            Log.d(TAG, "Catch exception");
            e.printStackTrace();
            return DECODE_ERROR;
        }
        //Log.d(TAG, "get result is " + result.trim());
        Log.d(TAG, keyName + " get result is " + result.trim());
        return result.trim();
    }

    public static Bundle getAllStates() {
        // TODO UNDERCONSTRUCTION
        return null;
    }

    @Deprecated
    /**
     * UnderConstruction now.
     * @param keyName the key name of the option.
     */
    public static void getOption(String keyName) {
        int maxLength = (keyName.equals(GENERALKEY) || keyName
                .equals(STORAGEKEY)) ? 1 : LENGTHOPTIONSTREAM;
        String[] Options = new String[maxLength];
        char[] result = new char[50];

        int counter = mCache.indexOf(keyName);
        int jump = keyName.length();

        for (int i = 0; i < maxLength;) {
            mCache.getChars(
                    counter + jump,
                    mCache.indexOf(i == maxLength - 1 ? "\n" : "\t", counter
                            + jump + 1), result, 0);

            System.out.println("i=" + i + " counter=" + counter + "jump="
                    + jump + "result=" + String.valueOf(result).trim());
            Options[i] = String.valueOf(result).trim();
            counter += jump;
            jump = Options[i++].length() + 1;
        }
    }

    public static int getLevel(String keyName){

        if (keyName.equals(MAINKEY)) {
            int searchCursor = mCache.indexOf(keyName);
            if (searchCursor < 0) {
                Log.e(TAG, "start index < 0, reset slog.conf");
                resetSlogConfig(null);
                return -1;
            }

            String subString = mCache.substring(searchCursor);
            String lineString = subString.substring(0, subString.indexOf("\n"));
            String[] keyValues = lineString.split("\t");
            return Integer.valueOf(keyValues[4]);
        }
        return -1;
    }

    public static void setLevel(String keyName, int level) {
        if (keyName.equals(MAINKEY)) {
            int searchCursor = mCache.indexOf(keyName);
            if (searchCursor < 0) {
                Log.e(TAG, "start index < 0, reset slog.conf");
                resetSlogConfig(null);
                return;
            }
            String subString = mCache.substring(searchCursor);
            String lineString = subString.substring(0, subString.indexOf("\n"));
            String[] keyValues = lineString.split("\t");
            String valueChangeTo = keyValues[0] + "\t" + keyValues[1] + "\t" + keyValues[2] + "\t"
                    + keyValues[3] + "\t" + level;

            Log.d(TAG, "[yao]change " + lineString + " to " + valueChangeTo);
            String tmp = mCache.replace(lineString, valueChangeTo);
            mCache = tmp;
            sDirty = true;
        }
    }

    public static void setState(String keyName, boolean status) {
        if (keyName == null || "".equals(keyName)) {
            return;
        }
        if (keyName.equals(GENERALKEY)) {
            setState(GENERALKEY, status, true);
            return;
        }
        if (keyName.equals(LOGOVRE)) {
            setState(LOGOVRE, status, true);
            return;
        }
        if (keyName.equals(STORAGEKEY)) {
            setState(STORAGEKEY, status, true);
            return;
        }
        if (keyName.equals(SAVEALLKEY)) {
            setState(SAVEALLKEY, status, true);
            return;
        }
        setState(keyName, status, false);
    }

    public static void setState(String keyName, boolean status,
            boolean isLastOption) {
        if (keyName == null) {
            Log.e("SetState(String,boolean,boolean):void",
                    "Do NOT give me null");
            return;
        }
        if (keyName.equals(GENERALKEY)) {
            if (status) {
                setState(keyName, GENERALON, GENERALOFF, true);
            } else {
                setState(keyName, GENERALOFF, GENERALON, true);
            }
        } else if (keyName.equals(LOGOVRE)) {
            if (status) {
                setState(keyName, GENERALON, GENERALOFF, true);
            } else {
                setState(keyName, GENERALOFF, GENERALON, true);
            }
        } else if (keyName.equals(STORAGEKEY)) {
            if (status) {
                setState(keyName, STORAGESDCARD, STORAGENAND, true);
            } else {
                setState(keyName, STORAGENAND, STORAGESDCARD, true);
            }
        } else {
            setState(keyName, status ? ON : OFF, status ? OFF : ON,
                    isLastOption);
        }
    }

    /** handle other case **/
    public static void setState(int otherCase, boolean status) {
        // TODO if you have otherCondition, please add the code under switch
        // with case
        switch (otherCase) {
            case ANDROIDKEY:
                setState(SYSTEMKEY, status, false);
                setState(KERNELKEY, status, false);
                setState(RADIOKEY, status, false);
                setState(MAINKEY, status, false);
                setState(EVENTKEY, status, false);
                if (BLUETHOOTH_ENABLE && status) {
                    setState(SlogAction.BLUETOOTHKEY, status, false);
                }
                if (CAP_ENABLE && status) {
                    setState(SlogAction.TCPKEY, status, false);
                }
                break;
            default:
                Log.w("SetState(int,boolean)", "You have given a invalid case");
        }
    }

    /**
     * @param keyName
     * @param status
     * @param isLastOption
     */
    public static synchronized void setState(String keyName, String status,
            String lastStatus, boolean isLastOption) {
        Log.d(TAG, "setState finally");
        if (keyName == null) {
            Log.e(TAG, "Do NOT give keyName null");
            return;
        }
        if (status == null) {
            Log.e(TAG, "Do NOT give status null");
            return;
        }
        if (!keyName.equals(GENERALKEY) || !keyName.equals(STORAGEKEY)) {
            if (!isKeyValid(keyName)) {
                Log.w(TAG, "no such key named " + keyName);
                return;
            }
        }
        try {
            synchronized (mLock) {
                int searchCursor = mCache.indexOf(keyName);
                if (searchCursor < 0) {
                    Log.e(TAG, "start index < 0, reset slog.conf");
                    resetSlogConfig(null);
                    return;
                }

                /*Log.d(TAG,
                        "Contains" + (keyName + lastStatus) + " ?"
                                + mCache.contains(keyName + lastStatus));*/
                String tmp = mCache.replace(keyName + lastStatus, keyName
                        + status);
                mCache = tmp;
                sDirty = true;
                // Log.d(TAG, "tmp is \n" + tmp);
            }
        } catch (Exception e) {
            e.printStackTrace();
            Log.e(TAG, "Catch Excepton,log:\n", e.getCause());
            return;
        }
    }

    public static final boolean isKeyValid(String key) {
        if (mCache != null && !"".equals(mCache) && mCache.contains(key)) {
            return true;
        } else {
            return false;
        }
    }

    public static synchronized void writeSlogConfig() {
        sHandler.post(new Runnable() {
            @Override
            public void run() {
                byte[] buffer = null;
                FileOutputStream fwriter = null;
                if (!sDirty) {
                    return;
                }
                try {
                    fwriter = new FileOutputStream(SLOG_CONF_LOCATION);

                    buffer = mCache.toString().getBytes("UTF-8");
                    fwriter.write(buffer);
                    fwriter.close();
                } catch (Exception e) {
                    try {
                        if (fwriter != null) {
                            fwriter.close();
                        }
                    } catch (IOException e1) {
                        e1.printStackTrace();
                    }
                    Log.e(TAG, "Writing file failed,now close,log:\n", e.getCause());
                    return;
                }
                reload();
                //sendMsgToSlogModem("slogctl reload");
                sDirty = false;
            }
        });
    }

    /**
     * Screenshot
     */
    public static void snap(final Context context) {
        if (context == null) {
            return;
        }

        TimeoutRunnable tr = new TimeoutRunnable(context);
        ScreenshotRunnable sr = new ScreenshotRunnable(context, tr);
        sHandler.postDelayed(sr, 100);
        sTimeoutHandler.postDelayed(tr, 10000);
        return;
    }

    private static void screenShot(Context context, TimeoutRunnable timeout) {
        // Log.d(TAG, "in screenshot, the handler will post the delayed runnable");
        // Log.d(TAG, "the delayed message has been posted");
        try {
            if (runSlogCommand(Contracts.SLOG_COMMAND_SCREENSHOT) == 0) {
                sTimeoutHandler.removeCallbacks(timeout);
                sHandler.post(new ToastRunnable(context, context
                        .getString(R.string.screenshot_successed)));
            } else {
                sTimeoutHandler.removeCallbacks(timeout);
                sHandler.post(new ToastRunnable(context, context
                        .getString(R.string.screenshot_failed)));
            }
        } catch (Exception e) {
            Log.e(TAG, "screen shot catch exception", e.getCause());
        }
        File screenpath;
        if (StorageUtil.getExternalStorageState()) {
            screenpath = StorageUtil.getExternalStorage();
        } else {
            screenpath = android.os.Environment.getDataDirectory();
        }
        scanScreenShotFile(new File(screenpath.getAbsolutePath()
                + File.separator + "slog"), context);
    }

    private static void scanScreenShotFile(File file, Context context) {
        if (file == null) {
            Log.i(TAG, "scanFailed!");
            return;
        }
        if ("last_log".equals(file.getName())) {
            return;
        }
        if (file.isDirectory()) {
            File[] files = file.listFiles();
            if(files != null){
                for (File f : files) {
                    scanScreenShotFile(f, context);
                }
            }
        }
        if (file.getName().endsWith("jpg")) {
            MediaScannerConnection.scanFile(context,
                    new String[] {
                        file.getAbsolutePath()
                    }, null, null);
        }
    }

    public static long getFreeSpace(IMediaContainerService imcs,
            boolean isExternal) {
        return StorageUtil.getFreespace(
                imcs,
                isExternal ? StorageUtil.getExternalStorage() : Environment
                        .getDataDirectory());
    }

    public static long getTotalSpace(IMediaContainerService imcs,
            boolean isExternal) {
        return StorageUtil.getTotalSpace(
                imcs,
                isExternal ? StorageUtil.getExternalStorage() : Environment
                        .getDataDirectory());
    }

    public static boolean isDumping() {
        return sDumpLog;
    }

    static class DumpRunnable implements Runnable {
        private String mName;
        private Context mContext;

        public DumpRunnable(Context context, String name) {
            mContext = context.getApplicationContext();
            mName = name;
        }

        @Override
        public void run() {
            SDumpOrClearHandler.sendEmptyMessageDelayed(
                    SlogAction.Contracts.SLOG_COMMAND_DUMP, 100000);
            sDumpLog = true;
            int result = runSlogCommand(SlogAction.Contracts.SLOG_COMMAND_DUMP,
                    mName);
            mContext.sendBroadcast(new Intent(ACTION_DUMPLOG_COMPLETED)
                    .putExtra(
                            EXTRA_DUMP_RESULT,
                            result));
            mContext = null;
            sDumpLog = false;
            SDumpOrClearHandler.removeMessages(SlogAction.Contracts.SLOG_COMMAND_DUMP);
        }
    }

    public static boolean dump(String name, Context context) {
        if (context == null) {
            return true;
        }
        if (SDumpOrClearHandler.hasMessages(SlogAction.Contracts.SLOG_COMMAND_DUMP)
                || SDumpOrClearHandler
                        .hasMessages(SlogAction.Contracts.SLOG_COMMAND_CLEAR)) {
            Toast.makeText(context,
                    context.getString(R.string.dump_clear_action_duplicated),
                    Toast.LENGTH_LONG).show();
            return true;
        } else {
            SDumpOrClearHandler.post(new DumpRunnable(context, name));
            return false;
        }
    }

    static class ClearLogRunnable implements Runnable {
        private Context mContext;

        public ClearLogRunnable(Context context) {
            mContext = context;
        }

        @Override
        public void run() {
            SDumpOrClearHandler.sendEmptyMessageDelayed(
                    SlogAction.Contracts.SLOG_COMMAND_CLEAR, 100000);
            sClearLog = true;
            /* SPRD: Bug 554822 can not delete modem log @{ */
            //int result = runSlogCommand(SlogAction.Contracts.SLOG_COMMAND_CLEAR);
            String strTmp = SocketUtils.sendCmdAndRecResult("slogmodem",
                    LocalSocketAddress.Namespace.ABSTRACT, "slogctl clear\n");
            if (strTmp != null && strTmp.contains(IATUtils.AT_OK)) {
                Log.d(TAG,"slogmodem clear successful");
            }
            mContext.sendBroadcast(new Intent(ACTION_CLEARLOG_COMPLETED)
                    .putExtra(
                            EXTRA_CLEAR_RESULT,
                            -1));
            /* @} */
            sClearLog = false;
            mContext = null;
            SDumpOrClearHandler.removeMessages(SlogAction.Contracts.SLOG_COMMAND_CLEAR);
        }

    }

    public static boolean isClearing() {
        return sClearLog;
    }

    /**
     * run the slog command of clearing log, command name is slogctl clear
     * 
     * @param context the context used to send broadcast
     * @return the enabled of current button
     */
    public static boolean clear(Context context) {
        if (context == null) {
            return true;
        }
        if (SDumpOrClearHandler.hasMessages(SlogAction.Contracts.SLOG_COMMAND_CLEAR)
                || SDumpOrClearHandler.hasMessages(SlogAction.Contracts.SLOG_COMMAND_DUMP)) {
            Toast.makeText(context,
                    context.getString(R.string.dump_clear_action_duplicated),
                    Toast.LENGTH_LONG).show();
            return true;
        } else {
            SDumpOrClearHandler.post(new ClearLogRunnable(context.getApplicationContext()));
            return false;
        }
    }

    /**
     * This function can change InputStream to String, if catching exception,
     * I'll return DECODE_ERROR.
     */
    public static boolean sendATCommand(int atCommandCode, boolean openLog) {
        /*
         * require set AT Control to modem and this may be FIXME 1. Why write
         * byte can catch IOException? 2. Whether Setting AT Command in main
         * thread can cause ANR or not?
         */

        // Feature changed, remove close action of openLog now.
        synchronized (mLock) {
            if (!openLog) {
                return false;
            }

            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            DataOutputStream dos = new DataOutputStream(baos);
            try {
                dos.writeBytes(String.format(Locale.US,"%d,%d,%d", atCommandCode, 1, openLog ? 1 : 0));
            } catch (IOException ioException) {
                Log.e(TAG, "IOException has catched, see logs " + ioException);
                return false;
            }
            // TODO REMOVE IT
            return false;
            /* engfetch em.. is a class! may be a bad coding style */
            //engfetch ef = new engfetch();
            //int sockid = ef.engopen();
            //ef.engwrite(sockid, baos.toByteArray(), baos.toByteArray().length);
            /* Whether engfetch has free function? */

            // Bring from logswitch ...
            //byte[] inputBytes = new byte[512];
            //int showlen = ef.engread(sockid, inputBytes, 512);
            //String result = new String(inputBytes, 0, showlen);
            //ef.engclose(atCommandCode);
            //if ("OK".equals(result)) {
            //    return true;
            //} else if ("Unknown".equals(result)) {
            //    Log.w("SlogUI", "ATCommand has catch a \"Unknow\" command!");
            //    return false;
            //} else {
            //    return false;
            //}
        }
    }

    public static void sendMsgToSlogModem(String msg) {
        Log.d(TAG,"sendMsgToSlogModem" + msg);
        try {
            mClient = new LocalSocket();
            LocalSocketAddress serverAddress = new LocalSocketAddress(
                    "slogmodem", LocalSocketAddress.Namespace.ABSTRACT);
            mClient.connect(serverAddress);
            Log.d(TAG, "client connet is " + mClient.isConnected());
            mOut = mClient.getOutputStream();
            if (mOut != null) {
                final StringBuilder cmdBuilder = new StringBuilder(msg).append('\0');
                final String cmd = cmdBuilder.toString();
                mOut.write(cmd.getBytes(StandardCharsets.UTF_8));
                mOut.flush();
            }
        } catch (Exception e) {
            Log.d(TAG, "client write exception " + e.getMessage());
        } finally {
            try {
                if (mOut != null) {
                    mOut.close();
                }
                if (mClient != null && mClient.isConnected()) {
                    mClient.close();
                }
            } catch (Exception e) {
                Log.d(TAG, "close client exception " + e.getMessage());
            }
        }
    }
}
