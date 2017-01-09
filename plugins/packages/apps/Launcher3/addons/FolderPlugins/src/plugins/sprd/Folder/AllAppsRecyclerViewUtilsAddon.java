package plugins.sprd.Folder;

import com.android.launcher3.Folderplugins.AllAppsRecyclerViewUtils;
import com.android.launcher3.allapps.AllAppsGridAdapter;

import android.app.AddonManager;
import android.content.Context;

public class AllAppsRecyclerViewUtilsAddon extends AllAppsRecyclerViewUtils implements AddonManager.InitialCallback {
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    @Override
    public boolean isViewTypeAvaliable(int viewtype) {
        return viewtype == AllAppsGridAdapter.ICON_VIEW_TYPE
                || viewtype == AllAppsGridAdapter.PREDICTION_ICON_VIEW_TYPE
                || viewtype == AllAppsGridAdapter.FOLDER_VIEW_TYPE;
    }
}