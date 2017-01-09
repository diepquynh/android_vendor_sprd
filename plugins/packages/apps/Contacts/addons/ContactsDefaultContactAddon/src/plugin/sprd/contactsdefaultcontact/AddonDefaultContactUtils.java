package plugin.sprd.contactsdefaultcontact;

import android.app.AddonManager;
import android.content.Context;

import com.sprd.contacts.plugin.DefaultContactUtils;

public class AddonDefaultContactUtils extends DefaultContactUtils implements AddonManager.InitialCallback {

    private static final String TAG = "DefaultContactAddon";
    private Context mAddonContext;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

}
