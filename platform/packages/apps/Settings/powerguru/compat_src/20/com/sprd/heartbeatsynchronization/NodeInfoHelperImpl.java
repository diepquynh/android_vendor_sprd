/*
 * The Spreadtrum Communication Inc.
 */

package com.sprd.heartbeatsynchronization;

import android.content.Context;
import android.os.ServiceManager;
import android.util.Log;

import com.android.internal.app.IUsageStats;
import com.android.internal.os.PkgUsageStats;

import java.util.Calendar;
import java.util.List;
import java.util.HashMap;

public class NodeInfoHelperImpl  extends NodeInfoHelper {

    // Use map for better query performance
    private HashMap<String, PkgUsageStats> mUsageMap = new HashMap<String, PkgUsageStats>();
    private IUsageStats mUsageStatsManager;

    NodeInfoHelperImpl(Context context) {
        super(context);
        mUsageStatsManager = IUsageStats.Stub.asInterface(ServiceManager.getService("usagestats"));
    }

    @Override
    public void updateInfo() {
        PkgUsageStats[] list = null;
        try {
            list = mUsageStatsManager.getAllPkgUsageStats();
        } catch (android.os.RemoteException e) {
            Log.e(TAG, "Caught remote exception, usage update failed!", e);
            return;
        }

        // Cleanup items
        mUsageMap.clear();
        // Update to map
        for (PkgUsageStats stats : list) {
            if (DEBUG) Log.d(TAG, "Adding stats to map key<"
                    + stats.packageName + "> value<" + stats + ">");
            mUsageMap.put(stats.packageName, stats);
        }
    }

    @Override
    public void attachInfo(MainActivity.OneApp outInfo) {
        Log.d(TAG, "attaching info outInfo.resolveInfo.resolvePackageName=" + outInfo.resolveInfo.activityInfo.packageName);
        PkgUsageStats stats = mUsageMap.get(outInfo.resolveInfo.activityInfo.packageName);
        if (stats != null) outInfo.count = stats.launchCount;
    }
}
