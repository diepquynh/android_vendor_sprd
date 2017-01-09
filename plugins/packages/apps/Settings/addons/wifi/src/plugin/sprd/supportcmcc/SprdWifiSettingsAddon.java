/** Created by Spreadst */

package plugin.sprd.supportcmcc;

import android.app.AddonManager;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.wifi.WifiManager;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.preference.CheckBoxPreference;
import android.provider.Settings;
import android.util.Log;
import android.widget.Toast;

import com.android.settings.wifi.SprdWifiSettingsAddonStub;

public class SprdWifiSettingsAddon extends SprdWifiSettingsAddonStub implements
        AddonManager.InitialCallback {

    private static final String TAG = "SprdWifiSettingsAddon";
    private Context mContext;
    IWifiSettings mIWifiSettings;

    private ServiceConnection mConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName className,
                IBinder service) {
            mIWifiSettings = IWifiSettings.Stub.asInterface(service);
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0) {
            mIWifiSettings = null;
        }
    };

    public SprdWifiSettingsAddon() {
    }

    public boolean isSupportCmcc() {
        return WifiManager.SUPPORT_CMCC;
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        Intent intent = new Intent(context, WifiSettingsService.class);
        context.bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
        return clazz;
    }

    public void startTrustedApListActivty() {
        Intent intent = new Intent(mContext, WifiTrustedAPList.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(intent);
    }

    public void setManulConnectFlags(boolean enabled) {
        try {
            mIWifiSettings.setManulConnectFlags(enabled);
        } catch (RemoteException re) {
            Log.d(TAG, "RemoteException : " + re);
        }
    }

    public boolean isWifiConnectingOrConnected() {
        return WifiConnectionPolicy.isWifiConnectingOrConnected();
    }

    public void initWifiConnectionPolicy() {
        WifiConnectionPolicy.init(mContext);
    }

    public void setMobileToWlanPolicy(String value) {
        try {
            int policyValue = Integer.parseInt(value);
            WifiConnectionPolicy.setMobileToWlanPolicy(mContext, policyValue);
        } catch (NumberFormatException e) {
            Toast.makeText(mContext, R.string.mobile_to_wlan_policy_error,
                    Toast.LENGTH_SHORT).show();
            return;
        }
        try {
            mIWifiSettings.resetTimer();
        } catch (RemoteException re) {
            Log.d(TAG, "RemoteException : " + re);
        }
    }

    public int getMobileToWlanPolicy() {
        return WifiConnectionPolicy.getMobileToWlanPolicy(mContext);
    }

    public void resetWifiPolicyDialogFlag() {
        ContentResolver cr = mContext.getContentResolver();
        Settings.Global.putInt(cr, WifiConnectionPolicy.DIALOG_WLAN_TO_WLAN, 0);
        Settings.Global.putInt(cr, WifiConnectionPolicy.DIALOG_MOBILE_TO_WLAN_ALWAYS_ASK, 0);
        Settings.Global.putInt(cr, WifiConnectionPolicy.DIALOG_MOBILE_TO_WLAN_MANUAL, 0);
        Settings.Global.putInt(cr, WifiConnectionPolicy.DIALOG_WLAN_TO_MOBILE, 0);
        Settings.Global.putInt(cr,WifiConnectionPolicy.AIRPLANE_MODE_WIFI_NOTIFICATION_FLAG, 0);
        try {
            mIWifiSettings.resetTimer();
        } catch (RemoteException re) {
            Log.d(TAG, "RemoteException : " + re);
        }
    }

    public boolean showDialogWhenConnectCMCC() {
        return Settings.Global.getInt(mContext.getContentResolver(), WifiConnectionPolicy.DIALOG_CONNECT_TO_CMCC, 1) == 1;
    }

    public void setConnectCmccDialogFlag(boolean enabled) {
        Settings.Global.putInt(mContext.getContentResolver(), WifiConnectionPolicy.DIALOG_CONNECT_TO_CMCC, enabled ? 1 : 0);
    }
}
