package com.sprd.generalsecurity.utils;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.animation.Animation.AnimationListener;
import android.view.View;
import java.text.NumberFormat;
import android.util.Log;

public final class MemoryUtils {
    private static final String TAG = "Utils";
    public static void handleLoadingContainer(View loading,
            View doneLoading, boolean done, boolean animate) {
        setViewShown(loading, !done, animate);
        setViewShown(doneLoading, done, animate);
    }

    private static void setViewShown(final View view, boolean shown,
            boolean animate) {
        Log.i(TAG," view="+view + " ; shown="+shown+" ;animate="+animate);
        if (animate) {
            Animation animation = AnimationUtils.loadAnimation(view
                    .getContext(), shown ? android.R.anim.fade_in
                    : android.R.anim.fade_out);
            if (shown) {
                view.setVisibility(View.VISIBLE);
            } else {
                animation.setAnimationListener(new AnimationListener() {
                    @Override
                    public void onAnimationStart(Animation animation) {
                    }

                    @Override
                    public void onAnimationRepeat(Animation animation) {
                    }

                    @Override
                    public void onAnimationEnd(Animation animation) {
                        view.setVisibility(View.INVISIBLE);
                    }
                });
            }
            view.startAnimation(animation);
        } else {
            view.clearAnimation();
            view.setVisibility(shown ? View.VISIBLE : View.INVISIBLE);
        }
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

    public static int getPicId(int picCount, String per) {
        String p = per.substring(0, per.length() - 1);
        Log.i(TAG, "percent string:"+p);
        double percent = Double.parseDouble(p)/100.0;
        Log.i(TAG, "percent:"+percent+"    picCount:"+picCount);
        int id = (int) Math.floor(percent * picCount);
        Log.i(TAG, "\t\tid:"+id);
        if (percent >= 1) {
            return picCount - 1;
        } else if (percent <= 0) {
            return 0;
        } else if (id == picCount-1) {
            return id - 1;
        } else {
            return id;
        }
    }
}

