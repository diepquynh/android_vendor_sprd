
package addon.sprd.AddonDeskClock;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.Locale;
import java.util.TimeZone;
import android.text.format.DateFormat;
import android.text.format.DateUtils;
import android.text.format.Time;
import android.content.res.Resources;
import android.R.bool;
import android.R.integer;
import android.R.array;
import com.android.deskclock.worldclock.CityObj;
import com.sprd.deskclock.TimeZonePlugin;

public class AddonDeskClockTimezone extends TimeZonePlugin implements
        AddonManager.InitialCallback {
    Resources r;
    private static final String TAG = "AddonDeskClockTimezone";

    public AddonDeskClockTimezone() {
       Log.d(TAG, "AddonDeskClockTimezone AddonDeskClockTimezone");
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        r = context.getResources();
        Log.d(TAG, "AddonDeskClockTimezone onCreateAddon");
        return clazz;
    }

    /* SPRD fix bug 469053 add two clock widget,delete a worldclock,return home,the cities in widget display not same with clock @{ */
    public CityObj[] getCityTimeZone() {
        String[] cities = getCites();
        String[] timezones = getTimezones();
        String[] ids = getTimeIds();
        int minLength = cities.length;
        if (cities.length != timezones.length || ids.length != cities.length) {
            minLength = Math.min(cities.length, Math.min(timezones.length, ids.length));
            Log.d(TAG,"City lists sizes are not the same, truncating");
        }
        CityObj[] tempList = new CityObj[minLength];
        for (int i = 0; i < cities.length; i++) {
            tempList[i] = new CityObj(cities[i], timezones[i], ids[i], getCityIndex(cities[i]));
        }
        return tempList;
    }
    /* @} */

    public String[] getCites(){
        String[] cities = r.getStringArray(R.array.cities_names_sprd);
        return cities;
    }

    public String[] getTimezones(){
        String[] timezones = r.getStringArray(R.array.cities_tz_sprd);
        return timezones;
    }

    public String[] getTimeIds(){
        String[] ids = r.getStringArray(R.array.cities_id_sprd);
        return ids;
    }

    public int getTimezoneOffset (TimeZone tz, long time) {
        return tz.getRawOffset();
    }

    public String getCityIndex(String displayName) {
        final String parseString = displayName;
        final int separatorIndex = parseString.indexOf("=");
        final String index;
        if (parseString.length() <= 1 && separatorIndex >= 0) {
            Log.d(TAG,"Cannot parse city name %s; skipping"+parseString);
            return null;
        }
        if (separatorIndex == 0) {
            // Default to using second character (the first character after the = separator)
            // as the index.
            index = parseString.substring(1, 2);
        } else if (separatorIndex == -1) {
            // Default to using the first character as the index
            index = parseString.substring(0, 1);
        } else {
             index = parseString.substring(0, separatorIndex);
        }
        return index;
    }

}
