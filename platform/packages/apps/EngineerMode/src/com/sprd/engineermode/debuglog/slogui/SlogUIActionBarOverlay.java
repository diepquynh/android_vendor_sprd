/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
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

package com.sprd.engineermode.debuglog.slogui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.sprd.engineermode.R;

public class SlogUIActionBarOverlay extends ViewGroup {
    private LayoutInflater mInflater;
    View mLogo;
    View mRight;
    TextView mTitle;

    SlogUIActionBarOverlay(Context context) {
        super(context);
    }

    SlogUIActionBarOverlay(Context context, AttributeSet attrs) {
        super(context);
    }

    public SlogUIActionBarOverlay(Context context, AttributeSet attrs,
            int defStyle) {
        super(context, attrs, 0);
        setBackgroundResource(R.drawable.slogui_ab_background_overlay);
        if (context == null) {
            throw new NullPointerException(
                    "The context is null, can not inflate views");
        }
        mInflater = (LayoutInflater) context
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View v = mInflater.inflate(R.layout.slogui_actionbar_overlay, null);
        mLogo = v.findViewById(R.id.logo);
        mRight = v.findViewById(R.id.right);
        mTitle = (TextView) v.findViewById(R.id.title);
    }

    public void setTitle(String title) {
        mTitle.setVisibility(title == null ? View.INVISIBLE : View.VISIBLE);
        if (title != null) {
            mTitle.setText(title);
        }
    }

    public void setLogo(int resId, OnClickListener l) {
        mLogo.setVisibility(View.VISIBLE);
        mLogo.setBackgroundResource(resId);
        mLogo.setOnClickListener(l);
    }

    public void setRightDrawable(int resId, OnClickListener l) {
        mRight.setVisibility(View.VISIBLE);
        mRight.setBackgroundResource(resId);
        mRight.setOnClickListener(l);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {

    }
}
