
package com.sprd.firewall.util;

import android.content.Context;
import android.text.format.DateFormat;

import java.text.SimpleDateFormat;
import java.util.Date;

public class DateUtil {

    public static final String DATE_FORMATE = "yyyy-MM-dd HH:mm:ss";

    public static String formatDate(long time, String dateFormatStr) {
        SimpleDateFormat df = new SimpleDateFormat(dateFormatStr);
        Date da = new Date(time);
        return df.format(da);
    }

    public static String formatDate(long time, Context context) {
        Date da = new Date(time);
        String dateStr = DateFormat.getDateFormat(context).format(da);
        String timeStr = DateFormat.getTimeFormat(context).format(da);
        String ret = dateStr + " " + timeStr;
        return ret;
    }

    public static String formatDate(String dateFormatStr) {
        SimpleDateFormat df = new SimpleDateFormat(dateFormatStr);
        Date da = new Date();
        return df.format(da);
    }

    public static String formatDate() {
        SimpleDateFormat df = new SimpleDateFormat(DATE_FORMATE);
        Date da = new Date();
        return df.format(da);
    }
}
