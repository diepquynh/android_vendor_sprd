package plugins.sprd.Folder;

import java.util.ArrayList;

import com.android.launcher3.AppInfo;
import com.android.launcher3.BubbleTextView;
import com.android.launcher3.DeviceProfile;
import com.android.launcher3.Folder;
import com.android.launcher3.FolderIcon;
import com.android.launcher3.FolderInfo;
import com.android.launcher3.ItemInfo;
import com.android.launcher3.Launcher;
import com.android.launcher3.LauncherSettings;
import com.android.launcher3.ShortcutInfo;
import com.android.launcher3.DropTarget.DragObject;
import com.android.launcher3.FolderIcon.FolderRingAnimator;
import com.android.launcher3.Folderplugins.FolderIconPlugin;
import com.android.launcher3.R;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;

public class FolderIconPluginAddon extends FolderIconPlugin implements AddonManager.InitialCallback {

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    private static final int DROP_IN_ANIMATION_DURATION = 400;
    private static final int INITIAL_ITEM_ANIMATION_DURATION = 350;

    public FolderIcon fromXml(int resId, Launcher launcher, ViewGroup group) {
        @SuppressWarnings("all") // suppress dead code warning
        final boolean error = INITIAL_ITEM_ANIMATION_DURATION >= DROP_IN_ANIMATION_DURATION;
        if (error) {
            throw new IllegalStateException("DROP_IN_ANIMATION_DURATION must be greater than " +
                    "INITIAL_ITEM_ANIMATION_DURATION, as sequencing of adding first two items " +
                    "is dependent on this");
        }

        FolderIcon icon = (FolderIcon) LayoutInflater.from(launcher).inflate(resId, group, false);
        icon.setClipToPadding(false);
        icon.mFolderName = (BubbleTextView) icon.findViewById(R.id.folder_icon_name);
        icon.mFolderName.setCompoundDrawablePadding(0);
        // Offset the preview background to center this view accordingly
        icon.mPreviewBackground = (ImageView) icon.findViewById(R.id.preview_background);
        icon.setOnClickListener(launcher);
        icon.mLauncher = launcher;
        icon.mFolderRingAnimator = new FolderRingAnimator(launcher, icon);
        icon.setOnFocusChangeListener(launcher.mFocusHandler);
        return icon;
    }

    public void bindFolderInfo(FolderInfo folderInfo,FolderInfo mInfo,Launcher mLauncher,Folder mFolder,BubbleTextView mFolderName,FolderIcon folderIcon) {
        DeviceProfile grid = mLauncher.getDeviceProfile();
        mFolderName.setText(folderInfo.title);
        FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams) folderIcon.mPreviewBackground.getLayoutParams();
        lp.topMargin = grid.folderBackgroundOffset;
        lp.width = grid.folderIconSizePx;
        lp.height = grid.folderIconSizePx;
        lp = (FrameLayout.LayoutParams) mFolderName.getLayoutParams();
        // Folder composit of ImageView and TextView. Textview topMargin = AllApp.AppIcon - AllApp.AppIcon.TextView.
        lp.topMargin = grid.allAppsIconSizePx - grid.allAppsIconTextSizePx;
        folderIcon.setTag(folderInfo);
        mInfo = folderInfo;
        folderIcon.setContentDescription(String.format(mLauncher.getString(R.string.folder_name_format),
                folderInfo.title));
        Folder folder = Folder.fromXml(mLauncher);
        folder.setDragController(mLauncher.getDragController());
        folder.setFolderIcon(folderIcon);
        folder.bind(folderInfo);
        mFolder = folder;
        folderInfo.addListener(folderIcon);
    }

    public ItemInfo onAlarm(ItemInfo item,ItemInfo mDragInfo,FolderInfo mInfo) {
        if (mInfo.container != LauncherSettings.Favorites.CONTAINER_APPLIST) {
            // Came from all apps -- make a copy.
            item = ((AppInfo) mDragInfo).makeShortcut();
            item.spanX = 1;
            item.spanY = 1;
        }
        return item;
    }

    public ItemInfo setItem(DragObject d, FolderInfo mInfo) {
        ItemInfo item = null;
        if (mInfo.container == LauncherSettings.Favorites.CONTAINER_APPLIST) {
            item = (ItemInfo) d.dragInfo;
        } else {
            if (d.dragInfo instanceof AppInfo) {
                // Came from all apps -- make a copy
                item = ((AppInfo) d.dragInfo).makeShortcut();
            } else {
                item = (ShortcutInfo) d.dragInfo;
            }
        }
        return item;
    }

    public void setFolderEnable(boolean enable,Folder mFolder){
        if (mFolder.mFolderName != null) {
            mFolder.mFolderName.setEnabled(!enable);
        }
    }

}