package plugin.sprd.pressbrightness;

import java.io.File;

import android.R.string;
import android.app.AddonManager;
import android.content.ContentResolver;
import android.content.Context;
import android.os.UserHandle;
import android.support.v7.preference.ListPreference;
import android.provider.Settings;
import android.util.Log;
import android.widget.Toast;

import com.android.settings.PressBrightness;
import com.android.settings.R;

public class AddonPressBrightness extends PressBrightness implements
        AddonManager.InitialCallback {
    private static final String TAG = "AddonPressBrightness";
    private static final int VALUE_KEYLIGHT_1500 = 1500;
    private static final int VALUE_KEYLIGHT_6000 = 6000;
    private static final int VALUE_KEYLIGHT_1 = -1;
    private static final int VALUE_KEYLIGHT_2 = -2;
    private static final int FALLBACK_TOUCH_LIGHT_TIMEOUT_VALUE = 1500;

    public static ListPreference mTouchLightTimeoutPreference;
    public static Context context;

    private int mValue;
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public boolean isSupport() {
        try {
            File file = new File(
                    "/sys/class/leds/keyboard-backlight/brightness");
            Log.d(TAG, " fileIsExists");
            if (!file.exists()) {
                Log.d(TAG, "fileIsExists false");
                return false;
            }
        } catch (Exception e) {
            // TODO: handle exception
            return false;
        }
        Log.d(TAG, "fileIsExists true");
        return true;
    }

    public void updateTouchLightPreferenceSummary(int value) {
        mValue = value;
        if (value == VALUE_KEYLIGHT_1500) {
            mTouchLightTimeoutPreference
                    .setSummary(mTouchLightTimeoutPreference.getEntries()[0]);
        } else if (value == VALUE_KEYLIGHT_6000) {
            mTouchLightTimeoutPreference
                    .setSummary(mTouchLightTimeoutPreference.getEntries()[1]);
        } else if (value == VALUE_KEYLIGHT_1) {
            mTouchLightTimeoutPreference
                    .setSummary(mTouchLightTimeoutPreference.getEntries()[2]);
        } else if (value == VALUE_KEYLIGHT_2) {
            mTouchLightTimeoutPreference
                    .setSummary(mTouchLightTimeoutPreference.getEntries()[3]);
        }
    }

    public void init(Context mContext, ListPreference mTouchLightPreference) {
        context = mContext;
        mTouchLightTimeoutPreference = mTouchLightPreference;
        final ContentResolver resolver = context.getContentResolver();
        final int touchlightcurrentTimeout = Settings.System.getIntForUser(
                resolver, Settings.System.BUTTON_LIGHT_OFF_TIMEOUT,
                FALLBACK_TOUCH_LIGHT_TIMEOUT_VALUE, UserHandle.USER_OWNER);
        mTouchLightTimeoutPreference.setValue(String
                .valueOf(touchlightcurrentTimeout));
        updateTouchLightPreferenceSummary(touchlightcurrentTimeout);
    }

    public void writeTouchLightPreference(Object objValue) {
        int value = Integer.parseInt(objValue.toString());
        /* Sprd: for bug 501208 @{ */
        Settings.System.putIntForUser(context.getContentResolver(),
                Settings.System.BUTTON_LIGHT_OFF_TIMEOUT, value,
                UserHandle.USER_OWNER);
        /* @} */
        updateTouchLightPreferenceSummary(value);
    }

    public void updateTouchLightStatus() {
        if (mTouchLightTimeoutPreference != null) {
            /* Sprd: for bug 501208 @{ */
            final int touchlightcurrentTimeout = Settings.System.getIntForUser(
                    context.getContentResolver(),
                    Settings.System.BUTTON_LIGHT_OFF_TIMEOUT,
                    FALLBACK_TOUCH_LIGHT_TIMEOUT_VALUE, UserHandle.USER_OWNER);
            /* @} */
            mTouchLightTimeoutPreference.setValue(String
                    .valueOf(touchlightcurrentTimeout));
            updateTouchLightPreferenceSummary(touchlightcurrentTimeout);
        }
    }

    public void setEnabled(boolean checked, ListPreference listPreference) {
        Log.i("xxx","checked:"+checked);
        listPreference.setEnabled(checked);
        if (checked) {
            Log.i("xxx","if");
            updateTouchLightPreferenceSummary(mValue);
        } else {
            Log.i("xxx","else");
            listPreference.setSummary(R.string.close_screen_bright_automode);
        }
    }
}
