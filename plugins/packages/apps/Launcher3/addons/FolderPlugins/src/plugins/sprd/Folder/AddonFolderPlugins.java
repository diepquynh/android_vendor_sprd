package plugins.sprd.Folder;


import java.util.ArrayList;

import com.android.launcher3.AppInfo;
import com.android.launcher3.Folder;
import com.android.launcher3.FolderInfo;
import com.android.launcher3.ItemInfo;
import com.android.launcher3.Launcher;
import com.android.launcher3.LauncherModel;
import com.android.launcher3.LauncherSettings;
import com.android.launcher3.LauncherSettings.Favorites;
import com.android.launcher3.DropTarget.DragObject;

import com.android.launcher3.Folderplugins.FolderPlugins;
import android.app.AddonManager;
import android.util.Log;
import android.content.Context;
import android.view.View;

public class AddonFolderPlugins extends FolderPlugins implements AddonManager.InitialCallback {

    private static final String TAG = "Launcher.Folder";
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public boolean isEnable() {
        return true;
    }

    public boolean beginDrag(Folder folder, Object tag, View v, final Launcher mLauncher) {
        if (!v.isInTouchMode()) {
            return false;
        }
        AppInfo item = (AppInfo) tag;
        Log.d(TAG, String.format("onLongClick, item.fromAppList: %s, item: %s", item.fromAppList, item));
        // Handle AllAppList folder long click event.
        mLauncher.getAppsView().beginDraggingFromFolder(v);
        folder.post(new Runnable() {
            @Override
            public void run() {
                mLauncher.closeFolder();
            }
        });
        return true;
    }

    public void updateItem(FolderInfo mInfo, Launcher mLauncher) {
        // When edit folder name in AllAppList, just save it into database.
        if (mInfo.container == LauncherSettings.Favorites.CONTAINER_APPLIST) {
            Log.d(TAG, "doneEditingFolderName in app list folder");
            LauncherModel.updateItemInAppsDatabase(mLauncher, mInfo);
        } else {
            LauncherModel.updateItemInDatabase(mLauncher, mInfo);
        }
    }

    public void saveFolderLocation(Folder folder, FolderInfo Info,Launcher launcher) {
        if (Info.container == LauncherSettings.Favorites.CONTAINER_APPLIST) {
            Log.d(TAG, "bind folder in apps");
            updateItemLocationsInAppsDatabase(folder,Info,launcher);
        } else {
            Log.d(TAG, "bind folder in workspace");
            updateItemLocationsInDatabase(folder,Info,launcher);
        }
    }


    public void updateItemLocationsInDatabase(Folder folder,FolderInfo mInfo,Launcher launcher) {
        ArrayList<View> list = folder.getItemsInReadingOrder();
        for (int i = 0; i < list.size(); i++) {
            View v = list.get(i);
            ItemInfo info = (ItemInfo) v.getTag();
            LauncherModel.moveItemInDatabase(launcher, info, mInfo.id, 0, info.cellX, info.cellY);
        }
    }

    public void updateItemLocationsInAppsDatabase(Folder folder,FolderInfo mInfo,Launcher launcher) {
        ArrayList<View> list = folder.getItemsInReadingOrder();
        for (int i = 0; i < list.size(); i++) {
            View v = list.get(i);
            ItemInfo info = (ItemInfo) v.getTag();
            LauncherModel.moveItemInAppsDatabase(launcher, info, mInfo.id, 0, info.cellX, info.cellY);
        }
    }

    public boolean deferDragView(FolderInfo mInfo, boolean successful, final DragObject d) {
        if (mInfo.fromAppList) {
            if (!successful) {
                d.deferDragViewCleanupPostAnimation = false;
            }
            return true;
        }
        return false;
    }
}
