/*
 * The Spreadtrum Communication Inc.
 */

package com.sprd.heartbeatsynchronization;

import android.app.usage.UsageStats;
import android.app.usage.UsageStatsManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.util.Log;

import java.util.Calendar;
import java.util.HashMap;
import java.util.List;

public class NodeInfoHelperImpl  extends NodeInfoHelper {

    // Use map for better query performance
    private HashMap<String, UsageStats> mUsageMap = new HashMap<String, UsageStats>();
    private UsageStatsManager mUsageStatsManager;

    NodeInfoHelperImpl(Context context) {
        super(context);
        mUsageStatsManager = (UsageStatsManager) context.getSystemService(Context.USAGE_STATS_SERVICE);
    }

    @Override
    public void updateInfo() {
        // Copy from Settings
        Calendar calendar = Calendar.getInstance();
        calendar.add(Calendar.DAY_OF_YEAR, -5);
        final List<UsageStats> list = mUsageStatsManager.queryUsageStats(UsageStatsManager.INTERVAL_BEST,
                calendar.getTimeInMillis(), System.currentTimeMillis());

        // Cleanup items
        mUsageMap.clear();
        if (list == null || list.isEmpty()) {
            Log.w(TAG, "queryUsageStats is null, it will be failed to get launch time");
        }
        // Update to map
        for (UsageStats stats : list) {
            mUsageMap.put(stats.getPackageName(), stats);
        }
    }

    @Override
    public void attachInfo(MainActivity.OneApp outInfo) {
        UsageStats stats = mUsageMap.get(outInfo.resolveInfo.activityInfo.packageName);
        if (stats != null) outInfo.count = stats.mLaunchCount;
    }
}
