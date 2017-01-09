package test.sprd.helloworld.plugin;

import android.app.AddonManager;
import android.content.Context;
import test.sprd.helloworld.R;

public class TestInterfaceUtils {
    static TestInterfaceUtils sInstance;

    public static TestInterfaceUtils getInstance(Context context){
        if (sInstance != null) return sInstance;
        //If the app share process with the other application,
        //you'd best to get your own AddonManager like this.
        //Do Not use the default AddonManager created by FW.
        AddonManager addonManager = new AddonManager(context);
        sInstance = (TestInterfaceUtils)addonManager.
                getAddon(R.string.feature_testinterface, TestInterfaceUtils.class);
        return sInstance;
    }

    public TestInterfaceUtils() {
    }

    public void test(Context context) {
    }
}
