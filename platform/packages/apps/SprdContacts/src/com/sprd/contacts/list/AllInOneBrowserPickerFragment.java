
package com.sprd.contacts.list;

import android.database.Cursor;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.util.Log;
import android.widget.Toast;

import com.android.contacts.R;
import com.android.contacts.common.list.ContactEntryListAdapter;
import com.sprd.contacts.common.util.MultiContactDataCacheUtils;
import com.sprd.contacts.common.util.MultiContactDataCacheUtils.CacheMode;

import java.util.HashMap;
import java.util.Set;

public class AllInOneBrowserPickerFragment extends AllInOneDataPickerFragment {

    private OnAllInOneDataMultiPickerActionListener mMultiPickerListener;

    public AllInOneBrowserPickerFragment() {
        super();
        setContactCacheModel(CacheMode.MULTI, Data.DATA1, Phone.DISPLAY_NAME_PRIMARY);
    }

    @Override
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
        for (Long i : checkedItems) {
            temp = getContactCache().getMultiCache().get(i);
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

    // add for Bug564316 getCheckedItemNum
    public int getCheckedItemNum() {
        ContactEntryListAdapter adapter = getAdapter();
        int num = (adapter == null ? 0 : adapter.getCurrentCheckedItems().size());
        return num;
    }
}
