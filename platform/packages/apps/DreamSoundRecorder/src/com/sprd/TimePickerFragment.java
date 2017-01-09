package com.sprd.soundrecorder;

import android.app.Activity;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.TimePickerDialog;
import android.content.DialogInterface;
import com.sprd.soundrecorder.TimeAndDurationPickerDialog.OnTimeSetListener;
import android.os.Bundle;
import android.text.format.DateFormat;
import android.util.Log;
import com.android.soundrecorder.R;

import java.util.Calendar;

public class TimePickerFragment extends DialogFragment {

    //private Alarm mAlarm;
    private OnTimeSetListener mListener;
    private static final String TAG = "TimePickerFragment";

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final int hour, minute;
        //if (mAlarm == null) {
            final Calendar c = Calendar.getInstance();
            hour = c.get(Calendar.HOUR_OF_DAY);
            minute = c.get(Calendar.MINUTE);
        /*} else {
            hour = mAlarm.hour;
            minute = mAlarm.minutes;
        }*/

        return new TimeAndDurationPickerDialog(getActivity(), R.style.TimePickerTheme, mListener, hour, minute,
                DateFormat.is24HourFormat(getActivity()));
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        if (getTargetFragment() instanceof OnTimeSetListener) {
            setOnTimeSetListener((OnTimeSetListener) getTargetFragment());
        }
    }

    public void setOnTimeSetListener(OnTimeSetListener listener) {
        mListener = listener;
    }

    /*public void setAlarm(Alarm alarm) {
        mAlarm = alarm;
    }*/

    public void onDismiss(DialogInterface dialog) {
        try {
            super.onDismiss(dialog);
        } catch (Exception e) {
            Log.w(TAG, "ignore a exception that was found when executed onDismiss,exception is:"+e.getMessage());
        }
    }
}
