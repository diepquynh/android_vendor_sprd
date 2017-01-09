
package plugin.sprd.supportcucc;

import android.app.AddonManager;
import android.content.Context;
import android.telephony.SubscriptionInfo;
import android.telephony.TelephonyManager;
import android.telephony.SubscriptionManager;
import android.provider.Settings;
import com.android.internal.telephony.Phone;
import com.android.settings.CUCCPluginsUtils;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
import android.preference.PreferenceScreen;

public class SettingsNotEditableForCUCC extends CUCCPluginsUtils implements
        AddonManager.InitialCallback {

    private static final String TAG = "SettingsNotEditableForCUCC";
    private Context mAddonContext;

    public SettingsNotEditableForCUCC() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void setNotEditableForCuccCard(boolean mIsEditable, PreferenceScreen prefSet) {
    Log.d(TAG, "[setNotEditableForCuccCard] mIsEditable: " + mIsEditable);
    if(!mIsEditable) {
         prefSet.setEnabled(false);
    }
  }

    public boolean getIfEditable(boolean mIsEditable) {
        Log.d(TAG, "[getIfEditable] mIsEditable: " + mIsEditable);
        return mIsEditable;
    }
}