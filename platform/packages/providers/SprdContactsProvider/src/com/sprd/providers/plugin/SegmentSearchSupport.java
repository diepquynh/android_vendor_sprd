package com.sprd.providers.plugin;

import com.android.providers.contacts.R;
import com.android.providers.contacts.SearchIndexManager.IndexBuilder;
import android.app.AddonManager;
import android.content.Context;
import com.android.providers.contacts.ContactsDatabaseHelper.PhoneLookupColumns;

public class SegmentSearchSupport {
    private static SegmentSearchSupport sInstance;
    public static SegmentSearchSupport getInstance(Context context) {
        if (sInstance != null) {
            return sInstance;
        }
        //SPRD:Bug542773 Can't create new contact in guest mode
        //according to context to get instance to avoid resource can't found
        sInstance = (SegmentSearchSupport) new AddonManager(context).getAddon(
                R.string.feature_segment_search, SegmentSearchSupport.class);
        return sInstance;
    }

    public void appendTokenForSegmentSearch(String normalizedNumber, IndexBuilder builder) {
    }

    /** SPRD: Bug628490 remove "ro.operator" to add plugins for SegmentSearchSupport feature
    * @{
    */
//    public boolean isSegmentSearchSupport() {
//        return false;
//    }

    public void appendSearchJoin(StringBuilder sb) {
        sb.append(" AND " + PhoneLookupColumns.NORMALIZED_NUMBER + " LIKE '");
    }
    /**
     * @}
     */
}
