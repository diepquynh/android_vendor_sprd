package plugin.sprd.apnutils;

import android.app.AddonManager;
import android.content.Context;
import android.database.Cursor;
import android.provider.Telephony;
import android.util.Log;

import com.sprd.settings.ApnUtils;

public class ApnUtilsPlugin extends ApnUtils implements AddonManager.InitialCallback {
    private static final String TAG = "ApnUtilsPlugin";
    private Context mAddonContext;

    public ApnUtilsPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean getEditable(Cursor cursor) {
        boolean editable = true;
        int index = cursor.getColumnIndex(Telephony.Carriers.EDITABLE);
        if (index > 0) {
            editable = (cursor.getInt(index) == 1);
        }

        return editable;
    }
}
