package com.dream.camera.settings;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Bundle;
import android.preference.ListPreference;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import com.android.camera2.R;

public class DreamUIPreferenceItemList extends ListPreference implements
        DreamUIPreferenceItemInterface {

    public DreamUIPreferenceItemList(Context context) {
        super(context);
    }

    public DreamUIPreferenceItemList(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public DreamUIPreferenceItemList(Context context, AttributeSet attrs,
            int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public DreamUIPreferenceItemList(Context context, AttributeSet attrs,
            int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    public static final String TAG = "DreamUIPreferenceItemList";
    private DreamUISettingPartBasic mSettingPart;

    @Override
    protected boolean persistString(String value) {

        if (shouldPersist()) {
            // Shouldn't store null
            if (value == getPersistedString(null)) {
                // It's already there, so the same as persisting
                return true;
            }

            if (mSettingPart != null) {
                return mSettingPart.persistString(getKey(), value);
            }

            return true;
        }
        return false;

    }

    @Override
    protected String getPersistedString(String defaultReturnValue) {

        if (!shouldPersist()) {
            return defaultReturnValue;
        }

        if (mSettingPart != null) {
            String value = mSettingPart.getPersistedString(getKey());
            return value == null ? defaultReturnValue : value;
        }

        return defaultReturnValue;
    }

    @Override
    public void initializeData(DreamUISettingPartBasic settingPart) {
        mSettingPart = settingPart;
        update();

    }

    @Override
    public void update() {
        this.setEntries(mSettingPart.getListEntries(getKey()));
        this.setEntryValues(mSettingPart.getListEntryValues(getKey()));
        this.setSummary(mSettingPart.getListSummaryFromKey(getKey()));
        this.setValue(mSettingPart.getListValue(getKey()));
        Log.e(TAG,
                "update key = " + getKey() + " value = "
                        + mSettingPart.getListValue(getKey()) + " summary = "
                        + mSettingPart.getListSummaryFromKey(getKey()) + " entries.length = "
                        + (mSettingPart.getListEntries(getKey()) == null ? 0 : mSettingPart.getListEntries(getKey()).length) + " entryValues.length = "
                        + (mSettingPart.getListEntryValues(getKey()) == null ? 0 : mSettingPart.getListEntryValues(getKey()).length));
    }

    private boolean fromSummary = false;

    @Override
    public void setValue(String value) {
        fromSummary = false;
        super.setValue(value);
    }

    @Override
    public void setValueIndex(int index) {
        fromSummary = false;
        super.setValueIndex(index);
    }

    @Override
    public void setSummary(CharSequence summary) {
        fromSummary = true;
        super.setSummary(summary);
    }

    @Override
    protected void notifyChanged() {
        if (fromSummary) {
            return;
        }
        super.notifyChanged();
        if (mSettingPart != null) {
            this.setSummary(mSettingPart.getListSummaryFromKey(getKey()));
            // mSettingPart.notifyChanged(this);
        }
    }
//
//    Dialog mDialog;
//
//    @Override
//    protected void showDialog(Bundle state) {
//        Context context = getContext();
//
//        AlertDialog.Builder mBuilder = new AlertDialog.Builder(context,
//                R.style.ThemeDeviceDefaultDialogAlert)
//                .setTitle(getDialogTitle()).setIcon(getDialogIcon())
//                .setPositiveButton(getPositiveButtonText(), this)
//                .setNegativeButton(getNegativeButtonText(), this);
//
//        View contentView = onCreateDialogView();
//        if (contentView != null) {
//            onBindDialogView(contentView);
//            mBuilder.setView(contentView);
//        } else {
//            mBuilder.setMessage(getDialogMessage());
//        }
//
//        onPrepareDialogBuilder(mBuilder);
//
//        // Create the dialog
//        final Dialog dialog = mDialog = mBuilder.create();
//        if (state != null) {
//            dialog.onRestoreInstanceState(state);
//        }
//        dialog.setOnDismissListener(this);
//        dialog.show();
//    }

}
