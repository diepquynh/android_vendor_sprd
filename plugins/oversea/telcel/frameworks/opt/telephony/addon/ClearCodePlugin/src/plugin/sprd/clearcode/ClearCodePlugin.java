package plugin.sprd.clearcode;

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
import com.android.internal.R;
import com.android.internal.telephony.dataconnection.DcFailCause;
import com.android.internal.telephony.plugin.ClearCodeAndRAUUtils;
import com.android.sprd.telephony.RadioInteractor;

public class ClearCodePlugin extends ClearCodeAndRAUUtils implements
        AddonManager.InitialCallback {

    private static final String TAG = "ClearCodeAndRAUUtils";
    private Context mAddonContext;
    private static final String RI_SERVICE_NAME =
            "com.android.sprd.telephony.server.RADIOINTERACTOR_SERVICE";
    private static final String RI_SERVICE_PACKAGE =
            "com.android.sprd.telephony.server";

    public ClearCodePlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        bind(context);
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

    private ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            Log.d(TAG, "RadioInteractor service connected: service=" + service);
            new Thread() {
                public void run() {
                    TelephonyManager telMgr = TelephonyManager.from(mAddonContext);
                    int numPhone = telMgr.getPhoneCount();
                    RadioInteractor ri = new RadioInteractor(mAddonContext);
                    for (int i = 0; i < numPhone; i++) {
                        ri.enableRauNotify(i);
                    }
                    Log.d(TAG, "enableRauNotify() done");
                }
            }.start();
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {}
    };

    private void bind(Context context) {
        Intent serviceIntent = new Intent(RI_SERVICE_NAME);
        serviceIntent.setPackage(RI_SERVICE_PACKAGE);
        Log.d(TAG, "bind RI service");
        context.bindService(serviceIntent, mConnection, Context.BIND_AUTO_CREATE);
    }
}
