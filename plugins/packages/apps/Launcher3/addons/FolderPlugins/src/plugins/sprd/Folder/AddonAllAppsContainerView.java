package plugins.sprd.Folder;

import com.android.launcher3.Folderplugins.PluginAllAppsContainerView;

import com.android.launcher3.allapps.AllAppsContainerView;
import com.android.launcher3.allapps.AllAppsGridAdapter;
import com.android.launcher3.Folderplugins.FolderPlugins;
import android.app.AddonManager;
import android.content.Context;
import com.android.launcher3.ItemInfo;
import android.view.View;
import com.android.launcher3.AppInfo;
import com.android.launcher3.DragSource;
import com.android.launcher3.Folder;
import com.android.launcher3.FolderIcon;
import com.android.launcher3.allapps.AlphabeticalAppsList;
import com.android.launcher3.Launcher;
import android.graphics.Point;

import java.util.List;
import android.util.Log;


public class AddonAllAppsContainerView extends PluginAllAppsContainerView implements AddonManager.InitialCallback {

    private static final int BEGIN_DRAG_DELAY = 150;
    private static final int SPRING_LOADED_DELAY = 100;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public void setApps2Folder(List<AppInfo> apps, List<ItemInfo> items, AlphabeticalAppsList mApps) {
        mApps.setApps2Folder(apps, items);
    }

    public void setApps2Folder(List<ItemInfo> items, AlphabeticalAppsList mApps) {
        mApps.setApps2Folder(items);
    }

    // Drag app from AllApplist folder.
    public void beginDraggingFromFolder(View child, final Launcher mLauncher, Point mIconLastTouchPos,
            AllAppsContainerView allapps) {
        mLauncher.getWorkspace().beginDragShared(child, mIconLastTouchPos, allapps, false);

        // We delay entering spring-loaded mode slightly to make sure the UI
        // thready is free of any work.
        allapps.postDelayed(new Runnable() {
            @Override
            public void run() {
                // Launcher may destroyed when this delay callback run.
                if (mLauncher == null || mLauncher.getDragController() == null) {
                    return;
                }
                // We don't enter spring-loaded mode if the drag has been
                // cancelled
                if (mLauncher.getDragController().isDragging()) {
                    // Go into spring loaded mode (must happen before we
                    // startDrag())
                    mLauncher.enterSpringLoadedDragMode();
                }
            }
        }, BEGIN_DRAG_DELAY);
    }

    public void enterSpringLoadedIfNeeded(AllAppsContainerView allapps, final Launcher mLauncher) {
        // We delay entering spring-loaded mode slightly to make sure the UI
        // thready is free of any work.
        allapps.postDelayed(new Runnable() {
            @Override
            public void run() {
                // Launcher may destroyed when this delay callback run.
                if (mLauncher == null || mLauncher.getDragController() == null) {
                    return;
                }
                // We don't enter spring-loaded mode if the drag has been
                // cancelled
                if (mLauncher.getDragController().isDragging()) {
                    // Go into spring loaded mode (must happen before we
                    // startDrag())
                    mLauncher.enterSpringLoadedDragMode();
                }
            }
        }, SPRING_LOADED_DELAY);
    }

    public Folder getOpenFolder(AllAppsGridAdapter mAdapter) {
        for (int i = 0; i < mAdapter.getItemCount(); i++) {
            if (mAdapter.getItemViewType(i) == AllAppsGridAdapter.FOLDER_VIEW_TYPE) {
                if (mAdapter.getLayoutManager().getChildAt(i) instanceof FolderIcon) {
                    Folder folder = ((FolderIcon) mAdapter.getLayoutManager().getChildAt(i)).getFolder();
                    if (folder.getInfo().opened) {
                        return folder;
                    }
                }
            }
        }
        return null;
    }
}
