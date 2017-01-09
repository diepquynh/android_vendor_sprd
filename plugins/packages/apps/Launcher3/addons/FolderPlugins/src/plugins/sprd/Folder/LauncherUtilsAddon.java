package plugins.sprd.Folder;

import java.util.ArrayList;

import android.util.Log;
import android.view.View;

import com.android.launcher3.AppInfo;
import com.android.launcher3.BubbleTextView;
import com.android.launcher3.CellLayout;
import com.android.launcher3.Folder;
import com.android.launcher3.FolderIcon;
import com.android.launcher3.FolderInfo;
import com.android.launcher3.IconCache;
import com.android.launcher3.Launcher;
import com.android.launcher3.LauncherCallbacks;
import com.android.launcher3.LauncherModel;
import com.android.launcher3.ShortcutInfo;
import com.android.launcher3.ItemInfo;
import com.android.launcher3.Workspace;
import com.android.launcher3.Folderplugins.LauncherClingsVolte;
import com.android.launcher3.Folderplugins.LauncherUtils;
import com.android.launcher3.allapps.AllAppsContainerView;
import com.android.launcher3.util.LongArrayMap;

import android.app.AddonManager;
import android.content.Context;
import android.view.ViewGroup;

public class LauncherUtilsAddon extends LauncherUtils implements AddonManager.InitialCallback {
    private Context mAddonContext;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public FolderIcon addCustomeFolder(CellLayout layout, long container,
        final long screenId, int cellX, int cellY, FolderInfo info, Workspace mWorkspace,
        IconCache iconcache, boolean isWorkspaceLocked,Launcher launcher,LongArrayMap<FolderInfo> folderarray) {

        final FolderInfo folderInfo = new FolderInfo();
        folderInfo.title = info.title;
        folderInfo.fromAppList = false;

        // Update the model
        LauncherModel.addItemToDatabase(launcher, folderInfo, container,
                       screenId, cellX, cellY);
        folderarray.put(folderInfo.id, folderInfo);

        // Create the view
        FolderIcon newFolder = FolderIcon.fromXml(com.android.launcher3.R.layout.folder_icon, launcher,
                   layout, folderInfo, iconcache);
        mWorkspace.addInScreen(newFolder, container, screenId, cellX, cellY, 1,
                   1, isWorkspaceLocked);
       CellLayout parent = mWorkspace.getParentCellLayoutForView(newFolder);
       parent.getShortcutsAndWidgets().measureChild(newFolder);
       return newFolder;
    }

    @Override
    public void createCustomeShortCut(ItemInfo info, BubbleTextView favorite, IconCache iconcache) {
        if (info instanceof ShortcutInfo) {
            favorite.applyFromShortcutInfo((ShortcutInfo) info, iconcache, true);
        } else if (info instanceof AppInfo) {
            favorite.applyFromApplicationInfo((AppInfo) info);
        }
    }

    @Override
    public void showCustomeFolderCling(FolderInfo info, Launcher launcher) {
        if (!info.fromAppList) {
            if (launcher.shouldShowIntroScreen()) {
                launcher.showIntroScreen();
            } else {
                launcher.showFirstRunActivity();
                launcher.showFirstVolteRunClings();
            }
        }
    }

    @Override
    public boolean isCellLayoutParam(ViewGroup.LayoutParams param) {
        return (param instanceof CellLayout.LayoutParams);
    }

    @Override
    public boolean isFoldeIconFromAppList(FolderInfo info) {
        return info.fromAppList;
    }

    @Override
    public void bindAllApplications2CustomeFolder(AllAppsContainerView appview,
        ArrayList<AppInfo> apps, ArrayList<ItemInfo> items,
        LauncherCallbacks launchercallbacks) {
        if (appview != null) {
            if (items == null) {
                appview.setApps(apps);
            } else {
                appview.setApps2Folder(items);
            }
        }

        if (launchercallbacks != null) {
            launchercallbacks.bindAllApplications(apps);
        }
    }

    @Override
    public void showFirstVolteRunCustomeClings(Launcher launcher) {
        LauncherClingsVolte launcherClingsVolte = new LauncherClingsVolte(launcher);
        if (launcherClingsVolte.shouldShowFirstRunOrMigrationClings()) {
            launcherClingsVolte.showLongPressCling(true);
        }
    }

    @Override
    public void showFolderVisiable(Folder folder, FolderIcon icon, Launcher launcher) {
        folder.setVisibility(View.VISIBLE);
        folder.animateOpen();
    }

    public void closeFolderInAllApp(Launcher launcher,AllAppsContainerView mAppsView) {
        Folder folder = mAppsView.getOpenFolder();
        if (folder != null) {
            launcher.closeFolder();
        }
    }

    public void showCling(Launcher launcher, FolderInfo info) {
        // Folder Launcher Cling.
        if (!info.fromAppList) {
            if (launcher.shouldShowIntroScreen()) {
                launcher.showIntroScreen();
            } else {
                launcher.showFirstRunActivity();
                launcher.showFirstVolteRunClings();
            }
        }
    }

    public boolean isInstanceofLayoutParams(FolderIcon folderIcon) {
        return (folderIcon.getLayoutParams() instanceof CellLayout.LayoutParams);
    }

    public void bindAllApplications2Folder(final ArrayList<AppInfo> apps, final ArrayList<ItemInfo> items,
            AllAppsContainerView mAppsView, LauncherCallbacks mLauncherCallbacks) {
        if (mAppsView != null) {
            if (items == null) {
                mAppsView.setApps(apps);
            } else {
                mAppsView.setApps2Folder(items);
            }
        }

        if (mLauncherCallbacks != null) {
            mLauncherCallbacks.bindAllApplications(apps);
        }
    }

}