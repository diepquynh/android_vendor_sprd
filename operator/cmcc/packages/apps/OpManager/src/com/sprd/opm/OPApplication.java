package com.sprd.opm;

import android.app.Application;

import cn.richinfo.dm.DMSDK;
import cn.richinfo.dm.util.DMLog;
import cn.richinfo.dm.util.MobileUtil;

public class OPApplication extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        DMLog.i("OPApplication", "OPApplication onCreate");

        DMSDK.init(this);
        DMSDK.setDebugMode(true);
    }
}
