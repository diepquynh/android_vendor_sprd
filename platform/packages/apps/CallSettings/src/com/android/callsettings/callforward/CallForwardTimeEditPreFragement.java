package com.android.callsettings.callforward;

import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Set;

import android.app.TimePickerDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.text.TextUtils;
import android.util.Log;
import android.widget.TimePicker;

import com.android.callsettings.EditPhoneNumberPreference;
import com.android.ims.internal.ImsCallForwardInfoEx;
import com.android.internal.telephony.IccCard;
import com.android.internal.telephony.uicc.IccRecords;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.SubscriptionController;

import com.android.internal.telephony.PhoneFactory;
import com.google.android.collect.Sets;
import com.google.common.base.Preconditions;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.telephony.PhoneNumberUtils;
import android.widget.Toast;
import com.android.callsettings.R;
import com.android.callsettings.UtExProxy;


public class CallForwardTimeEditPreFragement extends PreferenceFragment
        implements TimePickerDialog.OnTimeSetListener,
        EditPhoneNumberPreference.OnDialogClosedListener {

    private static final String LOG_TAG = CallForwardTimeEditPreFragement.class.getSimpleName();
    private static final String SRC_TAGS[] = {"{0}"};
    private static final String ACTION_CALLFORWARD_TIME_START =
            "com.android.phone.ACTION_CALLFORWARD_TIME_START";
    private static final String ACTION_CALLFORWARD_TIME_STOP =
            "com.android.phone.ACTION_CALLFORWARD_TIME_STOP";
    private static final String ICC_ID_KEY = "icc_id_key";
    /* SPRD: add for bug560757 @{ */
    private static final String IS_TIME_EDIT_DIALOG_SHOWING = "is_time_edit_dialog_showing";
    private static final String TIME_EDIT_PHONE_NUMBER = "time_edit_phone_number";
    /* @} */

    private Preference mStartTimePref;
    private Preference mStopTimePref;
    private EditPhoneNumberPreference mNumberEditPref;

    private boolean mIsStopTimeSetting;
    private static final int STARTTIME_SET = 1 << 0;
    private static final int STOPTIME_SET = 1 << 1;
    private static final int NUMBER_SET = 1 << 2;
    private int mSetNumber = 0;
    private int mSubId = 0;
    String mIccId;
    Context mContext;
    /* SPRD: add for callforward time @{ */
    private int mPhoneId = 0;
    private static final boolean DBG = true;
    Phone mPhone;
    private MyHandler mHandler = new MyHandler();
    String mStartTime;
    String mStopTime;
    String timeRule;
    private static final Set<Listener> mListeners = Sets.newArraySet();
    SharedPreferences mPrefs;
    static final String PREF_PREFIX = "phonecalltimeforward_";
    /* @} */
    /* SPRD: Add for bug 524927 @{ */
    private TimePickerDialog mTimePickerDialog = null;
    private TimePicker mTimePicker;
    private int mCurrentHour;
    private int mCurrentMinute;
    /* @} */

    public CallForwardTimeEditPreFragement() {
    }

    public CallForwardTimeEditPreFragement(Context context, int phoneid) {
        mContext = context;
        // SPRD: add for bug421361
        mPhoneId = phoneid;
        mPhone = PhoneFactory.getPhone(mPhoneId);
        /* SPRD: add for bug 478879 @{ */
        mSubId = mPhone.getSubId();
        mIccId = getIccId(mSubId);
        /* @} */
        mPrefs = mContext.getSharedPreferences(PREF_PREFIX + mPhoneId,
                mContext.MODE_PRIVATE);
        UtExProxy.getInstance().setPhoneId(mPhoneId);
    }

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        /* SPRD: add for bug 527266 @{ */
        if (mContext == null) {
            return;
        }
        /* @} */

        addPreferencesFromResource(R.xml.callforward_time_setting_ex);
        mStartTimePref = findPreference("start_time");
        mStopTimePref = findPreference("stop_time");
        mNumberEditPref = (EditPhoneNumberPreference) findPreference("cft_number_key");
        mNumberEditPref.setDialogOnClosedListener(this);

        /* SPRD: add for callforward time @{ */
        if (mPrefs.getString(PREF_PREFIX + "ruleset_" + mPhoneId, "") != "") {
            updateTimeSummary(
                    mPrefs.getString(PREF_PREFIX + "ruleset_" + mPhoneId, "").substring(4, 9), true);
            updateTimeSummary(
                    mPrefs.getString(PREF_PREFIX + "ruleset_" + mPhoneId, "").substring(10, 15),
                    false);
            mStartTime = mPrefs.getString(PREF_PREFIX + "ruleset_" + mPhoneId, "").substring(4, 9);
            mStopTime = mPrefs.getString(PREF_PREFIX + "ruleset_" + mPhoneId, "").substring(10, 15);
        }
        updateNumberSummary(mPrefs.getString(PREF_PREFIX + "num_" + mPhoneId, ""));
        /* @} */
        /* SPRD: add for bug560757 @{ */
        Bundle argument = this.getArguments();
        if(argument != null && argument.getBoolean(IS_TIME_EDIT_DIALOG_SHOWING)) {
            String editPhoneNumber = argument.getString(TIME_EDIT_PHONE_NUMBER);
            if (!TextUtils.isEmpty(editPhoneNumber)) {
                mNumberEditPref.setPhoneNumber(editPhoneNumber);
            }
            mNumberEditPref.showPhoneNumberDialog();
        }
        /* @} */
    }
    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {

        if (preference == mStartTimePref || preference == mStopTimePref) {
            final Calendar calendar = Calendar.getInstance();
            mTimePickerDialog = new TimePickerDialog(
                    getActivity(),
                    this,
                    calendar.get(Calendar.HOUR_OF_DAY),
                    calendar.get(Calendar.MINUTE),
                    true);
            mTimePickerDialog.show();
            if (preference == mStopTimePref) {
                mIsStopTimeSetting = true;
            }
        } else if (preference == mNumberEditPref) {
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    @Override
    public void onTimeSet(TimePicker view, int hourOfDay, int minute) {

        log("onTimeSet hourOfDay = " + hourOfDay + "  minute = " + minute);
        Calendar c = Calendar.getInstance();
        c.set(Calendar.HOUR_OF_DAY, hourOfDay);
        c.set(Calendar.MINUTE, minute);

        SimpleDateFormat mSimpleFmt = new SimpleDateFormat("HH:mm");
        Date curDate = new Date(c.getTimeInMillis());
        String hhmm = mSimpleFmt.format(curDate);

        if (mIsStopTimeSetting) {
            mStopTime = hhmm;
            mStopTimePref.setSummary(mStopTime);
            updateTimeSummary(mStopTime, false);
            mIsStopTimeSetting = false;
        } else {
            mStartTime = hhmm;
            updateTimeSummary(mStartTime, true);
            mStartTimePref.setSummary(mStartTime);
        }
        // SPRD: add for bug 526019
        notifySetNumberChange();
    }

    @Override
    public void onDialogClosed(EditPhoneNumberPreference preference, int buttonClicked) {

        log("onDialogClosed: request preference click on dialog close: " +
                buttonClicked);
        if (buttonClicked == DialogInterface.BUTTON_NEGATIVE) {
            /* SPRD: add for bug560757 @{ */
            if (getActivity() != null) {
                String defaultSetting = getActivity().getString(R.string.sum_cft_default_setting);
                if (!TextUtils.isEmpty(defaultSetting)
                        && defaultSetting.equals(mNumberEditPref.getSummary())) {
                    mNumberEditPref.setPhoneNumber("");
                }
            }
            /* @} */
            return;
        }

        if (preference == mNumberEditPref) {
            final String number = mNumberEditPref.getPhoneNumber();
            CallForwardTimeUtils.writeNumber(this.getActivity().getApplicationContext(), mIccId,
                    number);
            updateNumberSummary(number);
            notifySetNumberChange();
        }
    }

    private static void log(String msg) {
        Log.d(LOG_TAG, msg);
    }

    private void updateNumberSummary(String number) {
        CharSequence summaryOn;

        if (number != null && number.length() > 0) {
            summaryOn = number;
            mSetNumber |= NUMBER_SET;
        } else {
            summaryOn = getActivity().getString(R.string.sum_cft_default_setting);
            mSetNumber &= (STARTTIME_SET | STOPTIME_SET);
        }
        mNumberEditPref.setSummary(summaryOn);
    }

    private void updateTimeSummary(String time, boolean isStartTime) {
        CharSequence summaryOn;
        if (time != null && time.length() > 0) {
            summaryOn = time;
            mSetNumber |= isStartTime ? STARTTIME_SET : STOPTIME_SET;
        } else {
            summaryOn = getActivity().getString(R.string.sum_cft_default_setting);
        }
        if (isStartTime) {
            mStartTimePref.setSummary(summaryOn);
        } else {
            mStopTimePref.setSummary(summaryOn);
        }
    }

    protected void setParentActivity(int identifier) {
        mNumberEditPref.setParentActivity(getActivity(), identifier);
    }

    protected void onPickActivityResult(String pickedValue) {
        mNumberEditPref.onPickActivityResult(pickedValue);
    }

    protected int getSetNumber() {
        return mSetNumber;
    }

    protected void notifySetNumberChange() {
        ((CallForwardTimeEditPreference) getActivity()).notifySetNumberChange();
    }

    private class MyHandler extends Handler {
        static final int MESSAGE_GET_TIME_CF = 0;
        static final int MESSAGE_SET_TIME_CF = 1;

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MESSAGE_SET_TIME_CF:
                    handleSetCFResponse(msg);
                    break;
                case MESSAGE_GET_TIME_CF:
                    handleGetCFResponse(msg);
                    break;
            }
        }

        private void handleGetCFResponse(Message msg) {
            AsyncResult ar = (AsyncResult) msg.obj;
            if (DBG)
                log("handleGetCFResponse: done...msg1=" + msg.arg1 + " / msg2=" + msg.arg2);
            if ((ar.userObj instanceof Throwable) || ar.exception != null
                    || msg.arg2 != MyHandler.MESSAGE_SET_TIME_CF) {
                if (mContext != null) {
                    Toast.makeText(mContext, R.string.response_error, Toast.LENGTH_LONG).show();
                }
                return;
            }
            ImsCallForwardInfoEx cfInfoArray[] = (ImsCallForwardInfoEx[]) ar.result;
            if (cfInfoArray.length == 0) {
                if (DBG)
                    Log.d(LOG_TAG, "handleGetCFResponse: cfInfoArray.length==0");
                if (mContext != null) {
                    Toast.makeText(mContext, R.string.response_error, Toast.LENGTH_LONG).show();
                }
            } else {
                for (int i = 0, length = cfInfoArray.length; i < length; i++) {
                    if (DBG)
                        Log.d(LOG_TAG, "handleGetCFResponse, cfInfoArray[" + i + "]="
                                + cfInfoArray[i]);
                    if ((CommandsInterface.SERVICE_CLASS_VOICE & cfInfoArray[i].mServiceClass) != 0
                            && (CommandsInterface.CF_REASON_UNCONDITIONAL == cfInfoArray[i].mCondition)) {

                        String number = cfInfoArray[i].mNumber;
                        String numberToSave = null;// SPRD: add for bug478869

                        if (!TextUtils.isEmpty(number)
                                && !TextUtils.isEmpty(cfInfoArray[i].mRuleset)) {
                            if (number != null && PhoneNumberUtils.isUriNumber(number)) {
                                numberToSave = number;// SPRD: add for bug478869
                                savePrefData(PREF_PREFIX + "num_" + mPhoneId, numberToSave);
                            } else {
                                // SPRD: add for bug478869
                                numberToSave = PhoneNumberUtils.stripSeparators(number);
                                savePrefData(PREF_PREFIX + "num_" + mPhoneId, numberToSave);
                            }
                            savePrefData(PREF_PREFIX + "ruleset_" + mPhoneId,
                                    cfInfoArray[i].mRuleset);
                            savePrefData(PREF_PREFIX + "status_" + mPhoneId,
                                    String.valueOf(cfInfoArray[i].mStatus));
                            // SPRD: add for bug 478879
                            CallForwardTimeUtils.writeStatus(
                                    mContext, mIccId, Integer.valueOf(cfInfoArray[i].mStatus));
                            /* SPRD: add for bug421361@{ */
                        } else {
                              savePrefData(PREF_PREFIX +"num_"+ mPhoneId, "");
                              savePrefData(PREF_PREFIX +"ruleset_"+ mPhoneId,"");
                              savePrefData(PREF_PREFIX +"status_"+ mPhoneId,"");
                              // SPRD: add for bug 478879
                              CallForwardTimeUtils.writeStatus(mContext, mIccId, 0);
                         }
                        /* @} */
                        /* SPRD: add for bug478869. begin @{ */
                        if (mListeners != null) {
                            for (Listener listener : mListeners) {
                                listener.onCallForawrdTimeStateChanged(numberToSave);
                            }
                        }
                        /* SPRD: add for bug478869. end @{ */
                    }
                }
            }
        }

        private void handleSetCFResponse(Message msg) {
            if (DBG) log("handleSetCFResponse: done");
            AsyncResult ar = (AsyncResult) msg.obj;

            if (ar.exception != null) {
                if (DBG) log("handleSetCFResponse: ar.exception=" + ar.exception);
                if(mContext!=null) {
                    Toast.makeText(mContext, R.string.exception_error, Toast.LENGTH_LONG).show();
                }
            } else {
                UtExProxy.getInstance().getCallForwardingOption(CommandsInterface.CF_REASON_UNCONDITIONAL,
                        CommandsInterface.SERVICE_CLASS_VOICE,
                        null,
                        mHandler.obtainMessage(MyHandler.MESSAGE_GET_TIME_CF,
                                msg.arg1,
                                msg.arg2));
            }
        }

    }

    protected void onMenuOKClicked(String status) {
        int action = ("MENU_OK".equals(status)) ? CommandsInterface.CF_ACTION_REGISTRATION :
                CommandsInterface.CF_ACTION_DISABLE;
        String number = mNumberEditPref.getPhoneNumber();
        timeRule = "time" + String.valueOf(mStartTime) + "," + String.valueOf(mStopTime);
        if (null == mStartTime || null == mStopTime) {
            timeRule = mPrefs.getString(PREF_PREFIX + "ruleset_" + mPhoneId, "");
        }
        if (null == number) {
            number = mPrefs.getString(PREF_PREFIX + "num_" + mPhoneId, "");
        }
        int reason = CommandsInterface.CF_REASON_UNCONDITIONAL;
        int serviceClass = CommandsInterface.SERVICE_CLASS_VOICE;
        int time = (reason != CommandsInterface.CF_REASON_NO_REPLY) ? 0 : 20;
        Log.d(LOG_TAG, "reason=" + reason + "  action=" + action + " number="
                + number + "  timeRule=" + timeRule);
        UtExProxy.getInstance().setCallForwardingOption(mPhoneId,
                action,
                reason,
                serviceClass,
                number,
                time,
                timeRule,
                mHandler.obtainMessage(MyHandler.MESSAGE_SET_TIME_CF,
                action,
                MyHandler.MESSAGE_SET_TIME_CF));

    }

    public static void addListener(Listener listener) {
        Preconditions.checkNotNull(listener);
        mListeners.add(listener);
    }

    public static void removeListener(Listener listener) {
        Preconditions.checkNotNull(listener);
        mListeners.remove(listener);
    }

    public interface Listener {
        public void onCallForawrdTimeStateChanged(String num);
    }

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

    public void savePrefData(String key, String value) {
        Log.w(LOG_TAG, "savePrefData(" + key + ", " + value + ")");
        if (mPrefs != null) {
            try {
                SharedPreferences.Editor editor = mPrefs.edit();
                editor.putString(key, value);
                editor.apply();
            } catch (Exception e) {
                Log.e(LOG_TAG, "Exception happen.");
            }
        }
    }

    /* SPRD: Add for bug 524927 @{ */
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        if (mTimePickerDialog != null && mTimePickerDialog.isShowing()) {
            // SPRD: modify for bug606606
            mTimePicker = mTimePickerDialog.getTime();
            mCurrentHour = mTimePicker.getCurrentHour();
            mCurrentMinute = mTimePicker.getCurrentMinute();
            mTimePickerDialog.dismiss();
            showCurrentTimePicker();
        }
        super.onConfigurationChanged(newConfig);
    }

    private void showCurrentTimePicker() {
        mTimePickerDialog = new TimePickerDialog(
                getActivity(),
                this,
                mCurrentHour,
                mCurrentMinute,
                true);
        mTimePickerDialog.show();
    }
    /* @} */

    /* SPRD: add for bug560757 @{ */
    public EditPhoneNumberPreference getEditPhoneNumberPreference(){
        return mNumberEditPref;
    }
    /* @} */
}
