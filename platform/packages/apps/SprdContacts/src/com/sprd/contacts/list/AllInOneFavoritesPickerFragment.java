
package com.sprd.contacts.list;

import android.os.Bundle;

import com.android.contacts.R;
import com.android.contacts.common.list.ContactEntryListAdapter;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;

import com.android.contacts.common.list.ContactEntryListAdapter.IOnListCheckedChangeListener;
import com.sprd.contacts.common.util.MultiContactDataCacheUtils.CacheMode;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.CommonDataKinds.Phone;



public class AllInOneFavoritesPickerFragment extends AllInOneDataPickerFragment {
    private static final String TAG = AllInOneFavoritesPickerFragment.class.getSimpleName();

    private static final String KEY_DATA_SELECTION = "data_selection";
    private static final String KEY_CASCADING_DATA = "cascadingData";

    private static final String EMIAL_TYPE_DATA = "vnd.android.cursor.item/email_v2";

    private String mDataSelection;
    private String mMimeType;
    private List<String> mCascadingData;

    private int mMimeTypeCnt;

    @Override
    public void restoreSavedState(Bundle savedState) {
        super.restoreSavedState(savedState);

        if (savedState == null) {
            return;
        }
        mCascadingData = savedState.getStringArrayList(KEY_CASCADING_DATA);
        mDataSelection = savedState.getString(KEY_DATA_SELECTION);
    }

    // add for SPRD:Bug554884 set MultiContactDataCacheUtils state
    @Override
    protected void onCreateView(LayoutInflater inflater, ViewGroup container) {
        super.onCreateView(inflater, container);
        setContactCacheModel(CacheMode.MULTI, Data.DATA1, Phone.DISPLAY_NAME_PRIMARY);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putStringArrayList(KEY_CASCADING_DATA, new ArrayList<String>(mCascadingData));
        if (mDataSelection != null) {
            outState.putString(KEY_DATA_SELECTION, mDataSelection);
        }
    }

    @Override
    protected ContactEntryListAdapter createListAdapter() {
        AllInOneDataListAdapter adapter = new AllInOneDataListAdapter(getActivity());
        adapter.setDisplayPhotos(true);
        adapter.setCascadingData(mCascadingData);
        if (mDataSelection != null) {
            adapter.setLoaderSelection(mDataSelection);
        }
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
    protected void prepareEmptyView() {
        super.prepareEmptyView();
        if ((EMIAL_TYPE_DATA.equals(mMimeType)) && (mMimeTypeCnt == 1)) {
            setEmptyText(R.string.noContactwithEmail);
        } else {
            /* SPRD: Bug 568282 The DUT shows wrong notice massage.*/
            setEmptyText(R.string.listTotalPhoneContactsZero);

        }
    }

    public void setCascadingData(List<String> data) {
        super.setCascadingData(data);
        mCascadingData = data;
        mMimeTypeCnt = mCascadingData.size();
        for (String mimeType : mCascadingData) {
            mMimeType = mimeType;
        }
    }

    public void setSelection(String data) {
        mDataSelection = data;
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
    // add for Bug564316 getCheckedItemNum
    public int getCheckedItemNum() {
        ContactEntryListAdapter adapter = getAdapter();
        int num = (adapter == null ? 0 : adapter.getCurrentCheckedItems().size());
        return num;
    }
}
