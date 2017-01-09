package plugin.sprd.signalstrengthforoperator;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.os.SystemProperties;
import android.telephony.SignalStrength;
import com.android.telephony.SignalStrengthUtils;


public class SignalStrengthUtilsForOperator extends SignalStrengthUtils implements AddonManager.InitialCallback {

    public static final String LOG_TAG = "SignalStrengthUtilsForOperator";
    private Context mAddonContext;

    public SignalStrengthUtilsForOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public int processLteLevel(int level, int lteRsrp){
        if (lteRsrp > -44)
            level = SignalStrength.SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
        else if (lteRsrp >= -97)
            level = SignalStrength.SIGNAL_STRENGTH_GREAT;
        else if (lteRsrp >= -105)
            level = SignalStrength.SIGNAL_STRENGTH_GOOD;
        else if (lteRsrp >= -113)
            level = SignalStrength.SIGNAL_STRENGTH_MODERATE;
        else if (lteRsrp >= -120)
            level = SignalStrength.SIGNAL_STRENGTH_POOR;
        else
            level = SignalStrength.SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
        Log.d(LOG_TAG, "getOperatorLteLevel=" + level);
        return level;
    }
}
