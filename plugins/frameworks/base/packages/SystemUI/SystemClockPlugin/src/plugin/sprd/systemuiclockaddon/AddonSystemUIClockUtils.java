
package plugin.sprd.systemuiclockaddon;

import android.app.ActivityManager;
import android.app.AddonManager;
import android.content.Context;

import com.sprd.systemui.SystemUIClockUtil;

public class AddonSystemUIClockUtils extends SystemUIClockUtil implements AddonManager.InitialCallback{
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }
    public boolean isAllDay(boolean is24){
        if(!is24){
            return true;
        }else{
            return false;
        }
    }
}
