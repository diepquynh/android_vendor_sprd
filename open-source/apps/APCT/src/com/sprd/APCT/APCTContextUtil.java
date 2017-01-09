package com.sprd.APCT;
import android.app.Application;

public class APCTContextUtil extends Application {
    private static APCTContextUtil instance;

    public static APCTContextUtil getInstance() {
        return instance;
    }

    @Override
    public void onCreate() {
        // TODO Auto-generated method stub
        super.onCreate();
        instance = this;
    }
}
