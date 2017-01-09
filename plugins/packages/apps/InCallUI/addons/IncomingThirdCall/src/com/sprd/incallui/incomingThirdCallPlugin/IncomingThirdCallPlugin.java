package com.sprd.incallui.incomingThirdCallPlugin;

import android.app.AddonManager;
import android.content.Context;
import android.content.res.Resources;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;

import android.util.Log;
import com.android.incallui.CallList;
import com.sprd.incallui.IncomingThirdCallHelper;

public class IncomingThirdCallPlugin extends IncomingThirdCallHelper implements
        AddonManager.InitialCallback {

    private static final String TAG = "IncomingThirdCallPlugin";
    private static final boolean DBG = true;
    private Context mContext;
    private AlertDialog mHangupcallDialog = null;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    public void IncomingThirdCallPlugin() {
    }

    @Override
    public void dealIncomingThirdCall(Context context, boolean show) {
        if (show) {
            if ((CallList.getInstance().getActiveCall() != null)
                    && (CallList.getInstance().getBackgroundCall() != null)
                    && mHangupcallDialog == null) {
                showHangupCallDialog(context);
            }
        } else {
            dismissHangupCallDialog();
        }
    }

    private void showHangupCallDialog(Context context) {

        String note_title = mContext.getString(R.string.hangupcall_note_title);
        String note_message = mContext
                .getString(R.string.hangupcall_note_message);
        mHangupcallDialog = new AlertDialog.Builder(context)
                .setTitle(note_title).setMessage(note_message)
                .setPositiveButton(com.android.internal.R.string.ok, null)
                .setCancelable(false).create();
        mHangupcallDialog.show();
    }

    @Override
    public void dismissHangupCallDialog() {
        if (mHangupcallDialog != null) {
            mHangupcallDialog.dismiss();
            mHangupcallDialog = null;
        }
    }

    @Override
    public boolean isSupportIncomingThirdCall() {
        return true;
    }

}
