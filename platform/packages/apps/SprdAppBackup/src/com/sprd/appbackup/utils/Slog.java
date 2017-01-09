package com.sprd.appbackup.utils;

import android.os.Debug;
import android.util.Log;

/**
 * An updated tools from {@link Log}
 * 
 */
public final class Slog {
    private static final boolean DBG = true;
    private static final boolean NEEDINFO = true;
    private static final String TAG = "Slog";
    private static final String DEFAULT_MESSAGE = "The program has run here successfully, dumping Stack:";
    private static final String EMPTY_MESSAGE = "null";

    // We must stop the tracing after start it. If not, the tracing file will be
    // invalid.
    private static boolean mStartTracing = false;

    public static void i() {
        Log.i(TAG, DEFAULT_MESSAGE);
        Thread.dumpStack();
    }

    /**
     * Using this as Log.i
     * 
     * @param message
     *            , The message you want to show in Log.
     */
    public static void i(String message) {
        Slog.i(TAG, message);
    }

    public static void i(String tag, String message) {
        if (tag == null) {
            tag = TAG;
        }
        if (message == null) {
            message = EMPTY_MESSAGE;
        }
        if (!NEEDINFO)
            return;
        Log.i(tag, message);
    }

    public static void d() {
        Slog.d(TAG, DEFAULT_MESSAGE);
        Thread.dumpStack();
    }

    public static void d(String message) {
        Slog.d(TAG, message);
    }

    public static void d(String tag, String message) {
        if (tag == null) {
            tag = TAG;
        }
        if (message == null) {
            message = EMPTY_MESSAGE;
        }
        if (!DBG)
            return;
        Log.d(tag, message);
    }

    public static void e(String msg) {
        Log.e(TAG, msg);
    }

    public static void e(String tag, String msg) {
        Log.e(tag, msg);
    }

    public static void startTracing() {
        if (DBG) {
            Debug.startMethodTracing("/data/local/tmp/recentTracing");
            mStartTracing = true;
        }
    }

    public static void startTracing(String nameWithPath) {
        Debug.startMethodTracing(nameWithPath);
        mStartTracing = true;
    }

    public static void stopTracing() {
        if (mStartTracing && DBG) {
            Debug.stopMethodTracing();
            mStartTracing = false;
        }
    }
}
