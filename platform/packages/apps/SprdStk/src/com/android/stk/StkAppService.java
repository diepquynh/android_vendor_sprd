/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.android.stk;

import android.app.ActivityManager;
import android.app.ActivityManager.RunningTaskInfo;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RecentTaskInfo;
import android.app.ActivityManager.RunningAppProcessInfo;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.SystemProperties;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManagerEx;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.RemoteViews;
import android.widget.TextView;
import android.widget.Toast;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager.NameNotFoundException;

import com.android.internal.telephony.cat.AppInterfaceSprd;
import com.android.internal.telephony.cat.LaunchBrowserMode;
import com.android.internal.telephony.cat.Menu;
import com.android.internal.telephony.cat.Item;
import com.android.internal.telephony.cat.InputSprd;
import com.android.internal.telephony.cat.ResultCodeSprd;
import com.android.internal.telephony.cat.CatCmdMessageSprd;
import com.android.internal.telephony.cat.CatCmdMessageSprd.BrowserSettings;
import com.android.internal.telephony.cat.CatCmdMessageSprd.SetupEventListSettings;
import com.android.internal.telephony.cat.CatLog;
import com.android.internal.telephony.cat.CatResponseMessageSprd;
import com.android.internal.telephony.cat.TextMessage;
import com.android.internal.telephony.uicc.IccRefreshResponse;
import com.android.internal.telephony.uicc.IccCardStatus.CardState;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.SubscriptionController;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.GsmAlphabet;
import com.android.internal.telephony.cat.CatServiceSprd;
import com.android.internal.telephony.TeleFrameworkFactory;

import java.util.Iterator;
import java.util.LinkedList;
import java.lang.System;
import java.util.List;

import static com.android.internal.telephony.cat.CatCmdMessageSprd.
                   SetupEventListConstants.IDLE_SCREEN_AVAILABLE_EVENT;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.
                   SetupEventListConstants.LANGUAGE_SELECTION_EVENT;
/* SPRD: Add here for EVENTDOWNLOAD function @{ */
import static com.android.internal.telephony.cat.CatCmdMessageSprd.SetupEventListConstants.DATA_AVAILABLE_EVENT;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.SetupEventListConstants.CHANNEL_STATUS_EVENT;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.SetupEventListConstants.USER_ACTIVITY_EVENT;
/* @} */
/* SPRD: Add here for Open Channel function @{ */
import android.content.ContentResolver;
import android.content.ContentValues;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.Network;
import android.net.NetworkUtils;
import android.net.NetworkInfo;
import android.net.ConnectivityManager;
import android.os.SystemClock;
import android.os.ServiceManager;
import android.os.RemoteException;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.provider.Telephony;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.cat.CommandDetailsSprd;
import com.android.internal.telephony.cat.bip.OpenChannelData;
import com.android.internal.telephony.cat.bip.ReceiveChannelData;
import com.android.internal.telephony.cat.bip.SendChannelData;
import com.android.internal.telephony.TelephonyProperties;
import com.android.internal.telephony.ITelephony;
import java.net.InetSocketAddress;
import java.net.InetAddress;
import java.net.Socket;
import java.net.SocketException;
import java.net.DatagramSocket;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.UnknownHostException;
import java.io.IOException;
import java.io.InputStream;
/* @} */

/* SPRD: notification can not display long text, so we start activity to support  @{*/
import android.graphics.Bitmap;
/* @}*/

/* SPRD: STK SETUP CALL feature support @{*/
import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;
/* @} */

/* SPRD: add for query call forward when send ss, meger from Bug#64554.@{ */
import com.android.internal.telephony.CommandsInterface;
/* @}*/

import com.sprd.stk.StkPluginsHelper;
import com.sprd.stk.StkRefreshPluginsHelper;
import com.sprd.stk.StkSetupCallPluginsHelper;
import com.sprd.stk.StkSetupMenuPluginsHelper;
import com.sprd.stk.StkCuccOperatorPluginsHelper;
import com.android.sprd.telephony.RadioInteractor;
/**
 * SIM toolkit application level service. Interacts with Telephopny messages,
 * application's launch and user input from STK UI elements.
 *
 */
public class StkAppService extends Service implements Runnable {

    // members
    protected class StkContext {
        protected CatCmdMessageSprd mMainCmd = null;
        protected CatCmdMessageSprd mCurrentCmd = null;
        protected CatCmdMessageSprd mCurrentMenuCmd = null;
        protected Menu mCurrentMenu = null;
        protected String lastSelectedItem = null;
        protected boolean mMenuIsVisible = false;
        protected boolean mIsInputPending = false;
        protected boolean mIsMenuPending = false;
        protected boolean mIsDialogPending = false;
        protected boolean responseNeeded = true;
        protected boolean launchBrowser = false;
        protected BrowserSettings mBrowserSettings = null;
        protected LinkedList<DelayedCmd> mCmdsQ = null;
        protected boolean mCmdInProgress = false;
        protected int mStkServiceState = STATE_UNKNOWN;
        protected int mSetupMenuState = STATE_UNKNOWN;
        protected int mMenuState = StkMenuActivity.STATE_INIT;
        protected int mOpCode = -1;
        private Activity mActivityInstance = null;
        private Activity mDialogInstance = null;
        private Activity mMainActivityInstance = null;
        private int mSlotId = 0;
        private SetupEventListSettings mSetupEventListSettings = null;
        private boolean mClearSelectItem = false;
        private boolean mDisplayTextDlgIsVisibile = false;
        private CatCmdMessageSprd mCurrentSetupEventCmd = null;
        private CatCmdMessageSprd mIdleModeTextCmd = null;
        /* SPRD: STK SETUP CALL feature support @{*/
        protected boolean mSetupCallInProcess = false; // true means in process.
        /* @} */
        /* SPRD: Add here for BIP function @{ */
        private OpenChannelData mOpenChannelData = null;
        private ChannelStatus mChannelStatus = new ChannelStatus();
        private ChannelStatus mLastChannelStatus = new ChannelStatus();
        private boolean mTeardownRequested = false;
        private Uri mUri;
        private String mCurPos;
        private static final String DEFAULT = "0";
        private boolean mChannelEstablished = false;
        private Socket mTcpSocket = null;
        private DatagramSocket mUdpSocket = null;
        private int mReceiveDataLen = 0;
        private int mReceiveDataOffset = 0;
        private int mSendDataLen = 0;
        private byte[] mRecevieData = null;
        private int mConnectStatus = -1;
        private InetAddress mInetAddress =null;
        private int mConnectType = -1;
        private  BroadcastReceiver mReceiver = null;
        private final Object mConnectLock = new Object();
        private ConnectivityManager mConnMgr = null;
        private Network mNetwork;
        private NetworkRequest mNetworkRequest;
        private ConnectivityManager.NetworkCallback mNetworkCallback;
        /* @} */
        final synchronized void setPendingActivityInstance(Activity act) {
            CatLog.d(this, "setPendingActivityInstance act : " + mSlotId + ", " + act);
            callSetActivityInstMsg(OP_SET_ACT_INST, mSlotId, act);
        }
        final synchronized Activity getPendingActivityInstance() {
            CatLog.d(this, "getPendingActivityInstance act : " + mSlotId + ", " +
                    mActivityInstance);
            return mActivityInstance;
        }
        final synchronized void setPendingDialogInstance(Activity act) {
            CatLog.d(this, "setPendingDialogInstance act : " + mSlotId + ", " + act);
            callSetActivityInstMsg(OP_SET_DAL_INST, mSlotId, act);
        }
        final synchronized Activity getPendingDialogInstance() {
            CatLog.d(this, "getPendingDialogInstance act : " + mSlotId + ", " +
                    mDialogInstance);
            return mDialogInstance;
        }
        final synchronized void setMainActivityInstance(Activity act) {
            CatLog.d(this, "setMainActivityInstance act : " + mSlotId + ", " + act);
            callSetActivityInstMsg(OP_SET_MAINACT_INST, mSlotId, act);
        }
        final synchronized Activity getMainActivityInstance() {
            CatLog.d(this, "getMainActivityInstance act : " + mSlotId + ", " +
                    mMainActivityInstance);
            return mMainActivityInstance;
        }
        final synchronized void reset(){
            mMainCmd = null;
            mCurrentCmd = null;
            mCurrentMenuCmd = null;
            mIdleModeTextCmd = null;
            mReceiver = null;
        }
    }

    private volatile Looper mServiceLooper;
    private volatile ServiceHandler mServiceHandler;
    private Context mContext = null;
    private NotificationManager mNotificationManager = null;
    static StkAppService sInstance = null;
    private AppInterfaceSprd[] mStkService = null;
    private StkContext[] mStkContext = null;
    private int mSimCount = 0;
    private PowerManager mPowerManager = null;
    // SPRD: Add here for STK feature.
    private StkCmdReceiver mStkCmdReceiver = null;
    /* SPRD: Add here for BIP function @{ */
    private static StringBuilder[] sendDataStorer = null;
    private static final String TYPE_STK = "supl";
    /* @} */
    /* SPRD: Modify the bear type @{ */
    private static final String TYPE_PACKAGE_DATA_PROTOCOL_IP = "02";
    /* @} */

    /* SPRD: add for query call forward when send ss, meger from Bug#64554.@{ */
    private Phone mPhone;
    /* @} */
    public Toast mToast = null;

    // Used for setting FLAG_ACTIVITY_NO_USER_ACTION when
    // creating an intent.
    private enum InitiatedByUserAction {
        yes,            // The action was started via a user initiated action
        unknown,        // Not known for sure if user initated the action
    }

    // constants
    static final String OPCODE = "op";
    static final String CMD_MSG = "cmd message";
    static final String RES_ID = "response id";
    static final String MENU_SELECTION = "menu selection";
    static final String INPUT = "input";
    static final String HELP = "help";
    static final String CONFIRMATION = "confirm";
    static final String CHOICE = "choice";
    static final String SLOT_ID = "SLOT_ID";
    static final String STK_CMD = "STK CMD";
    static final String STK_DIALOG_URI = "stk://com.android.stk/dialog/";
    static final String STK_MENU_URI = "stk://com.android.stk/menu/";
    static final String STK_INPUT_URI = "stk://com.android.stk/input/";
    static final String STK_TONE_URI = "stk://com.android.stk/tone/";

    // These below constants are used for SETUP_EVENT_LIST
    static final String SETUP_EVENT_TYPE = "event";
    static final String SETUP_EVENT_CAUSE = "cause";

    // operations ids for different service functionality.
    static final int OP_CMD = 1;
    static final int OP_RESPONSE = 2;
    static final int OP_LAUNCH_APP = 3;
    static final int OP_END_SESSION = 4;
    static final int OP_BOOT_COMPLETED = 5;
    private static final int OP_DELAYED_MSG = 6;
    static final int OP_CARD_STATUS_CHANGED = 7;
    static final int OP_SET_ACT_INST = 8;
    static final int OP_SET_DAL_INST = 9;
    static final int OP_SET_MAINACT_INST = 10;
    static final int OP_LOCALE_CHANGED = 11;
    static final int OP_ALPHA_NOTIFY = 12;
    static final int OP_IDLE_SCREEN = 13;
    static final int OP_USER_ACTIVITY = 14;

    //Invalid SetupEvent
    static final int INVALID_SETUP_EVENT = 0xFF;

    // Response ids
    static final int RES_ID_MENU_SELECTION = 11;
    static final int RES_ID_INPUT = 12;
    static final int RES_ID_CONFIRM = 13;
    static final int RES_ID_DONE = 14;
    static final int RES_ID_CHOICE = 15;

    static final int RES_ID_TIMEOUT = 20;
    static final int RES_ID_BACKWARD = 21;
    static final int RES_ID_END_SESSION = 22;
    static final int RES_ID_EXIT = 23;

    static final int YES = 1;
    static final int NO = 0;

    static final int STATE_UNKNOWN = -1;
    static final int STATE_NOT_EXIST = 0;
    static final int STATE_EXIST = 1;

    /* SPRD: notification can not display long text, so we start activity to support @{*/
    static final int SETUP_CALL_NO_CALL_1 = 0x00;
    static final int SETUP_CALL_NO_CALL_2 = 0x01;
    static final int SETUP_CALL_HOLD_CALL_1 = 0x02;
    static final int SETUP_CALL_HOLD_CALL_2 = 0x03;
    static final int SETUP_CALL_END_CALL_1 = 0x04;
    static final int SETUP_CALL_END_CALL_2 = 0x05;
    /* @} */

    private static final String PACKAGE_NAME = "com.android.stk";
    private static final String STK_MENU_ACTIVITY_NAME = PACKAGE_NAME + ".StkMenuActivity";
    private static final String STK_INPUT_ACTIVITY_NAME = PACKAGE_NAME + ".StkInputActivity";
    private static final String STK_DIALOG_ACTIVITY_NAME = PACKAGE_NAME + ".StkDialogActivity";
    /* SPRD: add for cucc two icon feature . @{ */
    static final String HOMEPRESSEDFLAG = "homepressed";
    /* @} */
    // Notification id used to display Idle Mode text in NotificationManager.
    private static final int STK_NOTIFICATION_ID = 333;
    private static final String LOG_TAG = new Object(){}.getClass().getEnclosingClass().getName();
    /* SPRD:Bug#490274: When receive session_end,return to mainmenu,if go into the stk not by user @{ */
    public static boolean mAccessByUser = false;
    /* @} */
    /* SPRD: Add here for BIP function @{ */
    private static final int EVENT_CREATE_SOCKET = 104;
    private static final int EVENT_TOAST_SHOW    = 105;
    private static final int DEFAULT_BUFFER_SIZE = 0x05DC;
    private static final int RECEIVE_DATA_MAX_TR_LEN = 0xED;
    private static final int CHANNEL_MODE_NO_FURTHER_INFO = 0;
    private static final int CHANNEL_MODE_NOT_USED = 1;
    private static final int CHANNEL_MODE_LINK_DROPPED = 5;
    private static final int SEND_DATA_IMMEDIATELY = 1;
    private static final int SEND_DATA_STORE = 0;
    private static final int SOCKET_OK = 1;
    private static final int SOCKET_FAIL = 0;
    private static final String APN_ID = "apn_id";
    private static final int CREATE_SOCKET = 200;
    private static final int LAUNCH_OPEN_CHANNEL = 201;

    private TcpReceiveData mTcpReceData = null;
    private UdpReceiveData mUdpReceData = null;
    private boolean mNeedRuning = false;
    private boolean mThreadInLoop = false;

    private class ChannelStatus {
        int mChannelId;
        boolean mLinkStatus;
        int mode_info;

        ChannelStatus() {
            this.mChannelId = 0;
            this.mLinkStatus = false;
            mode_info = CHANNEL_MODE_NO_FURTHER_INFO;
        }

        void SetStatus(int id, boolean status) {
            mChannelId = id;
            mLinkStatus = status;
            mode_info = CHANNEL_MODE_NO_FURTHER_INFO;
        }

        void SetStatus(int id, boolean status, int mode) {
            mChannelId = id;
            mLinkStatus = status;
            mode_info = mode;
        }
    }
    /* @} */
    // Inner class used for queuing telephony messages (proactive commands,
    // session end) while the service is busy processing a previous message.
    private class DelayedCmd {
        // members
        int id;
        CatCmdMessageSprd msg;
        int slotId;

        DelayedCmd(int id, CatCmdMessageSprd msg, int slotId) {
            this.id = id;
            this.msg = msg;
            this.slotId = slotId;
        }
    }

    // system property to set the STK specific default url for launch browser proactive cmds
    private static final String STK_BROWSER_DEFAULT_URL_SYSPROP = "persist.radio.stk.default_url";

    @Override
    public void onCreate() {
        CatLog.d(LOG_TAG, "onCreate()+");
        // Initialize members
        int i = 0;
        mContext = getBaseContext();
        mSimCount = TelephonyManager.from(mContext).getSimCount();
        CatLog.d(LOG_TAG, "simCount: " + mSimCount);
        mStkService = new AppInterfaceSprd[mSimCount];
        mStkContext = new StkContext[mSimCount];
        mPowerManager = (PowerManager)getSystemService(Context.POWER_SERVICE);
        mStkCmdReceiver = new StkCmdReceiver();
        /* SPRD: Add here for BIP function for bug511850 @{ */
        sendDataStorer = new StringBuilder[mSimCount];
        /* @} */
        registerReceiver(mStkCmdReceiver, new IntentFilter(Intent.ACTION_SCREEN_OFF));
        for (i = 0; i < mSimCount; i++) {
            CatLog.d(LOG_TAG, "slotId: " + i);
            mStkService[i] = CatServiceSprd.getInstance(i);
            mStkContext[i] = new StkContext();
            mStkContext[i].mSlotId = i;
            mStkContext[i].mCmdsQ = new LinkedList<DelayedCmd>();
            /* SPRD: Add here for BIP function @{ */
            mStkContext[i].mConnMgr = (ConnectivityManager)mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
            /* @} */
        }

        Thread serviceThread = new Thread(null, this, "Stk App Service");
        serviceThread.start();
        mNotificationManager = (NotificationManager) mContext
                .getSystemService(Context.NOTIFICATION_SERVICE);
        sInstance = this;
    }

    @Override
    public void onStart(Intent intent, int startId) {
        if (intent == null) {
            CatLog.d(LOG_TAG, "StkAppService onStart intent is null so return");
            return;
        }

        Bundle args = intent.getExtras();
        if (args == null) {
            CatLog.d(LOG_TAG, "StkAppService onStart args is null so return");
            return;
        }

        /* SPRD: add for cucc two icon feature . @{ */
        CatLog.d(LOG_TAG, "StkAppService onStart is cucc: " + isCUCCOperator());
        /* @} */
        int op = args.getInt(OPCODE);
        int slotId = 0;
        int i = 0;
        if (op != OP_BOOT_COMPLETED) {
            slotId = args.getInt(SLOT_ID);
        }
        CatLog.d(LOG_TAG, "onStart sim id: " + slotId + ", op: " + op + ", *****");
        if ((slotId >= 0 && slotId < mSimCount) && mStkService[slotId] == null) {
            mStkService[slotId] = CatServiceSprd.getInstance(slotId);
            if (mStkService[slotId] == null) {
                CatLog.d(LOG_TAG, "mStkService is: " + mStkContext[slotId].mStkServiceState);
                mStkContext[slotId].mStkServiceState = STATE_NOT_EXIST;
                /* SPRD: add for cucc two icon feature . @{ */
                if(isCUCCOperator()){
                    StkAppInstaller.unInstall(mContext,slotId);
                }
                /* @} */
                //Check other StkService state.
                //If all StkServices are not available, stop itself and uninstall apk.
                for (i = PhoneConstants.SIM_ID_1; i < mSimCount; i++) {
                    if (i != slotId
                            && (mStkContext[i].mStkServiceState == STATE_UNKNOWN
                            || mStkContext[i].mStkServiceState == STATE_EXIST)) {
                       break;
                   }
                }
            } else {
                mStkContext[slotId].mStkServiceState = STATE_EXIST;
            }
            if (i == mSimCount) {
                stopSelf();
                /* SPRD: add for cucc two icon feature . @{ */
                if(!isCUCCOperator()){
                    StkAppInstaller.unInstall(mContext);
                }
                /* @} */
                return;
            }
        }

        waitForLooper();

        Message msg = mServiceHandler.obtainMessage();
        msg.arg1 = op;
        msg.arg2 = slotId;
        /* SPRD: add for query call forward when send ss, meger from Bug#64554.
        and avoid to make crash on phone process from Bug#396499@{ */
        if(msg.arg1 == OP_CMD){
            mPhone = PhoneFactory.getPhone(slotId);
            CatLog.d(this, "init mPhone");
        }
        /* @} */
        switch(msg.arg1) {
        case OP_CMD:
            msg.obj = args.getParcelable(CMD_MSG);
            break;
        case OP_RESPONSE:
        case OP_CARD_STATUS_CHANGED:
        case OP_LOCALE_CHANGED:
        case OP_ALPHA_NOTIFY:
        case OP_IDLE_SCREEN:
        case OP_USER_ACTIVITY:
            msg.obj = args;
            /* falls through */
        case OP_LAUNCH_APP:
        case OP_END_SESSION:
        case OP_BOOT_COMPLETED:
            break;
        default:
            return;
        }
        mServiceHandler.sendMessage(msg);
    }

    @Override
    public void onDestroy() {
        if(mToast != null){
            mToast.cancel();
        }
        CatLog.d(LOG_TAG, "onDestroy()");
        if (mStkCmdReceiver != null) {
            unregisterReceiver(mStkCmdReceiver);
            mStkCmdReceiver = null;
        }
        mPowerManager = null;
        waitForLooper();
        mServiceLooper.quit();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    public void run() {
        Looper.prepare();

        mServiceLooper = Looper.myLooper();
        mServiceHandler = new ServiceHandler();

        Looper.loop();
    }

    /*
     * Package api used by StkMenuActivity to indicate if its on the foreground.
     */
    void indicateMenuVisibility(boolean visibility, int slotId) {
        if (slotId >= 0 && slotId < mSimCount) {
            mStkContext[slotId].mMenuIsVisible = visibility;
        }
    }

    /*
     * Package api used by StkDialogActivity to indicate if its on the foreground.
     */
    void setDisplayTextDlgVisibility(boolean visibility, int slotId) {
        if (slotId >= 0 && slotId < mSimCount) {
            mStkContext[slotId].mDisplayTextDlgIsVisibile = visibility;
        }
    }

    boolean isInputPending(int slotId) {
        if (slotId >= 0 && slotId < mSimCount) {
            CatLog.d(LOG_TAG, "isInputFinishBySrv: " + mStkContext[slotId].mIsInputPending);
            return mStkContext[slotId].mIsInputPending;
        }
        return false;
    }

    boolean isMenuPending(int slotId) {
        if (slotId >= 0 && slotId < mSimCount) {
            CatLog.d(LOG_TAG, "isMenuPending: " + mStkContext[slotId].mIsMenuPending);
            return mStkContext[slotId].mIsMenuPending;
        }
        return false;
    }

    boolean isDialogPending(int slotId) {
        if (slotId >= 0 && slotId < mSimCount) {
            CatLog.d(LOG_TAG, "isDialogPending: " + mStkContext[slotId].mIsDialogPending);
            return mStkContext[slotId].mIsDialogPending;
        }
        return false;
    }

    /*
     * Package api used by StkMenuActivity to get its Menu parameter.
     */
    Menu getMenu(int slotId) {
        CatLog.d(LOG_TAG, "StkAppService, getMenu, sim id: " + slotId);
        if (slotId >=0 && slotId < mSimCount) {
            return mStkContext[slotId].mCurrentMenu;
        } else {
            return null;
        }
    }

    /*
     * Package api used by StkMenuActivity to get its Main Menu parameter.
     */
    Menu getMainMenu(int slotId) {
        CatLog.d(LOG_TAG, "StkAppService, getMainMenu, sim id: " + slotId);
        if (slotId >=0 && slotId < mSimCount && (mStkContext[slotId].mMainCmd != null)) {
            return mStkContext[slotId].mMainCmd.getMenu();
        } else {
            return null;
        }
    }

    /*
     * SPRD: Package api used by StkAppInstaller to set STK Launcher name.
     */
    boolean isShowSetupMenuTitle() {
        int count = 0;
        int id = 0;

        if (isAllCardsExsit()) {
            return false;
        }

        for (int i = 0; i < mSimCount; i++) {
            if (mStkContext[i].mCurrentMenu != null) {
                ++count;
                id = i;
            }
        }
        CatLog.d(LOG_TAG, "isShowSetupMenuTitle count=" + count + " id=" + id);
        if (count == 1) {
            if (StkSetupMenuPluginsHelper.getInstance(mContext).isShowSetupMenuTitle()) {
                return true;
            }
        }

        return false;

    }

     /* SPRD: add for cucc two icon feature . @{ */
     boolean isCUCCOperator() {
         if (StkCuccOperatorPluginsHelper.getInstance(mContext).isCUCCOperator()) {
             CatLog.d(LOG_TAG, "isCUCCOperator ,the operator is cucc!");
             return true;
         }
         CatLog.d(LOG_TAG, "NOT CUCC");
         return false;
     }
     /* @} */

    /*
     * Package api used by UI Activities and Dialogs to communicate directly
     * with the service to deliver state information and parameters.
     */
    static StkAppService getInstance() {
        return sInstance;
    }

    private void waitForLooper() {
        while (mServiceHandler == null) {
            synchronized (this) {
                try {
                    wait(100);
                } catch (InterruptedException e) {
                }
            }
        }
    }

    private final class ServiceHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            if(null == msg) {
                CatLog.d(LOG_TAG, "ServiceHandler handleMessage msg is null");
                return;
            }
            int opcode = msg.arg1;
            int slotId = msg.arg2;

            // SPRD: Add this condition for slotId is invalid.
            if (slotId < 0) {
                CatLog.d(LOG_TAG, "ServiceHandler handleMessage slotId is invalid");
                return;
            }

            CatLog.d(LOG_TAG, "handleMessage opcode[" + opcode + "], sim id[" + slotId + "]");
            if (opcode == OP_CMD && msg.obj != null &&
                    ((CatCmdMessageSprd)msg.obj).getCmdType()!= null) {
                CatLog.d(LOG_TAG, "cmdName[" + ((CatCmdMessageSprd)msg.obj).getCmdType().name() + "]");
            }
            mStkContext[slotId].mOpCode = opcode;
            switch (opcode) {
            case OP_LAUNCH_APP:
                if (mStkContext[slotId].mMainCmd == null) {
                    CatLog.d(LOG_TAG, "mMainCmd is null");
                    // nothing todo when no SET UP MENU command didn't arrive.
                    return;
                }
                CatLog.d(LOG_TAG, "handleMessage OP_LAUNCH_APP - mCmdInProgress[" +
                        mStkContext[slotId].mCmdInProgress + "]");

                //If there is a pending activity for the slot id,
                //just finish it and create a new one to handle the pending command.
                cleanUpInstanceStackBySlot(slotId);

                CatLog.d(LOG_TAG, "Current cmd type: " +
                        mStkContext[slotId].mCurrentCmd.getCmdType());
                //Restore the last command from stack by slot id.
                restoreInstanceFromStackBySlot(slotId);

                /* SPRD:Bug#490274: When receive session_end,return to mainmenu,if go into the stk not by user @{ */
                if (StkPluginsHelper.getInstance(mContext).needCloseMenu()) {
                    mAccessByUser = true;
                    CatLog.d(this, "OP_LAUNCH_APP mAccessByUser=" + mAccessByUser);
                }
                /* @} */
                break;
            case OP_CMD:
                CatLog.d(LOG_TAG, "[OP_CMD]");
                CatCmdMessageSprd cmdMsg = (CatCmdMessageSprd) msg.obj;
                // There are two types of commands:
                // 1. Interactive - user's response is required.
                // 2. Informative - display a message, no interaction with the user.
                //
                // Informative commands can be handled immediately without any delay.
                // Interactive commands can't override each other. So if a command
                // is already in progress, we need to queue the next command until
                // the user has responded or a timeout expired.
                if (!isCmdInteractive(cmdMsg)) {
                    handleCmd(cmdMsg, slotId);
                } else {
                    if (!mStkContext[slotId].mCmdInProgress) {
                        mStkContext[slotId].mCmdInProgress = true;
                        handleCmd((CatCmdMessageSprd) msg.obj, slotId);
                    } else {
                        CatLog.d(LOG_TAG, "[Interactive][in progress]");
                        mStkContext[slotId].mCmdsQ.addLast(new DelayedCmd(OP_CMD,
                                (CatCmdMessageSprd) msg.obj, slotId));
                    }
                }
                break;
            case OP_RESPONSE:
                handleCmdResponse((Bundle) msg.obj, slotId);
                // call delayed commands if needed.
                if (mStkContext[slotId].mCmdsQ.size() != 0) {
                    callDelayedMsg(slotId);
                } else {
                    mStkContext[slotId].mCmdInProgress = false;
                }
                break;
            case OP_END_SESSION:
                if (!mStkContext[slotId].mCmdInProgress) {
                    mStkContext[slotId].mCmdInProgress = true;
                    handleSessionEnd(slotId);
                } else {
                    mStkContext[slotId].mCmdsQ.addLast(
                            new DelayedCmd(OP_END_SESSION, null, slotId));
                }
                break;
            case OP_BOOT_COMPLETED:
                CatLog.d(LOG_TAG, " OP_BOOT_COMPLETED");
                int i = 0;
                for (i = PhoneConstants.SIM_ID_1; i < mSimCount; i++) {
                    /* SPRD: add for cucc two icon feature . @{ */
                    if(isCUCCOperator() && mStkContext[i].mMainCmd == null){
                        StkAppInstaller.unInstall(mContext,i);
                    }
                    /* @} */
                    if (mStkContext[i].mMainCmd != null) {
                        break;
                    }
                }
                if (i == mSimCount) {
                    /* SPRD: add for cucc two icon feature . @{ */
                    //StkAppInstaller.unInstall(mContext);
                    if(!isCUCCOperator()){
                        StkAppInstaller.unInstall(mContext);
                    }
                    /* @} */
                }
                break;
            case OP_DELAYED_MSG:
                handleDelayedCmd(slotId);
                break;
            case OP_CARD_STATUS_CHANGED:
                CatLog.d(LOG_TAG, "Card/Icc Status change received");
                handleCardStatusChangeAndIccRefresh((Bundle) msg.obj, slotId);
                break;
            case OP_SET_ACT_INST:
                Activity act = new Activity();
                act = (Activity) msg.obj;
                CatLog.d(LOG_TAG, "Set activity instance. " + act);
                mStkContext[slotId].mActivityInstance = act;
                break;
            case OP_SET_DAL_INST:
                Activity dal = new Activity();
                CatLog.d(LOG_TAG, "Set dialog instance. " + dal);
                dal = (Activity) msg.obj;
                mStkContext[slotId].mDialogInstance = dal;
                break;
            case OP_SET_MAINACT_INST:
                Activity mainAct = new Activity();
                mainAct = (Activity) msg.obj;
                CatLog.d(LOG_TAG, "Set activity instance. " + mainAct);
                mStkContext[slotId].mMainActivityInstance = mainAct;
                break;
            case OP_LOCALE_CHANGED:
                CatLog.d(this, "Locale Changed");
                for (int slot = PhoneConstants.SIM_ID_1; slot < mSimCount; slot++) {
                    checkForSetupEvent(LANGUAGE_SELECTION_EVENT, (Bundle) msg.obj, slot);
                }
                break;
            case OP_ALPHA_NOTIFY:
                handleAlphaNotify((Bundle) msg.obj);
                break;
            case OP_IDLE_SCREEN:
               for (int slot = 0; slot < mSimCount; slot++) {
                    if (mStkContext[slot] != null) {
                        handleIdleScreen(slot);
                    }
                }
                break;
            /* SPRD: Add for STK 27.22.7.5.1 @{ */
            case OP_USER_ACTIVITY:
                CatLog.d(this, "User Activity");
                checkForSetupEvent(USER_ACTIVITY_EVENT, (Bundle) msg.obj, slotId);
                break;
            /* @} */
            }
        }

        /* SPRD: Add here for REFRESH function @{ */
        private void launchRefreshMsg(int slotId) {
            if (mStkContext[slotId].mCurrentCmd == null) {
                CatLog.d(this, "[stkapp] launchRefreshMsg and mCurrentCmd is null");
                return;
            }

            /* SPRD: Add here for STK feature. @{ */
            if (StkRefreshPluginsHelper.getInstance(mContext).refreshNoToast()) {
                CatLog.d(this, "[stkapp] Don't need launchRefreshMsg for STK Feature");
                return;
            }
            /* @} */

            TextMessage msg = mStkContext[slotId].mCurrentCmd.geTextMessage();
            if (msg == null || msg.text == null || msg.text.length() == 0) {
                CatLog.d(this, "[stkapp] launchRefreshMsg is null");
                return;
            }
            if(mToast != null){
                mToast.cancel();
            }
            mToast = Toast.makeText(mContext.getApplicationContext(), msg.text,
                    Toast.LENGTH_LONG);
            mToast.setGravity(Gravity.BOTTOM, 0, 0);
            mToast.show();
        }
        /* @} */

        private void handleCardStatusChangeAndIccRefresh(Bundle args, int slotId) {
            boolean cardStatus = args.getBoolean(AppInterfaceSprd.CARD_STATUS);

            CatLog.d(LOG_TAG, "CardStatus: " + cardStatus);
            if (cardStatus == false) {
                CatLog.d(LOG_TAG, "CARD is ABSENT");
                // Uninstall STKAPP, Clear Idle text, Stop StkAppService
                mNotificationManager.cancel(getNotificationId(slotId));
                /* SPRD: add for cucc two icon feature . @{ */
                if(isCUCCOperator()){
                    StkAppInstaller.unInstall(mContext,slotId);
                }
                /* @} */
                /*SPRD: add for bug#611551 reset mStkService[slotId].
                 * Turn off/on sim card, StkService in FW will be recreate but App handle the old object*/
                mStkService[slotId] = null;
                CatLog.d("CAT", "mStkContext[slotId] = " + mStkContext[slotId]);
                mStkContext[slotId].reset();
                if (isAllOtherCardsAbsent(slotId)) {
                    CatLog.d(LOG_TAG, "All CARDs are ABSENT");
                    /* SPRD: add for cucc two icon feature . @{ */
                    /* SPRD: Due to the mIsCUCC @{ */
                    if(!isCUCCOperator()){
                        StkAppInstaller.unInstall(mContext);
                    }
                    /* @} */
                    stopSelf();
                }
            } else {
                IccRefreshResponse state = new IccRefreshResponse();
                state.refreshResult = args.getInt(AppInterfaceSprd.REFRESH_RESULT);

                CatLog.d(LOG_TAG, "Icc Refresh Result: "+ state.refreshResult);
                if ((state.refreshResult == IccRefreshResponse.REFRESH_RESULT_INIT) ||
                    (state.refreshResult == IccRefreshResponse.REFRESH_RESULT_RESET)) {
                    /* SPRD: Add here for REFRESH function @{ */
                    launchRefreshMsg(slotId);
                    /* @} */
                    // Clear Idle Text
                    mNotificationManager.cancel(getNotificationId(slotId));
                }

                if (state.refreshResult == IccRefreshResponse.REFRESH_RESULT_RESET) {
                    // Uninstall STkmenu
                    /* SPRD: add for cucc two icon feature . @{ */
                    if(isCUCCOperator()){
                        StkAppInstaller.unInstall(mContext,slotId);
                    }
                    /* @} */
                    if (isAllOtherCardsAbsent(slotId)) {
                        /* SPRD: add for cucc two icon feature . @{ */
                        if(!isCUCCOperator()){
                            StkAppInstaller.unInstall(mContext);
                        }
                        /* @} */
                    }
                    mStkContext[slotId].mCurrentMenu = null;
                    mStkContext[slotId].mMainCmd = null;
                }
            }
        }
    }
    /*
     * Check if all SIMs are absent except the id of slot equals "slotId".
     */
    private boolean isAllOtherCardsAbsent(int slotId) {
        TelephonyManager mTm = (TelephonyManager) mContext.getSystemService(
                Context.TELEPHONY_SERVICE);
        int i = 0;

        for (i = 0; i < mSimCount; i++) {
            if(!TelephonyManagerEx.from(mContext).isSimEnabled(i)){
                return false;
            }
            if (i != slotId && mTm.hasIccCard(i)) {
                break;
            }
        }
        if (i == mSimCount) {
            return true;
        } else {
            return false;
        }
    }

    /*
     * SPRD: Check if all SIMs are in the card slot.
     */
    private boolean isAllCardsExsit() {
        TelephonyManager mTm = (TelephonyManager) mContext.getSystemService(
                Context.TELEPHONY_SERVICE);
        int i = 0;
        int count = 0;

        for (i = 0; i < mSimCount; i++) {
            if (mTm.hasIccCard(i)) {
                count++;
            }
        }

        if (count > 1) {
            return true;
        } else {
            return false;
        }
    }

    /*
     * If the device is not in an interactive state, we can assume
     * that the screen is idle.
     */
    private boolean isScreenIdle() {
        return (!mPowerManager.isInteractive());
    }

    private void handleIdleScreen(int slotId) {

        // If the idle screen event is present in the list need to send the
        // response to SIM.
        CatLog.d(this, "Need to send IDLE SCREEN Available event to SIM");
        checkForSetupEvent(IDLE_SCREEN_AVAILABLE_EVENT, null, slotId);

        if (mStkContext[slotId].mIdleModeTextCmd != null) {
           launchIdleText(slotId);
        }
    }

    private void sendScreenBusyResponse(int slotId) {
        if (mStkContext[slotId].mCurrentCmd == null) {
            return;
        }
        CatResponseMessageSprd resMsg = new CatResponseMessageSprd(mStkContext[slotId].mCurrentCmd);
        CatLog.d(this, "SCREEN_BUSY");
        resMsg.setResultCode(ResultCodeSprd.TERMINAL_CRNTLY_UNABLE_TO_PROCESS);
        onCmdResponse(resMsg, slotId);
        if (mStkContext[slotId].mCmdsQ.size() != 0) {
            callDelayedMsg(slotId);
        } else {
            mStkContext[slotId].mCmdInProgress = false;
        }
    }

    private void sendResponse(int resId, int slotId, boolean confirm) {
        Message msg = mServiceHandler.obtainMessage();
        msg.arg1 = OP_RESPONSE;
        msg.arg2 = slotId;
        Bundle args = new Bundle();
        args.putInt(StkAppService.RES_ID, resId);
        args.putInt(SLOT_ID, slotId);
        args.putBoolean(StkAppService.CONFIRMATION, confirm);
        msg.obj = args;
        mServiceHandler.sendMessage(msg);
    }

    private boolean isCmdInteractive(CatCmdMessageSprd cmd) {
        switch (cmd.getCmdType()) {
        case SEND_DTMF:
        case SEND_SMS:
        case SEND_SS:
        case SEND_USSD:
        case SET_UP_IDLE_MODE_TEXT:
        case SET_UP_MENU:
        case CLOSE_CHANNEL:
        case RECEIVE_DATA:
        case SEND_DATA:
        case SET_UP_EVENT_LIST:
        case REFRESH:
            return false;
        }

        return true;
    }

    private void handleDelayedCmd(int slotId) {
        CatLog.d(LOG_TAG, "handleDelayedCmd, slotId: " + slotId);
        if (mStkContext[slotId].mCmdsQ.size() != 0) {
            DelayedCmd cmd = mStkContext[slotId].mCmdsQ.poll();
            if (cmd != null) {
                CatLog.d(LOG_TAG, "handleDelayedCmd - queue size: " +
                        mStkContext[slotId].mCmdsQ.size() +
                        " id: " + cmd.id + "sim id: " + cmd.slotId);
                switch (cmd.id) {
                case OP_CMD:
                    handleCmd(cmd.msg, cmd.slotId);
                    break;
                case OP_END_SESSION:
                    handleSessionEnd(cmd.slotId);
                    break;
                }
            }
        }
    }

    private void callDelayedMsg(int slotId) {
        Message msg = mServiceHandler.obtainMessage();
        msg.arg1 = OP_DELAYED_MSG;
        msg.arg2 = slotId;
        mServiceHandler.sendMessage(msg);
    }

    private void callSetActivityInstMsg(int inst_type, int slotId, Object obj) {
        Message msg = mServiceHandler.obtainMessage();
        msg.obj = obj;
        msg.arg1 = inst_type;
        msg.arg2 = slotId;
        mServiceHandler.sendMessage(msg);
    }

    private void handleSessionEnd(int slotId) {
        // We should finish all pending activity if receiving END SESSION command.
        cleanUpInstanceStackBySlot(slotId);

        mStkContext[slotId].mCurrentCmd = mStkContext[slotId].mMainCmd;
        CatLog.d(LOG_TAG, "[handleSessionEnd] - mCurrentCmd changed to mMainCmd!.");
        mStkContext[slotId].mCurrentMenuCmd = mStkContext[slotId].mMainCmd;
        CatLog.d(LOG_TAG, "slotId: " + slotId + ", mMenuState: " +
                mStkContext[slotId].mMenuState);

        mStkContext[slotId].mIsInputPending = false;
        mStkContext[slotId].mIsMenuPending = false;
        mStkContext[slotId].mIsDialogPending = false;

        if (mStkContext[slotId].mMainCmd == null) {
            CatLog.d(LOG_TAG, "[handleSessionEnd][mMainCmd is null!]");
        }
        mStkContext[slotId].lastSelectedItem = null;
        // In case of SET UP MENU command which removed the app, don't
        // update the current menu member.
        if (mStkContext[slotId].mCurrentMenu != null && mStkContext[slotId].mMainCmd != null) {
            mStkContext[slotId].mCurrentMenu = mStkContext[slotId].mMainCmd.getMenu();
        }
        CatLog.d(LOG_TAG, "[handleSessionEnd][mMenuState]" + mStkContext[slotId].mMenuIsVisible);
        // In mutiple instance architecture, the main menu for slotId will be finished when user
        // goes to the Stk menu of the other SIM. So, we should launch a new instance for the
        // main menu if the main menu instance has been finished.
        // If the current menu is secondary menu, we should launch main menu.
        if (StkMenuActivity.STATE_SECONDARY == mStkContext[slotId].mMenuState) {
            /* SPRD:Bug#490274: When receive session_end,return to mainmenu,if go into the stk not by user @{ */
            /* SPRD: add for cucc two icon feature . @{ */
            if (StkPluginsHelper.getInstance(mContext).needCloseMenu() && !mAccessByUser && !isCUCCOperator()) {
            /* @} */
                CatLog.d(this, "handleSessionEnd: mAccessByUser=" + mAccessByUser);
                closeMenuActivity(slotId);
            } else {
                launchMenuActivity(null, slotId);
            }
            /* @} */

        }
        if (mStkContext[slotId].mCmdsQ.size() != 0) {
            callDelayedMsg(slotId);
        } else {
            mStkContext[slotId].mCmdInProgress = false;
        }
    }

    // returns true if any Stk related activity already has focus on the screen
    private boolean isTopOfStack() {
        ActivityManager mAcivityManager = (ActivityManager) mContext
                .getSystemService(ACTIVITY_SERVICE);
        /* SPRD: Modify for running task is empty cause crash@{ */
        List<RunningTaskInfo> info=mAcivityManager.getRunningTasks(1);
        if( null != info && !(info.isEmpty())){
            String currentPackageName = info.get(0).topActivity.getPackageName();
            if (null != currentPackageName) {
                return currentPackageName.equals(PACKAGE_NAME);
            }
        } else {
            CatLog.d(LOG_TAG, "RunningTasks is empty");
        }
        /* @} */

        return false;
    }

    private void handleCmd(CatCmdMessageSprd cmdMsg, int slotId) {

        if (cmdMsg == null) {
            return;
        }
        // save local reference for state tracking.
        mStkContext[slotId].mCurrentCmd = cmdMsg;
        boolean waitForUsersResponse = true;

        mStkContext[slotId].mIsInputPending = false;
        mStkContext[slotId].mIsMenuPending = false;
        mStkContext[slotId].mIsDialogPending = false;

        CatLog.d(LOG_TAG,"[handleCmd]" + cmdMsg.getCmdType().name());
        switch (cmdMsg.getCmdType()) {
        case DISPLAY_TEXT:
            TextMessage msg = cmdMsg.geTextMessage();
            waitForUsersResponse = msg.responseNeeded;
            if (mStkContext[slotId].lastSelectedItem != null) {
                msg.title = mStkContext[slotId].lastSelectedItem;
            } else if (mStkContext[slotId].mMainCmd != null){
                msg.title = mStkContext[slotId].mMainCmd.getMenu().title;
            } else {
                // TODO: get the carrier name from the SIM
                msg.title = "";
            }
            //If we receive a low priority Display Text and the device is
            // not displaying any STK related activity and the screen is not idle
            // ( that is, device is in an interactive state), then send a screen busy
            // terminal response. Otherwise display the message. The existing
            // displayed message shall be updated with the new display text
            // proactive command (Refer to ETSI TS 102 384 section 27.22.4.1.4.4.2).
            if (!(msg.isHighPriority || mStkContext[slotId].mMenuIsVisible
                    || mStkContext[slotId].mDisplayTextDlgIsVisibile || isTopOfStack())) {
                /* SPRD: Modify for show normal priority DISPLAY TEXT  and show IDLE MODE TEXT@{ */
                if(isBusyOnCall()) {
                /* @} */
                    CatLog.d(LOG_TAG, "Screen is not idle");
                    sendScreenBusyResponse(slotId);
                } else {
                    launchTextDialog(slotId);
                }
            } else {
                launchTextDialog(slotId);
            }
            break;
        case SELECT_ITEM:
            CatLog.d(LOG_TAG, "SELECT_ITEM +");
            mStkContext[slotId].mCurrentMenuCmd = mStkContext[slotId].mCurrentCmd;
            mStkContext[slotId].mCurrentMenu = cmdMsg.getMenu();
            launchMenuActivity(cmdMsg.getMenu(), slotId);
            break;
        case SET_UP_MENU:
            mStkContext[slotId].mCmdInProgress = false;
            mStkContext[slotId].mMainCmd = mStkContext[slotId].mCurrentCmd;
            mStkContext[slotId].mCurrentMenuCmd = mStkContext[slotId].mCurrentCmd;
            mStkContext[slotId].mCurrentMenu = cmdMsg.getMenu();
            CatLog.d(LOG_TAG, "SET_UP_MENU [" + removeMenu(slotId) + "]");

            if (removeMenu(slotId)) {
                int i = 0;
                CatLog.d(LOG_TAG, "removeMenu() - Uninstall App");
                mStkContext[slotId].mCurrentMenu = null;
                mStkContext[slotId].mMainCmd = null;
                /* SPRD: add for cucc two icon feature . @{ */
                if(isCUCCOperator()){
                    StkAppInstaller.unInstall(mContext,slotId);
                }
                /* @} */
                //Check other setup menu state. If all setup menu are removed, uninstall apk.
                for (i = PhoneConstants.SIM_ID_1; i < mSimCount; i++) {
                    if (i != slotId
                            && (mStkContext[slotId].mSetupMenuState == STATE_UNKNOWN
                            || mStkContext[slotId].mSetupMenuState == STATE_EXIST)) {
                        CatLog.d(LOG_TAG, "Not Uninstall App:" + i + ","
                                + mStkContext[slotId].mSetupMenuState);
                        break;
                    }
                }
                if (i == mSimCount) {
                    /* SPRD: add for cucc two icon feature . @{ */
                    if(!isCUCCOperator()){
                        CatLog.d(LOG_TAG, "unstall App");
                        StkAppInstaller.unInstall(mContext);
                    }
                    /* @} */
                }
            } else {
                CatLog.d(LOG_TAG, "install App");
                /* SPRD: add for cucc two icon feature . @{ */
                //StkAppInstaller.install(mContext);
                boolean isAirPlaneModeOn = Settings.Global.getInt(mContext.getContentResolver(),
                        Settings.Global.AIRPLANE_MODE_ON, 0) != 0;
                if(!isAirPlaneModeOn){
                    if(isCUCCOperator()){
                       if (isCardReady(mContext, slotId)) {
                           StkAppInstaller.install(mContext, slotId);
                       }
                    }else{
                       StkAppInstaller.install(mContext);
                    }
                }
                /* @} */
            }
            if (mStkContext[slotId].mMenuIsVisible) {
                launchMenuActivity(null, slotId);
            }
            break;
        case GET_INPUT:
        case GET_INKEY:
            launchInputActivity(slotId);
            break;
        case SET_UP_IDLE_MODE_TEXT:
            waitForUsersResponse = false;
            mStkContext[slotId].mIdleModeTextCmd = mStkContext[slotId].mCurrentCmd;
            TextMessage idleModeText = mStkContext[slotId].mCurrentCmd.geTextMessage();
            if (idleModeText == null) {
                launchIdleText(slotId);
                mStkContext[slotId].mIdleModeTextCmd = null;
            }
            mStkContext[slotId].mCurrentCmd = mStkContext[slotId].mMainCmd;
            /* SPRD: Modify for show normal priority DISPLAY TEXT  and show IDLE MODE TEXT @{ */
            if ((mStkContext[slotId].mIdleModeTextCmd != null) && /*isScreenIdle()*/!isBusyOnCall()) {
            /* @} */
                CatLog.d(this, "set up idle mode");
                launchIdleText(slotId);
            }
            break;
        case SEND_DTMF:
        case SEND_SMS:
        // SPRD : delete for call forward Bug#64554
        //case SEND_SS:
        case SEND_USSD:
            waitForUsersResponse = false;
            launchEventMessage(slotId);
            break;
        /* SPRD: add for query call forward when send ss, meger from Bug#64554.@{ */
        case SEND_SS:
            waitForUsersResponse = false;
            launchEventMessage(slotId);
            mPhone.getCallForwardingOption(CommandsInterface.CF_REASON_UNCONDITIONAL,null);
            break;
        /* @} */
        case GET_CHANNEL_STATUS:
            /* SPRD: Modify here for BIP function @{ */
            //waitForUsersResponse = false;
            //launchEventMessage(slotId);
            launchChannelStatus(slotId);
            /* @} */
            break;
        case LAUNCH_BROWSER:
            TextMessage alphaId = mStkContext[slotId].mCurrentCmd.geTextMessage();
            if ((mStkContext[slotId].mCurrentCmd.getBrowserSettings().mode
                    == LaunchBrowserMode.LAUNCH_IF_NOT_ALREADY_LAUNCHED) &&
                    ((alphaId == null) || TextUtils.isEmpty(alphaId.text))) {
                // don't need user confirmation in this case
                // just launch the browser or spawn a new tab
                CatLog.d(this, "Browser mode is: launch if not already launched " +
                        "and user confirmation is not currently needed.\n" +
                        "supressing confirmation dialogue and confirming silently...");
                mStkContext[slotId].launchBrowser = true;
                mStkContext[slotId].mBrowserSettings =
                        mStkContext[slotId].mCurrentCmd.getBrowserSettings();
                sendResponse(RES_ID_CONFIRM, slotId, true);
            } else {
                launchConfirmationDialog(alphaId, slotId);
            }
            break;
        case SET_UP_CALL:
            processSetupCall(slotId);
            break;
        case PLAY_TONE:
            launchToneDialog(slotId);
            break;
        case OPEN_CHANNEL:
            /* SPRD: Modify here for BIP function @{ */
            //launchOpenChannelDialog(slotId);
            processOpenChannel(slotId);
            /* @} */
            break;
        /* SPRD: Modify here for BIP function @{ */
        case CLOSE_CHANNEL:
            launchCloseChannel(slotId);
            break;
        case RECEIVE_DATA:
            launchReceiveData(slotId);
            break;
        case SEND_DATA:
            /**
             * @orig
             * SPRD: Remove here for BIP function @{
            TextMessage m = mStkContext[slotId].mCurrentCmd.geTextMessage();

            if ((m != null) && (m.text == null)) {
                switch(cmdMsg.getCmdType()) {
                case CLOSE_CHANNEL:
                    m.text = getResources().getString(R.string.default_close_channel_msg);
                    break;
                case RECEIVE_DATA:
                    m.text = getResources().getString(R.string.default_receive_data_msg);
                    break;
                case SEND_DATA:
                    m.text = getResources().getString(R.string.default_send_data_msg);
                    break;
                }
            }
            @} */
            /*
             * Display indication in the form of a toast to the user if required.
             */
            /**
             * @orig
             * SPRD: Remove here for BIP function @{
            launchEventMessage(slotId);
            @} */
            launchSendData(cmdMsg.getCmdDet(), slotId);
            break;
        /* @} */
        case SET_UP_EVENT_LIST:
            mStkContext[slotId].mSetupEventListSettings =
                    mStkContext[slotId].mCurrentCmd.getSetEventList();
            mStkContext[slotId].mCurrentSetupEventCmd = mStkContext[slotId].mCurrentCmd;
            mStkContext[slotId].mCurrentCmd = mStkContext[slotId].mMainCmd;
            if (isScreenIdle()) {
                CatLog.d(this," Check if IDLE_SCREEN_AVAILABLE_EVENT is present in List");
                checkForSetupEvent(IDLE_SCREEN_AVAILABLE_EVENT, null, slotId);
            }
            break;
        }

        if (!waitForUsersResponse) {
            if (mStkContext[slotId].mCmdsQ.size() != 0) {
                callDelayedMsg(slotId);
            } else {
                mStkContext[slotId].mCmdInProgress = false;
            }
        }
    }

    /* SPRD: Add here for Open Channel function @ { */
    private void sendEventChannelStatus(int slotId) {
        CatLog.d(this, "sendEventChannelStatus");
        if ((mStkContext[slotId].mLastChannelStatus.mChannelId == mStkContext[slotId].mChannelStatus.mChannelId)
                && (mStkContext[slotId].mLastChannelStatus.mLinkStatus == mStkContext[slotId].mChannelStatus.mLinkStatus)
                && (mStkContext[slotId].mLastChannelStatus.mode_info == mStkContext[slotId].mChannelStatus.mode_info)) {
            return;
        }
        mStkContext[slotId].mLastChannelStatus.SetStatus(
                mStkContext[slotId].mChannelStatus.mChannelId,
                mStkContext[slotId].mChannelStatus.mLinkStatus,
                mStkContext[slotId].mChannelStatus.mode_info);
        sendSetUpEventResponse(CHANNEL_STATUS_EVENT, null, slotId);
    }

    private void handleDataState(Intent intent, int slotId) {
        CatLog.d(this, " handleDataState mConnectType = " + mStkContext[slotId].mConnectType);
        if (mStkContext[slotId].mConnectType == -1) {
            CatLog.d(this, "unkonwn type,ignore this action");
            return;
        }

        NetworkInfo ni = (NetworkInfo) intent
                .getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);
        if (ni != null) {
            CatLog.d(this, "handleDataState ni.getType: " + ni.getType());
            CatLog.d(this, "handleDataState ni.getState: " + ni.getState());
        }

        if (mStkContext[slotId].mConnectType == ConnectivityManager.TYPE_MOBILE_SUPL) {
            if (ni != null && ni.getType() == ConnectivityManager.TYPE_MOBILE_SUPL) {
                CatLog.d(this, "handleDataState ConnectivityManager.TYPE_MOBILE_SUPL");
                if (ni.getState() == NetworkInfo.State.CONNECTED) {
                    CatLog.d(this, "handleDataState connected");
                    synchronized (mStkContext[slotId].mConnectLock) {
                        try {
                            CatLog.d(this, "notify mConnectLock");
                            if (mStkContext[slotId].mNetwork != null) {
                                mStkContext[slotId].mConnectLock.notifyAll();
                            }
                        } catch (Exception e) {
                            CatLog.d(this, e.toString());
                        }
                    }
                } else {
                    CatLog.d(this, "handleDataState disconnected mChannelEstablished = "
                            + mStkContext[slotId].mChannelEstablished);
                    if (mStkContext[slotId].mChannelEstablished) {
                        mStkContext[slotId].mChannelStatus.SetStatus(AppInterfaceSprd.DEFAULT_CHANNELID, false,
                                CHANNEL_MODE_LINK_DROPPED);
                        sendEventChannelStatus(slotId);
                    }
                }
            }
        } else if (mStkContext[slotId].mConnectType == ConnectivityManager.TYPE_MOBILE) {
            if (ni != null && ni.getType() == ConnectivityManager.TYPE_MOBILE) {
                CatLog.d(this, "handleDataState TYPE_MOBILE");
                if (ni.getState() == NetworkInfo.State.CONNECTED) {
                    CatLog.d(this, "handleDataState connected");
                } else {
                    CatLog.d(this, "handleDataState disconnected mChannelEstablished = "
                            + mStkContext[slotId].mChannelEstablished);
                    if (mStkContext[slotId].mChannelEstablished) {
                        mStkContext[slotId].mChannelStatus.SetStatus(AppInterfaceSprd.DEFAULT_CHANNELID, false,
                                CHANNEL_MODE_LINK_DROPPED);
                        sendEventChannelStatus(slotId);
                    }
                }
            }
        }
    }

    private void setOpenChannelApn(OpenChannelData channeldata, int slotId) {
        CatLog.d(this, "[stkapp] setOpenChannelApn ");

        ContentResolver resolver = getContentResolver();
        ContentValues values = new ContentValues();

        String numeric = TelephonyManager.from(mContext).getSimOperatorNumericForPhone(slotId);
        CatLog.d(this, "[stkapp] numeric = " + numeric);
        if (numeric == null || numeric.length() < 5) {
            CatLog.d(this, "[stkapp] operator numeric error");
            return;
        }

        String mcc = numeric.substring(0, 3);
        String mnc = numeric.substring(3);

        values.put(Telephony.Carriers.NAME, channeldata.BearerParam);
        values.put(Telephony.Carriers.APN, channeldata.NetAccessName);
        values.put(Telephony.Carriers.NUMERIC, numeric);
        values.put(Telephony.Carriers.MCC, mcc);
        values.put(Telephony.Carriers.MNC, mnc);
        values.put(Telephony.Carriers.USER, channeldata.LoginStr);
        values.put(Telephony.Carriers.PASSWORD, channeldata.PwdStr);
        values.put(Telephony.Carriers.TYPE, TYPE_STK);
        values.put(Telephony.Carriers.SUBSCRIPTION_ID, SubscriptionController.getInstance().getSubIdUsingPhoneId(slotId));
        values.put(Telephony.Carriers.PROFILE_ID, 255);
        /* SPRD: Modify the bear type @{ */
        if (channeldata.BearerParam != null) {
            int len = channeldata.BearerParam.length();
            if (TYPE_PACKAGE_DATA_PROTOCOL_IP.equals(channeldata.BearerParam.substring(len - 2, len))) {
                values.put(Telephony.Carriers.PROTOCOL, "IP");
                values.put(Telephony.Carriers.ROAMING_PROTOCOL, "IP");
            }
        }
        /* @} */

        CatLog.d(this, " BearerType = " + channeldata.BearerType);
        int delNum = resolver.delete(Telephony.Carriers.CONTENT_URI,
                Telephony.Carriers.TYPE + "=?", new String[] {
                    TYPE_STK
                });
        CatLog.d(this, " BEARER_TYPE_DEFAULT :delNum = " + delNum);
        CatLog.d(this, " BEARER_TYPE_DEFAULT :APN = " + channeldata.NetAccessName);
        mStkContext[slotId].mUri = resolver.insert(Telephony.Carriers.CONTENT_URI, values);
        if (mStkContext[slotId].mUri == null) {
            CatLog.d(this, "[stkapp]  insert failed ");
            return;
        } else {
            CatLog.d(this, "[stkapp] mUri = " + mStkContext[slotId].mUri);
            String apn_id = mStkContext[slotId].mUri.getLastPathSegment();
            CatLog.d(this, "[stkapp] apn_id = " + apn_id);
            mStkContext[slotId].mCurPos = apn_id;
        }

        values.put(APN_ID, mStkContext[slotId].mCurPos);
    }

    private int createSocket(int slotId) {
        byte type = mStkContext[slotId].mOpenChannelData.transportType;
        int port = mStkContext[slotId].mOpenChannelData.portNumber;
        String address = mStkContext[slotId].mOpenChannelData.DataDstAddress;
        CatLog.d(this, "[stkapp]  createSocket ");

        if (type == OpenChannelData.TRANSPORT_TYPE_TCP) {
            CatLog.d(this, "[stkapp]  create tcp socket ");
            InetSocketAddress scktAddress = new InetSocketAddress(address, port);
            mStkContext[slotId].mTcpSocket = new Socket();
            if (mStkContext[slotId].mTcpSocket != null) {
                CatLog.d(this, "[stkapp] mTcpSocket != null");
                try {
                    CatLog.d(this, "[stkapp]  connect ");
                    mStkContext[slotId].mTcpSocket.connect(scktAddress, 8000);
                } catch (Throwable e) {
                    e.printStackTrace();
                    CatLog.d(this, "[stkapp]  connection problem ");
                    return SOCKET_FAIL;
                }
            } else {
                return SOCKET_FAIL;
            }
        } else if (type == OpenChannelData.TRANSPORT_TYPE_UDP) {
            CatLog.d(this, "[stkapp]  create udp socket ");
            try {
                mStkContext[slotId].mUdpSocket = new DatagramSocket();
                if (mStkContext[slotId].mUdpSocket != null) {
                    CatLog.d(this, "[stkapp] mUdpSocket != null");
                } else {
                    return SOCKET_FAIL;
                }
            } catch (SocketException e) {
                CatLog.d(this, "[stkapp] SocketException e = " + e);
                return SOCKET_FAIL;
            }
        }
        mNeedRuning = true;
        return SOCKET_OK;
    }

    private void onCreateSocket(int slotId) {
        CatLog.d(this, "[stkapp]  onCreateSocket");
        CatLog.d(this, "openchannel success buf size = "
                + mStkContext[slotId].mOpenChannelData.bufferSize);
        mStkContext[slotId].mChannelStatus.SetStatus(AppInterfaceSprd.DEFAULT_CHANNELID, true);
        mStkContext[slotId].mLastChannelStatus.SetStatus(
                mStkContext[slotId].mLastChannelStatus.mChannelId, true);
        if (createSocket(slotId) == SOCKET_OK) {
            CatLog.d(this, "[stkapp]  onCreateSocket  success");
            if (mStkContext[slotId].mOpenChannelData.bufferSize > DEFAULT_BUFFER_SIZE) {
                mStkContext[slotId].mOpenChannelData.bufferSize = DEFAULT_BUFFER_SIZE;
                SendChannelResponse(mStkContext[slotId].mOpenChannelData,
                        ResultCodeSprd.PRFRMD_WITH_MODIFICATION, slotId);
            } else {
                SendChannelResponse(mStkContext[slotId].mOpenChannelData, ResultCodeSprd.OK, slotId);
                //android.os.SystemProperties.set("gsm.stk.default_bearer" + slotId, "0");
            }
            mStkContext[slotId].mChannelEstablished = true;
        } else {
            CatLog.d(this, "[stkapp]  onCreateSocket  fail");
            mStkContext[slotId].mChannelStatus.SetStatus(0, false);
            SendChannelResponse(mStkContext[slotId].mOpenChannelData,
                    ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY, slotId);
            mStkContext[slotId].mChannelEstablished = false;
            if (mStkContext[slotId].mReceiver != null) {
                unregisterReceiver(mStkContext[slotId].mReceiver);
                mStkContext[slotId].mReceiver = null;
            }
        }
    }

    private class CreateCommonThread extends Thread {
        private int slotId;
        private int typeId;

        private CreateCommonThread(int sId, int tId) {
            slotId = sId;
            typeId = tId;
        }
        public void run() {
            switch (typeId) {
                case CREATE_SOCKET:
                    CatLog.d(this, "CreateSocket run....");
                    onCreateSocket(slotId);
                    break;
                case LAUNCH_OPEN_CHANNEL:
                    openChannel(slotId);
                    break;
                default:
                    break;
            }
        }
    }

    private Handler mDataHandler = new Handler() {
        public void handleMessage(Message msg) {
            int slotId = msg.arg1;
            switch (msg.what) {
                case EVENT_CREATE_SOCKET:
                    new CreateCommonThread(slotId, CREATE_SOCKET).start();
                    break;
                case EVENT_TOAST_SHOW:
                    if (mStkContext[slotId].mOpenChannelData.text == null
                            && !mStkContext[slotId].mOpenChannelData.isNullAlphaId) {
                        CatLog.d(this, "openChanne show default msg");
                        Toast toast = Toast.makeText(mContext.getApplicationContext(),
                                getString(R.string.channel_connecting), Toast.LENGTH_LONG);
                        toast.show();
                    }
                    break;
                default:
                    break;
            }
        }
    };
    private class StkAppBroadcastReceiver extends BroadcastReceiver {
        private int mSlotId;

        StkAppBroadcastReceiver(int sId) {
            mSlotId = sId;
        }
        public void onReceive(Context context, Intent intent) {
            CatLog.d(this, " mReceiver ");
            String action = intent.getAction();
            if (action.equals(ConnectivityManager.CONNECTIVITY_ACTION_SUPL)) {
                handleDataState(intent, mSlotId);
            }
        }
    }

    private void ensureRouteToHost(int slotId) throws IOException {
        CatLog.d(this,"ensureRouteToHost");
        String dstAddress = mStkContext[slotId].mOpenChannelData.DataDstAddress;
        CatLog.d(this, "ensureRouteToHost dstAddress: " + dstAddress);

        try {
            mStkContext[slotId].mInetAddress = InetAddress.getByName(dstAddress);
            CatLog.d(this, "IP address converted to: " + mStkContext[slotId].mInetAddress);
        } catch (UnknownHostException e) {
            CatLog.e(this, "Bad IP Address and e: " + e);
            mStkContext[slotId].mInetAddress = null;
        }

        CatLog.d(this, "ensureRouteToHost mInet: " + mStkContext[slotId].mInetAddress);

        if (mStkContext[slotId].mInetAddress == null) {
            throw new IOException("Cannot establish route for " + dstAddress + ": Unknown host");
        } else {
            if (!mStkContext[slotId].mConnMgr.requestRouteToHostAddress(ConnectivityManager.TYPE_MOBILE_SUPL,
                    mStkContext[slotId].mInetAddress)) {
                throw new IOException("Cannot establish route to proxy "
                        + mStkContext[slotId].mInetAddress);
            }
        }
    }

    private void SendResponseAndEndConnectivity(int slotId) {
        SendChannelResponse(mStkContext[slotId].mOpenChannelData,
                ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY, slotId);
        if (mStkContext[slotId].mReceiver != null) {
            unregisterReceiver(mStkContext[slotId].mReceiver);
            mStkContext[slotId].mReceiver = null;
        }
        endConnectivity(slotId);
    }

    private void newRequest(int slotId) {
        final int sId = SubscriptionController.getInstance().getSubIdUsingPhoneId(slotId);
        final int id = slotId;
        mStkContext[slotId].mNetworkRequest = new NetworkRequest.Builder()
                .addTransportType(NetworkCapabilities.TRANSPORT_CELLULAR)
                .addCapability(NetworkCapabilities.NET_CAPABILITY_SUPL)
                .setNetworkSpecifier(Integer.toString(sId)).build();
        mStkContext[slotId].mNetworkCallback = new ConnectivityManager.NetworkCallback() {
            public void onAvailable(Network network) {
                super.onAvailable(network);
                CatLog.d(this, "stk NetworkCallbackListener.onAvailable: network=" + network);
                synchronized (mStkContext[id].mConnectLock) {
                    mStkContext[id].mNetwork = network;
                    if (mStkContext[id].mConnMgr != null
                            && mStkContext[id].mConnMgr.getNetworkInfo(ConnectivityManager.TYPE_MOBILE_SUPL)
                                    .isConnected()) {
                        mStkContext[id].mConnectLock.notifyAll();
                    }
                }
            }
            public void onLost(Network network) {
                super.onLost(network);
                CatLog.d(this, "stk NetworkCallbackListener.onLost: network=" + network);
                synchronized (mStkContext[id].mConnectLock) {
                    endConnectivity(id);
                }
            }
            public void onUnavailable() {
                super.onUnavailable();
                CatLog.d(this, "stk NetworkCallbackListener.onUnavailable");
                synchronized (mStkContext[id].mConnectLock) {
                    endConnectivity(id);
                }
            }
        };

        //mStkContext[slotId].mConnMgr.requestNetwork(mStkContext[slotId].mNetworkRequest,
        //        mStkContext[slotId].mNetworkCallback, 30 * 1000);
        mStkContext[slotId].mConnMgr.requestNetwork(mStkContext[slotId].mNetworkRequest,
                mStkContext[slotId].mNetworkCallback, 60 * 1000);
    }

    private void SetupStkConnect(int slotId){
        CatLog.d(this, "[stkapp] stk SetupStkConnect");

        if (mStkContext[slotId].mNetwork != null) {
            Message msg = mDataHandler.obtainMessage(EVENT_CREATE_SOCKET);
            msg.arg1 = slotId;
            mDataHandler.sendMessage(msg);
            CatLog.d(this, "[stkapp] stk mNetwork Already available");
            return;
        }
        CatLog.d(this, "[stkapp] stk start new network request");
        newRequest(slotId);
        synchronized (mStkContext[slotId].mConnectLock) {
            try {
                //mStkContext[slotId].mConnectLock.wait(30 * 1000);
                mStkContext[slotId].mConnectLock.wait(60 * 1000);
                CatLog.d(this, "[stkapp] stk mConnectLock got the notify ");

                if (mStkContext[slotId].mNetwork != null
                        && mStkContext[slotId].mConnMgr != null
                        && mStkContext[slotId].mConnMgr.getNetworkInfo(
                                ConnectivityManager.TYPE_MOBILE_SUPL).isConnected()) {
                    CatLog.d(this, "[stkapp] stk TYPE_MOBILE_SUPL CONNECT");
                    boolean isBindSuccess = mStkContext[slotId].mConnMgr
                            .bindProcessToNetwork(mStkContext[slotId].mNetwork);
                    CatLog.d(this, "[stkapp] bindProcessToNetwork :isBindSuccess=" + isBindSuccess);
                    Message msg = mDataHandler.obtainMessage(EVENT_CREATE_SOCKET);
                    msg.arg1 = slotId;
                    mDataHandler.sendMessage(msg);
                } else {
                    CatLog.d(this, "[stkapp] stk TYPE_MOBILE_SUPL DISCONNECT");
                    SendResponseAndEndConnectivity(slotId);
                }

            } catch (InterruptedException e) {
                CatLog.d(this, "[stkapp] stk InterruptedException");
                SendResponseAndEndConnectivity(slotId);
            }
        }
    }

    private void openChannel(int slotId) {
        CatLog.d(this, "openChanne slotId =" + slotId);
        // show default connecting msg
        Message msg = mDataHandler.obtainMessage(EVENT_TOAST_SHOW);
        msg.arg1 = slotId;
        mDataHandler.sendMessage(msg);

        mStkContext[slotId].mChannelEstablished = false;
        if (!phoneIsIdle()) {
            CatLog.d(this, "openchannel busy on call");
            SendChannelResponse(mStkContext[slotId].mOpenChannelData,
                    ResultCodeSprd.TERMINAL_CRNTLY_UNABLE_TO_PROCESS, slotId);
            return;
        }
        if ((mStkContext[slotId].mOpenChannelData.BearerType != OpenChannelData.BEARER_TYPE_GPRS)
                && (mStkContext[slotId].mOpenChannelData.BearerType != OpenChannelData.BEARER_TYPE_DEFAULT)
                && (mStkContext[slotId].mOpenChannelData.BearerType != OpenChannelData.BEARER_TYPE_UTRAN_PACKET_SERVICE)) {
            CatLog.d(this, "openchannel BearerType error");
            SendChannelResponse(mStkContext[slotId].mOpenChannelData,
                    ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY, slotId);
            return;
        }
        if (mStkContext[slotId].mOpenChannelData.DataDstAddressType != OpenChannelData.ADDRESS_TYPE_IPV4) {
            CatLog.d(this, "openchannel DataDstAddressType error");
            SendChannelResponse(mStkContext[slotId].mOpenChannelData,
                    ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY, slotId);
            return;
        }
        //android.os.SystemProperties.set("gsm.stk.open_channel" + slotId, "1");
        if (mStkContext[slotId].mReceiver == null) {
            mStkContext[slotId].mReceiver = new StkAppBroadcastReceiver(slotId);
        }
        // Do datacall here
        if (mStkContext[slotId].mReceiver != null) {
            registerReceiver(mStkContext[slotId].mReceiver, new IntentFilter(
                    ConnectivityManager.CONNECTIVITY_ACTION_SUPL));
            if (mStkContext[slotId].mOpenChannelData.NetAccessName != null) {
                CatLog.d(this, "[stkapp] NetAccessName =  "
                        + mStkContext[slotId].mOpenChannelData.NetAccessName);
                mStkContext[slotId].mConnectType = ConnectivityManager.TYPE_MOBILE_SUPL;
                setOpenChannelApn(mStkContext[slotId].mOpenChannelData, slotId);
                CatLog.d(this, "[stkapp] start wait 1000ms ");
                SystemClock.sleep(1000);
                CatLog.d(this, "[stkapp] wait done ");
                SetupStkConnect(slotId);
            } else {
                CatLog.d(this, "[stkapp] no NetAccessName");
                if (mStkContext[slotId].mConnMgr.getNetworkInfo(
                        ConnectivityManager.TYPE_MOBILE_SUPL).isConnected()) {
                    CatLog.d(this, "[stkapp] TelephonyManager.DATA_CONNECTED");
                    mStkContext[slotId].mConnectType = ConnectivityManager.TYPE_MOBILE;
                    msg = mDataHandler.obtainMessage(EVENT_CREATE_SOCKET);
                    msg.arg1 = slotId;
                    mDataHandler.sendMessage(msg);
                } else {
                    CatLog.d(this, "[stkapp] setup stk APN data link ");
                    mStkContext[slotId].mConnectType = ConnectivityManager.TYPE_MOBILE_SUPL;
                    SetupStkConnect(slotId);
                }
            }
        } else {
            CatLog.d(this, " mReceiver == null, so can't listen data connection state ");
            SendChannelResponse(mStkContext[slotId].mOpenChannelData,
                    ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY, slotId);
        }
        return;
    }

    private String getDefaultBearerNetAccessName(int slotId) {
        RadioInteractor radioInteractor = new RadioInteractor(this);
        return radioInteractor.getDefaultNetworkAccessName(slotId);
    }

    private void launchOpenChannel(int slotId) {
        new CreateCommonThread(slotId, LAUNCH_OPEN_CHANNEL).start();
    }

    private void printOpenChannelInfo(int slotId) {
        CatLog.d(this, "Alpha id: " + mStkContext[slotId].mOpenChannelData.text
                + ", isNullAlphaId: " + mStkContext[slotId].mOpenChannelData.isNullAlphaId
                + ", NetAccessName: " + mStkContext[slotId].mOpenChannelData.NetAccessName
                + ", bufferSize: " + mStkContext[slotId].mOpenChannelData.bufferSize
                + ", BearerType: " + mStkContext[slotId].mOpenChannelData.BearerType + "\n"
                + ", BearerParam: " + mStkContext[slotId].mOpenChannelData.BearerParam
                + ", LocalAddressType: " + mStkContext[slotId].mOpenChannelData.OtherAddressType
                + ", LocalAddress: " + mStkContext[slotId].mOpenChannelData.OtherAddress
                + ", LoginStr: " + mStkContext[slotId].mOpenChannelData.LoginStr + "\n"
                + ", PwdStr: " + mStkContext[slotId].mOpenChannelData.PwdStr + ", transportType: "
                + mStkContext[slotId].mOpenChannelData.transportType + ", portNumber: "
                + mStkContext[slotId].mOpenChannelData.portNumber + ", DataDstAddressType: "
                + mStkContext[slotId].mOpenChannelData.DataDstAddressType + "\n"
                + ", DataDstAddress: " + mStkContext[slotId].mOpenChannelData.DataDstAddress);
    }
    private void processOpenChannel(int slotId) {
        mStkContext[slotId].mOpenChannelData = mStkContext[slotId].mCurrentCmd.getOpenChannelData();
        CatLog.d(this, "mOpenChannelData.BearerType=" + mStkContext[slotId].mOpenChannelData.BearerType);
        if (mStkContext[slotId].mOpenChannelData.BearerType == OpenChannelData.BEARER_TYPE_DEFAULT) {
            //android.os.SystemProperties.set("gsm.stk.default_bearer" + slotId, "1");
            //String val = android.os.SystemProperties.get("gsm.stk.default_bearer" + slotId, "fault");
            //CatLog.d(this, "mOpenChannelData.BearerType=3, SystemProperties value=" + val);
            String netAccessName = getDefaultBearerNetAccessName(slotId);
            CatLog.d(this, "processOpenChannel netAccessName: " + netAccessName);
            if (!TextUtils.isEmpty(mStkContext[slotId].mOpenChannelData.NetAccessName)) {
                mStkContext[slotId].mOpenChannelData.NetAccessName = mStkContext[slotId].mOpenChannelData.NetAccessName;
            } else if (!TextUtils.isEmpty(netAccessName)) {
                mStkContext[slotId].mOpenChannelData.NetAccessName = netAccessName;
            } else {
                mStkContext[slotId].mOpenChannelData.NetAccessName = "null";
            }

        } else {
            //android.os.SystemProperties.set("gsm.stk.default_bearer" + slotId, "0");
        }
        if (mStkContext[slotId].mOpenChannelData.text == null) {
            launchOpenChannel(slotId);
        } else {
            // Display user confirm dialog
            //isSendSS = false;//temporary remove
            TextMessage channelMsg = TeleFrameworkFactory.getInstance().createTextMessage();
            channelMsg.text = mStkContext[slotId].mOpenChannelData.text;
            launchConfirmationDialog(channelMsg, slotId);
        }
    }

    public boolean isIdleBySubId(int subId) {
        final Phone phone = getPhone(subId);
        if (phone != null) {
            return (phone.getState() == PhoneConstants.State.IDLE);
        } else {
            return false;
        }
    }

    private boolean phoneIsIdle() {
        boolean isIdle = true;
        for (int i = 0; i < mSimCount; i++) {
            isIdle = isIdleBySubId(i);
            if (false == isIdle) {
                return isIdle;
            }
        }
        return isIdle;
    }

    private void closeSocket(int slotId) {
        CatLog.d(this, "closeSocket");
        if (mStkContext[slotId].mTcpSocket != null) {
            try {
                if (!mStkContext[slotId].mTcpSocket.isClosed()) {
                    mStkContext[slotId].mTcpSocket.close();
                }
            } catch (Exception e) {
                e.printStackTrace();
                CatLog.d(this, "[stkapp] TCP Exception ");
            } finally {
                mStkContext[slotId].mTcpSocket = null;
            }
        }
        if (mStkContext[slotId].mUdpSocket != null) {
            try {
                if (!mStkContext[slotId].mUdpSocket.isClosed()) {
                    mStkContext[slotId].mUdpSocket.close();
                }
            } catch (Exception e) {
                e.printStackTrace();
                CatLog.d(this, "[stkapp] UDP Exception ");
            } finally {
                mStkContext[slotId].mUdpSocket = null;
            }
        }
    }

    private void launchCloseChannel(int slotId) {
        CatLog.d(this, "launchCloseChannel");
        mNeedRuning = false;
        //android.os.SystemProperties.set("gsm.stk.open_channel" + slotId, "0");
        synchronized (mTcpUdpLock) {
            closeSocket(slotId);

            synchronized (mReceiveDataLock) {
                mStkContext[slotId].mReceiveDataLen = 0;
                mStkContext[slotId].mReceiveDataOffset = 0;
                mStkContext[slotId].mSendDataLen = 0;
                mStkContext[slotId].mRecevieData = null;
            }
            if (mStkContext[slotId].mOpenChannelData != null) {
                if (mStkContext[slotId].mConnectType == ConnectivityManager.TYPE_MOBILE_SUPL) {
                    mStkContext[slotId].mConnectType = -1;
                    SystemClock.sleep(2000);
                    endConnectivity(slotId);
                }
                mStkContext[slotId].mConnectType = -1;
                deleteOpenChannelApn(slotId);
                if (mStkContext[slotId].mTcpSocket != null) {
                    try {
                        if (!mStkContext[slotId].mTcpSocket.isClosed()) {
                            mStkContext[slotId].mTcpSocket.close();
                        }
                    } catch (Exception e) {
                        e.printStackTrace();
                        CatLog.d(this, "[stkapp] Exception ");
                    }
                }
                SendCloseChannelResponse(ResultCodeSprd.OK, slotId);
                mStkContext[slotId].mOpenChannelData = null;
                if (mStkContext[slotId].mReceiver != null) {
                    unregisterReceiver(mStkContext[slotId].mReceiver);
                    mStkContext[slotId].mReceiver = null;
                }
            } else {
                CatLog.d(this, "launchCloseChannel channel already closed");
                SendCloseChannelResponse(ResultCodeSprd.BIP_ERROR, slotId);
            }
        }
    }

    private void endConnectivity(int slotId) {
        CatLog.d(this, "[stkapp] stk end connectivity");
        //android.os.SystemProperties.set("gsm.stk.end_connectivity", "1");
        if (mStkContext[slotId].mConnMgr != null && mStkContext[slotId].mNetworkCallback != null) {
            mStkContext[slotId].mConnMgr
                    .unregisterNetworkCallback(mStkContext[slotId].mNetworkCallback);
        }
        mStkContext[slotId].mNetworkCallback = null;
        mStkContext[slotId].mNetwork = null;
    }

    private void deleteOpenChannelApn(int slotId) {
        CatLog.d(this, "[stkapp] deleteOpenChannelApn mCurPos = " + mStkContext[slotId].mCurPos);
        if (mStkContext[slotId].mOpenChannelData.NetAccessName != null) {
            if (mStkContext[slotId].mCurPos != null) {
                ContentResolver resolver = getContentResolver();
                int delNum = resolver.delete(Telephony.Carriers.CONTENT_URI, "_id" + "="
                        + mStkContext[slotId].mCurPos, null);
                CatLog.d(this, " delNum = " + delNum);
            }
        } else {
            CatLog.d(this, "[stkapp] deleteOpenChannelApn used default apn, do nothing");
        }
    }

    private void launchReceiveData(int slotId) {
        CatLog.d(this, "launchReceiveData");
        String dataStr = null;
        ReceiveChannelData receiveData = mStkContext[slotId].mCurrentCmd.getReceiveChannelData();
        int remainLen = 0;
        CatLog.d(this, "channel datalen: " + receiveData.channelDataLength + " mReceiveDataLen = "
                + mStkContext[slotId].mReceiveDataLen + " mReceiveDataOffset = "
                + mStkContext[slotId].mReceiveDataOffset);
        synchronized (mReceiveDataLock) {
        if (mStkContext[slotId].mReceiveDataLen > 0 && mStkContext[slotId].mRecevieData != null) {
            if (receiveData.channelDataLength >= RECEIVE_DATA_MAX_TR_LEN) {
                if (mStkContext[slotId].mReceiveDataLen - RECEIVE_DATA_MAX_TR_LEN > RECEIVE_DATA_MAX_TR_LEN) {
                    remainLen = 0xff;
                } else {
                    remainLen = mStkContext[slotId].mReceiveDataLen - RECEIVE_DATA_MAX_TR_LEN;
                }
                byte[] data = new byte[RECEIVE_DATA_MAX_TR_LEN];
                System.arraycopy(mStkContext[slotId].mRecevieData,
                        mStkContext[slotId].mReceiveDataOffset, data, 0, RECEIVE_DATA_MAX_TR_LEN);
                dataStr = IccUtils.bytesToHexString(data);
                mStkContext[slotId].mReceiveDataLen -= RECEIVE_DATA_MAX_TR_LEN;
                mStkContext[slotId].mReceiveDataOffset += RECEIVE_DATA_MAX_TR_LEN;
            } else {
                // remainLen = 0;
                int channelDataLength = receiveData.channelDataLength;
                if (mStkContext[slotId].mReceiveDataLen > receiveData.channelDataLength) {
                    remainLen = mStkContext[slotId].mReceiveDataLen - receiveData.channelDataLength;
                    if (remainLen > RECEIVE_DATA_MAX_TR_LEN) {
                        remainLen = 0xFF;
                    }
                } else {
                    remainLen = 0;
                    channelDataLength = mStkContext[slotId].mReceiveDataLen;
                }
                byte[] data = new byte[channelDataLength];
                System.arraycopy(mStkContext[slotId].mRecevieData,
                        mStkContext[slotId].mReceiveDataOffset, data, 0, channelDataLength);
                dataStr = IccUtils.bytesToHexString(data);

                if (remainLen == 0) {
                    mStkContext[slotId].mReceiveDataLen = 0;
                    mStkContext[slotId].mReceiveDataOffset = 0;
                } else {
                    mStkContext[slotId].mReceiveDataLen -= channelDataLength;
                    mStkContext[slotId].mReceiveDataOffset += channelDataLength;
                }

            }
            /* SPRD: add here for 551864 @{ */
            if (mStkContext[slotId].mReceiveDataLen <= 0) {
                mStkContext[slotId].mRecevieData = null;
            }
            /* @} */
            ReceiveDataResponse(dataStr, remainLen, ResultCodeSprd.OK, slotId);
        } else {
            CatLog.d(this, "launchReceiveData receive nothing");
            ReceiveDataResponse(null, 0, ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY, slotId);
        }
        }
    }

    private void ReceiveDataResponse(String datastr, int len, ResultCodeSprd resCode, int slotId) {
        CatLog.d(this, "ReceiveDataResponse len = " + len + " data = " + datastr);
        mStkContext[slotId].mCmdInProgress = false;
        if (mStkContext[slotId].mCurrentCmd == null) {
            CatLog.d(this, "mCurrentCmd is null");
            return;
        }
        CatResponseMessageSprd resMsg = new CatResponseMessageSprd(mStkContext[slotId].mCurrentCmd);
        resMsg.setResultCode(resCode);
        resMsg.setChannelDataLen(len);
        resMsg.setChannelData(datastr);
        onCmdResponse(resMsg,slotId);
    }

    private void launchSendData(CommandDetailsSprd cmddet, int slotId) {
        CatLog.d(this, "launchSendData");
        SendChannelData sendData = mStkContext[slotId].mCurrentCmd.getSendChannelData();
        CatLog.d(this, "send mode: " + cmddet.commandQualifier + " senddata: "
                + sendData.sendDataStr);

        byte type = mStkContext[slotId].mOpenChannelData.transportType;
        int port = mStkContext[slotId].mOpenChannelData.portNumber;
        String address = mStkContext[slotId].mOpenChannelData.DataDstAddress;
        int sendlen = sendData.sendDataStr.length() / 2;
        int channellen = 0;
        CatLog.d(this, "[stkapp] type =  " + type);
        CatLog.d(this, "[stkapp] port =  " + port);
        CatLog.d(this, "[stkapp] address =  " + address);

        if (type == OpenChannelData.TRANSPORT_TYPE_TCP) {
            CatLog.d(this, "[stkapp] tcp send");
            if (mStkContext[slotId].mTcpSocket == null) {
                CatLog.d(this, "mTcpSocket is null, channel not established yet!");
                SendDataResponse(0, ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY, slotId);
                return;
            }
            if (cmddet.commandQualifier == SEND_DATA_STORE) {
                if (sendDataStorer[slotId] == null) {
                    sendDataStorer[slotId] = new StringBuilder(sendData.sendDataStr);
                } else {
                    sendDataStorer[slotId].append(sendData.sendDataStr);
                }
                // Set send TR
                channellen = CalcChannelDataLen(cmddet.commandQualifier, sendlen, slotId);
                SendDataResponse(channellen, ResultCodeSprd.OK, slotId);

            } else {
                String readySendDataStr = null;
                if (sendDataStorer[slotId] != null) {
                    sendDataStorer[slotId].append(sendData.sendDataStr);
                    readySendDataStr = sendDataStorer[slotId].toString();
                } else {
                    readySendDataStr = sendData.sendDataStr;
                }

                try {
                    byte[] buffer = IccUtils.hexStringToBytes(readySendDataStr);
                    CatLog.d(this, "[stkapp] mTcpSocket.write, buffer length=" + buffer.length);
                    mStkContext[slotId].mTcpSocket.getOutputStream()
                            .write(buffer, 0, buffer.length);
                    // Set send TR
                    channellen = CalcChannelDataLen(cmddet.commandQualifier, sendlen, slotId);
                    SendDataResponse(channellen, ResultCodeSprd.OK, slotId);
                    // Start receive
                    if (!mThreadInLoop) {
                        mTcpReceData = new TcpReceiveData(slotId);
                        CatLog.d(this, "[stkapp] tcp receive thread start");
                        mTcpReceData.start();
                    }
                } catch (IOException ex) {
                    CatLog.d(this, "[stkapp] mTcpSocket.write exception ex = " + ex);
                    SendDataResponse(0, ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY, slotId);
                } finally {
                    sendDataStorer[slotId] = null;
                }
            }
        } else if (type == OpenChannelData.TRANSPORT_TYPE_UDP) {
            CatLog.d(this, "[stkapp] udp send");
            if (mStkContext[slotId].mUdpSocket == null) {
                CatLog.d(this, "mUdpSocket is null, channel not established yet!");
                SendDataResponse(0, ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY, slotId);
                return;
            }
            if (cmddet.commandQualifier == SEND_DATA_STORE) {
                if (sendDataStorer[slotId] == null) {
                    sendDataStorer[slotId] = new StringBuilder(sendData.sendDataStr);
                } else {
                    sendDataStorer[slotId].append(sendData.sendDataStr);
                }
                channellen = CalcChannelDataLen(cmddet.commandQualifier, sendlen, slotId);
                SendDataResponse(channellen, ResultCodeSprd.OK, slotId);
            } else {
                String readySendDataStr = null;
                if (sendDataStorer[slotId] != null) {
                    sendDataStorer[slotId].append(sendData.sendDataStr);
                    readySendDataStr = sendDataStorer[slotId].toString();
                } else {
                    readySendDataStr = sendData.sendDataStr;
                }

                int sendlength = readySendDataStr.length() / 2;
                byte[] buffer = new byte[sendlength + 1];
                DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                try {
                    stringToPacket(readySendDataStr, packet);
                    InetAddress netAddress = InetAddress.getByName(address);
                    packet.setAddress(netAddress);
                    packet.setPort(port);
                } catch (UnknownHostException ex) {
                    CatLog.d(this, "[stkapp] unkonwnhostexception ex = " + ex);
                    SendDataResponse(0, ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY, slotId);
                    return;
                }
                try {
                    CatLog.d(this, "[stkapp] mUdpSocket.send");
                    mStkContext[slotId].mUdpSocket.send(packet);
                    // Set send TR
                    channellen = CalcChannelDataLen(cmddet.commandQualifier, sendlen, slotId);
                    SendDataResponse(channellen, ResultCodeSprd.OK, slotId);
                    // Start receive
                    if (!mThreadInLoop) {
                        mUdpReceData = new UdpReceiveData(slotId);
                        CatLog.d(this, "[stkapp] udp receive thread start");
                        mUdpReceData.start();
                    }
                } catch (IOException ex) {
                    CatLog.d(this, "[stkapp] mUdpSocket.send exception ex = " + ex);
                    SendDataResponse(0, ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY, slotId);
                } finally {
                    sendDataStorer[slotId] = null;
                }
            }
        } else {
            CatLog.d(this, "transportType mismatch");
            SendDataResponse(0, ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY, slotId);
        }
    }


    private void stringToPacket(String s, DatagramPacket packet) {
        byte[] bytes = IccUtils.hexStringToBytes(s);
        System.arraycopy(bytes, 0, packet.getData(), 0, bytes.length);
        packet.setLength(bytes.length);
    }

    private int CalcChannelDataLen(int mode, int sendlen, int slotId) {
        int retLen = 0;
        CatLog.d(this, "CalcChannelDataLen mode = " + mode + " sendlen = " + sendlen);

        mStkContext[slotId].mSendDataLen += sendlen;
        if (mode == SEND_DATA_IMMEDIATELY) {
            mStkContext[slotId].mSendDataLen = 0;
        }
        if (mStkContext[slotId].mOpenChannelData.bufferSize - mStkContext[slotId].mSendDataLen > RECEIVE_DATA_MAX_TR_LEN) {
            retLen = 0xff;
        } else {
            retLen = mStkContext[slotId].mOpenChannelData.bufferSize
                    - mStkContext[slotId].mSendDataLen;
        }
        CatLog.d(this, "CalcChannelDataLen retLen = " + retLen);
        return retLen;
    }

    private void SendDataResponse(int len, ResultCodeSprd resCode, int slotId) {
        CatLog.d(this, "SendDataResponse len = " + len);
        mStkContext[slotId].mCmdInProgress = false;
        if (mStkContext[slotId].mCurrentCmd == null) {
            CatLog.d(this, "mCurrentCmd is null");
            return;
        }
        CatResponseMessageSprd resMsg = new CatResponseMessageSprd(mStkContext[slotId].mCurrentCmd);
        resMsg.setResultCode(resCode);
        resMsg.setChannelDataLen(len);
        onCmdResponse(resMsg,slotId);
    }

    private void launchChannelStatus(int slotId) {
        CatLog.d(this, "launchChannelStatus");
        if ((mStkContext[slotId].mConnMgr.getNetworkInfo(ConnectivityManager.TYPE_MOBILE_SUPL))
                .isConnected()) {
            CatLog.d(this, "[stkapp] TYPE_MOBILE_SUPL CONNECT");
            mStkContext[slotId].mChannelStatus.SetStatus(AppInterfaceSprd.DEFAULT_CHANNELID, true);
        } else {
            CatLog.d(this, "[stkapp] TYPE_MOBILE_SUPL DISCONNECT");
            mStkContext[slotId].mChannelStatus.SetStatus(0, false);
        }
        SendChannelStatusResponse(ResultCodeSprd.OK, slotId);
    }

    private void SendChannelStatusResponse(ResultCodeSprd resCode, int slotId) {
        CatLog.d(this, "SendChannelStatusResponse ");
        mStkContext[slotId].mCmdInProgress = false;
        if (mStkContext[slotId].mCurrentCmd == null) {
            CatLog.d(this, "mCurrentCmd is null");
            return;
        }
        CatResponseMessageSprd resMsg = new CatResponseMessageSprd(mStkContext[slotId].mCurrentCmd);
        resMsg.setResultCode(resCode);
        CatLog.d(this, "SendChannelStatusResponse LinkStatus"
                + mStkContext[slotId].mChannelStatus.mLinkStatus);
        resMsg.setChannelStatus(mStkContext[slotId].mChannelStatus.mChannelId,
                mStkContext[slotId].mChannelStatus.mLinkStatus,
                mStkContext[slotId].mChannelStatus.mode_info);
        onCmdResponse(resMsg,slotId);
    }

    private void SendCloseChannelResponse(ResultCodeSprd resCode, int slotId) {
        CatLog.d(this, "SendCloseChannelResponse ");
        mStkContext[slotId].mCmdInProgress = false;
        if(mStkContext[slotId].mCurrentCmd == null) {
            CatLog.d(this, "mCurrentCmd is null");
            return;
        }
        CatResponseMessageSprd resMsg = new CatResponseMessageSprd(mStkContext[slotId].mCurrentCmd);
        resMsg.setResultCode(resCode);
        onCmdResponse(resMsg,slotId);
    }

    private static Object mTcpUdpLock = new Object();
    private static Object mReceiveDataLock = new Object();
    private class TcpReceiveData extends Thread {
//        int countRead = 0;
//        byte[] buffer = new byte[1600];
        int slotId;

        private TcpReceiveData(int sId) {
            slotId = sId;
        }

        public void run() {
            synchronized (mTcpUdpLock) {
                CatLog.d(this, "[stkapp] TcpReceiveData ");
            /* SPRD: add here for 551864 @{ */
            //while (mStkContext[slotId].mReceiveDataLen > 0 && mStkContext[slotId].mRecevieData != null) {
                while(mNeedRuning && mStkContext[slotId].mTcpSocket != null && !mStkContext[slotId].mTcpSocket.isClosed()) {
                    if (!mThreadInLoop) {
                        mThreadInLoop = true;
                    }
            //    try {
            //        CatLog.d(this, "[stkapp] Need wait until last command have been handled");
            //        Thread.sleep(50);
            //    } catch (InterruptedException e) {
            //        CatLog.d(this, "[stkapp]  InterruptedException ex = " + e);
            //    }
            //}
            /* @} */
            //while (true) {
                    try {
                        CatLog.d(this, "[stkapp] tcp setSoTimeout -> 60s");
                        if (mStkContext[slotId].mTcpSocket == null || mStkContext[slotId].mTcpSocket.isClosed()) {
                            break;
                        }
                        mStkContext[slotId].mTcpSocket.setSoTimeout(60000);
                        try {
                            InputStream is = mStkContext[slotId].mTcpSocket.getInputStream();
                            int countRead = 0;
                            countRead = is.available();
                            CatLog.d(this, "[stkapp]  countRead = " + countRead);
                            if (countRead > 0) {
                                byte[] buffer = new byte[countRead];
                                countRead = is.read(buffer);
                                CatLog.d(this, "[stkapp] tcp data received! len = " + countRead);

                                synchronized (mReceiveDataLock) {
                                    mStkContext[slotId].mReceiveDataLen = countRead;
                                    mStkContext[slotId].mRecevieData = new byte[countRead];
                                    System.arraycopy(buffer, 0, mStkContext[slotId].mRecevieData, 0, countRead);
                                    //CatLog.d(this, "[stkapp] break TcpReceiveData = "
                                    //            + IccUtils.bytesToHexString(mStkContext[slotId].mRecevieData));
                                    mStkContext[slotId].mReceiveDataOffset = 0;
                                }
                                sendEventDataAvailable(slotId);

                                while (mNeedRuning && mStkContext[slotId].mReceiveDataLen > 0 && mStkContext[slotId].mRecevieData != null) {
                                    try {
                                        CatLog.d(this, "[stkapp] mRecevieData != null , need wait...");
                                        Thread.sleep(50);
                                    } catch (InterruptedException e) {
                                        CatLog.d(this, "[stkapp] InterruptedException ex = " + e);
                                   }
                                }
                            } else {
                                try {
                                    CatLog.d(this, "[stkapp] no data available, need wait...");
                                    Thread.sleep(100);
                                } catch (InterruptedException e) {
                                    CatLog.d(this, "[stkapp] InterruptedException1 ex = " + e);
                                }
                            }
                            CatLog.d(this, "[stkapp] one try... ");
                        } catch (IOException ex) {
                           CatLog.d(this, "[stkapp]  IOException ex = " + ex);
                        } finally {
                            if (mStkContext[slotId].mTcpSocket != null) {
                                mStkContext[slotId].mTcpSocket.setSoTimeout(0);
                            }
                        }
                    } catch (SocketException ex) {
                        CatLog.d(this, "[stkapp] SocketException ex = " + ex);
                        closeSocket(slotId);
                        if (mStkContext[slotId].mChannelEstablished) {
                            mStkContext[slotId].mChannelStatus.SetStatus(AppInterfaceSprd.DEFAULT_CHANNELID, false, CHANNEL_MODE_LINK_DROPPED);
                            sendEventChannelStatus(slotId);
                        }
                    } catch (NullPointerException ex1) {
                        CatLog.d(this, "[stkapp] NullPointerException ex1 = " + ex1);
                        closeSocket(slotId);
                        if (mStkContext[slotId].mChannelEstablished) {
                            mStkContext[slotId].mChannelStatus.SetStatus(AppInterfaceSprd.DEFAULT_CHANNELID, false, CHANNEL_MODE_LINK_DROPPED);
                            sendEventChannelStatus(slotId);
                        }
                    }
                }

                mThreadInLoop = false;
                CatLog.d(this, "[stkapp] TcpReceiveData thread exit...");
            }
        }
    }

    private void sendEventDataAvailable(int slotId) {
        sendSetUpEventResponse(DATA_AVAILABLE_EVENT, null, slotId);
    }

    private class UdpReceiveData extends Thread {
        DatagramPacket packet;
        byte[] buffer = new byte[4096];
        int slotId;

        private UdpReceiveData(int sId) {
            slotId = sId;
        }
        public void run() {
            synchronized (mTcpUdpLock) {
                CatLog.d(this, "[stkapp] UdpReceiveData ");
                packet = new DatagramPacket(buffer, buffer.length);
                /* SPRD: add here for 551864 @{ */
//            while (mStkContext[slotId].mReceiveDataLen > 0 && mStkContext[slotId].mRecevieData != null) {
//                try {
//                    CatLog.d(this, "[stkapp] Need wait until last command have been handled");
//                    Thread.sleep(50);
//                } catch (InterruptedException e) {
//                    CatLog.d(this, "[stkapp]  InterruptedException ex = " + e);
//                }
//            }
//            /* @} */
//            while(true) {
                while (mNeedRuning && mStkContext[slotId].mUdpSocket != null && !mStkContext[slotId].mUdpSocket.isClosed()) {
                    if (!mThreadInLoop) {
                        mThreadInLoop = true;
                    }
                    try {
                        CatLog.d(this, "[stkapp] udp setSoTimeout ");
                        mStkContext[slotId].mUdpSocket.setSoTimeout(60000);
                        try{
                            mStkContext[slotId].mUdpSocket.receive(packet);
                            CatLog.d(this, "[stkapp] udp packet len = " + packet.getLength() + " offset = " + packet.getOffset());
                            int len = packet.getLength();
                            if (len > 0) {
                                synchronized (mReceiveDataLock) {
                                    mStkContext[slotId].mReceiveDataLen = len;
                                    mStkContext[slotId].mRecevieData = new byte[len];
                                    System.arraycopy(packet.getData(), packet.getOffset(), mStkContext[slotId].mRecevieData, 0, len);
                                    //CatLog.d(this, "[stkapp] break UdpReceiveData = " + IccUtils.bytesToHexString(mStkContext[slotId].mRecevieData));
                                    mStkContext[slotId].mReceiveDataOffset = 0;
                                }
                                sendEventDataAvailable(slotId);
                                while (mNeedRuning && mStkContext[slotId].mReceiveDataLen > 0 && mStkContext[slotId].mRecevieData != null) {
                                    try {
                                    CatLog.d(this, "[stkapp] mRecevieData != null , need wait ...");
                                        Thread.sleep(50);
                                    } catch (InterruptedException e) {
                                        CatLog.d(this, "[stkapp]  InterruptedException ex = " + e);
                                    }
                                }
                            } else {
                                try {
                                    CatLog.d(this, "[stkapp] no data available, need wait...");
                                    Thread.sleep(100);
                                } catch (InterruptedException e) {
                                    CatLog.d(this, "[stkapp] InterruptedException1 ex = " + e);
                                }
                            }
                        } catch (IOException ex) {
                            CatLog.d(this, "[stkapp]  IOException ex = " + ex);
                        } finally {
                            CatLog.d(this, "[stkapp] udp setSoTimeout -> 0");
                            mStkContext[slotId].mUdpSocket.setSoTimeout(0);
                        }
                    } catch (SocketException ex) {
                        CatLog.d(this, "[stkapp] SocketException ex = " + ex);
                        closeSocket(slotId);
                        if (mStkContext[slotId].mChannelEstablished) {
                            mStkContext[slotId].mChannelStatus.SetStatus(AppInterfaceSprd.DEFAULT_CHANNELID, false,
                                    CHANNEL_MODE_LINK_DROPPED);
                            sendEventChannelStatus(slotId);
                        }
                    } catch (NullPointerException ex1) {
                        CatLog.d(this, "[stkapp] NullPointerException ex1 = " + ex1);
                        closeSocket(slotId);
                        if (mStkContext[slotId].mChannelEstablished) {
                            mStkContext[slotId].mChannelStatus.SetStatus(AppInterfaceSprd.DEFAULT_CHANNELID, false,
                                    CHANNEL_MODE_LINK_DROPPED);
                            sendEventChannelStatus(slotId);
                        }
                    }
                }

                mThreadInLoop = false;
                CatLog.d(this, "[stkapp] UdpReceiveData thread exit...");
            }
        }
    }

    private void SendChannelResponse(OpenChannelData data, ResultCodeSprd resCode, int slotId) {
        CatLog.d(this, "SendChannelResponse ");
        mStkContext[slotId].mCmdInProgress = false;
        if (mStkContext[slotId].mCurrentCmd == null) {
            CatLog.d(this, "mCurrentCmd is null");
            return;
        }
        if (resCode == ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY) {
            deleteOpenChannelApn(slotId);
            if (mStkContext[slotId].mConnectType == ConnectivityManager.TYPE_MOBILE_SUPL) {
                endConnectivity(slotId);
            }
            mStkContext[slotId].mConnectType = -1;
            if (mStkContext[slotId].mTcpSocket != null) {
                try {
                    if (!mStkContext[slotId].mTcpSocket.isClosed()) {
                        mStkContext[slotId].mTcpSocket.close();
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                    CatLog.d(this, "[stkapp] Exception ");
                }
            }
        }

        CatResponseMessageSprd resMsg = new CatResponseMessageSprd(mStkContext[slotId].mCurrentCmd);
        resMsg.setResultCode(resCode);
        resMsg.setBearerParam(data.BearerType, data.BearerParam, data.bufferSize);
        CatLog.d(this, "SendChannelResponse LinkStatus = " + mStkContext[slotId].mChannelStatus.mLinkStatus);
        resMsg.setChannelStatus(mStkContext[slotId].mChannelStatus.mChannelId, mStkContext[slotId].mChannelStatus.mLinkStatus,
                mStkContext[slotId].mChannelStatus.mode_info);
        onCmdResponse(resMsg,slotId);
    }
    /* @ } */

    private void handleCmdResponse(Bundle args, int slotId) {
        CatLog.d(LOG_TAG, "handleCmdResponse, sim id: " + slotId);
        if (mStkContext[slotId].mCurrentCmd == null) {
            return;
        }

        if (mStkService[slotId] == null) {
            mStkService[slotId] = CatServiceSprd.getInstance(slotId);
            if (mStkService[slotId] == null) {
                // This should never happen (we should be responding only to a message
                // that arrived from StkService). It has to exist by this time
                CatLog.d(LOG_TAG, "Exception! mStkService is null when we need to send response.");
                /*SPRD: 626432  Turn off/on sim card or hut plug in/out sim card,
                 *CatServiceSprd/mStkService in FW will be null so return*/
//                throw new RuntimeException("mStkService is null when we need to send response");
                return;
            }
        }

        CatResponseMessageSprd resMsg = new CatResponseMessageSprd(mStkContext[slotId].mCurrentCmd);

        // set result code
        boolean helpRequired = args.getBoolean(HELP, false);
        boolean confirmed    = false;
        /* SPRD: add for cucc two icon feature . @{ */
        boolean homepressed = false;
        /* @} */

        switch(args.getInt(RES_ID)) {
        case RES_ID_MENU_SELECTION:
            CatLog.d(LOG_TAG, "MENU_SELECTION=" + mStkContext[slotId].
                    mCurrentMenuCmd.getCmdType());
            int menuSelection = args.getInt(MENU_SELECTION);
            switch(mStkContext[slotId].mCurrentMenuCmd.getCmdType()) {
            case SET_UP_MENU:
            case SELECT_ITEM:
                mStkContext[slotId].lastSelectedItem = getItemName(menuSelection, slotId);
                if (helpRequired) {
                    resMsg.setResultCode(ResultCodeSprd.HELP_INFO_REQUIRED);
                } else {
                    resMsg.setResultCode(mStkContext[slotId].mCurrentCmd.hasIconLoadFailed() ?
                            ResultCodeSprd.PRFRMD_ICON_NOT_DISPLAYED : ResultCodeSprd.OK);
                }
                resMsg.setMenuSelection(menuSelection);
                break;
            }
            break;
        case RES_ID_INPUT:
            CatLog.d(LOG_TAG, "RES_ID_INPUT");
            String input = args.getString(INPUT);
            if (input != null && (null != mStkContext[slotId].mCurrentCmd.geInput()) &&
                    (mStkContext[slotId].mCurrentCmd.geInput().yesNo)) {
                boolean yesNoSelection = input
                        .equals(StkInputActivity.YES_STR_RESPONSE);
                resMsg.setYesNo(yesNoSelection);
            } else {
                if (helpRequired) {
                    resMsg.setResultCode(ResultCodeSprd.HELP_INFO_REQUIRED);
                } else {
                    resMsg.setResultCode(mStkContext[slotId].mCurrentCmd.hasIconLoadFailed() ?
                            ResultCodeSprd.PRFRMD_ICON_NOT_DISPLAYED : ResultCodeSprd.OK);
                    resMsg.setInput(input);
                }
            }
            break;
        case RES_ID_CONFIRM:
            CatLog.d(this, "RES_ID_CONFIRM");
            confirmed = args.getBoolean(CONFIRMATION);
            switch (mStkContext[slotId].mCurrentCmd.getCmdType()) {
            case DISPLAY_TEXT:
                if (confirmed) {
                    resMsg.setResultCode(mStkContext[slotId].mCurrentCmd.hasIconLoadFailed() ?
                            ResultCodeSprd.PRFRMD_ICON_NOT_DISPLAYED : ResultCodeSprd.OK);
                } else {
                    resMsg.setResultCode(ResultCodeSprd.UICC_SESSION_TERM_BY_USER);
                }
                break;
            case LAUNCH_BROWSER:
                resMsg.setResultCode(confirmed ? ResultCodeSprd.OK
                        : ResultCodeSprd.UICC_SESSION_TERM_BY_USER);
                if (confirmed) {
                    mStkContext[slotId].launchBrowser = true;
                    mStkContext[slotId].mBrowserSettings =
                            mStkContext[slotId].mCurrentCmd.getBrowserSettings();
                }
                break;
            case SET_UP_CALL:
                if (confirmed) {
                    processSetupCallResponse(slotId);
                    return;
                }
                // Cancel
                mStkContext[slotId].mSetupCallInProcess = false;
                resMsg.setResultCode(ResultCodeSprd.OK);
                resMsg.setConfirmation(confirmed);
                /* @} */
                break;
            /* SPRD: Add here for Open Channel function @{ */
            case OPEN_CHANNEL:
                CatLog.d(this, "OPEN_CHANNEL confirmed = " + confirmed);
                if (confirmed) {
                    launchOpenChannel(slotId);
                } else {
                    // Cancel
                    resMsg.setConfirmation(confirmed);
                    SendChannelResponse(mStkContext[slotId].mOpenChannelData, ResultCodeSprd.USER_NOT_ACCEPT, slotId);
                }
                return;
            /* @} */
            }
            break;
        case RES_ID_DONE:
            resMsg.setResultCode(ResultCodeSprd.OK);
            break;
        case RES_ID_BACKWARD:
            CatLog.d(LOG_TAG, "RES_ID_BACKWARD");
            resMsg.setResultCode(ResultCodeSprd.BACKWARD_MOVE_BY_USER);
            break;
        case RES_ID_END_SESSION:
            CatLog.d(LOG_TAG, "RES_ID_END_SESSION");
            resMsg.setResultCode(ResultCodeSprd.UICC_SESSION_TERM_BY_USER);
            break;
        case RES_ID_TIMEOUT:
            CatLog.d(LOG_TAG, "RES_ID_TIMEOUT");
            // GCF test-case 27.22.4.1.1 Expected Sequence 1.5 (DISPLAY TEXT,
            // Clear message after delay, successful) expects result code OK.
            // If the command qualifier specifies no user response is required
            // then send OK instead of NO_RESPONSE_FROM_USER
            if ((mStkContext[slotId].mCurrentCmd.getCmdType().value() ==
                    AppInterfaceSprd.CommandTypeSprd.DISPLAY_TEXT.value())
                    && (mStkContext[slotId].mCurrentCmd.geTextMessage().userClear == false)) {
                resMsg.setResultCode(ResultCodeSprd.OK);
            } else {
                resMsg.setResultCode(ResultCodeSprd.NO_RESPONSE_FROM_USER);
            }
            break;
        case RES_ID_CHOICE:
            int choice = args.getInt(CHOICE);
            CatLog.d(this, "User Choice=" + choice);
            switch (choice) {
                case YES:
                    resMsg.setResultCode(ResultCodeSprd.OK);
                    confirmed = true;
                    break;
                case NO:
                    resMsg.setResultCode(ResultCodeSprd.USER_NOT_ACCEPT);
                    break;
            }

            if (mStkContext[slotId].mCurrentCmd.getCmdType().value() ==
                    AppInterfaceSprd.CommandTypeSprd.OPEN_CHANNEL.value()) {
                resMsg.setConfirmation(confirmed);
            }
            break;

        default:
            CatLog.d(LOG_TAG, "Unknown result id");
            return;
        }

        if (null != mStkContext[slotId].mCurrentCmd &&
                null != mStkContext[slotId].mCurrentCmd.getCmdType()) {
            CatLog.d(LOG_TAG, "handleCmdResponse- cmdName[" +
                    mStkContext[slotId].mCurrentCmd.getCmdType().name() + "]");
        }
        onCmdResponse(resMsg,slotId);
        /*SPRD: BUG 640200 In case a launch browser command was just after send terminal response, launch that url. @{*/
        if ((mStkContext[slotId].mCurrentCmd.getCmdType().value() ==
                AppInterfaceSprd.CommandTypeSprd.LAUNCH_BROWSER.value()) && mStkContext[slotId].launchBrowser) {
            mStkContext[slotId].launchBrowser = false;
            launchBrowser(mStkContext[slotId].mBrowserSettings);
        }
        /* @} */
    }

    /**
     * Returns 0 or FLAG_ACTIVITY_NO_USER_ACTION, 0 means the user initiated the action.
     *
     * @param userAction If the userAction is yes then we always return 0 otherwise
     * mMenuIsVisible is used to determine what to return. If mMenuIsVisible is true
     * then we are the foreground app and we'll return 0 as from our perspective a
     * user action did cause. If it's false than we aren't the foreground app and
     * FLAG_ACTIVITY_NO_USER_ACTION is returned.
     *
     * @return 0 or FLAG_ACTIVITY_NO_USER_ACTION
     */
    private int getFlagActivityNoUserAction(InitiatedByUserAction userAction, int slotId) {
        return ((userAction == InitiatedByUserAction.yes) | mStkContext[slotId].mMenuIsVisible)
                ? 0 : Intent.FLAG_ACTIVITY_NO_USER_ACTION;
    }
    /**
     * This method is used for cleaning up pending instances in stack.
     */
    private void cleanUpInstanceStackBySlot(int slotId) {
        Activity activity = mStkContext[slotId].getPendingActivityInstance();
        Activity dialog = mStkContext[slotId].getPendingDialogInstance();
        CatLog.d(LOG_TAG, "cleanUpInstanceStackBySlot slotId: " + slotId);
        if (mStkContext[slotId].mCurrentCmd == null) {
            CatLog.d(LOG_TAG, "current cmd is null.");
            return;
        }
        if (activity != null) {
            CatLog.d(LOG_TAG, "current cmd type: " +
                    mStkContext[slotId].mCurrentCmd.getCmdType());
            if (mStkContext[slotId].mCurrentCmd.getCmdType().value() ==
                    AppInterfaceSprd.CommandTypeSprd.GET_INPUT.value() ||
                    mStkContext[slotId].mCurrentCmd.getCmdType().value() ==
                    AppInterfaceSprd.CommandTypeSprd.GET_INKEY.value()) {
                mStkContext[slotId].mIsInputPending = true;
            } else if (mStkContext[slotId].mCurrentCmd.getCmdType().value() ==
                    AppInterfaceSprd.CommandTypeSprd.SET_UP_MENU.value() ||
                    mStkContext[slotId].mCurrentCmd.getCmdType().value() ==
                    AppInterfaceSprd.CommandTypeSprd.SELECT_ITEM.value()) {
                mStkContext[slotId].mIsMenuPending = true;
            } else {
            }
            CatLog.d(LOG_TAG, "finish pending activity.");
            activity.finish();
            mStkContext[slotId].mActivityInstance = null;
        }
        if (dialog != null) {
            CatLog.d(LOG_TAG, "finish pending dialog.");
            mStkContext[slotId].mIsDialogPending = true;
            dialog.finish();
            mStkContext[slotId].mDialogInstance = null;
        }
    }
    /**
     * This method is used for restoring pending instances from stack.
     */
    private void restoreInstanceFromStackBySlot(int slotId) {
        AppInterfaceSprd.CommandTypeSprd cmdType = mStkContext[slotId].mCurrentCmd.getCmdType();

        CatLog.d(LOG_TAG, "restoreInstanceFromStackBySlot cmdType : " + cmdType);
        switch(cmdType) {
            case GET_INPUT:
            case GET_INKEY:
                launchInputActivity(slotId);
                //Set mMenuIsVisible to true for showing main menu for
                //following session end command.
                mStkContext[slotId].mMenuIsVisible = true;
            break;
            case DISPLAY_TEXT:
                launchTextDialog(slotId);
            break;
            case LAUNCH_BROWSER:
                launchConfirmationDialog(mStkContext[slotId].mCurrentCmd.geTextMessage(),
                        slotId);
            break;
            case OPEN_CHANNEL:
                launchOpenChannelDialog(slotId);
            break;
            case SET_UP_CALL:
                launchConfirmationDialog(mStkContext[slotId].mCurrentCmd.getCallSettings().
                        confirmMsg, slotId);
            break;
            case SET_UP_MENU:
            case SELECT_ITEM:
                launchMenuActivity(null, slotId);
            break;
        default:
            break;
        }
    }

    private void launchMenuActivity(Menu menu, int slotId) {
        Intent newIntent = new Intent(Intent.ACTION_VIEW);
        String targetActivity = STK_MENU_ACTIVITY_NAME;
        String uriString = STK_MENU_URI + System.currentTimeMillis();
        //Set unique URI to create a new instance of activity for different slotId.
        Uri uriData = Uri.parse(uriString);

        CatLog.d(LOG_TAG, "launchMenuActivity, slotId: " + slotId + " , " +
                uriData.toString() + " , " + mStkContext[slotId].mOpCode + ", "
                + mStkContext[slotId].mMenuState);
        newIntent.setClassName(PACKAGE_NAME, targetActivity);
        int intentFlags = Intent.FLAG_ACTIVITY_NEW_TASK;

        if (menu == null) {
            // We assume this was initiated by the user pressing the tool kit icon
            intentFlags |= getFlagActivityNoUserAction(InitiatedByUserAction.yes, slotId);
            if (mStkContext[slotId].mOpCode == OP_END_SESSION) {
                CatLog.d(LOG_TAG, "launchMenuActivity, return OP_END_SESSION");
                /* SPRD: add for cucc two icon feature . @{ */
                /* SPRD: because the menuactivity is singal mode in the CUCC version */
                //if (mStkContext[slotId].mMainActivityInstance != null) {
                if (mStkContext[slotId].mMainActivityInstance != null&&!isCUCCOperator()
                        && mStkContext[slotId].mMenuState != StkMenuActivity.STATE_SECONDARY) {
                    CatLog.d(LOG_TAG, "launchMenuActivity, mMainActivityInstance is not null");
                    mStkContext[slotId].mMenuState = StkMenuActivity.STATE_MAIN;
                    return;
                }else{
                    CatLog.d(LOG_TAG,"launchMenuActivity, need to launch new menuactivity!");
                }
                mStkContext[slotId].mMenuState = StkMenuActivity.STATE_MAIN;
                /* @} */
            }

            //If the last pending menu is secondary menu, "STATE" should be "STATE_SECONDARY".
            //Otherwise, it should be "STATE_MAIN".
            if (mStkContext[slotId].mOpCode == OP_LAUNCH_APP &&
                    mStkContext[slotId].mMenuState == StkMenuActivity.STATE_SECONDARY) {
                newIntent.putExtra("STATE", StkMenuActivity.STATE_SECONDARY);
            } else {
                newIntent.putExtra("STATE", StkMenuActivity.STATE_MAIN);
                mStkContext[slotId].mMenuState = StkMenuActivity.STATE_MAIN;
            }
        } else {
            // We don't know and we'll let getFlagActivityNoUserAction decide.
            intentFlags |= getFlagActivityNoUserAction(InitiatedByUserAction.unknown, slotId);
            newIntent.putExtra("STATE", StkMenuActivity.STATE_SECONDARY);
            mStkContext[slotId].mMenuState = StkMenuActivity.STATE_SECONDARY;
        }
        newIntent.putExtra(SLOT_ID, slotId);
        newIntent.setData(uriData);
        newIntent.setFlags(intentFlags);
        mContext.startActivity(newIntent);
    }

    /* SPRD:Bug#490274: When receive session_end,return to mainmenu,if go into the stk not by user @{ */
    private void closeMenuActivity(int slotId) {
        Intent newIntent = new Intent(Intent.ACTION_VIEW);
        newIntent.setClassName(PACKAGE_NAME, STK_MENU_ACTIVITY_NAME);
        int intentFlags = Intent.FLAG_ACTIVITY_NEW_TASK
                | Intent.FLAG_ACTIVITY_CLEAR_TOP;

        intentFlags |= getFlagActivityNoUserAction(InitiatedByUserAction.unknown, slotId);
        newIntent.putExtra("STATE", StkMenuActivity.STATE_END);

        newIntent.setFlags(intentFlags);
        mContext.startActivity(newIntent);
        CatLog.d(this,"CloseMenuActivity");
    }

    public boolean needCloseMenu(int slotId) {
        //PersistableBundle pBundle = mCarrierConfigManager.getConfigForPhoneId(slotId);
        boolean needClose = false;
        needClose = StkPluginsHelper.getInstance(mContext).needCloseMenu();
        CatLog.d(this,"needCloseMenu , needClose : " + needClose);
        return needClose;
    }
    /* @} */

    private void launchInputActivity(int slotId) {
        Intent newIntent = new Intent(Intent.ACTION_VIEW);
        String targetActivity = STK_INPUT_ACTIVITY_NAME;
        String uriString = STK_INPUT_URI + System.currentTimeMillis();
        //Set unique URI to create a new instance of activity for different slotId.
        Uri uriData = Uri.parse(uriString);

        CatLog.d(LOG_TAG, "launchInputActivity, slotId: " + slotId);
        newIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                            | getFlagActivityNoUserAction(InitiatedByUserAction.unknown, slotId));
        newIntent.setClassName(PACKAGE_NAME, targetActivity);
        newIntent.putExtra("INPUT", mStkContext[slotId].mCurrentCmd.geInput());
        newIntent.putExtra(SLOT_ID, slotId);
        newIntent.setData(uriData);
        mContext.startActivity(newIntent);
    }

    private void launchTextDialog(int slotId) {
        CatLog.d(LOG_TAG, "launchTextDialog, slotId: " + slotId);
        Intent newIntent = new Intent();
        String targetActivity = STK_DIALOG_ACTIVITY_NAME;
        int action = getFlagActivityNoUserAction(InitiatedByUserAction.unknown, slotId);
        String uriString = STK_DIALOG_URI + System.currentTimeMillis();
        //Set unique URI to create a new instance of activity for different slotId.
        Uri uriData = Uri.parse(uriString);
        if (newIntent != null) {
            newIntent.setClassName(PACKAGE_NAME, targetActivity);
            newIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                    | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS
                    | getFlagActivityNoUserAction(InitiatedByUserAction.unknown, slotId));
            newIntent.setData(uriData);
            newIntent.putExtra("TEXT", mStkContext[slotId].mCurrentCmd.geTextMessage());
            newIntent.putExtra(SLOT_ID, slotId);
            startActivity(newIntent);
            // For display texts with immediate response, send the terminal response
            // immediately. responseNeeded will be false, if display text command has
            // the immediate response tlv.
            if (!mStkContext[slotId].mCurrentCmd.geTextMessage().responseNeeded) {
                sendResponse(RES_ID_CONFIRM, slotId, true);
            }
        }
    }

    public boolean isStkDialogActivated(Context context) {
        String stkDialogActivity = "com.android.stk.StkDialogActivity";
        boolean activated = false;
        final ActivityManager am = (ActivityManager) context.getSystemService(
                Context.ACTIVITY_SERVICE);
        String topActivity = am.getRunningTasks(1).get(0).topActivity.getClassName();

        CatLog.d(LOG_TAG, "isStkDialogActivated: " + topActivity);
        if (topActivity.equals(stkDialogActivity)) {
            activated = true;
        }
        CatLog.d(LOG_TAG, "activated : " + activated);
        return activated;
    }

    private void sendSetUpEventResponse(int event, byte[] addedInfo, int slotId) {
        CatLog.d(this, "sendSetUpEventResponse: event : " + event + "slotId = " + slotId);

        if (mStkContext[slotId].mCurrentSetupEventCmd == null){
            CatLog.e(this, "mCurrentSetupEventCmd is null");
            return;
        }

        CatResponseMessageSprd resMsg = new CatResponseMessageSprd(mStkContext[slotId].mCurrentSetupEventCmd);

        /* SPRD: Add some events for EVENTDOWNLOAD @{ */
        if (event == CHANNEL_STATUS_EVENT) {
            resMsg.setChannelStatus(mStkContext[slotId].mChannelStatus.mChannelId,
                    mStkContext[slotId].mChannelStatus.mLinkStatus,
                    mStkContext[slotId].mChannelStatus.mode_info);
        } else if (event == DATA_AVAILABLE_EVENT) {
            int length = 0;
            if (mStkContext[slotId].mReceiveDataLen > RECEIVE_DATA_MAX_TR_LEN) {
                length = 0xff;
            } else {
                length = mStkContext[slotId].mReceiveDataLen;
            }
            CatLog.d(this, "sendEventDataAvailable, length = " + length);
            resMsg.setChannelDataLen(length);
            resMsg.setChannelStatus(mStkContext[slotId].mChannelStatus.mChannelId,
                    mStkContext[slotId].mChannelStatus.mLinkStatus,
                    mStkContext[slotId].mChannelStatus.mode_info);
        }
        /* @} */
        resMsg.setResultCode(ResultCodeSprd.OK);
        resMsg.setEventDownload(event, addedInfo);

        onCmdResponse(resMsg,slotId);
    }

    private void checkForSetupEvent(int event, Bundle args, int slotId) {
        boolean eventPresent = false;
        byte[] addedInfo = null;
        CatLog.d(this, "Event :" + event);

        if (mStkContext[slotId].mSetupEventListSettings != null) {
            /* Checks if the event is present in the EventList updated by last
             * SetupEventList Proactive Command */
            for (int i : mStkContext[slotId].mSetupEventListSettings.eventList) {
                 if (event == i) {
                     eventPresent =  true;
                     break;
                 }
            }

            /* If Event is present send the response to ICC */
            if (eventPresent == true) {
                CatLog.d(this, " Event " + event + "exists in the EventList");

                switch (event) {
                    case IDLE_SCREEN_AVAILABLE_EVENT:
                        sendSetUpEventResponse(event, addedInfo, slotId);
                        removeSetUpEvent(event, slotId);
                        break;
                    case LANGUAGE_SELECTION_EVENT:
                        String language =  mContext
                                .getResources().getConfiguration().locale.getLanguage();
                        CatLog.d(this, "language: " + language);
                        // Each language code is a pair of alpha-numeric characters.
                        // Each alpha-numeric character shall be coded on one byte
                        // using the SMS default 7-bit coded alphabet
                        addedInfo = GsmAlphabet.stringToGsm8BitPacked(language);
                        sendSetUpEventResponse(event, addedInfo, slotId);
                        break;
                    /* SPRD: Add for STK 27.22.7.5.1 @{ */
                    case USER_ACTIVITY_EVENT:
                        sendSetUpEventResponse(event, addedInfo, slotId);
                        removeSetUpEvent(event, slotId);
                        break;
                    /* @} */
                    default:
                        break;
                }
            } else {
                CatLog.e(this, " Event does not exist in the EventList");
            }
        } else {
            CatLog.e(this, "SetupEventList is not received. Ignoring the event: " + event);
        }
    }

    private void  removeSetUpEvent(int event, int slotId) {
        CatLog.d(this, "Remove Event :" + event);

        if (mStkContext[slotId].mSetupEventListSettings != null) {
            /*
             * Make new  Eventlist without the event
             */
            for (int i = 0; i < mStkContext[slotId].mSetupEventListSettings.eventList.length; i++) {
                if (event == mStkContext[slotId].mSetupEventListSettings.eventList[i]) {
                    mStkContext[slotId].mSetupEventListSettings.eventList[i] = INVALID_SETUP_EVENT;
                    break;
                }
            }
        }
    }

    private void launchEventMessage(int slotId) {
        launchEventMessage(slotId, mStkContext[slotId].mCurrentCmd.geTextMessage());
    }

    private void launchEventMessage(int slotId, TextMessage msg) {
        if (msg == null || (msg.text != null && msg.text.length() == 0) || msg.text == null) {
            CatLog.d(LOG_TAG, "launchEventMessage return");
            return;
        }

        Toast toast = new Toast(mContext.getApplicationContext());
        LayoutInflater inflate = (LayoutInflater) mContext
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View v = inflate.inflate(R.layout.stk_event_msg, null);
        TextView tv = (TextView) v
                .findViewById(com.android.internal.R.id.message);
        ImageView iv = (ImageView) v
                .findViewById(com.android.internal.R.id.icon);
        if (msg.icon != null) {
            iv.setImageBitmap(msg.icon);
        } else {
            iv.setVisibility(View.GONE);
        }
        /* In case of 'self explanatory' stkapp should display the specified
         * icon in proactive command (but not the alpha string).
         * If icon is non-self explanatory and if the icon could not be displayed
         * then alpha string or text data should be displayed
         * Ref: ETSI 102.223,section 6.5.4
         */
        if (mStkContext[slotId].mCurrentCmd.hasIconLoadFailed() ||
                msg.icon == null || !msg.iconSelfExplanatory) {
            tv.setText(msg.text);
        }

        toast.setView(v);
        toast.setDuration(Toast.LENGTH_LONG);
        toast.setGravity(Gravity.BOTTOM, 0, 0);
        toast.show();
    }

    private void launchConfirmationDialog(TextMessage msg, int slotId) {
        msg.title = mStkContext[slotId].lastSelectedItem;
        Intent newIntent = new Intent();
        String targetActivity = STK_DIALOG_ACTIVITY_NAME;
        String uriString = STK_DIALOG_URI + System.currentTimeMillis();
        //Set unique URI to create a new instance of activity for different slotId.
        Uri uriData = Uri.parse(uriString);

        if (newIntent != null) {
            newIntent.setClassName(this, targetActivity);
            newIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                    | Intent.FLAG_ACTIVITY_NO_HISTORY
                    | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS
                    | getFlagActivityNoUserAction(InitiatedByUserAction.unknown, slotId));
            newIntent.putExtra("TEXT", msg);
            newIntent.putExtra(SLOT_ID, slotId);
            newIntent.setData(uriData);
            startActivity(newIntent);
        }
    }

    private void launchBrowser(BrowserSettings settings) {
        if (settings == null) {
            return;
        }

        Uri data = null;
        String url;
        if (settings.url == null) {
            // if the command did not contain a URL,
            // launch the browser to the default homepage.
            CatLog.d(this, "no url data provided by proactive command." +
                       " launching browser with stk default URL ... ");
            url = SystemProperties.get(STK_BROWSER_DEFAULT_URL_SYSPROP,
                    "http://www.google.com");
        } else {
            CatLog.d(this, "launch browser command has attached url = " + settings.url);
            url = settings.url;
        }

        if (url.startsWith("http://") || url.startsWith("https://")) {
            data = Uri.parse(url);
            CatLog.d(this, "launching browser with url = " + url);
        } else {
            String modifiedUrl = "http://" + url;
            data = Uri.parse(modifiedUrl);
            CatLog.d(this, "launching browser with modified url = " + modifiedUrl);
        }

        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(data);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        switch (settings.mode) {
        case USE_EXISTING_BROWSER:
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
            break;
        case LAUNCH_NEW_BROWSER:
            intent.addFlags(Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
            break;
        case LAUNCH_IF_NOT_ALREADY_LAUNCHED:
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
            break;
        }
        // start browser activity
        startActivity(intent);
        // a small delay, let the browser start, before processing the next command.
        // this is good for scenarios where a related DISPLAY TEXT command is
        // followed immediately.
        try {
            Thread.sleep(10000);
        } catch (InterruptedException e) {}
    }

    private void launchIdleText(int slotId) {
        TextMessage msg = mStkContext[slotId].mIdleModeTextCmd.geTextMessage();

        if (msg == null || msg.text ==null) {
            CatLog.d(LOG_TAG,  msg == null ? "mCurrent.getTextMessage is NULL"
                    : "mCurrent.getTextMessage.text is NULL");
            mNotificationManager.cancel(getNotificationId(slotId));
            return;
        } else {
            CatLog.d(LOG_TAG, "launchIdleText - text[" + msg.text
                    + "] iconSelfExplanatory[" + msg.iconSelfExplanatory
                    + "] icon[" + msg.icon + "], sim id: " + slotId);
            CatLog.d(LOG_TAG, "Add IdleMode text");

            /* SPRD: notification can not display long text, so we start activity to support  @{*/
            //PendingIntent pendingIntent = PendingIntent.getService(mContext, 0,
            //        new Intent(mContext, StkAppService.class), 0);
            Intent pendIntentData = new Intent();
            pendIntentData.setClassName("com.android.stk","com.android.stk.StkMessageActivity");

            StkApp.idleModeText = msg.text;
            StkApp.idleModeIcon = msg.icon;
            StkApp.idleModeIconSelfExplanatory = msg.iconSelfExplanatory;

            PendingIntent pendingIntent = PendingIntent.getActivity(mContext, 0,
                      pendIntentData, PendingIntent.FLAG_UPDATE_CURRENT);
            /* @}*/

            final Notification.Builder notificationBuilder = new Notification.Builder(
                    StkAppService.this);
            if (mStkContext[slotId].mMainCmd != null &&
                    mStkContext[slotId].mMainCmd.getMenu() != null) {
                notificationBuilder.setContentTitle(mStkContext[slotId].mMainCmd.getMenu().title);
            } else {
                notificationBuilder.setContentTitle("");
            }
            notificationBuilder
                    .setSmallIcon(com.android.internal.R.drawable.stat_notify_sim_toolkit);
            notificationBuilder.setContentIntent(pendingIntent);
            notificationBuilder.setOngoing(true);
            // Set text and icon for the status bar and notification body.
            if (mStkContext[slotId].mIdleModeTextCmd.hasIconLoadFailed() ||
                    !msg.iconSelfExplanatory) {
                notificationBuilder.setContentText(msg.text);
                notificationBuilder.setTicker(msg.text);
            }
            if (msg.icon != null) {
                notificationBuilder.setLargeIcon(msg.icon);
            } else {
                Bitmap bitmapIcon = BitmapFactory.decodeResource(StkAppService.this
                    .getResources().getSystem(),
                    com.android.internal.R.drawable.stat_notify_sim_toolkit);
                notificationBuilder.setLargeIcon(bitmapIcon);
            }
            notificationBuilder.setColor(mContext.getResources().getColor(
                    com.android.internal.R.color.system_notification_accent_color));
            mNotificationManager.notify(getNotificationId(slotId), notificationBuilder.build());
        }
    }

    private void launchToneDialog(int slotId) {
        Intent newIntent = new Intent(this, ToneDialog.class);
        String uriString = STK_TONE_URI + slotId;
        Uri uriData = Uri.parse(uriString);
        //Set unique URI to create a new instance of activity for different slotId.
        CatLog.d(LOG_TAG, "launchToneDialog, slotId: " + slotId);
        newIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                | Intent.FLAG_ACTIVITY_NO_HISTORY
                | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS
                | getFlagActivityNoUserAction(InitiatedByUserAction.unknown, slotId));
        newIntent.putExtra("TEXT", mStkContext[slotId].mCurrentCmd.geTextMessage());
        newIntent.putExtra("TONE", mStkContext[slotId].mCurrentCmd.getToneSettings());
        newIntent.putExtra(SLOT_ID, slotId);
        newIntent.setData(uriData);
        startActivity(newIntent);
    }

    private void launchOpenChannelDialog(final int slotId) {
        TextMessage msg = mStkContext[slotId].mCurrentCmd.geTextMessage();
        if (msg == null) {
            CatLog.d(LOG_TAG, "msg is null, return here");
            return;
        }

        msg.title = getResources().getString(R.string.stk_dialog_title);
        if (msg.text == null) {
            msg.text = getResources().getString(R.string.default_open_channel_msg);
        }

        final AlertDialog dialog = new AlertDialog.Builder(mContext)
                    .setIconAttribute(android.R.attr.alertDialogIcon)
                    .setTitle(msg.title)
                    .setMessage(msg.text)
                    .setCancelable(false)
                    .setPositiveButton(getResources().getString(R.string.stk_dialog_accept),
                                       new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            Bundle args = new Bundle();
                            args.putInt(RES_ID, RES_ID_CHOICE);
                            args.putInt(CHOICE, YES);
                            Message message = mServiceHandler.obtainMessage();
                            message.arg1 = OP_RESPONSE;
                            message.arg2 = slotId;
                            message.obj = args;
                            mServiceHandler.sendMessage(message);
                        }
                    })
                    .setNegativeButton(getResources().getString(R.string.stk_dialog_reject),
                                       new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            Bundle args = new Bundle();
                            args.putInt(RES_ID, RES_ID_CHOICE);
                            args.putInt(CHOICE, NO);
                            Message message = mServiceHandler.obtainMessage();
                            message.arg1 = OP_RESPONSE;
                            message.arg2 = slotId;
                            message.obj = args;
                            mServiceHandler.sendMessage(message);
                        }
                    })
                    .create();

        dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        if (!mContext.getResources().getBoolean(
                com.android.internal.R.bool.config_sf_slowBlur)) {
            dialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND);
        }

        dialog.show();
    }

    private void launchTransientEventMessage(int slotId) {
        TextMessage msg = mStkContext[slotId].mCurrentCmd.geTextMessage();
        if (msg == null) {
            CatLog.d(LOG_TAG, "msg is null, return here");
            return;
        }

        msg.title = getResources().getString(R.string.stk_dialog_title);

        final AlertDialog dialog = new AlertDialog.Builder(mContext)
                    .setIconAttribute(android.R.attr.alertDialogIcon)
                    .setTitle(msg.title)
                    .setMessage(msg.text)
                    .setCancelable(false)
                    .setPositiveButton(getResources().getString(android.R.string.ok),
                                       new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                        }
                    })
                    .create();

        dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        if (!mContext.getResources().getBoolean(
                com.android.internal.R.bool.config_sf_slowBlur)) {
            dialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND);
        }

        dialog.show();
    }

    private int getNotificationId(int slotId) {
        int notifyId = STK_NOTIFICATION_ID;
        if (slotId >= 0 && slotId < mSimCount) {
            notifyId += slotId;
        } else {
            CatLog.d(LOG_TAG, "invalid slotId: " + slotId);
        }
        CatLog.d(LOG_TAG, "getNotificationId, slotId: " + slotId + ", notifyId: " + notifyId);
        return notifyId;
    }

    private String getItemName(int itemId, int slotId) {
        Menu menu = mStkContext[slotId].mCurrentCmd.getMenu();
        if (menu == null) {
            return null;
        }
        for (Item item : menu.items) {
            if (item.id == itemId) {
                return item.text;
            }
        }
        return null;
    }

    private boolean removeMenu(int slotId) {
        try {
            if (mStkContext[slotId].mCurrentMenu.items.size() == 1 &&
                mStkContext[slotId].mCurrentMenu.items.get(0) == null) {
                mStkContext[slotId].mSetupMenuState = STATE_NOT_EXIST;
                return true;
            }
        } catch (NullPointerException e) {
            CatLog.d(LOG_TAG, "Unable to get Menu's items size");
            mStkContext[slotId].mSetupMenuState = STATE_NOT_EXIST;
            return true;
        }
        mStkContext[slotId].mSetupMenuState = STATE_EXIST;
        return false;
    }

    StkContext getStkContext(int slotId) {
        if (slotId >= 0 && slotId < mSimCount) {
            return mStkContext[slotId];
        } else {
            CatLog.d(LOG_TAG, "invalid slotId: " + slotId);
            return null;
        }
    }

    private void handleAlphaNotify(Bundle args) {
        String alphaString = args.getString(AppInterfaceSprd.ALPHA_STRING);

        CatLog.d(this, "Alpha string received from card: " + alphaString);
        Toast toast = Toast.makeText(sInstance, alphaString, Toast.LENGTH_LONG);
        toast.setGravity(Gravity.TOP, 0, 0);
        toast.show();
    }


    /* SPRD: STK SETUP CALL feature support @{*/
    private void showIconToast(TextMessage msg) {
        Toast t = new Toast(this);
        ImageView v = new ImageView(this);
        v.setImageBitmap(msg.icon);
        t.setView(v);
        t.setDuration(Toast.LENGTH_LONG);
        t.show();
    }

    private void showTextToast(TextMessage msg, int slotId) {
        msg.title = mStkContext[slotId].lastSelectedItem;

        Toast toast = Toast.makeText(mContext.getApplicationContext(), msg.text,
                Toast.LENGTH_LONG);
        toast.setGravity(Gravity.BOTTOM, 0, 0);
        toast.show();
    }

    private void showIconAndTextToast(TextMessage msg) {
        Toast t = new Toast(this);
        ImageView v = new ImageView(this);
        v.setImageBitmap(msg.icon);
        t.setView(v);
        t.setDuration(Toast.LENGTH_LONG);
        t.show();
    }

    private void launchCallMsg(int slotId) {
        TextMessage msg = mStkContext[slotId].mCurrentCmd.getCallSettings().callMsg;
        if (msg.iconSelfExplanatory == true) {
            // only display Icon.

            if (msg.icon != null) {
                showIconToast(msg);
            } else {
                // do nothing.
                return;
            }
        } else {
            // show text & icon.
            if (msg.icon != null) {
                if (msg.text == null || msg.text.length() == 0) {
                    // show Icon only.
                    showIconToast(msg);
                }
                else {
                    showIconAndTextToast(msg);
                }
            } else {
                if (msg.text == null || msg.text.length() == 0) {
                    // do nothing
                    return;
                } else {
                    showTextToast(msg, slotId);
                }

            }
        }
    }


    private void processNormalResponse(int slotId) {
        CatLog.d(this, "Normal Response PROCESS Start, sim id: " + slotId);
        mStkContext[slotId].mCmdInProgress = false;
        if (mStkContext[slotId].mSetupCallInProcess == false) {
            return;
        }
        mStkContext[slotId].mSetupCallInProcess = false;
        if (mStkContext[slotId].mCurrentCmd == null) {
            CatLog.d(this, "Normal Response PROCESS mCurrentCmd changed to null!");
            return;
        }
        if (mStkContext[slotId].mCurrentCmd.getCmdType() != null) {
            CatLog.d(this, "Normal Response PROCESS end! cmdName[" + mStkContext[slotId].mCurrentCmd.getCmdType().name() + "]");
        }
        CatResponseMessageSprd resMsg = new CatResponseMessageSprd(mStkContext[slotId].mCurrentCmd);
        resMsg.setResultCode(ResultCodeSprd.OK);
        resMsg.setConfirmation(true);
        launchCallMsg(slotId);
        onCmdResponse(resMsg,slotId);
    }

    private void processAbnormalResponse(int slotId) {
        mStkContext[slotId].mCmdInProgress = false;
        CatLog.d(this, "Abnormal Response PROCESS Start");
        if (mStkContext[slotId].mSetupCallInProcess == false) {
            return;
        }
        mStkContext[slotId].mSetupCallInProcess = false;
        CatLog.d(this, "Abnormal Response PROCESS");
        if (mStkContext[slotId].mCurrentCmd == null) {
            return;
        }
        if (mStkContext[slotId].mCurrentCmd.getCmdType() != null) {
            CatLog.d(this, "Abnormal Response PROCESS end! cmdName[" + mStkContext[slotId].mCurrentCmd.getCmdType().name() + "]");
        }
        CatResponseMessageSprd resMsg = new CatResponseMessageSprd(mStkContext[slotId].mCurrentCmd);
        resMsg.setResultCode(ResultCodeSprd.NETWORK_CRNTLY_UNABLE_TO_PROCESS);
        onCmdResponse(resMsg,slotId);
    }

    private void processHoldCallResponse(int slotId) {
        // get Call State.
        Call.State callState = getForegroundCallState(slotId);
        CatLog.d(this, "processHoldCallResponse callState[" + callState + "], sim id: " + slotId);

        switch(callState) {
        case IDLE:
        case HOLDING:
            processNormalResponse(slotId);
            CatLog.d(this, "processHoldCallResponse in Idle or HOLDING");
            break;
        case ACTIVE:
            CatLog.d(this, "processHoldCallResponse in Active ");
            /* SPRD: Delete for bug496885 and bug496825 no need to do by AP.Begin! */
            /*
            try {
                CatLog.d(this, "switchHoldingAndActive");
                Phone phone = getPhone(slotId);
                phone.switchHoldingAndActive();
            } catch (CallStateException ex) {
                CatLog.d(this, " Error: switchHoldingAndActive: caught " + ex);
                processAbnormalResponse(slotId);
            }*/
            CatLog.d(this, " processHoldCallResponse after swicth hold call");
            processNormalResponse(slotId);
            /*SPRD: Delete for bug496885 and bug496825 no need to do by AP.End! */
            break;
        default:
            CatLog.d(this, "processHoldCallResponse in other state");
            processAbnormalResponse(slotId);
            break;
        }
        return;
    }

    private void processEndCallResponse(int slotId) {
        // get Call State.
        Call.State callState = getForegroundCallState(slotId);
        CatLog.d(this, "call State  = " + callState + " ,sim id" + slotId);
        switch(callState) {
        case IDLE:
            processNormalResponse(slotId);
            break;
            // other state
        default:
            // End call
            CatLog.d(this, "End call");
            // 1A1H call
            if (is1A1H(slotId)) {
                try {
                    Phone phone = getPhone(slotId);
                    if (phone == null) {
                        CatLog.d(this, "ERROR: phone is null.");
                        break;
                    }
                    //TODO to hangup all call
                    //phone.hangupAll();
                } catch (Exception ex) {
                    CatLog.d(this, " Error: Call hangup: caught " + ex);
                    processAbnormalResponse(slotId);
                }
            } else {
                /* SPRD: Modify for STK case 27.22.4.13.1/5 begin. @{ */
                /* @orig
                 * SPRD: Remove for STK case 27.22.4.13.1/5. @{
                Phone phone = getPhone(slotId);
                if (phone == null) {
                    CatLog.d(this, "ERROR: phone is null.");
                    break;
                }
                Call fg = phone.getForegroundCall();
                if (fg != null) {
                    try {
                        CatLog.d(this, "End call  " + callState);
                        fg.hangup();
                    } catch (CallStateException ex) {
                        CatLog.d(this, " Error: Call hangup: caught " + ex);
                        processAbnormalResponse(slotId);
                    }
                @} */
                CatLog.d(this, " processEndCallResponse after End call");
                processNormalResponse(slotId);
                /* SPRD: Modify for STK case 27.22.4.13.1/5 end. @} */
            }
            CatLog.d(this, "call Not IDLE  = " + callState);
            break;
        }
    }


    private void processSetupCallResponse(int slotId) {
        CatLog.d(this, "processSetupCallResponse(), sim id: " + slotId);
        int cmdQualifier = mStkContext[slotId].mCurrentCmd.getCmdDet().commandQualifier;
        CatLog.d(this, "processSetupCallResponse() - cmdQualifier[" + cmdQualifier + "]");

        switch (cmdQualifier) {
        case SETUP_CALL_NO_CALL_1:
        case SETUP_CALL_NO_CALL_2:
            processNormalResponse(slotId);
            break;
        case SETUP_CALL_HOLD_CALL_1:
        case SETUP_CALL_HOLD_CALL_2:
            processHoldCallResponse(slotId);
            break;
        case SETUP_CALL_END_CALL_1:
        case SETUP_CALL_END_CALL_2:
            processEndCallResponse(slotId);
            break;
        }
    }





    private Phone getPhone(int slotId) {
        CatLog.d(this, "getPhone slotId: " + slotId);
        return PhoneFactory.getPhone(slotId);
    }

    private Call.State getForegroundCallState(int slotId) {
        CatLog.d(this, "getCallState: " + slotId);
        Phone phone = getPhone(slotId);
        if (phone == null) {
            CatLog.d(this, "getCallState: phone is null.");
            return Call.State.IDLE;
        }
        Call fg = phone.getForegroundCall();
        if (fg != null) {
            CatLog.d(this, "ForegroundCall State: " + fg.getState());
            return fg.getState();
        }
        return Call.State.IDLE;
    }


    private void processAbnormalPhone2BusyResponse(int slotId) {
        mStkContext[slotId].mCmdInProgress = false;
        mStkContext[slotId].mSetupCallInProcess = false;
        CatLog.d(this, "Abnormal No Call Response PROCESS - SIM 2 Call Busy");
        if (mStkContext[slotId].mCurrentCmd == null) {
            return;
        }
        if (mStkContext[slotId].mCurrentCmd.getCmdType() != null) {
            CatLog.d(this, "Abnormal No Call Response PROCESS end - SIM 2 Call Busy! cmdName["
                + mStkContext[slotId].mCurrentCmd.getCmdType().name() + "]");
        }

        CatResponseMessageSprd resMsg = new CatResponseMessageSprd(mStkContext[slotId].mCurrentCmd);
        resMsg.setResultCode(ResultCodeSprd.OK);
        resMsg.setConfirmation(false);
        onCmdResponse(resMsg,slotId);
    }

    private void processAbnormalNoCallResponse(int slotId) {
        mStkContext[slotId].mCmdInProgress = false;
        if (mStkContext[slotId].mSetupCallInProcess == false) {
            return;
        }
        mStkContext[slotId].mSetupCallInProcess = false;
        CatLog.d(this, "Abnormal No Call Response PROCESS");
        if (mStkContext[slotId].mCurrentCmd == null) {
            return;
        }
        if (mStkContext[slotId].mCurrentCmd.getCmdType() != null) {
            CatLog.d(this, "Abnormal No Call Response PROCESS end! cmdName[" + mStkContext[slotId].mCurrentCmd.getCmdType().name() + "]");
        }
        CatResponseMessageSprd resMsg = new CatResponseMessageSprd(mStkContext[slotId].mCurrentCmd);
        resMsg.setResultCode(ResultCodeSprd.TERMINAL_CRNTLY_UNABLE_TO_PROCESS);
        onCmdResponse(resMsg,slotId);
    }

    private Call.State getBackgroundCallState(int slotId) {
        CatLog.d(this, "getBackgroundCallState: " + slotId);
        Phone phone = getPhone(slotId);
        if (phone == null) {
            CatLog.d(this, "getBackgroundCallState: phone is null.");
            return Call.State.IDLE;
        }
        Call bg = phone.getBackgroundCall();

        if (bg != null) {
            CatLog.d(this, "BackgroundCall State: " + bg.getState());
            return bg.getState();
        }
        return Call.State.IDLE;
    }

    private boolean is1A1H(int slotId) {
        Call.State fgState = getForegroundCallState(slotId);
        Call.State bgState = getBackgroundCallState(slotId);
        if (fgState != Call.State.IDLE && bgState != Call.State.IDLE) {
            CatLog.d(this, "1A1H");
            return true;
        }
        return false;
    }

    private Call.State getCallState(int slotId) {

        Phone phone = getPhone(slotId);
        if (phone == null) {
            CatLog.d(this, "getCallState() : phone is null.");
            return Call.State.IDLE;
        }
        Call fg = phone.getForegroundCall();;
        if (fg != null) {
            CatLog.d(this, "getCallState() Phone" + slotId +
                    " ForegroundCall State: " + fg.getState());
            if ((Call.State.IDLE != fg.getState())) {
                return fg.getState();
            }
        }

        Call bg = phone.getBackgroundCall();
        if (bg != null) {
            CatLog.d(this, "getCallState() Phone" + slotId +
                    " BackgroundCall State: " + bg.getState());
            if (Call.State.IDLE != bg.getState()) {
                return bg.getState();
            }
        }

        Call ring = phone.getRingingCall();
        if (ring != null) {
            CatLog.d(this, "getCallState() Phone" + slotId +
                    " RingCall State: " + ring.getState());
            if (Call.State.IDLE != ring.getState()) {
                return ring.getState();
            }
        }

        CatLog.d(this, "getCallState() Phone" + slotId + " State: " + Call.State.IDLE);
        return Call.State.IDLE;
    }

    private void processNoCall(int slotId) {
        // get Call State.
        Call.State callState = getCallState(slotId);
        switch(callState) {
        case IDLE:
        case DISCONNECTED:
            launchConfirmationDialog(mStkContext[slotId].mCurrentCmd.getCallSettings().confirmMsg, slotId);
            break;
        default:
            CatLog.d(this, "Call Abnormal No Call Response");
            processAbnormalNoCallResponse(slotId);
            break;
        }
    }

    private void processHoldCall(int slotId) {
        // Just show the confirm dialog, and add the process when user click OK.
        if (!is1A1H(slotId)) {
            launchConfirmationDialog(mStkContext[slotId].mCurrentCmd.getCallSettings().confirmMsg, slotId);
        } else {
            CatLog.d(this, "Call Abnormal Hold Call Response(has 1A1H calls)");
            processAbnormalNoCallResponse(slotId);
        }
    }

    private void processEndCall(int slotId) {
        // Just show the confirm dialog, and add the process when user click OK.
        launchConfirmationDialog(mStkContext[slotId].mCurrentCmd.getCallSettings().confirmMsg, slotId);
    }

    private void processSetupCall(int slotId) {
        CatLog.d(this, "processSetupCall, sim id: " + slotId);
        int i = 0;
        boolean state_idle = true;

        /*If other sim is busy,we should not start setup call,so we set false to reponse */
        for (i = 0; i < mSimCount; i++) {
            if ((i != slotId) && (Call.State.IDLE != getCallState(i) && Call.State.DISCONNECTED != getCallState(i))) {
                state_idle = false;
                processAbnormalPhone2BusyResponse(slotId);
                CatLog.d(this, "The other sim is not idle, sim id: " + i);
                break;
            }
        }

        if (state_idle) {
            mStkContext[slotId].mSetupCallInProcess = true;
            int cmdQualifier = mStkContext[slotId].mCurrentCmd.getCmdDet().commandQualifier;
            CatLog.d(this, "Qualifier code is " + cmdQualifier);
            /* SPRD: Add here for STK feature. @{ */
            if (StkSetupCallPluginsHelper.getInstance(mContext).needConfirm()) {
                TextMessage cnfMsg = mStkContext[slotId].mCurrentCmd.getCallSettings().confirmMsg;
                if ((cnfMsg != null) && TextUtils.isEmpty(cnfMsg.text)) {
                    cnfMsg.text = getResources().getString(R.string.default_setup_call_msg);
                }
                CatLog.d(this, "SET_UP_CALL cnfMsg.text " + cnfMsg.text);
                //Toast.makeText(mContext.getApplicationContext(), cnfMsg.text, Toast.LENGTH_SHORT).show();
                //processSetupCallResponse(slotId);
                launchConfirmationDialog(cnfMsg, slotId);
                return;
            }
            /* @} */
            switch(cmdQualifier) {
            case SETUP_CALL_NO_CALL_1:
            case SETUP_CALL_NO_CALL_2:
                processNoCall(slotId);
                break;
            case SETUP_CALL_HOLD_CALL_1:
            case SETUP_CALL_HOLD_CALL_2:
                processHoldCall(slotId);
                break;
            case SETUP_CALL_END_CALL_1:
            case SETUP_CALL_END_CALL_2:
                processEndCall(slotId);
                break;
            }
        }
    }
    /* @} */

    /* SPRD: Modify for show normal priority DISPLAY TEXT  and show IDLE MODE TEXT@{ */
    private PhoneConstants.State getCallState() {
        Phone phone = null;
        for (int i = 0; i < mSimCount; i++) {
            phone = getPhone(i);
            if (phone == null) {
                CatLog.d(this, "Phone is null.");
                continue;
            }
            PhoneConstants.State ps = phone.getState();
            CatLog.d(this, "Phone " + i + " state: " + ps);
            if (ps == PhoneConstants.State.RINGING) {
                return PhoneConstants.State.RINGING;
            }
        }
        return PhoneConstants.State.IDLE;
    }

    private boolean isBusyOnCall() {
        PhoneConstants.State s = getCallState();

        CatLog.d(this, "isBusyOnCall: " + s);
        return (s == PhoneConstants.State.RINGING);
    }
    /* @} */

    /* SPRD: add for cucc two icon feature . @{ */
    private boolean isCardReady(Context context, int id) {
        TelephonyManager tm = TelephonyManager.from(context);
        // Check if the card is inserted.
        if (id < 0) {
            return false;
        }
        CatLog.d(this, "tm.getSimState(id)= " + tm.getSimState(id));
        if (tm.getSimState(id) == TelephonyManager.SIM_STATE_READY) {
            if (mStkContext[id] != null && mStkContext[id].mMainCmd != null) {
                CatLog.d(this, "SIM " + id + " is ready.");
                return true;
            }
        } else {
            CatLog.d(this, "SIM " + id + " is not inserted.");
        }
        return false;
    }
    /* @} */
    /* SPRD: add for bug#611551 . @{ */
    private void onCmdResponse(CatResponseMessageSprd resMsg, int slotId){
        if(mStkService[slotId] == null){
            CatLog.d(LOG_TAG, "mStkService[" + slotId + "] is null, reget it from CatServiceSprd");
            mStkService[slotId] = CatServiceSprd.getInstance(slotId);
        }
        if (mStkService[slotId] == null) {
            // This should never happen (we should be responding only to a message
            // that arrived from StkService). It has to exist by this time
            CatLog.d(LOG_TAG, "Exception! mStkService is null when we need to send response.");
        }else{
            mStkService[slotId].onCmdResponse(resMsg);
        }
    }
    /* @} */
}
