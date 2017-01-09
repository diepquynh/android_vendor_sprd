package com.android.sprdlauncher2;

/**
 *  SPRD: add for UUI empty folders @{
 */
public class SprdPendingAddFolderInfo extends PendingAddItemInfo {
    // Any configuration data that we want to pass to a configuration activity when
    // starting up a widget
    String mimeType;

    public SprdPendingAddFolderInfo(){
        itemType = LauncherSettings.Favorites.ITEM_TYPE_EMPTY_FOLDER;
    }
}
/** @} */