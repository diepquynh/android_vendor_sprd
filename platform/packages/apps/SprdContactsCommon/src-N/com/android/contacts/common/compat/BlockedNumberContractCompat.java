package com.android.contacts.common.compat;

import android.content.Context;
import android.provider.BlockedNumberContract;

public class BlockedNumberContractCompat {
    public static boolean canCurrentUserBlockNumbers(Context context) {
        boolean result = false;
        try {
            result = BlockedNumberContract.canCurrentUserBlockNumbers(context);
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
            result = false;
        }
        return result;
    }
}