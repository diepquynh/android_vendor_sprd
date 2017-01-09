package plugin.sprd.apneditor;

import android.app.AddonManager;
import android.content.Context;
import com.sprd.settings.ApnEditorUtils;
import android.util.Log;

public class ApnEditorPlugin extends ApnEditorUtils implements
        AddonManager.InitialCallback {

    private final String TAG = "VodafoneApnPlugin";
    private Context mAddonContext;

    public ApnEditorPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean allowEmptyApn() {
        Log.d(TAG, "allowEmptyApn(): true");
        return true;
    }

    @Override
    public boolean isApnEditable() {
        Log.d(TAG, "isApnEditable(): false");
        return false;
    }
}
