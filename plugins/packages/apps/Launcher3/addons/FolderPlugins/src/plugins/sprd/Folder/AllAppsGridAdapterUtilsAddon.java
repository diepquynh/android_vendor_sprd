package plugins.sprd.Folder;

import com.android.launcher3.FolderIcon;
import com.android.launcher3.FolderInfo;
import com.android.launcher3.ItemInfo;
import com.android.launcher3.Launcher;
import com.android.launcher3.Folderplugins.AllAppsGridAdapterUtils;
import com.android.launcher3.Folderplugins.AppFolderDbInfo;
import com.android.launcher3.allapps.AllAppsGridAdapter;
import com.android.launcher3.allapps.AlphabeticalAppsList;
import com.android.launcher3.R;
import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;

public class AllAppsGridAdapterUtilsAddon extends AllAppsGridAdapterUtils implements AddonManager.InitialCallback {
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    @Override
    public FolderIcon getFolderIcon(Launcher launcher, ViewGroup parent, View.OnTouchListener touchListener,
            View.OnClickListener iconclicklistener, View.OnLongClickListener iconlongclicklistener) {
        FolderIcon icon = FolderIcon.fromXml(R.layout.all_apps_folder_icon, launcher, parent);
        icon.setOnTouchListener(touchListener);
        icon.setOnClickListener(iconclicklistener);
        icon.setOnLongClickListener(iconlongclicklistener);
        icon.setFocusable(true);
        return icon;
    }

    @Override
    public void bindFolderInfo(AllAppsGridAdapter.ViewHolder viewholer, AlphabeticalAppsList apps, int position) {
        FolderIcon icon = (FolderIcon) viewholer.mContent;
        ItemInfo info = apps.getAdapterItems().get(position).appInfo;
        FolderInfo folderInfo = null;
        if (info instanceof FolderInfo) {
            folderInfo = (FolderInfo) info;
        } else {
            folderInfo = ((AppFolderDbInfo) info).folderInfo;
        }
        icon.bindFolderInfo(folderInfo);
        icon.setFolderEnable(true);
    }
}