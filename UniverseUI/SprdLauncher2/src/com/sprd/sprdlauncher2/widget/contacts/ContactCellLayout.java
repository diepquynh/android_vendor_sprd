/** Created by Spreadtrum */
package com.sprd.sprdlauncher2.widget.contacts;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.GridLayout;

public class ContactCellLayout extends GridLayout {

    public ContactCellLayout(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
    }

    public ContactCellLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public ContactCellLayout(Context context) {
        super(context);
        init();
    }

    private void init() {
        setColumnCount(3);
        setRowCount(3);
        setAlwaysDrawnWithCacheEnabled(false);
    }

    public static void setContactCellCount(int xCount, int yCount) {
    }
}
