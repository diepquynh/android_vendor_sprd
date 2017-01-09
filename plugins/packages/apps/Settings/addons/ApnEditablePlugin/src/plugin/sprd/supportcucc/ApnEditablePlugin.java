package plugin.sprd.apneditable;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

import com.sprd.settings.ApnEditableUtils;

public class ApnEditablePlugin extends ApnEditableUtils implements AddonManager.InitialCallback {
    private static final String TAG = "ApnEditablePlugin";
    private Context mAddonContext;

    public ApnEditablePlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean isApnEditable() {
        return false;
    }
}
