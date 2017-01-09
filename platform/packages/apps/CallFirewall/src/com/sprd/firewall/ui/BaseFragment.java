
package com.sprd.firewall.ui;

import com.sprd.firewall.db.BlackColumns;
import com.sprd.firewall.model.BlackEntity;
import com.sprd.firewall.model.BlackSmsEntity;
import com.sprd.firewall.ui.CallFireWallActivity.ViewPagerVisibilityListener;
import com.sprd.firewall.util.BlackEntityUtil;
import com.sprd.firewall.util.DateUtil;
import com.sprd.firewall.util.ProgressUtil;
import com.sprd.firewall.util.ProgressUtil.ProgressType;

import com.sprd.firewall.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ListFragment;
import android.app.LoaderManager;
import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Loader;
import android.content.CursorLoader;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.database.ContentObserver;
import android.database.Cursor;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.text.TextUtils;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.View.OnTouchListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CursorAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.CompoundButton.OnCheckedChangeListener;

public abstract class BaseFragment extends ListFragment implements ViewPagerVisibilityListener,
        OnItemClickListener, ProgressUtil.ProcessTask, LoaderManager.LoaderCallbacks<Cursor>{
    private static final String TAG = "BaseFragment";

    public static final int ITEM_ADD = Menu.FIRST;

    public static final int ITEM_DELETE = Menu.FIRST + 1;

    public static final int ITEM_CONFIRM_DELETE = Menu.FIRST + 2;

    public static final int ITEM_CANCEL = Menu.FIRST + 3;

    public static final int NONE_DIALOG = -1;

    public static final int DELETE_ALL_DIALOG = 0;

    public static final int DELETE_DIALOG = 1;

    public static final int DETAIL_DIALOG = 2;

    public static final String DIALOG_TYPE = "dialog_type";

    boolean mDeleteState = false;

    CheckBox mSelectAll;

    int count = 0;

    Bundle mMarkForDelete = new Bundle();

    boolean mMonitorSelectAll = false;

    int mCurrentCursorCount = 0;

    CursorAdapter mAdapter;

    AlertDialog.Builder mDeleteAllBuilder;

    AlertDialog.Builder mDeleteBuilder;

    AlertDialog.Builder mDetailBuilder;

    int mDialogType = NONE_DIALOG;

    Context mContext;

    MenuItem mMenuDelete;

    MenuItem mMenuDetail;

    View mSelectAllLinearLayout;

    View mDivider;

    TextView mSelectAllTextView;

    MenuItem mDeleteMenuItem;

    MenuItem mDeleteConfirmMenuItem;

    MenuItem mCancelMenuItem;

    ProgressUtil mProgressUtil;

    /**
     * Used with LoaderManager.
     */
    public static final int LOADER_ID_BLACK_NUMBER = 1;
    public static final int LOADER_ID_CALL_LOG = 2;
    public static final int LOADER_ID_SMS_LOG = 3;
    int mLoaderId;
    // SPRD: modify for bug620809
    int mMarkForDeleteCount = 0;

    @Override
    public void onVisibilityChanged(boolean visible) {
        Activity activity = getActivity();
        if (activity != null) {
            activity.invalidateOptionsMenu();
            mContext = activity;
            drawList();
        }
    }

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setRetainInstance(true);
        mContext = this.getActivity();
        setHasOptionsMenu(true);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putInt(BlackFragment.DIALOG_TYPE, mDialogType);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        getListView().setItemsCanFocus(true);
        getListView().setOnCreateContextMenuListener(this);
        getListView().setOnItemClickListener(this);

        mSelectAll.setOnTouchListener(new OnTouchListener() {

            public boolean onTouch(View v, MotionEvent event) {
                mMonitorSelectAll = true;
                return false;
            }

        });
        setOnSelectAllCheckedChangeListener(mSelectAll);
        drawList();
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        Log.i(TAG, "onActivityCreated");
        super.onActivityCreated(savedInstanceState);
        mContext = this.getActivity();
        if (savedInstanceState != null) {
            showDialogByType(savedInstanceState.getInt(DIALOG_TYPE, NONE_DIALOG));
        }
    }

    @Override
    public void onStart() {
        Log.i(TAG, "onStart");
        super.onStart();
        if (mProgressUtil != null && mProgressUtil.ismProgressRun()) {
            mProgressUtil.initProgressDialog(ProgressType.NO_TYPE);
        }
    }

    public void onResume() {
        if (getLoaderManager().getLoader(mLoaderId) == null) {
            getLoaderManager().initLoader(mLoaderId, null,
                    this);
        }

        drawList();
        super.onResume();
    }

    @Override
    public void onStop() {
        super.onStop();
        if (mProgressUtil != null && mProgressUtil.ismProgressRun()) {
            mProgressUtil.disMissProgressDailog();
         } else {
            mProgressUtil = null;
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        if (getLoaderManager().getLoader(mLoaderId) != null) {
            getLoaderManager().destroyLoader(mLoaderId);
        }
        if (mAdapter != null && mAdapter.getCursor() != null) {
            mAdapter.changeCursor(null);
            mAdapter = null;
        }
    }

    void drawList() {
        if (mAdapter == null) {
            mAdapter = createAdapter(mContext);
            setListAdapter(mAdapter);
        } else if (getActivity() != null
                && getLoaderManager().getLoader(mLoaderId) != null) {
            getLoaderManager().getLoader(mLoaderId).forceLoad();
        }
        if (mSelectAllLinearLayout != null) {
            // SPRD: modify for bug596773
            if (mDeleteState) {
                mSelectAllLinearLayout.setVisibility(View.VISIBLE);
            } else {
                mSelectAllLinearLayout.setVisibility(View.GONE);
            }
            if (mDivider != null) {
                if (mDeleteState) {
                    mDivider.setVisibility(View.VISIBLE);
                } else {
                    mDivider.setVisibility(View.GONE);
                }
            }
            reSetSelectAllState();
        }
    }

    void reSetSelectAllState() {
        if (mCurrentCursorCount != 0 && mCurrentCursorCount == mMarkForDelete.size()) {
            mSelectAll.setChecked(true);
        } else {
            mSelectAll.setChecked(false);
        }
    }

    public abstract class CallFireWallListAdapter extends CursorAdapter {

        LayoutInflater mInflater;
        int mResourceId;

        public CallFireWallListAdapter(Context context, Cursor c, int resource) {
            super(context, c, 0);
            mInflater = LayoutInflater.from(context); // initialize mInflater
            mResourceId = resource;
        }

        @Override
        public void bindView(View view, Context context, Cursor cursor) {
            ViewHolder holder;
            holder = new ViewHolder();

            String mId = setViewHolder(holder, view, cursor);
            if (mMarkForDelete.containsKey(mId)) {
                holder.select.setChecked(true);
            } else {
                holder.select.setChecked(false);
            }

            view.setTag(holder);
        }

        @Override
        public View newView(Context context, Cursor cursor, ViewGroup parent) {
            View convertView;
            convertView = mInflater.inflate(mResourceId, null);
            return convertView;
        }

        public abstract String setViewHolder(ViewHolder holder, View view, Cursor cursor);

        class ViewHolder {
            CheckBox select;
            TextView phone_number;
            ImageView block_type;
            TextView date_time;
            TextView sms_contact;
            TextView name;
            ImageView block_sms;
            ImageView block_call;
            View divider;
        }
    }

    abstract void setMenuItemState();

    abstract void showDialogByType(int type);

    abstract BlackEntity getBlackEntity();

    @Override
    public void onItemClick(AdapterView<?> adapterView, View view, int position, long itemId) {
        if (mDeleteState == true) {
            Cursor item = (Cursor) adapterView.getItemAtPosition(position);
            BlackEntity blackEntity = getBlackEntity();
            BlackEntityUtil.transform(blackEntity, item);
            String mId = blackEntity.getId() + "";
            if (mMarkForDelete.containsKey(mId)) {
                mMarkForDelete.remove(mId);
                mSelectAll.setChecked(false);
            } else {
                mMarkForDelete.putBoolean(mId, true);
                if (mMarkForDelete.size() == mCurrentCursorCount) {
                    mSelectAll.setChecked(true);
                }
            }
            mDeleteState = true;
            if (mMarkForDelete.size() > 0) {
                setDoneMenu(true);
            } else {
                setDoneMenu(false);
            }
            drawList();
        }
    }

    abstract Cursor getBlackListCursor();

    public void resetTabSwitchState(boolean needDrawList) {
        mDeleteState = false;
        mMarkForDelete.clear();
        setMenuItemState();
        if (needDrawList) {
            drawList();
        }
    }

    public boolean getDeleteState() {
        return mDeleteState;
    }

    public void setDoneMenu(boolean enabled) {
        Log.i(TAG, "setDoneMenu enabled:" + enabled);
        if (mDeleteConfirmMenuItem == null) {
            Log.i(TAG, "setDoneMenu mDeleteConfirmMenuItem:null");
            return;
        }
        if (enabled) {
            mDeleteConfirmMenuItem.setEnabled(true);
        } else {
            mDeleteConfirmMenuItem.setEnabled(false);
        }
    }

    @Override
    public abstract void doInBack(ProgressType type);

    @Override
    public void doResult() {
        Activity activity = getActivity();
        if (activity != null) {
            activity.invalidateOptionsMenu();
            // SPRD: add for bug 600800
            drawList();
        }
    }

    @Override
    public abstract Loader<Cursor> onCreateLoader(int arg0, Bundle arg1);

    @Override
    public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
        if (data != null) {
            mCurrentCursorCount = data.getCount();
        }
        mAdapter.changeCursor(data);
    }

    @Override
    public void onLoaderReset(Loader<Cursor> loader) {
        mCurrentCursorCount = 0;
        mAdapter.changeCursor(null);
    }

    public abstract void setOnSelectAllCheckedChangeListener(CheckBox selectAll);

    public abstract CursorAdapter createAdapter(Context context);

    void log(String tag, String msg) {
        Log.d(tag, msg);
    }
    /* SPRD: add for bug600800 @{ */
    public void refreshDialog(AlertDialog dialog) {
        if (dialog != null && dialog.isShowing()) {
            dialog.dismiss();
            mDeleteState = false;
            mMarkForDelete.clear();
            if (getActivity() != null) {
                getActivity().invalidateOptionsMenu();
            }
            if (mSelectAllLinearLayout != null ) {
                mSelectAllLinearLayout.setVisibility(View.GONE);
            }
        }
    }
    /* @} */
}
