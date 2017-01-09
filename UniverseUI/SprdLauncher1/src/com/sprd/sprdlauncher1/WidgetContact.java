/** Created by Spreadtrum */
package com.sprd.launcher3;

import android.content.Context;
import android.content.res.Resources;

import com.android.sprdlauncher1.R;

public class WidgetContact extends OwnerWidgetInfo {
    final static int SPANX = 4;

    final static int SPANY = 4;

    // final static int MIN_WIDTH = 400;

    // final static int MIN_HEIGHT = 440;

    public WidgetContact(Context context) {
        super();
        this.spanX = SPANX;
        this.spanY = SPANY;
        this.layoutResource = R.layout.widget_contact_add;
        this.flag = 2;
        this.icon = R.drawable.widgets_desktopcontact_launcher_icon;
        this.previewImage = R.drawable.widgets_desktopcontact_launcher_icon;
        this.lable = R.string.contact_widget_name;
        int actualWidth = context.getResources().getDimensionPixelSize(
                R.dimen.workspace_cell_width);
        int actualHeight = context.getResources().getDimensionPixelSize(
                R.dimen.workspace_cell_height);
        int smallerSize = Math.min(actualWidth, actualHeight);
        Resources r = context.getResources();
        /*fix bug203199:the contact widget whose span is now 5*5 should be 4*4, on 2013/8/19
         *subtract the padding space start
         */
        int paddingH = r
                .getDimensionPixelSize(com.android.internal.R.dimen.default_app_widget_padding_left)
                + r.getDimensionPixelSize(com.android.internal.R.dimen.default_app_widget_padding_right);
        int paddingV = r
                .getDimensionPixelSize(com.android.internal.R.dimen.default_app_widget_padding_bottom)
                + r.getDimensionPixelSize(com.android.internal.R.dimen.default_app_widget_padding_top);
        /*fix bug203199:the contact widget whose span is now 5*5 should be 4*4, on 2013/8/19
         *subtract the padding space end
         */
        /* SPRD : fix bug256366, set minResizeWidth and minResizeHeight for OWNER_WIDGET. @{ */
        this.minResizeWidth = this.minWidth = smallerSize * SPANX - paddingH;
        this.minResizeHeight = this.minHeight = smallerSize * SPANY - paddingV;
        /* @} */

    }
}
