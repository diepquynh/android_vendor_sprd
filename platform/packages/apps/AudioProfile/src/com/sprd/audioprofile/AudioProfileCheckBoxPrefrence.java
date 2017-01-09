package com.sprd.audioprofile;

import android.content.Context;
import android.preference.CheckBoxPreference;
import android.util.AttributeSet;
import android.view.View;
import android.widget.TextView;

public class AudioProfileCheckBoxPrefrence extends CheckBoxPreference {

    public AudioProfileCheckBoxPrefrence(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public AudioProfileCheckBoxPrefrence(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public AudioProfileCheckBoxPrefrence(Context context) {
        super(context);
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);
        View checkBox = view.findViewById(android.R.id.checkbox);
        checkBox.setRight(30);

    }

}