/*
 * Copyright (C) 2006 The Android Open Source Project
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

package android.view;

import android.content.Context;
import android.content.res.Resources;

import android.os.SystemProperties;
import android.util.Log;

public class SprdViewRootImpl extends ViewRootImpl{
    private static final String TAG = "SprdViewRootImpl";
    /* SPRD: add for dynamic navigationbar @{ */
    boolean mCanHaveNavigationBar = false;
    boolean mSwipeFromBottom = false;

    int mDisplayHeight = -1;

    static final int HEIGHT_TO_SKIP = 20;

    public SprdViewRootImpl(Context context, Display display) {
        super(context, display);

        mDisplayHeight = mDisplay.getMode().getPhysicalHeight();
        final Resources res = mContext.getResources();
        mCanHaveNavigationBar = res.getBoolean(com.android.internal.R.bool.config_showNavigationBar);
        String navBarOverride = SystemProperties.get("qemu.hw.mainkeys");
        if ("1".equals(navBarOverride)) {
            mCanHaveNavigationBar = false;
        } else if ("0".equals(navBarOverride)) {
            mCanHaveNavigationBar = true;
        }
    }

    boolean dropIfNeeded(InputEvent event) {
        boolean handled = false;
        if (mCanHaveNavigationBar && event instanceof MotionEvent) {
            MotionEvent e = (MotionEvent) event;
            switch (e.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    mSwipeFromBottom = false;
                    int orientation = mDisplay.getOrientation();
                    if (orientation%2 == 0 && e.getRawY() > mDisplayHeight - HEIGHT_TO_SKIP ||
                                orientation%2 == 1 && e.getRawX() > mDisplayHeight - HEIGHT_TO_SKIP) {
                        boolean keyguardShowing = false;
                        boolean hasNavigationBar = false;
                        try {
                            keyguardShowing = mWindowSession.isKeyguardShowingAndNotOccluded();
                            hasNavigationBar = mWindowSession.isNavigationBarShowing();
                        } catch (Exception er) {
                            Log.e(TAG, "throw exception, " + er);
                        }
                        if (!keyguardShowing && !hasNavigationBar) {
                            mSwipeFromBottom = true;
                            handled = true;
                        }
                    }
                    break;
                case MotionEvent.ACTION_UP:
                    if (mSwipeFromBottom == true) {
                        mSwipeFromBottom = false;
                        handled = true;
                    }
                    break;
                default:
                    if (mSwipeFromBottom == true) {
                        handled = true;
                    }
                    break;
            }
        }
        return handled;
    }
    /* @} */
}

