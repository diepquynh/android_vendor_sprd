package plugin.sprd.apneditor;

import android.app.AddonManager;
import android.content.Context;
import android.database.Cursor;
import com.sprd.settings.ApnEditorUtils;
import android.util.Log;

public class ApnEditorPlugin extends ApnEditorUtils implements
        AddonManager.InitialCallback {

    private final String TAG = "CUCCApnPlugin";
    private Context mAddonContext;

    public ApnEditorPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean getEditable(Cursor cursor) {
        boolean editable = true;
        int index = cursor.getColumnIndex("editable");
        if (index >= 0) {
            editable = (cursor.getInt(index) == 1);
        }

        return editable;
    }

    @Override
    public boolean allowDisplayXCAP() {
        return false;
    }
}
