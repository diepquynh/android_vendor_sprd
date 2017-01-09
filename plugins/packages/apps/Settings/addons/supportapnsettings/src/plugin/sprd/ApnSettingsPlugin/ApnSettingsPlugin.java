package plugin.sprd.supportapnsettings;

import android.app.AddonManager;
import android.content.Context;
import com.sprd.settings.ApnSettingsPluginsUtils;
import android.util.Log;
import android.os.SystemProperties;


public class ApnSettingsPlugin extends ApnSettingsPluginsUtils implements
        AddonManager.InitialCallback {

    private static final String TAG = "ApnSettingsPlugin";
    private Context mAddonContext;

    public ApnSettingsPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean apnSettingVisibility(String mccmnc, String apnType, boolean isSupportVolte) {
        boolean imsVisibility = Boolean.parseBoolean(SystemProperties
                .get("persist.sys.ims.visibility"));
        Log.d(TAG, "[ApnSettingVisibility] mccmnc: " + mccmnc + " apnType: "
                + apnType + " isSupportVolte: " + isSupportVolte
                + "imsVisibility: " + imsVisibility);
        if (apnType == null || "*".equals(apnType)) {
            return true;
        }

        if (isSupportVolte) {
            if("mms".equals(apnType)) {
                return mmsVisibilityForVolte(mccmnc, apnType);
            } else if ("ims".equals(apnType)) {
                return imsVisibility;
            } else {
                return true;
            }
        } else {
            return inVisibleForIms(apnType);
        }
  }

    public boolean getIsNeedSetChecked(String mccmnc, String apn) {
        Log.d(TAG, "[getIsNeedSetChecked] mccmnc: " + mccmnc + " apn: " + apn);
        boolean isCUCCMainlandCard = "46001".equals(mccmnc)
                || "46006".equals(mccmnc) || "46009".equals(mccmnc);
        if (isCUCCMainlandCard) {
            if ("3gnet".equals(apn)) {
                return true;
            } else {
                return false;
            }
        } else {
            return true;
        }
    }

    private boolean mmsVisibilityForVolte(String mccmnc, String apnType) {
        switch (mccmnc) {
            case "405874": {
                return inVisibleForMms(apnType);
            }
            case "405862": {
                return inVisibleForMms(apnType);
            }
            default: {
                return true;
            }
        }
    }

    private boolean inVisibleForMms(String apnType) {
        if ("mms".equals(apnType)) {
            return false;
        } else {
            return true;
        }
    }

    private boolean inVisibleForIms(String apnType) {
        if ("ims".equals(apnType)) {
            return false;
        } else {
            return true;
        }
    }
}