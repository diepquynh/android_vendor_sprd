/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit.cate;

import com.ucamera.ucam.modules.ugif.edit.callback.ProcessCallback;
import com.ucamera.ucam.modules.utils.Utils;
import com.android.camera2.R;

import android.content.Context;
import android.view.View;

public class PlayModeCate extends GifBasicCate{
    public PlayModeCate(Context context, ProcessCallback callback) {
        super(context, callback);
        mArrayRes = Utils.getIconIds(context.getResources(), R.array.ugif_edit_playmode_icons);
    }

    @Override
    public void onItemClick(int position) {
        if (mCallback != null) {
            mCallback.beforeProcess();
        }

        if (mCallback != null) {
            mCallback.afterProcess(position);
        }
    }
}
