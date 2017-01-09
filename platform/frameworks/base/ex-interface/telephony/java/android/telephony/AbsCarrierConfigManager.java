package android.telephony;

import android.content.Context;
import android.os.PersistableBundle;

public class AbsCarrierConfigManager {
    public static CarrierConfigManagerEx from(Context context) {
        return null;
    }

    public PersistableBundle getConfigForSubId(int subId) {
        return null;
    }

    public PersistableBundle getConfigForPhoneId(int phoneId) {
        return null;
    }

    public PersistableBundle getConfigForDefaultPhone() {
        return null;
    }

    public PersistableBundle getConfig() {
        return null;
    }

    public void notifyConfigChangedForSubId(int subId) {
    }

    public void updateConfigForPhoneId(int phoneId, String simState) {
    }

    public static PersistableBundle getDefaultConfig() {
       return null;
    }
}
