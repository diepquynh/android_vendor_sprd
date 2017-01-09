
package com.sprd.contacts.activities;

import android.R.anim;
import android.app.Activity;
import android.app.ListActivity;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.SparseArray;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

import com.android.contacts.R;
import com.sprd.contacts.DeduplicationCandidate;
import com.sprd.contacts.list.GroupCheckAdapter;
import com.sprd.contacts.list.DeduplicateCandidatesAdapter.RawContactWitAccount;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

public abstract class GroupCheckActivity<T> extends Activity implements OnItemClickListener {

    public static final int REQUEST_QUERY_ITEMS = 1;
    public static final int RESPONSE_QUERY_ITEMS = 50;
    public static final int REQUEST_EXTEND_BASE = 100;
    public static final int RESPONSE_EXTEND_BASE = 150;
    private static final String KEY_CHECKED_ITEM_POSITIONS = "key_checked_item_postions"; //bug370284 483388

    GroupCheckAdapter<T> mAdapter;
    DataAccessHander mDataAccessHander;
    HandlerThread mDataAccessThread;
    Button mCheckAll;
    Button mDone;
    ListView mList;
    TextView mLoadingText;
    TextView mEmpty;// bug 370543

    int mDoneResId;
    int mCheckResId;
    int mUncheckResId;
    int mTittleId;
    int mEmptyTextResId;// bug 370543
    long mCheckedContactIds[];// bug 370284 483388
    ArrayList<ArrayList<T>> mArray; // Bug 597550

    Handler mHander = new Handler() {
        public void handleMessage(Message msg) {
            if (msg.what == RESPONSE_QUERY_ITEMS) {
                ArrayList<ArrayList<T>> itemGroupArray = (ArrayList<ArrayList<T>>) msg.obj;
                mAdapter.changeDataSource(itemGroupArray);
                /**
                 * SPRD:Bug505856 while check all items on clearing up contacts, remove SIM card,
                 * buttons not show correctly.
                 *
                 * @{
                 */
                if (GroupCheckAdapter.isClearSelectItems) {
                    mCheckAll.setText(mCheckResId);
                    mDone.setEnabled(false);
                    GroupCheckAdapter.isClearSelectItems = false;
                }
                /**
                 * @}
                 */
                // bug 370543 begin
                if (itemGroupArray == null || itemGroupArray.size() == 0) {
                    mDone.setVisibility(View.GONE);
                    mCheckAll.setVisibility(View.GONE);
                    mList.setVisibility(View.GONE);
                    showEmptyText(mEmptyTextResId);
                } else {
                    showLoadingView(false);
                    mDone.setVisibility(View.VISIBLE);
                    mCheckAll.setVisibility(View.VISIBLE);
                    mList.setVisibility(View.VISIBLE);
                    // bug 370284 483388 begin
                    if (mCheckedContactIds != null) {
                        mAdapter.setAllCheckedItem(mCheckedContactIds);
                        boolean isAllChecked = mAdapter.isAllCheckd();
                        boolean isAllUnchecked = mAdapter.getCheckedItems().size() == 0;
                        mCheckAll.setText(isAllChecked ? mUncheckResId : mCheckResId);
                        mDone.setEnabled(!isAllUnchecked);
                        mCheckedContactIds = null;
                    }
                    // bug 370284 483388 end
                }
                // bug 370543 end
            } else {
                handleResponse(msg.what);
            }
        };
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.group_check_activity_layout);

        mAdapter = createListAdapter();
        mList = (ListView) findViewById(android.R.id.list);
        mList.setAdapter(mAdapter);
        mList.setOnItemClickListener(this);
        mLoadingText = (TextView) findViewById(android.R.id.empty);
        showLoadingView(true);
        mDataAccessThread = new HandlerThread("DataAccess") {
            @Override
            protected void onLooperPrepared() {
                mDataAccessHander = new DataAccessHander(mDataAccessThread.getLooper());
                onDataAccessReady();
            }
        };
        mDoneResId = getDoneResId();
        mCheckResId = getCheckResId();
        mUncheckResId = getUncheckResId();
        mEmptyTextResId = getEmptyResId();// bug 370543
        mDataAccessThread.start();
        mCheckAll = (Button) findViewById(R.id.groupcheck_select_all);
        mCheckAll.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                doCheckAll();
            }
        });
        mCheckAll.setText(mCheckResId);
        mCheckAll.setVisibility(View.GONE);// bug 370543

        mDone = (Button) findViewById(R.id.groupcheck_done);
        mDone.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                onDonePressed(mAdapter.getCheckedItems());
            }
        });

        mDone.setText(mDoneResId);
        mDone.setEnabled(false);// bug 369275
        mDone.setVisibility(View.GONE);

        // bug 370284 483388 begin
        mCheckedContactIds = null;
        if (savedInstanceState != null) {
            mCheckedContactIds = savedInstanceState.getLongArray(KEY_CHECKED_ITEM_POSITIONS);
        }
        // bug 370284 483388 end
    }

    class DataAccessHander extends Handler {

        public DataAccessHander(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            // Bug 597550 The DUT fails to re-check the checked item after closing the permission of Contacts.
            if (mArray == null && msg.what == REQUEST_QUERY_ITEMS) {
                ArrayList<ArrayList<T>> array = getItems();
                mArray = array;
                onDataAcquired(array);
            } else {
                handleRequest(msg.what, msg.obj);
            }
        }
    }

    protected final void requestDataAccessDelay(int what, Object obj, int delay) {
        if (mDataAccessHander != null) {
            mDataAccessHander.sendMessageDelayed(mDataAccessHander.obtainMessage(what, obj), delay);
        }
    }

    protected final void responseForDataAccess(int what) {
        mHander.sendEmptyMessage(what);
    }

    protected final void responseForDataAccessDelay(int what, int delay) {
        mHander.sendEmptyMessageDelayed(what, delay);
    }

    protected int getDoneResId() {
        return R.string.menu_done;
    }

    protected int getCheckResId() {
        return R.string.menu_select_all;
    }

    protected int getUncheckResId() {
        return R.string.menu_select_none;
    }

    // bug 370543 begin
    protected int getEmptyResId() {
        return R.string.no_duplicate_contact;
    }

    // bug 370543 end

    protected GroupCheckAdapter<T> getAdapter() {
        return mAdapter;
    }

    protected void handleRequest(int what, Object obj) {

    }

    protected void handleResponse(int what) {

    }

    protected void onDataAccessReady() {

    }

    public void showLoadingView(boolean isVisiable) {
        mLoadingText.setVisibility(isVisiable ? View.VISIBLE : View.GONE);
    }

    // bug 370543 begin
    public void showEmptyText(int resId) {
        mLoadingText.setVisibility(View.VISIBLE);
        mLoadingText.setText(resId);
    }

    // bug 370543 end

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        boolean isChecked = mAdapter.isChecked(position);
        mAdapter.setChecked(position, !isChecked);
        // bug 369275 begin
        boolean isAllChecked = mAdapter.isAllCheckd();
        boolean isAllUnchecked = mAdapter.getCheckedItems().size() == 0;
        mCheckAll.setText(isAllChecked ? mUncheckResId : mCheckResId);
        mDone.setEnabled(!isAllUnchecked);
        // bug 369275 end
        mAdapter.notifyDataSetChanged();
    }

    private void onDataAcquired(ArrayList<ArrayList<T>> itemGroupArray) {
        mHander.sendMessage(mHander.obtainMessage(RESPONSE_QUERY_ITEMS, itemGroupArray));
    }

    protected final void requestDataAccess(int what, Object obj) {
        if (mDataAccessHander != null) {
            mDataAccessHander.sendMessage(mDataAccessHander.obtainMessage(what, obj));
        }
    }

    private void doCheckAll() {
        boolean isAllChecked = mAdapter.isAllCheckd();
        mAdapter.checkAll(!isAllChecked);
        mCheckAll.setText(isAllChecked ? mCheckResId : mUncheckResId);
        mDone.setEnabled(!isAllChecked);// bug 369275
        mAdapter.notifyDataSetChanged();
    }

    // bug 370284 483388 begin
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        int size = mAdapter.getCheckedItems().size();
        SparseArray<ArrayList<Object>> checkedItems = mAdapter.getCheckedItems();
        ArrayList<Long> checkedIds = new ArrayList<Long>();
        for (int i = 0; i < size; i++) {
            ArrayList<Object> itemGroup = checkedItems.valueAt(i);
            for (Object l : itemGroup) {
                RawContactWitAccount rawContact = (RawContactWitAccount) l;
                checkedIds.add(rawContact.id);
            }
        }

        long checkedContactIds[] = new long[checkedIds.size()];
        for(int i = 0; i<checkedIds.size(); i++) {
            checkedContactIds[i] = checkedIds.get(i);
        }
        outState.putLongArray(KEY_CHECKED_ITEM_POSITIONS, checkedContactIds);
    }

    // bug 370284 483388 end

    protected abstract GroupCheckAdapter<T> createListAdapter();

    protected abstract ArrayList<ArrayList<T>> getItems();

    protected abstract void onDonePressed(SparseArray<ArrayList<Object>> sparseArray);

}
