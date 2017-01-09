/** Create by Spreadst */

package com.spreadst.hasmterscreen;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.widget.AbsLockScreen;
import android.widget.AbsoluteLayout;
import android.widget.Button;
import android.widget.ILockScreenListener;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.internal.telephony.IccCardConstants.State;

public class HasmterLockscreen extends AbsLockScreen {

    private static final String TAG = "HasmterLockscreen";
    private static final String PACKAGE_NAME = "com.spreadst.hasmterscreen";
    private final int SINGLECLICK = 0;
    private final int DOUBLECLICK = 1;
    private final int DELAY_UNLOCK = 2;
    private final int TOUCH_DOWN = 3;
    // lock screen core object
    private ILockScreenListener mLockScreenListener = null;
    private Context mContext = null;
    private TextView unlockText = null;
    // manager animation
    private HasmterViewDoubleClickListener mDoubleClickListeners;
    private HasmterAnimationManager mAnimationManager;
    private ImageView mCallImage;
    private ImageView mMsgImage;
    private ImageView mHomeImage;

    private Drawable mCallDrawable;
    private Drawable mMsgDrawable;
    private Drawable mHomeDrawable;

    private ImageView mMsgPromptImage;
    private ImageView mCallPromptImage;

    private List<AnimationDrawable> mCallAnimationSetList;
    private List<AnimationDrawable> mMsgAnimationSetList;
    private List<AnimationDrawable> mHomeAnimationSetList;

    private Resources mRes = null;
    // min animation repeats
    private final int mAnimationRepeats = 5;
    private final int mDelayUnlock = 500;
    private final int mFrameDuration = 200;
    // default mAnimation
    private int mDelayAnimation = 1;
    // modify by Coverity
    // private int mAnimationCount = 2;

    private boolean mUnlock = false;

    // missed messages and calls count
    private int mMissedMsgsCount = 0;
    private int mMissedCallsCount = 0;
    // do main view messages
    private Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            mLockScreenListener.pokeWakelock(10000);
            Log.d(TAG, "handleMessage");
            switch (msg.what) {
                case SINGLECLICK:
                    break;
                case TOUCH_DOWN: {
                    if ((View) msg.obj == mCallImage) {
                        int rand = randomAnimation();
                        AnimationDrawable animation = null;
                        if (rand < mCallAnimationSetList.size()) {
                            animation = mCallAnimationSetList.get(rand);
                        }
                        if (animation != null) {
                            mAnimationManager.startAnimation(mCallImage, animation,
                                    mCallDrawable, mAnimationRepeats,
                                    animation.getNumberOfFrames() * mFrameDuration);
                        }
                    } else if ((View) msg.obj == mMsgImage) {
                        int rand = randomAnimation();
                        AnimationDrawable animation = null;
                        if (rand < mMsgAnimationSetList.size()) {
                            animation = mMsgAnimationSetList.get(rand);
                            Log.d(TAG, String.format("animation %d", rand));
                        }
                        if (animation != null) {
                            mAnimationManager.startAnimation(mMsgImage, animation,
                                    mMsgDrawable, mAnimationRepeats,
                                    animation.getNumberOfFrames() * mFrameDuration);
                        }
                    } else if ((View) msg.obj == mHomeImage) {
                        int rand = randomAnimation();
                        AnimationDrawable animation = null;
                        if (rand < mHomeAnimationSetList.size()) {
                            animation = mHomeAnimationSetList.get(rand);
                            Log.d(TAG, String.format("animation %d", rand));
                        }
                        if (animation != null) {
                            mAnimationManager.startAnimation(mHomeImage, animation,
                                    mHomeDrawable, mAnimationRepeats,
                                    animation.getNumberOfFrames() * mFrameDuration);
                        }
                    }
                    // message prompt and call prompt signle click unlock event
                    /*
                     * else if ((View) msg.obj == mCallPromptImage || (View)
                     * msg.obj == mMsgPromptImage) { if (!mUnlock) { Drawable
                     * normalDrawable = null; List<AnimationDrawable>
                     * animationDrawables = null; View view = null; try {
                     * if((View) msg.obj == mCallPromptImage) {
                     * HasmterIntentAction.startHasmterIntentAction(
                     * mContext,HasmterIntentAction.ACTION_CALL); } else
                     * if((View) msg.obj == mMsgPromptImage) {
                     * HasmterIntentAction.startHasmterIntentAction(
                     * mContext,HasmterIntentAction.ACTION_SMS); } } catch
                     * (Exception e) { Log.d(TAG,
                     * "start new intent to call msg or home"); } finally {
                     * mUnlock = true;
                     * handler.sendMessage(Message.obtain(handler,
                     * DELAY_UNLOCK)); } } }
                     */
                }
                    break;

                case DOUBLECLICK: {
                    if (!mUnlock) {
                        Drawable normalDrawable = null;
                        List<AnimationDrawable> animationDrawables = null;
                        View view = null;
                        boolean bStartUnlock = false;
                        try {
                            view = (View) msg.obj;
                            if ((View) msg.obj == mCallImage) {
                                HasmterIntentAction.startHasmterIntentAction(
                                        mContext,
                                        HasmterIntentAction.ACTION_CALL);
                                normalDrawable = mCallDrawable;
                                animationDrawables = mCallAnimationSetList;
                                bStartUnlock = true;
                            } else if ((View) msg.obj == mMsgImage) {
                                HasmterIntentAction.startHasmterIntentAction(
                                        mContext,
                                        HasmterIntentAction.ACTION_SMS);
                                normalDrawable = mMsgDrawable;
                                animationDrawables = mMsgAnimationSetList;
                                bStartUnlock = true;
                            } else if ((View) msg.obj == mHomeImage) {
                                HasmterIntentAction.startHasmterIntentAction(
                                        mContext,
                                        HasmterIntentAction.ACTION_HOME);
                                normalDrawable = mHomeDrawable;
                                animationDrawables = mHomeAnimationSetList;
                                bStartUnlock = true;
                            }
                        } catch (Exception e) {
                            Log.d(TAG, "start new intent to call msg or home"
                                    + e.getMessage());
                        } finally {
                            if (bStartUnlock)
                            {
                                Log.d(TAG, "finally unlock");
                                mUnlock = true;
                                // start unlock animation
                                startUnlockAnimation(view, animationDrawables, normalDrawable);
                                // start unlock timer thread
                                sendUnlockThread();
                            }
                        }
                    }
                }
                    break;
                case DELAY_UNLOCK: {
                    Log.d(TAG, "delay unlock message");
                    mLockScreenListener.goToUnlockScreen();
                }
                    break;
            }
        }
    };

    private void startUnlockAnimation(View view, List<AnimationDrawable> animationDrawables,
            Drawable normalDrawable) {
        // stop animation
        if (normalDrawable != null) {
            mAnimationManager.stopAnimation(view,
                    normalDrawable);
        }
        AnimationDrawable animationDrawable = animationDrawables
                .get(mDelayAnimation);
        // start num animation
        if (animationDrawable != null) {
            mAnimationManager.startAnimation(view,
                    animationDrawable, normalDrawable, 2,
                    animationDrawable.getNumberOfFrames()
                            * mFrameDuration);
        }
    }

    private void sendUnlockThread() {
        /*
         * new Thread(new Runnable() {
         * @Override public void run() { try { Thread.sleep(2 * 400); } catch
         * (Exception e) { Log.d(TAG, e.getMessage()); } finally {
         * handler.sendMessage(Message.obtain( handler, DELAY_UNLOCK));
         * Log.d(TAG, "start unlock thread"); } } }).start();
         */
        handler.sendMessageDelayed(Message.obtain(handler, DELAY_UNLOCK), 2 * 400);
    }

    /**
     * hasmter view double click state saver
     */
    class HasmterViewDoubleClickState {
        public View mView = null;
        // public boolean mWaitDouble = true;
        public long mLastClickTime = 0L;

        public HasmterViewDoubleClickState() {
            mView = null;
            // mWaitDouble = true;
        }

        public HasmterViewDoubleClickState(View view, boolean waitDouble, long lastClickTime) {
            mView = view;
            // mWaitDouble = waitDouble;
            mLastClickTime = lastClickTime;
        }
    }

    /**
     * hasmter sub view click event monitor
     */
    class HasmterViewDoubleClickListener {
        private final static String TAG = "HasmterViewDoubleClickListener";
        // interval time between signclick and doubleclick
        private static final int DOUBLE_CLICK_TIME = 500;
        private HashMap<View, HasmterViewDoubleClickState> mListenerViews = null;

        private Handler mHandler = null;

        public HasmterViewDoubleClickListener(Handler handler) {
            this.mHandler = handler;
            mListenerViews = new HashMap<View, HasmterViewDoubleClickState>();
        }

        /**
         * @param add view target
         */
        public void addDoubleClickListener(View view) {
            if (view != null && !mListenerViews.containsKey(view)) {
                Log.d(TAG, "addDoubleClickListener");
                view.setSoundEffectsEnabled(false);
                // on touched event
                view.setOnTouchListener(new OnTouchListener() {

                    @Override
                    public boolean onTouch(View v, MotionEvent event) {
                        if (mListenerViews.containsKey(v)) {
                            int action = event.getAction();
                            if (action == MotionEvent.ACTION_DOWN) {
                                // send tap event
                                touchDown(v);
                                Log.d(TAG, "on touch down event");
                            }
                        }
                        return false;
                    }
                });

                // single click or double click event
                view.setOnClickListener(new OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        Log.d(TAG, "setOnsingleClick");
                        if (mListenerViews.containsKey(v)) {
                            final HasmterViewDoubleClickState clickState = mListenerViews
                                    .get(v);
                            final View actionView = v;
                            long currentTime = System.currentTimeMillis();
                            if ((currentTime - clickState.mLastClickTime) > 0
                                    && (currentTime - clickState.mLastClickTime) < DOUBLE_CLICK_TIME) {
                                doubleClick(actionView);
                                Log.d(TAG, "double click action");
                            }
                            Log.d(TAG, "interval time: "
                                    + (currentTime - clickState.mLastClickTime));
                            clickState.mLastClickTime = currentTime;
                        }
                        // single click and double click events
                        /*
                         * if (mListenerViews.containsKey(v)) { final
                         * HasmterViewDoubleClickState clickState =
                         * mListenerViews .get(v); final View actionView = v; if
                         * (clickState.mWaitDouble) { clickState.mWaitDouble =
                         * false; Thread thread = new Thread() {
                         * @Override public void run() { try {
                         * Thread.sleep(DOUBLE_CLICK_TIME); if
                         * (clickState.mWaitDouble == false) {
                         * clickState.mWaitDouble = true;
                         * singleClick(actionView); Log.d(TAG,
                         * "trigger single Click"); } } catch
                         * (InterruptedException e) { Log.d(TAG,
                         * e.getMessage()); } } }; thread.start(); } else {
                         * clickState.mWaitDouble = true; doubleClick(v);
                         * Log.d(TAG, "trigger double click"); } }
                         */
                    }
                });
                mListenerViews.put(view, new HasmterViewDoubleClickState());
            }
        }

        public void removeDoubleCliclListener(View view) {
            if (view != null && mListenerViews.containsKey(view)) {
                view.setOnClickListener(new OnClickListener() {
                    // /just onclick event
                    @Override
                    public void onClick(View v) {
                        singleClick(v);
                    }
                });
                mListenerViews.remove(view);
            }
        }

        /**
         * trigger signle click
         */
        private void singleClick(View view) {
            Log.d(TAG, "singleClick");
            if (view != null) {
                SendMessage(mHandler, SINGLECLICK, view);
            }
        }

        /**
         * trigger touch down
         */
        private void touchDown(View view)
        {
            Log.d(TAG, "touch down");
            if (view != null) {
                SendMessage(mHandler, TOUCH_DOWN, view);
            }
        }

        /**
         * trigger double click
         */
        private void doubleClick(View view) {
            Log.d(TAG, "doubleClick");
            if (view != null) {
                SendMessage(mHandler, DOUBLECLICK, view);
                Log.d(TAG, "sendmessage");
            }
        }

        /**
         * send message to main program
         */
        private void SendMessage(Handler handler, int what, Object obj) {
            if (handler != null) {
                try {
                    // just deal each events for one time
                    if (handler.hasMessages(what)) {
                        handler.removeMessages(what);
                        Log.d(TAG, "remove click events");
                    }
                    //
                    Message msg = Message.obtain(handler, what, obj);
                    handler.sendMessage(msg);
                } catch (Exception e) {
                    Log.d(TAG, e.getMessage());
                }
            }
        }
    }

    /**
     * manager hasmter animation manager status
     */
    class HasmterAnimationManager {

        private HashMap<View, Thread> mAnimationThreads = null;
        private final int ANIMATION_MANAGER_START = 0;
        private final int ANIMATION_MANAGER_STOP = 1;
        private final int ANIMATION_MANAGER_USERABOTR = 2;

        public HasmterAnimationManager() {
            mAnimationThreads = new HashMap<View, Thread>();
        }

        /**
         * Handler Message Parameters
         */
        class AnimationManagerParameters {
            private View mViewTarget = null;
            private AnimationDrawable mAnimationDrawable = null;
            private Drawable mNormalDrawable = null;

            public AnimationManagerParameters(View view,
                    AnimationDrawable animationDrawable, Drawable normalDrawable) {
                this.mViewTarget = view;
                this.mAnimationDrawable = animationDrawable;
                this.mNormalDrawable = normalDrawable;
            }
        }

        private Handler animationHandler = new Handler() {
            // run animation
            private void startAnimation(Message msg) {
                try {
                    AnimationManagerParameters animationManagerParameters = (AnimationManagerParameters) msg.obj;
                    ImageView viewTarget = (ImageView) animationManagerParameters.mViewTarget;
                    if (viewTarget != null
                            && animationManagerParameters.mAnimationDrawable != null) {
                        animationManagerParameters.mAnimationDrawable
                                .setOneShot(false);
                        viewTarget
                                .setImageDrawable(animationManagerParameters.mAnimationDrawable);
                        animationManagerParameters.mAnimationDrawable.start();
                    }
                } catch (Exception ex) {
                    Log.d(TAG, ex.getMessage());
                }
            }

            // stop animation
            private void stopAnimation(Message msg) {
                try {
                    AnimationManagerParameters animationManagerParameters = (AnimationManagerParameters) msg.obj;
                    ImageView viewTarget = (ImageView) animationManagerParameters.mViewTarget;

                    if (animationManagerParameters.mAnimationDrawable != null
                            && animationManagerParameters.mAnimationDrawable
                                    .isRunning()) {
                        animationManagerParameters.mAnimationDrawable.stop();
                    }

                    if (viewTarget != null) {
                        viewTarget
                                .setImageDrawable(animationManagerParameters.mNormalDrawable);
                    }
                } catch (Exception ex) {
                    Log.d(TAG, ex.getMessage());
                }
            }

            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case ANIMATION_MANAGER_START: {
                        startAnimation(msg);
                    }
                        break;

                    case ANIMATION_MANAGER_USERABOTR: {
                        stopAnimation(msg);
                    }
                        break;

                    case ANIMATION_MANAGER_STOP: {
                        try {
                            AnimationManagerParameters animationManagerParameters = (AnimationManagerParameters) msg.obj;
                            stopAnimation(msg);
                            mAnimationThreads
                                    .remove(animationManagerParameters.mViewTarget);
                        } catch (Exception ex) {
                            Log.d(TAG, "ANIMATION_MANAGER_STOP" + ex.getMessage());
                        }
                    }
                        break;
                }
            };
        };

        // check is parameters
        private boolean isEffectiveParameters(View view,
                AnimationDrawable animationDrawable, int repeatTime,
                int runOnceTimeCount) {
            if (view == null || animationDrawable == null || repeatTime < 0
                    || runOnceTimeCount < 0) {
                return false;
            } else {
                return true;
            }
        }

        // start animation
        public void startAnimation(View view,
                AnimationDrawable animationDrawable, Drawable normalDrawable,
                int repeatTime, int runOnceTimeCount) {
            try {
                if (isEffectiveParameters(view, animationDrawable, repeatTime,
                        runOnceTimeCount)) {
                    // not in AnimationManager
                    if (!mAnimationThreads.containsKey(view)) {
                        if (!animationDrawable.isRunning()) {
                            AnimationRunningManagerThread animationRunningManagerThread =
                                    new AnimationRunningManagerThread(view, animationDrawable,
                                            normalDrawable, repeatTime, runOnceTimeCount);
                            // run animation manager thread
                            mAnimationThreads.put(view,
                                    animationRunningManagerThread);
                            animationRunningManagerThread.start();
                        }
                    }
                    // already add in AnimationManager
                    else
                    {
                        if (animationDrawable.isRunning())
                        {
                            AnimationRunningManagerThread animationRunningManagerThread = (AnimationRunningManagerThread) mAnimationThreads
                                    .get(view);
                            animationRunningManagerThread.resetRepeatCount();
                        }
                    }
                }
            } catch (Exception ex) {
                Log.d(TAG, ex.getMessage());
            }
        }

        public void stopAnimation(View view, Drawable normalDrawable) {
            try {
                if (view != null && mAnimationThreads.containsKey(view)) {
                    AnimationRunningManagerThread animationRunningManagerThread = (AnimationRunningManagerThread) mAnimationThreads
                            .get(view);
                    // when finish get animationRunningManagerThread,
                    // the animationRunningManagerThread may finished
                    // send default drawable,in case of null pointer
                    SendMessage(ANIMATION_MANAGER_USERABOTR,
                            new AnimationManagerParameters(view, null,
                                    normalDrawable));
                    mAnimationThreads.remove(view);
                    animationRunningManagerThread.stop();
                }
            } catch (Exception e) {
                Log.d(TAG, "Stop animation: " + e.getMessage());
            }
        }

        /**
         * send message to manager handler
         */
        void SendMessage(int what, Object object) {
            Message msg = Message.obtain(animationHandler, what, object);
            animationHandler.sendMessage(msg);
        }

        /**
         * animation running manager
         */
        class AnimationRunningManagerThread extends Thread {
            private View mViewTarget = null;
            private AnimationDrawable mAnimationDrawable = null;
            private Drawable mNormalDrawable = null;
            // repeat time
            private int mCurrentRepeatCount = 0;
            private int mRepeatTime = 1;
            // count run time total time
            private int mRunOnceTimeCount = 0;
            private final Object mSynchronizer = new Object();

            public AnimationRunningManagerThread(View view,
                    AnimationDrawable animationDrawable,
                    Drawable normalDrawable, int repeatTime,
                    int runOnceTimeCount) {
                this.mViewTarget = view;
                this.mAnimationDrawable = animationDrawable;
                this.mNormalDrawable = normalDrawable;
                this.mRepeatTime = repeatTime;
                this.mRunOnceTimeCount = runOnceTimeCount;
            }

            public void resetRepeatCount()
            {
                synchronized (mSynchronizer) {
                    mCurrentRepeatCount = 0;
                }
            }

            @Override
            public void run() {
                AnimationManagerParameters managerParameters = new AnimationManagerParameters(
                        mViewTarget, mAnimationDrawable, mNormalDrawable);
                try {
                    // start animation message
                    SendMessage(ANIMATION_MANAGER_START, managerParameters);
                    // int repeatCount = 0
                    for (; mCurrentRepeatCount < mRepeatTime;) {
                        Thread.sleep(mRunOnceTimeCount);
                        synchronized (mSynchronizer) {
                            ++mCurrentRepeatCount;
                        }
                    }
                } catch (Exception ex) {
                    Log.d(TAG, "AnimationRunningManagerThread exception");
                } finally {
                    // stop animation message
                    Log.d(TAG, "AnimationRunningManagerThread finish");
                    SendMessage(ANIMATION_MANAGER_STOP, managerParameters);
                }
            }
        }
    }

    public HasmterLockscreen(Context context, ILockScreenListener listener)
            throws Exception {
        super(context, listener);
        // bind communite object
        mLockScreenListener = listener;
        mContext = context;

        mDoubleClickListeners = new HasmterViewDoubleClickListener(handler);
        mAnimationManager = new HasmterAnimationManager();
        try {
            init();
        } catch (Exception ex) {
            throw new Exception(ex.getMessage());
        }
    }

    private void init() throws Exception {
        try {
            Resources res = mContext.getPackageManager()
                    .getResourcesForApplication(PACKAGE_NAME);
            this.mRes = res;
            LayoutInflater inflater = LayoutInflater.from(mContext);
            inflater.inflate(
                    res.getLayout(res.getIdentifier(PACKAGE_NAME
                            + ":layout/hasmterlockscreen", null, null)), this,
                    true);
            // layout
            AbsoluteLayout borderLayout = (AbsoluteLayout) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/hasmter_border", null, null));

            Drawable backgroundDrawable = res.getDrawable(res.getIdentifier(
                    "hasmter_lockscreen_bg_sprd", "drawable", PACKAGE_NAME));

            borderLayout.setBackgroundDrawable(backgroundDrawable);

            // /Call
            mCallImage = (ImageView) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/hasmter_control_call", null, null));
            mCallDrawable = res.getDrawable(res.getIdentifier(
                    "hasmter_lockscreen_call_sprd", "drawable", PACKAGE_NAME));
            mCallImage.setImageDrawable(mCallDrawable);
            // add double click listeners
            mDoubleClickListeners.addDoubleClickListener(mCallImage);

            // /Message
            mMsgImage = (ImageView) findViewById(res.getIdentifier(PACKAGE_NAME
                    + ":id/hasmter_control_message", null, null));
            mMsgDrawable = res.getDrawable(res.getIdentifier(
                    "hasmter_lockscreen_message_sprd", "drawable", PACKAGE_NAME));
            mMsgImage.setImageDrawable(mMsgDrawable);
            // add double click listeners
            mDoubleClickListeners.addDoubleClickListener(mMsgImage);

            // /Home
            mHomeImage = (ImageView) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/hasmter_control_home", null, null));
            mHomeDrawable = res.getDrawable(res.getIdentifier(
                    "hasmter_lockscreen_home_sprd", "drawable", PACKAGE_NAME));
            mHomeImage.setImageDrawable(mHomeDrawable);
            // add double click listeners
            mDoubleClickListeners.addDoubleClickListener(mHomeImage);

            // UnlockText
            unlockText = (TextView) findViewById(res.getIdentifier(PACKAGE_NAME
                    + ":id/unlockText", null, null));
            // unlockText.setBackgroundDrawable(mUnLockTextable);
            unlockText.setText(res.getString(res.getIdentifier(PACKAGE_NAME
                    + ":string/unlock", null, null)));

            // Call Prompt
            mCallPromptImage = (ImageView) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/hasmter_prompt_call", null, null));
            // mDoubleClickListeners.addDoubleClickListener(mCallPromptImage);

            // msg Prompt
            mMsgPromptImage = (ImageView) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/hasmter_prompt_message", null, null));

            // mDoubleClickListeners.addDoubleClickListener(mMsgPromptImage);
            // init call msg home text

            TextView tvCall = (TextView) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/tv_phone", null, null));
            tvCall.setText(res.getString(res.getIdentifier(PACKAGE_NAME
                    + ":string/text_phone", null, null)));

            TextView tvMessage = (TextView) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/tv_message", null, null));
            tvMessage.setText(res.getString(res.getIdentifier(PACKAGE_NAME
                    + ":string/text_message", null, null)));

            TextView tvHome = (TextView) findViewById(res.getIdentifier(
                    PACKAGE_NAME + ":id/tv_home", null, null));
            tvHome.setText(res.getString(res.getIdentifier(PACKAGE_NAME
                    + ":string/text_home", null, null)));

            // load animation resources
            loadMissedMsgsAndCallsCount();
            initAnimationSets(res);
            initMessageAndCall();
        } catch (Exception ex) {
            Log.d(TAG, "Error: " + ex.getMessage());
            throw new Exception(ex.getMessage());
        }
    }

    private void loadMissedMsgsAndCallsCount() {
        mMissedMsgsCount = TelephoneInfoManager.getUnReadMessageCount(mContext);
        mMissedCallsCount = TelephoneInfoManager.getMissedCalls(mContext);
    }

    // init animationset resources
    private void initAnimationSets(Resources res) {
        mCallAnimationSetList = new ArrayList<AnimationDrawable>();
        mMsgAnimationSetList = new ArrayList<AnimationDrawable>();
        mHomeAnimationSetList = new ArrayList<AnimationDrawable>();

        mCallAnimationSetList.add(loadAnimationSetsResource(res, PACKAGE_NAME,
                "susliks_swim_action_phone0", 1, 3));
        mMsgAnimationSetList.add(loadAnimationSetsResource(res, PACKAGE_NAME,
                "susliks_swim_action_message0", 1, 3));
        mHomeAnimationSetList.add(loadAnimationSetsResource(res, PACKAGE_NAME,
                "susliks_swim_action_menu0", 1, 3));

        mCallAnimationSetList.add(loadAnimationSetsResource(res, PACKAGE_NAME,
                "susliks_start_phone0", 1, 2));
        mMsgAnimationSetList.add(loadAnimationSetsResource(res, PACKAGE_NAME,
                "susliks_start_message0", 1, 2));
        mHomeAnimationSetList.add(loadAnimationSetsResource(res, PACKAGE_NAME,
                "susliks_start_menu0", 1, 2));
    }

    // load animation resources from fromId to toId
    private AnimationDrawable loadAnimationSetsResource(Resources res, String packageName,
            String fileName,
            int fromId, int toId) {
        AnimationDrawable animationDrawable = new AnimationDrawable();
        try {
            for (; fromId <= toId; fromId++)
            {
                Drawable drawable = res.getDrawable(res.getIdentifier(
                        fileName + Integer.toString(fromId) + "_sprd",
                        "drawable", PACKAGE_NAME));
                animationDrawable.addFrame(drawable, mFrameDuration);
            }
        } catch (Exception e) {
            animationDrawable = null;
            Log.d(TAG, "load animation failed");
        }

        return animationDrawable;
    }

    // random animation create
    private int randomAnimation() {
        return 0;// ((int) (Math.random() * 10000)) % mAnimationCount;
    }

    @Override
    public void onRefreshBatteryInfo(boolean showBatteryInfo,
            boolean pluggedIn, int batteryLevel) {
    }

    @Override
    public void onTimeChanged() {
    }

    @Override
    public boolean needsInput() {
        return false;
    }

    @Override
    public void onPhoneStateChanged(int phoneState) {
    }

    @Override
    public void onRefreshCarrierInfo(CharSequence plmn, CharSequence spn,
            int subscription) {
    }

    @Override
    public void onRingerModeChanged(int state) {
    }

    @Override
    public void onSimStateChanged(State simState, int subscription) {
    }

    @Override
    public void onPause() {
    }

    @Override
    public void onResume() {
        loadMissedMsgsAndCallsCount();
        initMessageAndCall();
    }

    @Override
    public void cleanUp() {
    }

    @Override
    public void onStartAnim() {
    }

    @Override
    public void onStopAnim() {
    }

    @Override
    public void onClockVisibilityChanged() {
    }

    @Override
    public void onDeviceProvisioned() {
    }

    private void initMessageAndCall() {
        if (mContext != null)
        {
            setMissedCallCount(mMissedCallsCount);
            setMessageCount(mMissedMsgsCount);
        }
    }

    @Override
    public void onMessageCountChanged(int messagecount) {
        Log.d("wb", "onMessageCountChanged " + messagecount);
        if (messagecount > 0) {
            mMissedMsgsCount -= messagecount;
            if (mMissedMsgsCount < 0) {
                mMissedMsgsCount = 0;
            }
            setMessageCount(mMissedMsgsCount);
        }
        Log.d("wb", "onMessageCountChanged mMissedMsgsCount " + mMissedMsgsCount);
    }

    @Override
    public void onDeleteMessageCount(int messagecount) {
        Log.d("wb", "onDeleteMessageCount " + messagecount);
        if (messagecount > 0) {
            mMissedMsgsCount -= messagecount;
            if (mMissedMsgsCount < 0) {
                mMissedMsgsCount = 0;
            }
            setMessageCount(mMissedMsgsCount);
        }
        Log.d("wb", "onDeleteMessageCount " + mMissedMsgsCount);
    }

    private void setMessageCount(int messagecount) {
        if (messagecount > 0) {
            Drawable bgDrawable = mRes.getDrawable(mRes.getIdentifier(
                    "hasmter_lockscreen_promptmessagebg_sprd", "drawable", PACKAGE_NAME));
            final int max_missed = 99;
            final int left_margin = 27;
            final int top_margin = 12;
            Bitmap newBitmap = createMergeNumbersBimtap(bgDrawable, max_missed,
                    messagecount, left_margin, top_margin);
            mMsgPromptImage.setImageBitmap(newBitmap);
            mMsgPromptImage.setVisibility(View.VISIBLE);
            Log.d("wb", "msg visible");
        }
        else {
            mMsgPromptImage.setVisibility(View.INVISIBLE);
            Log.d("wb", "msg invisible");
        }
    }

    @Override
    public void onMissedCallCountChanged(int count) {
        if (count > 0) {
            mMissedCallsCount += count;
            if (mMissedCallsCount < 0) {
                mMissedCallsCount = 0;
            }
            setMissedCallCount(mMissedCallsCount);
        }
    }

    private void setMissedCallCount(int callcount) {
        if (callcount > 0) {
            Drawable bgDrawable = mRes
                    .getDrawable(mRes.getIdentifier(
                            "hasmter_lockscreen_promptcallbg_sprd", "drawable",
                            PACKAGE_NAME));
            final int max_missed = 99;
            final int left_margin = 27;
            final int top_margin = 15;
            Bitmap newBitmap = createMergeNumbersBimtap(bgDrawable, max_missed,
                    callcount, left_margin, top_margin);
            mCallPromptImage.setImageBitmap(newBitmap);
            mCallPromptImage.setVisibility(View.VISIBLE);
            Log.d("wb", "call visible");
        }
        else {
            mCallPromptImage.setVisibility(View.INVISIBLE);
            Log.d("wb", "call invisible");
        }
    }

    /**
     * merge two bitmap
     */
    private Bitmap createMergeNumbersBimtap(Drawable bgDrawable, int maxnumber,
            int count, int leftMargin, int topMargin) {
        final int max_missed = maxnumber;
        Bitmap bitmap = null;
        // no message or call
        if (count <= 0) {
            bitmap = mergeNumbersBitmap(bgDrawable, null, null, leftMargin, topMargin);
        }
        // over max numbers
        else if (count > max_missed) {
            Drawable drawable = mRes.getDrawable(mRes.getIdentifier("animation_numbermax_sprd",
                    "drawable", PACKAGE_NAME));
            bitmap = mergeNumbersBitmap(bgDrawable, drawable, null, leftMargin, topMargin);
        }
        // normal
        else {
            int tenDigit = count / 10;
            int bitDigit = (count % 10);
            Drawable tenDrawable = null;
            Drawable bitDrawable = null;
            if (tenDigit == 0) {
                if (bitDigit != 0) {
                    bitDrawable = mRes.getDrawable(mRes.getIdentifier(
                            String.format("animation_number%d_sprd", bitDigit),
                            "drawable", PACKAGE_NAME));
                }
            } else {
                tenDrawable = mRes.getDrawable(mRes.getIdentifier(
                        String.format("animation_number%d_sprd", tenDigit),
                        "drawable", PACKAGE_NAME));

                bitDrawable = mRes.getDrawable(mRes.getIdentifier(
                        String.format("animation_number%d_sprd", bitDigit),
                        "drawable", PACKAGE_NAME));
            }
            bitmap = mergeNumbersBitmap(bgDrawable, tenDrawable, bitDrawable, leftMargin, topMargin);
        }

        return bitmap;
    }

    /**
     * draw two bitmap to canvas
     */
    private Bitmap mergeNumbersBitmap(Drawable background,
            Drawable firstNumber, Drawable secondNumber, int leftMargin, int topMargin) {
        Bitmap newBitmap = null;
        try {
            BitmapDrawable bgBitmapDrawable = (BitmapDrawable) background;
            Bitmap bgBitmap = bgBitmapDrawable.getBitmap();
            int width = bgBitmap.getWidth();
            int height = bgBitmap.getHeight();
            newBitmap = Bitmap.createBitmap(width, height, Config.ARGB_8888);
            // copy background to canvas
            Canvas canvas = new Canvas(newBitmap);
            canvas.drawBitmap(bgBitmap, 0, 0, null);
            leftMargin = ((firstNumber != null && secondNumber != null) ? leftMargin / 2
                    : leftMargin);

            if (firstNumber != null) {
                BitmapDrawable firstBitmapDrawable = (BitmapDrawable) firstNumber;
                Bitmap firstBitmap = firstBitmapDrawable.getBitmap();
                int bitmapWidth = firstBitmap.getWidth();
                canvas.drawBitmap(firstBitmap, leftMargin, topMargin, null);
                leftMargin += bitmapWidth;
            }

            if (secondNumber != null) {
                BitmapDrawable secondBitmapDrawable = (BitmapDrawable) secondNumber;
                Bitmap secondBitmap = secondBitmapDrawable.getBitmap();
                canvas.drawBitmap(secondBitmap, leftMargin, topMargin, null);
            }
            canvas.save(Canvas.ALL_SAVE_FLAG);
            canvas.restore();
        } catch (Exception e) {
            Log.d(TAG, e.getMessage());
            newBitmap = null;
        } finally {
            return newBitmap;
        }
    }
    /* SPRD: Modify 20140207 Spreadst of Bug 267015 add owner info text for UUI lockscreen @{ */
    @Override
    public void onScreenTurnedOn() {
        // TODO Auto-generated method stub
    }

    @Override
    public void onScreenTurnedOff(int why) {
        // TODO Auto-generated method stub
    }
    /* @} */
}
