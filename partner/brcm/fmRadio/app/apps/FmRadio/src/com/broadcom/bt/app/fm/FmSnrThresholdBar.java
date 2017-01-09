/************************************************************************************
 *
 *  Copyright (C) 2009-2013 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ************************************************************************************/
package com.broadcom.bt.app.fm;

import android.content.Context;
import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

import com.broadcom.bt.app.fm.R;
import com.broadcom.bt.app.fm.rx.FmRadioSettings;

public class FmSnrThresholdBar extends DialogPreference implements OnSeekBarChangeListener {

    private static final String TAG = "FmSnrThresholdBar";

    public static final int FM_MAX_SNR = 31;
    public static final int FM_DEFAULT_SNR = 0;

    private SeekBar thresholdBar = null;

    private int initialValue = 0;
    private int persistValue = 0;

    private int offset = 0;
    private int max = FM_MAX_SNR;

    public FmSnrThresholdBar(Context context) {
        super(context, null);
        initView();
    }

    public FmSnrThresholdBar(Context context, AttributeSet attrs) {
        super(context, attrs);
        initView();
    }

    private void initView() {
        setDialogLayoutResource(R.layout.seekbar_dialog);
        setDialogTitle(getTitle());
        setPositiveButtonText(android.R.string.ok);
        setNegativeButtonText(android.R.string.cancel);
        setMax(FM_MAX_SNR);
        setProgress(FM_DEFAULT_SNR);
    }

    protected void onSetInitialValue(boolean restorePersistedValue, Object defaultValue) {
        super.onSetInitialValue(restorePersistedValue, defaultValue);
        Log.d(TAG, "onSetInitialValue()");
        initialValue = Integer
                .parseInt(restorePersistedValue ? getPersistedString(FmRadioSettings.FM_PREF_SNR_THRESHOLD)
                        : String.valueOf(FM_DEFAULT_SNR));
        Log.d(TAG, "initial value: " + initialValue);
        setProgress(initialValue);
    }

    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);
        Log.d(TAG, "onBindDialogView()");
        thresholdBar = (SeekBar) view.findViewById(R.id.seekbar);
        thresholdBar.setMax(max - offset);
        thresholdBar.setProgress(persistValue);
        thresholdBar.setOnSeekBarChangeListener(this);
        setPersistent(true);
    }

    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);
        // Only persist if the dialog result is 'ok'
        if (positiveResult) {
            setProgress(persistValue);
        } else {
            persistValue = initialValue;
        }
    }

    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromTouch) {
        int newValue = thresholdBar.getProgress();

        if (!callChangeListener(newValue)) {
            return;
        }

        persistValue = newValue;
    }

    public void onStartTrackingTouch(SeekBar seekBar) {
        // TODO Auto-generated method stub
    }

    public void onStopTrackingTouch(SeekBar seekBar) {
        // TODO Auto-generated method stub
    }

    public int getMax() {
        return max;
    }

    public void setMax(int value) {
        max = value;
    }

    public int getProgress() {
        return persistValue;
    }

    public void setProgress(int value) {
        initialValue = value;
        persistValue = value;
        persistString(String.valueOf(persistValue));
        notifyChanged();
    }

    public void setOffset(int offset) {
        this.offset = offset;
    }
}
