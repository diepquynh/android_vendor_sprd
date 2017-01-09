
package plugin.sprd.systemuiaddon;

import android.app.AddonManager;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.os.SystemProperties;
import android.util.Log;
import com.android.systemui.R;
import com.android.systemui.statusbar.KeyguardAffordanceView;
import com.android.systemui.statusbar.phone.ActivityStarter;
import com.android.systemui.statusbar.phone.PhoneStatusBar;
import android.view.WindowManager;
import com.sprd.systemui.SystemuiFeatureUtil;
import com.android.systemui.statusbar.KeyguardIndicationController;

public class AddonSystemuiUtils extends SystemuiFeatureUtil implements AddonManager.InitialCallback {

    private static final Intent PROFILE_INTENT = new Intent("com.sprd.action.AUDIO_PROFILE");
    private static final String TAG = "AddonSystemuiUtils";

    public AddonSystemuiUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public boolean launchAudioProfile(ActivityStarter activityStarter, Context context) {
        Log.d(TAG, "launchAudioProfile");
        AudioManager audioMgr = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        if (audioMgr != null) {
            audioMgr.setRingerMode(AudioManager.RINGER_MODE_VIBRATE);
        }
        activityStarter.startActivity(PROFILE_INTENT, false /* dismissShade */);
        return true;
    }

    public void changeCameraToProfile(KeyguardAffordanceView keyguardAffordanceView) {
        Log.d(TAG, "changeCameraToProfile");
        keyguardAffordanceView.setImageResource(R.drawable.ic_shock_alt_24dp);
    }
    /* SPRD: fixbug412801 Change the location of the headsUp @{ */
    public void changeHeadsUpbelowStatusBar(WindowManager.LayoutParams lp, PhoneStatusBar bar) {
        Log.d(TAG, "changeHeadsUpbelowStatusBar");
        lp.y = bar.getStatusBarHeight();
    }
    /* @} */
    /* SPRD: fixbug415156 Change the hint when click the Profile icon @{ */
    public boolean changeProfileHint(KeyguardIndicationController keyguardIndicationController) {
        Log.d(TAG, "changeProfileHint");
        keyguardIndicationController.showTransientIndication(R.string.vibrate_hint);
        return true;
    }
    /* @} */

    //SPRD: 541291 modify the SystemProperties to plugins
    public boolean isCMCC() {
       return true;
    }

}
