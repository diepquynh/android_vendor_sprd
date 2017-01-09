
package com.sprd.contacts.list;

import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.Toast;
import android.widget.AdapterView.OnItemLongClickListener;

import com.android.contacts.R;
import com.android.contacts.common.list.ContactEntryListAdapter;
import com.android.contacts.common.list.ContactEntryListFragment;
import com.android.contacts.common.list.ContactListFilter;
import com.android.contacts.common.list.ContactListFilterController;
import com.android.contacts.common.list.ContactListItemView;
import com.android.contacts.common.list.DirectoryListLoader;
import com.android.contacts.common.list.ContactListFilterController.ContactListFilterListener;
import com.android.contacts.common.list.ShortcutIntentBuilder.OnShortcutIntentCreatedListener;
import com.android.contacts.common.util.AccountFilterUtil;
import com.android.contacts.common.util.Constants;
import com.sprd.contacts.common.util.MultiContactDataCacheUtils;
import com.sprd.contacts.common.util.MultiContactDataCacheUtils.CacheMode;
import com.sprd.contacts.common.list.OnPhoneNumberMultiPickerActionListener;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
import com.sprd.contacts.activities.ContactSelectionMultiTabActivity;
import com.android.contacts.activities.ContactSelectionActivity;
import com.android.contacts.common.list.ContactEntryListAdapter.IOnListCheckedChangeListener;

public class AllInOneDataPickerFragment extends ContactEntryListFragment<ContactEntryListAdapter>
        implements OnShortcutIntentCreatedListener, OnItemLongClickListener {
    private static final String TAG = AllInOneDataPickerFragment.class.getSimpleName();

    private static final int REQUEST_CODE_ACCOUNT_FILTER = 1;

    private OnAllInOneDataPickerActionListener mListener;
    private OnAllInOneDataMultiPickerActionListener mMultiPickerListener;

    private String mShortcutAction;

    private ContactListFilter mFilter;

    private List<String> mCascadingData;

    private View mAccountFilterHeader;

    /**
     * Lives as ListView's header and is shown when
     * {@link #mAccountFilterHeader} is set to View.GONE.
     */

    private static final String KEY_FILTER = "filter";
    private static final String KEY_CASCADING_DATA = "cascadingData";

    /** true if the loader has started at least once. */
    private boolean mLoaderStarted;
    private int mCheckedLimitCount = 3500;

    private ContactListItemView.PhotoPosition mPhotoPosition =
            ContactListItemView.getDefaultPhotoPosition(false);

    private class FilterHeaderClickListener implements OnClickListener {
        @Override
        public void onClick(View view) {
            AccountFilterUtil.startAccountFilterActivityForResult(
                    AllInOneDataPickerFragment.this, REQUEST_CODE_ACCOUNT_FILTER, getFilter());
        }
    }

    private OnClickListener mFilterHeaderClickListener = new FilterHeaderClickListener();

    public AllInOneDataPickerFragment() {
        setQuickContactEnabled(false);
        setPhotoLoaderEnabled(true);
        setSectionHeaderDisplayEnabled(true);
        setDirectorySearchMode(DirectoryListLoader.SEARCH_MODE_DATA_SHORTCUT);

        // Show nothing instead of letting caller Activity show something.
        setHasOptionsMenu(true);
        setContactCacheModel(CacheMode.MULTI, Data.DATA1, Phone.DISPLAY_NAME_PRIMARY);
    }

    public void setOnAllInOneDataPickerActionListener(OnAllInOneDataPickerActionListener listener) {
        this.mListener = listener;
    }

    public void setOnAllInOneDataMultiPickerActionListener(
            OnAllInOneDataMultiPickerActionListener listener) {
        this.mMultiPickerListener = listener;
    }

    @Override
    protected void onCreateView(LayoutInflater inflater, ViewGroup container) {
        super.onCreateView(inflater, container);
        setVisibleScrollbarEnabled(!isLegacyCompatibilityMode());
        // add for:Bug538461 During conference meeting, more than five people select
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

        if (Constants.DEBUG)
            Log.d(TAG, "isSearchMode:" + isSearchMode());
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
        mCascadingData = savedState.getStringArrayList(KEY_CASCADING_DATA);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putParcelable(KEY_FILTER, mFilter);
        outState.putStringArrayList(KEY_CASCADING_DATA, new ArrayList<String>(mCascadingData));
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        final int itemId = item.getItemId();
        if (itemId == android.R.id.home) {
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    /**
     * @param shortcutAction either {@link Intent#ACTION_CALL} or
     *            {@link Intent#ACTION_SENDTO} or null.
     */
    public void setShortcutAction(String shortcutAction) {
        this.mShortcutAction = shortcutAction;
    }

    @Override
    protected void onItemClick(int position, long id) {
        Uri dataUri = null;

        AllInOneDataListAdapter adapter = (AllInOneDataListAdapter) getAdapter();
        dataUri = adapter.getDataUri(position);

        if (dataUri != null) {
            pickAllInOneData(dataUri);
        } else {
            Log.w(TAG, "Item at " + position + " was clicked before adapter is ready. Ignoring");
        }
    }

    // add for:Bug538461 During conference meeting, more than 5 people selected
    public void onItemClick(AdapterView<?> parent, View view, int position,
            long id) {
        hideSoftKeyboard();
        int adjPosition = position - mListView.getHeaderViewsCount();
        boolean isChecked = getAdapter().isChecked(adjPosition);
        boolean isMultiTabCalling = getActivity().toString().contains(
                "ContactSelectionMultiTabActivity");
        if (isMultiPickerSupported()) {
            if (adjPosition >= 0 && isMultiTabCalling) {
                if (!isChecked
                        && ((ContactSelectionMultiTabActivity) getActivity()).mCheckedLimitCount <= 0) {
                    Toast.makeText(
                            mContext,
                            mContext.getString(
                                    R.string.contacts_selection_for_mms_limit,
                                    mCheckedLimitCount), Toast.LENGTH_SHORT)
                            .show();
                    return;
                }
                getAdapter().setChecked(adjPosition, !isChecked);
                ((ContactSelectionMultiTabActivity) getActivity()).mCheckedLimitCount = (isChecked == true) ? ((ContactSelectionMultiTabActivity) getActivity()).mCheckedLimitCount + 1
                        : ((ContactSelectionMultiTabActivity) getActivity()).mCheckedLimitCount - 1;
            } else {
                if (!isChecked
                        && ((ContactSelectionActivity) getActivity()).mCheckedLimitCount <= 0
                        && getAdapter().getCurrentCheckedItems().size() >= mCheckedLimitCount) {
                    Toast.makeText(
                            mContext,
                            mContext.getString(
                                    R.string.contacts_selection_for_mms_limit,
                                    mCheckedLimitCount), Toast.LENGTH_SHORT)
                            .show();
                    return;
                }
                getAdapter().setChecked(adjPosition, !isChecked);
                ((ContactSelectionActivity) getActivity()).mCheckedLimitCount = ((isChecked == true) ? ((ContactSelectionActivity) getActivity()).mCheckedLimitCount + 1
                        : ((ContactSelectionActivity) getActivity()).mCheckedLimitCount - 1);

            }
            refreshRevertButton();
            getAdapter().notifyDataSetChanged();
        }
    }
    @Override
    protected void startLoading() {
        mLoaderStarted = true;
        super.startLoading();
    }

    @Override
    protected ContactEntryListAdapter createListAdapter() {
        AllInOneDataListAdapter adapter = new AllInOneDataListAdapter(getActivity());
        adapter.setDisplayPhotos(true);
        adapter.setCascadingData(mCascadingData);
        /**
         * SPRD:Bug549112 Can't choose or delete in multi tab
         * @{
         */
        adapter.setOnListCheckedChangeListener(new IOnListCheckedChangeListener() {
            @Override
            public void onListCheckedChanged() {
                refreshRevertButton();
            }
        });
        /**
         * @}
         */
        return adapter;
    }

    @Override
    protected void configureAdapter() {
        super.configureAdapter();

        final AllInOneDataListAdapter adapter = (AllInOneDataListAdapter) getAdapter();
        if (adapter == null) {
            return;
        }

        if (!isSearchMode() && mFilter != null) {
            adapter.setFilter(mFilter);
        }

        if (!isLegacyCompatibilityMode()) {
            adapter.setPhotoPosition(mPhotoPosition);
        }
    }

    @Override
    protected View inflateView(LayoutInflater inflater, ViewGroup container) {
            return inflater.inflate(R.layout.contact_list_content_overlay, null);
    }

    @Override
    protected void prepareEmptyView() {
        super.prepareEmptyView();
        int dataSize = 0;
        if (mCascadingData == null) {
            setEmptyText(R.string.listTotalPhoneContactsZero);
            return;
        }
        if (mCascadingData.size() == 1) {
            if (mCascadingData.contains(Email.CONTENT_ITEM_TYPE)) {
                setEmptyText(R.string.noContactwithEmail);
            } else if (mCascadingData.contains(Phone.CONTENT_ITEM_TYPE)) {
                setEmptyText(R.string.noContactsWithPhoneNumbers);
            }
        } else if (mCascadingData.size() == 2 && mCascadingData.contains(Email.CONTENT_ITEM_TYPE)
                && mCascadingData.contains(Phone.CONTENT_ITEM_TYPE)) {
            setEmptyText(R.string.noContactwithPhoneAndEmail);
        }
    }

    public void pickAllInOneData(Uri uri) {
        mListener.onPickAllInOneDataAction(uri);
    }

    public void onShortcutIntentCreated(Uri uri, Intent shortcutIntent) {
        mListener.onShortcutIntentCreated(shortcutIntent);
    }

    @Override
    public void onPickerResult(Intent data) {
        mListener.onPickAllInOneDataAction(data.getData());
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

    public void setPhotoPosition(ContactListItemView.PhotoPosition photoPosition) {
        mPhotoPosition = photoPosition;

        final AllInOneDataListAdapter adapter = (AllInOneDataListAdapter) getAdapter();
        if (adapter != null) {
            adapter.setPhotoPosition(photoPosition);
        }
    }

    @Override
    public boolean onItemLongClick(AdapterView<?> parent, View view,
            int position, long id) {
        hideSoftKeyboard();

        int adjPosition = position - mListView.getHeaderViewsCount();
        if (adjPosition >= 0) {
            onItemLongClick(adjPosition, id);
        }

        return false;
    }

    private void hideSoftKeyboard() {
        // Hide soft keyboard, if visible
        InputMethodManager inputMethodManager = (InputMethodManager)
                mContext.getSystemService(Context.INPUT_METHOD_SERVICE);
        inputMethodManager.hideSoftInputFromWindow(mListView.getWindowToken(), 0);
    }

 /*   private void onItemLongClick(int position, long id) {
        final Uri contactUri;

        AllInOneDataListAdapter adapter = (AllInOneDataListAdapter) getAdapter();
        contactUri = adapter.getContactUri(position);

        if (contactUri != null) {
            final Intent viewContactIntent = new Intent(Intent.ACTION_VIEW, contactUri);
            startActivity(viewContactIntent);
        } else {
            Log.w(TAG, "Item at " + position
                    + " was long clicked before adapter is ready. Ignoring");
        }
    }*/ //sprdPorting

    @Override
    public void onMultiPickerSelected() {
        HashMap<String, String> ret = new HashMap<String, String>();
        HashMap<String, String> temp = new HashMap<String, String>();
        ContactEntryListAdapter adapter = getAdapter();
        Set<Long> checkedItems = adapter.getCurrentCheckedItems();
        for (Long key : checkedItems) {
            temp = getContactCache().getMultiCache().get(key);
            if (temp != null) {
                ret.putAll(temp);
            }
        }
        // SPRD:Bug519952 "Done" button is grey while some contacts is chosen
        // if (ret.size() == 0) {
        // oast.makeText(getActivity(), R.string.toast_no_contact_selected,
        // Toast.LENGTH_SHORT).show();
        // }
        mMultiPickerListener.onPickAllInOneDataAction(ret);
    }

    // add for SPRD:Bug 547154 more than designated contacs can be selected while merge calls during
    // contacts inviting process
    @Override
    public void onResume() {
        super.onResume();
        if (getActivity().getIntent() != null) {
            mCheckedLimitCount = getActivity().getIntent().getIntExtra(
                    "checked_limit_count", -1);
        }
    }

    @Override
    public int getListFilterId() {
        return R.string.list_filter_all_in_one;
    }

    public void setCascadingData(List<String> data) {
        mCascadingData = data;
    }

    public void clearCheckedItem() {
        clearAllCheckItems();
        ContactEntryListAdapter adapter = getAdapter();
        adapter.checkAll(false, 0);
        clearToggleRevert();
    }
}
