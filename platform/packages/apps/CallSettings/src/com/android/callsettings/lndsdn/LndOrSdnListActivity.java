package com.android.callsettings.lndsdn;

import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.TelephonyIntents;
import com.android.callsettings.R;
import com.android.callsettings.SubscriptionInfoHelper;
import com.android.callsettings.fdnsettings.IccUriUtils;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.AsyncQueryHandler;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.telecom.PhoneAccount;
import android.telecom.TelecomManager;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.CursorAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.text.TextUtils;

public class LndOrSdnListActivity extends Activity {

    public static final String TAG = "LndOrSdnListActivity";

    public static final int MENU_ITEM_VIEW = 1;
    public static final int MENU_ITEM_CALL = 2;
    public static final int MENU_ITEM_DELETE = 3;

    public static final String NAME = "name";
    public static final String NUMBER = "number";

    public static final String[] CONTENT_PROJECTION = new String[] {
            NAME, NUMBER
    };

    private static final int QUERY = 0;
    private static final int DELETE = 1;

    private ContentResolver mContentResolver;
    protected QueryHandler mQueryHandler;
    private ListView mListView;
    private Uri mUri;
    MyAdapter mAdapter;
    private ProgressDialog mProgressDialog;

    int mSubId;
    int mListType;
    int mOperateType = 0;
    boolean isDelSuccess = false;
    Cursor mCursor = null;

    String mName;
    String mNumber;

    private SubscriptionInfoHelper mSubscriptionInfoHelper;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate...");
        setContentView(R.layout.lnd_sdn_list_activity_ex);

        final ActionBar actionBar = getActionBar();
        actionBar.setNavigationMode(ActionBar.NAVIGATION_MODE_STANDARD);
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setDisplayShowTitleEnabled(true);

        mListView = (ListView) findViewById(R.id.list_view);
        mContentResolver = getContentResolver();
        mQueryHandler = new QueryHandler(mContentResolver);

        mSubscriptionInfoHelper = new SubscriptionInfoHelper(this, getIntent());
        mSubId = mSubscriptionInfoHelper.getPhone().getSubId();
        mListType = getIntent().getIntExtra(IccUriUtils.LIST_TPYE, IccUriUtils.LND);
        Log.d(TAG, "onCreate...mListType = " + mListType + ", mSubId = " + mSubId);
        if (mListType == IccUriUtils.LND) {
            LndOrSdnListActivity.this.setTitle(R.string.lnd_list);
            registerForContextMenu(mListView);
        }
        mUri = IccUriUtils.getInstance().getIccURI(
                mListType == IccUriUtils.SDN ? IccUriUtils.SDN_STRING
                        : IccUriUtils.LND_STRING, mSubId);

        mAdapter = new MyAdapter(this, null);
        mListView.setAdapter(mAdapter);
        mListView.setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                TextView tvNumber = (TextView) view.findViewById(R.id.text_number);
                String number = (String) tvNumber.getText();
                dialLndOrSdnNum(number);
            }
        });

        queryLndOrSdn();
        displayProgress();
        mContentResolver.registerContentObserver(mUri, true, mObserver);
        IntentFilter intentFilter = new IntentFilter(
                TelephonyIntents.ACTION_SIM_STATE_CHANGED + mSubId);
        intentFilter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
        registerReceiver(myReceiver, intentFilter);
    }

    private BroadcastReceiver myReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(TelephonyIntents.ACTION_SIM_STATE_CHANGED + mSubId)) {
                if (IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(intent
                        .getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE))) {
                    if (null != mAdapter) {
                        mAdapter.changeCursor(null);
                    }
                    mContentResolver.unregisterContentObserver(mObserver);
                }
            }
        }
    };

    private void queryLndOrSdn() {
        mQueryHandler.startQuery(0, null, mUri, CONTENT_PROJECTION, null, null, null);
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo menuInfoIn) {
        Log.i(TAG, "onCreateContextMenu");
        AdapterView.AdapterContextMenuInfo menuInfo;
        try {
            menuInfo = (AdapterView.AdapterContextMenuInfo) menuInfoIn;
        } catch (ClassCastException e) {
            Log.i(TAG, "ClassCastException", e);
            return;
        }
        final Cursor c = (Cursor) mListView.getAdapter().getItem(menuInfo.position);
        String menuTitle = c.getString(c.getColumnIndex(NAME));

        menu.setHeaderTitle(menuTitle);

        menu.add(0, MENU_ITEM_CALL, 0, R.string.menu_call);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        Log.i(TAG, "onContextItemSelected");
        final AdapterView.AdapterContextMenuInfo menuInfo;
        try {
            menuInfo = (AdapterView.AdapterContextMenuInfo) item.getMenuInfo();
        } catch (ClassCastException e) {
            Log.i(TAG, "ClassCastException", e);
            return false;
        }
        Log.i(TAG, "menuInfo.id = " + menuInfo.id + "\n menuInfo.position = " + menuInfo.position);
        final Cursor c = (Cursor) mListView.getAdapter().getItem(menuInfo.position);
        mName = c.getString(c.getColumnIndex(NAME));
        mNumber = c.getString(c.getColumnIndex(NUMBER));
        Log.i(TAG, "nName = " + mName + "\nmNumber = " + mNumber);
        Intent intent = null;
        switch (item.getItemId()) {
            case MENU_ITEM_VIEW:
                String strUri = mUri + "/" + mName;
                intent = new Intent(Intent.ACTION_VIEW, Uri.parse(strUri));
                startActivity(intent);
                break;
            case MENU_ITEM_CALL:
                dialLndOrSdnNum(mNumber);
                break;
            case MENU_ITEM_DELETE: {
                deleteLndOrSdn();
            }
        }
        return super.onContextItemSelected(item);
    }

    private void displayProgress() {
        mProgressDialog = new ProgressDialog(this);
        mProgressDialog.setMessage(getText(R.string.wait_message));
        mProgressDialog.setIndeterminate(true);
        mProgressDialog.setCancelable(false);
        mProgressDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_DIALOG);
        mProgressDialog.getWindow().addFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND);
        mProgressDialog.show();
    }

    private void dismissProgress() {
        if (mProgressDialog != null) {
            mProgressDialog.dismiss();
            mProgressDialog = null;
        }
    }

    private boolean deleteLndOrSdn() {
        new AlertDialog.Builder(LndOrSdnListActivity.this)
                .setTitle(R.string.remove)
                .setIcon(android.R.drawable.ic_dialog_alert)
                .setMessage(R.string.delete_selected_contents)
                .setNegativeButton(android.R.string.cancel, null)
                .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {

                    public void onClick(DialogInterface dialog, int which) {
                        isDelSuccess = false;
                        if (mUri != Uri.EMPTY) {
                            mQueryHandler.startDelete(0, null, mUri, null, new String[] {
                                    mName, mNumber, null, null
                            });
                            displayProgress();
                        }
                    }
                }).show();

        return true;
    }

    public void dialLndOrSdnNum(String number) {
        // SPRD:  check number not null
        if (TextUtils.isEmpty(number)) {
           Log.w(TAG, "strNumber = null");
           return;
        }
        Log.d(TAG, "strNumber = " + number);
        Intent intent = new Intent(Intent.ACTION_CALL_PRIVILEGED,
                Uri.fromParts(PhoneAccount.SCHEME_TEL, number, null));
        Bundle extras = new Bundle();
        intent.putExtra(TelecomManager.EXTRA_OUTGOING_CALL_EXTRAS, extras);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
    }

    private class QueryHandler extends AsyncQueryHandler {
        public QueryHandler(ContentResolver cr) {
            super(cr);
        }

        @Override
        protected void onQueryComplete(int token, Object cookie, Cursor c) {
            mCursor = c;
            Log.d(TAG,"onQueryComplete...");
            updateList(c);
            dismissProgress();
        }

        @Override
        protected void onInsertComplete(int token, Object cookie, Uri uri) {
        }

        @Override
        protected void onUpdateComplete(int token, Object cookie, int result) {
        }

        @Override
        protected void onDeleteComplete(int token, Object cookie, int result) {
            Log.d(TAG, "onDeleteComplete");
            mOperateType = DELETE;
            updateScreen(result);
        }
    }

    private void updateScreen(int columnIndex) {
        if (columnIndex > 0) {
            isDelSuccess = true;
        }
        queryLndOrSdn();
    }

    private void updateList(Cursor cursor) {
        TextView noText = (TextView) findViewById(R.id.text);
        if (mListType == IccUriUtils.SDN) {
            noText.setText(R.string.sdn_no_contact_number);
        } else {
            noText.setText(R.string.lnd_no_contact_record);
        }

        if (cursor == null) {
            noText.setVisibility(View.GONE);
        } else {
            mAdapter.changeCursor(cursor);
            if (cursor.getCount() == 0) {
                noText.setVisibility(View.VISIBLE);
            } else {
                noText.setVisibility(View.GONE);
            }
        }

        if ((mOperateType == DELETE) && !isDelSuccess) {
            if (mSubId >= 0
                    && TelephonyManager.getDefault().getSimState()
                    != TelephonyManager.SIM_STATE_READY) {
                Toast.makeText(this, R.string.sim_notready_can_not_delete,
                        Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(this, R.string.contact_del_failed, Toast.LENGTH_SHORT).show();
            }
        }
    }

    class MyAdapter extends CursorAdapter {

        LayoutInflater mInflater;

        public MyAdapter(Context context, Cursor c) {
            super(context, c);
            mInflater = (LayoutInflater) LayoutInflater.from(context);
        }

        @Override
        public View newView(Context context, Cursor cursor, ViewGroup parent) {
            Log.d(TAG, "newView()");
            View view = mInflater.inflate(R.layout.lnd_sdn_list_item_ex, null);
            return view;
        }

        @Override
        public void bindView(View view, Context context, Cursor cursor) {
            Log.d(TAG, "bindView()");
            TextView tvName = (TextView) view.findViewById(R.id.text_name);
            TextView tvNumber = (TextView) view.findViewById(R.id.text_number);
            ImageView callIndicator = (ImageView) view.findViewById(R.id.secondary_action_button);

            String name = null;
            String number = null;
            if (cursor != null) {
                name = cursor.getString(cursor.getColumnIndex(NAME));
                number = cursor.getString(cursor.getColumnIndex(NUMBER));
            }
            tvName.setText("" + name);
            tvNumber.setText("" + number);
        }
    }

    @Override
    protected void onDestroy() {
        dismissProgress();
        if (null != mAdapter) {
            mAdapter.changeCursor(null);
        }
        mContentResolver.unregisterContentObserver(mObserver);
        unregisterReceiver(myReceiver);
        super.onDestroy();
    }

    ContentObserver mObserver = new ContentObserver(new Handler()) {
        public void onChange(boolean selfChange) {
            Log.d(TAG, "ContentObserver changed");
            queryLndOrSdn();
        };
    };

    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
        }
        return super.onOptionsItemSelected(item);
    };
}
