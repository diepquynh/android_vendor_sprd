/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 *
 * Copyright (C) 2009 The Android Open Source Project
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

package com.ucamera.ucomm.downloadcenter;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.widget.ImageView;

/**
 * A @{code ImageView} which can rotate it's content.
 */
public class ClickedImageView extends ImageView{

    public ClickedImageView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public ClickedImageView(Context context) {
        super(context);
    }

    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        Drawable drawable = getResources().getDrawable(R.drawable.download_thumbnail_press);
        if (drawable == null) return;
        int width = getWidth();
        int height = getHeight();
        if (width == 0 || height == 0) {
            return;     // nothing to draw (empty bounds)
        }
        if(isPressed()) {
            drawable.setBounds(0, 0, width, height);
            drawable.draw(canvas);
        }
    }
}
