package com.sprd.soundrecorder;

import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.RadioGroup;
import android.widget.RadioButton;
import android.widget.TimePicker;
import android.widget.TimePicker.OnTimeChangedListener;
import android.app.AlertDialog;

import com.android.soundrecorder.R;

public class TimeAndDurationPickerDialog extends AlertDialog implements OnClickListener,
        OnTimeChangedListener, RadioGroup.OnCheckedChangeListener{
    private static final String HOUR = "hour";
    private static final String MINUTE = "minute";
    private static final String IS_24_HOUR = "is24hour";

    private final TimePicker mTimePicker;
    private final OnTimeSetListener mTimeSetListener;

    private final int mInitialHourOfDay;
    private final int mInitialMinute;
    private final boolean mIs24HourView;

    private final RadioGroup mRadioGroup;
    private final RadioButton mRadioButton1;
    private final RadioButton mRadioButton2;
    private final RadioButton mRadioButton3;
    private final RadioButton mRadioButton4;
    private int mRecordDuration = 30;

    /**
     * The callback interface used to indicate the user is done filling in
     * the time (e.g. they clicked on the 'OK' button).
     */
    public interface OnTimeSetListener {
        public void onTimeSet(TimePicker view, int hourOfDay, int minute, int duration);
    }

    /**
     * Creates a new time picker dialog.
     *
     * @param context the parent context
     * @param listener the listener to call when the time is set
     * @param hourOfDay the initial hour
     * @param minute the initial minute
     * @param is24HourView whether this is a 24 hour view or AM/PM
     */
    public TimeAndDurationPickerDialog(Context context, OnTimeSetListener listener, int hourOfDay, int minute,
            boolean is24HourView) {
        this(context, 0, listener, hourOfDay, minute, is24HourView);
    }

    static int resolveDialogTheme(Context context, int resId) {
        if (resId == 0) {
            final TypedValue outValue = new TypedValue();
            context.getTheme().resolveAttribute(R.attr.timePickerDialogTheme, outValue, true);
            return outValue.resourceId;
        } else {
            return resId;
        }
    }

    /**
     * Creates a new time picker dialog with the specified theme.
     *
     * @param context the parent context
     * @param themeResId the resource ID of the theme to apply to this dialog
     * @param listener the listener to call when the time is set
     * @param hourOfDay the initial hour
     * @param minute the initial minute
     * @param is24HourView Whether this is a 24 hour view, or AM/PM.
     */
    public TimeAndDurationPickerDialog(Context context, int themeResId, OnTimeSetListener listener,
            int hourOfDay, int minute, boolean is24HourView) {
        super(context, resolveDialogTheme(context, themeResId));

        mTimeSetListener = listener;
        mInitialHourOfDay = hourOfDay;
        mInitialMinute = minute;
        mIs24HourView = is24HourView;

        final Context themeContext = getContext();

        final TypedValue outValue = new TypedValue();
        context.getTheme().resolveAttribute(R.attr.timePickerDialogTheme, outValue, true);
        final int layoutResId = outValue.resourceId;

        final LayoutInflater inflater = LayoutInflater.from(themeContext);
        final View view = inflater.inflate(R.layout.time_picker_dialog, null);
        setView(view);
        setButton(BUTTON_POSITIVE, themeContext.getString(R.string.button_ok), this);
        setButton(BUTTON_NEGATIVE, themeContext.getString(R.string.button_cancel), this);
        //setButtonPanelLayoutHint(LAYOUT_HINT_SIDE);

        mTimePicker = (TimePicker) view.findViewById(R.id.timePicker);
        mTimePicker.setIs24HourView(mIs24HourView);
        mTimePicker.setCurrentHour(mInitialHourOfDay);
        mTimePicker.setCurrentMinute(mInitialMinute);
        mTimePicker.setOnTimeChangedListener(this);

        mRadioGroup = (RadioGroup) view.findViewById(R.id.durationGroup);
        mRadioGroup.setOnCheckedChangeListener(this);
        mRadioButton1 = (RadioButton) view.findViewById(R.id.button1);
        mRadioButton2 = (RadioButton) view.findViewById(R.id.button2);
        mRadioButton3 = (RadioButton) view.findViewById(R.id.button3);
        mRadioButton4 = (RadioButton) view.findViewById(R.id.button4);
    }

    @Override
    public void onTimeChanged(TimePicker view, int hourOfDay, int minute) {
        /* do nothing */
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        switch (which) {
            case BUTTON_POSITIVE:
                if (mTimeSetListener != null) {
                    mTimeSetListener.onTimeSet(mTimePicker, mTimePicker.getCurrentHour(),
                            mTimePicker.getCurrentMinute(), mRecordDuration);
                }
                break;
            case BUTTON_NEGATIVE:
                cancel();
                break;
        }
    }

    /**
     * Sets the current time.
     *
     * @param hourOfDay The current hour within the day.
     * @param minuteOfHour The current minute within the hour.
     */
    public void updateTime(int hourOfDay, int minuteOfHour) {
        mTimePicker.setCurrentHour(hourOfDay);
        mTimePicker.setCurrentMinute(minuteOfHour);
    }

    @Override
    public Bundle onSaveInstanceState() {
        final Bundle state = super.onSaveInstanceState();
        state.putInt(HOUR, mTimePicker.getCurrentHour());
        state.putInt(MINUTE, mTimePicker.getCurrentMinute());
        state.putBoolean(IS_24_HOUR, mTimePicker.is24HourView());
        return state;
    }

    @Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        final int hour = savedInstanceState.getInt(HOUR);
        final int minute = savedInstanceState.getInt(MINUTE);
        mTimePicker.setIs24HourView(savedInstanceState.getBoolean(IS_24_HOUR));
        mTimePicker.setCurrentHour(hour);
        mTimePicker.setCurrentMinute(minute);
    }

    public TimePicker getTime(){
        return mTimePicker;
    }

    public void onCheckedChanged(RadioGroup group, int checkedId) {
        if(checkedId == mRadioButton1.getId()) {
            mRecordDuration = 30;
        } else if(checkedId == mRadioButton2.getId()) {
            mRecordDuration = 60;
        } else if(checkedId == mRadioButton3.getId()) {
            mRecordDuration = 120;
        } else if(checkedId == mRadioButton4.getId()) {
            mRecordDuration = 180;
        }
    }
}
