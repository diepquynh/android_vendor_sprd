package com.sprd.validationtools;
import android.os.SystemProperties;
import java.util.EnumSet;
import android.util.Log;

/**
 * Created by SPREADTRUM\zhengxu.zhang on 9/25/15.
 */
public class TelephonyManagerSprd {
	private static final String TAG = "TelephonyManagerSprd";
    //modem type
    public static final int MODEM_TYPE_GSM = 0;
    public static final int MODEM_TYPE_TDSCDMA = 1;
    public static final int MODEM_TYPE_WCDMA = 2;
    public static final int MODEM_TYPE_LTE = 3;
    public static final String MODEM_TYPE = "ro.radio.modemtype";
    private static String PROP_SSDA_MODE = "persist.radio.modem.config";

    // ssda mode
    private static String MODE_SVLTE = "svlte";
    private static String MODE_TDD_CSFB = "TL_TD_G,G";
    private static String MODE_FDD_CSFB = "TL_LF_W_G,G";
    private static String MODE_CSFB = "TL_LF_TD_W_G,G";

    /**
     * Returns the type of modem
     * for 0:GSM;1:TDSCDMA;2:WCDMA;3:LTE
     */
    public static int getModemType() {
        String modeType = SystemProperties.get(MODEM_TYPE, "");
        Log.d(TAG,"getModemType: modemType=" + modeType);
        if ("t".equals(modeType)) {
            return MODEM_TYPE_TDSCDMA;
        } else if ("w".equals(modeType)) {
            return MODEM_TYPE_WCDMA;
        } else if ("tl".equals(modeType) || "lf".equals(modeType) || "l".equals(modeType)) {
            return MODEM_TYPE_LTE;
        } else {
            return MODEM_TYPE_GSM;
        }
    }

    public static enum RadioCapbility {
        NONE, TDD_SVLTE, FDD_CSFB, TDD_CSFB, CSFB
    };

    public static RadioCapbility getRadioCapbility() {
        String ssdaMode = SystemProperties.get(PROP_SSDA_MODE);	
        Log.d(TAG, "getRadioCapbility: ssdaMode=" + ssdaMode);
        if (ssdaMode.equals(MODE_SVLTE)) {
            return RadioCapbility.TDD_SVLTE;
        } else if (ssdaMode.equals(MODE_TDD_CSFB)) {
            return RadioCapbility.TDD_CSFB;
        } else if (ssdaMode.equals(MODE_FDD_CSFB)) {
            return RadioCapbility.FDD_CSFB;	
        } else if (ssdaMode.equals(MODE_CSFB)) {
            return RadioCapbility.CSFB;	
        }	
        return RadioCapbility.NONE;
    }
}
