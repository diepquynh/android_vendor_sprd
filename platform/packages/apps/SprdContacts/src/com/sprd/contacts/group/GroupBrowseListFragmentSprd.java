/**
 *
 */
/**
 * @author spreadtrum.com
 *
 */

package com.sprd.contacts.group;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import com.android.contacts.R;
import com.android.contacts.ContactsApplication;
import com.android.contacts.ContactSaveService;
import com.google.common.collect.Lists;
import android.app.Activity;
import android.app.LoaderManager;
import android.app.LoaderManager.LoaderCallbacks;
import android.content.ContentUris;
import android.content.Context;
import android.content.CursorLoader;
import android.content.Intent;
import android.content.Loader;
import android.database.Cursor;
import android.database.MergeCursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Directory;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.CommonDataKinds.GroupMembership;
import android.provider.ContactsContract.Groups;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnCreateContextMenuListener;
import android.widget.AdapterView;
import android.widget.Toast;
import android.widget.AdapterView.AdapterContextMenuInfo;

import com.android.contacts.GroupListLoader;
import com.android.contacts.activities.GroupEditorActivity;
import com.android.contacts.common.list.AutoScrollListView;
import com.android.contacts.group.GroupBrowseListFragment;
import com.android.contacts.group.GroupBrowseListFragment.OnGroupBrowserActionListener;
import com.android.contacts.group.GroupListItem;
import com.android.contacts.interactions.GroupDeletionDialogFragment;
import android.os.Handler;
import android.os.Message;

/*
 * @}
 */
public class GroupBrowseListFragmentSprd extends GroupBrowseListFragment implements
        OnGroupBrowserActionListener {

    private static final int LOADER_GROUPS = 1;
    private static final int LOADER_GROUP_PHOTO = 2;
    private static final int MENU_EDIT_GROUP = 3;
    private static final int MENU_DELETE_GROUP = 4;

    private boolean mContextMenuEnabled = true;
    private boolean mSelectionVisible;

    private static final String KEY_SELECTION_VISIBLE = "selection_visible";
    private static final String EXTRA_KEY_CONTEXT_MENU_ENABLED = "menu_enabled";
    public static final String CURSOR_KEY_COUNT = "mergecursor_count";

    private Context mContext;
    private HashMap<Long, ArrayList<Uri>> mHashMap;
    private static ArrayList<Long> mGroupIdList;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        if (savedInstanceState != null) {
            mContextMenuEnabled = savedInstanceState.getBoolean(EXTRA_KEY_CONTEXT_MENU_ENABLED,
                    true);
            mSelectionVisible = savedInstanceState.getBoolean(KEY_SELECTION_VISIBLE);
            if (mSelectionVisible) {
                setSelectionVisible(mSelectionVisible);
            }
        }
        setLayout(R.layout.group_browse_list_fragment_overlay);

        mHashMap = mHashMap == null ? new HashMap<Long, ArrayList<Uri>>() : mHashMap;
        mGroupIdList = mGroupIdList == null ? new ArrayList<Long>() : mGroupIdList;

        return super.onCreateView(inflater, container, savedInstanceState);
    }

    @Override
    public void onViewGroupAction(Uri groupUri) {

    }

    final OnCreateContextMenuListener mMsgListMenuCreateListener = new OnCreateContextMenuListener() {
        @Override
        public void onCreateContextMenu(ContextMenu menu, View view,
                ContextMenuInfo menuInfo) {

            AdapterView.AdapterContextMenuInfo info = (AdapterContextMenuInfo) menuInfo;
            GroupListItem groupListItem = getAdapter().getItem(info.position);
            GroupListMenuClickListener l = new GroupListMenuClickListener(
                    groupListItem, mContext);
            menu.setHeaderTitle(groupListItem.getTitle());
            menu.add(0, MENU_EDIT_GROUP, 0, R.string.editGroup)
                    .setOnMenuItemClickListener(l);
            menu.add(0, MENU_DELETE_GROUP, 0,
                    R.string.delete_group_dialog_title)
                    .setOnMenuItemClickListener(l);
        }
    };

    @Override
    public void onStart() {
        if (mContextMenuEnabled) {
            getListView().setOnCreateContextMenuListener(mMsgListMenuCreateListener);
        }

        getLoaderManager().initLoader(LOADER_GROUP_PHOTO,
                null, mGroupListLoaderListener);

        super.onStart();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mHashMap != null) {
            mHashMap.clear();
            mHashMap = null;
        }
    }

    public void setSelectionVisible(boolean flag) {
        super.setSelectionVisible(flag);
        mSelectionVisible = flag;
    }

    public HashMap<Long, ArrayList<Uri>> getGroupPhotoUri() {
        return mHashMap;
    }

    public ArrayList<Long> getGroupIdList() {
        return mGroupIdList;
    }

    private final LoaderManager.LoaderCallbacks<Cursor> mGroupListLoaderListener =
            new LoaderCallbacks<Cursor>() {

                @Override
                public CursorLoader onCreateLoader(int id, Bundle args) {
                    return new GroupLoader(mContext, mGroupIdList);
                }

                @Override
                public void onLoadFinished(Loader<Cursor> loader, Cursor data) {

                    mHashMap.clear();
                    if (data instanceof MergeCursor) {
                        MergeCursor mergeCursor = (MergeCursor) data;
                        if (mergeCursor != null) {
                            int counts = mergeCursor.getCount();
                            int count = mergeCursor.getExtras().getInt(CURSOR_KEY_COUNT);
                            if (count != counts && mergeCursor.moveToPosition(count)) {
                                int num = 0;
                                List<Long> finishGroup = new ArrayList<Long>();
                                do {
                                    num++;
                                    String strUri = mergeCursor.getString(0);
                                    long groupId = mergeCursor.getLong(1);
                                    if (strUri != null && !finishGroup.contains(groupId)) {
                                        ArrayList<Uri> list = mHashMap.get(groupId);
                                        if (list == null) {
                                            list = new ArrayList<Uri>();
                                            mHashMap.put(groupId, list);
                                        }
                                        int size = list.size();
                                        if (size < 4) {
                                            list.add(Uri.parse(strUri));
                                            size++;
                                        }
                                        if (size >= 4) {
                                            finishGroup.add(groupId);
                                        }
                                    }
                                } while (mergeCursor.moveToPosition(count + num));
                                finishGroup.clear();
                                mergeCursor.moveToPosition(0);
                            }
                        }
                        setGroupListCursor(mergeCursor);
                    } else {
                        setGroupListCursor(data);
                    }
                    getAdapter().setUriMap(mHashMap);
                    bindGroupList();
                    getAdapter().notifyDataSetChanged();
                }

                public void onLoaderReset(Loader<Cursor> loader) {
                }
            };

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean(EXTRA_KEY_CONTEXT_MENU_ENABLED, mContextMenuEnabled);
        outState.putBoolean(KEY_SELECTION_VISIBLE, mSelectionVisible);
    }

    static class GroupLoader extends CursorLoader {
        final ForceLoadContentObserver mObserver;
        final ArrayList<Long> mInnerGroupIdList;
        final Context mContext;

        private GroupLoader(Context context, ArrayList<Long> groupIdList) {
            super(context);
            mContext = context;
            mObserver = new ForceLoadContentObserver();
            mInnerGroupIdList = groupIdList;
        }

        @Override
        public Cursor loadInBackground() {
            List<Cursor> cursors = Lists.newArrayList();
            MergeCursor mergeCursor;
            final int count;
            Cursor groupCursor = getContext().getContentResolver().query(
                    Groups.CONTENT_SUMMARY_URI,
                    GroupListLoader.COLUMNS,
                    Groups.ACCOUNT_TYPE + " NOT NULL AND "
                            + Groups.ACCOUNT_NAME + " NOT NULL AND " + Groups.AUTO_ADD + "=0 AND " +
                            Groups.FAVORITES + "=0 AND " + Groups.DELETED + "=0",
                    null,
                    Groups.ACCOUNT_TYPE + ", " + Groups.ACCOUNT_NAME + ", " + Groups.DATA_SET
                            + ", " +
                            Groups.TITLE + " COLLATE LOCALIZED ASC");

            if (groupCursor == null) {
                mergeCursor = null;
                return mergeCursor;
            }
            cursors.add(groupCursor);
            count = groupCursor.getCount();

            StringBuilder selection = new StringBuilder();
            if (groupCursor != null && groupCursor.moveToFirst()) {
                mInnerGroupIdList.clear();
                do {
                    if (!groupCursor.isFirst()) {
                        selection.append(",");
                    }
                    selection.append(groupCursor.getLong(GroupListLoader.GROUP_ID));
                    mInnerGroupIdList.add(groupCursor.getLong(GroupListLoader.GROUP_ID));
                } while (groupCursor.moveToNext());
            } else {
                if (groupCursor != null) {
                    groupCursor.registerContentObserver(mObserver);
                }
                return groupCursor;
            }
            Uri uri = Data.CONTENT_URI
                    .buildUpon()
                    .appendQueryParameter(ContactsContract.DIRECTORY_PARAM_KEY,
                            String.valueOf(Directory.DEFAULT)).build();
            Cursor photoListCursor = getContext().getContentResolver().query(
                    uri,
                    new String[] {
                            Data.PHOTO_URI, GroupMembership.GROUP_ROW_ID
                    },
                    Data.MIMETYPE + "=?" + " AND " + GroupMembership.GROUP_ROW_ID + " in ("
                            + selection.toString() + ")", new String[] {
                        GroupMembership.CONTENT_ITEM_TYPE
                    }, null);

            cursors.add(photoListCursor);
            mergeCursor = new MergeCursor(
                    cursors.toArray(new Cursor[cursors.size()])) {
                @Override
                public Bundle getExtras() {
                    Bundle bundle = new Bundle();
                    bundle.putInt(CURSOR_KEY_COUNT, count);
                    return bundle;
                }
            };
            if (mergeCursor != null) {
                mergeCursor.registerContentObserver(mObserver);
            }
            return mergeCursor;
        }
    }

    private final class GroupListMenuClickListener implements
            MenuItem.OnMenuItemClickListener {
        private GroupListItem mGroupListItem;
        private Context mContext;

        public GroupListMenuClickListener(GroupListItem groupItem,
                Context context) {
            mGroupListItem = groupItem;
            mContext = context;
        }

        @Override
        public boolean onMenuItemClick(MenuItem item) {
            if (mGroupListItem == null) {
                return false;
            }
            switch (item.getItemId()) {
                case MENU_EDIT_GROUP: {
                    if (ContactSaveService.mIsGroupSaving
                            || ContactsApplication.sApplication.isBatchOperation()) {
                        Toast.makeText(getActivity(), R.string.toast_batchoperation_is_running,
                                Toast.LENGTH_LONG).show();
                    } else {
                        final Intent intent = new Intent(mContext, GroupEditorActivity.class);
                        intent.setData(getGroupUriFromId(mGroupListItem.getGroupId()));
                        intent.setAction(Intent.ACTION_EDIT);
                        mContext.startActivity(intent);
                    }
                    return true;
                }
                case MENU_DELETE_GROUP: {
                    if (ContactSaveService.mIsGroupSaving
                            || ContactsApplication.sApplication.isBatchOperation()) {
                        Toast.makeText(getActivity(), R.string.toast_batchoperation_is_running,
                                Toast.LENGTH_LONG).show();
                    } else {
                        /**
                         * SPRD:633763 it occured crash low probably when click group menu item
                         * @{
                         */
                        if(getFragmentManager() == null){
                            return true;
                        }
                        /**
                         * @}
                         */
                        GroupDeletionDialogFragment.show(getFragmentManager(),
                                mGroupListItem.getGroupId(), mGroupListItem.getTitle(),
                                false);
                    }
                    return true;
                }
                default:
                    return false;
            }
        }

        private Uri getGroupUriFromId(long groupId) {
            return ContentUris.withAppendedId(Groups.CONTENT_URI, groupId);
        }
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        mContext = activity;
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mContext = null;
    }

    public void setContextMenuEnable(boolean enable) {
        mContextMenuEnabled = enable;
    }
}
