package com.sprd.engineermode.utils;
import java.lang.Thread;
public class ThreadUtils {
    private static final String TAG = "ThreadUtils";

    public static void stopThread(Thread thread) {
        Thread tmpThread = thread;
        thread = null;
        if (tmpThread != null) {
            tmpThread.interrupt();
        }
    }
}
