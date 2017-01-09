package plugin.sprd.telephonysupportorange;

import android.app.ActivityManagerNative;
import android.app.IActivityManager;
import android.app.AddonManager;
import android.content.Context;
import android.content.res.Configuration;
import com.android.internal.telephony.MccTable;
import com.android.internal.telephony.plugin.TelephonyForOrangeUtils;
import android.os.Environment;
import android.os.SystemProperties;
import android.util.Log;

import java.io.File;
import java.util.Locale;

public class TelephonySupportOrange extends TelephonyForOrangeUtils implements
        AddonManager.InitialCallback {
    private static String LOG_TAG = "TelephonySupportOrange";
    private static boolean DBG = true;

    private Context mAddonContext;
    public TelephonySupportOrange(){
    }
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }
    public boolean IsSupportOrange() {
        return true;
    }
    public void setSystemLocaleLock(String prefLang,String imsi,Context context){
        if (prefLang != null) {
            String country = null;
            if (imsi != null) {
                country = MccTable.countryCodeForMcc(Integer.parseInt(imsi.substring(0,3)));
            }
            if (DBG) Log.d(LOG_TAG,"Setting locale to " + prefLang + "_" + country);
            setSystemLocaleLock(context, prefLang, country);
        }else{
            if (DBG) Log.d(LOG_TAG,"No suitable LANG selected locale");
        }
    }
    /* SPRD: add for Orange sim language @{ */
    public static void setSystemLocaleLock(Context context, String language, String country) {
        if (DBG) Log.d(LOG_TAG,"Setting locale to setSystemLocaleLock" + "_" + country);
        String l = SystemProperties.get("persist.sys.language");
        String c = SystemProperties.get("persist.sys.country");
        if(!isAutoSelLang()){
            return;
        }
        if (null == language) {
            return; // no match possible
        }
        language = language.toLowerCase();
        if (null == country) {
            country = "  ";
        }
        country = country.toUpperCase();

        try {
            // try to find a good match
            String[] locales = context.getAssets().getLocales();
            final int N = locales.length;
            String bestMatch = null;

            for(int i = 0; i < N; i++) {
                // only match full (lang + country) locales
                if (locales[i]!=null && locales[i].length() >= 5 && locales[i].substring(0,2).equals(language)) {
                    if (locales[i].substring(3,5).equals(country)) {
                        bestMatch = locales[i];
                        break;
                    } else if (null == bestMatch) {
                        bestMatch = locales[i];
                    }
                }
            }
            if (null != bestMatch) {
                IActivityManager am = ActivityManagerNative.getDefault();
                Configuration config = am.getConfiguration();
                config.locale = new Locale(bestMatch.substring(0,2), bestMatch.substring(3,5));
                config.userSetLocale = true;
                am.updateConfiguration(config);
            }
        } catch (Exception e) {
            Log.e(LOG_TAG,"setSystemLocaleLock  set configure exception!");
        }
    }

    private static boolean isAutoSelLang(){
        File f = new File(Environment.getDataDirectory()+"/lang_auto_sel");
        if(f.exists()){
            return false;
        }else{
            return true;
        }
    }
    /* @} */

}
