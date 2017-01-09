package com.sprd.systemupdate;

import java.text.DecimalFormat;

import android.content.Context;
import android.preference.Preference;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.TextView;

public class ProgressPreference extends Preference {

    private Storage mStorage;
    private ProgressBar mDownloadprogress;
    private TextView mPercentage;
    private TextView mRatio;

    private int mProgress;

    public static final int KB_SIZE = 1024;
    public static final int MB_SIZE = 1024 * 1024;

    public ProgressPreference(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ProgressPreference(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        setLayoutResource(R.layout.progresspref);
    }

    public void onBindView(View view) {
        super.onBindView(view);
        mStorage = Storage.get(getContext());
        mDownloadprogress = (ProgressBar) view
                .findViewById(R.id.download_progress);
        mPercentage = (TextView) view.findViewById(R.id.percentage);
        mRatio = (TextView) view.findViewById(R.id.ratio);

        updateShow();
    }

    public void setProgress(int i) {
        mProgress = i;
        notifyDependencyChange(shouldDisableDependents());
        notifyChanged();
    }

    private void updateShow() {
        mDownloadprogress.setProgress(mProgress
                / (mStorage.getLatestVersion().mSize / 100));
        DecimalFormat to = new DecimalFormat("0.0");
        String ratio = getContext().getString(R.string.ratio);
        String percentage = getContext().getString(R.string.percentage);

        double current, total;
        current = mProgress * 1.0;
        total = mStorage.getLatestVersion().mSize * 1.0;

        if (total < KB_SIZE) {
            ratio = getContext().getString(R.string.ratio_B);
            ratio = String.format(ratio, to.format(current), to.format(total));
        } else if (total < MB_SIZE) {
            ratio = getContext().getString(R.string.ratio_KB);
            ratio = String.format(ratio, to.format(current / 1024.0),
                    to.format(total / 1024.0));
        } else if (total >= MB_SIZE) {
            ratio = getContext().getString(R.string.ratio_MB);
            ratio = String.format(ratio,
                    to.format(current / (1024.0 * 1024.0)),
                    to.format(total / (1024.0 * 1024.0)));
        }

        int percentageValue = mProgress/ (mStorage.getLatestVersion().mSize/100);
        if (percentageValue >= 100) {
            percentageValue = 100;
        }        
        percentage = String.format(percentage,percentageValue);        
        
        mRatio.setText(ratio);
        mPercentage.setText(percentage);
    }

}
