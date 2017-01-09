package plugins.sprd.dynamic.deskclock;

import android.content.Context;
import com.sprd.launcher3.utils.DynamicDeskClockUtil;
import android.app.AddonManager;

public class ClockBubbleTextViewAddon extends DynamicDeskClockUtil implements
        AddonManager.InitialCallback {
    private static final String TAG = "ClockBubbleTextViewAddon";

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public ClockBubbleTextViewAddon() {
        //TODO
    }

    public boolean isDynamicDeskClock(){
        return true;
    }
}
