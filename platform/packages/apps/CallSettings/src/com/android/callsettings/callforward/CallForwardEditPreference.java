package com.android.callsettings.callforward;

import com.android.ims.ImsManager;
import com.android.ims.ImsException;
import com.android.ims.ImsReasonInfo;
import com.android.internal.telephony.CallForwardInfo;
import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.IccCard;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.SubscriptionController;
import com.android.internal.telephony.uicc.IccRecords;
import android.telephony.TelephonyManager;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;

import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.TypedArray;
import android.content.Intent;
import android.content.pm.UserInfo;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.os.UserHandle;
import android.os.UserManager;
import android.text.BidiFormatter;
import android.text.SpannableString;
import android.text.TextDirectionHeuristics;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

import static com.android.callsettings.TimeConsumingPreferenceActivity.RESPONSE_ERROR;
import static com.android.callsettings.TimeConsumingPreferenceActivity.EXCEPTION_ERROR;
import static com.android.callsettings.TimeConsumingPreferenceActivity.RADIO_OFF_ERROR;
import static com.android.callsettings.TimeConsumingPreferenceActivity.FDN_CHECK_FAILURE;

/* SPRD: function caching edited CF number support. @{ */
import android.telephony.PhoneNumberUtils;
import android.content.SharedPreferences;
import android.widget.EditText;
/* @} */

import com.android.callsettings.callforward.CallForwardTimeUtils;
///* SPRD: function call barring support. @{ */
//import com.sprd.phone.settings.callbarring.TimeConsumingPreferenceListener;
///* @} */
import com.android.ims.internal.ImsCallForwardInfoEx;
import com.android.callsettings.UtExProxy;
import com.android.callsettings.EditPhoneNumberPreference;
import com.android.callsettings.TimeConsumingPreferenceListener;
import com.android.callsettings.R;
import com.android.callsettings.SubscriptionInfoHelper;

import java.util.List;

public class CallForwardEditPreference extends EditPhoneNumberPreference {
    private static final String LOG_TAG = "CallForwardEditPreference";
    private static final boolean DBG = true;

    private static final String SRC_TAGS[]       = {"{0}"};
    private CharSequence mSummaryOnTemplate;
    /**
     * Remembers which button was clicked by a user. If no button is clicked yet, this should have
     * {@link DialogInterface#BUTTON_NEGATIVE}, meaning "cancel".
     *
     * TODO: consider removing this variable and having getButtonClicked() in
     * EditPhoneNumberPreference instead.
     */
    private int mButtonClicked;
    private int mServiceClass;
    private MyHandler mHandler = new MyHandler();
    int reason;
    private Phone mPhone;
    CallForwardInfo callForwardInfo;
    private TimeConsumingPreferenceListener mTcpListener;
    /* SPRD: add for callforward time @{ */
    ImsCallForwardInfoEx mImsCallForwardInfoEx;
    private int mStatus;
    /* @} */
    // SPRD: modify for bug501744
    private static final int VIDEO_CALL_FORWARD = 2;

    /* SPRD: function caching edited CF number support. @{ */
    private Context mContext;
    private SharedPreferences mPrefs;
    private int mPhoneId = 0;
    static final String PREF_PREFIX = "phonecallforward_";
    private EditPhoneNumberPreference.GetDefaultNumberListener mCallForwardListener;
    /* @} */
    // SPRD: modify for bug540943
    private boolean mIsInitializing = false;
    /* SPRD: add for bug552643 @{ */
    private int mSubId = 0;
    private SharedPreferences mTimePrefs;
    static final String PREF_PREFIX_TIME = "phonecalltimeforward_";
    /* @} */
    /* SPRD: add for bug589103 @{ */
    private UserManager mUserManager;
    private TelephonyManager mTelephonyManager;
    private SubscriptionManager mSubscriptionManager;
    private NotificationManager mNotificationManager;
    private static final int[] sVideoCallfwdIcon = new int[] {
            R.drawable.stat_sys_phone_video_call_forward_sub1_ex,
            R.drawable.stat_sys_phone_video_call_forward_sub2_ex,
    };
    static final int VIDEO_CALL_FORWARD_NOTIFICATION = 7;
    /* @} */

    /* SPRD: add for bug626699 @{ */
    private boolean mSpecialOperator = false;
    private static final int SPECIAL_SERVICECLASS = 0;
    /* @} */

    /* SPRD: add for bug639415 @{ */
    private final static int DEF_STATUS = 0;
    private final static int TOOGLE_STATUS = 1;
    /* @} */

    public CallForwardEditPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        /* SPRD: function caching edited CF number support. @{ */
        mContext = context;
        mPrefs = mContext.getSharedPreferences(PREF_PREFIX + mPhoneId, mContext.MODE_PRIVATE);
        /* @} */

        mSummaryOnTemplate = this.getSummaryOn();

        TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.CallForwardEditPreference, 0, R.style.EditPhoneNumberPreference);
        mServiceClass = a.getInt(R.styleable.CallForwardEditPreference_serviceClass,
                CommandsInterface.SERVICE_CLASS_VOICE);
        reason = a.getInt(R.styleable.CallForwardEditPreference_reason,
                CommandsInterface.CF_REASON_UNCONDITIONAL);
        a.recycle();

        if (DBG) Log.d(LOG_TAG, "mServiceClass=" + mServiceClass + ", reason=" + reason);
    }

    public CallForwardEditPreference(Context context) {
        this(context, null);
    }

    /* SPRD: add for bug626699 @{ */
    void init(TimeConsumingPreferenceListener listener, boolean skipReading, Phone phone,
            boolean isSpecialOperator) {
        mSpecialOperator = isSpecialOperator;
        init(listener, skipReading, phone);
    }
    /* @} */

    void init(TimeConsumingPreferenceListener listener, boolean skipReading, Phone phone) {
        mPhone = phone;
        mTcpListener = listener;
        /* SPRD: function caching edited CF number support. @{ */
        mPhoneId = mPhone.getPhoneId();
        UtExProxy.getInstance().setPhoneId(mPhoneId);
        /* SPRD: add for bug552643 @{ */
        mSubId = mPhone.getSubId();
        mTimePrefs = mContext.getSharedPreferences(
                PREF_PREFIX_TIME + mPhoneId, mContext.MODE_PRIVATE);
        /* @} */
        /* SPRD: add for bug589103 @{ */
        mSubscriptionManager = SubscriptionManager.from(mContext);
        mUserManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        mTelephonyManager = (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
        mNotificationManager =
                (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        /* @} */
        mCallForwardListener = new EditPhoneNumberPreference.GetDefaultNumberListener() {
            public String onGetDefaultNumber(EditPhoneNumberPreference preference) {
                /* SPRD: modify for bug 529388 to distinguish voideo number and voice number @{ */
                String number;
                if (GsmUmtsCallForwardOptions.checkVideoCallServiceClass(mServiceClass)) {
                    number = mPrefs.getString(PREF_PREFIX + "Video_" + mPhoneId + "_" + reason, "");
                } else {
                    number = mPrefs.getString(PREF_PREFIX + mPhoneId + "_" + reason, "");
                }
                /* @} */
                return number;
            }
        };
        /* @} */

        if (!skipReading) {
            // SPRD: modify for bug540943
            mIsInitializing = true;
            /* SPRD: add for bug626699 @{ */
            if (mSpecialOperator && ImsManager.isVolteEnabledByPlatform(getContext())) {
                Log.d(LOG_TAG, "init Special Operator");
                UtExProxy.getInstance().getCallForwardingOption(
                        reason,
                        SPECIAL_SERVICECLASS,
                        null,
                        mHandler.obtainMessage(MyHandler.MESSAGE_GET_CFV,
                                // unused in this case
                                CommandsInterface.CF_ACTION_DISABLE,
                                MyHandler.MESSAGE_GET_CFV, null));
            } else {
                /* SPRD: add for callforward time @{ */
                if (ImsManager.isVolteEnabledByPlatform(getContext())
                        && mServiceClass == VIDEO_CALL_FORWARD) {
                    UtExProxy.getInstance().getCallForwardingOption(
                            reason,
                            mServiceClass,
                            null,
                            mHandler.obtainMessage(MyHandler.MESSAGE_GET_CFV,
                                    // unused in this case
                                    CommandsInterface.CF_ACTION_DISABLE,
                                    MyHandler.MESSAGE_GET_CFV, null));
                } else {
                    /* @} */
                    mPhone.getCallForwardingOption(reason, mHandler.obtainMessage(
                            MyHandler.MESSAGE_GET_CF,
                            // unused in this case
                            CommandsInterface.CF_ACTION_DISABLE,
                            MyHandler.MESSAGE_GET_CF, null));
                }
            }
            /* @} */
            if (mTcpListener != null) {
                mTcpListener.onStarted(this, true);
            }
        }
    }

    @Override
    protected void onBindDialogView(View view) {
        // default the button clicked to be the cancel button.
        mButtonClicked = DialogInterface.BUTTON_NEGATIVE;
        super.onBindDialogView(view);
        /* SPRD: function caching edited CF number support. @{ */
        EditText editText = getEditText();
        if (editText != null) {
            // see if there is a means to get a default number, set it accordingly.
            if (mCallForwardListener != null) {
                String defaultNumber = mCallForwardListener.onGetDefaultNumber(this);
                if (defaultNumber != null) {
                    editText.setText(defaultNumber);
                }
            }
        }
        /* @} */
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        super.onClick(dialog, which);
        mButtonClicked = which;
        // SPRD: modify for bug540943
        mIsInitializing = false;
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);

        if (DBG) Log.d(LOG_TAG, "mButtonClicked=" + mButtonClicked
                + ", positiveResult=" + positiveResult);
        // Ignore this event if the user clicked the cancel button, or if the dialog is dismissed
        // without any button being pressed (back button press or click event outside the dialog).
        if (this.mButtonClicked != DialogInterface.BUTTON_NEGATIVE) {
            int action = (isToggled() || (mButtonClicked == DialogInterface.BUTTON_POSITIVE)) ?
                    CommandsInterface.CF_ACTION_REGISTRATION :
                    CommandsInterface.CF_ACTION_DISABLE;
            int time = (reason != CommandsInterface.CF_REASON_NO_REPLY) ? 0 : 20;
            final String number = getPhoneNumber();

            if (DBG) Log.d(LOG_TAG, "callForwardInfo=" + callForwardInfo);

            if (action == CommandsInterface.CF_ACTION_REGISTRATION
                    && callForwardInfo != null
                    && callForwardInfo.status == 1
                    && number.equals(callForwardInfo.number)) {
                // no change, do nothing
                if (DBG) Log.d(LOG_TAG, "no change, do nothing");
            } else {
                // set to network
                if (DBG) Log.d(LOG_TAG, "reason=" + reason + ", action=" + action
                        + ", number=" + number);

                // Display no forwarding number while we're waiting for
                // confirmation
                setSummaryOn("");

                // the interface of Phone.setCallForwardingOption has error:
                // should be action, reason...
                /* SPRD: add for bug626699 @{ */
                if (mSpecialOperator && ImsManager.isVolteEnabledByPlatform(getContext())) {
                    Log.d(LOG_TAG, "set callforward Special Operator");
                    UtExProxy.getInstance().setCallForwardingOption(mPhoneId,
                            action,
                            reason,
                            SPECIAL_SERVICECLASS,
                            number,
                            time,
                            null,
                            mHandler.obtainMessage(MyHandler.MESSAGE_SET_CF,
                            action,
                            MyHandler.MESSAGE_SET_CF));
                } else {
                    /* SPRD: add for callforward time @{ */
                    if (ImsManager.isVolteEnabledByPlatform(getContext())
                            && mServiceClass == VIDEO_CALL_FORWARD) {
                        UtExProxy.getInstance().setCallForwardingOption(mPhoneId,
                                action,
                                reason,
                                mServiceClass,
                                number,
                                time,
                                null,
                                mHandler.obtainMessage(MyHandler.MESSAGE_SET_CF,
                                action,
                                MyHandler.MESSAGE_SET_CF));
                    } else {
                    /* @} */
                        mPhone.setCallForwardingOption(action,
                                reason,
                                number,
                                time,
                                mHandler.obtainMessage(MyHandler.MESSAGE_SET_CF,
                                        action,
                                        MyHandler.MESSAGE_SET_CF));
                    }
                }
                /* @} */
                if (mTcpListener != null) {
                    mTcpListener.onStarted(this, false);
                }
            }
        }
    }

    void handleCallForwardResult(CallForwardInfo cf) {
        callForwardInfo = cf;
        if (DBG) Log.d(LOG_TAG, "handleCallForwardResult done, callForwardInfo=" + callForwardInfo);

        setToggled(callForwardInfo.status == 1);
        setPhoneNumber(callForwardInfo.number);
        mTcpListener.onEnableStatus(CallForwardEditPreference.this, 0);

        /* SPRD: function caching edited CF number support. @{ */
        String numberToCache = callForwardInfo.number;
        if (!TextUtils.isEmpty(callForwardInfo.number)) {
            numberToCache = PhoneNumberUtils.stripSeparators(callForwardInfo.number);
            // SPRD: modify for bug552643
            saveStringPrefs(PREF_PREFIX + mPhoneId + "_" + reason, numberToCache, mPrefs);
        }
        /* @} */
    }

    void handleCallForwardVResult(ImsCallForwardInfoEx cf) {
        mImsCallForwardInfoEx = cf;
        /* SPRD: add for bug639415 @{ */
        if (DBG) {
            Log.d(LOG_TAG, "handleCallForwardVResult done, mImsCallForwardInfoEx ="
                    + mImsCallForwardInfoEx);
        }
        if (mSpecialOperator) {
            if (DBG) {
                Log.d(LOG_TAG, "special operator handleCallForwardVResult done");
            }
            setToggled(mImsCallForwardInfoEx.mStatus == TOOGLE_STATUS);
            setPhoneNumber(mImsCallForwardInfoEx.mNumber);
            mTcpListener.onEnableStatus(CallForwardEditPreference.this, DEF_STATUS);

            String numberToCache = mImsCallForwardInfoEx.mNumber;
            if (!TextUtils.isEmpty(numberToCache)) {
                numberToCache = PhoneNumberUtils.stripSeparators(numberToCache);
                saveStringPrefs(PREF_PREFIX + mPhoneId + "_" + reason, numberToCache, mPrefs);
            }
            return;
        }
        /* @} */
        // SPRD: modify for bug552643
        String mIccId = getIccId(mSubId);
        /*SPRD:add newfuture for bug423432@{*/
        mStatus = mImsCallForwardInfoEx.mStatus;
        if (mImsCallForwardInfoEx.mRuleset != null) {
            if ((mImsCallForwardInfoEx.mServiceClass & CommandsInterface.SERVICE_CLASS_VOICE) != 0) {
                /* SPRD: add for bug552643 @{ */
                String number = mImsCallForwardInfoEx.mNumber;
                String numberToSave = null;
                if (number != null && PhoneNumberUtils.isUriNumber(number)) {
                    numberToSave = number;
                    saveStringPrefs(PREF_PREFIX_TIME + "num_" + mPhoneId, numberToSave, mTimePrefs);
                } else {
                    numberToSave = PhoneNumberUtils.stripSeparators(number);
                    saveStringPrefs(PREF_PREFIX_TIME + "num_" + mPhoneId, numberToSave, mTimePrefs);
                }
                saveStringPrefs(PREF_PREFIX_TIME + "ruleset_" + mPhoneId,
                        mImsCallForwardInfoEx.mRuleset, mTimePrefs);
                saveStringPrefs(PREF_PREFIX_TIME + "status_" + mPhoneId,
                        String.valueOf(mImsCallForwardInfoEx.mStatus), mTimePrefs);
                CallForwardTimeUtils.writeStatus(
                        mContext, mIccId, Integer.valueOf(mImsCallForwardInfoEx.mStatus));
                /* @} */
                setToggled(true);
                setPhoneNumber(mImsCallForwardInfoEx.mNumber);
                mTcpListener.onEnableStatus(CallForwardEditPreference.this, 0);
            }
        } else {
            /* SPRD: add for bug552643 @{ */
            saveStringPrefs(PREF_PREFIX_TIME + "num_" + mPhoneId, "", mTimePrefs);
            saveStringPrefs(PREF_PREFIX_TIME + "ruleset_" + mPhoneId, "", mTimePrefs);
            saveStringPrefs(PREF_PREFIX_TIME + "status_" + mPhoneId, "", mTimePrefs);
            CallForwardTimeUtils.writeStatus(mContext, mIccId, 0);
            /* @} */
            if ((mImsCallForwardInfoEx.mServiceClass & CommandsInterface.SERVICE_CLASS_VOICE) == 0) {
                setToggled(mImsCallForwardInfoEx.mStatus == 1);
                setPhoneNumber(mImsCallForwardInfoEx.mNumber);
            }

            if (GsmUmtsCallForwardOptions
                    .checkVideoCallServiceClass(mImsCallForwardInfoEx.mServiceClass)) {
                /* @} */
                mTcpListener.onUpdateTwinsPref((mImsCallForwardInfoEx.mStatus == 1),
                        mImsCallForwardInfoEx.mServiceClass,
                        reason, mImsCallForwardInfoEx.mNumber, null);
            }
        }

        if (!TextUtils.isEmpty(mImsCallForwardInfoEx.mNumber)) {
            if (GsmUmtsCallForwardOptions.checkVideoCallServiceClass(mImsCallForwardInfoEx.mServiceClass)) {
                if (PhoneNumberUtils.isUriNumber(mImsCallForwardInfoEx.mNumber)) {
                    // SPRD: modify for bug552643
                    saveStringPrefs(PREF_PREFIX + "Video_" + mPhoneId + "_" + reason,
                           mImsCallForwardInfoEx.mNumber, mPrefs);
                } else {
                    // SPRD: modify for bug552643
                    saveStringPrefs(PREF_PREFIX + "Video_" + mPhoneId + "_" + reason,
                            PhoneNumberUtils.stripSeparators(mImsCallForwardInfoEx.mNumber), mPrefs);
                }
            }
        }

        /* SPRD: add for bug589103 @{ */
        if (mImsCallForwardInfoEx.mCondition == CommandsInterface.CF_REASON_UNCONDITIONAL
                &&  (mImsCallForwardInfoEx.mServiceClass
                        & CommandsInterface.SERVICE_CLASS_VOICE) == 0) {
            updateVideoCfi(mSubId, (mImsCallForwardInfoEx.mStatus == 1));
        }
        /* @} */
    }

    /* SPRD: add for bug552643 @{ */
    private String getIccId(int subId) {
        String iccId = "";
        int phoneId = SubscriptionController.getInstance().getPhoneId(subId);
        Phone phone = PhoneFactory.getPhone(phoneId);
        if (phone != null) {
            IccCard iccCard = phone.getIccCard();
            if (iccCard != null) {
                IccRecords iccRecords = iccCard.getIccRecords();
                if (iccRecords != null) {
                    iccId = iccRecords.getIccId();
                }
            }
        }
        return iccId;
    }
    /* @} */

    public void updateSummaryText() {
        if (isToggled()) {
            final String number = getRawPhoneNumber();
            if (number != null && number.length() > 0) {
                // Wrap the number to preserve presentation in RTL languages.
                String wrappedNumber = BidiFormatter.getInstance().unicodeWrap(
                        number, TextDirectionHeuristics.LTR);
                String values[] = { wrappedNumber };
                String summaryOn = String.valueOf(
                        TextUtils.replace(mSummaryOnTemplate, SRC_TAGS, values));
                int start = summaryOn.indexOf(wrappedNumber);

                SpannableString spannableSummaryOn = new SpannableString(summaryOn);
                PhoneNumberUtils.addTtsSpan(spannableSummaryOn,
                        start, start + wrappedNumber.length());
                setSummaryOn(spannableSummaryOn);
            } else {
                setSummaryOn(getContext().getString(R.string.sum_cfu_enabled_no_number));
            }
        }

    }

    // Message protocol:
    // what: get vs. set
    // arg1: action -- register vs. disable
    // arg2: get vs. set for the preceding request
    private class MyHandler extends Handler {
        static final int MESSAGE_GET_CF = 0;
        static final int MESSAGE_SET_CF = 1;
        static final int MESSAGE_GET_CFV = 2;

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MESSAGE_GET_CF:
                    handleGetCFResponse(msg);
                    break;
                case MESSAGE_GET_CFV:
                    handleGetCFVResponse(msg);
                    break;
                case MESSAGE_SET_CF:
                    handleSetCFResponse(msg);
                    break;
            }
        }

        private void handleGetCFResponse(Message msg) {
            if (DBG) Log.d(LOG_TAG, "handleGetCFResponse: done");

            mTcpListener.onFinished(CallForwardEditPreference.this, msg.arg2 != MESSAGE_SET_CF);

            AsyncResult ar = (AsyncResult) msg.obj;

            callForwardInfo = null;
            if (ar.exception != null) {
                setToggled(false);
                if (DBG) Log.d(LOG_TAG, "handleGetCFResponse: ar.exception=" + ar.exception);
                if (ar.exception instanceof CommandException) {
                    CommandException e = (CommandException) ar.exception;
                    if (e.getMessage() != null && e.getMessage().contains("code")) {
                        int length = e.getMessage().length();
                        try {
                            int errorCode = Integer.parseInt(
                                    e.getMessage().substring(length-3, length));
                            if (DBG) Log.d(LOG_TAG, "handleGetCFResponse: errorCode = " + errorCode);
                            if (errorCode == ImsReasonInfo.CODE_FDN_BLOCKED) {
                                mIsInitializing = false;
                                mTcpListener.onError(CallForwardEditPreference.this,
                                        FDN_CHECK_FAILURE);
                            } else if (errorCode == ImsReasonInfo.CODE_FDN_BLOCKED) {
                                mIsInitializing = false;
                                mTcpListener.onError(CallForwardEditPreference.this,
                                        RADIO_OFF_ERROR);
                            } else {
                                mTcpListener.onError(CallForwardEditPreference.this,
                                        EXCEPTION_ERROR);
                            }
                        } catch (Exception error) {
                            error.printStackTrace();
                        }
                        return;
                    }
                    if (e.getCommandError() == CommandException.Error.FDN_CHECK_FAILURE
                            || e.getCommandError()
                            == CommandException.Error.RADIO_NOT_AVAILABLE) {
                        mIsInitializing = false;
                    }
                    mTcpListener.onException(CallForwardEditPreference.this,
                            (CommandException) ar.exception);
                } else {
                    // Most likely an ImsException and we can't handle it the same way as
                    // a CommandException. The best we can do is to handle the exception
                    // the same way as mTcpListener.onException() does when it is not of type
                    // FDN_CHECK_FAILURE.
                    mTcpListener.onError(CallForwardEditPreference.this, EXCEPTION_ERROR);
                }
            } else {
                if (ar.userObj instanceof Throwable) {
                    mTcpListener.onError(CallForwardEditPreference.this, RESPONSE_ERROR);
                }
                CallForwardInfo cfInfoArray[] = (CallForwardInfo[]) ar.result;
                if (cfInfoArray.length == 0) {
                    if (DBG) Log.d(LOG_TAG, "handleGetCFResponse: cfInfoArray.length==0");
                    setEnabled(false);
                    mTcpListener.onError(CallForwardEditPreference.this, RESPONSE_ERROR);
                } else {
                    for (int i = 0, length = cfInfoArray.length; i < length; i++) {
                        if (DBG) Log.d(LOG_TAG, "handleGetCFResponse, cfInfoArray[" + i + "]="
                                + cfInfoArray[i]);
                        if (GsmUmtsCallForwardOptions
                                .checkServiceClassSupport(cfInfoArray[i].serviceClass)
                                && (cfInfoArray[i].serviceClass
                                            & CommandsInterface.SERVICE_CLASS_VOICE) != 0) {
                            // corresponding class
                            CallForwardInfo info = cfInfoArray[i];
                            handleCallForwardResult(info);

                            // Show an alert if we got a success response but
                            // with unexpected values.
                            // Currently only handle the fail-to-disable case
                            // since we haven't observed fail-to-enable.
                            if (msg.arg2 == MESSAGE_SET_CF &&
                                    msg.arg1 == CommandsInterface.CF_ACTION_DISABLE &&
                                    info.status == 1) {
                                CharSequence s;
                                switch (reason) {
                                    case CommandsInterface.CF_REASON_BUSY:
                                        s = getContext().getText(R.string.disable_cfb_forbidden);
                                        break;
                                    case CommandsInterface.CF_REASON_NO_REPLY:
                                        s = getContext().getText(R.string.disable_cfnry_forbidden);
                                        break;
                                    default: // not reachable
                                        s = getContext().getText(R.string.disable_cfnrc_forbidden);
                                }
                                AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
                                builder.setNeutralButton(R.string.close_dialog, null);
                                builder.setTitle(getContext().getText(R.string.error_updating_title));
                                builder.setMessage(s);
                                builder.setCancelable(true);
                                builder.create().show();
                            }
                        }
                    }
                }
            }

            // Now whether or not we got a new number, reset our enabled
            // summary text since it may have been replaced by an empty
            // placeholder.
            updateSummaryText();
        }

        private void handleGetCFVResponse(Message msg) {
            if (DBG) Log.d(LOG_TAG, "handleGetCFVResponse: done");

            mTcpListener.onFinished(CallForwardEditPreference.this, msg.arg2 != MESSAGE_SET_CF);

            AsyncResult ar = (AsyncResult) msg.obj;

            mImsCallForwardInfoEx = null;
            if (ar.exception != null) {
                if (DBG) Log.d(LOG_TAG, "handleGetCFVResponse: ar.exception=" + ar.exception);
                setToggled(false);
                if (ar.exception instanceof CommandException) {
                    mTcpListener.onException(CallForwardEditPreference.this,
                            (CommandException) ar.exception);
                } else if (ar.exception instanceof ImsException) {
                    ImsException e = (ImsException) ar.exception;
                    if (e.getCode() == ImsReasonInfo.CODE_UT_SERVICE_UNAVAILABLE) {
                        mIsInitializing = false;
                        mTcpListener.onError(CallForwardEditPreference.this, RADIO_OFF_ERROR);
                    } else if (e.getCode() == ImsReasonInfo.CODE_FDN_BLOCKED) {
                        mIsInitializing = false;
                        mTcpListener.onError(CallForwardEditPreference.this, FDN_CHECK_FAILURE);
                    } else {
                        mTcpListener.onError(CallForwardEditPreference.this, EXCEPTION_ERROR);
                    }
                } else {
                    // Most likely an ImsException and we can't handle it the same way as
                    // a CommandException. The best we can do is to handle the exception
                    // the same way as mTcpListener.onException() does when it is not of type
                    // FDN_CHECK_FAILURE.
                    mTcpListener.onError(CallForwardEditPreference.this, EXCEPTION_ERROR);
                }
            } else {
                if (ar.userObj instanceof Throwable) {
                    mTcpListener.onError(CallForwardEditPreference.this, RESPONSE_ERROR);
                }
                ImsCallForwardInfoEx cfInfoArray[] = (ImsCallForwardInfoEx[]) ar.result;
                if (cfInfoArray.length == 0) {
                    if (DBG) Log.d(LOG_TAG, "handleGetCFVResponse: cfInfoArray.length==0");
                    setEnabled(false);
                    mTcpListener.onError(CallForwardEditPreference.this, RESPONSE_ERROR);
                } else {
                    for (int i = 0, length = cfInfoArray.length; i < length; i++) {
                        if (DBG) Log.d(LOG_TAG, "handleGetCFVResponse, cfInfoArray[" + i + "]="
                                + cfInfoArray[i]);
                        if (GsmUmtsCallForwardOptions
                                .checkServiceClassSupport(cfInfoArray[i].mServiceClass)) {
                            // corresponding class
                            ImsCallForwardInfoEx info = cfInfoArray[i];
                            handleCallForwardVResult(info);

                            // Show an alert if we got a success response but
                            // with unexpected values.
                            // Currently only handle the fail-to-disable case
                            // since we haven't observed fail-to-enable.
                            if (msg.arg2 == MESSAGE_SET_CF &&
                                    msg.arg1 == CommandsInterface.CF_ACTION_DISABLE &&
                                    info.mStatus == 1) {
                                CharSequence s;
                                switch (reason) {
                                    case CommandsInterface.CF_REASON_BUSY:
                                        s = getContext().getText(R.string.disable_cfb_forbidden);
                                        break;
                                    case CommandsInterface.CF_REASON_NO_REPLY:
                                        s = getContext().getText(R.string.disable_cfnry_forbidden);
                                        break;
                                    default: // not reachable
                                        s = getContext().getText(R.string.disable_cfnrc_forbidden);
                                }
                                AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
                                builder.setNeutralButton(R.string.close_dialog, null);
                                builder.setTitle(getContext().getText(R.string.error_updating_title));
                                builder.setMessage(s);
                                builder.setCancelable(true);
                                builder.create().show();
                            }
                        }
                    }
                }
            }

            // Now whether or not we got a new number, reset our enabled
            // summary text since it may have been replaced by an empty
            // placeholder.
            updateSummaryText();
        }

        private void handleSetCFResponse(Message msg) {
            AsyncResult ar = (AsyncResult) msg.obj;

            if (ar.exception != null) {
                if (DBG) Log.d(LOG_TAG, "handleSetCFResponse: ar.exception=" + ar.exception);
                // setEnabled(false);
            }
            if (DBG) Log.d(LOG_TAG, "handleSetCFResponse: re get");
            /* SPRD: add for bug626699 @{ */
            if (mSpecialOperator && ImsManager.isVolteEnabledByPlatform(getContext())) {
                Log.d(LOG_TAG, "query callforward Special Operator");
                UtExProxy.getInstance().getCallForwardingOption(reason, SPECIAL_SERVICECLASS, null,
                        obtainMessage(MESSAGE_GET_CFV, msg.arg1, MESSAGE_SET_CF, ar.exception));
            } else {
                /* SPRD: add for callforward time @{ */
                if (ImsManager.isVolteEnabledByPlatform(getContext())
                        && mServiceClass == CommandsInterface.SERVICE_CLASS_DATA) {
                    if (DBG) {
                        Log.d(LOG_TAG, "mServiceClass ： " + mServiceClass);
                    }
                    UtExProxy.getInstance().getCallForwardingOption(reason, mServiceClass, null,
                            obtainMessage(MESSAGE_GET_CFV, msg.arg1, MESSAGE_SET_CF, ar.exception));
                } else {
                    if (DBG) Log.d(LOG_TAG, "else mServiceClass : " + mServiceClass);
                    mPhone.getCallForwardingOption(reason,
                            obtainMessage(MESSAGE_GET_CF, msg.arg1, MESSAGE_SET_CF, ar.exception));
                }
                /* @} */
            }
            /* @} */
        }
    }

    /* SPRD: function caching edited CF number support. & bug552643 @{ */
    void saveStringPrefs(String key, String value, SharedPreferences prefs) {
        Log.w(LOG_TAG, "saveStringPrefs(" + key + ", " + value + ")");
        try {
            SharedPreferences.Editor editor = prefs.edit();
            editor.putString(key, value);
            editor.apply();
        } catch (Exception e) {
            Log.e(LOG_TAG, "Exception happen.");
        }
    }
    /* @} */

    public int getServiceClass() {
        return mServiceClass;
    }

    public int getReason() {
        return reason;
    }

    public int getStatus() {
        return mStatus;
    }

    /* SPRD: add for bug495303 @{ */
    public boolean isInitializing() {
        return mIsInitializing;
    }
    /* @} */

    void initCallTimeForward() {
        UtExProxy.getInstance().setPhoneId(mPhoneId);

        if (ImsManager.isVolteEnabledByPlatform(getContext())) {
            UtExProxy.getInstance().getCallForwardingOption(
                    CommandsInterface.CF_REASON_UNCONDITIONAL,
                    CommandsInterface.SERVICE_CLASS_VOICE,
                    null,
                    mHandler.obtainMessage(MyHandler.MESSAGE_GET_CFV,
                            // unused in this case
                            CommandsInterface.CF_ACTION_DISABLE,
                            MyHandler.MESSAGE_GET_CFV, null));
        }
        if (mTcpListener != null) {
            mTcpListener.onStarted(this, true);
        }
    }

    /* SPRD: add for bug589103 @{ */
    /**
     * Updates the message video call forwarding indicator notification.
     *
     * @param visible true if there are messages waiting
     */
    void updateVideoCfi(int subId, boolean visible) {
        if (DBG) Log.d(LOG_TAG, "updateVideoCfi(): " + visible + " subId: " + subId);
        if (visible) {
            SubscriptionInfo subInfo = mSubscriptionManager.getActiveSubscriptionInfo(subId);
            if (subInfo == null) {
                Log.w(LOG_TAG, "Found null subscription info for: " + subId);
                return;
            }
            boolean isMultiSimEnabled = mTelephonyManager.isMultiSimEnabled();
            int iconId;
            if (isMultiSimEnabled) {
                iconId = sVideoCallfwdIcon[mPhoneId];
            } else {
                iconId = R.drawable.stat_sys_phone_video_call_forward;
            }
            String notificationTitle;
            if (mTelephonyManager.getPhoneCount() > 1) {
                notificationTitle = subInfo.getDisplayName().toString();
            } else {
                notificationTitle = mContext.getString(R.string.labelVideoCF);
            }

            Notification.Builder builder = new Notification.Builder(mContext)
                    .setSmallIcon(iconId)
                    .setColor(subInfo.getIconTint())
                    .setContentTitle(notificationTitle)
                    .setContentText(mContext.getString(R.string.sum_vcfu_enabled_indicator))
                    .setShowWhen(false)
                    .setOngoing(true);

            Intent intent = new Intent(Intent.ACTION_MAIN);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
            intent.setClassName("com.android.phone", "com.android.phone.CallFeaturesSetting");
            SubscriptionInfoHelper.addExtrasToIntent(
                    intent, mSubscriptionManager.getActiveSubscriptionInfo(subId));
            PendingIntent contentIntent =
                    PendingIntent.getActivity(mContext, subId /* requestCode */, intent, 0);

            List<UserInfo> users = mUserManager.getUsers(true);
            for (int i = 0; i < users.size(); i++) {
                final UserInfo user = users.get(i);
                if (user.isManagedProfile()) {
                    continue;
                }
                UserHandle userHandle = user.getUserHandle();
                builder.setContentIntent(userHandle.isOwner() ? contentIntent : null);
                mNotificationManager.notifyAsUser(
                        Integer.toString(subId) /* tag */,
                        VIDEO_CALL_FORWARD_NOTIFICATION,
                        builder.build(),
                        userHandle);
            }
        } else {
            mNotificationManager.cancelAsUser(
                    Integer.toString(subId) /* tag */,
                    VIDEO_CALL_FORWARD_NOTIFICATION,
                    UserHandle.ALL);
        }
    }
    /* @} */
}
