package com.android.server.policy;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.PowerManager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.TextView;
import com.android.internal.R;

public class RebootSinglePressAction extends GlobalActions.SinglePressAction {
    private AlertDialog mConfirmDialog = null;
    private Context mContext;

    protected RebootSinglePressAction(int iconResId, int messageResId, Context context) {
        super(iconResId, messageResId);
        mContext = context;
    }
    @Override
    public View create(Context context, View convertView, ViewGroup parent,
            LayoutInflater inflater) {
        View v = super.create(context, convertView, parent, inflater);
        TextView statusView = (TextView) v.findViewById(R.id.status);
        statusView.setText(R.string.global_reboot_close_apps);
        v.findViewById(R.id.status).setVisibility(View.VISIBLE);
        return v;
    }

    public void onPress() {
        dismiss();
        mConfirmDialog = new AlertDialog.Builder(mContext)
                .setTitle(com.android.internal.R.string.reboot_device_title)
                .setMessage(com.android.internal.R.string.reboot_device_confirm)
                .setPositiveButton(com.android.internal.R.string.yes,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int which) {
                                dismiss();
                                PowerManager pm = (PowerManager) mContext
                                        .getSystemService(Context.POWER_SERVICE);
                                pm.reboot("power_reboot");
                            }
                        })
                .setNegativeButton(com.android.internal.R.string.no, null)
                .create();
        mConfirmDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_KEYGUARD_DIALOG);
        mConfirmDialog.show();
    }

    public boolean onLongPress() {
        return true;
    }

    public boolean showDuringKeyguard() {
        return true;
    }

    public boolean showBeforeProvisioning() {
        return true;
    }

    public void dismiss() {
        if (mConfirmDialog != null && mConfirmDialog.isShowing()) {
            mConfirmDialog.dismiss();
        }
    }
}


