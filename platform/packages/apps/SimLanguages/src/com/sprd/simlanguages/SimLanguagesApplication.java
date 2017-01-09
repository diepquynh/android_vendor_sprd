package com.sprd.simlanguages;
import android.app.Application;
import android.util.Log;

public class SimLanguagesApplication extends Application{
    private static SimLanguagesApplication instance;
    private static final String TAG = "SimLanguagesApplication";

    public static SimLanguagesApplication getInstance() {
        return instance;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
        Log.d(TAG, "onCreate");
    }
}
