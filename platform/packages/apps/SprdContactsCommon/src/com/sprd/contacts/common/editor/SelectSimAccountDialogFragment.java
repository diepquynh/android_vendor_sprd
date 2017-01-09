/*
 * Copyright (C) 2010 The Android Open Source Project
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
 * limitations under the License
 */

package com.sprd.contacts.common.editor;

import com.android.contacts.common.model.account.AccountWithDataSet;
import com.sprd.contacts.common.util.SimAccountsListAdapter;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.Fragment;
import android.app.FragmentManager;
import android.content.DialogInterface;
import android.os.Bundle;
import com.android.contacts.common.R;
/**
 * Shows a dialog asking the user which account to chose.
 *
 * The result is passed to {@code targetFragment} passed to {@link #show}.
 */
public final class SelectSimAccountDialogFragment extends DialogFragment {
    public static final String TAG = "SelectSimAccountDialogFragment";

    private static final String KEY_TITLE_RES_ID = "title_res_id";
    private static final String KEY_EXTRA_ARGS = "extra_args";

    public SelectSimAccountDialogFragment() { // All fragments must have a public default constructor.
    }

    /**
     * Show the dialog.
     *
     * @param fragmentManager {@link FragmentManager}.
     * @param targetFragment {@link Fragment} that implements {@link Listener}.
     * @param titleResourceId resource ID to use as the title.
     * @param accountListFilter account filter.
     * @param extraArgs Extra arguments, which will later be passed to
     *     {@link Listener#onAccountChosen}.  {@code null} will be converted to
     *     {@link Bundle#EMPTY}.
     */
    public static void show(FragmentManager fragmentManager,
            Fragment listener, Bundle extraArgs) {
        final Bundle args = new Bundle();
        args.putInt(KEY_TITLE_RES_ID, R.string.choose_sim_card);
        args.putBundle(KEY_EXTRA_ARGS, (extraArgs == null) ? Bundle.EMPTY : extraArgs);

        final SelectSimAccountDialogFragment instance = new SelectSimAccountDialogFragment();
        instance.setArguments(args);
        instance.setTargetFragment(listener, 0);
        // instance.mListener=listener;
        instance.show(fragmentManager, null);
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        final Bundle args = getArguments();

        final SimAccountsListAdapter accountAdapter = new SimAccountsListAdapter(builder.getContext());

        final DialogInterface.OnClickListener clickListener =
                new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();

                onAccountSelected(accountAdapter.getItem(which));
            }
        };

        builder.setTitle(args.getInt(KEY_TITLE_RES_ID));
        builder.setSingleChoiceItems(accountAdapter, 0, clickListener);
        final AlertDialog result = builder.create();
        return result;
    }

    @Override
    public void onCancel(DialogInterface dialog) {
        super.onCancel(dialog);
        Fragment fragment = getTargetFragment();
        if (fragment instanceof Listener) {
            final Listener target = (Listener) fragment;
            target.onAccountSelectorCancelled();
        }
    }

    /**
     * Calls {@link Listener#onAccountChosen} of {@code targetFragment}.
     */
    private void onAccountSelected(AccountWithDataSet account) {
        Fragment fragment = getTargetFragment();
        if (fragment instanceof Listener) {
            final Listener target = (Listener) fragment;
            target.onAccountChosen(account, getArguments().getBundle(KEY_EXTRA_ARGS));
        }
    }

    public interface Listener {
        void onAccountChosen(AccountWithDataSet account, Bundle extraArgs);

        void onAccountSelectorCancelled();
    }

    @Override
    public void onPause() {
        super.onPause();
        dismiss();
    }
}
