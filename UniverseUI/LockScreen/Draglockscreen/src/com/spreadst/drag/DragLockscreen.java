/** Create by Spreadst */

package com.spreadst.drag;

import java.util.Date;

import android.app.ActivityManagerNative;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.media.AudioManager;
import android.media.RemoteController;
import android.media.RemoteController.MetadataEditor;
import android.net.Uri;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.Vibrator;
import android.provider.MediaStore;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.sim.SimManager;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.text.format.DateFormat;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.AbsLockScreen;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ILockScreenListener;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.android.internal.R;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.IccCardConstants.State;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.widget.LockPatternUtils;
import com.android.music.IMediaPlaybackService;

public class DragLockscreen extends AbsLockScreen implements
        View.OnTouchListener, OnViewChangeListener, View.OnLongClickListener,
        View.OnClickListener {

    protected static final String PACKAGE_NAME = "com.spreadst.drag";
    private static final boolean DEBUG = true;
    private static final String TAG = "DragLockscreen";
    private static final int MESSAGE_MMS_GONE = 1;

    private static final int MESSAGE_PHONE_GONE = 2;

    private static final int MESSAGE_MUSIC_PLAY = 4;

    protected static Context mContext = null;

    private PreScrollLayout mScrollLayout;

    private Resources mRes;

    private LinearLayout mUnlockblock;

    private RelativeLayout mContactsblock;

    private LinearLayout mMusicblock;

    private TextView mmspromptTextView;

    private TextView phonepromptTextView;

    private int mWhichScreen = 1;

    private Handler mProcesssShowHandle;

    private LayoutInflater mInflater;

    private ContactsData mContactsData;

    private ImageView mImgv_arrow;

    private int arrow_index = 0;

    private ImageView mPlayandpause;

    private ImageView mBack;

    private ImageView mForword;

    private TextView mTxtTrackName;

    private TextView mTxtArtistName;

    private TextView mTxtPosition;

    private TextView mTxtDuration;

    private ProgressBar mProgressBar;

    private Runnable mProRunnable;

    private boolean is_arrow_round;

    private final static String[] ARROW_STRING_IDS = {
            "ic_lock_arrow1_sprd",
            "ic_lock_arrow2_sprd", "ic_lock_arrow3_sprd", "ic_lock_arrow4_sprd"
    };

    private Drawable[] mDra_arrows;

    private View mSc1;

    private View mSc2;

    private View mSc3;

    private View mSc4;

    private LinearLayout mTime_layout;

    private DigitalClock mDigitalClock;

    private BitmapDrawable mBitmapDrawable;

    /* Add 20130515 Spreadst of 163555 ,Lock screen start */
    private final static String IS_LOCK_SCREEN_DIAL = "is_lock_screen_dial";
    /* Add 20130515 Spreadst of 163555 ,Lock screen end */

    private CharSequence[] mPlmn;
    private CharSequence[] mSpn;
    // SPRD: Modify 20130822 Spreadst of 204612 draglockscreen carrier show
    // wrong
    private CharSequence[] mCarrierTexts;
    private StatusMode mStatus;
    protected State[] mSimState;
    // SPRD: Modify 20130822 Spreadst of 204612 draglockscreen carrier show
    // wrong
    private TextView[] mCarrierViews;

    private SimManager simManager;
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

    private ImageView iv_battery;
    private TextView tv_battery;
    private String battery_no;
    private String battery_full;
    private Drawable battery;

    public DragLockscreen(Context context, ILockScreenListener listener)
            throws NameNotFoundException {
        super(context, listener);
        Log.d(PACKAGE_NAME, "create DragLockscreen " +this);
        mContext = context;
        // SPRD: Modify 20131107 Spreadst of Bug 235935 emergency call info display wrong
        mLockPatternUtils = new LockPatternUtils(context);
        Resources res = context.getPackageManager().getResourcesForApplication(
                PACKAGE_NAME);
        mRes = res;
        mProcesssShowHandle = new ProcesssShowHandle();
        if (SERVICECONNECTION == null) {
            SERVICECONNECTION = new PlayServiceConnection();
        }
        if (MMUSICSERVICE == null) {
            context.startService(new  Intent("com.android.music.IMediaPlaybackService"));
            boolean issuccess = context.bindService(new Intent(
                    "com.android.music.IMediaPlaybackService"),
                    SERVICECONNECTION, 0);
            Log.d(PACKAGE_NAME, "music service binded is issuccess = "
                    + issuccess);
        }
        mPlmn = new CharSequence[TelephonyManager.getPhoneCount()];
        mSpn = new CharSequence[TelephonyManager.getPhoneCount()];
        mSimState = new State[TelephonyManager.getPhoneCount()];
        // SPRD: Modify 20130822 Spreadst of 204612 draglockscreen carrier show
        // wrong
        mCarrierTexts = new CharSequence[TelephonyManager.getPhoneCount()];
        simManager = SimManager.get(mContext);
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
        /* @} */
        init();
        // SPRD: Modify 20131009 Spreadst of Bug 222504 add emergencycall
        initEmergencyCallButton();
        setUnlockPage();
        setContacts();
        setPlayer();
    }

    @Override
    protected void finalize() throws Throwable {
        // TODO Auto-generated method stub
        super.finalize();
        Log.d(PACKAGE_NAME, "DragLockscreen finalize " + this);
    }

    private void init() {

        mInflater = LayoutInflater.from(mContext);
        mInflater.inflate(
                mRes.getLayout(Tools.getLayoutId(mRes, "draglockscreen")),
                this, true);

        FrameLayout frameLayout = (FrameLayout) findViewById(Tools.getInnerId(
                mRes, "frameLayout1"));
        mScrollLayout = new PreScrollLayout(mContext);
        frameLayout.addView(mScrollLayout);
        mScrollLayout.init(mContext, mWhichScreen);
        mScrollLayout.SetOnViewChangeListener(this);
        mSc1 = mInflater
                .inflate(mRes.getLayout(Tools.getLayoutId(mRes,
                        "singlescreen_0")), null);
        mScrollLayout.addView(mSc1);
        mSc2 = mInflater.inflate(mRes.getLayout(Tools.getLayoutId(mRes,
                "vertical_singlescreen_1")), null);
        mScrollLayout.addView(mSc2);
        mSc3 = mInflater
                .inflate(mRes.getLayout(Tools.getLayoutId(mRes,
                        "singlescreen_2")), null);
        mScrollLayout.addView(mSc3);
        mSc4 = mInflater.inflate(mRes.getLayout(Tools.getLayoutId(mRes,
                "vertical_singlescreen_0")), null);
        mScrollLayout.addView(mSc4);
    }

    private void setUnlockPage() {
        mUnlockblock = (LinearLayout) findViewById(Tools.getInnerId(mRes,
                "unlockblock"));
        Drawable dra_Unlockblock = mRes.getDrawable(Tools.getDrawableId(mRes,
                "ic_lock_bg_sprd"));
        mUnlockblock.setBackgroundDrawable(dra_Unlockblock);
        mTime_layout = (LinearLayout) findViewById(Tools.getInnerId(mRes,
                "time_layout"));
        mDigitalClock = new DigitalClock(mContext);
        mDigitalClock.setTextSize(37);
        mDigitalClock.setSingleLine();
        mDigitalClock.setTextColor(Color.WHITE);
        mTime_layout.addView(mDigitalClock);

        TextView mmsTextView = (TextView) findViewById(Tools.getInnerId(mRes,
                "mmstext"));
        mmsTextView.setText(mRes.getString(Tools.getStringId(mRes, "to_mms")));
        Drawable dra_message = mRes.getDrawable(Tools.getDrawableId(mRes,
                "ic_lock_message_sprd"));
        mmsTextView.setCompoundDrawablesWithIntrinsicBounds(null, dra_message,
                null, null);

        TextView phoneTextView = (TextView) findViewById(Tools.getInnerId(mRes,
                "phonetext"));
        phoneTextView
                .setText(mRes.getString(Tools.getStringId(mRes, "to_all")));
        Drawable dra_call = mRes.getDrawable(Tools.getDrawableId(mRes,
                "ic_lock_call_sprd"));
        phoneTextView.setCompoundDrawablesWithIntrinsicBounds(null, dra_call,
                null, null);

        LinearLayout unlock_ll = (LinearLayout) findViewById(Tools.getInnerId(
                mRes, "unlock_layout"));

        ImageView imageView1 = (ImageView) findViewById(Tools.getInnerId(mRes,
                "imageView1"));
        ImageView imageView2 = (ImageView) findViewById(Tools.getInnerId(mRes,
                "imageView2"));
        Drawable dra_lockBg = mRes.getDrawable(Tools.getDrawableId(mRes,
                "ic_lock_seperationline_sprd"));
        imageView1.setBackgroundDrawable(dra_lockBg);
        imageView2.setBackgroundDrawable(dra_lockBg);
        mDra_arrows = new Drawable[ARROW_STRING_IDS.length];
        for (int i = 0; i < ARROW_STRING_IDS.length; i++) {
            mDra_arrows[i] = mRes.getDrawable(Tools.getDrawableId(mRes,
                    ARROW_STRING_IDS[i]));
        }
        mImgv_arrow = (ImageView) findViewById(Tools.getInnerId(mRes, "arrow"));
        Drawable dra_arrow = mRes.getDrawable(Tools.getDrawableId(mRes,
                "ic_lock_arrow1_sprd"));
        mImgv_arrow.setBackgroundDrawable(dra_arrow);

        TextView promptTextView = (TextView) findViewById(Tools.getInnerId(
                mRes, "prompt"));
        promptTextView.setText(mRes.getString(Tools.getStringId(mRes,
                "drag_prompt")));

        mmspromptTextView = (TextView) findViewById(Tools.getInnerId(mRes,
                "mmsprompt"));
        phonepromptTextView = (TextView) findViewById(Tools.getInnerId(mRes,
                "phoneprompt"));
        Drawable dra_mmsprompt = mRes.getDrawable(Tools.getDrawableId(mRes,
                "list_message_sprd"));
        Drawable dra_phoneprompt = mRes.getDrawable(Tools.getDrawableId(mRes,
                "list_call_sprd"));
        mmspromptTextView.setCompoundDrawablesWithIntrinsicBounds(
                dra_mmsprompt, null, null, null);
        phonepromptTextView.setCompoundDrawablesWithIntrinsicBounds(
                dra_phoneprompt, null, null, null);

        unlock_ll.setOnTouchListener(this);
        mmsTextView.setOnTouchListener(this);
        phoneTextView.setOnTouchListener(this);
        /*
         * SPRD: Modify 20130821 Spreadst of 204612 draglockscreen carrier show
         * wrong @{
         */
        mCarrierViews = new TextView[TelephonyManager.getPhoneCount()];
        LinearLayout carriersArea = (LinearLayout) findViewById(Tools.getInnerId(
                mRes, "carrier"));
        for (int i = 0; i < TelephonyManager.getPhoneCount(); i++) {
            TextView carrier = new TextView(getContext());
            carrier.setTextSize(16);
            carrier.setGravity(Gravity.CENTER_HORIZONTAL);
            carriersArea.addView(carrier);
            mCarrierViews[i] = carrier;
        }
        /* @} */

        /* SPRD: Modify 20140207 Spreadst of Bug 267015 add owner info text for UUI lockscreen @{ */
        owner_info = (TextView) findViewById(Tools.getInnerId(mRes, "owner_info"));
        String ownerInfo = getOwnerInfo();
        if (!TextUtils.isEmpty(ownerInfo)) {
            final boolean screenOn = mLockScreenListener.isScreenOn();
            owner_info.setSelected(screenOn); // This is required to ensure
                                              // marquee works
            owner_info.setText(ownerInfo);
        } else {
            owner_info.setVisibility(View.INVISIBLE);
        }
        /* @} */
        iv_battery = (ImageView) findViewById(Tools.getInnerId(mRes, "battery"));
        tv_battery = (TextView) findViewById(Tools.getInnerId(mRes, "battery_info"));
        battery_no = mRes.getString(Tools.getStringId(mRes, "battery_in"));
        battery_full = mRes.getString(Tools.getStringId(mRes, "battery_full"));
        battery = mRes.getDrawable(Tools.getDrawableId(mRes, "battery_sprd"));
        iv_battery.setImageDrawable(battery);
    }

    private void setContacts() {
        mContactsblock = (RelativeLayout) findViewById(Tools.getInnerId(mRes,
                "contactsblock"));
        Drawable dra_contactsblock = mRes.getDrawable(Tools.getDrawableId(mRes,
                "ic_lock_bg_sprd"));
        mContactsblock.setBackgroundDrawable(dra_contactsblock);
        ImageView cutback_image = (ImageView) findViewById(Tools.getInnerId(
                mRes, "cutback_image"));
        Drawable dra_cutBg = mRes.getDrawable(Tools.getDrawableId(mRes,
                "ic_lock_line_sprd"));
        cutback_image.setBackgroundDrawable(dra_cutBg);
        mContactsData = new ContactsData(mContext, mProcesssShowHandle);
        mContactsData.startLoadData();
    }

    private void setPlayer() {
        mMusicblock = (LinearLayout) findViewById(Tools.getInnerId(mRes,
                "musicblock"));
        Drawable dra_musicblock = mRes.getDrawable(Tools.getDrawableId(mRes,
                "ic_lock_bg_sprd"));
        mMusicblock.setBackgroundDrawable(dra_musicblock);
        mPlayandpause = (ImageView) findViewById(Tools.getInnerId(mRes,
                "playandpause"));
        mBack = (ImageView) findViewById(Tools.getInnerId(mRes, "back"));
        mBack.setBackgroundDrawable(mRes.getDrawable(Tools.getDrawableId(mRes,
                "ic_lock_music_previous_normal_sprd")));
        mForword = (ImageView) findViewById(Tools.getInnerId(mRes, "forword"));
        mForword.setBackgroundDrawable(mRes.getDrawable(Tools.getDrawableId(
                mRes, "ic_lock_music_next_normal_sprd")));
        mTxtTrackName = (TextView) findViewById(Tools.getInnerId(mRes,
                "txtTrackName"));
        mTxtArtistName = (TextView) findViewById(Tools.getInnerId(mRes,
                "txtArtistName"));
        mTxtPosition = (TextView) findViewById(Tools.getInnerId(mRes,
                "txtPosition"));
        mTxtDuration = (TextView) findViewById(Tools.getInnerId(mRes,
                "txtDuration"));
        setProgressBar();
        mPlayandpause.setOnClickListener(this);
        mBack.setOnClickListener(this);
        mForword.setOnClickListener(this);
        Log.d(PACKAGE_NAME, "setPlayer send MESSAGE_MUSIC_PLAY");
        mProcesssShowHandle.sendEmptyMessage(MESSAGE_MUSIC_PLAY);
        mAudioManager = new AudioManager(mContext);
        mRemoteController = new RemoteController(mContext, mRCClientUpdateListener);
        mAudioManager.registerRemoteController(mRemoteController);
    }

    private void setProgressBar() {
        mProgressBar = (ProgressBar) findViewById(Tools.getInnerId(mRes,
                "progressBar1"));
        mProgressBar.setEnabled(false);
        mProgressBar.setMax(1000);
        Drawable dra_progress = mRes.getDrawable(Tools.getDrawableId(mRes,
                "progress_drawable"));
        LayerDrawable progressDrawable = (LayerDrawable) dra_progress;
        Log.d(PACKAGE_NAME,
                "" + progressDrawable.getNumberOfLayers());
        progressDrawable.setDrawableByLayerId(android.R.id.secondaryProgress,
                mRes.getDrawable(Tools.getDrawableId(mRes, "secondary_progress_sprd")));
        mProgressBar.setProgressDrawable(dra_progress);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);

        if (mUnlockblock != null) {
            Log.d(PACKAGE_NAME, "unlockblock=" + mUnlockblock.getHeight());
            mScrollLayout.setUnlockblockHeight(mUnlockblock.getHeight());
        }
        is_arrow_round = true;
        updateMessageArrowRound();
    }

    @Override
    public void onRefreshBatteryInfo(boolean showBatteryInfo,
            boolean pluggedIn, int batteryLevel) {
        Log.d(PACKAGE_NAME, "pluggedIn :  " + pluggedIn);
        Log.d(PACKAGE_NAME, "batteryLevel :  " + batteryLevel);
        if (pluggedIn) {
            tv_battery.setText(batteryLevel == 100 ? battery_full : battery_no
                    + batteryLevel + "%");
            tv_battery.setVisibility(View.VISIBLE);
            iv_battery.setVisibility(View.VISIBLE);
        } else {
            tv_battery.setVisibility(View.GONE);
            iv_battery.setVisibility(View.GONE);
        }
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
        TextView dateTextView = (TextView) findViewById(Tools.getInnerId(mRes, "date"));
        final Context context = getContext();
        Date now = new Date();
        CharSequence dow = DateFormat.format("EEEE", now);
        java.text.DateFormat shortDateFormat = DateFormat.getDateFormat(context);
        CharSequence date = shortDateFormat.format(now.getTime());
        dateTextView.setText(date + "\n" + dow);
        /*
         * Modify 20130626 Spreadst of 174786 Date format is different from
         * LockSettings style end
         */
    }

    @Override
    public boolean needsInput() {
        return false;
    }

    @Override
    public void onPhoneStateChanged(int phoneState) {
        /* SPRD: Modify 20131009 Spreadst of Bug 222504 add emergencycall @{ */
        if (DEBUG) Log.d(TAG, "onPhoneStateChanged = " + phoneState);
        mPhoneState = phoneState;
        updateEmergencyCallButtonState(phoneState);
        /* @} */
    }

    @Override
    public void onRefreshCarrierInfo(CharSequence plmn, CharSequence spn,
            int subscription) {
        Log.d(PACKAGE_NAME, "onRefreshCarrierInfo  plmn = " + plmn + "| spn = " + spn);
        mPlmn[subscription] = plmn;
        mSpn[subscription] = spn;
        updateCarrierStateWithSimStatus(mSimState[subscription], subscription);
    }

    @Override
    public void onRingerModeChanged(int state) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onSimStateChanged(State simState, int subscription) {
        Log.d(PACKAGE_NAME, "onSimStateChanged  simState = " + simState + "| subscription = "
                + subscription);
        updateCarrierStateWithSimStatus(simState, subscription);
    }

    @Override
    public void onPause() {
        if (DEBUG) Log.d(PACKAGE_NAME, "onPause");
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
        if (DEBUG) Log.d(PACKAGE_NAME, "onResume");
        /* SPRD: Modify 20131009 Spreadst of Bug 222504 add emergencycall @{ */
        for (int i=0; i < TelephonyManager.getPhoneCount(); i++) {
            ((TelephonyManager)getContext().getSystemService(TelephonyManager.getServiceName(
                    Context.TELEPHONY_SERVICE, i))).listen(mPhoneStateListener[i],PhoneStateListener.LISTEN_SERVICE_STATE);
        }
        /* @} */
    }

    @Override
    public void cleanUp() {
        Log.d(PACKAGE_NAME, "cleanUp " + this);
        mStopPlay = true;
        if (mProcesssShowHandle != null) {
            mProcesssShowHandle.removeMessages(MESSAGE_MUSIC_PLAY);
        }
        if (mContactsData != null) {
            mContactsData.cancelTask();
        }
        if (mDigitalClock != null) {
            mDigitalClock = null;
        }
        is_arrow_round = false;
        mScrollLayout.unRegistrationViewChangeListener();
        Tools.recycleAllBitmap();
        mAudioManager.unregisterRemoteController(mRemoteController);
    }

    @Override
    public void onStartAnim() {
        // TODO Auto-generated method stub

    }

    @Override
    public void onStopAnim() {
        // TODO Auto-generated method stub

    }

    @Override
    public void onClockVisibilityChanged() {
        // TODO Auto-generated method stub

    }

    @Override
    public void onDeviceProvisioned() {
        // TODO Auto-generated method stub

    }

    @Override
    public void onMessageCountChanged(int messagecount) {

        Log.d(PACKAGE_NAME, "onMessageCountChanged=" + messagecount);
        if (messagecount == 0) {
            return;
        }
        if (mmspromptTextView != null) {
            mmspromptTextView.setText(mRes.getString(
                    Tools.getStringId(mRes, "mms_prompt"),
                    String.valueOf(messagecount)));
            mmspromptTextView.setVisibility(View.VISIBLE);
            mProcesssShowHandle.sendEmptyMessageDelayed(MESSAGE_MMS_GONE, 3000);
        }

    }

    @Override
    public void onDeleteMessageCount(int messagecount) {
        Log.d(PACKAGE_NAME, "onDeleteMessageCount=" + messagecount);
    }

    @Override
    public void onMissedCallCountChanged(int count) {
        Log.d(PACKAGE_NAME, "onMissedCallCountChanged=" + count);
        if (count == 0) {
            return;
        }
        if (phonepromptTextView != null) {
            phonepromptTextView.setText(mRes.getString(
                    Tools.getStringId(mRes, "call_prompt"),
                    String.valueOf(count)));
            phonepromptTextView.setVisibility(View.VISIBLE);
            mProcesssShowHandle.sendEmptyMessageDelayed(MESSAGE_PHONE_GONE,
                    3000);
        }
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            if (v.getId() == Tools.getInnerId(mRes, "unlock_layout")) {
                toVibrator();
                mScrollLayout.setToAction(mScrollLayout.ACTION_TO_UNLOCK);
            }
            if (v.getId() == Tools.getInnerId(mRes, "mmstext")) {
                toVibrator();
                mScrollLayout.setToAction(mScrollLayout.ACTION_TO_MMS);

            }
            if (v.getId() == Tools.getInnerId(mRes, "phonetext")) {
                toVibrator();
                mScrollLayout.setToAction(mScrollLayout.ACTION_TO_PHONE);
            }
        }
        return false;
    }

    private void startCallIntent() {
        if (mContext != null) {
            Intent intent = new Intent(Intent.ACTION_DIAL);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                    | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
            mContext.startActivity(intent);
        }

    }

    private void startSMSIntent() {
        if (mContext != null) {
            Intent intent = new Intent(Intent.ACTION_MAIN);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                    | Intent.FLAG_ACTIVITY_SINGLE_TOP
                    | Intent.FLAG_ACTIVITY_CLEAR_TOP);
            intent.setType("vnd.android-dir/mms-sms");
            mContext.startActivity(intent);
        }
    }

    @Override
    public void OnViewChange(int view) {
        mWhichScreen = view;
    }

    @Override
    public void pokeWakelock() {
        mLockScreenListener.pokeWakelock();
    }

    @Override
    public void goToUnlockScreen() {
        mLockScreenListener.goToUnlockScreen();
    }

    @Override
    public void goToCall() {
        startCallIntent();
    }

    @Override
    public void goToSMS() {
        startSMSIntent();
    }

    private class ProcesssShowHandle extends Handler {

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {

                case MESSAGE_MMS_GONE:
                    Log.d(PACKAGE_NAME, "MESSAGE_MMS_GONE");
                    mmspromptTextView.setVisibility(View.GONE);
                    break;

                case MESSAGE_PHONE_GONE:
                    Log.d(PACKAGE_NAME, "MESSAGE_PHONE_GONE");
                    phonepromptTextView.setVisibility(View.GONE);
                    break;

                case ContactsData.DATA_ALREADY_UPLOAD:
                    Log.d(PACKAGE_NAME, "DATA_ALREADY_UPLOAD");
                    int[] cellids = {
                            Tools.getInnerId(mRes, "cell1"),
                            Tools.getInnerId(mRes, "cell2"),
                            Tools.getInnerId(mRes, "cell3"),
                            Tools.getInnerId(mRes, "cell4"),
                            Tools.getInnerId(mRes, "cell5"),
                            Tools.getInnerId(mRes, "cell6"),
                            Tools.getInnerId(mRes, "cell7"),
                            Tools.getInnerId(mRes, "cell8"),
                            Tools.getInnerId(mRes, "cell9")
                    };
                    Drawable dra_contacts_notify_Bg = mRes.getDrawable(Tools
                            .getDrawableId(mRes, "ic_lock_contacts_notify_sprd"));
                    Drawable dra_contacts_nothing_Bg = mRes.getDrawable(Tools
                            .getDrawableId(mRes, "ic_lock_contacts_nothing_sprd"));
                    int i = 0;
                    for (ContactItemInfo contactItemInfo : mContactsData
                            .getSelectedItemsList()) {

                        Log.d(PACKAGE_NAME, contactItemInfo.mCallNumber + ", "
                                + contactItemInfo.mPhotoId + ", "
                                + contactItemInfo.mContactId + ", "
                                + contactItemInfo.mNameStr + ", "
                                + contactItemInfo.mIsMissed + ", "
                                + contactItemInfo.mFastNumber);
                        FrameLayout fmly = (FrameLayout) findViewById(cellids[i]);
                        View itemView = mInflater.inflate(mRes.getLayout(Tools
                                .getLayoutId(mRes, "widget_contact_add_item")),
                                null);
                        itemView.setOnLongClickListener(DragLockscreen.this);
                        ImageView cell_image = (ImageView) (ImageView) itemView
                                .findViewById(Tools.getInnerId(mRes,
                                        "contacts_cell_photo"));
                        if (i == 0) {
                            cell_image
                                    .setImageDrawable(dra_contacts_notify_Bg);
                        } else if (contactItemInfo.mNameStr != null) {
                            ((TextView) itemView.findViewById(Tools.getInnerId(
                                    mRes, "contacts_cell_name")))
                                    .setText(contactItemInfo.mNameStr);
                        } else {
                            cell_image
                                    .setImageDrawable(dra_contacts_nothing_Bg);
                        }

                        fmly.addView(itemView);
                        itemView.setTag(contactItemInfo);
                        i++;
                    }
                    break;

                case MESSAGE_MUSIC_PLAY:
                    try {
                        if (MMUSICSERVICE != null && MMUSICSERVICE.duration() != -1) {
                            mPlayandpause.setBackgroundDrawable(mRes
                                    .getDrawable(Tools.getDrawableId(mRes,
                                            "ic_lock_music_pause_normal_sprd")));
                            mTxtTrackName.setText(MMUSICSERVICE.getTrackName());
                            //SPRD: Bug 297644 lockscreen play music ArtistName is wrong
                            mTxtArtistName.setText(getArtistName(MMUSICSERVICE.getArtistName()));
                            mTxtDuration.setText(Tools.makeTimeString4MillSec(mRes, MMUSICSERVICE
                                    .duration()));
                            Bitmap bm = Tools.getArtwork(mContext,
                                    MMUSICSERVICE.getAudioId(),
                                    MMUSICSERVICE.getAlbumId(), false);
                            if (bm == null) {
                                bm = Tools.getDefaultArtwork(mContext);
                            }
                            Log.d(PACKAGE_NAME, "play bm= " + bm+"  "+DragLockscreen.this);
                            setMusicAlbumPic(bm);
                            updateProgress();
                        } else {
                            mPlayandpause.setBackgroundDrawable(mRes
                                        .getDrawable(Tools.getDrawableId(mRes,
                                                "ic_lock_music_play_normal_sprd")));
                            mTxtTrackName.setText(mRes.getString(Tools.getStringId(mRes,
                                        "music_list_prompt")));
                            mTxtArtistName.setText(mRes.getString(Tools.getStringId(mRes,
                                        "music_operation_prompt")));
                            mTxtPosition.setText("--:--");
                            mTxtDuration.setText("--:--");
                            mProgressBar.setProgress(0);
                            Bitmap bm = Tools.getDefaultArtwork(mContext);
                            Log.d(PACKAGE_NAME, "unplay bm= " + bm);
                            setMusicAlbumPic(bm);
                        }
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    } catch (NotFoundException e) {
                        e.printStackTrace();
                    }

                    break;
                default:
                    break;
            }

        }
    }

    private void setMusicAlbumPic(Bitmap bm) {
        if (bm != null) {
            Log.d(PACKAGE_NAME, "getStatusBarHeight: " + getStatusBarHeight()
                    + " getScreenHeight: " + getScreenHeight());
            mBitmapDrawable = new BitmapDrawable(mRes,
                    Tools.getBackgroundBitmap(bm, getScreenWidth(),
                            (getScreenHeight() - getStatusBarHeight())));
            mSc3.setBackgroundDrawable(mBitmapDrawable);
        } else {
            mSc3.setBackgroundDrawable(null);
        }
        Tools.recycleLastBackgroundBitmap();
    }

    private int getStatusBarHeight() {
        final Resources res = mContext.getResources();
        return res
                .getDimensionPixelSize(com.android.internal.R.dimen.status_bar_height);
    }

    private int getScreenHeight() {
        WindowManager wm = (WindowManager) mContext
                .getSystemService(Context.WINDOW_SERVICE);
        return wm.getDefaultDisplay().getHeight();
    }

    private int getScreenWidth() {
        WindowManager wm = (WindowManager) mContext
                .getSystemService(Context.WINDOW_SERVICE);
        return wm.getDefaultDisplay().getWidth();
    }

    private static IMediaPlaybackService MMUSICSERVICE = null;
    private static PlayServiceConnection SERVICECONNECTION = null;

    private static class PlayServiceConnection implements ServiceConnection {
        boolean try2paly;
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            Log.d(PACKAGE_NAME, "onServiceConnected...");
            MMUSICSERVICE = IMediaPlaybackService.Stub.asInterface(service);
            if (try2paly && MMUSICSERVICE != null) {
                try2paly = false;
                try {
                    MMUSICSERVICE.play();
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            Log.d(PACKAGE_NAME, "onServiceDisconnected...");
            MMUSICSERVICE = null;
            mContext.unbindService(SERVICECONNECTION);
        }

    };

    @Override
    public boolean onLongClick(View v) {
        Log.d(PACKAGE_NAME, "onLongClick");
        try {
            if (v.getTag() instanceof ContactItemInfo) {
                ContactItemInfo itemInfo = (ContactItemInfo) (v.getTag());
                if (!itemInfo.mIsMissed) {
                    toVibrator();
                    String number = itemInfo.mCallNumber;
                    Log.d(PACKAGE_NAME, " to call " + number);
                    Intent intent = new Intent();
                    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                            | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
                    /* Add 20130515 Spreadst of 163555 ,Lock screen start */
                    intent.putExtra(IS_LOCK_SCREEN_DIAL, true);
                    /* Add 20130515 Spreadst of 163555 ,Lock screen end */
                    intent.setAction(Intent.ACTION_CALL_PRIVILEGED);
                    // first position is 'voice mail'
                    if (itemInfo.mFastNumber == 1) {
                        Log.d(PACKAGE_NAME, " voiceMailPhoneId: " + itemInfo.voiceMailPhoneId);
                        intent.setData(Uri.parse("voicemail:" + number));
                        intent.putExtra(TelephonyIntents.EXTRA_PHONE_ID, itemInfo.voiceMailPhoneId);
                    } else {
                        intent.setData(Uri.parse("tel:" + number));
                    }
                    Log.d(PACKAGE_NAME, " startActivity " + number);
                    mContext.startActivity(intent);
                    Log.d(PACKAGE_NAME, " over " + number);
                    /* SPRD: Modify 20131031 Spreadst of Bug 225409 incallscreen show too slow when fast dial @{ */
                    try {
                        ActivityManagerNative.getDefault().dismissKeyguardOnNextActivity();
                    } catch (RemoteException e) {
                        Log.w(TAG, "can't dismiss keyguard");
                    }
                    /* @} */
                }
            }
        } catch (Exception e) {
        }
        return false;
    }

    private void toVibrator() {
        Log.d(PACKAGE_NAME, "isTactileFeedbackEnabled= "+isTactileFeedbackEnabled());
        if (!isTactileFeedbackEnabled()) {
            return;
        }
        Vibrator vibrator = (Vibrator) mContext
                .getSystemService(mContext.VIBRATOR_SERVICE);
        long[] pattern = {
                0, 50
        };
        vibrator.vibrate(pattern, -1);
    }

    @Override
    public void onClick(View v) {
        if (MMUSICSERVICE == null) {
            mContext.startService(new Intent("com.android.music.IMediaPlaybackService"));
            SERVICECONNECTION.try2paly = true;
            mContext.bindService(new Intent(
                                "com.android.music.IMediaPlaybackService"),
                                SERVICECONNECTION, 0);
            return;
        }

        try {
            if (v.getId() == Tools.getInnerId(mRes, "playandpause")) {

                boolean isPlaying = MMUSICSERVICE.isPlaying();
                if (isPlaying) {
                    MMUSICSERVICE.pause();
                    /* Bug 295700 Lock screen mode,when there is no song,can not click play button @{ */
                    // v.setBackgroundDrawable(mRes.getDrawable(Tools
                    //        .getDrawableId(mRes, "ic_lock_music_play_normal_sprd")));
                    /* @} */
                } else {
                    mProcesssShowHandle.removeMessages(MESSAGE_MUSIC_PLAY);
                    MMUSICSERVICE.play();
                    /* Bug 295700 Lock screen mode,when there is no song,can not click play button @{ */
                    // v.setBackgroundDrawable(mRes.getDrawable(Tools
                    //        .getDrawableId(mRes,
                    //                "ic_lock_music_pause_normal_sprd")));
                    /* @} */
				}

			}

			if (v.getId() == Tools.getInnerId(mRes, "back")) {
				Log.d(PACKAGE_NAME, "ic_lock_music_previous_normal");
                mProcesssShowHandle.removeMessages(MESSAGE_MUSIC_PLAY);
				MMUSICSERVICE.prev();
			}

			if (v.getId() == Tools.getInnerId(mRes, "forword")) {
				Log.d(PACKAGE_NAME, "ic_lock_music_next_normal");
                mProcesssShowHandle.removeMessages(MESSAGE_MUSIC_PLAY);
				MMUSICSERVICE.next();
			}

            /* SPRD: Modify 20131009 Spreadst of Bug 222504 add emergencycall @{ */
            if (v.getId() == Tools.getInnerId(mRes, "emergencycallbutton")) {
                if (DEBUG) Log.d(PACKAGE_NAME, "takeEmergencyCallAction()");
                mLockScreenListener.takeEmergencyCallAction();
            }
            /* @} */

		} catch (RemoteException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}


    }

    private void updateProgress() {
        if (MMUSICSERVICE == null) {
            return;
        }
        if (mProRunnable == null) {
            mProRunnable = new Runnable() {
                public void run() {
                    long position = 0;
                    long duration = 0;
                    try {
                        position = MMUSICSERVICE.position();
                        duration = MMUSICSERVICE.duration();
                        if (duration > 0) {
                            mTxtPosition.setText(Tools.makeTimeString4MillSec(mRes, position));
                            mProgressBar
                                        .setProgress((int) (position * 1000 / duration));
                        }
                        if (!MMUSICSERVICE.isPlaying()) {
                            mPlayandpause.setBackgroundDrawable(mRes
                                        .getDrawable(Tools.getDrawableId(mRes,
                                                "ic_lock_music_play_normal_sprd")));
                        }
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    }
                }
            };
        }
        mProcesssShowHandle.post(mProRunnable);
    }

    private Runnable mArrowRoundRunnable;

    private void updateMessageArrowRound() {

        if (mArrowRoundRunnable == null) {

            mArrowRoundRunnable = new Runnable() {

                @Override
                public void run() {
                    if (!is_arrow_round) {
                        return;
                    }
                    if (mDra_arrows != null) {
                        if (arrow_index > mDra_arrows.length - 1) {
                            arrow_index = 0;
                        }
                        if (mImgv_arrow != null) {
                            mImgv_arrow
                                    .setBackgroundDrawable(mDra_arrows[arrow_index]);
                        }
                        arrow_index++;
                        mProcesssShowHandle.postDelayed(mArrowRoundRunnable,
                                500);
                    }
                }
            };
            mArrowRoundRunnable.run();
        }
    }

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
        setCarrierText(carrierText[phoneId],phoneId);
        if (DEBUG) Log.d(PACKAGE_NAME, "updateCarrierTextWithSimStatus()--" +
                  ", carrierText["+phoneId+"]="+carrierText[phoneId]+", mStatus="+mStatus+", phoneId="+phoneId);
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
        /* SPRD: Modify 20130821 Spreadst of 204612 draglockscreen carrier show wrong @{ */
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
    private void initEmergencyCallButton(){
        mEmergencyCallButton = (Button) findViewById(Tools.getInnerId(
                mRes, "emergencycallbutton"));
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
    }

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
            mLockPatternUtils.updateEmergencyCallButtonState(mEmergencyCallButton, phoneState, true,false);
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
    /* @} */

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

    private AudioManager mAudioManager;
    private RemoteController mRemoteController;
    private RemoteController.OnClientUpdateListener mRCClientUpdateListener =
            new RemoteController.OnClientUpdateListener() {

                @Override
                public void onClientChange(boolean clearing) {
                    // TODO Auto-generated method stub
                    Log.d(PACKAGE_NAME, "onClientChange: "+clearing);
                }

                @Override
                public void onClientPlaybackStateUpdate(int state) {
                    // TODO Auto-generated method stub
                    Log.d(PACKAGE_NAME, "onClientPlaybackStateUpdate: "+ state);
                    refreshPlayStatus();
                }

                @Override
                public void onClientPlaybackStateUpdate(int state, long stateChangeTimeMs,
                        long currentPosMs, float speed) {
                    // TODO Auto-generated method stub
                    Log.d(PACKAGE_NAME, "onClientPlaybackStateUpdate"+" "+currentPosMs+" "+DragLockscreen.this);
                    updateProgress();
                }

                @Override
                public void onClientTransportControlUpdate(int transportControlFlags) {
                    // TODO Auto-generated method stub
                }

                @Override
                public void onClientMetadataUpdate(MetadataEditor metadataEditor) {
                    // TODO Auto-generated method stub
                    Log.d(PACKAGE_NAME, "onClientMetadataUpdate");
                    refreshPlayStatus();
                }

            };

    private boolean mStopPlay;
    private void refreshPlayStatus() {
        Log.d(PACKAGE_NAME, "mStopPlay= " + mStopPlay + "  " + this);
        if (MMUSICSERVICE == null || mStopPlay) {
            return;
        }
        try {
            boolean isPlaying = MMUSICSERVICE.isPlaying();
            if (isPlaying) {
                /* Bug 295700 Lock screen mode,when there is no song,can not click play button @{ */
                mPlayandpause.setBackgroundDrawable(mRes.getDrawable(Tools
                        .getDrawableId(mRes, "ic_lock_music_pause_normal_sprd")));
                /* @} */
                mProcesssShowHandle.removeMessages(MESSAGE_MUSIC_PLAY);
                Log.d(PACKAGE_NAME, "" + MMUSICSERVICE.duration());
                Log.d(PACKAGE_NAME, "" + MMUSICSERVICE.position());
                Log.d(PACKAGE_NAME, "" + MMUSICSERVICE.getTrackName());
                Log.d(PACKAGE_NAME, "" + MMUSICSERVICE.getArtistName());
                mProcesssShowHandle.sendEmptyMessage(MESSAGE_MUSIC_PLAY);
            /* Bug 295700 Lock screen mode,when there is no song,can not click play button @{ */
            } else {
                mPlayandpause.setBackgroundDrawable(mRes.getDrawable(Tools
              .getDrawableId(mRes, "ic_lock_music_play_normal_sprd")));
           /* @} */
            }
        } catch (RemoteException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    /* Bug 297644 lockscreen play music ArtistName is wrong @{ */
    private String getArtistName(String name) {
        if (name != null && MediaStore.UNKNOWN_STRING.equals(name)) {
            return mRes.getString(Tools.getStringId(mRes, "unknown_artist_name"));
        }
        return name;
    }
    /* @} */

    public boolean isTactileFeedbackEnabled() {
        return Settings.System.getIntForUser(mContext.getContentResolver(),
                        Settings.System.HAPTIC_FEEDBACK_ENABLED, 1, UserHandle.USER_CURRENT) != 0;

    }
}
