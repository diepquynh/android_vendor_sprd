
package com.sprd.contacts.list;

import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.os.Bundle;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.Toast;

import com.android.contacts.R;
import com.android.contacts.common.list.ContactEntryListAdapter;
import com.android.contacts.common.list.ContactEntryListFragment;
import com.android.contacts.common.list.ContactListItemView;
import com.android.contacts.common.list.ContactListFilter;
import com.android.contacts.common.list.ContactListFilterController;
import com.android.contacts.common.list.ContactListFilterController.ContactListFilterListener;
import com.android.contacts.common.util.AccountFilterUtil;
import com.sprd.contacts.common.list.OnPhoneNumberMultiPickerActionListener;
import com.sprd.contacts.list.OnEmailAddressMultiPickerActionListener;

import java.util.HashMap;
import java.util.Set;

public abstract class EmailAddressPickerFragmentSprd extends
        ContactEntryListFragment<ContactEntryListAdapter> implements OnItemLongClickListener {

    private static final String TAG = EmailAddressPickerFragmentSprd.class.getSimpleName();

    private static final int REQUEST_CODE_ACCOUNT_FILTER = 1;
    private OnEmailAddressMultiPickerActionListener mMultiPickerListener;
    private String mShortcutAction;
    private ContactListFilter mFilter;

    private View mAccountFilterHeader;
    /**
     * Lives as ListView's header and is shown when
     * {@link #mAccountFilterHeader} is set to View.GONE.
     */

    private static final String KEY_FILTER = "filter";

    /** true if the loader has started at least once. */
    private boolean mLoaderStarted;

    private class FilterHeaderClickListener implements OnClickListener {
        @Override
        public void onClick(View view) {
            AccountFilterUtil.startAccountFilterActivityForResult(
                    EmailAddressPickerFragmentSprd.this, REQUEST_CODE_ACCOUNT_FILTER, getFilter());
        }
    }

    private OnClickListener mFilterHeaderClickListener = new FilterHeaderClickListener();

    @Override
    protected void onCreateView(LayoutInflater inflater, ViewGroup container) {
        super.onCreateView(inflater, container);
        mAccountFilterHeader = getView().findViewById(R.id.account_filter_header_container);
        mAccountFilterHeader.setOnClickListener(mFilterHeaderClickListener);
        mListView.setOnItemLongClickListener(this);
        updateFilterHeaderView();
    }

    public void setOnEmailAddressMultiPickerActionListener(
            OnEmailAddressMultiPickerActionListener listener) {
        this.mMultiPickerListener = listener;
    }

    @Override
    protected void setSearchMode(boolean flag) {
        super.setSearchMode(flag);
        updateFilterHeaderView();
    }

    private void updateFilterHeaderView() {
        final ContactListFilter filter = getFilter();
        if (mAccountFilterHeader == null || filter == null) {
            return;
        }
        final boolean shouldShowHeader = AccountFilterUtil.updateAccountFilterTitle(
                mAccountFilterHeader, filter, true, getListFilterId());

        if (shouldShowHeader && !isSearchMode()) {
            mAccountFilterHeader.setVisibility(View.VISIBLE);
        } else {
            mAccountFilterHeader.setVisibility(View.GONE);
        }
    }

    @Override
    public void restoreSavedState(Bundle savedState) {
        super.restoreSavedState(savedState);

        if (savedState == null) {
            return;
        }

        mFilter = savedState.getParcelable(KEY_FILTER);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putParcelable(KEY_FILTER, mFilter);
    }

    @Override
    protected void startLoading() {
        mLoaderStarted = true;
        super.startLoading();
    }

    @Override
    protected void configureAdapter() {
        super.configureAdapter();

        final ContactEntryListAdapter adapter = getAdapter();
        if (adapter == null) {
            return;
        }

        if (!isSearchMode() && mFilter != null) {
            adapter.setFilter(mFilter);
        }
    }

    @Override
    protected void prepareEmptyView() {
        super.prepareEmptyView();
        setEmptyText(R.string.listTotalEmailContactsZero);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQUEST_CODE_ACCOUNT_FILTER) {
            if (getActivity() != null) {
                final ContactListFilterController controller = ContactListFilterController
                        .getInstance(getActivity());
                controller.addListener(new ContactListFilterListener() {
                    public void onContactListFilterChanged() {
                        setFilter(controller.getFilter());
                    }
                });
                AccountFilterUtil.handleAccountFilterResult(controller, resultCode, data);
            } else {
                Log.e(TAG, "getActivity() returns null during Fragment#onActivityResult()");
            }
        }
    }

    public ContactListFilter getFilter() {
        return mFilter;
    }

    public void setFilter(ContactListFilter filter) {
        if (filter == null) {
            return;
        }
        mFilter = filter;
        if (mLoaderStarted) {
            reloadData();
        }
        updateFilterHeaderView();
    }

    private void hideSoftKeyboard() {
        // Hide soft keyboard, if visible
        InputMethodManager inputMethodManager = (InputMethodManager)
                mContext.getSystemService(Context.INPUT_METHOD_SERVICE);
        inputMethodManager.hideSoftInputFromWindow(mListView.getWindowToken(), 0);
    }

    @Override
    public int getListFilterId() {
        return R.string.list_filter_emails;
    }

    @Override
    public void onMultiPickerSelected() {
        HashMap<String, String> ret = new HashMap<String, String>();
        HashMap<String, String> cache = new HashMap<String, String>();
        ContactEntryListAdapter adapter = getAdapter();
        Set<Long> checkedItems = adapter.getCurrentCheckedItems();
        for (Long key : checkedItems) {
            cache = getContactCache().getMultiCache().get(key);
            if (cache != null) {
                ret.putAll(cache);
            }
        }
        if (ret.size() == 0) {
            Toast.makeText(getActivity(), R.string.toast_no_contact_selected,
                    Toast.LENGTH_SHORT).show();
        }
        mMultiPickerListener.onPickEmailAddressAction(ret);
    }

    @Override
    public boolean onItemLongClick(AdapterView<?> parent, View view,
            int position, long id) {
        return false;
    }
}
