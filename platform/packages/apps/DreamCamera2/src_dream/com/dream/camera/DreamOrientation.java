
package com.dream.camera;

import android.view.View;
import android.view.ViewGroup;
import com.android.camera.MultiToggleImageButton;
import com.android.camera.ui.Rotatable;

/*
 * Add for ui check 122
 */
public class DreamOrientation {

    public DreamOrientation() {

    }

    public static void setOrientation(View view, int orientation,
            boolean animation) {
        if (view == null) {
            return;
        }
        if (view instanceof Rotatable) {
            ((Rotatable) view).setOrientation(orientation, animation);
            if (view instanceof MultiToggleImageButton) {
                ((MultiToggleImageButton) view).setOrientation(orientation);
            }
            if (view instanceof ViewGroup) {
                ViewGroup group = (ViewGroup) view;
                for (int i = 0, count = group.getChildCount(); i < count; i++) {
                    setOrientation(group.getChildAt(i), orientation, animation);
                }
            }
        } else if (view instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) view;
            for (int i = 0, count = group.getChildCount(); i < count; i++) {
                setOrientation(group.getChildAt(i), orientation, animation);
            }
        }
    }
}
