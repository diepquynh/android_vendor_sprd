package plugin.sprd.keyguardaddon;

import com.sprd.keyguard.KeyguardSupportAmPm;

import android.app.AddonManager;
import android.content.Context;
import android.text.format.DateFormat;
import android.content.res.Resources;
import android.content.res.Configuration;
import java.util.Locale;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.AbsoluteSizeSpan;
import android.text.style.StyleSpan;
import android.text.style.TypefaceSpan;
import android.widget.TextClock;
import android.content.res.Resources;

public class KeyguardPluginForAmPm extends KeyguardSupportAmPm implements AddonManager.InitialCallback {

    public static final String LOG_TAG = "KeyguardPluginForAMPM";

    public KeyguardPluginForAmPm() {
    }
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public void setFormat12Hour(int textSize,TextClock clockView){
        int amPmFontSize = (int) textSize;
        String skeleton = "hma";
        String pattern = DateFormat.getBestDateTimePattern(Locale.getDefault(), skeleton);
        // Remove the am/pm
        if (amPmFontSize <= 0) {
            pattern.replaceAll("a", "").trim();
        }
        // Replace spaces with "Hair Space"
        pattern = pattern.replaceAll(" ", "\u200A");
        // Build a spannable so that the am/pm will be formatted
        int amPmPos = pattern.indexOf('a');
        if (amPmPos == -1) {
            clockView.setFormat12Hour(pattern);
            return;
        }
        Spannable sp = new SpannableString(pattern);
        sp.setSpan(new StyleSpan(android.graphics.Typeface.BOLD), amPmPos, amPmPos + 1,
                Spannable.SPAN_POINT_MARK);
        sp.setSpan(new AbsoluteSizeSpan(amPmFontSize), amPmPos, amPmPos + 1,
                Spannable.SPAN_POINT_MARK);
        sp.setSpan(new TypefaceSpan("sans-serif-condensed"), amPmPos, amPmPos + 1,
                Spannable.SPAN_POINT_MARK);
        clockView.setFormat12Hour(sp);
    }

    public boolean isEnabled(){
        return true;
    }
}
