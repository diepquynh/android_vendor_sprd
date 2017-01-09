package plugin.sprd.logEmergencyCalls;

import android.app.AddonManager;
import android.content.Context;
import android.telephony.PhoneNumberUtils;
import android.util.Log;
import android.widget.TextView;

import com.sprd.dialer.plugins.EmergencyNumberHelper;
import com.android.contacts.common.location.CountryDetector;
import com.android.dialer.R;

/**
 * Various utilities for dealing with phone number strings.
 */
public class AddonEmergencyPlugin extends EmergencyNumberHelper
        implements AddonManager.InitialCallback{
    private static final String TAG = "[AddonEmergencyPlugin]";
    private static final boolean DBG = true;

    public AddonEmergencyPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        log("clazz: " + clazz);
        return clazz;
    }

    @Override
    public String getEmergencyNumber(Context context, String displayNumber,
            String detailsNumber) {
        String countryIso = CountryDetector.getInstance(context).getCurrentCountryIso();
        String displayName;
        log("countryIso = " + countryIso);
        if (PhoneNumberUtils.isLocalEmergencyNumberEx(detailsNumber, countryIso)) {
            displayName = context.getResources().getString(R.string.emergency_call);
        } else {
            displayName = displayNumber;
        }
        log("displayName = " + displayName);
        return displayName;
    }

    private static void log(String msg) {
        if (DBG) Log.d(TAG, msg);
    }
}
