
package com.sprd.firewall.ui;

import com.sprd.firewall.model.BlackEntity;
import com.sprd.firewall.model.BlackNumberEntity;
import com.sprd.firewall.ui.CallFireWallActivity.ViewPagerVisibilityListener;
import com.sprd.firewall.util.BlackEntityUtil;
import com.sprd.firewall.R;

import android.R.integer;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ListFragment;
import android.app.LoaderManager;
import android.app.ProgressDialog;
import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.ContentUris;
import android.content.Context;
import android.content.CursorLoader;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.Loader;
import android.content.OperationApplicationException;
import android.content.DialogInterface.OnClickListener;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.graphics.Color;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.RemoteException;
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
import android.view.View.OnLongClickListener;
import android.view.ViewGroup;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.View.OnTouchListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CursorAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.os.SystemProperties;
import android.provider.BlockedNumberContract;
import android.provider.BlockedNumberContract.BlockedNumbers;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.telephony.PhoneNumberUtils;
import android.content.ActivityNotFoundException;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map.Entry;
import java.util.Map;

import com.sprd.firewall.util.BlackCallsUtils;
import com.sprd.firewall.util.ProgressUtil;
import com.sprd.firewall.util.ProgressUtil.ProgressType;
import android.net.Uri;

import android.database.ContentObserver;
import android.os.Handler;
import android.os.Message;

public class BlackFragment extends BaseFragment{
    private static final String TAG = "BlackFragment";

    private static final int MENU_DELETE = 0;

    private static final int MENU_EDIT = 1;

    private static final int MENU_DETAIL = 2;

    private boolean mBatchOperation = false;

    private BlackNumberEntity mNumberEntity;

    private static final int SMS_SHIFT = 0;
    private static final int CALL_SHIFT = 1;
    private static final int VT_SHIFT = 2;

    private static final int SMS_SELECT = 1 << SMS_SHIFT;
    private static final int CALL_SELECT = 1 << CALL_SHIFT;
    private static final int VT_SELECT = 1 << VT_SHIFT;

    private static final int REQUEST_CODE_BLACK_EDIT = 1;
    private static final String MULTI_PICK_CONTACTS_ACTION = "com.android.contacts.action.MULTI_TAB_PICK";
    private static final int REQUEST_CODE_PICK = 99;
    private static final String BLOCK_TYPE = "block_type";
    private static final String NAME = "name";
    public static final String MIN_MATCH = "min_match";

    private MenuItem mAddMenuItem;
    private MenuItem mMenuEdit;

    private boolean mDeleteDone = false;

    private MenuItem mAddCallMenuItem;
    private MenuItem mAddSmsMenuItem;
    private MenuItem mAddCallWithSmsMenuItem;

    private static final Uri baseUri = BlockedNumberContract.BlockedNumbers.CONTENT_URI;
    private int mBatchAddCount = 0;
    private Intent mData;
    /* SPRD: add for bug600800 @{ */
    private AlertDialog mDialog = null;
    private DeleteHandler mDeleteHandler = null;
    private final int DISMISS = 1;
    /* @} */
    // SPRD: Add for bug 611089.
    private SharedPreferences mSharedPreferences;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mLoaderId = LOADER_ID_BLACK_NUMBER;
        // SPRD: Add for bug 611089.
        mSharedPreferences = mContext.getSharedPreferences("isBatchOperating",
                Context.MODE_PRIVATE);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        final View listLayout = inflater.inflate(R.layout.black_list, container, false);
        mSelectAllLinearLayout = listLayout.findViewById(R.id.selecte_all_layout);
        mDivider = listLayout.findViewById(R.id.divider);
        mSelectAll = (CheckBox) listLayout.findViewById(R.id.selete_all);
        mSelectAllTextView = (TextView) listLayout.findViewById(R.id.selete_all_text);
        // SPRD: add for bug 600800
        mDeleteHandler = new DeleteHandler();
        mContext.getContentResolver().registerContentObserver(baseUri, true, mObserver);

        return listLayout;
    }

    @Override
    public void onResume() {
        Log.i(TAG, "onResume");
        /* SPRD: modify for bug620809 @{ */
        mMarkForDeleteCount = mMarkForDelete.size();
        mSelectAll.setChecked(mCurrentCursorCount != 0
                && !refershMarkForDelete()
                && mCurrentCursorCount <= mMarkForDeleteCount);
        /* @} */
        setDoneMenu(mMarkForDelete.size() > 0);
        if (mCurrentCursorCount == 0) {
            mDeleteState = false;
            Activity activity = getActivity();
            if (activity != null) {
                activity.invalidateOptionsMenu();
            }
        }
        super.onResume();
    }

    public class BlackListAdapter extends BaseFragment.CallFireWallListAdapter {

        private BlackNumberEntity member = new BlackNumberEntity();

        public BlackListAdapter(Context context, Cursor c) {
            super(context, c, R.layout.black_list_item);
        }

        @Override
        public String setViewHolder(
                BaseFragment.CallFireWallListAdapter.ViewHolder holder,
                View view, Cursor cursor) {
            holder.select = (CheckBox) view.findViewById(R.id.select);
            holder.select.setFocusable(false);
            holder.select.setFocusableInTouchMode(false);
            holder.select.setClickable(false);

            holder.phone_number = (TextView) view.findViewById(R.id.phone_number);
            holder.name = (TextView) view.findViewById(R.id.phone_name);
            holder.block_sms = (ImageView) view.findViewById(R.id.image_block_sms);
            holder.block_call = (ImageView) view.findViewById(R.id.image_block_call);
            holder.divider = (View) view.findViewById(R.id.divider02);

            if (mDeleteState) {
                holder.select.setVisibility(View.VISIBLE);
            } else {
                holder.select.setVisibility(View.GONE);
            }
            BlackEntityUtil.transform(member, cursor);

            holder.phone_number.setText(member.getNumber());
            if ((member.getName() != null)
                    && (!TextUtils.isEmpty(member.getName()))) {
                holder.name.setText(member.getName());
            } else {
                holder.name.setText(member.getNumber());
            }

            int type = member.getType();

            if ((SMS_SELECT & type) == SMS_SELECT) {
                holder.block_sms.setImageResource(R.drawable.block_type_list_sms_selected);
            } else {
                holder.block_sms.setImageResource(R.drawable.block_type_list_sms_default);
            }
            if ((CALL_SELECT & type) == CALL_SELECT) {
                holder.block_call.setImageResource(R.drawable.block_type_list_call_selected);
            } else {
                holder.block_call.setImageResource(R.drawable.block_type_list_call_default);
            }
            return (member.getId() + "");
        }
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);

        // SPRD: modify for bug609151
        //inflater.inflate(R.menu.black_options, menu);
    }

    @Override
    public void onPrepareOptionsMenu(Menu menu) {
        /* SPRD: modify for bug609151 @{ */
        if (menu != null) {
            mAddMenuItem = menu.findItem(R.id.black_menu_add);
            mDeleteMenuItem = menu.findItem(R.id.black_menu_delete);
            mAddCallMenuItem = menu.findItem(R.id.add_call_menu);
            mAddSmsMenuItem = menu.findItem(R.id.add_sms_menu);
            mAddCallWithSmsMenuItem = menu.findItem(R.id.add_call_with_sms_menu);
            mDeleteConfirmMenuItem = menu.findItem(R.id.black_menu_delete_confirm);
            mCancelMenuItem = menu.findItem(R.id.black_menu_cancel);
            if (mAddMenuItem == null) {
                return;
            }

            setMenuItemState();
        }
        /* @} */
    }

    @Override
    void setMenuItemState() {
        if (mAddMenuItem == null || mDeleteMenuItem == null || mDeleteConfirmMenuItem == null
                || mCancelMenuItem == null) {
            return;
        }
        if (mDeleteState) {
            mAddMenuItem.setVisible(false);
            mAddCallMenuItem.setVisible(false);
            mAddSmsMenuItem.setVisible(false);
            mAddCallWithSmsMenuItem.setVisible(false);
            mDeleteMenuItem.setVisible(false);
            mDeleteConfirmMenuItem.setVisible(true);
            mDeleteConfirmMenuItem.setEnabled(false);
            if (mMarkForDelete.size() > 0) {
                setDoneMenu(true);
            }
            mCancelMenuItem.setVisible(true);
        } else {
            mAddMenuItem.setVisible(true);
            mDeleteMenuItem.setVisible(true);
            mAddCallMenuItem.setVisible(true);
            mAddSmsMenuItem.setVisible(true);
            mAddCallWithSmsMenuItem.setVisible(true);
            mDeleteConfirmMenuItem.setVisible(false);
            mCancelMenuItem.setVisible(false);
            Cursor cur = getBlackListCursor();
            mDeleteMenuItem.setEnabled(cur != null && cur.getCount() > 0);
            if (cur != null) {
                cur.close();
            }
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        Intent intentPick = new Intent(MULTI_PICK_CONTACTS_ACTION);
        SharedPreferences sp = mContext.getSharedPreferences("isBatchOperating",
                Context.MODE_PRIVATE);
        mBatchOperation = sp.getBoolean("batchOperation", false);
        switch (item.getItemId()) {
            /* SPRD: Add for bug 611089 @{ */
            case R.id.black_menu_add:
                if (!canCurrentUserOpenCallFireWall(mContext)) {
                    Toast.makeText(mContext, R.string.refuse_after_close_blocked_storage,
                            Toast.LENGTH_SHORT).show();
                } else if (mBatchOperation) {
                    Toast.makeText(mContext, getString(R.string.adding_blacklist_hold_on),
                            Toast.LENGTH_SHORT).show();
                } else {
                    startActivity(new Intent(this.getActivity(), BlackCallsListAddActivity.class));
                }
                break;
            case R.id.add_sms_menu:
                if (!canCurrentUserOpenCallFireWall(mContext)) {
                    Toast.makeText(mContext, R.string.refuse_after_close_blocked_storage,
                            Toast.LENGTH_SHORT).show();
                } else if (mBatchOperation) {
                    Toast.makeText(mContext, getString(R.string.adding_blacklist_hold_on),
                            Toast.LENGTH_SHORT).show();
                } else {
                    try {
                        intentPick.putExtra("blackcall_type", "1");
                        intentPick.putExtra("cascading",
                                new Intent(MULTI_PICK_CONTACTS_ACTION)
                                        .setType(Phone.CONTENT_ITEM_TYPE));
                        startActivityForResult(intentPick, REQUEST_CODE_PICK);
                    } catch (ActivityNotFoundException e) {
                        Log.i(TAG, "Contacts has been stopped.");
                        Toast.makeText(mContext, getString(R.string.Contact_error),
                                Toast.LENGTH_SHORT).show();
                    }
                }
                break;
            case R.id.add_call_menu:
                if (!canCurrentUserOpenCallFireWall(mContext)) {
                    Toast.makeText(mContext, R.string.refuse_after_close_blocked_storage,
                            Toast.LENGTH_SHORT).show();
                } else if (mBatchOperation) {
                    Toast.makeText(mContext, getString(R.string.adding_blacklist_hold_on),
                            Toast.LENGTH_SHORT).show();
                } else {
                    try {
                        intentPick.putExtra("blackcall_type", "2");
                        intentPick.putExtra("cascading",
                                new Intent(MULTI_PICK_CONTACTS_ACTION)
                                        .setType(Phone.CONTENT_ITEM_TYPE));
                        startActivityForResult(intentPick, REQUEST_CODE_PICK);
                    } catch (ActivityNotFoundException e) {
                        Log.i(TAG, "Contacts has been stopped.");
                        Toast.makeText(mContext, getString(R.string.Contact_error),
                                Toast.LENGTH_SHORT).show();
                    }
                }
                break;
            case R.id.add_call_with_sms_menu:
                if (!canCurrentUserOpenCallFireWall(mContext)) {
                    Toast.makeText(mContext, R.string.refuse_after_close_blocked_storage,
                            Toast.LENGTH_SHORT).show();
                } else if (mBatchOperation) {
                    Toast.makeText(mContext, getString(R.string.adding_blacklist_hold_on),
                            Toast.LENGTH_SHORT).show();
                } else {
                    try {
                        intentPick.putExtra("blackcall_type", "3");
                        intentPick.putExtra("cascading",
                                new Intent(MULTI_PICK_CONTACTS_ACTION)
                                        .setType(Phone.CONTENT_ITEM_TYPE));
                        startActivityForResult(intentPick, REQUEST_CODE_PICK);
                    } catch (ActivityNotFoundException e) {
                        Log.i(TAG, "Contacts has been stopped.");
                        Toast.makeText(mContext, getString(R.string.Contact_error),
                                Toast.LENGTH_SHORT).show();
                    }
                }
                break;
            case R.id.black_menu_delete:
                if (!canCurrentUserOpenCallFireWall(mContext)) {
                    Toast.makeText(mContext, R.string.refuse_after_close_blocked_storage,
                            Toast.LENGTH_SHORT).show();
                } else if (mBatchOperation) {
                    Toast.makeText(mContext, getString(R.string.adding_blacklist_hold_on),
                            Toast.LENGTH_SHORT).show();
                } else {
                    Cursor cur = getBlackListCursor();
                    if (cur != null) {
                        if (cur.getCount() > 0) {
                            mDeleteState = true;
                            drawList();
                        }
                        cur.close();
                    }
                    getActivity().invalidateOptionsMenu();
                }
                break;
            /* @} */
            case R.id.black_menu_delete_confirm:
                showDialogByType(DELETE_ALL_DIALOG);
                break;
            case R.id.black_menu_cancel:
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
        if (type == NONE_DIALOG) {
            return;
        }
        if (type == DELETE_ALL_DIALOG) {
            if (mDeleteAllBuilder == null) {
                mDeleteAllBuilder = new AlertDialog.Builder(mContext);
            }
            mDeleteAllBuilder.setTitle(R.string.confirm_delete_number_title)
                    .setIconAttribute(android.R.attr.alertDialogIcon)
                    .setMessage(R.string.confirm_delete_number)
                    .setPositiveButton(android.R.string.ok, new OnClickListener() {
                        public void onClick(DialogInterface dialog, int whichButton) {
                            mProgressUtil = new ProgressUtil(mContext,
                                    mMarkForDelete.size(),
                                    ProgressType.BLACKLIST_DEL,
                                    BlackFragment.this);
                            mProgressUtil.setMtitleId(R.string.Delete_Blacklist_title);
                            mProgressUtil.execute();
                        }
                    }).setNegativeButton(android.R.string.cancel, null).setCancelable(true)
                    .setOnDismissListener(new DialogInterface.OnDismissListener() {
                        public void onDismiss(DialogInterface dialog) {
                            mDialogType = NONE_DIALOG;
                        }
                    });
            // SPRD: modify for bug 600800
            mDialog = mDeleteAllBuilder.create();
            mDialog.show();
        } else if (type == DELETE_DIALOG) {
            if (mDeleteBuilder == null) {
                mDeleteBuilder = new AlertDialog.Builder(mContext);
            }
            mDeleteBuilder.setTitle(R.string.confirm_delete_number_title)
                    .setIconAttribute(android.R.attr.alertDialogIcon)
                    .setMessage(R.string.confirm_delete_number)
                    .setPositiveButton(android.R.string.ok, new OnClickListener() {
                        public void onClick(DialogInterface dialog, int whichButton) {
                            try {
                                /**
                                 * Delete data from the database
                                 */
                                deleteOneBlack(mNumberEntity.getNumber());
                                updataBlackList();
                                drawList();
                                mDeleteDone = true;
                            } finally {
                                dialog.dismiss();
                            }
                        }
                    }).setNegativeButton(android.R.string.cancel, null).setCancelable(true)
                    .setOnDismissListener(new DialogInterface.OnDismissListener() {
                        public void onDismiss(DialogInterface dialog) {
                            mDialogType = NONE_DIALOG;
                        }
                    });
            // SPRD: modify for bug 600800
            mDialog = mDeleteBuilder.create();
            mDialog.show();
        } else if (type == DETAIL_DIALOG) {
            String phoneName;
            if (mNumberEntity.getName() == null || TextUtils.isEmpty(mNumberEntity.getName())) {
                phoneName = mNumberEntity.getNumber();
            } else {
                phoneName = mNumberEntity.getName();
            }
            String phoneNumber = mNumberEntity.getNumber();
            if (mDetailBuilder == null) {
                mDetailBuilder = new AlertDialog.Builder(mContext);
            }
            mDetailBuilder.setTitle(R.string.blackCallsIconLabel)
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
                            mDialogType = NONE_DIALOG;
                        }
                    });
            mDetailBuilder.setMessage(getString(
                    R.string.blackCalls_blacklist_detail, phoneName, phoneNumber));
            mDetailBuilder.create().show();
        }
        mDialogType = type;
    }
    /** @} */

    private AlertDialog mAlertDialog = null;

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
        /* SPRD: add for bug 624314. @{ */
        if (mSharedPreferences != null) {
            mBatchOperation = mSharedPreferences.getBoolean("batchOperation", false);
        }

        if (mBatchOperation) {
            Toast.makeText(mContext, getString(R.string.adding_blacklist_hold_on),
                    Toast.LENGTH_SHORT).show();
        } else if (!mDeleteState) {
            menu.setHeaderTitle(R.string.BlaclList_conMenu_title);
            mMenuDelete = menu.add(0, MENU_DELETE, 0, R.string.BlaclList_conMenu_delete);
            mMenuEdit = menu.add(0, MENU_EDIT, 0, R.string.BlaclList_conMenu_edit);
            mMenuDetail = menu.add(0, MENU_DETAIL, 0, R.string.BlackList_conMenu_detail);
        }
        /* @} */
    }

    @Override
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
        mNumberEntity = new BlackNumberEntity();

        if (aItem == mMenuDelete) {
            BlackEntityUtil.transform(mNumberEntity, cursor);
            showDialogByType(DELETE_DIALOG);
            return true;
        } else if (aItem == mMenuEdit) {
            BlackEntityUtil.transform(mNumberEntity, cursor);
            Log.d(TAG, "Type=" + mNumberEntity.getType());
            Intent intent = new Intent(mContext, BlackCallsListAddActivity.class);
            Bundle bundle = new Bundle();
            bundle.putString("Click_BlackCalls_Number", mNumberEntity.getNumber());
            bundle.putInt("Click_BlackCalls_Type", mNumberEntity.getType());
            bundle.putBoolean("Click_BlackCalls_Edit", true);
            bundle.putString("Click_BlackCalls_Name", mNumberEntity.getName());
            intent.putExtras(bundle);
            startActivityForResult(intent, REQUEST_CODE_BLACK_EDIT);
            return true;
        } else if (aItem == mMenuDetail) {
            BlackEntityUtil.transform(mNumberEntity, cursor);
            showDialogByType(DETAIL_DIALOG);
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
                BlockedNumberContract.BlockedNumbers.COLUMN_ID,
                BlockedNumberContract.BlockedNumbers.COLUMN_ORIGINAL_NUMBER,
                BlockedNumbers.COLUMN_E164_NUMBER,
                BLOCK_TYPE, NAME
        };
        cur = cr.query(BlockedNumberContract.BlockedNumbers.CONTENT_URI, columns ,null, null, null);
        if (cur != null) {
            mCurrentCursorCount = cur.getCount();
        }
        return cur;
    }

    public void onActivityResult(int requestCode, int resultCode, Intent
            data) {
        final int mRequestCode = requestCode;
        final int mResultCode = resultCode;
        mData = data;

        if (mData == null) {
            return;
        }
        if (requestCode == REQUEST_CODE_BLACK_EDIT) {
           Bundle bunde = mData.getExtras();
           String add_numberString = bunde.getString("phonenumber");
           int add_type = bunde.getInt("type");
           updataBlackList();
        } else {
            int mAddBlacklistCount = 0;
            HashMap<String, String> contacts;
            if ((mData.getExtras().getString("blackcall_type") != null)
                    && (null != mData.getSerializableExtra("result"))) {
                Log.i(TAG, "onActivityResult add");
                contacts = (HashMap<String, String>)mData.getSerializableExtra("result");
                mProgressUtil = new ProgressUtil(mContext,contacts.size(),ProgressType.BLACKLIST_ADD,this);
                mProgressUtil.setMtitleId(R.string.Add_Blacklist_title);
                mProgressUtil.execute();
            }
        }
    }

    private void BatchOperationAddBlackCalls(ArrayList<ContentProviderOperation> operations) {

        ContentResolver cr = mContext.getContentResolver();
        if (!operations.isEmpty()) {
            try {
                cr.applyBatch(BlockedNumberContract.AUTHORITY, operations);
            } catch (OperationApplicationException e) {
                // TODO: handle exception
                Log.e(TAG, "Version consistency failed");
            } catch (RemoteException e) {
                // TODO: handle exception
                Log.e(TAG, "Add blacklist is failed");
            /* SPRD: add for bug596886 @{ */
            } catch (IllegalArgumentException e) {
                Log.e(TAG, "BatchOperationAddBlackCalls  " + e);
            } catch (Exception e) {
                Log.e(TAG, "BatchOperationAddBlackCalls Exception  " + e);
            }
            /* @} */
        }
    }

    public boolean getDeleteDone() {
        return mDeleteDone;
    }

    public void setDeleteDone(boolean b) {
        mDeleteDone = b;
    }

    public void updataBlackList() {
        Log.i(TAG, "updataBlackList");
        mContext.getContentResolver().notifyChange(baseUri, null);
    }

    private void deleteOneBlack(String number) {
        BlackCallsUtils blackCallsUtils = new BlackCallsUtils(mContext);
        blackCallsUtils.DelBlackCalls(number);
    }

    @Override
    public void doInBack(ProgressType type) {
        // TODO Auto-generated method stub

        Log.i(TAG, "doInBack-doInBack type="+type);
        if (mProgressUtil == null) {
            Log.i(TAG, "doInBack-mProgressUtil = null");
            return;
        }

        if (type == ProgressType.BLACKLIST_ADD) {

            HashMap<String, String> contacts;
            contacts = (HashMap<String, String>) mData.getSerializableExtra("result");
            ArrayList<ContentProviderOperation> operations
                    = new ArrayList<ContentProviderOperation>();
            int icount = 0;
            int mAddBlacklistCount = 0;
            Iterator i = contacts.entrySet().iterator();
            ContentResolver cr = mContext.getContentResolver();

            /* SPRD: add for bug 624314. @{ */
            if (mSharedPreferences != null) {
                mSharedPreferences.edit()
                    .putBoolean("batchOperation", true)
                    .commit();
            }
            /* @} */
            while (i.hasNext()) {

                Map.Entry entry = (Map.Entry) i.next();
                String val;
                if (entry.getValue() == null) {
                    val = entry.getKey().toString();
                } else {
                    val = entry.getValue().toString();
                    mAddBlacklistCount++;
                    mProgressUtil.UpdateProgress(mAddBlacklistCount);
                }

                String key = entry.getKey().toString();
                // SPRD: Bug 601543 Can add duplicate black number
                String origNumber = PhoneNumberUtils.stripSeparators(key);
                cr.delete(BlockedNumberContract.BlockedNumbers.CONTENT_URI,
                        BlockedNumberContract.BlockedNumbers.COLUMN_ORIGINAL_NUMBER + "='"
                        + origNumber + "'", null); //SPRD: Bug 601543 Can add duplicate black number
                ContentValues values = new ContentValues();
                String normalizedNumber = PhoneNumberUtils.normalizeNumber(key);
                // SPRD: Bug 601543 Can add duplicate black number
                values.put(BlockedNumberContract.BlockedNumbers.COLUMN_ORIGINAL_NUMBER, origNumber);
                values.put(BLOCK_TYPE,
                        Integer.parseInt(mData.getExtras().getString("blackcall_type")));
                values.put(MIN_MATCH, PhoneNumberUtils.toCallerIDMinMatch(normalizedNumber));
                values.put(NAME, val);
                operations.add(ContentProviderOperation
                        .newInsert(BlockedNumberContract.BlockedNumbers.CONTENT_URI).withValues(values)
                        .withYieldAllowed(false).build());
                icount++;
                if (icount >= 20) {
                    BatchOperationAddBlackCalls(operations);
                    operations.clear();
                    icount = 0;
                }
            }
            if (icount > 0) {
                BatchOperationAddBlackCalls(operations);
                operations.clear();
            }
            mData = null;
            /* SPRD: add for bug 624314. @{ */
            if (mSharedPreferences != null) {
                mSharedPreferences.edit()
                    .putBoolean("batchOperation", false)
                    .commit();
            }
            /* @} */
        }else if (type == ProgressType.BLACKLIST_DEL) {

            ContentResolver cr = mContext.getContentResolver();
            String mId;
            Cursor cur = getBlackListCursor();
            /* SPRD: add for bug 624314. @{ */
            if (mSharedPreferences != null) {
                mSharedPreferences.edit()
                    .putBoolean("batchOperation", true)
                    .commit();
            }

            // SPRD: add for Bug 632790
            Bundle tempBundle = new Bundle(mMarkForDelete);
            /* @} */
            int deleteNums = 0;
            try {
                if (cur != null && cur.moveToFirst()) {
                    do {
                        /* SPRD: modify for Bug 632790 @{ */
                        mId = cur.getString(
                                cur.getColumnIndex(BlockedNumberContract.BlockedNumbers.COLUMN_ID));
                        if (tempBundle.containsKey(mId)) {
                            String blackNumberForDelete = cur.getString(
                                    cur.getColumnIndex(BlockedNumberContract.
                                            BlockedNumbers.COLUMN_ORIGINAL_NUMBER));
                            cr.delete(BlockedNumberContract.BlockedNumbers.CONTENT_URI,
                                    BlockedNumberContract.
                                            BlockedNumbers.COLUMN_ID + "='" + mId + "'", null);
                            cr.delete(BlockedNumberContract.BlockedNumbers.CONTENT_URI,
                                    BlockedNumberContract.BlockedNumbers.COLUMN_ORIGINAL_NUMBER
                                            + "='" + blackNumberForDelete + "'", null);
                            deleteNums++;
                            mProgressUtil.UpdateProgress(deleteNums);
                        }
                        /* @} */
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

            /* SPRD: add for bug 624314. @{ */
            if (mSharedPreferences != null) {
                mSharedPreferences.edit()
                    .putBoolean("batchOperation", false)
                    .commit();
            }
            /* @} */
        }
    }

    @Override
    public void doResult() {
        // TODO Auto-generated method stub
        Log.i(TAG, "doResult");

        Activity activity = getActivity();
        if (activity != null) {

            activity.invalidateOptionsMenu();
            drawList();
            updataBlackList();
        }
    }

    private boolean refershMarkForDelete() {
        Cursor cur = getBlackListCursor();
        if (cur == null || cur.getCount() == 0) {
            mMarkForDelete.clear();
            Log.i(TAG, "refershMarkForDelete: cur is null");
            return true;
        }

        boolean addFlag = false;
        Bundle tempBundle = new Bundle(mMarkForDelete);

        try {
            if (cur.moveToFirst()) {
                mMarkForDelete.clear();
                do {
                    String strId = cur.getString(cur
                            .getColumnIndex(BlockedNumberContract.BlockedNumbers.COLUMN_ID));
                    if (tempBundle.containsKey(strId)) {
                        mMarkForDelete.putBoolean(strId, true);
                    } else {
                        addFlag = true;
                    }
                } while (cur.moveToNext());
            }
        } finally {
            if (cur != null) {
                cur.close();
            }
        }

        return addFlag;
    }

    @Override
    public Loader<Cursor> onCreateLoader(int arg0, Bundle arg1) {
        String[] columns = new String[] {
                BlockedNumberContract.BlockedNumbers.COLUMN_ID,
                BlockedNumberContract.BlockedNumbers.COLUMN_ORIGINAL_NUMBER,
                BlockedNumbers.COLUMN_E164_NUMBER,
                BLOCK_TYPE, NAME
        };
        return new CursorLoader(getActivity(), BlockedNumberContract.BlockedNumbers.CONTENT_URI, columns,null, null, null);
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
                            if (cur.moveToFirst()) {
                                do {
                                    mMarkForDelete.putBoolean(cur.getString(cur
                                            .getColumnIndex(BlockedNumberContract.BlockedNumbers.COLUMN_ID)), true);
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
                    if (mMonitorSelectAll) {
                        mMarkForDelete.clear();
                        drawList();
                        mMonitorSelectAll = false;
                    }
                }
                setDoneMenu(isChecked);
            }
        });
    }

    @Override
    public CursorAdapter createAdapter(Context context) {
        return new BlackListAdapter(context, null);
    }

    @Override
    BlackEntity getBlackEntity() {
        return new BlackNumberEntity();
    }

    /* SPRD: add for bug600800 @{ */
    @Override
    public void onDetach() {
        mContext.getContentResolver().unregisterContentObserver(mObserver);
        super.onDetach();
    }

    ContentObserver mObserver = new ContentObserver(mDeleteHandler) {
        @Override
        public void onChange(boolean selfChange) {
            super.onChange(selfChange);
            mDeleteHandler.removeMessages(DISMISS);
            mDeleteHandler.sendEmptyMessage(DISMISS);
        }
    };

    private class DeleteHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case DISMISS:
                /* SPRD: add for bug620809 @{ */
                if (mSelectAllLinearLayout != null
                        && mSelectAllLinearLayout.getVisibility() == View.VISIBLE) {
                    mMarkForDeleteCount = mMarkForDelete.size();
                    mSelectAll.setChecked(mCurrentCursorCount != 0
                            && !refershMarkForDelete()
                            && mCurrentCursorCount <= mMarkForDeleteCount);
                    setDoneMenu(mMarkForDelete.size() > 0);
                }
                /* @} */
                    refreshDialog(mDialog);
                    break;
            }
            super.handleMessage(msg);
        }
    };
    /* @} */

    /* SPRD: Add for bug 611089 @{ */
    private boolean canCurrentUserOpenCallFireWall(Context context) {
        // BlockedNumberContract blocking, verify through Contract API
        try {
            return BlockedNumberContract.canCurrentUserBlockNumbers(context);
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
            return false;
        }
    }
    /* @} */
}
