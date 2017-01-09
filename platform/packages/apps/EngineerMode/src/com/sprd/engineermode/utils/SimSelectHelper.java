
package com.sprd.engineermode;

import android.content.Intent;
import android.content.Context;
import android.util.Log;

import com.sprd.engineermode.telephony.SimSelectHelperActivity;

public class SimSelectHelper {
    private static Intent mIntentForStart = null;
    private static boolean mAutoDisable = false;
    private static Runnable mOnChange = null;

    public static void startSimSelect(Intent intent, boolean autoDisble, Runnable onChangeRun,
            Context context) {
        mIntentForStart = intent;
        mAutoDisable = autoDisble;
        mOnChange = onChangeRun;

        Intent simSelectIntent = new Intent(context, SimSelectHelperActivity.class);
        context.startActivity(simSelectIntent);
    }

    public static Intent getIntentForStart() {
        return mIntentForStart;
    }

    public static boolean isAutoDisable() {
        return mAutoDisable;
    }

    public static Runnable getOnChangeCB() {
        return mOnChange;
    }
}
