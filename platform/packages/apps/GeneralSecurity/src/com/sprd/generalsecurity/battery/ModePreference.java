package com.sprd.generalsecurity.battery;

import java.util.ArrayList;
import java.util.List;

import com.android.internal.os.BatteryStatsHelper;
import com.android.internal.os.BatterySipper;

import android.os.BatteryStats;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.UserManager;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.TextView;

import com.sprd.generalsecurity.R;

public class ModePreference extends Preference {
    private static final String TAG = "ModePreference";
    private RadioButton mRadioButton;
    private int mId;
    private boolean mIsSelected;
    private boolean mProtectFromCheckedChange;
    private Context mContext;

    public ModePreference(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mContext = context;
        init();
    }

    public ModePreference(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ModePreference(Context context) {
        this(context, null);
    }

    private void init() {
        setLayoutResource(R.layout.battery_mode_item);
    }

    @Override
    public View getView(View convertView, ViewGroup parent) {
        View view = super.getView(convertView, parent);
        View widget = view.findViewById(R.id.radioBtn);
        if ((widget != null) && widget instanceof RadioButton) {
            mRadioButton = (RadioButton) widget;
            mProtectFromCheckedChange = true;
            mRadioButton.setChecked(mIsSelected);
            mProtectFromCheckedChange = false;
        }

        View textLayout = view.findViewById(R.id.title);
        if ((textLayout != null) && textLayout instanceof TextView) {
            ((TextView) textLayout).setText(getTitle());
        }
        View textSummary = view.findViewById(R.id.summary);
        if ((textSummary != null) && textSummary instanceof TextView
                && getSummary() != null) {
            Log.i(TAG,"...setVisibility...");
            ((TextView) textSummary).setText(getSummary());
            ((TextView) textSummary).setVisibility(View.VISIBLE);
        }
        view.setTag(mId);
        return view;
    }

    public void setId(int id) {
        mId = id;
    }

    public int getId() {
        return mId;
    }

    public void setChecked(boolean checked) {
        Log.i(TAG,"checked:"+checked+" mIsSelected:"+mIsSelected);
        if (checked != mIsSelected) {
            mIsSelected = checked;
            Log.i(TAG,"mIsSelected:"+mIsSelected);
        }
        if (mRadioButton != null) {
            Log.i(TAG,"mRadioButton:"+mRadioButton);
            mRadioButton.setChecked(mIsSelected);
        }
    }

}
