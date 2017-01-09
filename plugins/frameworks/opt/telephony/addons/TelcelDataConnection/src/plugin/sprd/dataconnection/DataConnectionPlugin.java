package plugin.sprd.dataconnection;

import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.WindowManager;
import com.android.internal.telephony.dataconnection.DcFailCause;
import com.android.internal.telephony.plugin.DataConnectionUtils;
import com.android.sprd.telephony.RadioInteractor;

public class DataConnectionPlugin extends DataConnectionUtils implements
        AddonManager.InitialCallback {

    private static final String TAG = "TelcelDataConnection";
    private Context mAddonContext;

    public DataConnectionPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean supportSpecialClearCode() {
        Log.d(TAG, "supportSpecialClearCode : true");
        return true;
    }

    @Override
    public boolean isSpecialCode(DcFailCause fc) {
        return fc == DcFailCause.USER_AUTHENTICATION
                || fc == DcFailCause.SERVICE_OPTION_NOT_SUBSCRIBED;
    }

    @Override
    public AlertDialog getErrorDialog(DcFailCause fc) {
        int res = -1;
        if (fc == DcFailCause.USER_AUTHENTICATION) {
            res = R.string.failcause_user_authentication;
        } else if (fc == DcFailCause.SERVICE_OPTION_NOT_SUBSCRIBED) {
            res = R.string.failcause_service_option_not_subscribed;
        }

        if (res == -1) {
            return null;
        }

        AlertDialog dialog = new AlertDialog.Builder(mAddonContext,
                AlertDialog.THEME_DEVICE_DEFAULT_LIGHT)
                .setMessage(res)
                .setPositiveButton(android.R.string.ok, null)
                .create();
        dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }

    @Override
    protected void onRadioInteractorConnected(RadioInteractor ri) {
        final RadioInteractor radioInteractor = ri;
        new Thread() {
            public void run() {
                TelephonyManager telMgr = TelephonyManager.from(mAddonContext);
                int numPhone = telMgr.getPhoneCount();
                for (int i = 0; i < numPhone; i++) {
                    radioInteractor.enableRauNotify(i);
                }
                Log.d(TAG, "enableRauNotify() done");
            }
        }.start();
    }
}
