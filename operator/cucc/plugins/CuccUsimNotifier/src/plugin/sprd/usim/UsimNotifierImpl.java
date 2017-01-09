package plugin.sprd.usim;

import com.android.internal.telephony.UsimNotifier;

import android.app.AddonManager;
import android.app.AlertDialog;
import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.ApplicationInfo;
import android.content.res.Resources;
import android.os.Handler;
import android.os.Message;
import android.os.Process;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.text.TextUtils;
import android.util.Log;
import android.content.Intent;

public class UsimNotifierImpl extends UsimNotifier implements AddonManager.InitialCallback {
    private Resources mAddonResources;
    private Context mContext;
    private AlertDialog mAppGoOnDialog = null;
    private boolean mUserChoosed = false;
    private Activity mCurResumedAct;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonResources = context.getResources();
        mContext = context;
        return clazz;
    }

    private static final int MSG_PROMPT_DIALOG = 1;
    private Handler mHandler = new Handler() {
        public void handleMessage(android.os.Message msg) {
            if (msg.obj != null) {
                promptAPPDialog((Activity) msg.obj);
            };
        };
    };

    boolean isBeginActivity(String name) {
        if (name == null) {
            return false;
        }
        return name.equals("com.infinit.wostore.ui.BeginActivity")
                || name.endsWith("com.sinovatech.unicom.ui.WelcomeClient");
    }

    private void promptAPPDialog(Activity activity) {
        if (activity == null) {
            return;
        }
        if (isBeginActivity(activity.getClass().getName())) {
            return;
        }
        if (mCurResumedAct != null
            && (mCurResumedAct.toString().equals(activity.toString()))){
            // dialog's activity is the same, just return
            return;
        }
        if (mUserChoosed) {
            // user is already choose one
            return;
        }
        if (mAppGoOnDialog != null && mAppGoOnDialog.isShowing()) {
            // this app's pre activity is already show the dialog.dismiss it
            mAppGoOnDialog.dismiss();
        }
        mCurResumedAct = activity;

        AlertDialog.Builder b = new AlertDialog.Builder(mCurResumedAct);
        b.setTitle(mAddonResources.getString(R.string.usim_recommanded));
        b.setMessage(mAddonResources.getString(R.string.usim_recommanded_for_cucc_app));
        b.setCancelable(false);
        b.setNegativeButton(mAddonResources.getString(R.string.pick_no), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                Log.d("[UsimNotifierImpl]","on click cancel");
                mCurResumedAct.finish();
                Intent intent = new Intent(Intent.ACTION_MAIN);
                intent.addCategory(Intent.CATEGORY_HOME);
                mContext.startActivity(intent);
                Process.killProcess(Process.myPid());
            }
        });
        b.setPositiveButton(mAddonResources.getString(R.string.pick_yes), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                mUserChoosed = true;
            }
        });
        mAppGoOnDialog = b.create();
        mAppGoOnDialog.show();
    }

    @Override
    public void startNotify(Activity activity) {
        if (activity != null) {
            SubscriptionManager subManager = SubscriptionManager.from(activity.getApplicationContext());
            SubscriptionInfo dataSubInfoRecords = subManager.getDefaultDataSubscriptionInfo();
            if (dataSubInfoRecords == null) return;
            String iccId = dataSubInfoRecords.getIccId();
            Log.d("[UsimNotifierImpl]", "startNotify --->"+iccId);
            if (TextUtils.isEmpty(iccId)) return;
            if (!iccId.startsWith("898601") && !iccId.startsWith( "898609")) {
                Message message = Message.obtain();
                message.obj = activity;
                message.what = MSG_PROMPT_DIALOG;
                mHandler.sendMessage(message);
            }
        }
    }
}
