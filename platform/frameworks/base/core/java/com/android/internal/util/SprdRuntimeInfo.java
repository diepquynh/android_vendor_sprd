/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.android.internal.util;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

import android.util.Log;
import android.util.SparseArray;

/**
 * @hide
 */
public class SprdRuntimeInfo {

    public static final int SPRD_CPU_INFO = 1;
    /**
     * @deprecated now we don't need print memory info
     */
    public static final int SPRD_MEM_INFO = 1 << 1;
    public static final int SPRD_SCHED_INFO = 1 << 2;

    private static final String TAG = "SprdRuntimeInfo";

    private static SparseArray<String> sFilePathConfig;

    static {
        sFilePathConfig = new SparseArray<String>(5);
        sFilePathConfig.put(SPRD_CPU_INFO, "/sys/kernel/debug/sprd_debug/cpu/cpu_usage");
        sFilePathConfig.put(SPRD_SCHED_INFO, "/sys/kernel/debug/sprd_debug/sche/schedlog");
    }

    private static void readFile(String filePath) {
        BufferedReader fr = null;
        try {
            fr = new BufferedReader(new FileReader(filePath));
            String line;
            while ((line = fr.readLine()) != null) {
                Log.d(TAG, line);
            }
        } catch (Exception e) {
            Log.w(TAG , "print sprd runtime info caughed exception", e);
        } finally {
            try {
                if (fr != null)
                    fr.close();
            } catch (IOException e) {
                Log.w(TAG, "fr close exception");
            }
        }
    }

    public static  void printSprdRuntimeInfo(int flag) {
        if((flag & SPRD_CPU_INFO) != 0) {
            Log.d(TAG , "------------- start print sprd cpu info -------------");
            readFile(sFilePathConfig.get(SPRD_CPU_INFO));
            Log.d(TAG , "------------- end print sprd cpu info -------------");
        }
        if((flag & SPRD_SCHED_INFO) != 0) {
            Log.d(TAG , "------------- start print sprd sched info -------------");
            readFile(sFilePathConfig.get(SPRD_SCHED_INFO));
            Log.d(TAG , "------------- end print sprd sched info -------------");
        }
    }
}
