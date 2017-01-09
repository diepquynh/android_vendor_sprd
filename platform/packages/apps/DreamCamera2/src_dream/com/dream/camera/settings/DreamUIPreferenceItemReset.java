package com.dream.camera.settings;

import com.android.camera.settings.CameraSettingsActivity;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.preference.Preference;
import android.util.AttributeSet;
import com.android.camera2.R;

public class DreamUIPreferenceItemReset extends Preference {

    public DreamUIPreferenceItemReset(Context context) {
        super(context);
    }

    public DreamUIPreferenceItemReset(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public DreamUIPreferenceItemReset(Context context, AttributeSet attrs,
            int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public DreamUIPreferenceItemReset(Context context, AttributeSet attrs,
            int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    @Override
    protected void onClick() {
        showAlertDialog();
    }

    public void showAlertDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
        final AlertDialog alertDialog = builder.create();
        builder.setTitle(getContext().getString(R.string.dream_pref_restore_detail));
        builder.setMessage(getContext().getString(R.string.dream_restore_message));
        builder.setPositiveButton(getContext().getString(R.string.restore_done),
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        DataModuleManager.getInstance(getContext()).reset();
                    }
                });
        builder.setNegativeButton(getContext().getString(R.string.restore_cancel),
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        alertDialog.dismiss();
                    }
                });
        AlertDialog dialog = builder.create();
        dialog.show();
    }
}
