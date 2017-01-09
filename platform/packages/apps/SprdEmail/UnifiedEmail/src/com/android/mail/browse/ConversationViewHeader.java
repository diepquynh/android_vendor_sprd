/*
 * Copyright (C) 2012 Google Inc.
 * Licensed to The Android Open Source Project.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.mail.browse;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import com.android.mail.R;
import com.android.mail.browse.ConversationViewAdapter.ConversationHeaderItem;
import com.android.mail.providers.Conversation;
import com.android.mail.providers.Folder;
import com.android.mail.providers.UIProvider;
import com.android.mail.providers.UIProvider.FolderType;
import com.android.mail.ui.ConversationCheckedSet;
import com.android.mail.ui.ConversationListCallbacks;
import com.android.mail.ui.ConversationUpdater;
import com.android.mail.ui.FolderController;
import com.android.mail.utils.LogTag;
import com.android.mail.utils.LogUtils;
import com.android.mail.utils.Utils;
import android.view.MotionEvent;
import com.android.mail.ui.ControllableActivity;

/**
 * A view for the subject and folders in the conversation view. This container
 * makes an attempt to combine subject and folders on the same horizontal line if
 * there is enough room to fit both without wrapping. If they overlap, it
 * adjusts the layout to position the folders below the subject.
 */
public class ConversationViewHeader extends LinearLayout implements OnClickListener,
        View.OnLongClickListener {

    public interface ConversationViewHeaderCallbacks {
        /**
         * Called in response to a click on the folders region.
         */
        void onFoldersClicked();

        /**
         * Called when the height of the {@link ConversationViewHeader} changes.
         *
         * @param newHeight the new height in px
         */
        void onConversationViewHeaderHeightChange(int newHeight);
        /* SPRD: Modify for bug504210 {@ */
        void onConversationViewHeaderStarredChange(boolean starred);
        /* @} */

        Activity getActivity();
    }

    private static final String LOG_TAG = LogTag.getLogTag();
    private SubjectAndFolderView mSubjectAndFolderView;
    private StarView mStarView;
    private ConversationViewHeaderCallbacks mCallbacks;
    private ConversationAccountController mAccountController;
    private ConversationUpdater mConversationUpdater;
    private ConversationHeaderItem mHeaderItem;
    private Conversation mConversation;

    /* SPRD: Modify for bug492900 {@ */
    private boolean mIsLongClick = false;
    /* @} */
    /**
     * Instantiated from this layout: conversation_view_header.xml
     * @param context
     */
    public ConversationViewHeader(Context context) {
        this(context, null);
    }

    public ConversationViewHeader(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mSubjectAndFolderView =
                (SubjectAndFolderView) findViewById(R.id.subject_and_folder_view);
        mSubjectAndFolderView.setOnLongClickListener(this);
        mStarView = (StarView) findViewById(R.id.conversation_header_star);
        mStarView.setOnClickListener(this);
    }

    public void setCallbacks(ConversationViewHeaderCallbacks callbacks,
            ConversationAccountController accountController,
            ConversationUpdater conversationUpdater) {
        mCallbacks = callbacks;
        mAccountController = accountController;
        mConversationUpdater = conversationUpdater;
    }

    public void setSubject(final String subject) {
        mSubjectAndFolderView.setSubject(subject);
    }

    public void setFolders(Conversation conv) {
        mSubjectAndFolderView.setFolders(mCallbacks, mAccountController.getAccount(), conv);
    }

    public void setStarred(boolean isStarred) {
        mStarView.setStarred(isStarred);
        /* SPRD: Modify for bug496342 {@ */
        boolean showStar = mConversation != null && !mConversation.isInTrash()
                && !mConversation.isInDraft();
        /* @} */
        mStarView.setStarred(isStarred);
        mStarView.setVisibility(showStar ? View.VISIBLE : View.INVISIBLE);
    }

    public void bind(ConversationHeaderItem headerItem) {
        mHeaderItem = headerItem;
        mConversation = mHeaderItem.mConversation;
        if (mSubjectAndFolderView != null) {
            mSubjectAndFolderView.bind(headerItem);
        }
        setStarred(mConversation.starred);
    }

    private int measureHeight() {
        ViewGroup parent = (ViewGroup) getParent();
        if (parent == null) {
            LogUtils.e(LOG_TAG, "Unable to measure height of conversation header");
            return getHeight();
        }
        final int h = Utils.measureViewHeight(this, parent);
        return h;
    }

    /**
     * Update the conversation view header to reflect the updated conversation.
     */
    public void onConversationUpdated(Conversation conv) {
        // The only things we have to worry about when the conversation changes
        // in the conversation header are the folders, priority indicators, and starred state.
        // Updating these will resize the space for the header.
        mConversation = conv;
        /* SPRD: Modify for bug492937 {@ */
        if(mHeaderItem != null){
            mHeaderItem.mConversation.starred = mConversation.starred;
        }
        /* @} */
        setSubject(conv.subject);
        setFolders(conv);
        setStarred(conv.starred);
        if (mHeaderItem != null) {
            final int h = measureHeight();
            if (mHeaderItem.setHeight(h)) {
                mCallbacks.onConversationViewHeaderHeightChange(h);
            }
        }
    }

    @Override
    public void onClick(View v) {
        final int id = v.getId();
        if (mConversation != null && id == R.id.conversation_header_star) {
            mConversation.starred = !mConversation.starred;
            setStarred(mConversation.starred);
            mConversationUpdater.updateConversation(Conversation.listOf(mConversation),
                    UIProvider.ConversationColumns.STARRED, mConversation.starred);
            /* SPRD: Modify for bug504210 {@ */
            mCallbacks.onConversationViewHeaderStarredChange(mConversation.starred);
            /* @} */
            /* SPRD: Modify for bug614018 bug628647 stared email crash issue{@ */
            if (mCallbacks != null) {
                ControllableActivity controllableActivity = (ControllableActivity) mCallbacks
                        .getActivity();
                if (controllableActivity != null) {
                    ConversationCheckedSet set = controllableActivity.getCheckedSet();
                    if (set != null) {
                        Conversation con = set.getConversation(mConversation.id);
                        if (con != null) {
                            con.starred = mConversation.starred;
                        }
                    }

                    FolderController foldercontroller = controllableActivity.getFolderController();
                    Folder currentFolder = (foldercontroller == null) ? null : foldercontroller
                            .getFolder();
                    if (!mConversation.starred
                            && currentFolder != null
                            && currentFolder.isType(FolderType.STARRED)) {
                        ConversationListCallbacks listHandler = controllableActivity
                                .getListHandler();
                        if (listHandler != null) {
                            listHandler.onConversationSelected(null, true);
                        }
                    }
                }
            }
            /* @} */
        }
    }

    @Override
    public boolean onLongClick(View v) {
        /* SPRD: Modify for bug492900 {@ */
        mIsLongClick = true;
        /* @} */
        final DialogFragment frag =
                CopySubjectDialog.newInstance(mSubjectAndFolderView.getSubject());
        /* SPRD: Modify for bug496341 {@ */
        if(mCallbacks != null){
            frag.show(mCallbacks.getActivity().getFragmentManager(), CopySubjectDialog.TAG);
        }
        /* @} */
        return true;
    }

    /* SPRD: Modify for bug492900 {@ */
    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        switch (ev.getAction()) {
        case MotionEvent.ACTION_DOWN:
            break;
        case MotionEvent.ACTION_MOVE:
            break;
        case MotionEvent.ACTION_UP:
            if (mIsLongClick) {
                mIsLongClick = false;
                return true;
            }
            break;
        }
        return super.dispatchTouchEvent(ev);
    }
    /* @} */
    public static class CopySubjectDialog extends DialogFragment
            implements DialogInterface.OnClickListener {

        public static final String TAG = "copy-subject-dialog";

        private static final String ARG_SUBJECT = "subject";

        private String mSubject;

        public static CopySubjectDialog newInstance(String subject) {
            final CopySubjectDialog frag = new CopySubjectDialog();
            final Bundle args = new Bundle(1);
            args.putString(ARG_SUBJECT, subject);
            frag.setArguments(args);
            return frag;
        }

        public CopySubjectDialog() {}

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            mSubject = getArguments().getString(ARG_SUBJECT);
            return new AlertDialog.Builder(getActivity())
                    .setMessage(mSubject)
                    .setPositiveButton(R.string.contextmenu_copy, this)
                    .setNegativeButton(R.string.cancel, this)
                    .create();
        }

        @Override
        public void onClick(DialogInterface dialog, int which) {
            if (which == DialogInterface.BUTTON_POSITIVE) {
                final ClipboardManager clipboard = (ClipboardManager)
                        getActivity().getSystemService(Context.CLIPBOARD_SERVICE);
                clipboard.setPrimaryClip(ClipData.newPlainText(null, mSubject));
            }
        }
    }
}
