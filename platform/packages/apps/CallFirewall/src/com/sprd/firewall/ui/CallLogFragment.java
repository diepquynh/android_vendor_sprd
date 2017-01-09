
package com.sprd.firewall.ui;

import com.sprd.firewall.db.BlackColumns;
import com.sprd.firewall.model.BlackCallEntity;
import com.sprd.firewall.model.BlackEntity;
import com.sprd.firewall.model.BlackNumberEntity;
import com.sprd.firewall.ui.BlackFragment.BlackListAdapter;
import com.sprd.firewall.ui.CallFireWallActivity.ViewPagerVisibilityListener;
import com.sprd.firewall.util.BlackCallsUtils;
import com.sprd.firewall.util.BlackEntityUtil;
import com.sprd.firewall.util.DateUtil;
import com.sprd.firewall.util.ProgressUtil;
import com.sprd.firewall.util.ProgressUtil.ProgressType;

import com.sprd.firewall.R;
import android.R.integer;

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
import android.provider.BlockedNumberContract;
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
import android.os.Message;

public class CallLogFragment extends BaseFragment{
    private static final String TAG = "CallLogFragment";

    private static final int MENU_DELETE = 0;

    private static final int MENU_DETAIL = 1;

    private BlackCallEntity mCallEntity;
    /* SPRD: modify for bug600800 @{ */
    private AlertDialog mDialog = null;
    private DeleteHandler mDeleteHandler = null;
    private final int DISMISS = 1;

    private class DeleteHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case DISMISS:
                    refreshDialog(mDialog);
                    break;
            }
            super.handleMessage(msg);
        }
    };

    private ContentObserver mObserver = new ContentObserver(mDeleteHandler) {

        @Override
        public void onChange(boolean selfChange) {
            super.onChange(selfChange);
            mDeleteHandler.removeMessages(DISMISS);
            mDeleteHandler.sendEmptyMessage(DISMISS);
            drawList();
        }
    };
    /* @} */

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mLoaderId = LOADER_ID_CALL_LOG;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        final View listLayout = inflater.inflate(R.layout.black_log_list, container, false);
        mSelectAllLinearLayout = listLayout.findViewById(R.id.Log_selecte_all_layout);
        mDivider = listLayout.findViewById(R.id.divider);
        mSelectAll = (CheckBox) listLayout.findViewById(R.id.Log_selete_all);
        mSelectAllTextView = (TextView) listLayout.findViewById(R.id.Log_selete_all_text);

        return listLayout;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        // SPRD: add for bug 600800
        mDeleteHandler = new DeleteHandler();
        mContext.getContentResolver().registerContentObserver(
                BlackColumns.BlockRecorder.CONTENT_URI, true,
                mObserver);
    }

    /* SPRD: add for bug619156 @{ */
    @Override
    public void onResume() {
        Log.i(TAG, "onResume");
        if (mCurrentCursorCount == 0) {
            mDeleteState = false;
            Activity activity = getActivity();
            if (activity != null) {
                activity.invalidateOptionsMenu();
            }
        }
        super.onResume();
    }
    /* @} */

    @Override
    public void onDetach() {
        super.onDetach();
        mContext.getContentResolver().unregisterContentObserver(mObserver);
    }

    private class BlackListAdapter extends BaseFragment.CallFireWallListAdapter {

        private BlackCallEntity member = new BlackCallEntity();

        public BlackListAdapter(Context context, Cursor c) {
            super(context, c, R.layout.black_log_list_item);
        }

        @Override
        public String setViewHolder(
                BaseFragment.CallFireWallListAdapter.ViewHolder holder,
                View view, Cursor cursor) {
            holder.select = (CheckBox) view.findViewById(R.id.log_select);
            holder.select.setFocusable(false);
            holder.select.setFocusableInTouchMode(false);
            holder.select.setClickable(false);

            holder.phone_number = (TextView) view.findViewById(R.id.log_phone_number);
            holder.date_time = (TextView) view.findViewById(R.id.log_date_time);
            holder.name = (TextView) view.findViewById(R.id.log_phone_name);
            if (mDeleteState) {
                holder.select.setVisibility(View.VISIBLE);
            } else {
                holder.select.setVisibility(View.GONE);
            }

            BlackEntityUtil.transform(member, cursor);
            String format_time = DateUtil.formatDate(member.getTime(), mContext);
            holder.phone_number.setText(member.getNumber());

            int calltype = member.getType();

            BlackCallsUtils blackCallsUtils = new BlackCallsUtils(mContext);
            String phoneName = blackCallsUtils.FindPhoneNamebyNumber(member.getNumber());
            Log.d(TAG, "FindPhoneNamebyNumber:phoneName=" + phoneName);
            if (phoneName == null || TextUtils.isEmpty(phoneName)) {
                holder.name.setText(member.getNumber());
            } else {
                holder.name.setText(phoneName);
            }

            holder.date_time.setText(format_time);
            return (member.getId() + "");
        }
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);
        // SPRD: modify for bug609151
        //inflater.inflate(R.menu.call_log_options, menu);
    }

    @Override
    public void onPrepareOptionsMenu(Menu menu) {
        /* SPRD: modify for bug609151 @{ */
        if (menu != null) {
            mDeleteMenuItem = menu.findItem(R.id.call_log_menu_delete);
            mDeleteConfirmMenuItem = menu.findItem(R.id.call_log_menu_delete_confirm);
            mCancelMenuItem = menu.findItem(R.id.call_log_menu_cancel);

            setMenuItemState();
        }
        /* @} */
    }

    @Override
    void setMenuItemState() {
        if (mDeleteMenuItem == null || mDeleteConfirmMenuItem == null || mCancelMenuItem == null) {
            return;
        }
        if (mDeleteState) {
            mDeleteMenuItem.setVisible(false);
            mDeleteConfirmMenuItem.setVisible(true);
            if (mMarkForDelete.size() > 0) {
                setDoneMenu(true);
            }
            mCancelMenuItem.setVisible(true);
        } else {
            if (mDeleteMenuItem != null) {
                mDeleteMenuItem.setVisible(true);
                Cursor cur = getBlackListCursor();
                mDeleteMenuItem.setEnabled(cur != null && cur.getCount() > 0);
                if (cur != null) {
                    cur.close();
                }
            }
            mDeleteConfirmMenuItem.setVisible(false);
            mCancelMenuItem.setVisible(false);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {

            case R.id.call_log_menu_delete:
                mDeleteState = true;
                drawList();
                getActivity().invalidateOptionsMenu();
                break;
            case R.id.call_log_menu_delete_confirm:
                showDialogByType(BlackFragment.DELETE_ALL_DIALOG);
                break;
            case R.id.call_log_menu_cancel:
                mDeleteState = false;
                mMarkForDelete.clear();
                drawList();
                getActivity().invalidateOptionsMenu();
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    void showDialogByType(int type){
        if (type == BlackFragment.NONE_DIALOG) {
            return;
        }
        if (type == BlackFragment.DELETE_ALL_DIALOG) {
            if (mDeleteAllBuilder == null ) {
                mDeleteAllBuilder = new AlertDialog.Builder(mContext);
            }
            mDeleteAllBuilder.setTitle(R.string.CallLog_confirm_delete_title)
                    .setIconAttribute(android.R.attr.alertDialogIcon)
                    .setMessage(R.string.CallLog_confirm_delete)
                    .setPositiveButton(android.R.string.ok, new OnClickListener() {
                        public void onClick(DialogInterface dialog, int whichButton) {
                            mProgressUtil = new ProgressUtil(mContext,
                                    mMarkForDelete.size(),
                                    ProgressType.CALLLOG_DEL,
                                    CallLogFragment.this);
                            mProgressUtil.setMtitleId(R.string.Delete_CallLog_title);
                            mProgressUtil.execute();
                        }
                    }).setNegativeButton(android.R.string.cancel, null).setCancelable(true)
                    .setOnDismissListener(new DialogInterface.OnDismissListener() {
                        public void onDismiss(DialogInterface dialog) {
                            mDialogType = BlackFragment.NONE_DIALOG;
                        }
                    });
            // SPRD: add for bug 600800
            mDialog = mDeleteAllBuilder.create();
            mDialog.show();
        } else if (type == BlackFragment.DELETE_DIALOG) {
            if (mDeleteBuilder == null) {
                mDeleteBuilder = new AlertDialog.Builder(mContext);
            }
            mDeleteBuilder.setTitle(R.string.CallLog_confirm_delete_title)
                    .setIconAttribute(android.R.attr.alertDialogIcon)
                    .setMessage(R.string.CallLog_confirm_delete)
                    .setPositiveButton(android.R.string.ok, new OnClickListener() {
                        public void onClick(DialogInterface dialog, int whichButton) {
                            try {
                                BlackCallsUtils blackCallsUtils = new BlackCallsUtils(mContext);
                                blackCallsUtils.DelNewBlackLogsFromId(mCallEntity.getId());
                                drawList();
                            } catch (Exception e) {
                            }
                            dialog.dismiss();
                        }
                    }).setNegativeButton(android.R.string.cancel, null).setCancelable(true)
                    .setOnDismissListener(new DialogInterface.OnDismissListener() {
                        public void onDismiss(DialogInterface dialog) {
                            mDialogType = BlackFragment.NONE_DIALOG;
                        }
                    });
            // SPRD: add for bug 600800
            mDialog = mDeleteBuilder.create();
            mDialog.show();
        } else if (type == BlackFragment.DETAIL_DIALOG) {
            String phoneName = null;
            BlackCallsUtils blackCallsUtils = new BlackCallsUtils(mContext);
            phoneName = blackCallsUtils.FindPhoneNamebyNumber(mCallEntity.getNumber());
            Log.d(TAG, "onListItemClick:phoneName=" + phoneName);
            if (phoneName == null || TextUtils.isEmpty(phoneName)) {
                phoneName = mCallEntity.getNumber();
            }
            String phoneNumber = mCallEntity.getNumber();
            String blockTime = DateUtil.formatDate(mCallEntity.getTime(), mContext);
            if (mDetailBuilder == null) {
                mDetailBuilder = new AlertDialog.Builder(mContext);
            }
            mDetailBuilder.setTitle(R.string.blackCallsLogIconLabel)
                    .setCancelable(false)
                    .setPositiveButton(android.R.string.ok,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                    mDeleteState = false;
                                }
                            }).setOnKeyListener(new DialogInterface.OnKeyListener() {
                        @Override
                        public boolean onKey(DialogInterface dialog,
                                int keyCode, KeyEvent event) {
                            if (KeyEvent.KEYCODE_BACK == keyCode) {
                                dialog.dismiss();
                            }
                            return false;
                        }
                    }).setOnDismissListener(new DialogInterface.OnDismissListener() {
                        public void onDismiss(DialogInterface dialog) {
                            mDialogType = BlackFragment.NONE_DIALOG;
                        }
                    });
            mDetailBuilder.setMessage(
                    getString(R.string.blackCalls_calllog_detail, phoneName, phoneNumber,
                    blockTime));
            mDetailBuilder.create().show();
        }
        mDialogType = type;
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
        if (!mDeleteState) {
            menu.setHeaderTitle(R.string.BlaclList_conMenu_title);
            mMenuDelete = menu.add(0, MENU_DELETE, 0, R.string.BlaclList_conMenu_delete);
            mMenuDetail = menu.add(0, MENU_DETAIL, 0, R.string.BlackList_conMenu_detail);
        }
    }

    public boolean onContextItemSelected(MenuItem aItem) {
        AdapterView.AdapterContextMenuInfo info;
        try {
            info = (AdapterView.AdapterContextMenuInfo) aItem.getMenuInfo();
        } catch (ClassCastException e) {
            Log.e(TAG, "bad menuInfo", e);
            return false;
        }
        if (info == null) {
            return false;
        }
        Cursor cursor = (Cursor) getListAdapter().getItem(info.position);
        mCallEntity = new BlackCallEntity();
        if (aItem == mMenuDelete) {
            BlackEntityUtil.transform(mCallEntity, cursor);
            showDialogByType(BlackFragment.DELETE_DIALOG);
            return true;
        } else if (aItem == mMenuDetail) {
            BlackEntityUtil.transform(mCallEntity, cursor);
            showDialogByType(BlackFragment.DETAIL_DIALOG);
            return true;
        } else {
            return false;
        }
    }

    @Override
    Cursor getBlackListCursor() {
        ContentResolver cr = mContext.getContentResolver();
        Cursor cur;
        String[] columns = new String[] {
                BlackColumns.BlockRecorder._ID, BlackColumns.BlockRecorder.NUMBER_VALUE,
                BlackColumns.BlockRecorder.CALL_TYPE, BlackColumns.BlockRecorder.BLOCK_DATE,
                BlackColumns.BlockRecorder.NAME
        };
        cur = cr.query(BlackColumns.BlockRecorder.CONTENT_URI, columns, null, null, null);
        if (cur != null) {
            mCurrentCursorCount = cur.getCount();
        }
        return cur;
    }

    @Override
    public void doInBack(ProgressType type) {
        if (mProgressUtil == null) {
            Log.i(TAG, "doInBack-mProgressUtil = null");
            return;
        }

        if (type == ProgressType.CALLLOG_DEL) {
            ContentResolver cr = mContext.getContentResolver();
            String mId;
            Cursor cur = getBlackListCursor();

            int deleteNums = 0;
            try {
                if (cur != null && cur.moveToFirst()) {
                    do {
                        mId = cur.getString(cur.getColumnIndex(BlockedNumberContract.BlockedNumbers.COLUMN_ID));
                        if (mMarkForDelete.containsKey(mId)) {

                            cr.delete(BlackColumns.BlockRecorder.CONTENT_URI,
                                    BlackColumns.BlockRecorder._ID + "='" +
                                            mId + "'", null);
                            deleteNums++;
                            mProgressUtil.UpdateProgress(deleteNums);
                        }
                    } while (cur.moveToNext());
                }
            } catch (Exception e) {
                Log.e(TAG, "deleteSelected()", e);
            } finally {
                if (cur != null) {
                    cur.close();
                }
            }
            mDeleteState = false;
            mMarkForDelete.clear();
        }
    }

    @Override
    public CursorLoader onCreateLoader(int id, Bundle args) {
        String[] columns = new String[] {
                BlackColumns.BlockRecorder._ID, BlackColumns.BlockRecorder.NUMBER_VALUE,
                BlackColumns.BlockRecorder.CALL_TYPE, BlackColumns.BlockRecorder.BLOCK_DATE,
                BlackColumns.BlockRecorder.NAME
        };

        return new CursorLoader(getActivity(), BlackColumns.BlockRecorder.CONTENT_URI, columns,
                null, null, null);
    }

    @Override
    BlackEntity getBlackEntity() {
        return new BlackCallEntity();
    }

    @Override
    public void setOnSelectAllCheckedChangeListener(CheckBox selectAll) {
        selectAll.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    mSelectAllTextView.setText(R.string.blackCalls_all_button_Unselected);
                    if (mMonitorSelectAll) {
                        Cursor cur = getBlackListCursor();
                        try {
                            if (cur != null && cur.moveToFirst()) {
                                do {
                                    mMarkForDelete.putBoolean(cur.getString(cur
                                            .getColumnIndex(BlackColumns.BlockRecorder._ID)), true);
                                    count++;
                                } while (cur.moveToNext());
                            }
                        } finally {
                            if (cur != null) {
                                cur.close();
                            }
                        }
                        drawList();
                        mMonitorSelectAll = false;
                    }

                } else {
                    mSelectAllTextView.setText(R.string.blackCalls_all_button_Selected);
                    count = mMarkForDelete.size();
                    if (mMonitorSelectAll) {
                        mMarkForDelete.clear();
                        drawList();
                        mMonitorSelectAll = false;
                    }
                }
                Cursor cur = getBlackListCursor();
                try {
                    if (cur == null || (!cur.moveToFirst())) {
                        setDoneMenu(false);
                    } else {
                        setDoneMenu(isChecked || !(count == cur.getCount()));
                        count = 0;
                    }
                } finally {
                    if (cur != null) {
                        cur.close();
                    }
                }
            }
        });
    }

    @Override
    public CursorAdapter createAdapter(Context context) {
        return new BlackListAdapter(context, null);
    }
}
