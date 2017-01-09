package plugin.sprd.jiemail;

import android.content.Intent;
import android.content.ComponentName;
import android.content.ActivityNotFoundException;

import android.app.AddonManager;
import android.content.Context;
import android.app.Activity;
import com.sprd.jiemail.SprdJiEmailAddonStub;

public class SprdJiEmailAddon extends SprdJiEmailAddonStub implements AddonManager.InitialCallback {

    private static final String TAG = "SprdJiEmailAddon";

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    @Override
    public void startJiEmailActivity(Activity mActivity) {
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_LAUNCHER);
        ComponentName componentName = new ComponentName(
                "cn.richinfo.automail", "cn.richinfo.automail.ui.activity.TransparentActivity");
        intent.setComponent(componentName);
        if (intent.resolveActivity(mActivity.getPackageManager()) != null) {
            try {
                mActivity.startActivity(intent);
            } catch (ActivityNotFoundException e) {
                android.util.Log.d(TAG," no application can handle the URL: " + e);
            }
        }
    }
}
