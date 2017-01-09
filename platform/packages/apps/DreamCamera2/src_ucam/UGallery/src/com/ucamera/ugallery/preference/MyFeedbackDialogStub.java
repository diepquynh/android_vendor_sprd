/**
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ugallery.preference;

import java.io.File;

import android.os.AsyncTask;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.ucamera.ugallery.MailSender;
import com.ucamera.ugallery.R;
import com.ucamera.ugallery.UCamToast;
import com.ucamera.ugallery.util.HostUtil;

public class MyFeedbackDialogStub extends MyDialogStub {
    private EditText mTopicEdit;
    private EditText mEmailEdit;
    private EditText mDescriptionEdit;
    private Button mOkButton;

    @Override
    public int getContentView() {
        return R.layout.settings_feedback;
    }

    @Override
    protected void onBind() {
        ((TextView)mContext.findViewById(R.id.tv_dialog_title)).setText(R.string.text_feedback_title);
        mTopicEdit = (EditText) mContext.findViewById(R.id.feedback_edit_topic);
        mEmailEdit = (EditText) mContext.findViewById(R.id.feedback_edit_email);
        mDescriptionEdit = (EditText) mContext.findViewById(R.id.feedback_edit_description);
        mOkButton = (Button) mContext.findViewById(R.id.feedback_btn_ok);
        mTopicEdit.addTextChangedListener(mTextWatcher);
        mDescriptionEdit.addTextChangedListener(mTextWatcher);
        mOkButton.setEnabled(false);

        mOkButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                asyncSendMail();
                mContext.closeDialog();
            }
        });
    }

    private final TextWatcher mTextWatcher = new TextWatcher() {
        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
        }

        @Override
        public void afterTextChanged(Editable s) {
            mOkButton.setEnabled(mTopicEdit.getText().length() != 0
                    && mDescriptionEdit.getText().length() != 0);
        }
    };

    private void asyncSendMail() {
        new AsyncTask<Void, Void, Boolean>() {
            @Override
            protected Boolean doInBackground(Void... params) {
                String prefFile = null;
                try {
                    prefFile = HostUtil.zipCameraPreference(getContext());
                    new MailSender(getContext()).addAttachment(prefFile)
                                    .sendMail(generateSubject(), generateMessageBody(),
                                            getReplyTo());
                    return true;
                } catch (Throwable e) {
                    Log.e("Feedback", "Send Feedback fail", e);
                } finally {
                    if (prefFile != null) {
                        try {
                            new File(prefFile).delete();
                        } catch (Throwable e) {
                            // IGNORE
                        }
                    }
                }
                return false;
            }

            @Override
            protected void onPostExecute(Boolean result) {
                int message = R.string.message_feedback_send_success;
                if (result == null || !result.booleanValue()) {
                    message = R.string.message_feedback_send_fail;
                }
                UCamToast.showToast(getContext(), message, Toast.LENGTH_SHORT);
            }
        }.execute();
    }

    private String generateMessageBody() {
        return new StringBuilder()
                .append(mDescriptionEdit.getText().toString())
                .append("\nReplyTo:" + getReplyTo())
                .append("\n\n====  SOFTWARE INFO\n")
                .append(HostUtil.getSoftwareInfo(getContext()))
                .append("\n\n====  CPU INFO\n")
                .append(HostUtil.getCpuInfo(getContext()))
                .append("\n\n====  MEMORY INFO\n")
                .append(HostUtil.getMemoryInfo(getContext()))
                .append("\n\n====  SYSTEM PROPS\n")
                .append(HostUtil.getSystemProps(getContext()))
                .append("\n\n====  CAMERA PARAMETERS\n")
                .append(HostUtil.getCameraParameters(getContext()))
                .toString();
    }

    /**
     * @return
     */
    private String generateSubject() {
        return new StringBuilder()
                .append("【UGallery:Feedback】")
                .append(mTopicEdit.getText().toString())
                .toString();
    }

    private String getReplyTo() {
        return mEmailEdit.getText().toString();
    }
}
