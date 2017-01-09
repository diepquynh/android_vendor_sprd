package plugin.sprd.systemuiaudioprofile;

import android.app.ActivityManagerNative;
import android.app.AddonManager;
import android.content.Context;

import com.android.systemui.qs.QSTile;
import com.android.systemui.qs.QSTile.Host;
import com.android.systemui.qs.tiles.IntentTile;
import com.sprd.systemui.SystemUIAudioProfileUtils;

import android.content.IntentFilter;
import android.content.Intent;
import android.content.pm.UserInfo;
import android.os.BatteryManager;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.UserManager;
import android.util.Log;
import android.view.View;


public class SystemUIAudioProfile extends SystemUIAudioProfileUtils implements
        AddonManager.InitialCallback {
    private static final String TAG = "SystemUIAudioProfile";
    private Context mAddonContext;

    public SystemUIAudioProfile() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean isSupportAudioProfileTile() {
        /* SPRD: Added for bug 602041,remove audio profile quick setting label under guest mode @{ */
        if (!isOwner()) {
            return false;
        } else {
            return true;
        }
        /* @} */
    }

    /* SPRD: Added for bug 602041,remove audio profile quick setting label under guest mode @{ */
    public boolean isOwner() {
        UserManager userManager = (UserManager) mAddonContext.getSystemService(Context.USER_SERVICE);
        UserInfo currentUser;
        try {
            currentUser = ActivityManagerNative.getDefault().getCurrentUser();
        } catch (RemoteException e) {
            Log.e(TAG, "Couldn't wipe session because ActivityManager is dead");
            return false;
        }
        if(currentUser.isAdmin()){
            return true;
        } else {
            return false;
        }
    }
    /* @} */

    public QSTile<?> createAudioProfileTile(Host host, Context context) {
        return new AudioProfileTile(host);
    }
}
