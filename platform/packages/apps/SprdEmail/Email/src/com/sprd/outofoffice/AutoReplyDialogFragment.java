package com.sprd.outofoffice;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;

import com.android.email.R;
import com.android.emailcommon.utility.TextUtilities;
import android.text.InputFilter;
import com.android.emailcommon.utility.Utility;

public class AutoReplyDialogFragment extends DialogFragment {
    public static final String TAG = "AutoReplyDialogFragment";
    private static final String OOF_REPLY = "oof_reply";
    //limit the length to avoid ANR
    public static final int EDITVIEW_MAX_LENGTH = 512;

    private static long mAccountId;

    /* SPRD: Modify for bug 556038 & 562472{@ */
    public static final String REPLY_SUMM = "reply_summ";
    private String mReplySumm = null;

    private OnAddAutoReplyCallback mCallback;
    public interface OnAddAutoReplyCallback {
        /**
        * Called when the writing finished.
        */
        void onAddAutoReplyFinished(String replySumm);
    }

    public void setOnAddAutoReplyCallback(OnAddAutoReplyCallback callback) {
        mCallback = callback;
    }
    /* @} */
    public static AutoReplyDialogFragment newInstance(long accountId) {
        AutoReplyDialogFragment frag = new AutoReplyDialogFragment();
        mAccountId = accountId;
        return frag;
    }

    /**
     * Use {@link #newInstance} This public constructor is still required so
     * that DialogFragment state can be automatically restored by the
     * framework.
     */
    public AutoReplyDialogFragment() {
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        final LayoutInflater layoutInflater = LayoutInflater.from(builder.getContext());
        final View view = layoutInflater.inflate(R.layout.auto_reply_dialog, null);
        final EditText editText = (EditText) view.findViewById(R.id.reply_label);

        builder.setTitle(R.string.account_settings_oof_auto_reply_label);
        builder.setView(view);
        editText.requestFocus();
        builder.setPositiveButton(R.string.okay_action,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int whichButton) {
                        /* SPRD: Modify for bug 556038 {@ */
                        String mText = editText.getText().toString().trim();
                        setReplyText(mText);
                        if (mCallback != null) {
                            mCallback.onAddAutoReplyFinished(mText);
                        }
                        /* @} */
                    }
                }
            );

        builder.setNegativeButton(android.R.string.cancel, null);
        /* SPRD: Modify for bug 556038 & 562472 {@ */
        if (savedInstanceState != null) {
            mReplySumm = savedInstanceState.getString(REPLY_SUMM, "");
            editText.setText(mReplySumm);
        } else {
            mReplySumm = getReplyText().trim();
            editText.setText(mReplySumm);
        }
        mCallback = (OnAddAutoReplyCallback) getActivity();
        /* @} */
        final AlertDialog dialog = builder.create();

        editText.addTextChangedListener(new TextWatcher() {
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                Button okButton = dialog.getButton(AlertDialog.BUTTON_POSITIVE);
                /* SPRD: Modify for bug 556038 & 562472{@ */
                mReplySumm = editText.getText().toString().trim();
                if (!TextUtils.isEmpty(mReplySumm)) {
                    okButton.setEnabled(true);
                } else {
                    okButton.setEnabled(false);
                }
                /* @} */
            }
        });

        editText.getText().setFilters(new InputFilter[]{
            new InputFilter.LengthFilter(EDITVIEW_MAX_LENGTH)
        });

        dialog.getWindow().setSoftInputMode(
                WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
        dialog.setCanceledOnTouchOutside(false);
        return dialog;
    }

    protected String getReplyText() {
        SharedPreferences pre = getActivity().getSharedPreferences(OOF_REPLY,
                android.content.Context.MODE_PRIVATE);
        return pre.getString(OOF_REPLY + mAccountId, "");
    }

    protected void setReplyText(String replyText) {
        SharedPreferences.Editor editor = getActivity().getSharedPreferences(OOF_REPLY,
                android.content.Context.MODE_PRIVATE).edit();
        editor.putString(OOF_REPLY + mAccountId, replyText);
        editor.commit();
    }
    /* SPRD: Modify for bug 556038 & 562472{@ */
    @Override
    public void onSaveInstanceState(Bundle outState) {
        outState.putString(REPLY_SUMM, mReplySumm);
        /* SPRD:bug572597 modify begin @{ */
        super.onSaveInstanceState(outState);
        /* @} */
    }
    @Override
    public void onResume () {
        super.onResume();
        Button okButton = ((AlertDialog)getDialog()).getButton(AlertDialog.BUTTON_POSITIVE);
        if (TextUtils.isEmpty(mReplySumm)) {
            okButton.setEnabled(false);
        }
    }
    /* @} */
}
