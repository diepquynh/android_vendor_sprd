package com.sprd.fileexplorer.util;


import android.content.Context;
import android.os.Handler;
import android.widget.Toast;

public class NotifyManager {
    
    private static final int TOAST_LONG_DELAY = 3500;
    private static final int TOAST_SHORT_DELAY = 2000;
    
    private static NotifyManager mManager;
    
    private Context mContext;
    private Toast mToast;
    private Handler mMainThreadHandler;

    private NotifyManager(Context context) {
        mContext = context;
        mMainThreadHandler = new Handler(mContext.getMainLooper());
    }

    public static void init(Context context) {
        if (context == null) {
            throw new NullPointerException("context == null");
        }
        mManager = new NotifyManager(context);
    }

    public static NotifyManager getInstance() {
        return mManager;
    }

    private Runnable cancelToast = new Runnable() {
        @Override
        public void run() {
            mToast = null;
        }
    };

    public void showToast(String msg) {
        //Should get text from toast instead of last message.
        showToast(msg, Toast.LENGTH_SHORT);
    }

    public void showToast(String msg, int duration) {
        if (msg == null) {
            throw new NullPointerException("Toast to show what?");
        }
        int delay = TOAST_SHORT_DELAY;
        if(duration == Toast.LENGTH_LONG) {
            delay = TOAST_LONG_DELAY;
        }
        if (mToast != null) {
            mToast.setText(msg);
        } else {
            mMainThreadHandler.removeCallbacks(cancelToast);
            mToast = Toast.makeText(mContext, msg, duration);
            mMainThreadHandler.postDelayed(cancelToast, delay);
            mToast.show();
        }
    }

}
