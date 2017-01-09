package com.sprd.audioprofile;

import android.content.Context;
import android.preference.Preference;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.RadioButton;
import android.widget.TextView;

public class AudioProfilePreference extends Preference implements
        CompoundButton.OnCheckedChangeListener {
    final static String TAG = "AudioProfilePreference";

    private int mId;
    private RadioButton mRadioButton;

    /**
     * @param context
     * @param attrs
     * @param defStyle
     */
    public AudioProfilePreference(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
    }

    /**
     * @param context
     * @param attrs
     */
    public AudioProfilePreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    /**
     * @param context
     */
    public AudioProfilePreference(Context context) {
        super(context);
        init();
    }

    private boolean mIsSelected = false;
    private boolean mProtectFromCheckedChange = false;

    @Override
    public View getView(View convertView, ViewGroup parent) {
        View view = super.getView(convertView, parent);
        View widget = view.findViewById(R.id.audio_profile_radiobutton);
		if ((widget != null) && widget instanceof RadioButton) {
			mRadioButton = (RadioButton) widget;
			mRadioButton.setOnCheckedChangeListener(this);
			mProtectFromCheckedChange = true;
			mRadioButton.setChecked(mIsSelected);
			mProtectFromCheckedChange = false;
		}

        View textLayout = view.findViewById(R.id.text_layout);
        if ((textLayout != null) && textLayout instanceof TextView) {
            ((TextView) textLayout).setText(getTitle());
        }
        View textSummary = view.findViewById(R.id.text_summary);
        if ((textSummary != null) && textSummary instanceof TextView && getSummary() != null) {
            ((TextView) textSummary).setText(getSummary());
            ((TextView) textSummary).setVisibility(View.VISIBLE);
        }
        view.setTag(mId);
        return view;
    }

    private void init() {
        setLayoutResource(R.layout.audio_profile_preference_layout);
    }

	public void setChecked(boolean checked) {
		if (checked != mIsSelected) {
			mIsSelected = checked;
		}
		if (mRadioButton != null) {
			mRadioButton.setChecked(mIsSelected);
		}
	}

    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        Log.i(TAG, "ID: " + mId + " :" + isChecked);

        if (mProtectFromCheckedChange) {
            return;
        }

        if (isChecked) {
            mIsSelected = true;
            callChangeListener(Integer.valueOf(mId));
        } else {
            mIsSelected = false;
        }
    }

	public void setId(int id) {
		mId = id;
	}

	public int getId() {
		return mId;
	}
}
