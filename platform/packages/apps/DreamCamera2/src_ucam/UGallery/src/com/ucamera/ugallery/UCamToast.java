/**
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */
package com.ucamera.ugallery;

import android.content.Context;
import android.widget.Toast;

import java.util.WeakHashMap;

public class UCamToast {

    private UCamToast() {}

    public static void showToast(Context context, int msgid, int duration) {
        showToast(context, context.getString(msgid), duration);
    }

    public static void showToast(Context context,String msg, int duration) {
        long durationTime = 2000;
        switch (duration) {
            case Toast.LENGTH_LONG: durationTime = 3500; break;
            case Toast.LENGTH_SHORT:durationTime = 2000; break;
        }
        Long last = sLastMessages.get(msg);
        if (last == null || (System.currentTimeMillis() - last) > durationTime ) {
            sLastMessages.put(msg, System.currentTimeMillis());
            Toast.makeText(context, msg, duration).show();
        }
    }

    private static WeakHashMap<String,Long> sLastMessages = new WeakHashMap<String, Long>();
}
