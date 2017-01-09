
package com.sprd.contacts.list;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.LoaderManager;
import android.content.CursorLoader;
import android.content.DialogInterface;
import android.view.View.OnClickListener;
import android.content.Intent;
import android.content.Loader;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.app.LoaderManager.LoaderCallbacks;
import android.widget.Toast;
import android.util.Log;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.RawContacts;

import com.android.contacts.activities.ContactSelectionActivity;
import com.android.contacts.common.util.AccountFilterUtil;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.android.contacts.common.util.Constants;
import com.android.contacts.common.list.ContactEntryListAdapter;
import com.android.contacts.common.list.ContactEntryListFragment;
import com.android.contacts.common.list.ContactListAdapter;
import com.android.contacts.common.list.ContactListFilter;
import com.android.contacts.common.list.ContactListFilterController;
import com.android.contacts.common.list.DefaultContactListAdapter;
import com.android.contacts.common.list.DirectoryListLoader;
import com.android.contacts.common.list.ShortcutIntentBuilder;
import com.android.contacts.common.list.ShortcutIntentBuilder.OnShortcutIntentCreatedListener;
import com.android.contacts.ContactsApplication;
import com.android.contacts.ContactSaveService;
import com.android.contacts.group.GroupDetailFragment;
import com.android.contacts.R;

import com.sprd.contacts.common.util.MultiContactDataCacheUtils;
import com.sprd.contacts.common.util.MultiContactDataCacheUtils.CacheMode;
import com.sprd.contacts.group.GroupDetailFragmentSprd;
import com.sprd.contacts.list.OnContactMultiPickerActionListener;

import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map.Entry;
import java.util.Set;
import java.util.Iterator;

public abstract class ContactPickerFragmentSprd extends
        ContactEntryListFragment<ContactEntryListAdapter> {

    private static final int REQUEST_CODE_ACCOUNT_FILTER = 1;
    private static final String KEY_FILTER = "mFilter";
    private static final String KEY_GROUP_SELECTION = "addGroupMemSelection";
    private static final String KEY_STAR_SELECTION = "addStarMemSelection";
    private static final String KEY_LIST_REQUEST_MODE_SELECTION = "listRequestModeSelection";
    private static final String KEY_PERMANENT_FILTER = "permanentAccountFilter";
    private static final String KEY_EXCLUDE_READONLY = "excludereadonly";
    private static final int SHOW_TARGET_FRAGMENT_ID = 0;
    private static final int LOADER_RAW_CONTACT_ID = 1;
    public static final String MMS_MULTI_VCARD = "multiVcard";
    /* SPRD:Bug 406295 @{ */
    private static final String TARGET_GROUP_ID = "targetGroupId";
    /* @} */

    private boolean mLoaderStarted;
    private ContactListFilter mFilter;
    private View mAccountFilterHeader;
    private boolean mPermanentAccountFilter = false;
    private OnContactMultiPickerActionListener mMultiPickerListener;
    private static final String TAG = "ContactPickerFragmentSprd";
    private long[] memberToMove;
    private ContactListFilterController mContactListFilterController;
    private String mAddGroupMemSelection;
    private String mListRequestModeSelection;
    private boolean mIsStarMemFlag = false;
    private boolean mExcludeReadOnly = false;
    private ArrayList<String> mGroupIds;
    private ArrayList<String> mGroupTitles;

    public ContactPickerFragmentSprd() {
        setContactCacheModel(CacheMode.MULTI, Contacts.LOOKUP_KEY, Contacts._ID);
    }

    private class FilterHeaderClickListener implements OnClickListener {
        @Override
        public void onClick(View view) {
            if (!mPermanentAccountFilter) {
                AccountFilterUtil.startAccountFilterActivityForResult(
                        ContactPickerFragmentSprd.this, REQUEST_CODE_ACCOUNT_FILTER, getFilter());

            }
        }
    }

    private OnClickListener mFilterHeaderClickListener = new FilterHeaderClickListener();

    public void setAddGroupMemSelection(String selection) {
        mAddGroupMemSelection = selection;
    }

    public void setListRequestModeSelection(String selection) {
        mListRequestModeSelection = selection;
    }

    public void setStarMemFlag() {
        mIsStarMemFlag = true;
    }

    public void setExcludeReadOnly(boolean excludeReadOnly){
        mExcludeReadOnly = excludeReadOnly;
    }
    @Override
    protected void onCreateView(LayoutInflater inflater, ViewGroup container) {
        super.onCreateView(inflater, container);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putParcelable(KEY_FILTER, mFilter);
        outState.putString(KEY_GROUP_SELECTION, mAddGroupMemSelection);
        outState.putString(KEY_LIST_REQUEST_MODE_SELECTION, mListRequestModeSelection);
        outState.putBoolean(KEY_STAR_SELECTION, mIsStarMemFlag);
        outState.putBoolean(KEY_PERMANENT_FILTER, mPermanentAccountFilter);
        outState.putBoolean(KEY_EXCLUDE_READONLY, mExcludeReadOnly);
    }

    @Override
    public void restoreSavedState(Bundle savedState) {
        super.restoreSavedState(savedState);

        if (savedState == null) {
            return;
        }
        mFilter = (ContactListFilter) savedState.getParcelable(KEY_FILTER);
        mAddGroupMemSelection = savedState.getString(KEY_GROUP_SELECTION);
        mListRequestModeSelection = savedState.getString(KEY_LIST_REQUEST_MODE_SELECTION);
        mIsStarMemFlag = savedState.getBoolean(KEY_STAR_SELECTION);
        mPermanentAccountFilter = savedState.getBoolean(KEY_PERMANENT_FILTER);
        mExcludeReadOnly = savedState.getBoolean(KEY_EXCLUDE_READONLY);
    }

    @Override
    protected void configureAdapter() {
        super.configureAdapter();
        ContactEntryListAdapter adapter = getAdapter();
        if (!isSearchMode() && mFilter != null) {
            adapter.setFilter(mFilter);
        }
        if (adapter instanceof DefaultContactListAdapter) {
            DefaultContactListAdapter defaultAdapter = (DefaultContactListAdapter) adapter;
            defaultAdapter.setAddGroupMemSelection(mAddGroupMemSelection);
            defaultAdapter.setListRequestModeSelection(mListRequestModeSelection);
            if (mIsStarMemFlag) {
                defaultAdapter.setStarMemSelection();
            }
            defaultAdapter.setExcludeReadOnly(mExcludeReadOnly);
        }
    }

    @Override
    protected void prepareEmptyView() {
        super.prepareEmptyView();
        setEmptyText(R.string.listTotalAllContactsZero);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQUEST_CODE_ACCOUNT_FILTER) {
            if (getActivity() != null) {
                final ContactListFilterController controller = ContactListFilterController
                        .getInstance(getActivity());
                AccountFilterUtil.handleAccountFilterResult(controller, resultCode, data);
            }
        }
    }

    public void setFilter(ContactListFilter filter) {
        if (filter == null) {
            return;
        }
        if (mFilter == null || !mFilter.toString().equals(filter.toString())) {
            mFilter = filter;
        }
        if (mLoaderStarted) {
            reloadData();
            updateFilterHeaderView();
        }
    }

    public void setPermanentFilter(ContactListFilter filter) {
        mPermanentAccountFilter = true;
        setFilter(filter);
    }

    public ContactListFilter getFilter() {
        return mFilter;
    }

    @Override
    protected void startLoading() {
        mLoaderStarted = true;
        super.startLoading();
    }

    @Override
    protected void setSearchMode(boolean flag) {
        super.setSearchMode(flag);
        updateFilterHeaderView();
    }

    protected void checkFilter(Activity activity) {
        if (activity != null) {
            mContactListFilterController = ContactListFilterController.getInstance(activity);
            mContactListFilterController.checkFilterValidity(false);
        }
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
    public void onMultiPickerSelected() {
        boolean setStarredFlag = getActivity().getIntent()
                .getBooleanExtra("setMulitStarred", false);
        String setMultiVcard = null;
        if (getActivity().getIntent().getExtras() != null) {
            setMultiVcard = getActivity().getIntent().getExtras().getString(MMS_MULTI_VCARD);
        }
            ArrayList<String> ret = new ArrayList<String>();
            ArrayList<String> ret2 = new ArrayList<String>();
            HashMap<String, String> multiCache = new HashMap<String, String>();
            ContactEntryListAdapter adapter = getAdapter();
            Set<Long> checkedItems = adapter.getCurrentCheckedItems();
            for (Long i : checkedItems) {
                multiCache = getContactCache().getMultiCache().get(i);
                if (multiCache != null && multiCache.size() == 1) {
                    Iterator it = multiCache.entrySet().iterator();
                    if (it.hasNext()) {
                        Entry<String, String> entry = (Entry<String, String>) it.next();
                        String key = entry.getKey();
                        String value = entry.getValue();
                        if (key != null && value != null) {
                            ret.add(key);
                            ret2.add(value);
                        }
                    }
                }
            }
            if (ret.size() == 0) {
                Toast.makeText(getActivity(),
                        R.string.toast_no_contact_selected, Toast.LENGTH_SHORT)
                        .show();
                if (mMultiPickerListener != null) {
                    mMultiPickerListener.onCancel();
                } else {
                    Log.e(TAG, "mMultiPickerListener is null");
                }
            } else {
                if (getActivity().getIntent().hasExtra("move_group_member")
                        || getActivity().getIntent().hasExtra("delete_group_member")) {
                    Bundle args = new Bundle();
                    long[] contactIdsToMove = new long[checkedItems.size()];
                    int i = 0;
                    for (Long item : checkedItems) {
                        contactIdsToMove[i] = Long.parseLong(item.toString());
                        i++;
                    }
                    args.putLongArray("contact_Ids", contactIdsToMove);
                    getLoaderManager().restartLoader(LOADER_RAW_CONTACT_ID, args,
                            mSelectedGroupMemberListener);
                } else {
                    if (mMultiPickerListener != null) {
                        mMultiPickerListener.onPickContactAction(ret, ret2);
                    } else {
                        Log.e(TAG, "mMultiPickerListener is null");
                    }
                }
            }
    }

    private final LoaderManager.LoaderCallbacks<Cursor> mSelectedGroupMemberListener =
            new LoaderCallbacks<Cursor>() {
                ArrayList<Long> rawContactIds = new ArrayList<Long>();

                @Override
                public CursorLoader onCreateLoader(int id, Bundle args) {
                    CursorLoader ret = new CursorLoader(getActivity());
                    if (id == LOADER_RAW_CONTACT_ID) {
                        ret.setUri(RawContacts.CONTENT_URI);
                        ret.setProjection(new String[] {
                                RawContacts._ID, RawContacts.CONTACT_ID
                        });
                        long[] contactIds = args.getLongArray("contact_Ids");
                        StringBuilder sb = new StringBuilder();
                        boolean init = true;
                        for (long contactId : contactIds) {
                            if (!init) {
                                sb.append(",");
                            }
                            init = false;
                            sb.append(contactId);
                        }
                        ret.setSelection(RawContacts.CONTACT_ID + " in (" + sb.toString() + ")");
                    }
                    return ret;
                }

                @Override
                public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
                    int id = loader.getId();
                    if (id == LOADER_RAW_CONTACT_ID) {
                       /**
                        * SPRD: Bug 525629 The previous selected group members are moved to
                        * another group after re-selecting the moved group members.
                        * {@
                        */
                        rawContactIds.clear();
                        /*
                         * }@
                         */
                        if (data != null && data.moveToFirst()) {
                            do {
                                long rawContactId = data.getLong(
                                        data.getColumnIndex(RawContacts._ID));
                                rawContactIds.add(rawContactId);
                            } while (data.moveToNext());
                            if (rawContactIds != null) {
                                memberToMove = new long[rawContactIds.size()];
                                for (int i = 0; i < rawContactIds.size(); i++) {
                                    memberToMove[i] = rawContactIds.get(i);
                                }
                            }
                            getLoaderManager().destroyLoader(LOADER_RAW_CONTACT_ID);
                            if (getActivity().getIntent().hasExtra("move_group_member")) {
                                mGroupIds = (ArrayList<String>) getActivity()
                                        .getIntent().getStringArrayListExtra(
                                                GroupDetailFragmentSprd.TARGET_GROUP_IDS);
                                mGroupTitles = (ArrayList<String>) getActivity()
                                        .getIntent().getStringArrayListExtra(
                                                GroupDetailFragmentSprd.TARGET_GROUP_TITLES);
                                if (mGroupIds != null && mGroupTitles != null) {
                                    Message message = mHandler
                                            .obtainMessage(SHOW_TARGET_FRAGMENT_ID);
                                    mHandler.sendMessage(message);
                                }
                            } else if (getActivity().getIntent().hasExtra("delete_group_member")) {
                                /**
                                 * SPRD: add iLog Original Android code:
                                 *
                                 * @{
                                 *//*
                                if (Log.isIloggable()) {
                                    Log.startPerfTracking(Constants.PERFORMANCE_TAG
                                            + String.format(
                                                    ": Start deleting %d contacts from group",
                                                    memberToMove.length));
                                } //sprd_4.4
                                /**
                                 * @}
                                 */
                                getContext()
                                        .startService(
                                                ContactSaveService
                                                        .createGroupUpdateIntent(
                                                                getContext(),
                                                                (Long) (getActivity().getIntent()
                                                                        .getLongExtra("delete_group_member", -1)),
                                                                null,
                                                                null,
                                                                memberToMove,
                                                                ContactSelectionActivity.class,
                                                                ContactSelectionActivity.MOVE_GROUP_COMPLETE));
                                getActivity().finish();

                            }

                        }
                    }
                }

                @Override
                public void onLoaderReset(Loader<Cursor> loader) {
                }
            };

    public void setOnContactMultiPickerActionListener(OnContactMultiPickerActionListener listener) {
        mMultiPickerListener = listener;
    }

    public static class ShowTargetGroupDialogFragment extends DialogFragment {
        static Long mTargetGroupId;

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            ArrayList<String> groupIds = getArguments().getStringArrayList("groupId");
            ArrayList<String> groupTitles = getArguments().getStringArrayList("groupTitle");
            final long[] member = getArguments().getLongArray("memberToMove");
            final Long[] ids = (Long[]) groupIds.toArray(new Long[groupIds.size()]);
            final String[] titles = (String[]) groupTitles.toArray(new String[groupTitles.size()]);
            if (ids.length > 0) {
                /* SPRD:Bug 406295 * @{ */
                /* mTargetGroupId = Long.valueOf(ids[0]); */
                mTargetGroupId = mTargetGroupId == null ? Long.valueOf(ids[0]) : mTargetGroupId;
                /* @} */
            }

            AlertDialog dialog = new AlertDialog.Builder(getActivity())
                    .setTitle(R.string.moveGroupMemberDialogTitle)
                    .setSingleChoiceItems(titles, 0,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    if (which >= 0) {
                                        mTargetGroupId = ids[which];
                                    }
                                }
                            })
                    .setPositiveButton(android.R.string.ok,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int whichButton) {
                                    if (getActivity() != null) {
                                        Intent serviceIntent = ContactSaveService
                                                .createGroupMoveIntent(
                                                        getActivity(),
                                                        mTargetGroupId,
                                                        (Long) (getActivity()
                                                                .getIntent()
                                                                .getLongExtra(GroupDetailFragmentSprd.SRC_GROUP_ID, -1)),
                                                        null,
                                                        member,
                                                        ContactSelectionActivity.class,
                                                        ContactSelectionActivity.MOVE_GROUP_COMPLETE);
                                        getActivity().startService(serviceIntent);

                                        getActivity().finish();
                                    }
                                }
                            })
                    .setNegativeButton(android.R.string.cancel, null).create();
            return dialog;
        }

        /* SPRD:Bug 406295 Group members have not moved to target group when rotating the screen
         * after change the default group.  * @{ */
        @Override
        public void onSaveInstanceState(Bundle outState) {
            // TODO Auto-generated method stub
            super.onSaveInstanceState(outState);
            outState.putLong(TARGET_GROUP_ID, mTargetGroupId);
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            // TODO Auto-generated method stub
            super.onCreate(savedInstanceState);
            if (savedInstanceState == null) {
                return;
            }
            mTargetGroupId = savedInstanceState.getLong(TARGET_GROUP_ID);
        }

        /**
         * SPRD:Bug442855 Recovery mTargetGroupId on onDestory function to avoid crash when unlock screen.
         * @{
         */
        @Override
        public void onDestroy() {
            // TODO Auto-generated method stub
            super.onDestroy();
            mTargetGroupId = null;
        }
        /**
         * @}
         */
        /* @} */
    }

    protected boolean showBatchToast() {
        if (ContactsApplication.sApplication.isBatchOperation()
                || ContactSaveService.mIsGroupSaving) {
            Toast.makeText(getActivity(), R.string.toast_batchoperation_is_running,
                    Toast.LENGTH_LONG).show();
            return true;
        }
        return false;
    }

    private Handler mHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case SHOW_TARGET_FRAGMENT_ID:
                    ShowTargetGroupDialogFragment showDialogFragment = new ShowTargetGroupDialogFragment();
                    Bundle bundle = new Bundle();
                    bundle.putStringArrayList("groupId", mGroupIds);
                    bundle.putStringArrayList("groupTitle", mGroupTitles);
                    bundle.putLongArray("memberToMove", memberToMove);
                    showDialogFragment.setArguments(bundle);
                    showDialogFragment.show(getFragmentManager(), null);
                    break;

                default:
                    break;
            }
        }
    };
}
