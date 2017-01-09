package test.sprd.helloworld.plugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import test.sprd.helloworld.R;

public class PromptUtils {

    static PromptUtils sInstance;

    public static PromptUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (PromptUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_prompt, PromptUtils.class);
        return sInstance;
    }

    public PromptUtils() {
    }

    public void showPrompt(Context context) {
    }
}
