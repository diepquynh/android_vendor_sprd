/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server.policy;

import static android.content.res.Configuration.EMPTY;
import static android.content.res.Configuration.UI_MODE_TYPE_CAR;
import static android.content.res.Configuration.UI_MODE_TYPE_MASK;
import static android.view.WindowManager.LayoutParams.*;

import android.app.ActivityManager;
import android.app.ActivityManagerNativeEx;
import android.content.ActivityNotFoundException;
import android.content.pm.PackageManager;
import android.app.ActivityManager.RunningTaskInfo;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.res.CompatibilityInfo;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.database.ContentObserver;
import android.media.AudioManager;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.PowerManager.WakeLock;
import android.os.PowerManager;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.provider.Settings;
import android.util.Log;
import android.util.Slog;
import android.view.IWindowManager;
import android.view.KeyEvent;
import android.view.WindowManager;
import android.view.HapticFeedbackConstants;
import android.view.ViewGroup;
import android.view.Window;
import android.view.MotionEvent;
import android.view.View;
import android.provider.MediaStore;
import android.os.UserHandle;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.widget.ImageView;

import com.android.internal.R;
import com.android.internal.policy.PhoneWindow;
import com.android.server.statusbar.StatusBarManagerInternal;

import com.android.server.DebugController;

import java.io.PrintWriter;
import java.util.List;
import java.util.ArrayList;
import java.io.FileInputStream;
import java.util.HashMap;
import java.io.IOException;

public class SprdPhoneWindowManager extends PhoneWindowManager {
  //SPRD: Camera key long pressed, launch camer.
  static public final String SYSTEM_DIALOG_REASON_CAMERA_KEY = "camerakey";
  // SPRD: if home is physical,Phykey = true.

  /*SPRD:add for Bug511212 @{*/
  private boolean mOpenCameraGestureEnabledSetting;
  MyTapEventListener mTapEventListener;
  /*@}*/
    //modify for Bug#618149  begin
    private BitmapFactory.Options mBitmapOptions = new BitmapFactory.Options();
    private Drawable getStartingWindowBackgroundDrawable(IBinder appToken, Intent intent) {
        ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        Bitmap thumbnail = null;
        ActivityManager.TaskThumbnail thumb = null;
        if (intent != null) {
            try {
                thumb = ActivityManagerNativeEx.getDefault()
                        .getRecentTaskThumbnail(intent);
                if (thumb != null) {
                    thumbnail = thumb.mainThumbnail;
                    if( thumbnail != null && thumbnail.isRecycled() ) {
                        Log.d(TAG, "getTaskThumbnail recycled... parcel = " + thumb.thumbnailFileDescriptor);
                        thumbnail = null;
                    }
                    if (thumbnail == null && thumb.thumbnailFileDescriptor != null) {
                        thumbnail = BitmapFactory.decodeFileDescriptor(thumb.thumbnailFileDescriptor.getFileDescriptor(), null, mBitmapOptions);
                        Log.d(TAG, "getTaskThumbnail null or recyled,decode from file...->" + thumbnail);
                    }
                }
                if (thumbnail != null) {
                    Log.d(TAG, "getTaskThumbnail thumbnail----->" + thumbnail);
                    return new BitmapDrawable(thumbnail);
                }
            } catch (Exception e) {
                e.printStackTrace();
            } finally {
                if (thumb != null && thumb.thumbnailFileDescriptor != null) {
                    try {
                        thumb.thumbnailFileDescriptor.close();
                    } catch (IOException ioerror) {
                        ioerror.printStackTrace();
                    }
                }
            }
        }
        return null;
    }
    /*
    *@hide
    */
    boolean isRemoveStarting = false;
    View startView = null;

    class StartingView extends ImageView {

        private IBinder token;

        public StartingView(Context context,IBinder binder) {
            super(context);
            token = binder;
        }

        public boolean dispatchTouchEvent(MotionEvent event) {
            if(isRemoveStarting && startView != null) {
                isRemoveStarting = false;
                try {
                    removeStartingWindow(token,startView);//wms interface?
                }catch(Exception e) {}
            }
            //remove all screenshot-task in queue
            Slog.w(TAG,"starting window on touch -->"+event);
            try {
                ActivityManagerNativeEx.getDefault().removePendingUpdateThumbTask();
            } catch (Exception e) {}
            return false;
        }
        public boolean dispatchKeyEvent(KeyEvent event) {
            Slog.w(TAG,"starting window on key -->"+event);
            try {
                ActivityManagerNativeEx.getDefault().removePendingUpdateThumbTask();
            } catch (Exception e) {}
            return false;
        }
    }

    public void startWindowRemoving(View startingView) {
        isRemoveStarting = true;
        startView = startingView;
    }
    public View addStartingWindow(Intent intent, boolean fullscreen, IBinder appToken, String packageName, int theme,
            CompatibilityInfo compatInfo, CharSequence nonLocalizedLabel, int labelRes,
            int icon, int logo, int windowFlags, Configuration overrideConfig) {
        if (!SHOW_STARTING_ANIMATIONS) {
            return null;
        }
        if (packageName == null) {
            return null;
        }

        WindowManager wm = null;
        View view = null;

        try {
            Context context = mContext;
            if (DEBUG_STARTING_WINDOW) Slog.d(TAG, "addStartingWindow " + packageName
                    + ": nonLocalizedLabel=" + nonLocalizedLabel + " theme="
                    + Integer.toHexString(theme));
            if (theme != context.getThemeResId() || labelRes != 0) {
                try {
                    context = context.createPackageContext(packageName, 0);
                    context.setTheme(theme);
                } catch (PackageManager.NameNotFoundException e) {
                    // Ignore
                }
            }

            if (overrideConfig != null && overrideConfig != EMPTY) {
                if (DEBUG_STARTING_WINDOW) Slog.d(TAG, "addStartingWindow: creating context based"
                        + " on overrideConfig" + overrideConfig + " for starting window");
                final Context overrideContext = context.createConfigurationContext(overrideConfig);
                overrideContext.setTheme(theme);
                final TypedArray typedArray = overrideContext.obtainStyledAttributes(
                        com.android.internal.R.styleable.Window);
                final int resId = typedArray.getResourceId(R.styleable.Window_windowBackground, 0);
                if (resId != 0 && overrideContext.getDrawable(resId) != null) {
                    // We want to use the windowBackground for the override context if it is
                    // available, otherwise we use the default one to make sure a themed starting
                    // window is displayed for the app.
                    if (DEBUG_STARTING_WINDOW) Slog.d(TAG, "addStartingWindow: apply overrideConfig"
                            + overrideConfig + " to starting window resId=" + resId);
                    context = overrideContext;
                }
            }

            final PhoneWindow win = new PhoneWindow(context);
            win.setIsStartingWindow(true);
            Log.d(TAG, "getTaskThumbnail ----->addStartingWindow fullscreen = " + fullscreen);
            if(fullscreen){
                Drawable background = getStartingWindowBackgroundDrawable(appToken, intent);
                if (background != null) {
                    Log.d(TAG, "add def-window begin");
                    int height = 0;
                    int resId = context.getResources().getIdentifier("status_bar_height", "dimen", "android");
                    if (resId > 0) {
                        height = context.getResources().getDimensionPixelSize(resId);
                    }
                    win.requestFeature(Window.FEATURE_NO_TITLE);
                    StartingView bg = new StartingView(context,appToken);
                    bg.setScaleType(ImageView.ScaleType.FIT_XY);
                    bg.setImageDrawable(background);
                    bg.setPadding(0, 0, 0, 0);
                    win.addContentView(bg, new WindowManager.LayoutParams(WindowManager.LayoutParams.WRAP_CONTENT, WindowManager.LayoutParams.WRAP_CONTENT));
                    Log.d(TAG, "add def-window end");
                }
            }

            CharSequence label = context.getResources().getText(labelRes, null);
            // Only change the accessibility title if the label is localized
            if (label != null) {
                win.setTitle(label, true);
            } else {
                win.setTitle(nonLocalizedLabel, false);
            }

            win.setType(
                WindowManager.LayoutParams.TYPE_APPLICATION_STARTING);

            synchronized (mWindowManagerFuncs.getWindowManagerLock()) {
                // Assumes it's safe to show starting windows of launched apps while
                // the keyguard is being hidden. This is okay because starting windows never show
                // secret information.
                if (mKeyguardHidden) {
                    windowFlags |= FLAG_SHOW_WHEN_LOCKED;
                }
            }

            // Force the window flags: this is a fake window, so it is not really
            // touchable or focusable by the user.  We also add in the ALT_FOCUSABLE_IM
            // flag because we do know that the next window will take input
            // focus, so we want to get the IME window up on top of us right away.
            win.setFlags(
                windowFlags|
                WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE|
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE|
                WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM,
                windowFlags|
                WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE|
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE|
                WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);

            win.setDefaultIcon(icon);
            win.setDefaultLogo(logo);
            win.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            win.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION);
            win.setLayout(WindowManager.LayoutParams.MATCH_PARENT,
                    WindowManager.LayoutParams.MATCH_PARENT);

            final WindowManager.LayoutParams params = win.getAttributes();
            params.token = appToken;
            params.packageName = packageName;
            params.windowAnimations = win.getWindowStyle().getResourceId(
                    com.android.internal.R.styleable.Window_windowAnimationStyle, 0);
            params.privateFlags |=
                    WindowManager.LayoutParams.PRIVATE_FLAG_FAKE_HARDWARE_ACCELERATED;
            params.privateFlags |= WindowManager.LayoutParams.PRIVATE_FLAG_SHOW_FOR_ALL_USERS;

            if (!compatInfo.supportsScreen()) {
                params.privateFlags |= WindowManager.LayoutParams.PRIVATE_FLAG_COMPATIBLE_WINDOW;
            }

            params.setTitle("Starting " + packageName);

            wm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
            view = win.getDecorView();

            if (DEBUG_STARTING_WINDOW) Slog.d(TAG, "Adding starting window for "
                + packageName + " / " + appToken + ": " + (view.getParent() != null ? view : null));

            wm.addView(view, params);

            // Only return the view if it was successfully added to the
            // window manager... which we can tell by it having a parent.
            return view.getParent() != null ? view : null;
        } catch (WindowManager.BadTokenException e) {
            // ignore
            Log.w(TAG, appToken + " already running, starting window not displayed. " +
                    e.getMessage());
        } catch (RuntimeException e) {
            // don't crash if something else bad happens, for example a
            // failure loading resources because we are loading from an app
            // on external storage that has been unmounted.
            Log.w(TAG, appToken + " failed creating starting window", e);
        } finally {
            if (view != null && view.getParent() == null) {
                Log.w(TAG, "view not successfully added to wm, removing view");
                wm.removeViewImmediate(view);
            }
        }

        return null;
    }
    //modify for Bug#618149  end
    @Override
    public long interceptKeyBeforeDispatching(WindowState win, KeyEvent event, int policyFlags) {
        final boolean keyguardOn = keyguardOn();
        final int keyCode = event.getKeyCode();
        final int flags = event.getFlags();
        final boolean down = event.getAction() == KeyEvent.ACTION_DOWN;
        /* SPRD: add pocket mode acquirement @ { */
        if (isPocketModeEnabled() && (flags & KeyEvent.FLAG_FALLBACK) == 0) {
            if (keyCode == KeyEvent.KEYCODE_VOLUME_UP
                    && isDisableTouchModeVolumeUpKeyConsumed()) {
                if (!down) {
                    setDisableTouchModeVolumeUpKeyConsumed(false);
                }
                return -1;
            }
            if ((keyCode == KeyEvent.KEYCODE_VOLUME_UP || keyCode == KeyEvent.KEYCODE_VOLUME_DOWN)
                    && !isDisableTouchModeVolumeUpKeyConsumed()) {
                if (down){
                    if (mDisableTouchModePanel != null && mDisableTouchModePanel.isShowing()) {
                        if (DEBUG_INPUT) Slog.d(TAG, "DisableTouchPanel showBackground, keyCode = " + keyCode);
                        mDisableTouchModePanel.showBackground();
                        return -1;
                    }
                }
            }
        }
        /* @ } */
        /* SPRD: Determined to factory mode,shielded HOME &  APP_SWITCH key. @{ */
        if (keyCode == KeyEvent.KEYCODE_HOME || keyCode == KeyEvent.KEYCODE_APP_SWITCH) {
            WindowManager.LayoutParams attr = win != null ? win.getAttrs() : null;
            if (attr != null
                    && (null != attr.packageName)
                    && ((attr.packageName.startsWith("com.sprd.validationtools")) || (attr.packageName
                            .startsWith("com.sprd.factorymode")))) {
                return 0;
            }
        }else if(keyCode == KeyEvent.KEYCODE_CAMERA){
           if ((event.getFlags() & KeyEvent.FLAG_LONG_PRESS) != 0 && !keyguardOn) {
                ActivityManager mActivityManager = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
                List<RunningTaskInfo> list = mActivityManager.getRunningTasks(1);
                if (!list.isEmpty() && list.get(0) != null && list.get(0).topActivity != null) {
                   String className = list.get(0).topActivity.getClassName();
               if (className.equals("com.android.camera.CameraActivity") ||
                       className.equals("com.android.camera.CameraLauncher"))
                  return 0;
               }
               performHapticFeedbackLw(null, HapticFeedbackConstants.LONG_PRESS, false);
               sendCloseSystemWindows(SYSTEM_DIALOG_REASON_CAMERA_KEY);
               //launch camera
               Intent intent = new Intent(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA);
               intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP
                             | Intent.FLAG_ACTIVITY_CLEAR_TOP);
               try {
                   mContext.startActivityAsUser(intent, UserHandle.CURRENT_OR_SELF);
               } catch (ActivityNotFoundException e) {
                   Slog.w(TAG, "No activity to handle camera action.", e);
               }
          }
        }
        /* @} */
        return super.interceptKeyBeforeDispatching(win, event, policyFlags);
    }

    /* SPRD: double-tap volume-down/camera key to quick capture @{ */
    private long mLastVolumeKeyDownTime = 0L;
    private long mLastCameraKeyDownTime = 0L;
     /**
     * Time in milliseconds in which the volume-down/camera button must be pressed twice so it will be considered
     * as a camera launch.
     */
    private static final long CAMERA_DOUBLE_TAP_MAX_TIME_MS = 500L;
    private static final long CAMERA_DOUBLE_TAP_MIN_TIME_MS = 50L;

    final Object mQuickCameraLock = new Object();
    ServiceConnection mQuickCameraConnection = null;
    WakeLock mWakeLock = null;

    final Runnable mQuickCameraTimeout = new Runnable() {
        @Override
        public void run() {
            synchronized (mQuickCameraLock) {
                if (mQuickCameraConnection != null) {
                    if (DEBUG_INPUT) Log.v("QuickCamera", "unbind");
                    if (mWakeLock != null && mWakeLock.isHeld()) {
                        mWakeLock.release();
                        mWakeLock = null;
                    }
                    mContext.unbindService(mQuickCameraConnection);
                    mQuickCameraConnection = null;
                }
            }
        };
    };

    private void launchQuickCamera() {
        synchronized (mQuickCameraLock) {
            if (mQuickCameraConnection != null) {
                return;
            }
        }
        ComponentName cn = new ComponentName("com.sprd.quickcamera",
                "com.sprd.quickcamera.QuickCameraService");
        Intent intent = new Intent();
        intent.setComponent(cn);
        ServiceConnection conn = new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                synchronized (mQuickCameraLock) {
                    if (DEBUG_INPUT) Log.d(TAG, "launchQuickCamera" + "onServiceConnected" );
                    Messenger messenger = new Messenger(service);
                    Message msg = Message.obtain(null, 1);
                    final ServiceConnection myConn = this;
                    Handler h = new Handler(mHandler.getLooper()) {
                        @Override
                        public void handleMessage(Message msg) {
                            synchronized (mQuickCameraLock) {
                                if (DEBUG_INPUT) Log.d(TAG, "launchQuickCamera" + "receive reply");
                                if (mQuickCameraConnection == myConn) {
                                    if (DEBUG_INPUT) Log.d(TAG, "launchQuickCamera" + "receive reply unbind");
                                    if (mWakeLock != null && mWakeLock.isHeld()) {
                                        mWakeLock.release();
                                        mWakeLock = null;
                                    }
                                    mContext.unbindService(mQuickCameraConnection);
                                    mQuickCameraConnection = null;
                                    mHandler.removeCallbacks(mQuickCameraTimeout);
                                }
                            }
                        }
                    };
                    msg.replyTo = new Messenger(h);
                    try {
                        messenger.send(msg);
                    } catch (RemoteException e) {}
                }
            }

            @Override
            public void onServiceDisconnected(ComponentName name) {}
        };
        if (DEBUG_INPUT) Log.d(TAG, "launchQuickCamera bind start");
        if (mContext.bindServiceAsUser(
                intent, conn, Context.BIND_AUTO_CREATE, UserHandle.CURRENT)) {
            if (DEBUG_INPUT) Log.d(TAG, "launchQuickCamera bind successful");
            PowerManager pm = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
            mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,"QuickCamera");
            mWakeLock.acquire(6000); // more than 5000
            mQuickCameraConnection = conn;
            mHandler.postDelayed(mQuickCameraTimeout, 5000);
        }
        if (DEBUG_INPUT) Log.d(TAG, "launchQuickCamera bind end");
    };

    public void handleQuickCamera(KeyEvent event, boolean down) {
        if (!SystemProperties.getBoolean("persist.sys.cam.quick", true)) {
            return;
        }

        boolean launched = false;
        long doubleTapInterval = 0L;
        if (!SystemProperties.getBoolean("persist.sys.cam.hascamkey", false)) {
            AudioManager audioManager = (AudioManager) mContext
                    .getSystemService(Context.AUDIO_SERVICE);
            if (!audioManager.isMusicActive()
                    && !isScreenOn()
                    && event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_DOWN
                    && down) {
                doubleTapInterval = event.getEventTime() - mLastVolumeKeyDownTime;
                if (doubleTapInterval < CAMERA_DOUBLE_TAP_MAX_TIME_MS
                        && doubleTapInterval > CAMERA_DOUBLE_TAP_MIN_TIME_MS) {
                    launched = true;
                }
                mLastVolumeKeyDownTime = event.getEventTime();
            }
        } else {
            if (!isScreenOn()
                    && event.getKeyCode() == KeyEvent.KEYCODE_CAMERA
                    && down) {
                doubleTapInterval = event.getEventTime() - mLastCameraKeyDownTime;
                if (doubleTapInterval < CAMERA_DOUBLE_TAP_MAX_TIME_MS
                        && doubleTapInterval > CAMERA_DOUBLE_TAP_MIN_TIME_MS) {
                    launched = true;
                }
                mLastCameraKeyDownTime = event.getEventTime();
            }
        }

        if (launched) {
            if (DEBUG_INPUT) Log.d(TAG, "launchQuickCamera, " + "launched = " + launched
                    + ", doubleTapInterval = " + doubleTapInterval);
            launchQuickCamera();
        }
    }
    /* @} */


    /* SPRD: add for dynamic navigationbar @{ */
    void initNavStatus() {
        mDynamicNavigationBar = (Settings.System.getInt(mContext.getContentResolver(), NAVIGATIONBAR_CONFIG, 0) & 0x10) != 0;
    }

    void showNavigationBar(boolean show){
        if (mHasNavigationBar == show) {
            return;
        }
        if (!isKeyguardShowingAndNotOccluded()) {
            mNavigationBarController.setBarShowingLw(show);
            if (show) {
                mNavigationBarController.setLayoutNeeded(true);
            }
            mHasNavigationBar = show;
        }
    }

    @Override
    public boolean isNavigationBarShowing() {
        return mHasNavigationBar;
    }

    void registerNavIfNeeded() {
        if (mHasNavigationBar) {
            mContext.getContentResolver().registerContentObserver(
                    Settings.System.getUriFor(NAVIGATIONBAR_CONFIG),
                    true,
                    mHideNavObserver,
                    UserHandle.USER_ALL);
        }
    }

    private ContentObserver mHideNavObserver = new ContentObserver(mHandler) {
        public void onChange(boolean selfChange) {
            mDynamicNavigationBar = (Settings.System.getInt(mContext.getContentResolver(), NAVIGATIONBAR_CONFIG, 0) & 0x10) != 0;
            if (!mDynamicNavigationBar) {
                showNavigationBar(true);
            }
        }
    };
    /* @} */


    /* SPRD: pocket mode requirement @{ */
    @Override
    public void init(Context context, IWindowManager windowManager,
            WindowManagerFuncs windowManagerFuncs) {
        super.init(context, windowManager, windowManagerFuncs);
        mPolicyHandlerEx = new PolicyHandlerEx();
        registerSprdSensors();
    }

    private static final int MSG_INIT_TOUCH_PANEL = 100;
    private static final int MSG_DISABLE_TOUCH_PANEL = 101;
    PolicyHandlerEx mPolicyHandlerEx;

    boolean mPocketModeEnabledSetting;

    static final long DISABLE_TOUCH_CHORD_DEBOUNCE_DELAY_MILLIS = 150;
    private boolean mDisableTouchModeVolumeUpKeyConsumed;
    private long mDisableTouchModeVolumeUpKeyTime;

    DisableTouchModePanel mDisableTouchModePanel;

    class MyPocketEventListener extends PocketEventListener {
        private final Runnable mStartPocketRunnable = new Runnable() {
            @Override
            public void run() {
                synchronized (mLock) {
                    if (DEBUG_INPUT) Slog.d(TAG, "onStartPocketMode");
                    if (mDisableTouchModePanel != null) {
                        mDisableTouchModePanel.setPocketMode(true);
                    }
                }
            }
        };

        private final Runnable mStopPocketRunnable = new Runnable() {
            @Override
            public void run() {
                synchronized (mLock) {
                    if (DEBUG_INPUT) Slog.d(TAG, "onStopPocketMode");
                    if (mDisableTouchModePanel != null) {
                        mDisableTouchModePanel.setPocketMode(false);
                    }
                }
            }
        };

        MyPocketEventListener(Context context, Handler handler) {
            super(context, handler);
        }

        @Override
        public void onStartPocketMode() {
            mPolicyHandlerEx.post(mStartPocketRunnable);
        }

        @Override
        public void onStopPocketMode() {
            mPolicyHandlerEx.post(mStopPocketRunnable);
        }
    }
    MyPocketEventListener mPocketEventListener;

    protected void updatePocketEventListenerLp() {
        if (shouldEnablePocketEventGestureLp()) {
            if (DEBUG_INPUT) Slog.d(TAG, "regist pocketEvent sensor");
            mPolicyHandlerEx.sendEmptyMessage(MSG_INIT_TOUCH_PANEL);
            mPocketEventListener.enable();
        } else {
            if (DEBUG_INPUT) Slog.d(TAG, "unregist pocketEvent sensor");
            mPolicyHandlerEx.sendEmptyMessage(MSG_DISABLE_TOUCH_PANEL);
            mPocketEventListener.disable();
        }
    }

    protected boolean shouldEnablePocketEventGestureLp() {
        if (DEBUG_INPUT) Slog.d(TAG, "mPocketModeEnabledSetting = " + mPocketModeEnabledSetting + ",isSupported = " + mPocketEventListener.isSupported());
        return mPocketModeEnabledSetting && mPocketEventListener.isSupported();
    }

    protected void initTouchPanel() {
        if (mDisableTouchModePanel == null) {
            mDisableTouchModePanel = new DisableTouchModePanel(mContext);
            mDisableTouchModePanel.init();
        }
    }

    protected void disableTouchPanel() {
        if (mDisableTouchModePanel != null) {
            mDisableTouchModePanel.setPocketMode(false);
        }
    }

    protected void hideDisableTouchModePanel() {
        if (mDisableTouchModePanel != null && mDisableTouchModePanel.isShowing()) {
            mDisableTouchModePanel.show(false);
        }
    }

    protected void showDisableTouchModePanel() {
        if (mDisableTouchModePanel != null && mDisableTouchModePanel.getPocketMode()) {
            mDisableTouchModePanel.show(true);
        }
    }

    protected void registerSprdSensors() {
        mPocketEventListener = new MyPocketEventListener(mContext, mPolicyHandlerEx);
        /*SPRD:add for Bug511212 @{*/
        mTapEventListener = new MyTapEventListener(mContext, mPolicyHandlerEx);
        /*@}*/
    }

    protected void updateSprdSettings(ContentResolver resolver) {
        if (mSystemReady) {
            boolean pocketModeEnabledSetting = Settings.Global.getInt(resolver,
                     Settings.Global.TOUCH_DISABLE, 0) != 0;
            if (mPocketModeEnabledSetting != pocketModeEnabledSetting) {
                mPocketModeEnabledSetting = pocketModeEnabledSetting;
                updatePocketEventListenerLp();
            }
            /*SPRD:add for Bug511212 @{*/
            boolean openCameraGestureEnabledSetting = Settings.Global.getInt(resolver,
                    Settings.Global.EASY_START, 0) != 0;
            if (mOpenCameraGestureEnabledSetting != openCameraGestureEnabledSetting) {
                mOpenCameraGestureEnabledSetting = openCameraGestureEnabledSetting;
                updateTapEventListenerLp();
            }
            /*@}*/
        }

    }

    public void updateSettings() {
        ContentResolver resolver = mContext.getContentResolver();
        updateSprdSettings(resolver);
        super.updateSettings();
    }

    protected boolean isPocketModeEnabled() {
        return mPocketModeEnabledSetting;
    }

    protected boolean isDisableTouchModeVolumeUpKeyConsumed() {
        return mDisableTouchModeVolumeUpKeyConsumed;
    }

    protected void setDisableTouchModeVolumeUpKeyConsumed(boolean consumed) {
        mDisableTouchModeVolumeUpKeyConsumed = consumed;
    }

    protected void sprdObserve(ContentResolver resolver, ContentObserver observer) {
        resolver.registerContentObserver(Settings.Global.getUriFor(
                Settings.Global.TOUCH_DISABLE), false, observer,
                UserHandle.USER_ALL);
        /*SPRD:add for Bug511212 @{*/
        resolver.registerContentObserver(Settings.Global.getUriFor(
                Settings.Global.EASY_START), false, observer,
                UserHandle.USER_ALL);
        /*@}*/
    }

    protected void interceptDisableTouchModeChord() {
        if (isPocketModeEnabled()
                && mScreenshotChordVolumeUpKeyTriggered && mScreenshotChordPowerKeyTriggered
                && !mScreenshotChordVolumeDownKeyTriggered) {
            final long now = SystemClock.uptimeMillis();
            if (now <= mDisableTouchModeVolumeUpKeyTime + DISABLE_TOUCH_CHORD_DEBOUNCE_DELAY_MILLIS
                    || now <= mScreenshotChordPowerKeyTime
                            + DISABLE_TOUCH_CHORD_DEBOUNCE_DELAY_MILLIS) {
                mDisableTouchModeVolumeUpKeyConsumed = true;
                cancelPendingPowerKeyAction();
                disableTouchPanel();
            }
        }
    }

    protected void interceptUpKeyEx(KeyEvent event) {
        mDisableTouchModeVolumeUpKeyConsumed = false;
        mDisableTouchModeVolumeUpKeyTime = event.getDownTime();
        interceptDisableTouchModeChord();
    }

    protected class PolicyHandlerEx extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_INIT_TOUCH_PANEL:
                    initTouchPanel();
                    break;
                case MSG_DISABLE_TOUCH_PANEL:
                    disableTouchPanel();
                    break;
             }
         }
     }

    @Override
    public void screenTurnedOff() {
        hideDisableTouchModePanel();
        super.screenTurnedOff();
    }

    @Override
    public void screenTurningOn(final ScreenOnListener screenOnListener) {
        showDisableTouchModePanel();
        super.screenTurningOn(screenOnListener);
    }
    /* @} */

    void toggleSplitScreen() {
        StatusBarManagerInternal statusbar = getStatusBarManagerInternal();
        if (statusbar != null) {
            statusbar.toggleSplitScreenIfPossible();
        }
    }

    void cancelPendingAppSwitchKeyAction() {
        if (!mAppSwitchKeyHandled) {
            mAppSwitchKeyHandled = true;
            mHandler.removeMessages(MSG_APP_SWITCH_LONG_PRESS);
        }
    }

    void appswitchLongPress() {
        if (ActivityManager.supportsMultiWindow()) {
            mAppSwitchKeyHandled = true;
            toggleSplitScreen();
        }
    }

    /*SPRD:add for Bug511212 @{*/
    class MyTapEventListener extends TapEventListener {
        private final Runnable mUpdateTapRunnable = new Runnable() {
            @Override
            public void run() {
                // open Camera
                synchronized (mLock) {
                    if (shouldEnableTapEventGestureLp()) {
                        ActivityManager mActivityManager = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
                        List<ActivityManager.RunningTaskInfo> list = mActivityManager.getRunningTasks(1);
                        if (!list.isEmpty() && list.get(0) != null
                                && list.get(0).topActivity != null) {
                            String className = list.get(0).topActivity.getClassName();
                            if (className.equals("com.android.camera.CameraActivity")
                                    || className.equals("com.android.camera.CameraLauncher")
                                    || className.equals("com.android.camera.SecureCameraActivity")) {
                                if (DEBUG_INPUT) Slog.d(TAG, "current top activity className = " + className.toString());
                                return;
                            }
                        }
                        // launch camera
                        Intent intent = new Intent(
                                MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA);
                        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                                | Intent.FLAG_ACTIVITY_SINGLE_TOP
                                | Intent.FLAG_ACTIVITY_CLEAR_TOP);
                        final boolean keyguardActive = mKeyguardDelegate == null ? false :
                                mKeyguardDelegate.isShowing();
                        if (keyguardActive) {
                            intent.setAction(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA_SECURE);
                        } else {
                            intent.setAction(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA);
                        }

                        try {
                            if (DEBUG_INPUT) Slog.d(TAG, "tap sensor launch camera");
                            mContext.startActivityAsUser(intent, UserHandle.CURRENT_OR_SELF);
                        } catch (Exception e) {
                            Slog.w(TAG, "No activity to handle camera action.", e);
                        }
                    }
                }
            }
        };

        MyTapEventListener(Context context, Handler handler) {
            super(context, handler);
        }

        @Override
        public void onTapChanged() {
            mPolicyHandlerEx.post(mUpdateTapRunnable);
        }
    }

    private void updateTapEventListenerLp() {
        if (shouldEnableTapEventGestureLp()) {
            if (DEBUG_INPUT) Slog.d(TAG, "regist tapEvent sensor");
            mTapEventListener.enable();
        } else {
            if (DEBUG_INPUT) Slog.d(TAG, "unregist tapEvent sensor");
            mTapEventListener.disable();
        }
    }

    private boolean shouldEnableTapEventGestureLp() {
        if (DEBUG_INPUT) Slog.d(TAG, "mOpenCameraGestureEnabledSetting = " + mOpenCameraGestureEnabledSetting + ",isSupported = " + mTapEventListener.isSupported());
        return mOpenCameraGestureEnabledSetting && mTapEventListener.isSupported();
    }
    /*@}*/

    @Override
    public void dump(String prefix, PrintWriter pw, String[] args) {
        try {
            List<String> lst = new ArrayList<String>();
            boolean dropfirst = true;
            for (String arg : args) {
                if (dropfirst) {
                    dropfirst = false;
                    continue;
                }
                lst.add(arg);
            }
            int len = lst.size();
            if (len == 0) {
                super.dump(prefix, pw, args);
                return;
            }
            String[] nargs = (String[])lst.toArray(new String[len]);
            if (!DebugController.dump(null, pw, nargs, this)) {
                super.dump(prefix, pw, args);
            }
        }catch (Exception e) {
            Slog.w(TAG, "dump SprdPhoneWindowManager error");
        }
    }
}

