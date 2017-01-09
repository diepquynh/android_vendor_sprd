
package com.sprd.mail;

import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.os.Bundle;
import com.android.mail.R;
import com.android.mail.providers.Attachment;
import com.android.mail.providers.UIProvider.AttachmentDestination;
import android.content.Context;
import android.app.AlertDialog;
import com.android.mail.utils.LogUtils;

public class DownloadConfirmDialogFragment extends DialogFragment
        implements DialogInterface.OnClickListener {
    private static final String TAG = "DownConfirmFragment";

    public static final String ATTACHMENT_KEY = "attachment";
    public static final String DIALOG_TYPE = "dialogtype";
    public static final String DESTINATION = "destination";
    public static final int FLAG_DOWNLOAD_USER_REQUEST = 1<<1;

    private Attachment mAttachment;
    private int mDialogType;
    private int mDestination;

    private AlertDialog mDialog;
    private DownloadConfirmInterface mCallback;

    public static final class DownloadConfirmDialogType {
        public static final int TYPE_SAVE_ATTACHMENT = 1;
        public static final int TYPE_DOWNLOAD_AGAIN = 2;
        public static final int TYPE_SAVE_DUMMY_ATTACHMENT = 3;
        public static final int TYPE_SHOW_ATTACHMENT = 4;
    }

    public static DownloadConfirmDialogFragment newInstance(Attachment attachment, int dialogType) {
        return newInstance(attachment, dialogType, AttachmentDestination.CACHE);
    }

    public static DownloadConfirmDialogFragment newInstance(Attachment attachment, int dialogType, int destination) {
        final DownloadConfirmDialogFragment frag = new DownloadConfirmDialogFragment();
        final Bundle args = new Bundle(1);
        args.putParcelable(ATTACHMENT_KEY, attachment);
        args.putInt(DIALOG_TYPE, dialogType);
        args.putInt(DESTINATION, destination);
        frag.setArguments(args);
        return frag;
    }

    public DownloadConfirmDialogFragment() {}

    @Override
    public Dialog onCreateDialog(final Bundle savedInstanceState) {
        Context context = getActivity();
        final Bundle args = getArguments();
        mAttachment = args.getParcelable(ATTACHMENT_KEY);
        mDialogType = args.getInt(DIALOG_TYPE);
        mDestination = args.getInt(DESTINATION);
        mDialog = new AlertDialog.Builder(getActivity())
                .setMessage(context.getString(R.string.download_confirm_msg))
                .setPositiveButton(context.getString(R.string.ok), this)
                .setNegativeButton(context.getString(R.string.cancel), this)
                .create();
        return mDialog;
    }

    /* SPRD: Modify for bug497888 */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setRetainInstance(true);
    }
    /* @} */

    @Override
    public void onClick(DialogInterface dialog, int which) {
        if (which == DialogInterface.BUTTON_POSITIVE) {
            if (mAttachment == null) {
                LogUtils.i(TAG, "[onClick][okay]mAttachment is null");
                return;
            }

            mAttachment.flags |= FLAG_DOWNLOAD_USER_REQUEST;
            if (mCallback != null) {
                // Call different callback function by dialog type.
                if (DownloadConfirmDialogType.TYPE_SAVE_ATTACHMENT == mDialogType) {
                    LogUtils.i(TAG, "[onClick][okay]TYPE_SAVE_ATTACHMENT->mCallback.saveAttachment");
                    mCallback.saveAttachment();
                } else if (DownloadConfirmDialogType.TYPE_DOWNLOAD_AGAIN == mDialogType) {
                    LogUtils.i(TAG, "[onClick][okay]TYPE_DOWNLOAD_AGAIN->mCallback.downloadAgain");
                    mCallback.downloadAgain();
                } else if (DownloadConfirmDialogType.TYPE_SAVE_DUMMY_ATTACHMENT == mDialogType) {
                    LogUtils.i(TAG, "[onClick][okay]TYPE_SAVE_DUMMY_ATTACHMENT->mCallback.saveDummyAttachment");
                    mCallback.saveDummyAttachment();
                } else if (DownloadConfirmDialogType.TYPE_SHOW_ATTACHMENT == mDialogType) {
                    LogUtils.i(TAG, "[onClick][okay]TYPE_SHOW_ATTACHMENT->mCallback.showAndDownloadAttachment");
                    mCallback.showAndDownloadAttachment(mDestination);
                }
            } else {
                LogUtils.i(TAG, "[onClick][okay]mCallback is null");
            }
            dismiss();
        } else if (which == DialogInterface.BUTTON_NEGATIVE) {
            LogUtils.i(TAG, "[onClick][cancel]start");
            dismiss();
        }
    }

    public void setCallback(DownloadConfirmInterface cb) {
        mCallback = cb;
    }

}
