/**
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */
package com.ucamera.ugallery.preference;

import android.content.Context;

import com.ucamera.ugallery.MyFullDialogActivity;

public abstract class MyDialogStub {
    protected MyFullDialogActivity mContext;

    public static final int FeedBackDialogStubType = 1;
    public static final int AboutUsDialogStubType = 2;

    public static MyDialogStub create(int type) {
        switch (type) {
            case FeedBackDialogStubType:
                return new MyFeedbackDialogStub();
            case AboutUsDialogStubType:
                return new MyAboutusDialogStub();
            default:
                return null;
        }
    }

    public void bind(MyFullDialogActivity dlg) {
        mContext = dlg;
        onBind();
    }

    protected Context getContext() {
        return mContext;
    }

    protected abstract void onBind();
    public abstract int getContentView();
}
