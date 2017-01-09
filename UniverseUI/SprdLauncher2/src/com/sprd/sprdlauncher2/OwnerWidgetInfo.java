/** Created by Spreadtrum */
package com.sprd.sprdlauncher2;
import android.content.ComponentName;
import com.android.sprdlauncher2.ItemInfo;
import com.android.sprdlauncher2.LauncherSettings;

public class OwnerWidgetInfo extends ItemInfo {
    public int layoutResource;

    public int flag;

    public int lable;

    public int icon;

    public int previewImage;

    /* SPRD : fix bug256366, set minResizeWidth and minResizeHeight for OWNER_WIDGET. @{ */
    public int minWidth;

    public int minHeight;

    public int minResizeWidth;

    public int minResizeHeight;
    /* @} */

    public ComponentName configure = null;

    public OwnerWidgetInfo() {
        this.itemType = LauncherSettings.Favorites.ITEM_TYPE_WIDGET_OWNER;
    }
}
