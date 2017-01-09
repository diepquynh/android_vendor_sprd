/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.email.activity.setup;

import android.content.Context;
import android.database.Cursor;
import android.graphics.Rect;
import android.os.AsyncTask;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.MultiAutoCompleteTextView.Tokenizer;

import com.android.email.R;
import com.android.email.activity.UiUtilities;
import com.android.emailcommon.mail.Address;

import com.sprd.address.suggests.provider.EmailAddress;
import com.sprd.address.suggests.ui.DropdownAccountsArrayAdapter;
import com.sprd.address.suggests.ui.DropdownAccountsFilter;
import com.sprd.address.suggests.ui.DropdownAddressFilter;
import com.sprd.address.suggests.ui.EmailAccountAutoCompleteTextView;
import com.sprd.address.suggests.ui.EmailAccountAutoCompleteTextView.EmailHistoryTokenizer;
import com.sprd.address.suggests.ui.EmailAccountAutoCompleteTextView.EmailAccountTokenizer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import com.sprd.jiemail.SprdJiEmailAddonStub;

public class AccountSetupBasicsFragment extends AccountSetupFragment {
    /* SPRD:  523599 add for email address history and account list.@{ */
    private EmailAccountAutoCompleteTextView mEmailView;
    private DropdownAccountsArrayAdapter<String> mDropdownAdapter;
    private DropdownAccountsArrayAdapter<String> mHisAddressAdapter;
    private Tokenizer mHisAddressTokenizer = new EmailHistoryTokenizer();
    private Tokenizer mEmailAccountTokenizer = new EmailAccountTokenizer();
    private Context mContext = null;
    private ArrayList<String> mDomainList = new ArrayList<String>();
    private String mDefaultDomain;
    /* @} */

    private View mManualSetupView;
    private boolean mManualSetup;

    public interface Callback extends AccountSetupFragment.Callback {
    }

    public static AccountSetupBasicsFragment newInstance() {
        return new AccountSetupBasicsFragment();
    }

    public AccountSetupBasicsFragment() {}

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        final View view = inflateTemplatedView(inflater, container,
                R.layout.account_setup_basics_fragment, -1);

        /* SPRD:  523599 add for email address history and account list.@{ */
        mEmailView = (EmailAccountAutoCompleteTextView) UiUtilities.getView(view,
                R.id.account_email);
        // When the horizontal screen state, to set Ime not full screen mode.
        /* @ */
        mManualSetupView = UiUtilities.getView(view, R.id.manual_setup);
        mManualSetupView.setOnClickListener(this);

        final TextWatcher textWatcher = new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                /* SPRD:  523599 Add for auto match address when create account. @{ */
                if (!TextUtils.isEmpty(s)) {
                    int indexOfAt = s.toString().indexOf("@");
                    boolean isNotEqDropdownAdapter = mEmailView.getAdapter() != mDropdownAdapter;
                    boolean isNotEqHisAddressAdapter = mEmailView.getAdapter() != mHisAddressAdapter;

                    int selectionStart = mEmailView.getSelectionStart();
                    // If there is a "@", and the cursor is at the back of the "@".
                    // Then we use DropdownAdapter,otherwise use HisAddressAdapter.
                    if (indexOfAt > -1 && indexOfAt < selectionStart) {
                        if (isNotEqDropdownAdapter) {
                            mEmailView.setAdapter(mDropdownAdapter);
                            mEmailView.setTokenizer(mEmailAccountTokenizer);
                        }
                    } else {
                        if (isNotEqHisAddressAdapter) {
                            mEmailView.setAdapter(mHisAddressAdapter);
                            mEmailView.setTokenizer(mHisAddressTokenizer);
                        }
                    }
                }
                if (mDropdownAdapter != null) {
                    String email = mEmailView.getText().toString();
                    String[] emailParts = email.split("@");

                    if (emailParts.length > 0) {
                        mDropdownAdapter.setUserName(emailParts[0]);
                    }
                    // Scroll the editTextView to the top of the screen.
                    int scrollX = mEmailView.getScrollX();
                    int scrollY = mEmailView.getScrollY();
                    int xoff = 0;
                    int yoff = 0;
                    Rect r = new Rect(scrollX, scrollY, scrollX + mEmailView.getDropDownWidth()
                            + xoff, scrollY + 500 + mEmailView.getHeight() + yoff);
                    mEmailView.requestRectangleOnScreen(r, true);
                }
                /* @} */
            }

            @Override
            public void afterTextChanged(Editable s) {
                validateFields();
            }
        };

        mEmailView.addTextChangedListener(textWatcher);

        setPreviousButtonVisibility(View.GONE);

        setManualSetupButtonVisibility(View.VISIBLE);

        /*
         * SPRD: 523599 create adapter.Then set history adapter if we don't have a
         * valid domain set, otherwise we use drop-down adapter with only the
         * selected domain @{
         */
        mContext = getActivity();
        mDomainList = AccountSettingsUtils.collectAutoSetupDomain(mContext);

        mDropdownAdapter = new DropdownAccountsArrayAdapter<String>(mContext,
                R.layout.sprd_account_setup_popup_list, mDomainList);
        mDropdownAdapter.setFilter(new DropdownAccountsFilter<String>(mDropdownAdapter));

        mHisAddressAdapter = new DropdownAccountsArrayAdapter<String>(mContext,
                R.layout.sprd_account_setup_popup_list);
        mHisAddressAdapter.setFilter(new DropdownAddressFilter<String>(mHisAddressAdapter));

        mEmailView.setAdapter(mHisAddressAdapter);
        mEmailView.setTokenizer(mHisAddressTokenizer);

        if (mEmailType != null && mEmailType.startsWith("@")) {
            mDefaultDomain = mEmailType.substring(1);
        }
        if (AccountSettingsUtils.isServerDomainValid(mDefaultDomain)) {
            // replace objects with only the selected valid domain
            mDropdownAdapter.setObjects(Arrays.asList("@" + mDefaultDomain));
            // replace with a new filter
            mDropdownAdapter.setFilter(new DropdownAccountsFilter<String>(mDropdownAdapter));
            mEmailView.setAdapter(mDropdownAdapter);
            mEmailView.setTokenizer(mEmailAccountTokenizer);
        }
        new PrepareAddressDataTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, "");
        /* @} */

        /* SPRD:bug471289 add account guide function {@ */
        mEmailView.setFocusable(true);
        mEmailView.requestFocus();
        mEmailView.post(new Runnable() {
            @Override
            public void run() {
                showKeyboard(mEmailView);
            }
        });

        if(mEmailType!=null){
            mEmailView.setText(mEmailType);
        }
        /* @} */
        /* SPRD:bug566991 add begin @{ */
        if (mEmailType != null && mEmailType.equals("@139.com")) {
            SprdJiEmailAddonStub jiEmailAddon = new SprdJiEmailAddonStub().getInstance();
            if (jiEmailAddon != null) {
                jiEmailAddon.startJiEmailActivity(getActivity());
            }
        }
        /*@}*/

        return view;
    }

    @Override
    public void onViewStateRestored(Bundle savedInstanceState) {
        super.onViewStateRestored(savedInstanceState);
        validateFields();
    }

    @Override
    public void onClick(View v) {
        final int viewId = v.getId();
        final Callback callback = (Callback) getActivity();

        if (viewId == R.id.next) {
            // Handle "Next" button here so we can reset the manual setup diversion
            mManualSetup = false;
            callback.onNextButton();
        } else if (viewId == R.id.manual_setup) {
            mManualSetup = true;
            callback.onNextButton();
        } else {
            super.onClick(v);
        }
    }

    private void validateFields() {
        final String emailField = getEmail();
        final Address[] addresses = Address.parse(emailField);

        final boolean emailValid = !TextUtils.isEmpty(emailField)
                && addresses.length == 1
                && !TextUtils.isEmpty(addresses[0].getAddress());

        setNextButtonEnabled(emailValid);
    }


    /**
     * Set visibitlity of the "manual setup" button
     * @param visibility {@link View#INVISIBLE}, {@link View#VISIBLE}, {@link View#GONE}
     */
    public void setManualSetupButtonVisibility(int visibility) {
        mManualSetupView.setVisibility(visibility);
    }

    @Override
    public void setNextButtonEnabled(boolean enabled) {
        super.setNextButtonEnabled(enabled);
        mManualSetupView.setEnabled(enabled);
    }

    public void setEmail(final String email) {
        mEmailView.setText(email);
    }

    public String getEmail() {
        return mEmailView.getText().toString().trim();
    }

    public boolean isManualSetup() {
        return mManualSetup;
    }

    /**
     * SPRD:  523599 PrepareAddressDataTask for querying address outside of the UI
     * thread, and update the data of mHisAddressAdapter.
     */
    private class PrepareAddressDataTask extends AsyncTask<String, Void, Cursor> {
        private HashSet<String> mHisAddressSet = new HashSet<String>();

        @Override
        protected Cursor doInBackground(String... params) {
            return EmailAddress.queryAddress(mContext, EmailAddress.CONTENT_URI, params[0],
                    AccountSettingsUtils.FROM_ACCOUNT_ADD);
        }

        @Override
        protected void onPostExecute(Cursor c) {
            super.onPostExecute(c);
            mHisAddressAdapter.clear();
            if (c != null) {
                try {
                    if (!TextUtils.isEmpty(mEmailType) && mEmailType.startsWith("@")) {
                        while (c.moveToNext()) {
                            String address = c.getString(1);
                            if (EmailAddress.isValidEmailAddress(address)) {
                                address = address.split("@")[0] + mEmailType;
                                mHisAddressSet.add(address);
                            }
                        }
                        mHisAddressAdapter.addAll(mHisAddressSet);
                    } else {
                        while (c.moveToNext()) {
                            String address = c.getString(1);
                            mHisAddressAdapter.add(address);
                        }
                    }
                } finally {
                    if (c != null) {
                        c.close();
                    }
                }
            }

        }

    }

}
