
package addon.sprd.AddonDeskClock;

import android.app.AddonManager;
import android.content.Context;
import android.util.ArrayMap;
import android.util.Log;

import java.util.Calendar;
import java.util.Collections;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.Locale;
import java.util.Map;
import java.util.TimeZone;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;
import android.text.format.DateFormat;
import android.text.format.DateUtils;
import android.text.format.Time;
import android.content.res.Resources;
import android.R.bool;
import android.R.integer;
import android.R.array;

import com.android.deskclock.data.City;
import com.sprd.deskclock.TimeZonePlugin;

public class AddonDeskClockTimezone extends TimeZonePlugin implements
        AddonManager.InitialCallback {
    Resources r;
    private static final String TAG = "AddonDeskClockTimezone";

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        r = context.getResources();
        Log.d(TAG, "AddonDeskClockTimezone onCreateAddon");
        return clazz;
    }

    /**
     * @return the domain of cities from which the user may choose a world clock
     */
    /* SPRD: Bug 588700 It displays an empty view when you tap the global icon @{ */
    public String[] getIds(Resources resources) {
        Log.e(TAG, "Addon getIds + " + resources.getStringArray(R.array.cities_id_sprd));
        return r.getStringArray(R.array.cities_id_sprd);
    }
    public String[] getNames(Resources resources) {
        return r.getStringArray(R.array.cities_names_sprd);
    }
    public String[] getTimezones(Resources resources) {
        return r.getStringArray(R.array.cities_tz_sprd);
    }
    /* @} */
}
