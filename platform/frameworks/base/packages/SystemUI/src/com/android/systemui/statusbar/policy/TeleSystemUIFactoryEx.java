package com.android.systemui.statusbar.policy;

import android.content.Context;
import android.content.res.Resources;
import android.util.Log;

public class TeleSystemUIFactoryEx extends TeleSystemUIFactory {
    private static final String TAG = "TeleSystemUIFactoryEx";

    public CallbackHandler createCallbackHandler () {
        return new CallbackHandlerEx();
    }
}
