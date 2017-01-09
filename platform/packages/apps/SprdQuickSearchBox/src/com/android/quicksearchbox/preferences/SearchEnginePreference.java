package com.android.quicksearchbox.preferences;

import android.content.Context;
import android.preference.Preference;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioButton;
import android.widget.TextView;

import com.android.quicksearchbox.R;

public class SearchEnginePreference extends Preference  {
    final static String TAG = "SearchEnginePreference";

    private int mId;
    private RadioButton mRadioButton;

    /**
     * @param context
     * @param attrs
     * @param defStyle
     */
    public SearchEnginePreference(Context context, AttributeSet attrs,
            int defStyle) {
        super(context, attrs, defStyle);
        init();
    }

    /**
     * @param context
     * @param attrs
     */
    public SearchEnginePreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    /**
     * @param context
     */
    public SearchEnginePreference(Context context) {
        super(context);
        init();
    }

    @Override
    public View getView(View convertView, ViewGroup parent) {
        View view = super.getView(convertView, parent);
        View widget = view.findViewById(R.id.engine_profile_radiobutton);
        if ((widget != null) && widget instanceof RadioButton) {
            mRadioButton = (RadioButton) widget;
        }
        //if this radioButton is checked ,then setCheck(true)
        mRadioButton.setChecked(getPersistedBoolean(false));
        View textLayout = view.findViewById(R.id.text_layout);
        if ((textLayout != null) && textLayout instanceof TextView) {
            ((TextView) textLayout).setText(getTitle());
        }

        View summary = view.findViewById(R.id.engine_summary);
        if ((summary != null) && summary instanceof TextView) {
            ((TextView) summary).setText(getSummary());
        }
        view.setTag(mId);
        return view;
    }

    private void init() {
        setLayoutResource(R.layout.engine_preference_layout);
    }

    public void setChecked(boolean checked) {
        if (mRadioButton != null) {
            mRadioButton.setChecked(checked);
            //save radioButton checkStat
            persistBoolean(checked);
        }
    }

    public boolean isChecked() {
        if (mRadioButton != null) {
            return mRadioButton.isChecked();
        } else {
            return false;
        }
    }

    public void setId(int id) {
        mId = id;
    }

    public int getId() {
        return mId;
    }
}
