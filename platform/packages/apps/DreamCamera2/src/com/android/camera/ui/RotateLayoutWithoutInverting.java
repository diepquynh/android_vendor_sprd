
package com.android.camera.ui;

import android.content.Context;
import android.util.AttributeSet;

import com.android.camera.ui.RotateLayout;

// A DreamRotateLayout is designed to display a single item and provides the
// capabilities to rotate the item.
public class RotateLayoutWithoutInverting extends RotateLayout {

    public RotateLayoutWithoutInverting(Context context, AttributeSet attrs) {
        super(context, attrs);
        // The transparent background here is a workaround of the render issue
        // happened when the view is rotated as the device's orientation
        // changed. The view looks fine in landscape. After rotation, the view
        // is invisible.
    }

    protected void onMeasure(int widthSpec, int heightSpec) {
        super.onMeasureWithoutInverting(widthSpec, heightSpec);
    }
}
