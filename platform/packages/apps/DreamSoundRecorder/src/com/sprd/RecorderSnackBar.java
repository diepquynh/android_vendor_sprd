package com.sprd.soundrecorder;

import android.content.Context;
import android.graphics.PixelFormat;
import android.os.Handler;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;
import com.android.soundrecorder.R;

/**
 * The view used to display the customized toast
 *
 * Usage:
 *
 * RecorderSnackBar snackBar = RecorderSnackBar.make(context, title, action,
 * listener,RecorderSnackBar.DEFAULT_DURATION);
 * snackBar.show();
 * snackBar.dismiss();
 */
public final class RecorderSnackBar extends View {

    private static final String TAG = "RecorderSnackBar";
    private static final Object LOCK = new Object();
    public static final int DEFAULT_DURATION = 3000;
    public static final int MIN_DURATION = 1000;
    private Context mContext = null;
    private WindowManager.LayoutParams mWindowParams = null;
    private RelativeLayout mLayout = null;
    private boolean mIsDisplayed = false;
    private Button mButton = null;
    private TextView mTextView = null;
    private OnActionTriggerListener mActionListener = null;
    private Handler mHandler = null;
    private int mDuration = DEFAULT_DURATION;

    private final Runnable mDismissionRunnable = new Runnable() {
        @Override
        public void run() {
            RecorderSnackBar.this.dismiss();
        }
    };

    /**
     * The callback listener, it will called while the action button
     * was set and the action button was clicked
     */
    public interface OnActionTriggerListener {
        /**
         * Action button callback
         */
        void onActionTriggered();
    }

    /**
     * To make a RecorderSnackBar instance
     *
     * @param context The context instance
     * @param title The notification text
     * @param actionName The action name displayed to end user
     * @param listener The callback listener
     * @param duration The displaying duration
     * @return The RecorderSnackBar instance
     */
    public static synchronized RecorderSnackBar make(Context context, String title, String actionName,
            OnActionTriggerListener listener, int duration) {
        RecorderSnackBar instance = new RecorderSnackBar(context);
        if (title == null) {
            instance.mTextView.setText("");
        } else {
            instance.mTextView.setText(title);
        }
        if (actionName != null & listener != null) {
            instance.mButton.setText(actionName);
            instance.mActionListener = listener;
            instance.mButton.setVisibility(View.VISIBLE);
        } else {
            instance.mButton.setVisibility(View.GONE);
        }
        if (duration < MIN_DURATION) {
            instance.mDuration = MIN_DURATION;
        } else {
            instance.mDuration = duration;
        }
        return instance;
    }

    private RecorderSnackBar(Context context) {
        super(context);
        init(context);
    }

    private void init(Context context) {
        mContext = context;
        mHandler = new Handler();
        mLayout = (RelativeLayout) RelativeLayout.inflate(context, R.layout.snackbar, null);
        mWindowParams = new WindowManager.LayoutParams();
        mWindowParams.type = WindowManager.LayoutParams.TYPE_APPLICATION;
        mWindowParams.format = PixelFormat.RGBA_8888;
        mWindowParams.flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
        mWindowParams.gravity = Gravity.LEFT | Gravity.BOTTOM;
        mWindowParams.x = 0;
        mWindowParams.y = 0;
        mWindowParams.width = WindowManager.LayoutParams.MATCH_PARENT;
        mWindowParams.height = WindowManager.LayoutParams.WRAP_CONTENT;

        mButton = (Button) mLayout.findViewById(R.id.snackbar_action);
        mButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                if (mActionListener != null) {
                    mActionListener.onActionTriggered();
                }
            }
        });
        mButton.setVisibility(View.GONE);

        mTextView = (TextView) mLayout.findViewById(R.id.snackbar_text);
    }

    /**
     * To display the view of RecorderSnackBar
     */
    public void show() {
        synchronized (LOCK) {
            WindowManager manager = (WindowManager) mContext
                    .getSystemService(Context.WINDOW_SERVICE);
            if (mIsDisplayed) {
                manager.removeViewImmediate(mLayout);
            }
            manager.addView(mLayout, mWindowParams);
            mIsDisplayed = true;
            mHandler.postDelayed(mDismissionRunnable, mDuration);
        }
    }

    /**
     * To dismiss the view of Snackbar
     */
    public void dismiss() {
        synchronized (LOCK) {
            WindowManager manager = (WindowManager) mContext
                    .getSystemService(Context.WINDOW_SERVICE);
            if (mIsDisplayed) {
                try {
                    manager.removeViewImmediate(mLayout);
                } catch (Exception e) {
                    Log.d(TAG, "dismiss, " + e.toString());
                }
            }
            mIsDisplayed = false;
        }
    }
}
