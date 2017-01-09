package plugin.sprd.addmusicxposed;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

import com.sprd.music.plugin.*;

public class AddonAddMusicForCMCC extends AddMusicForCMCC implements AddonManager.InitialCallback{
    private static final String LOGTAG = "AddonMusicForCMCC";
    private Context mAddonContext;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        Log.e(LOGTAG, "onCreateAddon");
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean isCMCCVersion() {
        return true;
    }
}
