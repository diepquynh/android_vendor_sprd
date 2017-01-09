/**
 * Copyright (c) 2013, Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.email.activity;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.os.Looper;
import android.os.Parcelable;
import android.os.Handler;
import android.text.InputFilter;
import android.text.SpannableStringBuilder;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Surface;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

import com.android.email.R;
import com.android.emailcommon.provider.EmailContent;
import com.android.email.activity.setup.AccountSettingsUtils;
import com.android.mail.compose.ComposeActivity;
import com.android.mail.utils.LogUtils;
import android.widget.Toast;
import android.util.Log;
import com.android.emailcommon.utility.EmailAsyncTask;
import com.android.mail.analytics.Analytics;
import com.android.mail.compose.AttachmentsView.AttachmentFailureException;
import com.android.mail.providers.Attachment;
import com.android.mail.utils.Utils;
import com.android.mail.utils.AttachmentUtils;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import com.sprd.xposed.DrmXposedUtils;

import java.util.List;

public class ComposeActivityEmail extends ComposeActivity
        implements InsertQuickResponseDialog.Callback {
    static final String insertQuickResponseDialogTag = "insertQuickResponseDialog";

    /* SPRD: Modify for bug475074 {@ */
    private static final String TAG = "ComposeActivityEmail";
    private LoadingAttachProgressDialog mProgressDialog = null;
    private final EmailAsyncTask.Tracker mTaskTracker = new EmailAsyncTask.Tracker();
    /* @} */

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        final boolean superCreated = super.onCreateOptionsMenu(menu);
        //SPRD Modify bug493411 Quick response not display while reply mail from statusbar notification
        if (mReplyFromAccount != null || mAccount != null) {
            getMenuInflater().inflate(R.menu.email_compose_menu_extras, menu);
            return true;
        } else {
            LogUtils.d(LogUtils.TAG, "mReplyFromAccount is null, not adding Quick Response menu");
            return superCreated;
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.insert_quick_response_menu_item) {
            if (mReplyFromAccount == null || mReplyFromAccount.account == null){
                LogUtils.d(LogUtils.TAG, "mReplyFromAccount is null, do nothing");
                return true;
            }
            InsertQuickResponseDialog dialog = InsertQuickResponseDialog.newInstance(null,
                    mReplyFromAccount.account);
            /* SPRD: Modify for bug497167 {@ */
            final View view = getWindow().peekDecorView();
            if (view != null&& view.getWindowToken() != null) {
                InputMethodManager imm = (InputMethodManager) getSystemService(
                        Context.INPUT_METHOD_SERVICE);
                imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
            }
            /* @} */
            dialog.show(getFragmentManager(), insertQuickResponseDialogTag);
        }
        return super.onOptionsItemSelected(item);
    }

    public void onQuickResponseSelected(CharSequence quickResponse) {
        /* SPRD: Modify for bug 607985 {@ */
        int maxLength = getResources().getInteger(com.android.mail.R.integer.body_with_character_lengths);
        int bodyLen = mBodyView != null ? mBodyView.getText().length() : 0;
        int quickResponseLen = quickResponse != null ? quickResponse.length() : 0;
        if (bodyLen + quickResponseLen > maxLength) {
            Toast.makeText(this, getResources().getString(
                    com.android.mail.R.string.body_input_more, maxLength), Toast.LENGTH_SHORT).show();
            return;
        }
        /* @} */
        final int selEnd = mBodyView.getSelectionEnd();
        final int selStart = mBodyView.getSelectionStart();

        if (selEnd >= 0 && selStart >= 0) {
            final SpannableStringBuilder messageBody =
                    new SpannableStringBuilder(mBodyView.getText());
            final int replaceStart = selStart < selEnd ? selStart : selEnd;
            final int replaceEnd = selStart < selEnd ? selEnd : selStart;
            messageBody.replace(replaceStart, replaceEnd, quickResponse);
            mBodyView.setText(messageBody);
            /* SPRD: Modify for bug 604572 {@ */
            int selectionIndex = replaceStart + quickResponse.length();
            if (selectionIndex >= maxLength) {
                Toast.makeText(this, getResources().getString(
                        com.android.mail.R.string.body_input_more, maxLength), Toast.LENGTH_SHORT).show();
                selectionIndex = maxLength;
            }
            mBodyView.setSelection(selectionIndex);
            /* @} */
        } else {
            mBodyView.append(quickResponse);
            mBodyView.setSelection(mBodyView.getText().length());
        }
    }

    @Override
    protected String getEmailProviderAuthority() {
        return EmailContent.AUTHORITY;
    }
    /**
     * SPRD: 523599 Add for save recipients and auto match emailAddress when
     * create mail.@{
     */
    @Override
    protected boolean saveRecipientsToHistoryDB(List<String> recipients) {
        if (recipients != null) {
            for (int i = 0; i < recipients.size(); i++) {
                String address = recipients.get(i);
                AccountSettingsUtils.saveAddressToHistoryDB(this, address,
                        AccountSettingsUtils.FROM_OTHER_ADD);
            }
            return true;
        }
        return false;
    }
    /** @} */

    /* SPRD: Modify for bug557044 {@ */
    /**
     * we cancel all the task if activity destroy
     */
    @Override
    protected void onDestroy() {
        super.onDestroy();
        mTaskTracker.cancelAllInterrupt();
    }

    /**
     * Override default to load attachment in background, not block UI thread.
     * This is the entrance of Share/Send attachment from other Apps.
     */
    @Override
    protected void initAttachmentsFromIntent(Intent intent) {
        Bundle extras = intent.getExtras();
        if (extras == null) {
            extras = Bundle.EMPTY;
        }
        final String action = intent.getAction();
        ArrayList<Uri> uriArray = null;
        HashMap<Integer, ArrayList<Uri>> uriMap = new HashMap<Integer, ArrayList<Uri>>();
        if (!mAttachmentsChanged) {
            if (extras.containsKey(EXTRA_ATTACHMENTS)) {
                uriArray = new ArrayList<Uri>();
                String[] uris = (String[]) extras.getSerializable(EXTRA_ATTACHMENTS);
                for (String uriString : uris) {
                    final Uri uri = Uri.parse(uriString);
                    uriArray.add(uri);
                }
                uriMap.put(TYPE_ATTACHMENTS, uriArray);
            }
            if (extras.containsKey(Intent.EXTRA_STREAM)) {
                if (Intent.ACTION_SEND_MULTIPLE.equals(action)) {
                    uriArray = new ArrayList<Uri>();
                    ArrayList<Parcelable> uris = extras.getParcelableArrayList(Intent.EXTRA_STREAM);
                    for (Parcelable uri : uris) {
                        Uri newUri = (Uri) uri;
                        uriArray.add(newUri);
                    }
                    uriMap.put(TYPE_SEND_MULTIPLE, uriArray);
                } else {
                    uriArray = new ArrayList<Uri>();
                    final Uri uri = (Uri) extras.getParcelable(Intent.EXTRA_STREAM);
                    /*SPRD modify for bug519331  no permission to open download provider {@ */
                    try{
                        if(!DrmXposedUtils.getInstance().isSDFile(this, uri)) return;
                    } catch (SecurityException e) {
                        LogUtils.e(TAG, e, "Error read attachment,permission denied");
                        showErrorToast(getString(R.string.attachment_permission_denied));
                        return;
                    }
                    /* @} */
                    if (uri != null) {
                        uriArray.add(uri);
                    }
                    uriMap.put(TYPE_SEND, uriArray);
                }
            }

            loadAttachmentsInBackground(uriMap);
        }
    }

    /**
     * Load attachment in background EmailAsyncTask, and show Load Attachment
     * ProgressDialog when attachments is loading. avoid user make UI operation
     * cause ANR.
     */
    @Override
    public void loadAttachmentsInBackground(final HashMap<Integer, ArrayList<Uri>> uriMap) {
        LogUtils.d(TAG, "loadAttachmentsInBackground: %s", uriMap.toString());
        final HashMap<Integer, ArrayList<Uri>> uriMaps = uriMap;
        if (uriMap.size() > 0) {
            new EmailAsyncTask<Void, Void, HashMap<Integer, List<Attachment>>>(mTaskTracker) {

                @Override
                protected HashMap<Integer, List<Attachment>> doInBackground(Void... params) {
                    showLoadAttachmentProgressDialog(this);
                    HashMap<Integer, List<Attachment>> attachmentsMap = new HashMap<Integer, List<Attachment>>();
                    if (uriMaps.containsKey(TYPE_ADD)) {
                        attachmentsMap.put(TYPE_ADD, handleAttachmentFromAdd(TYPE_ADD, uriMaps));
                    } else {
                        if (uriMaps.containsKey(TYPE_ATTACHMENTS)) {
                            attachmentsMap.put(TYPE_ATTACHMENTS,
                                    handleAttachmentsFromIntent(TYPE_ATTACHMENTS, uriMaps));
                        }
                        if (uriMaps.containsKey(TYPE_SEND_MULTIPLE)) {
                            attachmentsMap.put(TYPE_SEND_MULTIPLE,
                                    handleAttachmentsFromIntent(TYPE_SEND_MULTIPLE, uriMaps));
                        } else if (uriMaps.containsKey(TYPE_SEND)) {
                            attachmentsMap.put(TYPE_SEND,
                                    handleAttachmentsFromIntent(TYPE_SEND, uriMaps));
                        }
                    }
                    return attachmentsMap;
                }

                @Override
                protected void onCancelled(HashMap<Integer, List<Attachment>> attachmentsMap) {
                    LogUtils.d(TAG, "loadAttachmentsInBackground : onCancelled");
                    super.onCancelled(attachmentsMap);
                    releaseProgressDialog(this);
                    /* SPRD: Modify for bug 565076 {@ */
                    showErrorToast(getString(R.string.loaded_attachment_failed));
                    ComposeActivityEmail.this.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
                    /* @} */
                    // close opend vcf fds avoid fd leakage.
                    List<Attachment> attachments = null;
                    if (mAttachmentsView != null) {
                        // Judge whether attachments is null, avoid NPE
                        if (null == attachmentsMap || attachmentsMap.size() == 0) {
                            return;
                        }
                        if (uriMaps.containsKey(TYPE_SEND)) {
                            attachments = attachmentsMap.get(TYPE_SEND);
                        }
                        if (attachments == null || attachments.size() == 0) {
                            return;
                        }
                        for (Attachment att : attachments) {
                            mAttachmentsView.closeVcfFileDescriptors(att);
                        }
                    }
                }

                @Override
                protected void onSuccess(HashMap<Integer, List<Attachment>> attachmentsMap) {
                    LogUtils.d(TAG, "loadAttachmentsInBackground : onSuccess");
                    super.onSuccess(attachmentsMap);
                    releaseProgressDialog(this);
                    /* SPRD: Modify for bug 618526 Compose Screen can not rotate issue {@ */
                    ComposeActivityEmail.this
                            .setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
                    /* @} */
                    if (null == attachmentsMap || attachmentsMap.size() == 0) {
                        return;
                    }
                    // Add attachments list to UI
                    long totalSize = 0;
                    if (attachmentsMap.containsKey(TYPE_ADD)) {
                        totalSize = addAttachments(attachmentsMap.get(TYPE_ADD));
                        if (totalSize > 0) {
                            mAttachmentsChanged = true;
                            updateSaveUi();
                        }
                        return;
                    } else {
                        if (attachmentsMap.containsKey(TYPE_ATTACHMENTS)) {
                            totalSize += addAttachments(attachmentsMap.get(TYPE_ATTACHMENTS));
                        }
                        if (attachmentsMap.containsKey(TYPE_SEND_MULTIPLE)) {
                            totalSize = 0;
                            List<Attachment> attachments = attachmentsMap.get(TYPE_SEND_MULTIPLE);
                            if (attachments != null && attachments.size() > 0) {
                                 /* SPRD: modify for bug 520803 @{ */
                                if (DrmXposedUtils.getInstance().drmPluginEnabled()) {
                                    totalSize += addAttachmentsForDrm(attachments);
                                } else {
                                    totalSize += addAttachments(attachments);
                                }
                                /* }@ */
                            }

                        } else if (attachmentsMap.containsKey(TYPE_SEND)) {
                            totalSize += addAttachments(attachmentsMap.get(TYPE_SEND));
                        }
                    }

                    if (totalSize > 0) {
                        mAttachmentsChanged = true;
                        updateSaveUi();

                        Analytics.getInstance().sendEvent("send_intent_with_attachments",
                                Integer.toString(getAttachments().size()), null, totalSize);
                    }
                }

            }.executeParallel((Void[]) null);
        }
    }

    /**
     * When have add attachment, show progress dialog.
     */
    private void showLoadAttachmentProgressDialog(
            final EmailAsyncTask<Void, Void, HashMap<Integer, List<Attachment>>> task) {
        runOnUiThread(new Runnable() {
            public void run() {
                int orientation = ComposeActivityEmail.this.getResources().getConfiguration().orientation;
                /* SPRD: Modify for bug 568043 {@ */
                int rotation = ComposeActivityEmail.this.getWindowManager().getDefaultDisplay().getRotation();
                /* @} */
                switch (orientation) {
                    case Configuration.ORIENTATION_LANDSCAPE:
                        /* SPRD: Modify for bug 568043 {@ */
                        if (rotation == Surface.ROTATION_270) {
                            ComposeActivityEmail.this
                                    .setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE);
                        } else {
                            ComposeActivityEmail.this
                                    .setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
                        }
                        /* @} */
                        break;
                    case Configuration.ORIENTATION_PORTRAIT:
                        ComposeActivityEmail.this
                                .setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
                        break;
                    default:
                        break;
                }
                /* SPRD: Modify for bug 565076 {@ */
                if (isResumed()) {
                    FragmentManager fm = getFragmentManager();
                    mProgressDialog = LoadingAttachProgressDialog.newInstance(null);
                    // Set Loading Task for loading dialog
                    mProgressDialog.setLoadingTask(task);
                    fm.beginTransaction().add(mProgressDialog, LoadingAttachProgressDialog.TAG)
                            .commitAllowingStateLoss();
                    fm.executePendingTransactions();
                /* SPRD: Modify for bug 614984 {@ */
                } else {
                    //if Avtivity is not resumed,we should set screen orientation to unspecified
                    ComposeActivityEmail.this
                            .setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
                /* @} */
                }
                /* @} */
            }
        });
    }

    /**
     * When finished or canceled add attachment, release progress dialog.
     */
    private void releaseProgressDialog(
            final EmailAsyncTask<Void, Void, HashMap<Integer, List<Attachment>>> task) {
        mProgressDialog = (LoadingAttachProgressDialog) getFragmentManager().findFragmentByTag(
                LoadingAttachProgressDialog.TAG);
        if (mProgressDialog == null) {
            /* SPRD: Modify for bug 618526 Compose Screen can not rotate issue {@ */
            LogUtils.i(TAG, "releaseProgressDialog : mProgressDialog is NULL");
            /* @} */
            return;
        }
        final EmailAsyncTask<Void, Void, HashMap<Integer, List<Attachment>>> taskRunning = mProgressDialog
                .getLoadingTask();
        if (taskRunning == null || !taskRunning.equals(task)) {
            // do nothing if the running task was not started by the calling
            // task.
            /* SPRD: Modify for bug 618526 Compose Screen can not rotate issue {@ */
            LogUtils.i(TAG, "releaseProgressDialog : releaseProgressDialog taskRunning is NULL");
            /* @} */
            return;
        }
        mProgressDialog.dismissAllowingStateLoss();
        // Reset Loading Task for loading dialog
        mProgressDialog.setLoadingTask(null);
        mProgressDialog = null;
    }

    /**
     * Loading attachment Progress dialog
     */
    public static class LoadingAttachProgressDialog extends DialogFragment {
        @SuppressWarnings("hiding")
        public static final String TAG = "LoadingAttachProgressDialog";

        private EmailAsyncTask<Void, Void, HashMap<Integer, List<Attachment>>> mLoadingTask;

        /**
         * Create a dialog for Loading attachment asynctask.
         */
        public static LoadingAttachProgressDialog newInstance(Fragment parentFragment) {
            LoadingAttachProgressDialog f = new LoadingAttachProgressDialog();
            f.setTargetFragment(parentFragment, 0);
            return f;
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            setRetainInstance(true);
        }

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            Context context = getActivity();
            ProgressDialog dialog = new ProgressDialog(context);
            dialog.setIndeterminate(true);
            dialog.setMessage(getString(R.string.loading_attachment));
            dialog.setCancelable(true);
            dialog.setCanceledOnTouchOutside(false);
            return dialog;
        }

        /**
         * Listen for cancellation, which can happen from places other than the
         * negative button (e.g. touching outside the dialog), and stop the
         * checker
         */
        @Override
        public void onCancel(DialogInterface dialog) {
            super.onCancel(dialog);
            if (mLoadingTask != null) {
                mLoadingTask.cancel(true);
            }
        }

        @Override
        public void onDismiss(DialogInterface dialog) {
            super.onDismiss(dialog);
            if (mLoadingTask != null) {
                mLoadingTask = null;
            }
            /* SPRD: Modify for bug 563878 {@ */
            ComposeActivity activity = (ComposeActivity)getActivity();
            if (activity != null) {
                /* SPRD: Modify for bug 618526 Compose Screen can not rotate issue {@ */
                LogUtils.d(ComposeActivityEmail.TAG, "onDismiss : setRequestedOrientation");
                /* @} */
                activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
            }
            /* @} */
        }

        /**
         * Set Runnable task for Loading Progress dialog
         * @param task Asynctask of background loading
         */
        public void setLoadingTask(
                final EmailAsyncTask<Void, Void, HashMap<Integer, List<Attachment>>> task) {
            mLoadingTask = task;
        }

        /**
         * get Runnable task from loading process dialog
         * @return
         */
        public final EmailAsyncTask<Void, Void, HashMap<Integer, List<Attachment>>> getLoadingTask() {
            return mLoadingTask;
        }
    }

    /**
     * If the dialog doesn't have attached task , which means the dialog is
     * meaningless, just dismiss it.
     */
    @Override
    protected void onStart() {
        super.onStart();
        if (mProgressDialog == null) {
            mProgressDialog = (LoadingAttachProgressDialog) getFragmentManager().findFragmentByTag(
                    LoadingAttachProgressDialog.TAG);
            if (mProgressDialog != null && mProgressDialog.getLoadingTask() == null) {
                mProgressDialog.dismissAllowingStateLoss();
                mProgressDialog = null;
            }
        }
    }

    /**
     * Helper function to handle a attachment to add.
     * @return List<Attachment> attachements : a attached files.
     */
    private List<Attachment> handleAttachmentFromAdd(int type,
            HashMap<Integer, ArrayList<Uri>> uriMaps) {
        List<Attachment> attachments = new ArrayList<Attachment>();
        ArrayList<Uri> uris = uriMaps.get(type);
        if (uris == null || uris.size() < 1) {
            return attachments;
        }
        for (Uri contentUri : uris) {
            try {
                /*SPRD: modify for bug612567 {@*/
                if (contentUri != null && mAttachmentsView != null && !handleSpecialAttachmentUri(contentUri)) {
                    attachments.add(mAttachmentsView.generateLocalAttachment(contentUri));
                }
                /* @}*/
            } catch (AttachmentFailureException e) {
                LogUtils.e(TAG, e, "Error adding attachment");
                showErrorToastInfo(getResources().getString(
                        e.getErrorRes(),
                        AttachmentUtils.convertToHumanReadableSize(getApplicationContext(),
                                mAccount.settings.getMaxAttachmentSize())));
            }
        }
        return attachments;
    }

    /**
     * Helper function to handle a list of uris to attach.
     * @return List<Attachment> attachements : all successfully attached files.
     */
    private List<Attachment> handleAttachmentsFromIntent(int type,
            HashMap<Integer, ArrayList<Uri>> uriMaps) {
        List<Attachment> attachments = new ArrayList<Attachment>();
        ArrayList<Uri> uris = uriMaps.get(type);
        if (uris == null || uris.size() < 1) {
            return attachments;
        }
        for (Uri uri : uris) {
            try {
                if (uri != null) {
                    if ("file".equals(uri.getScheme())) {
                        // We must not allow files from /data, even from our process.
                        final File f = new File(uri.getPath());
                        final String filePath = f.getCanonicalPath();
                        if (filePath.startsWith(DATA_DIRECTORY_ROOT)) {
                          showErrorToast(getString(R.string.attachment_permission_denied));
                          Analytics.getInstance().sendEvent(ANALYTICS_CATEGORY_ERRORS,
                                  "send_intent_attachment", "data_dir", 0);
                          continue;
                        }
                    }
                    if (!handleSpecialAttachmentUri(uri)) {
                        final Attachment a = mAttachmentsView.generateLocalAttachment(uri);
                        attachments.add(a);

                        Analytics.getInstance().sendEvent("send_intent_attachment",
                                Utils.normalizeMimeType(a.getContentType()), null, a.size);
                    }
                }
            } catch (AttachmentFailureException e) {
                LogUtils.e(TAG, e, "Error adding attachment uri [%s]", uri);
                showAttachmentTooBigToastInfo(e.getErrorRes());
            } catch (IllegalStateException e) {
                // Maybe this Exception happen when the file of the URI doesn't
                // exsit
                LogUtils.e(TAG, e, "Error adding attachment uri [%s]", uri);
                showErrorToastInfo("Failed add attachment IllegalStateException");
            } catch (IOException | SecurityException e) {
                LogUtils.e(TAG, e, "Error adding attachment");
                showErrorToastInfo(getString(R.string.attachment_permission_denied));
            }
        }
        return attachments;
    }

    /**
     * When an attachment is too large to be added to a message, show a toast.
     * This method also updates the position of the toast so that it is shown
     * clearly above they keyboard if it happens to be open.
     */
    // SPRD: To deal with drm files.
    public void showAttachmentTooBigToastInfo(int errorRes) {
        String maxSize = AttachmentUtils.convertToHumanReadableSize(getApplicationContext(),
                mAccount.settings.getMaxAttachmentSize());
        showErrorToastInfo(getString(errorRes, maxSize));
    }

    // SPRD: To deal with drm files.
    /* SPRD: Modify for bug640499  {@ */
    public void showErrorToastInfo(final String message) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                Toast t = Toast.makeText(ComposeActivityEmail.this, message, Toast.LENGTH_LONG);
                t.setText(message);
                t.setGravity(Gravity.CENTER_HORIZONTAL, 0,
                        getResources().getDimensionPixelSize(R.dimen.attachment_toast_yoffset));
                t.show();
            }
        });
    }
    /* @} */
}
