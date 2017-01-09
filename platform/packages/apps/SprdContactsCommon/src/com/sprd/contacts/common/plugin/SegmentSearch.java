package com.sprd.contacts.common.plugin;

import android.app.AddonManager;
import android.content.Context;
import com.android.contacts.common.R;
import com.android.contacts.common.util.SearchUtil;

public class SegmentSearch {
    private static SegmentSearch sInstance;
    public static SegmentSearch getInstance(Context context) {
        if (sInstance != null) {
            return sInstance;
        }
        //SPRD:Bug542773 Can't create new contact in guest mode
        //according to context to get instance to avoid resource can't found
        sInstance = (SegmentSearch) new AddonManager(context).getAddon(
                R.string.feature_segment_search, SegmentSearch.class);
        return sInstance;
    }

    public int getIndex(String contents, String substring) {
        return SearchUtil.contains(contents, substring);
    }

}
