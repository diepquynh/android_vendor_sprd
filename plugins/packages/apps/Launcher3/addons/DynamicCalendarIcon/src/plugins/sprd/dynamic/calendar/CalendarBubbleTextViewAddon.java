package plugins.sprd.dynamic.calendar;

import android.content.Context;
import com.sprd.launcher3.utils.DynamicCalendarUtil;
import android.app.AddonManager;

public class CalendarBubbleTextViewAddon extends DynamicCalendarUtil implements
        AddonManager.InitialCallback {
    private static final String TAG = "CalendarBubbleTextViewAddon";

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public CalendarBubbleTextViewAddon() {
    }

    public boolean isDynamicCalendar() {
        return true;
    }

}
