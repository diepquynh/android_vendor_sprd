package plugin.sprd.showcallingnumber;

import android.app.AddonManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.sprd.telephony.RadioInteractor;
import com.android.sprd.telephony.RadioInteractorCallbackListener;
import com.sprd.phone.TeleServicePluginsHelper;

public class ShowCallingNumberPlugin extends TeleServicePluginsHelper
        implements AddonManager.InitialCallback {

    private static final String TAG = "ShowCallingNumberPlugin";
    private Context mAddonContext;
    private RadioInteractor mRi;
    private RadioInteractorCallbackListener[] mRadioInteractorListeners;
    private TelephonyManager mTelephonyManager;
    private int mPhoneCount;

    public ShowCallingNumberPlugin() {
    }

    /**
     * CMCC new case:Is connected to the number of restrictions on the business; the need to open
     * the default COLP limit calling number whether enable
     */
    private RadioInteractorCallbackListener getRadioInteractorCallbackListener(final int slotId) {
        return new RadioInteractorCallbackListener(slotId) {
            @Override
            public void onRiConnectedEvent() {
                mRi.setCallingNumberShownEnabled(slotId, true);
                Log.d(TAG, "setCallingNumberShownEnabled " + slotId + " complete.");
            }
        };
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        mTelephonyManager = new TelephonyManager(mAddonContext);
        mPhoneCount = mTelephonyManager.getPhoneCount();
        mRadioInteractorListeners = new RadioInteractorCallbackListener[mPhoneCount];
        mRi = new RadioInteractor(mAddonContext);
        return clazz;
    }

    /**
     * CMCC new case:Is connected to the number of restrictions on the business;
     * the need to open the default COLP
     * limit calling number whether enable
     */
    @Override
    public void setCallingNumberShownEnabled() {
        mAddonContext.bindService(
                new Intent("com.android.sprd.telephony.server.RADIOINTERACTOR_SERVICE")
                        .setPackage("com.android.sprd.telephony.server"), new ServiceConnection() {
                    @Override
                    public void onServiceConnected(ComponentName name, IBinder service) {
                        Log.d(TAG, "on radioInteractor service connected");
                        registerRiConnected();
                    }

                    @Override
                    public void onServiceDisconnected(ComponentName name) {
                        unregisterRiConnected();
                    }
                }, Context.BIND_AUTO_CREATE);
    }

    private void unregisterRiConnected() {
        for (int i = 0; i < mPhoneCount; i++) {
            if (mRadioInteractorListeners[i] != null) {
                mRi.listen(mRadioInteractorListeners[i],
                        RadioInteractorCallbackListener.LISTEN_NONE,
                        false);
                Log.d(TAG, "unregisterRiConnected " + i + " complete.");
            }
        }
    }

    private void registerRiConnected() {
        for (int i = 0; i < mPhoneCount; i++) {
            mRadioInteractorListeners[i] = getRadioInteractorCallbackListener(i);
            mRi.listen(mRadioInteractorListeners[i],
                    RadioInteractorCallbackListener.LISTEN_RI_CONNECTED_EVENT, false);
            Log.d(TAG, "registerRiConnected " + i + " complete.");
        }
    }
}