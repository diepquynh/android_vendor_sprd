package plugin.sprd.usim;

import android.app.Activity;
import android.app.AddonManager;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.res.Resources;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.Message;
import android.os.Process;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.WindowManager;

import com.sprd.internal.telephony.UsimNotifier;

public class UsimNotifierImpl extends UsimNotifier implements AddonManager.InitialCallback {
    private Resources mAddonResources;
    private Context mContext;
    private AlertDialog mAppGoOnDialog = null;
    private boolean mUserChoosed = false;
    private Activity mCurResumedAct;
    private static final int MSG_PROMPT_DIALOG = 1;
    private static final String IS_CHINA_UNICOM = "898601";

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonResources = context.getResources();
        mContext = context;
        return clazz;
    }

    private Handler mHandler = new Handler() {
        public void handleMessage(android.os.Message msg) {
            switch (msg.what) {
            case MSG_PROMPT_DIALOG:
                if (msg.obj != null) {
                    promptAPPDialog((Activity)msg.obj);
                }
                break;
            }
        }
    };

    boolean isWoStoreBeginActivity(String name) {
        if (name == null) {
            return false;
        }
        return name.equals("com.infinit.wostore.ui.BeginActivity");
    }

    private void promptAPPDialog(Activity activity) {
        if (activity == null) {
            return;
        }
        if (isWoStoreBeginActivity(activity.getClass().getName())) {
            return;
        }
        if (mCurResumedAct != null
            && (mCurResumedAct.toString().equals(activity.toString())))
        {
            // dialog's activity is the same, just return
            return;
        }
        if (mUserChoosed) {
            // user is already choose one
            return;
        }
        if (mAppGoOnDialog != null) {
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
                mCurResumedAct.finish();
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
        int subId = SubscriptionManager.getDefaultDataSubId();
        TelephonyManager tm = TelephonyManager.from(mContext);
        if (SubscriptionManager.isValidSubscriptionId(subId)){
            String iccId = tm.getSimSerialNumber(subId);
            if (!iccId.isEmpty() && iccId != null && iccId.length() >= 6) {
                String usimRecordTag = iccId.substring(0,6);
                Log.d("[startNotify]", "usimRecordTag = " + usimRecordTag);
                if (!usimRecordTag.equals(IS_CHINA_UNICOM) && !mUserChoosed) {
                    Message msg = new Message();
                    msg.obj = activity;
                    msg.what = MSG_PROMPT_DIALOG;
                    mHandler.sendMessageDelayed(msg, 200);
                }
            }
        }
    }
}
