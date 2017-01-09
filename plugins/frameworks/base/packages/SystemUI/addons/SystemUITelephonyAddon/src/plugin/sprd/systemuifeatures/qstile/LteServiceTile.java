package plugin.sprd.systemuifeatures.qstile;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.drawable.BitmapDrawable;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.telecom.TelecomManager;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManagerEx;
import android.util.Log;
import android.widget.Toast;
import com.android.internal.telephony.RILConstants;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.logging.MetricsLogger;
import com.android.systemui.R;
import com.android.systemui.qs.QSIconView;
import com.android.systemui.qs.QSTile;
import com.android.systemui.qs.QSTileView;
import com.android.systemui.qs.SignalTileView;
import com.android.systemui.qs.QSTile.ResourceIcon;
import android.widget.Toast;
/** Quick settings tile: Lte service **/
public class LteServiceTile extends QSTile<QSTile.BooleanState> {

    private static final String AT_QUERY_LTE_STATE = "AT+SPUECAP?";
    private static final int UPDATE_LTE_STATE = 100;
    private static final String LTE_STATE_ENABLE = "0";
    int NT_MODE_LTE_GSM_WCDMA = RILConstants.NETWORK_MODE_LTE_GSM_WCDMA;
    int NT_MODE_WCDMA_PREF = RILConstants.NETWORK_MODE_WCDMA_PREF;

    private TelephonyManager mTelephonyManager;
    private TelephonyManagerEx mTelephonyManagerEx;
    private boolean mListening;
    private boolean mLteEnabled;
    private boolean mLteAvailable;
    private QSTileView mQSTileView;
    public static final int QS_LTESERVICE = 413;

    public LteServiceTile(Host host) {
        super(host);
        mTelephonyManager = TelephonyManager.from(mContext);
        mTelephonyManagerEx = TelephonyManagerEx.from(mContext);
    }

    public QSTileView createTileView(Context context, QSIconView icon) {
        mQSTileView = new QSTileView(context,icon);
        return mQSTileView;
    }


    @Override
    /*protected*/public BooleanState newTileState() {
        return new BooleanState();
    }

    @Override
    public Intent getLongClickIntent() {
        return new Intent();
    }

    /* SPRD: modify by BUG 607871 @{ */
    @Override
    protected void handleLongClick() {
        handleClick();
    }
    /* @} */

    @Override
    public CharSequence getTileLabel() {
        return mContext.getString(R.string.quick_settings_lte_service_label);
    }

    @Override
    public int getMetricsCategory() {
        return QS_LTESERVICE;
    }

    public boolean isAvailable() {
        final boolean showLTETile = mContext.getResources().getBoolean(
                R.bool.config_show_lte_tile);
        return showLTETile;
    }

    /* SPRD: During the call,user can't switch 4G service for bug542092. @{ */
    private boolean isInCall() {
        return getTelecommManager().isInCall();
    }

    private TelecomManager getTelecommManager() {
        return (TelecomManager) mContext.getSystemService(Context.TELECOM_SERVICE);
    }
    /* @} */

    @Override
    public void handleClick() {
        MetricsLogger.action(mContext, getMetricsCategory(), !mState.value);
        if (isAirplaneModeOn()) {
            mHost.collapsePanels();
            Toast.makeText(mContext, R.string.lte_service_error_airplane, Toast.LENGTH_SHORT)
                    .show();
            return;
        }
        /* SPRD: During the call,user can't switch 4G service for bug542092. @{ */
        if (isInCall()) {
            mHost.collapsePanels();
            Toast.makeText(mContext, R.string.lte_service_error_incall, Toast.LENGTH_SHORT)
                    .show();
            return;
        }
        /* @} */
        if (!mLteAvailable) {
            return;
        } else {
            setLteEnabled();
        }
    }

    private void setLteEnabled() {
        Log.d(TAG, "setLteEnabled: " + !mState.value);
        mLteEnabled = !mState.value;
        int defaultDataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
        boolean result = false;
        /* SPRD: reset to the previous state if set up network fail,modify for bug 599770 @{ */
        int previousType = android.provider.Settings.Global.getInt(mContext.getContentResolver(),
                android.provider.Settings.Global.PREFERRED_NETWORK_MODE + defaultDataSubId,
                RILConstants.PREFERRED_NETWORK_MODE);
        android.provider.Settings.Global.putInt(mContext.getContentResolver(),
                android.provider.Settings.Global.PREFERRED_NETWORK_MODE + defaultDataSubId,
                mLteEnabled ? NT_MODE_LTE_GSM_WCDMA : NT_MODE_WCDMA_PREF);
        result = mTelephonyManager.setPreferredNetworkType(defaultDataSubId,
                mLteEnabled ? NT_MODE_LTE_GSM_WCDMA : NT_MODE_WCDMA_PREF);
        if (!result) {
            android.provider.Settings.Global.putInt(mContext.getContentResolver(),
                    android.provider.Settings.Global.PREFERRED_NETWORK_MODE + defaultDataSubId,
                    previousType);
        }
        /* @} */
        handleRefreshState(!mState.value);
    }

    private boolean getLteEnabled() {
        int defaultDataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
        int settingsNetworkMode = android.provider.Settings.Global.getInt(
                mContext.getContentResolver(),
                android.provider.Settings.Global.PREFERRED_NETWORK_MODE + defaultDataSubId,
                RILConstants.PREFERRED_NETWORK_MODE);
        boolean lteEnable = (settingsNetworkMode == RILConstants.NETWORK_MODE_LTE_GSM_WCDMA
                || settingsNetworkMode == RILConstants.NETWORK_MODE_LTE_ONLY);
        return lteEnable;
    }

    @Override
    protected void handleUpdateState(BooleanState state, Object arg) {
        if (!isAirplaneModeOn()) {
            updateLteEnabledState();
        }
        state.value = mLteEnabled;
//      state.visible = true;
        state.label = mContext.getString(R.string.quick_settings_lte_service_label);
        int defaultDataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
        int phoneId = SubscriptionManager.getPhoneId(defaultDataSubId);
        int slotId = SubscriptionManager.getSlotId(defaultDataSubId);
        boolean isPrimaryCardUsim = mTelephonyManagerEx.isUsimCard(phoneId);
        boolean isPrimaryCardReady =
                mTelephonyManager.getSimState(slotId) == TelephonyManager.SIM_STATE_READY;
        mLteAvailable = isPrimaryCardUsim && isPrimaryCardReady;

        Log.d(TAG, "handleUpdateState: mLteEnabled = " + mLteEnabled +
                " isDefaultDataCardUsim = " + isPrimaryCardUsim +
                " isDefaultDataCardReady = " + isPrimaryCardReady);

        if (mLteEnabled && mLteAvailable
                && !isAirplaneModeOn()) {
            // SPRD: Replace the LTE switch icon.
            state.icon = ResourceIcon.get(R.drawable.ic_qs_4g_on_ex);
            state.contentDescription =  mContext.getString(
                    R.string.accessibility_quick_settings_lte_on);
        } else {
            // SPRD: Replace the LTE switch icon.
            state.icon = ResourceIcon.get(R.drawable.ic_qs_4g_off_ex);
            state.contentDescription =  mContext.getString(
                    R.string.accessibility_quick_settings_lte_off);
        }
    }

    public void updateLteEnabledState() {
        final int defaultDataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
        boolean lteEnabled = false;

        if (SubscriptionManager.isValidSubscriptionId(defaultDataSubId)) {
            /* SPRD: query LTE status from AP instead of from BP with AT commands,
             * as the latter method may cause ANR for multiple blocked thread.*/;
            lteEnabled = getLteEnabled();

            if (mLteEnabled != lteEnabled) {
                mState.value = mLteEnabled = lteEnabled;
                if (mQSTileView != null) {
                    mQSTileView.onStateChanged(mState);
                }
            }
            /* @} */
        }

    }

    @Override
    protected String composeChangeAnnouncement() {
        if (mState.value) {
            return mContext.getString(R.string.accessibility_quick_settings_lte_changed_on);
        } else {
            return mContext.getString(R.string.accessibility_quick_settings_lte_changed_off);
        }
    }

    public void setListening(boolean listening) {
        if (mListening == listening) return;
        mListening = listening;
        if (listening) {
            final IntentFilter filter = new IntentFilter();
            filter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
            filter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
            mContext.registerReceiver(mReceiver, filter);
        } else {
            mContext.unregisterReceiver(mReceiver);
        }
    }

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (TelephonyIntents.ACTION_SIM_STATE_CHANGED.equals(intent.getAction())) {
                refreshState();
            } else if (Intent.ACTION_AIRPLANE_MODE_CHANGED.equals(intent.getAction())) {
                refreshState();
            }
        }
    };

    public boolean isAirplaneModeOn() {
        return Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 0) != 0;
    }

}

