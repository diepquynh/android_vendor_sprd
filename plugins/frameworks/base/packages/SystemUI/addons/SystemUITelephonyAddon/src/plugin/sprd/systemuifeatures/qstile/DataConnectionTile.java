package plugin.sprd.systemuifeatures.qstile;


import java.util.List;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.provider.Settings;
import android.os.SystemProperties;
import android.provider.Settings.Global;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telecom.TelecomManager;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import com.android.settingslib.net.DataUsageController;
import com.android.settingslib.net.DataUsageController.DataUsageInfo;
import com.android.systemui.R;
import com.android.systemui.qs.GlobalSetting;
import com.android.systemui.qs.QSTile;
import com.android.systemui.qs.tiles.DataUsageDetailView;
import com.android.systemui.statusbar.policy.NetworkController;
//import com.android.systemui.statusbar.policy.NetworkController.MobileDataController;
//import com.android.systemui.statusbar.policy.NetworkController.MobileDataController.DataUsageInfo;
import com.android.systemui.statusbar.policy.TelephonyIconsEx;
import com.android.internal.logging.MetricsLogger;

/** Quick settings tile: DataConnection **/
public class DataConnectionTile extends QSTile<QSTile.BooleanState> {
    private static final Intent CELLULAR_SETTINGS = new Intent().setComponent(new ComponentName(
            "com.android.settings", "com.android.settings.Settings$DataUsageSummaryActivity"));

    private final NetworkController mController;
    private final DataUsageController mDataController;
    private final CellularDetailAdapter mDetailAdapter;

    private final GlobalSetting mDataSetting;
    private TelephonyManager mTelephonyManager;
    private SubscriptionManager mSubscriptionManager;
    public static final int QS_DATACONNECTION = 411;
    public static final int QS_DATACONNECTION_DETAILS = 412;
    private boolean mListening;

    public DataConnectionTile(Host host) {
        super(host);
        mController = host.getNetworkController();
        mDataController = mController.getMobileDataController();
        mDetailAdapter = new CellularDetailAdapter();

        mTelephonyManager = TelephonyManager.from(mContext);
        mSubscriptionManager = SubscriptionManager.from(mContext);

        mDataSetting = new GlobalSetting(mContext, mHandler, Global.MOBILE_DATA) {
            @Override
            protected void handleValueChanged(int value) {
                Log.d(TAG, "mDataSetting handleValueChanged");
                mState.value = mTelephonyManager.getDataEnabled();
                handleRefreshState(value);
                mDetailAdapter.setMobileDataEnabled(mState.value);
            }
        };
    }

    @Override
    /*protected*/public BooleanState newTileState() {
        return new BooleanState();
    }

    @Override
    public DetailAdapter getDetailAdapter() {
        return mDetailAdapter;
    }

    @Override
    public void setListening(boolean listening) {
        if (mListening == listening) return;
        mListening = listening;
        if (listening) {
            final IntentFilter filter = new IntentFilter();
            filter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
            mContext.registerReceiver(mReceiver, filter);
        } else {
            mContext.unregisterReceiver(mReceiver);
        }
        mDataSetting.setListening(listening);
    }

    /* SPRD: Bug 594164 Add listening when back from Detail Setting. @{ */
    public void setDetailListening(boolean listening) {
        if (mListening == listening) return;
        refreshState();
    }
    /* @} */

    @Override
    public Intent getLongClickIntent() {
        /* SPRD: Bug 474780 CMCC version hidden data traffic interface. @{ */
        if (!mContext.getResources().getBoolean(R.bool.config_showDataUsageSummary)) {
            return new Intent();
        } else {
            return CELLULAR_SETTINGS;
        }
        /* @} */
    }

    @Override
    public CharSequence getTileLabel() {
        return mContext.getString(R.string.quick_settings_data_connection_label);
    }

    @Override
    public int getMetricsCategory() {
        return QS_DATACONNECTION;
    }

    public boolean isAvailable() {
        return true;
    }

    /* SPRD: During the call,user can't switch data tile for bug610656. @{ */
    private boolean isInCall() {
        return getTelecommManager().isInCall();
    }

    private TelecomManager getTelecommManager() {
        return (TelecomManager) mContext.getSystemService(Context.TELECOM_SERVICE);
    }
    /* @} */

    @Override
    protected void handleClick() {
        Log.d(TAG, "handleClick");
        MetricsLogger.action(mContext, getMetricsCategory(), !mState.value);
        /* SPRD: modify by BUG 620467 @{ */
        int defaultDataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
        int defaultDataVoiceReg = TelephonyManager.getDefault().getVoiceNetworkType(defaultDataSubId);
        int netWorkClass = TelephonyManager.getNetworkClass(defaultDataVoiceReg);
        /* @} */
        if (mDataController.isMobileDataSupported()) {
            if (isAirplaneModeOn()) {
                mHost.collapsePanels();
                Toast.makeText(mContext, R.string.toggle_data_error_airplane, Toast.LENGTH_SHORT)
                        .show();
                return;
            }

            /* SPRD: During the call,user can't switch data tile for bug610656. @{ */
            if (isInCall() && netWorkClass < TelephonyManager.NETWORK_CLASS_3_G) {
                mHost.collapsePanels();
                Toast.makeText(mContext, R.string.data_error_incall, Toast.LENGTH_SHORT)
                        .show();
                return;
            }
            /* @} */

            if (isDefaultDataSimAvailable()) {
                boolean enabled = !mTelephonyManager.getDataEnabled();
                toggleDataConnectionToDesired(enabled);
                handleRefreshState(!mState.value);
            }
        }
    }

    private void toggleDataConnectionToDesired(boolean enabled) {
        mState.value = enabled;
        mDataController.setMobileDataEnabled(enabled);
    }

    private boolean isDefaultDataSimAvailable() {
        int defaultDataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
        int defaultDataPhoneId = SubscriptionManager.getSlotId(defaultDataSubId);
        boolean isDefaultDataSimReady = SubscriptionManager
                .getSimStateForSlotIdx(defaultDataPhoneId) == TelephonyManager.SIM_STATE_READY;
        boolean isDefaultDataValid = SubscriptionManager.isValidPhoneId(defaultDataPhoneId);
        Log.d(TAG, "defaultDataSubId = " + defaultDataSubId + " isDefaultDataSimReady = "
                + isDefaultDataSimReady + " isDefaultDataStandby = " + isDefaultDataValid);
        return isDefaultDataSimReady && isDefaultDataValid;
    }

    @Override
    protected void handleSecondaryClick() {
        Log.d(TAG, "handleSecondaryClick");
        if (mDataController.isMobileDataSupported() && isDefaultDataSimAvailable()) {
            showDetail(true);
        } else {
            mHost.startActivityDismissingKeyguard(CELLULAR_SETTINGS);
        }
    }

    @Override
    protected void handleUpdateState(BooleanState state, Object arg) {
        final boolean dataConnected = mTelephonyManager.getDataEnabled();
        state.value = dataConnected;
//      state.visible = true;
        state.label = mContext.getString(R.string.quick_settings_data_connection_label);
        Log.d(TAG, "dataConnected = " + dataConnected);
        if (dataConnected
                && isDefaultDataSimAvailable()
                && !isAirplaneModeOn()) {
            state.icon = ResourceIcon.get(R.drawable.ic_qs_mobile_data_on);
            state.contentDescription = mContext.getString(
                    R.string.accessibility_quick_settings_data_on);
        } else {
            state.icon = ResourceIcon.get(R.drawable.ic_qs_mobile_data_off);
            state.contentDescription = mContext.getString(
                    R.string.accessibility_quick_settings_data_off);
        }
    }

    @Override
    protected String composeChangeAnnouncement() {
        if (mState.value) {
            return mContext.getString(R.string.accessibility_quick_settings_data_changed_on);
        } else {
            return mContext.getString(R.string.accessibility_quick_settings_data_changed_off);
        }
    }

    private final class CellularDetailAdapter implements DetailAdapter {

        @Override
        public CharSequence getTitle() {
            return mContext.getString(R.string.quick_settings_cellular_detail_title);
        }

        @Override
        public int getMetricsCategory() {
            return QS_DATACONNECTION_DETAILS;
        }

        @Override
        public Boolean getToggleState() {
            return mDataController.isMobileDataSupported()
                    ? mDataController.isMobileDataEnabled()
                    : null;
        }

        @Override
        public Intent getSettingsIntent() {
            return CELLULAR_SETTINGS;
        }

        @Override
        public void setToggleState(boolean state) {
            toggleDataConnectionToDesired(state);
        }

        @Override
        public View createDetailView(Context context, View convertView, ViewGroup parent) {
            final DataUsageDetailView v = (DataUsageDetailView) (convertView != null
                    ? convertView
                    : LayoutInflater.from(mContext).inflate(R.layout.data_usage, parent, false));
            final DataUsageInfo info = mDataController.getDataUsageInfo();
            if (info == null) return v;
            v.bind(info);
            return v;
        }

        public void setMobileDataEnabled(boolean enabled) {
            fireToggleStateChanged(enabled);
        }
    }
    public boolean isAirplaneModeOn() {
        return Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 0) != 0;
    }

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (Intent.ACTION_AIRPLANE_MODE_CHANGED.equals(intent.getAction())) {
                refreshState();
            }
        }
    };
}
