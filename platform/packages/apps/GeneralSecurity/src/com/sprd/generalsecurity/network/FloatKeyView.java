
package com.sprd.generalsecurity.network;

import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.SharedPreferences;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Handler;
import android.os.Message;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.UserHandle;
import android.text.format.Formatter;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.Slog;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.IWindowManager;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.SoundEffectConstants;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.TextView;
import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.TeleUtils;
import com.sprd.generalsecurity.utils.TeleUtils.NetworkType;
import java.lang.Override;

public class FloatKeyView extends LinearLayout implements OnTouchListener {
    private String TAG = "FloatKey";
    private static final String PREF_X = "FloatKeyViewX";
    private static final String PREF_Y = "FloatKeyViewY";
    private static final String POSITION = "position";
    private static final String PERCENT = "Percent";

    private static final String WIFI = "WIFI";
    private static final String SIM1 = "SIM1";
    private static final String SIM2 = "SIM2";
    private static final String SIM = "SIM";

    private static final String ACTION_SPEED_SETTING = "com.sprd.generalsecurity.network.dataflowmainentry";

    private Paint mPaint;
    private float mOffsetX;
    private float mOffsetY;
    private long mDownTime;

    private int mKeyWidth;
    private int mBoundWidth;

    private int mHideWidth;
    private int mHideHeight;

    private int mDragTrigger;

    private boolean mOnDrag;
    private float mDownX;
    private float mDownY;

    private WindowManager mWm;
    private WindowManager.LayoutParams mLp;
//    private Handler mHandler;

    private Context mContext;
//    private FloatPanelView mFloatPanelView;
    private SharedPreferences mSp;
    private boolean mShown = false;
    private int mPosition = 0;
    private float mDelta = 0;

    private TextView textNetworkType;

    private FloatKeyView(Context context) {
        super(context);
        mWm = (WindowManager) context.getSystemService(context.WINDOW_SERVICE);
        mContext = context;

        DisplayMetrics displayMetrics = getResources().getDisplayMetrics();
        float density = displayMetrics.density;

        LayoutInflater.from(context).inflate(R.layout.float_speed, this);
        View v = findViewById(R.id.speed_container);
        textNetworkType = (TextView)findViewById(R.id.networktype);

        mLp = new WindowManager.LayoutParams();
        mLp.width = LayoutParams.WRAP_CONTENT;
        mLp.height = (int)(density * 50);
        mLp.format = PixelFormat.RGBA_8888;
        mLp.type = WindowManager.LayoutParams.TYPE_DRAG;
        mLp.flags |= WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
        mLp.privateFlags |= WindowManager.LayoutParams.PRIVATE_FLAG_SHOW_FOR_ALL_USERS;
        mLp.gravity = Gravity.LEFT | Gravity.TOP;
        mLp.setTitle("FloatKeyView");

        mSp = mContext.getSharedPreferences("FloatKeyView", Context.MODE_PRIVATE);
        mPosition = mSp.getInt(POSITION, 0);
        mDelta = mSp.getFloat(PERCENT, 0);

        Resources res = getResources();

        setOnTouchListener(this);
    }

    private static FloatKeyView mInstance;
    public static FloatKeyView getInstance(Context context) {
        if (mInstance == null && context != null) {
            mInstance = new FloatKeyView(context);
        }
        return mInstance;
    }

    private void startSettingsActivity() {
        Intent it = new Intent(ACTION_SPEED_SETTING);
        it.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        it.addFlags(Intent.FLAG_ACTIVITY_BROUGHT_TO_FRONT);
        mContext.startActivity(it);
    }

    public void addToWindow() {
        if (!mShown) {
            setVisibility(VISIBLE);
            adjustEdge();
            mWm.addView(this, mLp);
            mShown = true;
            startRealSpeed();
        }
    }

    public void removeFromWindow() {
        if (mShown) {
            setVisibility(GONE);
            mWm.removeViewImmediate(this);
            mShown = false;
            stopRealSpeed();
        }
    }

    private static DataNetworkRate mNetworkRate;
    private static DataNetworkRate.NetworkTrafficCurrentRate mNetworkTrafficRateOld;
    private static DataNetworkRate.NetworkTrafficCurrentRate mNetworkTrafficRateNew;

    public Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);

            if (ActivityManager.getCurrentUser() != UserHandle.USER_SYSTEM) {
                removeFromWindow();
                return;
            }
            NetworkInfo info = mNetworkRate.getNetworkInfo(mContext);

            Log.e(TAG, "handlermessage");
            if (mNetworkRate.isMobileConnected(info)) {

                mNetworkTrafficRateNew = mNetworkRate.getCurrentNetworkTrafficRate(mContext, true);
            } else {
                mNetworkTrafficRateNew = mNetworkRate.getCurrentNetworkTrafficRate(mContext, false);
            }

            if (mNetworkTrafficRateOld != null && mNetworkTrafficRateNew != null) {
                long speed = mNetworkTrafficRateNew.downLinkRate - mNetworkTrafficRateOld.downLinkRate +
                                mNetworkTrafficRateNew.upLinkRate - mNetworkTrafficRateOld.upLinkRate;
                speed = speed < 0 ? 0 : speed;
                String rate;
                if (speed >= 1000) {
                    rate = Formatter.formatShortFileSize(mContext, speed) + "/S";
                } else {
                    rate = "" + speed + "B/S";
                }
                NetworkType type = TeleUtils.getCurrentNetworkType(mContext);

                if (type == NetworkType.WIFI){
                    setCurrentSpeed(WIFI, rate);
                } else if (type == NetworkType.SIM1 || type == NetworkType.SIM2) {
                    if (TeleUtils.getSimCount(mContext) == 1) {
                        setCurrentSpeed(SIM, rate);
                    } else {
                        if (type == NetworkType.SIM1) {
                            setCurrentSpeed(SIM1, rate);
                        } else if (type == NetworkType.SIM2) {
                            setCurrentSpeed(SIM2, rate);
                        }
                    }
                }else {
                    setCurrentSpeed(mContext.getResources().getString(R.string.current_speed_default), rate);
                }
            } else {
                String rate = Formatter.formatShortFileSize(mContext, 0);
                setCurrentSpeed(mContext.getResources().getString(R.string.current_speed_default), rate);
            }

            if (mHandler.hasMessages(0)) {
                mHandler.removeMessages(0);
            }
            mHandler.sendEmptyMessageDelayed(0, 1000);
            mNetworkTrafficRateOld = mNetworkTrafficRateNew;
        }
    };

    public void startRealSpeed() {
        addToWindow();
        if (mShown) {
            mNetworkRate = new DataNetworkRate(mContext);
            setCurrentSpeed(mContext.getResources().getString(R.string.current_speed_default), "0");
            NetworkInfo info = mNetworkRate.getNetworkInfo(mContext);

            if (mNetworkRate.isMobileConnected(info)) {
                mNetworkTrafficRateOld = mNetworkRate.getCurrentNetworkTrafficRate(mContext, true);
            } else {
                mNetworkTrafficRateOld = mNetworkRate.getCurrentNetworkTrafficRate(mContext, false);
            }
            if (mNetworkTrafficRateOld != null) {
                mHandler.removeMessages(0);
                mHandler.sendEmptyMessageDelayed(0, 1000);
            }
        }
    }

    protected void stopRealSpeed() {
        mHandler.removeMessages(0);
    }

    protected void hide() {
        if (mShown) {
            this.setVisibility(View.INVISIBLE);
        }
    }

    protected void show() {
        if (mShown) {
            this.setVisibility(View.VISIBLE);
        }
    }

    public void setCurrentSpeed(String type, String speed) {
        String networkSpeed = String.format(mContext.getResources().getString(R.string.network_speed), type, speed);
        textNetworkType.setText(networkSpeed);
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                mDownTime = SystemClock.uptimeMillis();

                setPressed(true);
                invalidate();

                mDownX = event.getRawX();
                mDownY = event.getRawY();

                mOffsetX = mDownX - mLp.x;
                mOffsetY = mDownY - mLp.y;
                break;

            case MotionEvent.ACTION_MOVE:
                if (mOnDrag) {
                    mLp.x = (int) (event.getRawX() - mOffsetX);
                    mLp.y = (int) (event.getRawY() - mOffsetY);
                    mWm.updateViewLayout(this, mLp);
                } else {
                    if (Math.abs(event.getRawX() - mDownX) > mDragTrigger
                            || Math.abs(event.getRawY() - mDownY) > mDragTrigger)
                        mOnDrag = true;
                }
                break;

            case MotionEvent.ACTION_CANCEL:
                setPressed(false);
                invalidate();
                mOnDrag = false;
                break;

            case MotionEvent.ACTION_UP:
                setPressed(false);
                invalidate();
                if (!mOnDrag) {
                    Slog.v(TAG, "ACTION_UP" + mOnDrag);
                    startSettingsActivity();
                    playSoundEffect(SoundEffectConstants.CLICK);
                } else {
                    Slog.v(TAG, "ACTION_UP" + mOnDrag);
                    mOnDrag = false;
                    DisplayMetrics metrics = getResources().getDisplayMetrics();
                    int screenWidth = metrics.widthPixels;
                    int screenHeight = metrics.heightPixels;
                    int xGap = screenWidth - mLp.x - getWidth();
                    int yGap = screenHeight - mLp.y - getHeight();
                    int minist = Math.min(Math.min(mLp.x, mLp.y), Math.min(xGap, yGap));
                    Log.d(TAG, minist + "");
                    if (mLp.x == minist) {
                        mLp.x = 0;
                        mPosition = 0;
                        mDelta = (float) mLp.y / screenHeight;
                    } else if (mLp.y == minist) {
                        mLp.y = 0;
                        mPosition = 1;
                        mDelta = (float) mLp.x / screenWidth;
                    } else if (xGap == minist) {
                        mLp.x = screenWidth - getWidth();
                        mPosition = 2;
                        mDelta = (float) mLp.y / screenHeight;
                    } else {
                        mLp.y = screenHeight - getHeight();
                        mPosition = 3;
                        mDelta = (float) mLp.x / screenWidth;
                    }
                    mWm.updateViewLayout(this, mLp);
                    SharedPreferences.Editor editor = mSp.edit();
                    editor.putInt(POSITION, mPosition);
                    editor.putFloat(PERCENT, mDelta);
                    editor.commit();
                }
                break;
        }
        return false;
    }

    @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        adjustEdge();
        super.onConfigurationChanged(newConfig);
    }

    private void adjustEdge() {
        DisplayMetrics metrics = getResources().getDisplayMetrics();
        int screenWidth = metrics.widthPixels;
        int screenHeight = metrics.heightPixels;

        switch (mPosition) {
            case 0:
                mLp.x = 0;
                mLp.y = (int) (screenHeight * mDelta);
                break;
            case 1:
                mLp.x = (int) (screenWidth * mDelta);
                mLp.y = 0;
                break;
            case 2:
                mLp.x = screenWidth - getWidth();
                mLp.y = (int) (mDelta * screenHeight);
                break;
            case 3:
                mLp.x = (int) (mDelta * screenWidth);
                mLp.y = screenHeight - getHeight();
                break;
        }
        if (mShown) {
            mWm.updateViewLayout(this, mLp);
        }

    }
}
