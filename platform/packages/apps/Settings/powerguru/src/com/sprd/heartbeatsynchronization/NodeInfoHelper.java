/*
 *  Spreadtrum Communication Inc.
 */

package com.sprd.heartbeatsynchronization;

import android.app.ActivityThread;
import android.content.Context;
import android.util.Log;

public class NodeInfoHelper {
    protected static final String TAG = "NodeInfoHelper";
    protected static final boolean DEBUG = false;
    // Container to hold different implements. The implements
    // is controled by build system.
    static NodeInfoHelper sInstance;

    /**
     * Create a singleleton by application context.
     *
     * @param context will call getApplicationContext to ensure no memory leaks
     */
    public static void createInstance(Context context) {
        if (sInstance != null)
            return;

        // Lazy initialize
        synchronized (NodeInfoHelper.class) {
            if (sInstance != null)
                return;
            sInstance = new NodeInfoHelperImpl(context);
        }
    }

    /**
     * Get the instance of Helper, before this, you SHOULD call createInstance.
     */
    public static NodeInfoHelper getInstance() {
        if (sInstance == null) {
            Log.w(TAG, "It is a bad action, has createInstance been called?");
            createInstance(ActivityThread.currentApplication());
        }
        return sInstance;
    }

    protected Context mContext;

    NodeInfoHelper(Context context) {
        mContext = context;
    }

    /**
     * To prepare the launch time info map and store it to local,
     */
    public void updateInfo() {
    }

    /**
     * Attach the node infos here.
     */
    public void attachInfo(MainActivity.OneApp outInfo) {
    }
}
