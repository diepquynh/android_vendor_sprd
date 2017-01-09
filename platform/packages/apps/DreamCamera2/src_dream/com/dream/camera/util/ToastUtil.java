package com.dream.camera.util;

import android.annotation.IntDef;
import android.annotation.StringRes;
import android.content.Context;
import android.widget.Toast;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Toast util class
 *
 * @author yuyao
 * @date 16-7-13
 * @version 1.1
 */
public class ToastUtil {

    private static Toast mToast;

    @IntDef({LENGTH_SHORT, LENGTH_LONG})
    @Retention(RetentionPolicy.SOURCE)
    public @interface Duration {}
    /**
     * Show the view or text notification for a short period of time.  This time
     * could be user-definable.  This is the default.
     */
    public static final int LENGTH_SHORT = 0;

    /**
     * Show the view or text notification for a long period of time.  This time
     * could be user-definable.
     */
    public static final int LENGTH_LONG = 1;

    /**
     * show toast
     *
     * @param context the required context
     * @param msg  string toast content
     * @param duration  toast time length
     */
    public static void showToast(Context context, String msg,@Duration int duration) {
        if (mToast == null) {
            mToast = Toast.makeText(context, null, Toast.LENGTH_SHORT);
        }
        mToast.setDuration(duration);
        mToast.setText(msg);
        mToast.show();
    }

    /**
     * show toast
     *
     * @param context the required context
     * @param msg  the string resid to toast
     * @param duration  toast time length
     */
    public static void showToast(Context context, @StringRes int msg,@Duration int duration) {
        if (mToast == null) {
            mToast = Toast.makeText(context, null, Toast.LENGTH_SHORT);
        }
        mToast.setDuration(duration);
        mToast.setText(msg);
        mToast.show();
    }

    /**
     * cancel toast display
     *
     */
    public static void cancelToast(){
        if (mToast != null){
            mToast.cancel();
            mToast=null;
        }
    }
}
