
package com.sprd.contacts.activities;

import java.util.ArrayList;

import com.android.contacts.ContactSaveService;
import com.android.contacts.GroupListLoader;
import com.sprd.contacts.group.GroupDeleteListAdapter;
import com.android.contacts.R;
import com.android.contacts.list.ProviderStatusWatcher;
import com.android.contacts.list.ProviderStatusWatcher.ProviderStatusListener;

import android.app.ActionBar;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.app.ListActivity;
import android.app.LoaderManager;
import android.app.LoaderManager.LoaderCallbacks;
import android.content.Context;
import android.content.Loader;
import android.database.Cursor;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.provider.ContactsContract.ProviderStatus;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.content.CursorLoader;

import java.util.Set;

public class DeleteGroupActivity extends ListActivity implements
        OnItemClickListener, OnClickListener, ProviderStatusListener {

    private static final String TAG = DeleteGroupActivity.class.getSimpleName();

    public static final String GROUP_PHOTO_URI = "group_photo_uri";
    private static final String KEY_CHECKED_ITEMS = "key_checked_items";

    private ProviderStatusWatcher mProviderStatusWatcher;
    private int mProviderStatus;
    private GroupDeleteListAdapter mAdapter;
    private Button mBtnOk;
    private Button mBtnAll;
    private boolean mIsSelectAll = false;
    private ListView mListView;
    final private int LOADER_GROUPS = 1;
    private Cursor mGroupListCursor;
    private Context mContext;
    private Uri mSelectedGroupUri;
    private LinearLayout mFooter;
    private TextView mSelectAllGroup;
    private CheckBox mSelectAllGroupCb;
    private Button mDoneMenuItem;
    private RelativeLayout mSelectGroups;
    private Bundle mPhotoUriBundle;
    private TextView mEmptyView;

    @Override
    public void onCreate(Bundle savedState) {
        super.onCreate(savedState);
        mContext = this;
        mProviderStatusWatcher = ProviderStatusWatcher.getInstance(this);
        mProviderStatusWatcher.addListener(this);
        mProviderStatusWatcher.start();
        // TODO: Create Intent Resolver to handle the different ways users can
        // get to this list.
        // TODO: Handle search or key down
        /**
         * SPRD: Bug 379412 Android origin:setContentView(R.layout.group_delete_activity);
         *
         * @{
         */
        setContentView(R.layout.group_delete_activity_overlay);
        /**
         * @}
         */
        mSelectGroups = (RelativeLayout) findViewById(R.id.select_group);
        mSelectAllGroup = (TextView) findViewById(R.id.selece_all_group);
        mSelectAllGroupCb = (CheckBox) findViewById(R.id.select_group_cb);
        mSelectAllGroupCb.setOnClickListener(this);
        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            actionBar.setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP
                    | ActionBar.DISPLAY_SHOW_TITLE);
            actionBar.setTitle(R.string.menu_delete_group);
            LayoutInflater layoutInflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            View customActionBarView = layoutInflater.inflate(
                    R.layout.editor_custom_action_bar_overlay, null);
            mDoneMenuItem = (Button) customActionBarView
                    .findViewById(R.id.save_menu_item_button);
            mDoneMenuItem.setEnabled(false);
            /**
             * SPRD:Bug 379412 Android origin:mDoneMenuItem.setTextColor(Color.GRAY);
             *
             * @{
             */
            mDoneMenuItem.setTextColor(getResources().getColor(
                    R.color.action_bar_button_disable_text_color));
            /**
             * @}
             */
            mDoneMenuItem.setText(R.string.menu_done);
            mDoneMenuItem.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    GroupDeleteDialogFragment dialog = new GroupDeleteDialogFragment();
                    Bundle args = new Bundle();
                    long[] selectGroups = setToLongArray(mAdapter.getAllCheckedItems());
                    args.putLongArray("selectedGroup", selectGroups);
                    dialog.setArguments(args);
                    dialog.show(getFragmentManager(), null);
                }
            });
            Button cancelMenuItem = (Button) customActionBarView
                    .findViewById(R.id.cancel_menu_item_button);
            cancelMenuItem.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    DeleteGroupActivity.this.finish();
                }
            });
            actionBar.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM
                    | ActionBar.DISPLAY_SHOW_TITLE
                    | ActionBar.DISPLAY_HOME_AS_UP);
            actionBar.setCustomView(customActionBarView,
                    new ActionBar.LayoutParams(
                            ActionBar.LayoutParams.WRAP_CONTENT,
                            ActionBar.LayoutParams.WRAP_CONTENT,
                            Gravity.CENTER_VERTICAL | Gravity.END));

        }

        getLoaderManager()
                .initLoader(LOADER_GROUPS, null, mGroupLoaderListener);
        mPhotoUriBundle = getIntent().getBundleExtra(GROUP_PHOTO_URI);
        mAdapter = new GroupDeleteListAdapter(mContext);
        mAdapter.setPhotoUri(mPhotoUriBundle);
        mListView = getListView();
        mListView.setItemsCanFocus(false);
        mListView.setOnItemClickListener(this);
        setListAdapter(mAdapter);
        if (savedState != null) {
            mAdapter.setAllCheckedItems(savedState.getLongArray(KEY_CHECKED_ITEMS));
            //mProviderStatus = mProviderStatusWatcher.getProviderStatus().status;
            mProviderStatus = mProviderStatusWatcher.getProviderStatus();
            configUnavailableView(ProviderStatus.STATUS_NORMAL != mProviderStatus);
        }
    }

    @Override
    protected void onDestroy() {
        mProviderStatusWatcher.stop();
        mProviderStatusWatcher.removeListener(this);
        super.onDestroy();
    }

    private final LoaderManager.LoaderCallbacks<Cursor> mGroupLoaderListener = new LoaderCallbacks<Cursor>() {
        public CursorLoader onCreateLoader(int id, Bundle args) {
            return new GroupListLoader(mContext, true);
        }

        public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
            mGroupListCursor = data;
            bindGroupList();
            /**
             * SPRD: Bug454361 Check status is wrong after pulling out SIM card.
             * @{
             */
            mAdapter.updateChecked();
            /**
             * @}
             */
            resetSelectAll(true);
            refreshDone();
        }

        public void onLoaderReset(Loader<Cursor> loader) {
        }
    };

    private void bindGroupList() {
        /**
         * SPRD: Bug454361 Check status is wrong after pulling out SIM card.
         * @{
         */
        if (mGroupListCursor == null) {
            return;
        } else if ( mGroupListCursor.getCount() == 0) {
            mSelectGroups.setVisibility(View.GONE);
            mEmptyView = (TextView) findViewById(R.id.empty);
            mEmptyView.setText(getString(R.string.noGroups));
            mListView.setEmptyView(mEmptyView);
        }
        /**
         * @}
         */
        mAdapter.setCursor(mGroupListCursor);
    }

    public void onItemClick(AdapterView<?> parent, View view, int position,
            long id) {
        mAdapter.setChecked(position, !mAdapter.isChecked(position));
        resetSelectAll(true);
        refreshDone();
        mAdapter.notifyDataSetChanged();
    }

    public void onClick(View v) {
        // TODO Auto-generated method stub
        View mView;
        if (v == mSelectAllGroupCb) {
            resetSelectAll(false);
            refreshDone();
            mAdapter.notifyDataSetChanged();
        }
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home: {
                onBackPressed();
                return true;
            }
        }
        return false;
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putLongArray(KEY_CHECKED_ITEMS, setToLongArray(mAdapter.getAllCheckedItems()));
    }

    private void refreshDone() {
        if (mDoneMenuItem != null && mAdapter != null) {
            /**
             * SPRD: Bug454361 Check status is wrong after pulling out SIM card.
             * @{
             */
            if (mAdapter.hasCheckedItems() && (mSelectGroups.getVisibility() == View.VISIBLE)) {
            /**
             * @}
             */
                mDoneMenuItem.setEnabled(true);
                mDoneMenuItem.setTextColor(Color.WHITE);
            } else {
                mDoneMenuItem.setEnabled(false);
                /**
                 * SPRD:Bug 379412 Android origin:mDoneMenuItem.setTextColor(Color.GRAY);
                 *
                 * @{
                 */
                mDoneMenuItem.setTextColor(getResources().getColor(
                        R.color.action_bar_button_disable_text_color));
                /**
                 * @}
                 */
            }
        }
    }

    private void resetSelectAll(boolean isFresh) {
        if (isFresh) {
            if (mAdapter != null && mAdapter.isAllCheckedItems()) {
                mSelectAllGroup.setText(R.string.cancel_select_all_contacts);
                mSelectAllGroupCb.setChecked(true);
            } else {
                mSelectAllGroup.setText(R.string.select_all_contacts);
                mSelectAllGroupCb.setChecked(false);
            }
        } else {
            if (mAdapter != null && mAdapter.isAllCheckedItems()) {
                mAdapter.checkAll(false);
                mSelectAllGroup.setText(R.string.select_all_contacts);
                mSelectAllGroupCb.setChecked(false);
            } else {
                mAdapter.checkAll(true);
                mSelectAllGroup.setText(R.string.cancel_select_all_contacts);
                mSelectAllGroupCb.setChecked(true);
            }
        }
    }

    private long[] setToLongArray(Set<Long> set) {
        if (set == null) {
            return null;
        }
        final int arraySize = set.size();
        long[] result = new long[arraySize];
        int index = 0;
        for (Long id : set) {
            if (index >= arraySize) {
                break;
            }
            result[index++] = id.longValue();
        }

        return result;
    }

    @Override
    public void onProviderStatusChange() {
        mProviderStatus = mProviderStatusWatcher.getProviderStatus();
        configUnavailableView(ProviderStatus.STATUS_NORMAL != mProviderStatus);
    }

    private void configUnavailableView(boolean isVisible) {
        if (isVisible) {
            findViewById(R.id.unavailable_view).setVisibility(View.VISIBLE);
            mSelectGroups.setVisibility(View.GONE);
            findViewById(R.id.divider).setVisibility(View.GONE);
            findViewById(R.id.list_container).setVisibility(View.GONE);
            mDoneMenuItem.setEnabled(false);
            /**
             * SPRD:Bug 379412 Android origin:mDoneMenuItem.setTextColor(Color.GRAY);
             *
             * @{
             */
            mDoneMenuItem.setTextColor(getResources().getColor(
                    R.color.action_bar_button_disable_text_color));
            /**
             * @}
             */
        } else {
            /**
             * SPRD: Bug454361 Check status is wrong after pulling out SIM card.
             * @{
             */
            if (mAdapter.getCount() != 0) {
                findViewById(R.id.unavailable_view).setVisibility(View.GONE);
                mSelectGroups.setVisibility(View.VISIBLE);
                findViewById(R.id.divider).setVisibility(View.VISIBLE);
                findViewById(R.id.list_container).setVisibility(View.VISIBLE);
                refreshDone();
            }
            /**
             * @}
             */
        }
    }

    public static class GroupDeleteDialogFragment extends DialogFragment {

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            AlertDialog.Builder builder = new AlertDialog.Builder(
                    this.getActivity())
                    .setTitle(R.string.menu_delete_group)
                    .setMessage(R.string.delete_group_dialog_commit)
                    .setNegativeButton(android.R.string.cancel, null)
                    .setPositiveButton(android.R.string.ok,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    final long[] selectedGroups = (long[]) getArguments()
                                            .getLongArray("selectedGroup");
                                    for (int i = 0; i < selectedGroups.length; i++) {
                                        getActivity().startService(
                                                ContactSaveService.createGroupDeletionIntent(
                                                        getActivity(), selectedGroups[i]));
                                    }
                                    getActivity().finish();
                                }
                            });
            return builder.create();
        }
    }
}
