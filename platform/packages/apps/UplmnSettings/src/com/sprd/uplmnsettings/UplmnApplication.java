package com.sprd.uplmnsettings;

import android.app.Application;

public class UplmnApplication extends Application{
    private static UplmnApplication instance;

    public static UplmnApplication getInstance() {
        return instance;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
    }
}
