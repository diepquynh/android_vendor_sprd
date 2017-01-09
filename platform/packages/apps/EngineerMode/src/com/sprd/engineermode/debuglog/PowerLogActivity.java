
package com.sprd.engineermode.debuglog;

import android.os.Bundle;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.TwoStatePreference;
import android.util.Log;

import com.sprd.engineermode.R;

public class PowerLogActivity extends PreferenceActivity implements
        Preference.OnPreferenceClickListener {
    private static String TAG = "PowerLogActivity";
    private TwoStatePreference mAll;
    private TwoStatePreference mBattery;
    private TwoStatePreference mDoze;
    private TwoStatePreference mKeyLEDBackLight;
    private TwoStatePreference mKeyWakeUp;
    private TwoStatePreference mLcdBackLight;
    private TwoStatePreference mNotityLight;
    private TwoStatePreference mPowerGuru;
    private TwoStatePreference mPowerHint;
    private TwoStatePreference mWakeLock;

    private static final String SWITCH_ALL_LOG = "sprd_power_log_all";
    private static final String SWITCH_BATTERY_LOG = "sprd_battery_log";
    private static final String SWITCH_DOZE_LOG = "sprd_doze_log";
    private static final String SWITCH_LED_LOG = "sprd_key_led_backlight_log";
    private static final String SWITCH_WAKEUP_LOG = "sprd_key_wakeup_log";
    private static final String SWITCH_LCD_BACK_LOG = "sprd_lcd_backlight_log";
    private static final String SWITCH_NOTITY_LOG = "sprd_notify_light_log";
    private static final String SWITCH_POWER_GURU_LOG = "sprd_power_guru_log";
    private static final String SWITCH_POWER_HINT_LOG = "sprd_power_hint_log";
    private static final String SWITCH_WAKELOCK_LOG = "sprd_wakelock_log";

    private static final String KEY_DEBUG_BATTERY = "debug.power.battery";
    private static final String KEY_DEBUG_DOZE = "debug.power.doze";
    private static final String KEY_DEBUG_LED = "debug.power.key_led_backlight";
    private static final String KEY_DEBUG_LCD = "debug.power.lcd_backlight";
    private static final String KEY_DEBUG_NOTITY = "debug.power.notify_light";
    private static final String KEY_DEBUG_WAKELOCK = "debug.power.wakelock";
    private static final String KEY_DEBUG_POWERGURU = "debug.power.power_guru";
    private static final String KEY_DEBUG_POWERHINT = "debug.power.hint";
    private static final String KEY_DEBUG_WAKEUP = "debug.power.key_wakeup";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.power_log);

        mAll = (TwoStatePreference) findPreference(SWITCH_ALL_LOG);
        mAll.setOnPreferenceClickListener(this);

        mBattery = (TwoStatePreference) findPreference(SWITCH_BATTERY_LOG);
        mBattery.setOnPreferenceClickListener(this);

        mDoze = (TwoStatePreference) findPreference(SWITCH_DOZE_LOG);
        mDoze.setOnPreferenceClickListener(this);

        mKeyLEDBackLight = (TwoStatePreference) findPreference(SWITCH_LED_LOG);
        mKeyLEDBackLight.setOnPreferenceClickListener(this);

        mKeyWakeUp = (TwoStatePreference) findPreference(SWITCH_WAKEUP_LOG);
        mKeyWakeUp.setOnPreferenceClickListener(this);

        mLcdBackLight = (TwoStatePreference) findPreference(SWITCH_LCD_BACK_LOG);
        mLcdBackLight.setOnPreferenceClickListener(this);

        mNotityLight = (TwoStatePreference) findPreference(SWITCH_NOTITY_LOG);
        mNotityLight.setOnPreferenceClickListener(this);

        mPowerGuru = (TwoStatePreference) findPreference(SWITCH_POWER_GURU_LOG);
        mPowerGuru.setOnPreferenceClickListener(this);

        mPowerHint = (TwoStatePreference) findPreference(SWITCH_POWER_HINT_LOG);
        mPowerHint.setOnPreferenceClickListener(this);

        mWakeLock = (TwoStatePreference) findPreference(SWITCH_WAKELOCK_LOG);
        mWakeLock.setOnPreferenceClickListener(this);

        updateItemState();
        updateAllState();
    }

    @Override
    public boolean onPreferenceClick(Preference pref) {
        if (pref == mAll) {
            if (mAll.isChecked()) {
                Log.d(TAG, "open all log.");
                openAll();
            } else {
                Log.d(TAG, "off all log.");
                offAll();
            }
        } else if (pref == mBattery) {
            if (mBattery.isChecked()) {
                Log.d(TAG, "open Battery log");
                setBatteryPropertyTrue();
                updateAllState();
            } else {
                Log.d(TAG, "off Battery log");
                setBatteryPropertyFalse();
                updateAllState();
            }
        } else if (pref == mDoze) {
            if (mDoze.isChecked()) {
                Log.d(TAG, "open doze log");
                setDozePropertyTrue();
                updateAllState();
            } else {
                Log.d(TAG, "off doze log");
                setDozePropertyFalse();
                updateAllState();
            }
        } else if (pref == mKeyLEDBackLight) {
            if (mKeyLEDBackLight.isChecked()) {
                Log.d(TAG, "open led backlight log");
                setLedPropertyTrue();
                updateAllState();
            } else {
                Log.d(TAG, "off led backlight log");
                setLedPropertyFalse();
                updateAllState();
            }
        } else if (pref == mKeyWakeUp) {
            if (mKeyWakeUp.isChecked()) {
                Log.d(TAG, "open wakeup log");
                setWakeUpPropertyTrue();
                updateAllState();
            } else {
                Log.d(TAG, "off wakeup log");
                setWakeUpPropertyFalse();
                updateAllState();
            }
        } else if (pref == mLcdBackLight) {
            if (mLcdBackLight.isChecked()) {
                Log.d(TAG, "open lcd backlight log");
                setLcdPropertyTrue();
                updateAllState();
            } else {
                Log.d(TAG, "off lcd backlight log");
                setLcdPropertyFalse();
                updateAllState();
            }
        } else if (pref == mNotityLight) {
            if (mNotityLight.isChecked()) {
                Log.d(TAG, "open notitylight log");
                setNotityPropertyTrue();
                updateAllState();
            } else {
                Log.d(TAG, "off notitylight log");
                setNotifyPropertyFalse();
                updateAllState();
            }
        } else if (pref == mPowerGuru) {
            if (mPowerGuru.isChecked()) {
                Log.d(TAG, "open powerguru log");
                setPowerGuruPropertyTrue();
                updateAllState();
            } else {
                Log.d(TAG, "off powerguru log");
                setPowerGuruPropertyFalse();
                updateAllState();
            }
        } else if (pref == mPowerHint) {
            if (mPowerHint.isChecked()) {
                Log.d(TAG, "open powerhint log");
                setPowerHintPropertyTrue();
                updateAllState();
            } else {
                Log.d(TAG, "off powerhint log");
                setPowerHintPropertyFalse();
                updateAllState();
            }
        } else if (pref == mWakeLock) {
            if (mWakeLock.isChecked()) {
                Log.d(TAG, "open wakelock log");
                setWakeLockPropertyTrue();
                updateAllState();
            } else {
                Log.d(TAG, "off wakelock log");
                setWakeLockPropertyFalse();
                updateAllState();
            }
        }
        return true;
    }

    public void openAll() {
        if (!mBattery.isChecked()) {
            setBatteryPropertyTrue();
            mBattery.setChecked(true);
        }
        if (!mDoze.isChecked()) {
            setDozePropertyTrue();
            mDoze.setChecked(true);
        }
        if (!mKeyLEDBackLight.isChecked()) {
            setLedPropertyTrue();
            mKeyLEDBackLight.setChecked(true);
        }
        if (!mKeyWakeUp.isChecked()) {
            setWakeUpPropertyTrue();
            mKeyWakeUp.setChecked(true);
        }
        if (!mLcdBackLight.isChecked()) {
            setLcdPropertyTrue();
            mLcdBackLight.setChecked(true);
        }
        if (!mNotityLight.isChecked()) {
            setNotityPropertyTrue();
            mNotityLight.setChecked(true);
        }
        if (!mPowerGuru.isChecked()) {
            setPowerGuruPropertyTrue();
            mPowerGuru.setChecked(true);
        }
        if (!mPowerHint.isChecked()) {
            setPowerHintPropertyTrue();
            mPowerHint.setChecked(true);
        }
        if (!mWakeLock.isChecked()) {
            setWakeLockPropertyTrue();
            mWakeLock.setChecked(true);
        }
    }

    public void offAll() {
        if (mBattery.isChecked()) {
            setBatteryPropertyFalse();
            mBattery.setChecked(false);
        }
        if (mDoze.isChecked()) {
            setDozePropertyFalse();
            mDoze.setChecked(false);
        }
        if (mKeyLEDBackLight.isChecked()) {
            setLedPropertyFalse();
            mKeyLEDBackLight.setChecked(false);
        }
        if (mKeyWakeUp.isChecked()) {
            setWakeUpPropertyFalse();
            mKeyWakeUp.setChecked(false);
        }
        if (mLcdBackLight.isChecked()) {
            setLcdPropertyFalse();
            mLcdBackLight.setChecked(false);
        }
        if (mNotityLight.isChecked()) {
            setNotifyPropertyFalse();
            mNotityLight.setChecked(false);
        }
        if (mPowerGuru.isChecked()) {
            setPowerGuruPropertyFalse();
            mPowerGuru.setChecked(false);
        }
        if (mPowerHint.isChecked()) {
            setPowerHintPropertyFalse();
            mPowerHint.setChecked(false);
        }
        if (mWakeLock.isChecked()) {
            setWakeLockPropertyFalse();
            mWakeLock.setChecked(false);
        }
    }

    public void updateAllState() {
        if (mWakeLock.isChecked() && mPowerHint.isChecked() && mPowerGuru.isChecked()
                && mNotityLight.isChecked() && mLcdBackLight.isChecked() && mKeyWakeUp.isChecked()
                && mKeyLEDBackLight.isChecked() && mDoze.isChecked() && mBattery.isChecked()) {
            mAll.setChecked(true);
        } else if (!mWakeLock.isChecked() || !mPowerHint.isChecked() || !mPowerGuru.isChecked()
                || !mNotityLight.isChecked() || !mLcdBackLight.isChecked()
                || !mKeyWakeUp.isChecked() || !mKeyLEDBackLight.isChecked() || !mDoze.isChecked()
                || !mBattery.isChecked()) {
            mAll.setChecked(false);
        }
    }

    public void updateItemState() {
        if (SystemProperties.get(KEY_DEBUG_BATTERY).equals("true")) {
            mBattery.setChecked(true);
        } else {
            mBattery.setChecked(false);
        }
        if (SystemProperties.get(KEY_DEBUG_DOZE).equals("true")) {
            mDoze.setChecked(true);
        } else {
            mDoze.setChecked(false);
        }
        if (SystemProperties.get(KEY_DEBUG_LCD).equals("true")) {
            mLcdBackLight.setChecked(true);
        } else {
            mLcdBackLight.setChecked(false);
        }
        if (SystemProperties.get(KEY_DEBUG_LED).equals("true")) {
            mKeyLEDBackLight.setChecked(true);
        } else {
            mKeyLEDBackLight.setChecked(false);
        }
        if (SystemProperties.get(KEY_DEBUG_NOTITY).equals("true")) {
            mNotityLight.setChecked(true);
        } else {
            mNotityLight.setChecked(false);
        }
        if (SystemProperties.get(KEY_DEBUG_WAKELOCK).equals("true")) {
            mWakeLock.setChecked(true);
        } else {
            mWakeLock.setChecked(false);
        }
        if (SystemProperties.get(KEY_DEBUG_WAKEUP).equals("true")) {
            mKeyWakeUp.setChecked(true);
        } else {
            mKeyWakeUp.setChecked(false);
        }
        if (SystemProperties.get(KEY_DEBUG_POWERGURU).equals("true")) {
            mPowerGuru.setChecked(true);
        } else {
            mPowerGuru.setChecked(false);
        }
        if (SystemProperties.get(KEY_DEBUG_POWERHINT).equals("true")) {
            mPowerHint.setChecked(true);
        } else {
            mPowerHint.setChecked(false);
        }
    }

    public void setBatteryPropertyTrue() {
        SystemProperties.set(KEY_DEBUG_BATTERY, "true");
    }

    public void setBatteryPropertyFalse() {
        SystemProperties.set(KEY_DEBUG_BATTERY, "false");
    }

    public void setPowerHintPropertyTrue() {
        SystemProperties.set(KEY_DEBUG_POWERHINT, "true");
    }

    public void setPowerHintPropertyFalse() {
        SystemProperties.set(KEY_DEBUG_POWERHINT, "false");
    }

    public void setPowerGuruPropertyTrue() {
        SystemProperties.set(KEY_DEBUG_POWERGURU, "true");
    }

    public void setPowerGuruPropertyFalse() {
        SystemProperties.set(KEY_DEBUG_POWERGURU, "false");
    }

    public void setDozePropertyTrue() {
        SystemProperties.set(KEY_DEBUG_DOZE, "true");
    }

    public void setDozePropertyFalse() {
        SystemProperties.set(KEY_DEBUG_DOZE, "false");
    }

    public void setLcdPropertyTrue() {
        SystemProperties.set(KEY_DEBUG_LCD, "true");
    }

    public void setLcdPropertyFalse() {
        SystemProperties.set(KEY_DEBUG_LCD, "false");
    }

    public void setWakeUpPropertyTrue() {
        SystemProperties.set(KEY_DEBUG_WAKEUP, "true");
    }

    public void setWakeUpPropertyFalse() {
        SystemProperties.set(KEY_DEBUG_WAKEUP, "false");
    }

    public void setLedPropertyTrue() {
        SystemProperties.set(KEY_DEBUG_LED, "true");
    }

    public void setLedPropertyFalse() {
        SystemProperties.set(KEY_DEBUG_LED, "false");
    }

    public void setNotityPropertyTrue() {
        SystemProperties.set(KEY_DEBUG_NOTITY, "true");
    }

    public void setNotifyPropertyFalse() {
        SystemProperties.set(KEY_DEBUG_NOTITY, "false");
    }

    public void setWakeLockPropertyTrue() {
        SystemProperties.set(KEY_DEBUG_WAKELOCK, "true");
    }

    public void setWakeLockPropertyFalse() {
        SystemProperties.set(KEY_DEBUG_WAKELOCK, "false");
    }

}
