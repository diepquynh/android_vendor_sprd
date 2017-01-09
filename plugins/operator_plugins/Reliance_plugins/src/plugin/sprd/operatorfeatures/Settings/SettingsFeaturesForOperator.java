package plugin.sprd.operatorfeatures.Settings;

import java.util.List;

import com.android.settings.sim.SimDialogActivity;
import com.sprd.settings.SettingsPluginsHelper;

import android.app.Activity;
import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.telephony.SubscriptionInfo;
import android.telephony.TelephonyManager;
import android.telephony.SubscriptionManager;
import android.text.TextUtils;
import android.util.Log;

import plugin.sprd.reliancefeatures.R;

public class SettingsFeaturesForOperator extends SettingsPluginsHelper
        implements AddonManager.InitialCallback {
    private Context mAddonContext;
    private static final String TAG = "SettingsFeaturesForOperator";

    public SettingsFeaturesForOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean displayConfirmDataDialog(final Context context,
           final SubscriptionInfo subscriptionInfo,final int currentDataSubId,final SimDialogActivity simDialogActivity) {
        TelephonyManager teleMgr = TelephonyManager.from(context);
        int primaryCardPhoneId = teleMgr.getPrimaryCard();
        int primaryCardSubId = getPrimaryCardSubId(context,primaryCardPhoneId);
        Log.d(TAG, "displayConfirmDataDialog currentDataSubId"
                + currentDataSubId + ",primaryCardSubId," + primaryCardSubId);
        Resources res = mAddonContext.getResources();
        if (primaryCardSubId == currentDataSubId) {
            if (!isJioPrimaryCard(context,primaryCardPhoneId)) {
                return false;
            }
            AlertDialog.Builder confirmDataDialog = new AlertDialog.Builder(
                    context);
            confirmDataDialog.setTitle(
                    res.getString(R.string.confirm_data_dialog_title,
                    (subscriptionInfo.getSimSlotIndex()+1)));
            confirmDataDialog.setMessage(
                    res.getString(R.string.confirm_data_dialog_message));
            confirmDataDialog.setNegativeButton(
                    res.getString(R.string.confirm_dialog_cancel), new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            simDialogActivity.finish();
                        }
                    });
            confirmDataDialog.setPositiveButton(
                    res.getString(R.string.confirm_dialog_OK),
                    new DialogInterface.OnClickListener() {

                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            Log.d(TAG, "confirm dialog onClick");
                            setDefaultDataSubId(context,
                                    subscriptionInfo.getSubscriptionId());
                            disableDataForOtherSubscriptions(context,
                                    subscriptionInfo.getSubscriptionId());
                            simDialogActivity.finish();
                        }
                    });
            AlertDialog datadialog = confirmDataDialog.create();
            datadialog.show();
            return true;
        }
        return false;
    }

    private int getPrimaryCardSubId(Context context,int primaryCardPhoneId) {
        int subId[] = SubscriptionManager.getSubId(primaryCardPhoneId);
        if (subId == null || subId.length == 0) {
            return SubscriptionManager.INVALID_PHONE_INDEX;
        }
        return subId[0];
    }

    private boolean isJioPrimaryCard(Context context,int primaryCardPhoneId) {
        TelephonyManager teleMgr = TelephonyManager.from(context);
        String primaryCardCarrierName = teleMgr.getSimOperatorNameForPhone(primaryCardPhoneId);
        Log.d(TAG, "sim operator name = " + primaryCardCarrierName);
        if (primaryCardCarrierName.startsWith("Jio")) {
            return true;
        }
        Log.d(TAG, "return false");
        return false;
    }


    private void setDefaultDataSubId(Context context, int subId) {
        final SubscriptionManager subscriptionManager = SubscriptionManager
                .from(context);
        subscriptionManager.setDefaultDataSubId(subId);
        TelephonyManager teleMgr = TelephonyManager.from(context);
        if (!teleMgr.getDataEnabled(subId)) {
            teleMgr.setDataEnabled(subId, true);
        }
    }

    private final void disableDataForOtherSubscriptions(Context context,
            int subId) {
        final SubscriptionManager subscriptionManager = SubscriptionManager
                .from(context);
        TelephonyManager teleMgr = TelephonyManager.from(context);
        List<SubscriptionInfo> subInfoList = subscriptionManager
                .getActiveSubscriptionInfoList();
        if (subInfoList != null) {
            for (SubscriptionInfo subInfo : subInfoList) {
                if (subInfo.getSubscriptionId() != subId) {
                    teleMgr.setDataEnabled(subInfo.getSubscriptionId(), false);
                }
            }
        }
    }
}