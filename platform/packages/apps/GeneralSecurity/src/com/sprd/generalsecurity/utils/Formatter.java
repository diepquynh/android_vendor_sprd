package com.sprd.generalsecurity.utils;

import android.content.Context;
import com.sprd.generalsecurity.R;

public class Formatter {

    private static final int UNITCHANGE = 900;

    public static String formatFileSize(Context context, long number,
            boolean shorter) {
        if (context == null) {
            return "";
        }

        float result = number;
        int suffix = R.string.byteShort;
        if (result > UNITCHANGE) {
            suffix = R.string.kilobyteShort;
            result = result / 1024;
        }
        if (result > UNITCHANGE) {
            suffix = R.string.megabyteShort;
            result = result / 1024;
        }
        if (result > UNITCHANGE) {
            suffix = R.string.gigabyteShort;
            result = result / 1024;
        }
        if (result > UNITCHANGE) {
            suffix = R.string.terabyteShort;
            result = result / 1024;
        }
        if (result > UNITCHANGE) {
            suffix = R.string.petabyteShort;
            result = result / 1024;
        }
        String value;
        if (result < 1) {
            value = String.format("%.2f", result);
        } else if (result < 10) {
            if (shorter) {
                value = String.format("%.1f", result);
            } else {
                value = String.format("%.2f", result);
            }
        } else if (result < 100) {
            if (shorter) {
                value = String.format("%.0f", result);
            } else {
                value = String.format("%.2f", result);
            }
        } else {
            value = String.format("%.0f", result);
        }
        return context.getResources().getString(R.string.fileSizeSuffix, value,
                context.getString(suffix));
    }
}
