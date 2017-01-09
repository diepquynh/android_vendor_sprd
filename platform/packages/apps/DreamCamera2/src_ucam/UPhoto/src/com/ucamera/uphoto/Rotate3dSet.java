/*
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */

/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.ucamera.uphoto;

import android.view.View;
import android.view.animation.Animation;

public class Rotate3dSet {
    public void applyRotation(final View container, final View view) {
        float start = 0;
        float end = -75;
        final float centerX = 0;
        final float centerY = view.getHeight() / 2.0f;
        float width = view.getWidth();
        float offsetX = (float) (width * Math.cos(Math.toRadians(75)));

        final Rotate3dAnimation rotation = new Rotate3dAnimation(start, end, centerX, centerY, 300.0f, true, offsetX);
        rotation.setDuration(500);
        rotation.setFillAfter(true);
        rotation.setAnimationListener(new Animation.AnimationListener() {
            @Override
            public void onAnimationStart(Animation animation) {
                container.post(new DisplayView(container));
            }

            @Override
            public void onAnimationRepeat(Animation animation) {

            }

            @Override
            public void onAnimationEnd(Animation animation) {
                view.setOnClickListener(null);
            }
        });

        view.startAnimation(rotation);
    }

    /**
     * This class is responsible for swapping the views and start the second
     * half of the animation.
     */
    private final class DisplayView implements Runnable {
        private final View mContainer;

        public DisplayView(View container) {
            mContainer = container;
        }

        public void run() {
            mContainer.setVisibility(View.VISIBLE);
            final float centerX = mContainer.getRight() / 2.0f;
            final float centerY = mContainer.getHeight() / 2.0f;

            mContainer.requestFocus();

            Rotate3dAnimation rotation = new Rotate3dAnimation(90, 0, centerX, centerY, 300.0f, false, 0);

            rotation.setDuration(750);
            rotation.setFillAfter(true);

            mContainer.bringToFront();
            mContainer.startAnimation(rotation);
        }
    }

}
