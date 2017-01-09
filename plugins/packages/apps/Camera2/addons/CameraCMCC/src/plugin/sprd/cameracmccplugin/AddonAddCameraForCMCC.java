package plugin.sprd.cameracmccplugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

import com.sprd.camera.plugin.AddCameraForCMCC;

public class AddonAddCameraForCMCC extends AddCameraForCMCC implements AddonManager.InitialCallback{
    private static final String LOGTAG = "AddonAddCameraForCMCC";
    private final static int PEEP_HOLE_ANIMATION_DURATION_MS = 350;
    private Context mAddonContext;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        Log.d(LOGTAG, "onCreateAddon");
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean isCMCCVersion() {
        return true;
    }

    public int getAnimationDurationCmcc() {
        return PEEP_HOLE_ANIMATION_DURATION_MS;
    }
}
