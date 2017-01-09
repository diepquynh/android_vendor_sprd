
package com.android.callsettings.callbarring;

import com.android.callsettings.TimeConsumingPreferenceListener;
import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.Phone;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import com.android.callsettings.R;
import com.android.internal.telephony.GsmCdmaPhoneEx;

import static com.android.callsettings.TimeConsumingPreferenceActivity.EXCEPTION_ERROR;
import static com.android.callsettings.TimeConsumingPreferenceActivity.RESPONSE_ERROR;

public class CallBarringChgPwdPreference extends EditPassWordPreference {
    private static final String LOG_TAG = "CallBarringChgPwdPreference";
    private static final boolean DBG = true;// Debug.isDebug();

    private static final boolean ENABLE_RIL = true;
    private MyHandler mHandler = new MyHandler();
    GsmCdmaPhoneEx mPhone;
    CallBarringInfo callBarringInfo;
    TimeConsumingPreferenceListener tcpListener;

    private static final int TIME_OLD = 1;
    private static final int TIME_NEW_FIRST = 2;
    private static final int TIME_NEW_LAST = 3;

    private int mTimes = 0;
    private String mOldPwd;
    private String mNewPwd;

    public CallBarringChgPwdPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        // phone = PhoneFactory.getDefaultPhone();
        setEnableText(context.getString(R.string.ok));
    }

    public CallBarringChgPwdPreference(Context context) {
        this(context, null);
    }

    public void init(TimeConsumingPreferenceListener listener, boolean skipReading, Phone phone) {
        // getting selected subscription
        if (DBG)
            Log.d(LOG_TAG, "Getting CallBarringChgPwdPreference....Phone:" + phone);
        mPhone = (GsmCdmaPhoneEx) phone;
        tcpListener = listener;
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        super.onClick(dialog, which);
    }

    @Override
    protected void onClick() {
        Log.d(LOG_TAG, "onClick()");
        mTimes = 0;
        setDialogTitle(R.string.DlgChgPwd_oldpwd);
        super.onClick();
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);
        EditText editText = getEditText();
        editText.setText(null);
        setDialogMessage(null);
        mTimes++;
        Log.d(LOG_TAG, "onBindDialogView(), mTimes: " + mTimes);

        switch (mTimes) {
            case TIME_OLD:
                setDialogTitle(R.string.DlgChgPwd_newpwd_first);
                break;
            case TIME_NEW_FIRST:
                setDialogTitle(R.string.DlgChgPwd_newpwd_last);
                break;
        }
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);
        if (positiveResult) {
            String txt = getEditText().getText().toString();
            Log.d(LOG_TAG, "onDialogClosed(), mTimes: " + mTimes);
            if (mTimes == TIME_NEW_FIRST) {
                mNewPwd = txt;
                Log.d(LOG_TAG, "new password: " + mNewPwd);
            }

            if ((mTimes == TIME_OLD) || (mTimes == TIME_NEW_FIRST)) {
                if (mTimes == TIME_OLD) {
                    Log.d(LOG_TAG, "old password: " + mNewPwd);
                    mOldPwd = txt;
                }
                showDialog(null);
                return;
            }
            if (mTimes == TIME_NEW_LAST) {
                if (mNewPwd.compareTo(txt) != 0) {
                    Log.d(LOG_TAG, "twice password don't match");
                    // mTimes = 0;
                    CharSequence s = null;
                    s = getContext().getText(R.string.cb_pw_no_match);
                    if (s != null) {
                        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
                        builder.setNeutralButton(R.string.close_dialog, null);
                        builder.setTitle(getContext().getText(R.string.error_updating_title));
                        builder.setMessage(s);
                        builder.setCancelable(true);
                        builder.create().show();
                    }
                    return;
                } else {
                    Log.d(LOG_TAG, "twice password match");
                    // mTimes = 0;
                    if (ENABLE_RIL) {
                        mPhone.changeBarringPassword("AB", mOldPwd, mNewPwd,
                                mHandler.obtainMessage(MyHandler.MESSAGE_CHG_LOCK_PWD,
                                        0, 0));
                    } else {
                        mHandler.sendMessageDelayed(
                                mHandler.obtainMessage(MyHandler.MESSAGE_CHG_LOCK_PWD,
                                        0, 0), 300);
                    }
                }
            }
        }
        tcpListener.onFinished(CallBarringChgPwdPreference.this, false);
    }

    void handleCallBarringResult(CallBarringInfo cf) {
        callBarringInfo = cf;
        if (DBG)
            Log.d(LOG_TAG, "handleCallBarringResult done, callBarringInfo=" + callBarringInfo);

        setToggled(callBarringInfo.status == 1);
        setPassWord(callBarringInfo.password);
    }

    // Message protocol:
    // what: get vs. set
    // arg1: action -- register vs. disable
    // arg2: get vs. set for the preceding request
    private class MyHandler extends Handler {
        private static final int MESSAGE_CHG_LOCK_PWD = 1;

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MESSAGE_CHG_LOCK_PWD:
                    handleChgPwdResponse(msg);
                    break;
            }
        }

        private void handleChgPwdResponse(Message msg) {
            if (DBG)
                Log.d(LOG_TAG, "handleChgPwdResponse: done");
            if (msg.arg2 == MESSAGE_CHG_LOCK_PWD) {
                tcpListener.onFinished(CallBarringChgPwdPreference.this, false);
            }
            AsyncResult ar = (AsyncResult) msg.obj;
            CharSequence s = null;
            callBarringInfo = null;
            if (ar.exception != null) {
                if (DBG)
                    Log.d(LOG_TAG, "handleGetCFResponse: ar.exception=" + ar.exception);
                // add by chengyake for NEWMS00135138 Monday, October 31 2011 begin
                // setEnabled(false);
                // add by chengyake for NEWMS00135138 Monday, October 31 2011 end
                if (ar.exception instanceof CommandException) {
                    CommandException.Error err = ((CommandException) (ar.exception))
                            .getCommandError();
                    if (err == CommandException.Error.PASSWORD_INCORRECT) {
                        if (DBG)
                            Log.d(LOG_TAG, "handleChgPwdResponse: INCORRECT PASSWORD");
                        s = getContext().getText(R.string.cb_pw_error);
                        if (s != null) {
                            AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
                            builder.setNeutralButton(R.string.close_dialog, null);
                            builder.setTitle(getContext().getText(R.string.error_updating_title));
                            builder.setMessage(s);
                            builder.setCancelable(true);
                            builder.create().show();
                        }
                    } else {
                        if (DBG)
                            Log.d(LOG_TAG, "handleChgPwdResponse:@ EXCEPTION_ERROR");
                        tcpListener.onError(CallBarringChgPwdPreference.this, EXCEPTION_ERROR);
                    }
                } else {
                    if (DBG)
                        Log.d(LOG_TAG, "handleChgPwdResponse:# EXCEPTION_ERROR");
                    tcpListener.onError(CallBarringChgPwdPreference.this, EXCEPTION_ERROR);
                }
            } else {
                if (ar.userObj instanceof Throwable) {
                    tcpListener.onError(CallBarringChgPwdPreference.this, RESPONSE_ERROR);
                } else {
                    if (DBG)
                        Log.d(LOG_TAG, "handleChgPwdResponse:CORRECT PASSWORD");
                    s = getContext().getText(R.string.cb_pw_correct);
                    if (s != null) {
                        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
                        builder.setNeutralButton(R.string.close_dialog, null);
                        builder.setTitle(getContext().getText(R.string.call_settings));
                        builder.setMessage(s);
                        builder.setCancelable(true);
                        builder.create().show();
                    }
                }
            }
        }
    }
}
