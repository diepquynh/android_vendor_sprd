
package addon.sprd.AddonDeskClockStreamMedia;

import android.app.AddonManager;
import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Context;
import android.util.Log;
import android.content.res.Resources;
import com.sprd.deskclock.AlarmStreamMediaPlugin;

public class AddonDeskClockStreamMedia extends AlarmStreamMediaPlugin implements
        AddonManager.InitialCallback {
    Resources r;
    private static final String TAG = "AddonDeskClockStreamMedia";
    private static final int FIRED_STATE = 5;
    private static Context mAddonContext;

    public AddonDeskClockStreamMedia() {
       Log.d(TAG, "AddonDeskClockStreamMedia AddonDeskClockStreamMedia");
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        Log.d(TAG, "AddonDeskClockStreamMedia onCreateAddon");
        return clazz;
    }
    public boolean alarmStreamMediaInteract(int alarmState) {
        Log.d(TAG, "AddonDeskClockStreamMedia alarmStreamMediaInteract");
        return (FIRED_STATE == alarmState && isLiveMoveTopActivity(mAddonContext));
    }

    public boolean isLiveMoveTopActivity(Context context) {
        Log.d(TAG, "AddonDeskClockStreamMedia isLiveMoveTopActivity");
        ActivityManager am = (ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE);
        ComponentName cn = am.getRunningTasks(2).get(0).topActivity;
        String activityName = cn.getClassName();

        if("com.android.gallery3d.app.MovieActivity".equals(activityName)) {
            return true ;
        } else {
            return false ;
        }
    }
}
