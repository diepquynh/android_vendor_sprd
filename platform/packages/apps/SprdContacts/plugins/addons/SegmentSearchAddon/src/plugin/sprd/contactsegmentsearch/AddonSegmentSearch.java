package plugin.sprd.contactsegmentsearch;

import android.app.AddonManager;
import android.content.Context;
import com.sprd.contacts.common.plugin.SegmentSearch;
import android.util.Log;


public class AddonSegmentSearch extends SegmentSearch implements AddonManager.InitialCallback {

    private static final String TAG = "SegmentSearchAddon";
    private Context mAddonContext;
    public static final int MAX_SEARCH_SNIPPET_SIZE = 50;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public int getIndex(String contents, String substring) {
        return contents.indexOf(substring);
    }
}
