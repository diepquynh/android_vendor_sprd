/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.sprdlauncher2;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;

import java.io.*;
import java.util.ArrayList;

public class Stats {
    private static final boolean DEBUG_BROADCASTS = false;
    private static final String TAG = "Launcher3/Stats";

    /* SPRD: bug338045 2014-07-29 not record log when start apps for performance. @{ */
    public static final boolean ENABLE_STATS = false;
    private static final boolean LOCAL_LAUNCH_LOG = false;
    /* SPRD: bug338045 2014-07-29 not record log when start apps for performance. @} */

    public static final String ACTION_LAUNCH = "com.android.sprdlauncher2.action.LAUNCH";
    public static final String PERM_LAUNCH = "com.android.sprdlauncher2.permission.RECEIVE_LAUNCH_BROADCASTS";
    public static final String EXTRA_INTENT = "intent";
    public static final String EXTRA_CONTAINER = "container";
    public static final String EXTRA_SCREEN = "screen";
    public static final String EXTRA_CELLX = "cellX";
    public static final String EXTRA_CELLY = "cellY";

    private static final String LOG_FILE_NAME = "launches.log";
    private static final int LOG_VERSION = 1;
    private static final int LOG_TAG_VERSION = 0x1;
    private static final int LOG_TAG_LAUNCH = 0x1000;

    private static final String STATS_FILE_NAME = "stats.log";
    private static final int STATS_VERSION = 1;
    private static final int INITIAL_STATS_SIZE = 100;

    /* SPRD: bug338045 2014-07-29 not record log when start apps for performance. @{ */
    // TODO: delayed/batched writes
    private static final boolean FLUSH_IMMEDIATELY = false;
    /* SPRD: bug338045 2014-07-29 not record log when start apps for performance. @} */

    private final Launcher mLauncher;

    /* SPRD: Fix bug 284094 @{
    DataOutputStream mLog;
    @} */

    ArrayList<String> mIntents;
    ArrayList<Integer> mHistogram;

    /* SPRD: Fix bug 280630 @{ */
    private boolean mStatsLoaded = false;
    /* @} */

    public Stats(Launcher launcher) {
        mLauncher = launcher;

        /* SPRD: Fix bug 280630 @{ */
        new Thread(new Runnable() {
            @Override
            public void run() {
                loadStats();
                /* SPRD: Fix bug 284094 @{ */
                if (LOCAL_LAUNCH_LOG) {
                    DataOutputStream mLog = null;
                    try {
                        mLog = new DataOutputStream(mLauncher.openFileOutput(LOG_FILE_NAME, Context.MODE_APPEND));
                        mLog.writeInt(LOG_TAG_VERSION);
                        mLog.writeInt(LOG_VERSION);
                        mLog.close();
                        mLog = null;
                    } catch (FileNotFoundException e) {
                        Log.e(TAG, "unable to create stats log: " + e);
                    } catch (IOException e) {
                        Log.e(TAG, "unable to write to stats log: " + e);
                    } finally {
                        if (mLog != null) {
                            try {
                                mLog.close();
                                mLog = null;
                            } catch (IOException e) { }
                        }
                    }
                }
                /* @} */
            }
        }).start();
        /* @} */

        if (DEBUG_BROADCASTS) {
            launcher.registerReceiver(
                    new BroadcastReceiver() {
                        @Override
                        public void onReceive(Context context, Intent intent) {
                            android.util.Log.v("Stats", "got broadcast: " + intent + " for launched intent: "
                                    + intent.getStringExtra(EXTRA_INTENT));
                        }
                    },
                    new IntentFilter(ACTION_LAUNCH),
                    PERM_LAUNCH,
                    null
            );
        }
    }

    public void incrementLaunch(String intentStr) {
        int pos = mIntents.indexOf(intentStr);
        if (pos < 0) {
            mIntents.add(intentStr);
            mHistogram.add(1);
        } else {
            mHistogram.set(pos, mHistogram.get(pos) + 1);
        }
    }

    public void recordLaunch(Intent intent) {
        recordLaunch(intent, null);
    }

    /* SPRD: Fix bug 280630 @{ */
    public void recordLaunch(Intent intent, final ShortcutInfo shortcut) {
        intent = new Intent(intent);
        intent.setSourceBounds(null);

        final String flat = intent.toUri(0);

        Intent broadcastIntent = new Intent(ACTION_LAUNCH).putExtra(EXTRA_INTENT, flat);
        if (shortcut != null) {
            broadcastIntent.putExtra(EXTRA_CONTAINER, shortcut.container)
                    .putExtra(EXTRA_SCREEN, shortcut.screenId)
                    .putExtra(EXTRA_CELLX, shortcut.cellX)
                    .putExtra(EXTRA_CELLY, shortcut.cellY);
        }
        mLauncher.sendBroadcast(broadcastIntent, PERM_LAUNCH);

        if (mStatsLoaded) incrementLaunch(flat);
        /* SPRD: Fix bug 284094 @{ */
        if (FLUSH_IMMEDIATELY || LOCAL_LAUNCH_LOG) {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    if (FLUSH_IMMEDIATELY && mStatsLoaded) {
                        saveStats();
                    }
                    if (LOCAL_LAUNCH_LOG) {
                        DataOutputStream mLog = null;
                        try {
                            mLog = new DataOutputStream(mLauncher.openFileOutput(LOG_FILE_NAME, Context.MODE_APPEND));
                            mLog.writeInt(LOG_TAG_LAUNCH);
                            mLog.writeLong(System.currentTimeMillis());
                            if (shortcut == null) {
                                mLog.writeShort(0);
                                mLog.writeShort(0);
                                mLog.writeShort(0);
                                mLog.writeShort(0);
                            } else {
                                mLog.writeShort((short) shortcut.container);
                                mLog.writeShort((short) shortcut.screenId);
                                mLog.writeShort((short) shortcut.cellX);
                                mLog.writeShort((short) shortcut.cellY);
                            }
                            mLog.writeUTF(flat);
                            //if (FLUSH_IMMEDIATELY) {
                            //    mLog.flush(); // TODO: delayed writes
                            //}
                            mLog.close();
                            mLog = null;
                        } catch (FileNotFoundException e) {
                            Log.e(TAG, "unable to create stats log: " + e);
                        } catch (IOException e) {
                            e.printStackTrace();
                        } finally {
                            if (mLog != null) {
                                try {
                                    mLog.close();
                                    mLog = null;
                                } catch (IOException e) { }
                            }
                        }
                    }
                }
            }).start();
        }
        /* @} */

    }
    /* @} */

    private void saveStats() {
        DataOutputStream stats = null;
        try {
            stats = new DataOutputStream(mLauncher.openFileOutput(STATS_FILE_NAME + ".tmp", Context.MODE_PRIVATE));
            stats.writeInt(STATS_VERSION);
            final int N = mHistogram.size();
            stats.writeInt(N);
            for (int i=0; i<N; i++) {
                stats.writeUTF(mIntents.get(i));
                stats.writeInt(mHistogram.get(i));
            }
            stats.close();
            stats = null;
            mLauncher.getFileStreamPath(STATS_FILE_NAME + ".tmp")
                     .renameTo(mLauncher.getFileStreamPath(STATS_FILE_NAME));
        } catch (FileNotFoundException e) {
            Log.e(TAG, "unable to create stats data: " + e);
        } catch (IOException e) {
            Log.e(TAG, "unable to write to stats data: " + e);
        } finally {
            if (stats != null) {
                try {
                    stats.close();
                } catch (IOException e) { }
            }
        }
    }

    /* SPRD: Fix bug 280630 @{ */
    private void loadStats() {
        mStatsLoaded = false;
        mIntents = new ArrayList<String>(INITIAL_STATS_SIZE);
        mHistogram = new ArrayList<Integer>(INITIAL_STATS_SIZE);
        DataInputStream stats = null;
        try {
            stats = new DataInputStream(mLauncher.openFileInput(STATS_FILE_NAME));
            final int version = stats.readInt();
            if (version == STATS_VERSION) {
                final int N = stats.readInt();
                for (int i=0; i<N; i++) {
                    final String pkg = stats.readUTF();
                    final int count = stats.readInt();
                    mIntents.add(pkg);
                    mHistogram.add(count);
                }
            }
        } catch (FileNotFoundException e) {
            // not a problem
        } catch (IOException e) {
            // more of a problem

        } finally {
            mStatsLoaded = true;
            if (stats != null) {
                try {
                    stats.close();
                } catch (IOException e) { }
            }
        }
    }
    /* @} */
}
