package com.sprd.systemupdate;

import android.content.Context;
import android.content.res.TypedArray;
import android.preference.Preference;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.RadioButton;
import android.widget.TextView;

public class RadioPreference extends Preference {

    private boolean isChecked;
    private int mId;
    private long mInterval;

    public RadioPreference(Context context, AttributeSet attrs) {
        this(context, attrs, 0);

    }

    public RadioPreference(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        setLayoutResource(R.layout.radiopref);
        TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.ThisStyle, defStyle, 0);
        int time = a.getInt(R.styleable.ThisStyle_time, 0);// hour
        mInterval = (long) time * 3600000; // millisecond
        a.recycle();
    }

    public void onBindView(View view) {
        super.onBindView(view);

        TextView title = (TextView) view.findViewById(R.id.title);
        title.setText(getTitle());

        RadioButton radio = (RadioButton) view.findViewById(R.id.radio);

        if (isChecked) {
            radio.setChecked(true);
        } else {
            radio.setChecked(false);
        }
    }

    protected void onClick() {
        super.onClick();
        boolean newValue = !isChecked();

        if (!callChangeListener(newValue)) {
            return;
        }
        setChecked(newValue);

    }

    protected void onSetInitialValue(boolean restoreValue, Object defaultValue) {

        setChecked(restoreValue ? getPersistedBoolean(isChecked)
                : (Boolean) defaultValue);
    }

    public void setChecked(boolean checked) {
        if (isChecked != checked) {
            isChecked = checked;
            persistBoolean(checked);
            notifyDependencyChange(shouldDisableDependents());
            notifyChanged();
        }
    }

    public boolean isChecked() {
        return isChecked;
    }

    public void setId(int id) {
        mId = id;
    }

    public int getId() {
        return mId;
    }

    public long getTime() {
        return mInterval;
    }

}
