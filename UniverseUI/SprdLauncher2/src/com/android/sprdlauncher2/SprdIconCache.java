package com.android.sprdlauncher2;

import java.util.HashMap;

import com.android.sprdlauncher2.IconCache.CacheEntry;

import android.content.ComponentName;
import android.content.Context;
import android.content.pm.ResolveInfo;
import android.graphics.drawable.Drawable;

/**
 *  SPRD: UUI theme: system icon @{
 */
public class SprdIconCache extends IconCache {
    String[] mSysIcons;

    public SprdIconCache(Context context) {
        super(context);
        mSysIcons = context.getResources().getStringArray(R.array.sysicons);
    }

    private int sysIndexOf(String classname) {
        int index = -1;
        for (int i = 0; i < mSysIcons.length; i++) {
            if (mSysIcons[i].equals(classname)) {
                index = i;
                break;
            }
        }
        return index;
    }
}
/** @} */
