package com.sprd.settings;

import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;

import java.util.List;

import com.android.settings.sim.SimDialogActivity;
import com.sprd.settings.SettingsRelianceHelper;

public class SettingsReliancePlugin extends SettingsRelianceHelper implements
        AddonManager.InitialCallback {

    public static Context mContext;
    private static final String TAG = "SettingsReliancePlugin";

    public SettingsReliancePlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    /* SPRD: new featrue:Smart Dual SIM @{ */
    public boolean shouldShowSmartDualSimSOption() {
    Log.i(TAG , "Smart Dual Sim is support in Reliance");
    return true;
    }
    /* @} */
    public boolean displayConfirmDataDialog(final Context context, final SubscriptionInfo subscriptionInfo,
            final int currentDataPhoneId, final SimDialogActivity simDialogActivity) {
        Log.d(TAG, "displayConfirmDataDialog currentDataPhonId =" + currentDataPhoneId);
        Resources res = mContext.getResources();
        if (!isJioPrimaryCard(context, currentDataPhoneId)) {
            return false;
        }
        AlertDialog.Builder confirmDataDialog = new AlertDialog.Builder(context);
        confirmDataDialog
                .setTitle(res.getString(R.string.confirm_data_dialog_title, (subscriptionInfo.getSimSlotIndex() + 1)));
        confirmDataDialog.setMessage(res.getString(R.string.confirm_data_dialog_message));
        confirmDataDialog.setNegativeButton(res.getString(R.string.confirm_dialog_cancel),
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        simDialogActivity.finish();
                    }
                });
        confirmDataDialog.setPositiveButton(res.getString(R.string.confirm_dialog_OK),
                new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        Log.d(TAG, "confirm dialog onClick");
                        final SubscriptionManager subscriptionManager = SubscriptionManager.from(context);
                        subscriptionManager.setDefaultDataSubId(subscriptionInfo.getSubscriptionId());
                        simDialogActivity.finish();
                    }
                });
        AlertDialog datadialog = confirmDataDialog.create();
        datadialog.show();
        return true;
    }

     private boolean isJioPrimaryCard(Context context,int currentDataPhonId) {
         TelephonyManager teleMgr = TelephonyManager.from(context);
         String currentDataCardCarrierName = teleMgr.getSimOperatorNameForPhone(currentDataPhonId);
         Log.d(TAG, "sim operator name = " + currentDataCardCarrierName);
         if (currentDataCardCarrierName.startsWith("Jio")) {
             return true;
         }
         Log.d(TAG, "return false");
         return false;
     }
}
