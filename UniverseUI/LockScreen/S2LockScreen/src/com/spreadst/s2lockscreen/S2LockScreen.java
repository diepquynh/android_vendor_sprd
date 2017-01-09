/** Create by Spreadst */

package com.spreadst.s2lockscreen;

import java.util.Date;
import java.util.HashMap;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.Drawable;
import android.os.Debug;
import android.os.Handler;
import android.os.SystemProperties;
import android.provider.Settings;
import android.sim.SimManager;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.text.format.DateFormat;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.widget.AbsLockScreen;
import android.widget.Button;
import android.widget.ILockScreenListener;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.android.internal.R;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.IccCardConstants.State;
import com.android.internal.widget.LockPatternUtils;

public class S2LockScreen extends AbsLockScreen implements View.OnClickListener{

    // SPRD: Modify 20130823 Spreadst of 204585 S2lockscreen add carrier info
    private static final boolean DEBUG = Debug.isDebug();
    private static final String TAG = "S2Lockscreen";
    protected static final String PACKAGE_NAME = "com.spreadst.s2lockscreen";

    private LayoutInflater inflater;
    private RelativeLayout lv_lock;
    private RelativeLayout lv_lockbg;
    private RelativeLayout lv_time;
    private RelativeLayout lv_main;
    private ImageView iv_circle;
    private ImageView iv_lock;
    private ImageView iv_big_lock;
    private ImageView iv_phone;
    private ImageView iv_msg;
    private ImageView iv_arrow;
    private ImageView iv_battery;
    private TextView tv_week;
    private TextView tv_battery;
    private TextView tv_phone;
    private TextView tv_msg;
    private DigitalClock mDigitalClock;

    private String mDateFormatString;
    private String battery_no;
    private String battery_full;
    private Drawable dw_circle, dw_circle_max, battery, dw_call, dw_msg, bg;
    private Drawable lock1, lock2, lock3, lock4, lock5, lock6;
    private AnimationDrawable ad_start, ad_close, ad_arrow, ad_arrow_in;

    private HashMap<Integer, Drawable> locksMap;
    private Handler mHandler;
    private Runnable downRun, moveRun, upRun;
    private int duration_start, duration_close, duration_ad_arrow_in;
    private double nowL;
    private float startX, startY, endX, endY;
    private Resources res;

    /* SPRD: Modify 20130823 Spreadst of 204585 S2lockscreen add carrier info @{ */
    private CharSequence[] mPlmn;
    private CharSequence[] mSpn;
    private CharSequence[] mCarrierTexts;
    private StatusMode mStatus;
    protected State[] mSimState;
    private TextView[] mCarrierViews;

    private SimManager simManager;

    /* @} */
    // SPRD: Modify 20130910 Spreadst of Bug 212843 unlock UUI lockscreen sound play two times
    private Boolean isUnlock = false;
    /* SPRD: Modify 20131009 Spreadst of Bug 222504 add emergencycall @{ */
    private Button mEmergencyCallButton;
    ServiceState[] mServiceState;
    protected int mPhoneState;
    private TelephonyManager[] mTelephonyManager;
    private PhoneStateListener[] mPhoneStateListener;
    /* @} */
    // SPRD: Modify 20131107 Spreadst of Bug 235935 emergency call info display wrong
    LockPatternUtils mLockPatternUtils;
    // SPRD: Modify 20140207 Spreadst of Bug 267015 add owner info text for UUI lockscreen
    private TextView owner_info;

    public S2LockScreen(Context context, ILockScreenListener listener) {
        super(context, listener);
        try {
            Log.d(TAG, TAG + " initView." + PACKAGE_NAME);
            mContext = context;
            mLockScreenListener = listener;
            // SPRD: Modify 20131107 Spreadst of Bug 235935 emergency call info display wrong
            mLockPatternUtils = new LockPatternUtils(context);
            res = context.getPackageManager().getResourcesForApplication(
                    PACKAGE_NAME);
            inflater = LayoutInflater.from(context);
            inflater.inflate(
                    res.getLayout(res.getIdentifier(PACKAGE_NAME
                            + ":layout/activity_main", null, null)), this, true);

            lv_main = (RelativeLayout) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/layout_main", null, null));
            lv_time = (RelativeLayout) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/layout_time", null, null));
            lv_lock = (RelativeLayout) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/lock_layout", null, null));
            lv_lockbg = (RelativeLayout) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/lock_layout_bg", null, null));
            iv_circle = (ImageView) findViewById(res.getIdentifier(PACKAGE_NAME
                    + ":id/ad_lock", null, null));
            iv_lock = (ImageView) findViewById(res.getIdentifier(PACKAGE_NAME
                    + ":id/lock", null, null));
            iv_phone = (ImageView) findViewById(res.getIdentifier(PACKAGE_NAME
                    + ":id/miss_phone", null, null));
            iv_msg = (ImageView) findViewById(res.getIdentifier(PACKAGE_NAME
                    + ":id/miss_msg", null, null));
            iv_big_lock = (ImageView) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/big_lock", null, null));
            iv_arrow = (ImageView) findViewById(res.getIdentifier(PACKAGE_NAME
                    + ":id/iv_arrow", null, null));
            iv_battery = (ImageView) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/battery", null, null));

            tv_week = (TextView) findViewById(res.getIdentifier(PACKAGE_NAME
                    + ":id/text_week", null, null));
            tv_battery = (TextView) findViewById(res.getIdentifier(PACKAGE_NAME
                    + ":id/battery_info", null, null));
            tv_phone = (TextView) findViewById(res.getIdentifier(PACKAGE_NAME
                    + ":id/phone_no", null, null));
            tv_msg = (TextView) findViewById(res.getIdentifier(PACKAGE_NAME
                    + ":id/msg_no", null, null));

            mDateFormatString = res.getString(res.getIdentifier(PACKAGE_NAME
                    + ":string/abbrev_wday_month_day_no_year", null, null));

            dw_call = res.getDrawable(res.getIdentifier(PACKAGE_NAME
                    + ":drawable/call_sprd", null, null));
            dw_msg = res.getDrawable(res.getIdentifier(PACKAGE_NAME
                    + ":drawable/message_sprd", null, null));
            battery = res.getDrawable(res.getIdentifier(PACKAGE_NAME
                    + ":drawable/battery_sprd", null, null));
            dw_circle = res.getDrawable(res.getIdentifier(PACKAGE_NAME
                    + ":drawable/circle_sprd", null, null));
            dw_circle_max = res.getDrawable(res.getIdentifier(PACKAGE_NAME
                    + ":drawable/circle_action_10_sprd", null, null));
            bg = res.getDrawable(res.getIdentifier(PACKAGE_NAME
                    + ":drawable/bg_sprd", null, null));

            lock1 = res.getDrawable(res.getIdentifier(PACKAGE_NAME
                    + ":drawable/lock_01_sprd", null, null));
            lock2 = res.getDrawable(res.getIdentifier(PACKAGE_NAME
                    + ":drawable/lock_02_sprd", null, null));
            lock3 = res.getDrawable(res.getIdentifier(PACKAGE_NAME
                    + ":drawable/lock_03_sprd", null, null));
            lock4 = res.getDrawable(res.getIdentifier(PACKAGE_NAME
                    + ":drawable/lock_04_sprd", null, null));
            lock5 = res.getDrawable(res.getIdentifier(PACKAGE_NAME
                    + ":drawable/lock_05_sprd", null, null));
            lock6 = res.getDrawable(res.getIdentifier(PACKAGE_NAME
                    + ":drawable/lock_06_sprd", null, null));
            locksMap = new HashMap<Integer, Drawable>();
            locksMap.put(1, lock1);
            locksMap.put(2, lock2);
            locksMap.put(3, lock3);
            locksMap.put(4, lock4);
            locksMap.put(5, lock5);
            locksMap.put(6, lock6);

            ad_start = (AnimationDrawable) AnimationDrawable.createFromXml(
                    res,
                    res.getAnimation(res.getIdentifier(PACKAGE_NAME
                            + ":anim/open_anim", null, null)));
            ad_close = (AnimationDrawable) AnimationDrawable.createFromXml(
                    res,
                    res.getAnimation(res.getIdentifier(PACKAGE_NAME
                            + ":anim/close_anim", null, null)));
            ad_arrow = (AnimationDrawable) AnimationDrawable.createFromXml(
                    res,
                    res.getAnimation(res.getIdentifier(PACKAGE_NAME
                            + ":anim/arrow_anim", null, null)));
            ad_arrow_in = (AnimationDrawable) AnimationDrawable.createFromXml(
                    res,
                    res.getAnimation(res.getIdentifier(PACKAGE_NAME
                            + ":anim/arrow_in_anim", null, null)));

            iv_lock.setImageDrawable(lock1);
            iv_lock.setBackgroundDrawable(dw_circle);
            iv_big_lock.setImageDrawable(dw_circle_max);
            iv_battery.setImageDrawable(battery);
            iv_phone.setImageDrawable(dw_call);
            iv_msg.setImageDrawable(dw_msg);

            battery_no = res.getString(res.getIdentifier(PACKAGE_NAME
                    + ":string/battery_in", null, null));
            battery_full = res.getString(res.getIdentifier(PACKAGE_NAME
                    + ":string/battery_full", null, null));

            /* SPRD: Modify 20140207 Spreadst of Bug 267015 add owner info text for UUI lockscreen @{ */
            owner_info = (TextView) findViewById(res.getIdentifier(PACKAGE_NAME + ":id/owner_info",
                    null, null));
            String ownerInfo = getOwnerInfo();
            if (!TextUtils.isEmpty(ownerInfo)) {
                final boolean screenOn = mLockScreenListener.isScreenOn();
                owner_info.setSelected(screenOn); // This is required to ensure marquee works
                owner_info.setText(ownerInfo);
            } else {
                owner_info.setVisibility(View.INVISIBLE);
            }
            /* @} */

            /*
             * SPRD: Modify 20130823 Spreadst of 204585 S2lockscreen add carrier
             * info @{
             */
            mPlmn = new CharSequence[TelephonyManager.getPhoneCount()];
            mSpn = new CharSequence[TelephonyManager.getPhoneCount()];
            mSimState = new State[TelephonyManager.getPhoneCount()];
            mCarrierTexts = new CharSequence[TelephonyManager.getPhoneCount()];
            simManager = SimManager.get(mContext);
            mCarrierViews = new TextView[TelephonyManager.getPhoneCount()];
            LinearLayout carriersArea = (LinearLayout) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/carrier", null, null));
            for (int i = 0; i < TelephonyManager.getPhoneCount(); i++) {
                TextView carrier = new TextView(getContext());
                carrier.setTextSize(16);
                carrier.setGravity(Gravity.CENTER_HORIZONTAL);
                carriersArea.addView(carrier);
                mCarrierViews[i] = carrier;
            }
            /* @} */
            /* SPRD: Modify 20131009 Spreadst of Bug 222504 add emergencycall @{ */
            mServiceState = new ServiceState[TelephonyManager.getPhoneCount()];
            mPhoneStateListener = new PhoneStateListener[TelephonyManager.getPhoneCount()];
            mTelephonyManager = new TelephonyManager[TelephonyManager.getPhoneCount()];
            for (int i=0; i < TelephonyManager.getPhoneCount(); i++) {
                mTelephonyManager[i] = (TelephonyManager)getContext().getSystemService(TelephonyManager.getServiceName(
                        Context.TELEPHONY_SERVICE, i));
                mPhoneStateListener[i] = getPhoneStateListener(i);
                mTelephonyManager[i].listen(mPhoneStateListener[i],PhoneStateListener.LISTEN_SERVICE_STATE);
            }

            mEmergencyCallButton = (Button) findViewById(res.getIdentifier(PACKAGE_NAME
                    + ":id/emergencycallbutton", null, null));

            if (mEmergencyCallButton != null) {
                if (!canEmergencyCall()) {
                    mEmergencyCallButton.setClickable(false);
                    mEmergencyCallButton.setText(R.string.emergency_call_no_service);

                } else {
                    mEmergencyCallButton.setClickable(true);
                    mEmergencyCallButton.setText(R.string.lockscreen_emergency_call);
                }
                mEmergencyCallButton.setOnClickListener(this);
                mEmergencyCallButton.setFocusable(false); // touch only!
            }
            /* @} */
            refreshDate();
            lv_main.setOnTouchListener(mOntouchListener);
            iv_phone.setOnTouchListener(mOntouchListener);
            iv_msg.setOnTouchListener(mOntouchListener);

            initTouchEvent();
            initDigitalClock(context);
            onMissedCallCountChanged(TelephoneInfoManager
                    .getMissedCalls(context));
            onDeleteMessageCount(TelephoneInfoManager
                    .getUnReadMessageCount(context));
            onRefreshBatteryInfo(listener.shouldShowBatteryInfo(),
                    listener.isDevicePluggedIn(), listener.getBatteryLevel());

            System.gc();
        } catch (Exception e) {
            Log.e(TAG, "S2Lockscreen", e);
        }

    }

    private void initTouchEvent() {

        if (mHandler != null)
            return;
        mHandler = new Handler();

        duration_start = Tools.initAdDuration(ad_start);
        duration_close = Tools.initAdDuration(ad_close);

        downRun = new Runnable() {
            public void run() {
                iv_circle.setVisibility(View.VISIBLE);
                iv_circle.setImageDrawable(ad_arrow);
                ad_arrow.start();
                iv_big_lock.setVisibility(View.VISIBLE);
            }
        };

        moveRun = new Runnable() {
            public void run() {
                iv_circle.setImageDrawable(ad_arrow);
                ad_arrow.start();
            }
        };

        upRun = new Runnable() {
            public void run() {
                ad_arrow.stop();
                iv_big_lock.setVisibility(View.GONE);
                iv_circle.setVisibility(View.GONE);
            }
        };

    }

    private void initDigitalClock(Context context) {
        mDigitalClock = new DigitalClock(context);
        // SPRD: Modify 20140207 Spreadst of Bug 277050 time show incomplete when word style is DroidSerif-Regular
        mDigitalClock.setTextSize(TypedValue.COMPLEX_UNIT_DIP, 90);
        mDigitalClock.setSingleLine();
        mDigitalClock.setTextColor(0xffdcdcdc);
        mDigitalClock.setShadowLayer(0.5f, 0.1f, 0.1f, Color.BLACK);
        lv_time.addView(mDigitalClock);
    }

    View.OnTouchListener mOntouchListener = new OnTouchListener() {

        @Override
        public boolean onTouch(View v, MotionEvent event) {

            Log.d(TAG, "     onTouch  :  event : " + v.getId());
            mLockScreenListener.pokeWakelock();
            boolean isLayout = v.getId() == lv_main.getId();
            float maxL = iv_big_lock.getWidth() / 2;
            float mixL = isLayout ? iv_big_lock.getWidth() / 2 / 2 : v
                    .getWidth() / 2;

            Log.d(TAG, "maxL  :  " + maxL);
            Log.d(TAG, "mixL  :  " + mixL);

            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:

                    Log.v(TAG, "     onTouch  :  ACTION_DOWN  ");
                    lv_lockbg.setBackgroundDrawable(bg);
                    startX = isLayout ? event.getX() : v.getX() + v.getWidth() / 2;
                    startY = isLayout ? event.getY() : v.getY() + v.getHeight() / 2;

                    lv_lock.setX(startX - lv_lock.getWidth() / 2);
                    lv_lock.setY(startY - lv_lock.getHeight() / 2);

                    if (isLayout) {
                        iv_lock.setImageDrawable(lock1);
                        iv_lock.setBackgroundDrawable(dw_circle);
                    } else {
                        iv_lock.setImageDrawable(dw_circle);
                        iv_lock.setBackgroundColor(Color.TRANSPARENT);
                    }
                    iv_lock.setAlpha(255);
                    iv_lock.getBackground().setAlpha(255);
                    iv_lock.setVisibility(View.VISIBLE);
                    iv_arrow.setVisibility(View.VISIBLE);

                    ad_close.stop();
                    mHandler.removeCallbacks(upRun);

                    iv_big_lock.setVisibility(View.GONE);
                    iv_big_lock.setAlpha(128);
                    iv_circle.setAlpha(128);
                    iv_circle.setVisibility(View.VISIBLE);
                    iv_circle.setVisibility(View.VISIBLE);
                    iv_circle.setImageDrawable(ad_start);
                    ad_start.start();

                    mHandler.postDelayed(downRun, duration_start);

                    break;

                case MotionEvent.ACTION_MOVE:
                    endX = isLayout ? event.getX() : v.getX() + event.getX();
                    endY = isLayout ? event.getY() : v.getY() + event.getY();
                    nowL = Tools.isout(startX, startY, endX, endY);

                    if (nowL >= mixL - (maxL - mixL) / 7) {
                        ad_arrow.stop();
                        ad_start.stop();
                        iv_circle.setVisibility(View.GONE);
                        int alpha = Tools.getL(nowL, maxL, mixL);
                        if (isLayout) {
                            iv_lock.setImageDrawable(locksMap.get(alpha));
                            iv_lock.getBackground().setAlpha(255 / (alpha * 2));
                        } else {
                            iv_lock.setAlpha(255 / alpha);
                        }
                        iv_big_lock.setAlpha(130 + alpha * 21);
                        if (Tools.isout(startX, startY, endX, endY, maxL)) {
                            // SPRD: Bug 310170 After leaving the screen to unlock
                            iv_lock.setImageDrawable(lock6);
                        }

                    } else if ((Tools.isout(startX, startY, endX, endY, mixL / 2))
                            && iv_circle.getVisibility() == View.GONE) {
                        if (isLayout)
                            iv_lock.setImageDrawable(lock1);
                        iv_lock.setAlpha(255);
                        iv_lock.getBackground().setAlpha(255);
                        iv_circle.setVisibility(View.VISIBLE);
                        iv_circle.setImageDrawable(ad_arrow_in);
                        ad_arrow_in.start();
                        mHandler.postDelayed(moveRun, duration_ad_arrow_in);
                    }
                    break;

                case MotionEvent.ACTION_UP:
                    lv_lockbg.setBackgroundColor(Color.TRANSPARENT);
                    Log.d(TAG, "     onTouch  :   ACTION_UP  ");
                    if (v.getBackground() != null)
                        v.getBackground().setAlpha(255);
                    ad_arrow_in.stop();
                    ad_arrow.stop();
                    iv_lock.setVisibility(View.GONE);
                    iv_arrow.setVisibility(View.GONE);
                    iv_big_lock.setVisibility(View.GONE);

                    ad_start.stop();
                    iv_circle.setImageDrawable(ad_close);
                    ad_close.start();

                    mHandler.removeCallbacks(downRun);
                    mHandler.removeCallbacks(moveRun);
                    mHandler.postDelayed(upRun, duration_close);
                    mLockScreenListener.pokeWakelock();
                    /* SPRD: Bug 310170 After leaving the screen to unlock @{ */
                    if (Tools.isout(startX, startY, endX, endY, maxL)) {
                        lv_lock.setVisibility(View.GONE);
                        try {
                            if (v.getId() == iv_phone.getId()) {
                                Call_Action.startHasmterIntentAction(mContext,
                                        Call_Action.ACTION_CALL);
                            } else if (v.getId() == iv_msg.getId())
                                Call_Action.startHasmterIntentAction(mContext,
                                        Call_Action.ACTION_SMS);
                        } catch (Exception e) {
                            Log.e(TAG, "start phone or msm error", e);
                        }
                        mLockScreenListener.goToUnlockScreen();
                        return false;
                    }
                    /* @} */
                    break;

                default:
                    // Log.v(TAG , "     onTouch  :   default ..........  ");
                    break;
            }
            return true;
            // v.onTouchEvent(event);
        }
    };

    private void myGC() {
        if (mHandler != null) {
            mHandler.removeCallbacks(downRun);
            mHandler.removeCallbacks(moveRun);
            mHandler.removeCallbacks(upRun);
        }
        if (ad_arrow_in != null) {
            ad_arrow_in.stop();
            ad_arrow_in = null;
        }
        if (ad_arrow != null) {
            ad_arrow.stop();
            ad_arrow = null;
        }
        if (ad_start != null) {
            ad_start.stop();
            ad_start = null;
        }
        if (ad_close != null) {
            ad_close.stop();
            ad_close = null;
        }
        if (mDigitalClock != null)
            mDigitalClock = null;
        if (locksMap != null)
            locksMap = null;
        if (downRun != null)
            downRun = null;
        if (moveRun != null)
            moveRun = null;
        if (upRun != null)
            upRun = null;
        System.gc();
    }

    @Override
    public void onTimeChanged() {
        refreshDate();
    }

    void refreshDate() {
        /*
         * Modify 20130626 Spreadst of 174786 Date format is different from
         * LockSettings style start
         */
        final Context context = getContext();
        Date now = new Date();
        CharSequence dow = DateFormat.format("EEEE", now);
        java.text.DateFormat shortDateFormat = DateFormat.getDateFormat(context);
        CharSequence date = shortDateFormat.format(now.getTime());
        tv_week.setText(date + "\n" + dow);
        /*
         * Modify 20130626 Spreadst of 174786 Date format is different from
         * LockSettings style end
         */
    }

    @Override
    public void onRefreshBatteryInfo(boolean showBatteryInfo,
            boolean pluggedIn, int batteryLevel) {
        Log.i(TAG, "showBatteryInfo :  " + showBatteryInfo);
        Log.i(TAG, "pluggedIn :  " + pluggedIn);
        Log.i(TAG, "batteryLevel :  " + batteryLevel);
        if (pluggedIn) {
            tv_battery.setText(batteryLevel == 100 ? battery_full : battery_no
                    + batteryLevel + "%");
            tv_battery.setVisibility(View.VISIBLE);
            iv_battery.setVisibility(View.VISIBLE);
        } else {
            tv_battery.setVisibility(View.GONE);
            iv_battery.setVisibility(View.GONE);
        }
        Log.i(TAG, "onRefreshBatteryInfo... ");

    }

    @Override
    public boolean needsInput() {
        Log.i(TAG, "needsInput... ");
        return false;
    }

    @Override
    public void onPhoneStateChanged(int phoneState) {
        if (DEBUG)
            Log.d(TAG, "onPhoneStateChanged = " + phoneState);
        mPhoneState = phoneState;
        updateEmergencyCallButtonState(phoneState);
    }

    @Override
    public void onRefreshCarrierInfo(CharSequence plmn, CharSequence spn,
            int subscription) {
        /*
         * SPRD: Modify 20130823 Spreadst of 204585 S2lockscreen add carrier
         * info @{
         */
        Log.d(PACKAGE_NAME, "onRefreshCarrierInfo  plmn = " + plmn + "| spn = " + spn);
        mPlmn[subscription] = plmn;
        mSpn[subscription] = spn;
        updateCarrierStateWithSimStatus(mSimState[subscription], subscription);
        /* @} */
    }

    @Override
    public void onRingerModeChanged(int state) {
        Log.i(TAG, "onRingerModeChanged... ");
    }

    @Override
    public void onSimStateChanged(State simState, int subscription) {
        /* SPRD: Modify 20130823 Spreadst of 204585 S2lockscreen add carrier info @{ */
        Log.d(PACKAGE_NAME, "onSimStateChanged  simState = " + simState + "| subscription = " + subscription);
        updateCarrierStateWithSimStatus(simState, subscription);
        /* @} */
    }

    @Override
    public void onPause() {
        Log.i(TAG, "onPause... ");
        /* SPRD: Modify 20131009 Spreadst of Bug 222504 add emergencycall @{ */
        for (int i = 0; i < TelephonyManager.getPhoneCount(); i++) {
            ((TelephonyManager) getContext().getSystemService(TelephonyManager.getServiceName(Context.TELEPHONY_SERVICE,i))).listen(
                    mPhoneStateListener[i],
                    PhoneStateListener.LISTEN_NONE);
        }
        /* @} */
    }

    @Override
    public void onResume() {
        Log.i(TAG, "onResume... ");
        /* SPRD: Modify 20131009 Spreadst of Bug 222504 add emergencycall @{ */
        for (int i=0; i < TelephonyManager.getPhoneCount(); i++) {
            ((TelephonyManager)getContext().getSystemService(TelephonyManager.getServiceName(
                    Context.TELEPHONY_SERVICE, i))).listen(mPhoneStateListener[i],PhoneStateListener.LISTEN_SERVICE_STATE);
        }
        /* @} */
    }

    @Override
    public void cleanUp() {
        myGC();
        Log.i(TAG, "cleanUp... ");
    }

    @Override
    public void onStartAnim() {
        Log.i(TAG, "needsInput... ");
    }

    @Override
    public void onStopAnim() {
    }

    @Override
    public void onClockVisibilityChanged() {
        Log.i(TAG, "onClockVisibilityChanged... ");
    }

    @Override
    public void onDeviceProvisioned() {
        Log.i(TAG, "onDeviceProvisioned... ");
    }

    @Override
    public void onMessageCountChanged(int messagecount) {
        /*
         * Modify 20130730 Spreadst of 195388 S2LockScreen not show missedcall
         * and unread message info start
         */
        Log.d(TAG, "onMessageCountChanged messagecount = " + messagecount);
        if (iv_msg != null && tv_msg != null) {
            if (messagecount > 0) {
                iv_msg.setVisibility(View.VISIBLE);
                tv_msg.setVisibility(View.VISIBLE);
                tv_msg.setText("" + messagecount);
            } else {
                iv_msg.setVisibility(View.GONE);
                tv_msg.setVisibility(View.GONE);
            }
        }
        /*
         * Modify 20130730 Spreadst of 195388 S2LockScreen not show missedcall
         * and unread message info end
         */
    }

    @Override
    public void onDeleteMessageCount(int messagecount) {
        Log.i(TAG, "onDeleteMessageCount :   " + messagecount);
        if (messagecount > 0) {
            iv_msg.setVisibility(View.VISIBLE);
            tv_msg.setVisibility(View.VISIBLE);
            tv_msg.setText("" + messagecount);
        } else {
            iv_msg.setVisibility(View.GONE);
            tv_msg.setVisibility(View.GONE);
        }
    }

    @Override
    public void onMissedCallCountChanged(int count) {
        Log.i(TAG, "onMissedCallCountChanged:  " + count);
        if (count > 0) {
            iv_phone.setVisibility(View.VISIBLE);
            tv_phone.setVisibility(View.VISIBLE);
            tv_phone.setText("" + count);
        } else {
            iv_phone.setVisibility(View.GONE);
            tv_phone.setVisibility(View.GONE);
        }
    }

    /* SPRD: Modify 20130823 Spreadst of 204585 S2lockscreen add carrier info @{ */
    /**
     * Update carrier text, carrier help and emergency button to match the
     * current status based on SIM state.
     *
     * @param simState
     */
    private void updateCarrierStateWithSimStatus(State simState, int phoneId) {
        if (DEBUG)
            Log.d(TAG, "updateCarrierTextWithSimStatus(), simState = " + simState);

        CharSequence[] carrierText = new CharSequence[TelephonyManager.getPhoneCount()];
        mStatus = getStatusForIccState(simState);
        mSimState[phoneId] = simState;
        switch (mStatus) {
            case Normal:
                carrierText[phoneId] = makeCarierString(mPlmn[phoneId], mSpn[phoneId]);
                break;

            case NetworkLocked:
                carrierText[phoneId] = makeCarierString(mPlmn[phoneId],
                        getContext().getText(R.string.lockscreen_network_locked_message));
                break;

            case SimMissing:
                // Shows "No SIM card | Emergency calls only" on devices that
                // are voice-capable.
                // This depends on mPlmn containing the text
                // "Emergency calls only" when the radio
                // has some connectivity. Otherwise, it should be null or empty
                // and just show
                // "No SIM card"
                // carrierText[phoneId] = getContext().getText(
                // R.string.lockscreen_missing_sim_message_short);
                /*
                 * if (mLockPatternUtils.isEmergencyCallCapable()) {
                 * carrierText[phoneId] =
                 * makeCarierString(mPlmn[phoneId],carrierText[phoneId]); }
                 */
                if ("cucc".equals(SystemProperties.get("ro.operator", ""))) {
                    carrierText[phoneId] = makeCarierString("",
                            getContext().getText(R.string.lockscreen_missing_sim_message_short));
                } else {
                    carrierText[phoneId] = makeCarierString(mPlmn[phoneId],
                            getContext().getText(R.string.lockscreen_missing_sim_message_short));
                }
                break;

            case SimPermDisabled:
                carrierText[phoneId] = makeCarierString(mPlmn[phoneId],
                        getContext().getText(R.string.lockscreen_blocked_sim_message_short));
                break;

            case SimMissingLocked:
                carrierText[phoneId] = makeCarierString(mPlmn[phoneId],
                        getContext().getText(R.string.lockscreen_missing_sim_message_short));
                break;

            case SimLocked:
                carrierText[phoneId] = makeCarierString(mPlmn[phoneId],
                        getContext().getText(R.string.lockscreen_sim_locked_message));
                break;

            case SimPukLocked:
                carrierText[phoneId] = makeCarierString(mPlmn[phoneId],
                        getContext().getText(R.string.lockscreen_sim_puk_locked_message));
                break;
            case SimAirplaneMode:
                carrierText[phoneId] = getContext().getText(R.string.lockscreen_carrier_default);

                break;
        }
        setCarrierText(carrierText[phoneId], phoneId);
        if (DEBUG)
            Log.d(PACKAGE_NAME, "updateCarrierTextWithSimStatus()--" +
                    ", carrierText[" + phoneId + "]=" + carrierText[phoneId] + ", mStatus="
                    + mStatus + ", phoneId=" + phoneId);
        // SPRD: Modify 20131009 Spreadst of Bug 222504 add emergencycall
        updateEmergencyCallButtonState(mPhoneState);
    }

    /**
     * The status of this lock screen. Primarily used for widgets on LockScreen.
     */
    enum StatusMode {
        /**
         * Normal case (sim card present, it's not locked)
         */
        Normal(true),

        /**
         * The sim card is 'network locked'.
         */
        NetworkLocked(true),

        /**
         * The sim card is missing.
         */
        SimMissing(false),

        /**
         * The sim card is missing, and this is the device isn't provisioned, so
         * we don't let them get past the screen.
         */
        SimMissingLocked(false),

        /**
         * The sim card is PUK locked, meaning they've entered the wrong sim
         * unlock code too many times.
         */
        SimPukLocked(false),

        /**
         * The sim card is locked.
         */
        SimLocked(true),

        /**
         * The sim card is permanently disabled due to puk unlock failure
         */
        SimPermDisabled(false),

        /**
         * The sim card is NOT_READY and AIRPLANE_MODE is on.
         */
        SimAirplaneMode(true);

        private final boolean mShowStatusLines;

        StatusMode(boolean mShowStatusLines) {
            this.mShowStatusLines = mShowStatusLines;
        }

        /**
         * @return Whether the status lines (battery level and / or next alarm)
         *         are shown while in this state. Mostly dictated by whether
         *         this is room for them.
         */
        public boolean shouldShowStatusLines() {
            return mShowStatusLines;
        }
    }

    /**
     * Performs concentenation of PLMN/SPN
     *
     * @param plmn
     * @param spn
     * @return
     */
    private static CharSequence makeCarierString(CharSequence plmn,
            CharSequence spn) {
        if ("cucc".equals(SystemProperties.get("ro.operator", ""))) {
            return concatenateForCUCC(plmn, spn);
        } else {
            return concatenate(plmn, spn);
        }
    }

    void setCarrierText(CharSequence string, int phoneId) {
        /*
         * SPRD: Modify 20130821 Spreadst of 204612 draglockscreen carrier show
         * wrong @{
         */
        mCarrierTexts[phoneId] = string;
        int phoneCount = TelephonyManager.getPhoneCount();
        if (phoneCount > 1) {
            boolean isAllCardsAbsent = true;
            for (int i = 0; i < phoneCount; i++) {
                isAllCardsAbsent = isAllCardsAbsent && (mSimState[i] == IccCardConstants.State.ABSENT);
            }
            for (int i = 0;i < phoneCount; i++) {
                if (isAllCardsAbsent) {
                    mCarrierViews[0].setText(mCarrierTexts[0]);
                    mCarrierViews[0].setVisibility(View.VISIBLE);
                    mCarrierViews[0].setTextColor(getResources().getColor(com.android.internal.R.color.secondary_text_dark));
                    for (int j = 1; j < phoneCount; j++) {
                        mCarrierViews[j].setVisibility(View.INVISIBLE);
                    }
                } else {
                    mCarrierViews[i].setText(mCarrierTexts[i]);
                    mCarrierViews[i].setVisibility(View.VISIBLE);
                    /** SPRD: Bug 306537 lockscreen carrier set color for UUI @{ */
                    if (mSimState[i] == IccCardConstants.State.ABSENT) {
                        mCarrierViews[i].setTextColor(getResources().getColor(com.android.internal.R.color.secondary_text_dark));
                        if ("cucc".equals(SystemProperties.get("ro.operator", ""))) {
                            mCarrierViews[i].setVisibility(View.GONE);
                        }
                    } else {
                        setCarrierTextColor(i);
                    }
                    /** @} */
                }
            }
        } else {
            setCarrierTextColor(0);
            mCarrierViews[0].setText(mCarrierTexts[0]);
        }
        /* @} */
    }

    /**
     * Determine the current status of the lock screen given the sim state and
     * other stuff.
     */
    public StatusMode getStatusForIccState(IccCardConstants.State simState) {
        // Since reading the SIM may take a while, we assume it is present until
        // told otherwise.
        if (simState == null) {
            return StatusMode.Normal;
        }

        final boolean isAirPlaneMode = Settings.System.getInt(getContext()
                .getContentResolver(), Settings.System.AIRPLANE_MODE_ON, 0) != 0;

        switch (simState) {
            case ABSENT:
                return StatusMode.SimMissing;
            case NETWORK_LOCKED:
                return StatusMode.SimMissingLocked;
            case NOT_READY:
                if (isAirPlaneMode) {
                    return StatusMode.SimAirplaneMode;
                } else {
                    return StatusMode.SimMissing;
                }
            case PIN_REQUIRED:
                return StatusMode.SimLocked;
            case PUK_REQUIRED:
                return StatusMode.SimPukLocked;
            case READY:
                return StatusMode.Normal;
                // case BLOCKED:
            case PERM_DISABLED:
                return StatusMode.SimPermDisabled;
            case UNKNOWN:
                return StatusMode.SimMissing;
        }
        return StatusMode.SimMissing;
    }
    /* @} */

    private void setCarrierTextColor(int phoneId) {
        int mSimColor = 0;
        if (simManager != null) {
            mSimColor = simManager.getColor(phoneId);
        }
        if (mSimColor < 0) {
            mCarrierViews[phoneId].setTextColor(mSimColor);
        }
    }

    /* SPRD: Modify 20131009 Spreadst of Bug 222504 add emergencycall @{ */
    @Override
    public void onClick(View v) {
        if (DEBUG) Log.d(TAG, "onClick emergencycallbutton");
        if (v.getId() == res.getIdentifier(PACKAGE_NAME + ":id/emergencycallbutton", null, null)) {
            Log.d(PACKAGE_NAME, "takeEmergencyCallAction()");
            mLockScreenListener.takeEmergencyCallAction();
        }
    }
    /* @} */

    private boolean canEmergencyCall(){
        boolean isEmergencyOnly = false;
        boolean hasService = false;
        for (int i = 0; i < TelephonyManager.getPhoneCount(); i++) {
            if (mServiceState[i] != null) {
                isEmergencyOnly = isEmergencyOnly ? true : mServiceState[i].isEmergencyOnly();
                hasService = hasService(i);
                Log.d(TAG, "canEmergencyCall i = "+ i +"| isEmergencyOnly = " + isEmergencyOnly + "|hasService = " + hasService);
                if(hasService){
                    return true;
                } else {
                    if(isEmergencyOnly){
                        return true;
                    }
                }
            }
        }
        return false;
    }

    private PhoneStateListener getPhoneStateListener(final int phoneId) {
        PhoneStateListener phoneStateListener = new PhoneStateListener() {
            @Override
            public void onServiceStateChanged(ServiceState state) {
                if (DEBUG)
                    Log.v(TAG, "onServiceStateChanged(), phoneId = " + phoneId + " , serviceState = " + state);
                    mServiceState[phoneId] = state;
                updateEmergencyCallButtonState(mPhoneState);
            }
        };
        return phoneStateListener;
    }

    private boolean hasService(int subscription) {
        if (mServiceState[subscription] != null) {
            switch (mServiceState[subscription].getState()) {
                case ServiceState.STATE_OUT_OF_SERVICE:
                    return false;
                default:
                    return true;
            }
        } else {
            return false;
        }
    }

    private void updateEmergencyCallButtonState(int phoneState) {
        if (DEBUG)
            Log.d(TAG, "updateEmergencyCallButtonState(), phoneState = " + phoneState);
        if (mEmergencyCallButton != null) {
            // SPRD: Modify 20131107 Spreadst of Bug 235935 emergency call info display wrong
            mLockPatternUtils.updateEmergencyCallButtonState(mEmergencyCallButton, phoneState,
                    true, false);
            if (!canEmergencyCall()) {
                mEmergencyCallButton.setClickable(false);
                mEmergencyCallButton.setText(R.string.emergency_call_no_service);
            } else {
                mEmergencyCallButton.setClickable(true);
            }
        }
        if ("cucc".equals(SystemProperties.get("ro.operator", ""))) {
            boolean hasIccCard = false;
            for (int i = 0; i < TelephonyManager.getPhoneCount(); i++) {
                hasIccCard |= mTelephonyManager[i].hasIccCard();
            }
            if (!hasIccCard) {
                mEmergencyCallButton.setVisibility(View.INVISIBLE);
            }
        }
    }

    /* SPRD: Modify 20140207 Spreadst of Bug 267015 add owner info text for UUI lockscreen @{ */
    private String getOwnerInfo() {
        String info = null;
        final boolean ownerInfoEnabled = mLockPatternUtils.isOwnerInfoEnabled();
        if (ownerInfoEnabled) {
            info = mLockPatternUtils.getOwnerInfo(mLockPatternUtils.getCurrentUser());
        }
        return info;
    }

    @Override
    public void onScreenTurnedOn() {
        Log.d(TAG, "onScreenTurnedOn");
        if (owner_info != null) {
            owner_info.setSelected(true);
        }
    }

    @Override
    public void onScreenTurnedOff(int why) {
        Log.d(TAG, "onScreenTurnedOff");
        if (owner_info != null) {
            owner_info.setSelected(false);
        }
    }
    /* @} */

    private static CharSequence concatenate(CharSequence plmn, CharSequence spn) {
        final boolean plmnValid = !TextUtils.isEmpty(plmn);
        final boolean spnValid = !TextUtils.isEmpty(spn);
        if (plmnValid && spnValid) {
            return new StringBuilder().append(plmn).append(" | ").append(spn).toString();
        } else if (plmnValid) {
            return plmn;
        } else if (spnValid) {
            return spn;
        } else {
            return "";
        }
    }

    private static CharSequence concatenateForCUCC(CharSequence plmn, CharSequence spn) {
        final boolean plmnValid = !TextUtils.isEmpty(plmn);
        final boolean spnValid = !TextUtils.isEmpty(spn);
        if (plmnValid) {
            return plmn;
        } else if (spnValid) {
            return spn;
        } else {
            return "";
        }
    }
}
