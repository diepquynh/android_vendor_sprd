package com.sprd.generalsecurity.utils;

import android.text.format.Time;

import android.util.Log;

import java.util.Calendar;
import android.content.Context;

import android.content.SharedPreferences;
import android.preference.PreferenceManager;



public class DateCycleUtils {
    //cycle values
    public static final String KEY_DATA_CYCLE_SIM1 = "pref_data_cycle";
    public static final String KEY_DATA_CYCLE_SIM2 = "pref_data_cycle_sim2";

    public static final String KEY_DAY_RESTRICT = "key_day_flow_restrict";
    public static final String KEY_MONTH_TOTAL = "key_edit_month_total";

    public static final int CYCLE_MONTH = 0;
    public static final int CYCLE_WEEK = 1;
    public static final int CYCLE_DAY = 2;

    public class DataRestriction {
        public long dayRestriction;
        public long monthRestriction;
    }

    private DateCycleUtils() {

    }

    private static DateCycleUtils mInstance;

    public static DateCycleUtils getInstance() {
        if (mInstance == null) {
            mInstance = new DateCycleUtils();
        }
        return mInstance;
    }

    private void testTimeCycle() {
        Time t = new Time();
        t.setToNow();

        t.set(1, t.month, t.year);

    }

    public static long getWeekCycleStart() {
        Time t = new Time();
        t.set(System.currentTimeMillis());
        t.monthDay += 1;

        Calendar c = Calendar.getInstance();
        c.set(Calendar.HOUR_OF_DAY, 0);
        c.clear(Calendar.MINUTE);
        c.clear(Calendar.SECOND);
        c.clear(Calendar.MILLISECOND);

        c.set(Calendar.DAY_OF_WEEK, c.getFirstDayOfWeek());
        t.set(c.getTimeInMillis());

        return t.toMillis(true);
    }

    public static long getMonthCycleStart() {
        Time t = new Time();
        t.setToNow();
        t.set(1, t.month, t.year);

        return t.toMillis(true);
    }

    public static long getDayCycleStart() {
        Time t = new Time();
        t.setToNow();
        t.set(0, 0, 0, t.monthDay, t.month, t.year);

        return t.toMillis(true);
    }

    public DataRestriction getDataFlowRestriction(Context context, int sim) {
        SharedPreferences sharedPref;
        if (sim == 1) {
            sharedPref = context.getSharedPreferences("sim1", Context.MODE_PRIVATE);
        } else {
            sharedPref = context.getSharedPreferences("sim2", Context.MODE_PRIVATE);
        }
        String v = sharedPref.getString(DateCycleUtils.KEY_DAY_RESTRICT, "0");
        DataRestriction dt = new DataRestriction();
        dt.dayRestriction = (long)(Float.parseFloat(v) * 1024 * 1024);

        v = sharedPref.getString(DateCycleUtils.KEY_MONTH_TOTAL, "0");
        dt.monthRestriction = (long)(Float.parseFloat(v) * 1024 * 1024);

        return dt;
    }
}