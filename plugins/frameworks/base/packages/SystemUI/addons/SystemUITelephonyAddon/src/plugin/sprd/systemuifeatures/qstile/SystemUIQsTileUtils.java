package plugin.sprd.systemuifeatures.qstile;

import android.app.AddonManager;
import android.content.Context;

import com.android.systemui.qs.QSTile;
import com.android.systemui.qs.QSTile.Host;
import com.android.systemui.qs.tiles.IntentTile;

import com.sprd.systemui.SystemUIPluginUtils;
import android.util.Log;

public class SystemUIQsTileUtils extends SystemUIPluginUtils implements
        AddonManager.InitialCallback {
    private static final String TAG = "SystemUIQsTileUtils";
    private Context mAddonContext;

    public SystemUIQsTileUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public QSTile<?> createDataTile(Host host) {
        return new DataConnectionTile(host);
    }

    public QSTile<?> createFourGTile(Host host) {
        return new LteServiceTile(host);
    }
}
