/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.sprdlauncher1;

import android.content.Context;
import android.view.View;

public class AppsCustomizeCellLayout extends CellLayout implements Page {
    public AppsCustomizeCellLayout(Context context) {
        super(context);
    }

    @Override
    public void removeAllViewsOnPage() {
        removeAllViews();
        setLayerType(LAYER_TYPE_NONE, null);
    }

    @Override
    public void removeViewOnPageAt(int index) {
        removeViewAt(index);
    }

    @Override
    public int getPageChildCount() {
        return getChildCount();
    }

    @Override
    public View getChildOnPageAt(int i) {
        return getChildAt(i);
    }

    @Override
    public int indexOfChildOnPage(View v) {
        return indexOfChild(v);
    }

    /**
     * Clears all the key listeners for the individual icons.
     */
    public void resetChildrenOnKeyListeners() {
        ShortcutAndWidgetContainer children = getShortcutsAndWidgets();
        int childCount = children.getChildCount();
        for (int j = 0; j < childCount; ++j) {
            children.getChildAt(j).setOnKeyListener(null);
        }
    }

    /* SPRD: Feature 248544, add menu for sort and edit in allapp's view @{ */
    public void invalidatePagedViewIcon() {
        int count = this.getChildCount();
        for (int i = 0; i < count; i++) {
            View v = this.getChildAt(i);
            if (v instanceof ShortcutAndWidgetContainer) {
                ((ShortcutAndWidgetContainer) v).invalidatePagedViewIcon();
            }
        }
    }
    /* @} */
}