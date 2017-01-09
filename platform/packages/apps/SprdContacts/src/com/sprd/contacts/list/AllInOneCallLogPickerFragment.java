
package com.sprd.contacts.list;

import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.CallLog.Calls;
import android.util.Log;
import android.widget.Toast;
import com.android.contacts.R;
import com.android.contacts.common.list.ContactEntryListAdapter;
import com.android.contacts.common.list.DirectoryListLoader;
import com.sprd.contacts.common.util.MultiContactDataCacheUtils;
import com.sprd.contacts.common.util.MultiContactDataCacheUtils.CacheMode;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Set;
import com.android.contacts.common.list.ContactEntryListAdapter.IOnListCheckedChangeListener;
import android.view.LayoutInflater;
import android.view.ViewGroup;


public class AllInOneCallLogPickerFragment extends AllInOneDataPickerFragment {
    private static final String TAG = AllInOneCallLogPickerFragment.class.getSimpleName();
    private OnAllInOneDataMultiPickerActionListener mMultiPickerListener;

    public AllInOneCallLogPickerFragment() {
        super();
        setSectionHeaderDisplayEnabled(false);
        setContactCacheModel(CacheMode.MULTI, Calls.NUMBER, Calls.CACHED_NAME);
    }

    @Override
    protected void onCreateView(LayoutInflater inflater, ViewGroup container) {
        super.onCreateView(inflater, container);
        setContactCacheModel(CacheMode.MULTI, Calls.NUMBER, Calls.CACHED_NAME);
    }

    @Override
    protected void onItemClick(int position, long id) {
        Uri dataUri = null;

        AllInOneCallLogListAdapter adapter = (AllInOneCallLogListAdapter) getAdapter();
        dataUri = adapter.getDataUri(position);

        if (dataUri != null) {
            pickAllInOneData(dataUri);
        }
    }

    @Override
    protected ContactEntryListAdapter createListAdapter() {
        AllInOneCallLogListAdapter adapter = new AllInOneCallLogListAdapter(getActivity());
        adapter.setDisplayPhotos(true);
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

    public void setOnAllInOneDataMultiPickerActionListener(
            OnAllInOneDataMultiPickerActionListener listener) {
        this.mMultiPickerListener = listener;
    }

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

    public HashMap<String, String> getMultiPickerSelected() {
        HashMap<String, String> ret = new HashMap<String, String>();
        HashMap<String, String> temp = new HashMap<String, String>();
        ContactEntryListAdapter adapter = getAdapter();
        Set<Long> checkedItems = adapter.getCurrentCheckedItems();
        for (Long i : checkedItems) {
            temp = getContactCache().getMultiCache().get(i);
            if (temp != null) {
                ret.putAll(temp);
            }
        }
        return ret;
    }
    @Override
    protected void prepareEmptyView() {
        super.prepareEmptyView();
        setEmptyText(R.string.recentCalls_empty);
    }

    // add for Bug564316 getCheckedItemNum
    public int getCheckedItemNum() {
        ContactEntryListAdapter adapter = getAdapter();
        int num = (adapter == null ? 0 : adapter.getCurrentCheckedItems().size());
        return num;
    }
}
