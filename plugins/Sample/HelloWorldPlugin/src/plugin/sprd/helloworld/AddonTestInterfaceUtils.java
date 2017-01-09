package plugin.sprd.helloworld;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import test.sprd.helloworld.R;

import test.sprd.helloworld.plugin.TestInterfaceUtils;
import test.sprd.helloworld.HelloWorldActivity;

public class AddonTestInterfaceUtils extends TestInterfaceUtils implements
    AddonManager.InitialCallback, HelloWorldActivity.TestInterface{

    public static final String LOGTAG = "AddonTestInterfaceUtils";

    public AddonTestInterfaceUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    @Override
    public void test(Context context) {
        context.getString(R.string.hello_world);
        Log.d(LOGTAG, "->test ");
        testInterface();
    }

    @Override
    public void testInterface(){
        Log.d(LOGTAG, "-> testInterface " + TEST);
    }
}
