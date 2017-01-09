package plugin.sprd.orangefeatures;

import android.app.Activity;
import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionInfo;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;

import com.android.settings.sim.SimDialogActivity;
import com.android.settings.inputmethod.InputMethodAndLanguageSettings;
import com.android.settings.inputmethod.plugin.SimLangListUtils;

import plugin.sprd.orangefeatures.SimLangListActivity;
import java.util.List;

public class AddonSimLangListPlugin extends SimLangListUtils implements AddonManager.InitialCallback {
    private Context mAddonContext;
    private AlertDialog mLeavingDialog;
    private int mContemporaryCardId = 0;
    private static final String KEY_SIM_STORED_LANG_EDIT = "simcard_stored_lang_edit";
    public AddonSimLangListPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void setPreferenceByCardForOrange(InputMethodAndLanguageSettings baseClass){
        TelephonyManager tm = TelephonyManager.from(baseClass.getActivity());
        int phoneCount = tm.getPhoneCount();
        boolean hasSimCard = false;
        for(int i = 0; i< phoneCount; i++) {
            if(tm.hasIccCard(i)) {
                hasSimCard = true;
                mContemporaryCardId = i;
            }
        }
        if(!hasSimCard && (baseClass.findPreference(KEY_SIM_STORED_LANG_EDIT) != null) ) {
            baseClass.findPreference(KEY_SIM_STORED_LANG_EDIT).setEnabled(false);
        }
        /* @} */
    }
    @Override
    public void startActivityByCards(Context context){
        ComponentName targetComponent = null;
        Intent intent = null;
        TelephonyManager tm = TelephonyManager.from(context);
        List<SubscriptionInfo> availableSubInfoList = SubscriptionManager.from(context).getActiveSubscriptionInfoList();
        if(tm.getPhoneCount() > 1 && availableSubInfoList.size()>1) {
            intent = new Intent(context, SimDialogActivity.class);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.putExtra(SimDialogActivity.DIALOG_TYPE_KEY, 5);
        } else {
            intent = new Intent(Intent.ACTION_MAIN);
            if(tm.getPhoneCount() > 1){
                intent.putExtra("sub_id", (SubscriptionManager.getSubId(mContemporaryCardId))[0]);
            }
            intent.setComponent(new ComponentName("plugin.sprd.orangefeatures","plugin.sprd.orangefeatures.SimLangListActivity"));
        }
        context.startActivity(intent);
    }

    @Override
    public void setTitle(AlertDialog.Builder builder ,int title){
        builder.setTitle(title);
    }

    @Override
    public void startSimLangListActivity(Context context,int chooseId, List<SubscriptionInfo> subInfoList){
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.setComponent(new ComponentName("plugin.sprd.orangefeatures","plugin.sprd.orangefeatures.SimLangListActivity"));
        final SubscriptionInfo sir = subInfoList.get(chooseId);
        int phoneId = sir.getSimSlotIndex();
        intent.putExtra("sub_id", (SubscriptionManager.getSubId(phoneId))[0]);
        context.startActivity(intent);
    }
}
