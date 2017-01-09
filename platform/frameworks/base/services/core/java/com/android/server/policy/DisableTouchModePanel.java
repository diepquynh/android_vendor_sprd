
package com.android.server.policy;


import android.content.Context;
import android.content.res.Resources;
import android.content.SharedPreferences;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;

import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Slog;

import android.view.KeyEvent;
import android.view.View;
import android.view.MotionEvent;
import android.view.View.OnTouchListener;
import android.view.ViewConfiguration;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.view.IWindowManager;
import android.view.Gravity;
import android.view.ViewGroup;
import android.view.LayoutInflater;

/**
 * The view added by windowmanager for pocket-mode.
 * @hide
 */
public class DisableTouchModePanel {
    private static final String TAG = "DisableTouchModePanel";
    private static final int MSG_RESET_PANEL = 1;
    private static final int MSG_SHOW_PANEL = 2;
    private static final int MSG_HIDE_PANEL = 3;
    private static final int MSG_SHOW_BACKGROUND = 4;
    private final WindowManager mWindowManager;
    private final Object mLock = new Object();
    Handler mHandler = new DH();
    private final View.OnTouchListener mOnTouchListener = new View.OnTouchListener() {
        @Override
        public boolean onTouch(View view, MotionEvent event) {
            if (PhoneWindowManager.DEBUG_INPUT) {
                Slog.i(TAG, "onTouch, set backgournd to visible, mShowBG = " + mShowBG);
            }
            //set the background view opaque
            showBackground();
            return true;
        }
    };
    private Context mContext;
    private View mDisableTouchView;
    private boolean mShowBG = false;

    private int STATUS_HIDDEN = 0;
    private int STATUS_SHOWING = 1;
    private int STATUS_SHOWN = 2;
    private int mShow = STATUS_HIDDEN;

    private boolean mInit = false;
    private boolean mPocketModeOn = false;
    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (PhoneWindowManager.DEBUG_INPUT) {
                Slog.i(TAG, "mReceiver  onReceive  intent.getAction(): " + intent.getAction());
            }
            if (intent.getAction().equals(Intent.ACTION_LOCALE_CHANGED) && mDisableTouchView != null) {
                if (PhoneWindowManager.DEBUG_INPUT) {
                    Slog.i(TAG, "language changed, reinit Disable-Touch view");
                }
                mHandler.sendEmptyMessage(MSG_RESET_PANEL);
            }
        }
    };

    DisableTouchModePanel(Context context) {
        mContext = context;
        mWindowManager = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
    }

    void setPocketMode(boolean isOn) {
        synchronized (mLock) {
            mPocketModeOn = isOn;
            if (!mPocketModeOn && mShow == STATUS_SHOWN) {
                show(false);
            } else if (!mPocketModeOn && mShow == STATUS_SHOWING) {
                mShow = STATUS_HIDDEN;
            }
        }
    }

    boolean getPocketMode() {
        synchronized (mLock) {
            return mPocketModeOn;
        }
    }

    void showBackground() {
        if (!mShowBG) {
            mHandler.sendEmptyMessage(MSG_SHOW_BACKGROUND);
        }
    }

    void init() {
        synchronized (mLock) {
            if (mDisableTouchView == null) {
                try {
                    //Inflate the disable-touch-mode-panel view
                    mDisableTouchView = LayoutInflater.from(mContext).inflate(com.android.internal.R.layout.disable_touch_mode_panel, null);

                    //set this view to invisible and transparent
                    mDisableTouchView.setVisibility(View.INVISIBLE);
                    mDisableTouchView.setAlpha(0.0f);

                    //set onTouchListener for this view
                    mDisableTouchView.setOnTouchListener(mOnTouchListener);

                    //add this view in WMS
                    WindowManager.LayoutParams lp = getLayoutParams();
                    mWindowManager.addView(mDisableTouchView, lp);

                    IntentFilter filter = new IntentFilter();
                    filter.addAction(Intent.ACTION_LOCALE_CHANGED);
                    mContext.registerReceiver(mReceiver, filter);

                    mInit = true;
                    if (PhoneWindowManager.DEBUG_INPUT) {
                        Slog.i(TAG, "init");
                    }
                } catch (Exception e) {
                    Slog.e(TAG, "init failed, exception: " + e);
                    Throwable tr = new Throwable();
                    tr.fillInStackTrace();
                    Slog.e(TAG, "Throw", tr);
                }
            }
        }
    }

    void reset() {
        mHandler.sendEmptyMessage(MSG_RESET_PANEL);
    }

    public void show(boolean show) {
        synchronized (mLock) {
            if (show) {
                mShow = STATUS_SHOWING;
                mHandler.sendEmptyMessage(MSG_SHOW_PANEL);
            } else {
                mHandler.sendEmptyMessage(MSG_HIDE_PANEL);
            }
        }
    }

    void resetHandle() {
        synchronized (mLock) {
            try {
                if (mInit && mDisableTouchView != null) {
                    mDisableTouchView.setOnTouchListener(null);
                    mWindowManager.removeView(mDisableTouchView);
                    mDisableTouchView = null;

                    //Inflate the disable-touch-mode-panel view
                    mDisableTouchView = LayoutInflater.from(mContext).inflate(com.android.internal.R.layout.disable_touch_mode_panel, null);

                    //set this view to invisible and transparent
                    mDisableTouchView.setVisibility(View.INVISIBLE);
                    mDisableTouchView.setAlpha(0.0f);

                    //set onTouchListener for this view
                    mDisableTouchView.setOnTouchListener(mOnTouchListener);

                    //add this view in WMS
                    WindowManager.LayoutParams lp = getLayoutParams();
                    mWindowManager.addView(mDisableTouchView, lp);

                    if (PhoneWindowManager.DEBUG_INPUT) {
                        Slog.i(TAG, "reset");
                    }
                }
            }catch (Exception e) {
                Slog.e(TAG, "reset failed, exception: " + e);
                Throwable tr = new Throwable();
                tr.fillInStackTrace();
                Slog.e(TAG, "Throw", tr);
            }
        }
    }

    private void showBackgroundHandle() {
        synchronized (mLock) {
            if (!mShowBG) {
                mShowBG = true;
                mDisableTouchView.setAlpha(1.0f);
            }
        }
    }

    private void showHandle() {
        synchronized (mLock) {
            if (mDisableTouchView != null && mPocketModeOn) {
                if (PhoneWindowManager.DEBUG_INPUT) {
                    Slog.i(TAG, "DisableTouchView, show = true, mShow = " + mShow);
                }
                if (mShow == STATUS_SHOWING) {
                    if (PhoneWindowManager.DEBUG_INPUT) {
                        Slog.i(TAG, "set VISIBLE");
                    }
                    mDisableTouchView.setVisibility(View.VISIBLE);
                    mDisableTouchView.setAlpha(0.0f);
                    mShow = STATUS_SHOWN;
                    mShowBG = false;
                }
            }
        }
    }

    private void hideHandle() {
        synchronized (mLock) {
            if (mDisableTouchView != null) {
                if (PhoneWindowManager.DEBUG_INPUT) {
                    Slog.i(TAG, "DisableTouchView, show = false, mShow = " + mShow);
                }
                if (mShow == STATUS_SHOWN) {
                    if (PhoneWindowManager.DEBUG_INPUT) {
                        Slog.i(TAG, "STATUS_SHOWN, set GONE");
                    }
                    mDisableTouchView.setVisibility(View.GONE);
                    mDisableTouchView.setAlpha(0.0f);
                    mShow = STATUS_HIDDEN;
                    mShowBG = false;
                } else if (mShow == STATUS_SHOWING) {
                    if (PhoneWindowManager.DEBUG_INPUT) {
                        Slog.i(TAG, "STATUS_SHOWING, set GONE");
                    }
                    mShow = STATUS_HIDDEN;
                }
            }
        }
    }

    public boolean isShown() {
        synchronized (mLock) {
            return mShow == STATUS_SHOWN;
        }
    }

    public boolean isShowing() {
        synchronized (mLock) {
            return mShow == STATUS_SHOWING || mShow == STATUS_SHOWN;
        }
    }

    public boolean isBackgroundShown() {
        synchronized (mLock) {
            return mShowBG;
        }
    }

    private WindowManager.LayoutParams getLayoutParams() {
        WindowManager.LayoutParams lp = new WindowManager.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.TYPE_SYSTEM_ERROR,
                WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
                PixelFormat.TRANSLUCENT);
        lp.gravity = Gravity.BOTTOM | Gravity.START;
        lp.setTitle("DisableTouchModePanel");
        lp.softInputMode = WindowManager.LayoutParams.SOFT_INPUT_STATE_UNCHANGED
                | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING;
        return lp;
    }

    private class DH extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_RESET_PANEL:
                    resetHandle();
                    break;
                case MSG_SHOW_PANEL:
                    showHandle();
                    break;
                case MSG_HIDE_PANEL:
                    hideHandle();
                    break;
                case MSG_SHOW_BACKGROUND:
                    showBackgroundHandle();
                default:
                    break;
            }
        }
    }
}
