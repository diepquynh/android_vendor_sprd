package plugin.sprd.contactsegmentsearchsupport;

import android.app.AddonManager;
import android.content.Context;
import com.android.providers.contacts.DataRowHandler;
import com.android.providers.contacts.ContactsDatabaseHelper.PhoneLookupColumns;
import com.android.providers.contacts.SearchIndexManager.IndexBuilder;
import com.sprd.providers.plugin.SegmentSearchSupport;
import android.util.Log;

public class AddonSegmentSearchSupport extends SegmentSearchSupport implements AddonManager.InitialCallback {

    private static final String TAG = "SegmentSearchSupportAddon";
    private Context mAddonContext;
    public static final int MAX_SEARCH_SNIPPET_SIZE = 50;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void appendTokenForSegmentSearch(String normalizedNumber, IndexBuilder builder) {
        Log.d(TAG, "appendTokenForSegmentSearch normalizedNumber: ");
        int N = normalizedNumber.length();
        for (int i = 1; i < N; ++i) {
            builder.appendToken(normalizedNumber.substring(i,
                    Math.min(i + MAX_SEARCH_SNIPPET_SIZE, N)));
        }
    }
/**
 * SPRD: Bug628490 remove "ro.operator" to add plugins for SegmentSearchSupport feature
 * {@
 * */
//    @Override
//    public boolean isSegmentSearchSupport() {
//        // SPRD: Bug608407 CTS case testSearchSnippets_MultipleMatchesCorrectSnippet fails.
//        // As the snip search is not requiered by CMCC and it can cause the cts test fail, so
//        // disable the feature.
//        return false;
//    }

    @Override
    public void appendSearchJoin(StringBuilder sb) {
        sb.append(" AND " + PhoneLookupColumns.NORMALIZED_NUMBER + " LIKE '%");
    }
    /**
     * @}
     * */
}
