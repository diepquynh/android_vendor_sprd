/**
 * Manages carrier configs for DcTracker
 *
 * Created by Spreadst.
 */

package com.android.internal.telephony.dataconnection;

import android.content.Context;
import android.os.PersistableBundle;
import android.telephony.CarrierConfigManagerEx;

public final class DctConfig {
    private static DctConfig sInstance;

    private Context mContext;
    private CarrierConfigManagerEx mConfigManager;
    private PersistableBundle mPersistableBundle;

    /**
     * Get the singleton instance of DctConfig
     */
    public synchronized static DctConfig getInstance(Context context) {
        if (sInstance == null) {
            sInstance = new DctConfig(context);
        }
        return sInstance;
    }

    /**
     * Get the delay before tearing down data
     */
    public int getDataKeepAliveDuration(int defValue) {
        ensureConfig();
        if (mPersistableBundle != null) {
            return mPersistableBundle.getInt("dct_data_keep_alive_duration_int", defValue);
        }

        return defValue;
    }

    /**
     * Is National roaming enabled
     */
    public boolean isNationalDataRoamingEnabled() {
        ensureConfig();

        boolean retVal = false;
        if (mPersistableBundle != null) {
            retVal = mPersistableBundle.getBoolean("national_data_roaming_bool");
        }
        return retVal;
    }

    public void dispose() {
        mConfigManager = null;
        mPersistableBundle = null;
    }

    private DctConfig(Context context) {
        mContext = context;
        mConfigManager = (CarrierConfigManagerEx) mContext.getSystemService(
                "carrier_config_ex");
    }

    private void ensureConfig() {
        if (mPersistableBundle == null) {
            if (mConfigManager == null) {
                mConfigManager = (CarrierConfigManagerEx) mContext.getSystemService(
                        "carrier_config_ex");
            }

            if (mConfigManager != null) {
                mPersistableBundle = mConfigManager.getConfigForDefaultPhone();
            }
        }
    }
}
