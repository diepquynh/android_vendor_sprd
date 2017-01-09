
package plugin.sprd.simsupportoperator;

import android.app.AddonManager;
import android.content.Context;
import android.telephony.SubscriptionInfo;
import android.telephony.TelephonyManager;
import android.telephony.SubscriptionManager;
import android.provider.Settings;
import com.android.internal.telephony.Phone;
import com.android.settings.sim.OperatorPluginsUtils;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
import com.android.settings.R;

public class SettingsForOperator extends OperatorPluginsUtils implements
        AddonManager.InitialCallback {

    private Context mAddonContext;

    public SettingsForOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public void showMoreDetailsForOperator(SubscriptionInfo subInfoRecord,
            ImageView imageView, TextView textView) {

        try {
            Resources res = mAddonContext.getPackageManager().getResourcesForApplication(
                    "com.android.settings");
            imageView.setVisibility(View.VISIBLE);
            if (subInfoRecord.getSimSlotIndex() != SubscriptionManager.INVALID_SIM_SLOT_INDEX) {
                textView.setVisibility(View.VISIBLE);
            }

            String simNumber = "";
            int primaryCard = Settings.Global.getInt(mAddonContext.getContentResolver(),
                    Settings.Global.SERVICE_PRIMARY_CARD, -1);
            if (subInfoRecord.getSimSlotIndex() == primaryCard) {
                simNumber = res.getString(res.getIdentifier("com.android.settings" + ":string/" +
                        "main_card_slot", null, null));
            } else if (primaryCard != -1) {
                simNumber = res.getString(res.getIdentifier("com.android.settings" +
                        ":string/" + "gsm_card_slot", null, null));
            }
            simNumber += subInfoRecord.getNumber();
            textView.setText(simNumber);
        } catch (NameNotFoundException e) {
        }
    }
}
