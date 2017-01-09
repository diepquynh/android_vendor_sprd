package com.sprd.contacts.group;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

import com.google.common.base.Objects;
import com.android.contacts.R;
import android.content.ContentUris;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.ContactsContract.Groups;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.TextView;

import com.android.contacts.GroupListLoader;
import com.android.contacts.common.ContactPhotoManager;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.group.GroupListItem;

public class GroupSelectListAdapterSprd extends BaseAdapter {

    private final Context mContext;
    private final LayoutInflater mLayoutInflater;
    private final AccountTypeManager mAccountTypeManager;

    private Cursor mCursor;

    private boolean mSelectionVisible;
    private Uri mSelectedGroupUri;
    private boolean mSelectMode = false;
    private static final int LOADER_METADATA = 0;
    private static final int LOADER_MEMBERS = 1;
    private ArrayList<Long> mOldGroupList = new ArrayList<Long>();
    private Set<Long> mCheckedItems = new HashSet<Long>();
    private ContactPhotoManager mPhotoManager;

    public GroupSelectListAdapterSprd(Context context) {
        mContext = context;
        mLayoutInflater = LayoutInflater.from(context);
        mAccountTypeManager = AccountTypeManager.getInstance(mContext);
    }

    public GroupSelectListAdapterSprd(Context context, ArrayList<Long> groupList) {
        mContext = context;
        mLayoutInflater = LayoutInflater.from(context);
        mAccountTypeManager = AccountTypeManager.getInstance(mContext);
        mOldGroupList = groupList;
    }

    public void setCursor(Cursor cursor) {
        mCursor = cursor;

        // If there's no selected group already and the cursor is valid, then by
        // default, select the
        // first group
        if (mSelectedGroupUri == null && cursor != null
                && cursor.getCount() > 0) {
            GroupListItem firstItem = getItem(0);
            long groupId = (firstItem == null) ? null : firstItem.getGroupId();
            mSelectedGroupUri = getGroupUriFromId(groupId);
        }

        notifyDataSetChanged();
    }

    public int getSelectedGroupPosition() {
        if (mSelectedGroupUri == null || mCursor == null
                || mCursor.getCount() == 0) {
            return -1;
        }

        int index = 0;
        mCursor.moveToPosition(-1);
        while (mCursor.moveToNext()) {
            long groupId = mCursor.getLong(GroupListLoader.GROUP_ID);
            Uri uri = getGroupUriFromId(groupId);
            if (mSelectedGroupUri.equals(uri)) {
                return index;
            }
            index++;
        }
        return -1;
    }

    public void setSelectionVisible(boolean flag) {
        mSelectionVisible = flag;
    }

    public void setSelectedGroup(Uri groupUri) {
        mSelectedGroupUri = groupUri;
    }

    private boolean isSelectedGroup(Uri groupUri) {
        return mSelectedGroupUri != null && mSelectedGroupUri.equals(groupUri);
    }

    public Uri getSelectedGroup() {
        return mSelectedGroupUri;
    }

    @Override
    public int getCount() {
        return mCursor == null ? 0 : mCursor.getCount();
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public GroupListItem getItem(int position) {
        if (mCursor == null || mCursor.isClosed()
                || !mCursor.moveToPosition(position)) {
            return null;
        }
        String accountName = mCursor.getString(GroupListLoader.ACCOUNT_NAME);
        String accountType = mCursor.getString(GroupListLoader.ACCOUNT_TYPE);
        String dataSet = mCursor.getString(GroupListLoader.DATA_SET);
        long groupId = mCursor.getLong(GroupListLoader.GROUP_ID);
        String title = mCursor.getString(GroupListLoader.TITLE);
        int memberCount = mCursor.getInt(GroupListLoader.MEMBER_COUNT);

        // Figure out if this is the first group for this account name / account
        // type pair by
        // checking the previous entry. This is to determine whether or not we
        // need to display an
        // account header in this item.
        int previousIndex = position - 1;
        boolean isFirstGroupInAccount = true;
        if (previousIndex >= 0 && mCursor.moveToPosition(previousIndex)) {
            String previousGroupAccountName = mCursor
                    .getString(GroupListLoader.ACCOUNT_NAME);
            String previousGroupAccountType = mCursor
                    .getString(GroupListLoader.ACCOUNT_TYPE);
            String previousGroupDataSet = mCursor
                    .getString(GroupListLoader.DATA_SET);

            if (accountName.equals(previousGroupAccountName)
                    && accountType.equals(previousGroupAccountType)
                    && Objects.equal(dataSet, previousGroupDataSet)) {
                isFirstGroupInAccount = false;
            }
        }

        return new GroupListItem(accountName, accountType, dataSet, groupId,
                title, isFirstGroupInAccount, memberCount);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        final GroupListItem entry = getItem(position);
        View result;
        final GroupListItemViewCache viewCache;
        boolean isInOldGroups = false;
        if (convertView != null) {
            result = convertView;
            viewCache = (GroupListItemViewCache) result.getTag();
        } else {
            result = LayoutInflater.from(mContext).inflate(
                    R.layout.group_select_list_item_new_ui, null);
            viewCache = new GroupListItemViewCache(result);
            result.setTag(viewCache);
        }
        // Bind the group data
        final Uri groupUri = getGroupUriFromId(entry.getGroupId());
        String memberCountString = mContext.getResources().getQuantityString(
                R.plurals.group_list_num_contacts_in_group,
                entry.getMemberCount(), entry.getMemberCount());
        viewCache.setUri(groupUri);
        viewCache.groupTitle.setText(entry.getTitle());
        viewCache.groupMemberCount.setText(memberCountString);
        viewCache.select.setChecked(mCheckedItems.contains(getItem(position)
                .getGroupId()));

        if (mSelectionVisible) {
            result.setActivated(isSelectedGroup(groupUri));
        }

        return result;
    }

    private static Uri getGroupUriFromId(long groupId) {
        return ContentUris.withAppendedId(Groups.CONTENT_URI, groupId);
    }

    public boolean isChecked(int position) {
        return mCheckedItems.contains(getItem(position).getGroupId());
    }

    public void setChecked(int position, boolean checked) {
        long groupId = getItem(position).getGroupId();
        if (checked) {
            mCheckedItems.add(groupId);
        } else {
            mCheckedItems.remove(groupId);
        }
    }

    public void setChecked(ArrayList<Long> oldGroupList, boolean checked) {
        if (checked) {
            for (int i = 0; i < oldGroupList.size(); i++) {
                mCheckedItems.add(oldGroupList.get(i));
            }
        }
    }

    public void checkAll(boolean checked) {
        if (checked) {
            int count = getCount();
            for (int i = 0; i < count; ++i) {
                long groupId = getItem(i).getGroupId();
                mCheckedItems.add(groupId);
            }
        } else {
            mCheckedItems.clear();
        }
    }

    /**
     * Cache of the children views of a contact detail entry represented by a {@link GroupListItem}
     */
    public static class GroupListItemViewCache {
        public final TextView groupTitle;
        public final TextView groupMemberCount;
        public final CheckBox select;
        private Uri mUri;

        public GroupListItemViewCache(View view) {
            groupTitle = (TextView) view.findViewById(R.id.label);
            groupMemberCount = (TextView) view.findViewById(R.id.count);
            select = (CheckBox) view.findViewById(R.id.select_checkbox);
        }

        public void setUri(Uri uri) {
            mUri = uri;
        }

        public Uri getUri() {
            return mUri;
        }
    }

}