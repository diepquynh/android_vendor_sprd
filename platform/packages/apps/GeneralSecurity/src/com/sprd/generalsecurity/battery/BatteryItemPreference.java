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
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
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
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RadioButton;
import android.widget.TextView;

import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.BatteryUtils;

public class BatteryItemPreference extends Preference {
    private static final String TAG = "BatteryItemPreference";
    private int mId;
    private double mPercent;
    private Context mContext;

    public BatteryEntry mInfo;
    private Drawable mICon;

    private ProgressBar process;
    private TextView percent;

    private double mPercentOfMax;
    private double mPercentOfTotal;

    private int mTintColor;

    public double getPercent() {
        return mPercent;
    }

    public BatteryItemPreference(Context context, BatteryEntry info) {
        this(context);
        mContext = context;
        mInfo = info;

        init();
    }

    public BatteryItemPreference(Context context, AttributeSet attrs,
            int defStyle) {
        super(context, attrs, defStyle);
    }

    public BatteryItemPreference(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public BatteryItemPreference(Context context) {
        this(context, null, 0);
    }

    private void init() {
        setLayoutResource(R.layout.battery_item);
    }

    public void updateNameAndIcon(String name, Drawable icon) {
        mInfo.name = name;
        mInfo.icon = icon;
        notifyChanged();
    }

    @Override
    public View getView(View convertView, ViewGroup parent) {
        View view = super.getView(convertView, parent);
        ImageView icon = (ImageView) view.findViewById(R.id.icon);
        TextView title = (TextView) view.findViewById(R.id.title);
        percent = (TextView) view.findViewById(R.id.percent);
        percent.setText(((int) (mPercentOfTotal + 0.5) < 1  ? "<":"") + BatteryUtils
                .formatPercentage((int) (mPercentOfTotal + 0.5) < 1 ?
                    (int)1.0 : (int) (mPercentOfTotal + 0.5)));
        icon.setImageDrawable(mInfo.icon);
        title.setText(mInfo.name);

        if (mTintColor != 0) {
            ((ImageView) view.findViewById(R.id.icon)).setImageTintList(
                    ColorStateList.valueOf(mTintColor));
        }

        return view;
    }

    public void setPercent(double percentOfMax, double percentOfTotal) {
        this.mPercentOfMax = percentOfMax;
        this.mPercentOfTotal = percentOfTotal;
        Log.i(TAG, "percentOfMax-"+percentOfMax+ "  percentOfTotal-"+percentOfTotal);
    }

    public void setTint(int color) {
        mTintColor = color;
        notifyChanged();
    }
}
