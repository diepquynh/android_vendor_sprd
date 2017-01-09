package com.sprd.cellbroadcastreceiver.util;

import android.util.Log;

import com.sprd.cellbroadcastreceiver.R;

public class LanguageIds {
    private static final String TAG = "LanguageIds";
    // LangId
    public static final int GSM_GERMAN          = 0x00;
    public static final int GSM_ENGLISH         = 0x01;
    public static final int GSM_ITALIAN         = 0x02;
    public static final int GSM_FRENCH          = 0x03;
    public static final int GSM_SPANISH         = 0x04;
    public static final int GSM_DUTCH           = 0x05;
    public static final int GSM_SWEDISH         = 0x06;
    public static final int GSM_DANISH          = 0x07;
    public static final int GSM_PORTUGUESE      = 0x08;
    public static final int GSM_FINNISH         = 0x09;
    public static final int GSM_NORWEGIAN       = 0x0a;
    public static final int GSM_GREEK           = 0x0b;
    public static final int GSM_TURKISH         = 0x0c;
    public static final int GSM_HUNGARIAN       = 0x0d;
    public static final int GSM_POLISH          = 0x0e;
    public static final int GSM_CZECH           = 0x20;
    public static final int GSM_HEBREW          = 0x21;
    public static final int GSM_ARABIC          = 0x22;
    public static final int GSM_RUSSIAN         = 0x23;
    public static final int GSM_ICELANDIC       = 0x24;

    public static int[] LANGUAGE_ID = {GSM_GERMAN, GSM_ENGLISH, GSM_ITALIAN, GSM_FRENCH, GSM_SPANISH,
        GSM_DUTCH, GSM_SWEDISH, GSM_DANISH, GSM_PORTUGUESE, GSM_FINNISH, GSM_NORWEGIAN, GSM_GREEK,
        GSM_TURKISH, GSM_HUNGARIAN, GSM_POLISH, GSM_CZECH, GSM_HEBREW, GSM_ARABIC, GSM_RUSSIAN, GSM_ICELANDIC};

    public static final int[] LangMap = { R.string.lang_german,
            R.string.lang_english, R.string.lang_italian, R.string.lang_french,
            R.string.lang_spanish, R.string.lang_dutch, R.string.lang_swedish,
            R.string.lang_danish, R.string.lang_portuguese,
            R.string.lang_finnish, R.string.lang_norwegian,
            R.string.lang_greek, R.string.lang_turkish,
            R.string.lang_hungarian, R.string.lang_polish, R.string.lang_czech,
            R.string.lang_hebrew, R.string.lang_arabic, R.string.lang_russian,
            R.string.lang_icelandic };

    public static int MAX_LANG = LangMap.length;
}
