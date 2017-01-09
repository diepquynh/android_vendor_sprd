/** Create by Spreadst */
/*

 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.spreadst.s2lockscreen;

import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Locale;

import android.content.Context;
import android.database.ContentObserver;
import android.graphics.Typeface;
import android.os.Handler;
import android.os.SystemClock;
import android.provider.Settings;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.style.CharacterStyle;
import android.text.style.RelativeSizeSpan;
import android.util.AttributeSet;
import android.widget.TextView;

import com.android.internal.R;

/**
 * Like AnalogClock, but digital. Shows seconds. FIXME: implement separate views
 * for hours/minutes/seconds, so proportional fonts don't shake rendering
 */

public class DigitalClock extends TextView {

    Calendar mCalendar;
    private final static String m12 = "hh:mm";
    private final static String m24 = "k:mm";
    private static final String SYSTEM = "/system/fonts/";
    private static final String SYSTEM_FONT_TIME_BACKGROUND = SYSTEM
            + "AndroidClock.ttf";
    private static final String SYSTEM_FONT_TIME_FOREGROUND = SYSTEM
            + "AndroidClock_Highlight.ttf";

    private FormatChangeObserver mFormatChangeObserver;

    private Runnable mTicker;
    private Handler mHandler;

    private boolean mTickerStopped = false;

    String mFormat;
    private String mClockFormatString;
    private SimpleDateFormat mClockFormat;

    private static final Typeface sBackgroundFont;
    private static final Typeface sForegroundFont;

    static {
        sBackgroundFont = Typeface.createFromFile(SYSTEM_FONT_TIME_BACKGROUND);
        sForegroundFont = Typeface.createFromFile(SYSTEM_FONT_TIME_FOREGROUND);
    }

    public DigitalClock(Context context) {
        super(context);
        initClock(context);
    }

    public DigitalClock(Context context, AttributeSet attrs) {
        super(context, attrs);
        initClock(context);
    }

    private void initClock(Context context) {
        if (mCalendar == null) {
            mCalendar = Calendar.getInstance();
        }
        mFormatChangeObserver = new FormatChangeObserver();
        getContext().getContentResolver().registerContentObserver(
                Settings.System.CONTENT_URI, true, mFormatChangeObserver);
        setFormat();
    }

    @Override
    protected void onAttachedToWindow() {

        setTypeface(sBackgroundFont);
        mTickerStopped = false;
        super.onAttachedToWindow();
        mHandler = new Handler();

        /**
         * requests a tick on the next hard-second boundary
         */
        mTicker = new Runnable() {
            public void run() {
                if (mTickerStopped)
                    return;
                mCalendar.setTimeInMillis(System.currentTimeMillis());
                updateTime();
                invalidate();
                long now = SystemClock.uptimeMillis();
                long next = now + (1000 - now % 1000);
                mHandler.postAtTime(mTicker, next);
            }
        };
        mTicker.run();
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mTickerStopped = true;
        if (mFormatChangeObserver != null) {
            mContext.getContentResolver().unregisterContentObserver(
                    mFormatChangeObserver);
        }
        mFormatChangeObserver = null;
    }

    /**
     * Pulls 12/24 mode from system settings
     */
    private boolean get24HourMode() {
        return android.text.format.DateFormat.is24HourFormat(getContext());
    }

    private void setFormat() {
        if (get24HourMode()) {
            mFormat = m24;
        } else {
            mFormat = m12;
        }
    }

    private class FormatChangeObserver extends ContentObserver {
        public FormatChangeObserver() {
            super(new Handler());
        }

        @Override
        public void onChange(boolean selfChange) {
            setFormat();
        }
    }

    /* Modify 20130626 Spreadst of 174787 do not display AMPM start */
    void updateTime() {
        mCalendar.setTimeInMillis(System.currentTimeMillis());

        boolean timeFormat = android.text.format.DateFormat.is24HourFormat(getContext());

        int res;

        if (timeFormat) {
            /* SPRD: Modify 20130905 Spreadst of Bug 210096 Symbol not found build fail @{ */
            res = R.string.twenty_four_hour_time_format;
        } else {
            res = R.string.twelve_hour_time_format;
            /* @} */
        }

        String format = getContext().getString(res);
        final char MAGIC1 = '\uEF00';
        final char MAGIC2 = '\uEF01';
        SimpleDateFormat sdf;
        if (!format.equals(mClockFormatString)) {
            int a = -1;
            boolean quoted = false;
            for (int i = 0; i < format.length(); i++) {
                char c = format.charAt(i);

                if (c == '\'') {
                    quoted = !quoted;
                }
                if (!quoted && c == 'a') {
                    a = i;
                    break;
                }
            }

            if (a >= 0) {
                // Move a back so any whitespace before AM/PM is also in the
                // alternate size.
                final int b = a;
                while (a > 0 && Character.isWhitespace(format.charAt(a - 1))) {
                    a--;
                }
                format = format.substring(0, a) + MAGIC1 + "a" + MAGIC2 + format.substring(b + 1);
            }

            mClockFormat = sdf = new SimpleDateFormat(format);
            mClockFormatString = format;
        } else {
            sdf = mClockFormat;
        }
        String newTime = sdf.format(mCalendar.getTime());

        int magic1 = newTime.indexOf(MAGIC1);
        int magic2 = newTime.indexOf(MAGIC2);
        SpannableStringBuilder formatted = new SpannableStringBuilder(newTime.trim());
        if (magic1 >= 0 && magic2 > magic1) {
            if (timeFormat) {
                formatted.delete(magic1, magic2 + 1);
            } else {
                CharacterStyle style = new RelativeSizeSpan(0.4f);
                formatted.setSpan(style, magic1, magic2,
                        Spannable.SPAN_EXCLUSIVE_INCLUSIVE);
            }
            for (int i = 0; i < formatted.length(); i++) {
                char c = formatted.charAt(i);
                if (c == MAGIC1 || c == MAGIC2)
                    formatted.delete(i, i + 1);
            }
        }
        setText(formatted);
    }
    /* Modify 20130626 Spreadst of 174787 do not display AMPM end */
}
