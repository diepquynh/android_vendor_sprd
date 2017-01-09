package com.android.internal.telephony.plugin;

import android.app.AlertDialog;
import android.app.AddonManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.util.Log;
import com.android.internal.R;
import com.android.internal.telephony.dataconnection.DcFailCause;
import com.android.sprd.telephony.RadioInteractor;

import java.util.ArrayList;

public class DataConnectionUtils {

    private static DataConnectionUtils mInstance;
    private static final String LOG_TAG = "DataConnectionUtils";
    private static final String RI_SERVICE_NAME =
            "com.android.sprd.telephony.server.RADIOINTERACTOR_SERVICE";
    private static final String RI_SERVICE_PACKAGE =
            "com.android.sprd.telephony.server";

    public static final int RETRY_DELAY_LONG  = 45000;
    public static final int RETRY_DELAY_SHORT = 10000;
    public static final int RETRY_FROM_FAILURE_DELAY = 2 * 3600 * 1000; // 2 hours

    private Context mContext;
    private ArrayList<RadioInteractorCallback> mCallbacks =
            new ArrayList<RadioInteractorCallback>();
    protected RadioInteractor mRi;

    public interface RadioInteractorCallback {
        void onRiConnected(RadioInteractor ri);
    }

    public DataConnectionUtils() {
    }

    public static DataConnectionUtils getInstance(Context context) {
        if (mInstance != null) {
            return mInstance;
        }
        AddonManager addonManager = new AddonManager(context);
        mInstance = (DataConnectionUtils) addonManager.getAddon(
                R.string.feature_dataconnection_plugin,
                DataConnectionUtils.class);
        Log.d(LOG_TAG, "mInstance = " + mInstance);
        mInstance.init(context);
        return mInstance;
    }

    public void addRadioInteractorCallback(RadioInteractorCallback cb) {
        mCallbacks.add(cb);
        if (mRi != null) {
            cb.onRiConnected(mRi);
        }
    }

    public void removeRadioInteractorCallback(RadioInteractorCallback cb) {
        mCallbacks.remove(cb);
    }

    protected void init(Context context) {
        mContext = context;
        if (supportTrafficClass() || supportSpecialClearCode()) {
            // bind to radio interactor service
            Intent serviceIntent = new Intent(RI_SERVICE_NAME);
            serviceIntent.setPackage(RI_SERVICE_PACKAGE);
            Log.d(LOG_TAG, "bind RI service");
            context.bindService(serviceIntent, mConnection, Context.BIND_AUTO_CREATE);
        }
    }

    private ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            Log.d(LOG_TAG, "RadioInteractor service connected: service=" + service);
            mRi = new RadioInteractor(mContext);
            for (RadioInteractorCallback cb : mCallbacks) {
                cb.onRiConnected(mRi);
            }
            onRadioInteractorConnected(mRi);
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            mRi = null;
        }
    };

    protected void onRadioInteractorConnected(RadioInteractor ri) {
    }

    public boolean supportTrafficClass() {
        return false;
    }

    public boolean supportSpecialClearCode() {
        Log.d(LOG_TAG, "supportSpecialClearCode = false");
        return false;
    }

    public boolean isSpecialCode(DcFailCause fc) {
        return false;
    }

    public AlertDialog getErrorDialog(DcFailCause cause) {
        return null;
    }
}
