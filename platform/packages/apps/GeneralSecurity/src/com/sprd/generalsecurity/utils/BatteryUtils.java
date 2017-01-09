package com.sprd.generalsecurity.utils;

import java.text.NumberFormat;

import android.content.Context;
import android.content.Intent;
import android.os.BatteryManager;
import android.util.Log;

import com.sprd.generalsecurity.R;

public final class BatteryUtils {
    private static final String TAG = "Utils";
    private static final int SECONDS_PER_MINUTE = 60;
    private static final int SECONDS_PER_HOUR = 60 * 60;
    private static final int SECONDS_PER_DAY = 24 * 60 * 60;

    public static int getBatteryLevel(Intent batteryChangedIntent) {
        int level = batteryChangedIntent.getIntExtra(
                BatteryManager.EXTRA_LEVEL, 0);
        int scale = batteryChangedIntent.getIntExtra(
                BatteryManager.EXTRA_SCALE, 100);
        return (level * 100) / scale;
    }

    public static String getBatteryPercentage(Intent batteryChangedIntent) {
        return formatPercentage(getBatteryLevel(batteryChangedIntent));
    }

    public static String getBatteryPercentage(int batteryLevel) {
        return formatPercentage(batteryLevel);
    }

    /** Formats the ratio of amount/total as a percentage. */
    public static String formatPercentage(long amount, long total) {
        return formatPercentage(((double) amount) / total);
    }

    /** Formats an integer from 0..100 as a percentage. */
    public static String formatPercentage(int percentage) {
        return formatPercentage(((double) percentage) / 100.0);
    }

    /** Formats a double from 0.0..1.0 as a percentage. */
    private static String formatPercentage(double percentage) {
        return NumberFormat.getPercentInstance().format(percentage);
    }

    public static int getPicId(int picCount, int percent) {
        int id = (int) Math.round(percent / 100.0 * (picCount - 2));
        if (percent >= 100) {
            Log.i(TAG, "2picCount:" + picCount + "  percent:" + percent
                    + "  id:" + (picCount - 1));
            return picCount - 1;
        } else if (percent <= 0) {
            Log.i(TAG, "2picCount:" + picCount + "  percent:" + percent
                    + "  id:" + 0);
            return 0;
        } else {
            Log.i(TAG, "2picCount:" + picCount + "  percent:" + percent
                    + "  id:" + id);
            return id;
        }
    }

    /**
     * Returns elapsed time for the given millis, in the following format:
     * 2d 5h 40m 29s
     * @param context the application context
     * @param millis the elapsed time in milli seconds
     * @param withSeconds include seconds?
     * @return the formatted elapsed time
     */
    public static String formatElapsedTime(Context context, double millis,
            boolean withSeconds) {
        StringBuilder sb = new StringBuilder();
        Log.i(TAG, "millis" + millis);
        int seconds = (int) Math.floor(millis / 1000);
        Log.i(TAG, "seconds" + seconds);
        if (seconds <= 0) {
            return sb.append(
                    context.getString(R.string.battery_history_millis, millis))
                    .toString();
        }
        if (!withSeconds) {
            // Round up.
            seconds += 30;
        }

        int days = 0, hours = 0, minutes = 0;
        if (seconds >= SECONDS_PER_DAY) {
            days = seconds / SECONDS_PER_DAY;
            seconds -= days * SECONDS_PER_DAY;
        }
        if (seconds >= SECONDS_PER_HOUR) {
            hours = seconds / SECONDS_PER_HOUR;
            seconds -= hours * SECONDS_PER_HOUR;
        }
        if (seconds >= SECONDS_PER_MINUTE) {
            minutes = seconds / SECONDS_PER_MINUTE;
            seconds -= minutes * SECONDS_PER_MINUTE;
        }
        if (withSeconds) {
            if (days > 0) {
                sb.append(context.getString(R.string.battery_history_days,
                        days, hours, minutes, seconds));
            } else if (hours > 0) {
                sb.append(context.getString(R.string.battery_history_hours,
                        hours, minutes, seconds));
            } else if (minutes > 0) {
                sb.append(context.getString(R.string.battery_history_minutes, minutes, seconds));
            } else {
                sb.append(context.getString(R.string.battery_history_seconds, seconds));
            }
        } else {
            if (days > 0) {
                sb.append(context.getString(
                        R.string.battery_history_days_no_seconds, days, hours,
                        minutes));
            } else if (hours > 0) {
                sb.append(context.getString(
                        R.string.battery_history_hours_no_seconds, hours,
                        minutes));
            } else {
                sb.append(context.getString(
                        R.string.battery_history_minutes_no_seconds, minutes));
            }
        }
        return sb.toString();
    }
}