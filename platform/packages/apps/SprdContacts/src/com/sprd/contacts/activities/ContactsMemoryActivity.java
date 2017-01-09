
package com.sprd.contacts.activities;

import android.app.ActionBar;
import android.app.Activity;
import android.content.AsyncTaskLoader;
import android.content.Context;
import android.content.Loader;
import android.app.LoaderManager.LoaderCallbacks;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;
import java.util.ArrayList;
import java.util.List;
import com.android.contacts.common.list.ContactListFilter;
import com.android.contacts.R;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.account.AccountType;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.google.common.collect.Lists;
import com.sprd.contacts.common.list.ContactMemoryListView;

public class ContactsMemoryActivity extends Activity {
    private static final String TAG = "ContactsMemoryActivity";
    private static final int FILTER_LOADER_ID = 0;
    private static final String SIM_ACCOUNT_TYPE = "sprd.com.android.account.sim";
    private static final String USIM_ACCOUNT_TYPE = "sprd.com.android.account.usim";
    private ListView mListView;
    private TextView mEmptyView;

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
        setContentView(R.layout.memory_overlay);
        mListView = (ListView) findViewById(android.R.id.list);
        if (hasSimAccount()) {
            getLoaderManager().initLoader(FILTER_LOADER_ID, null, new MyLoaderCallbacks());
        } else {
            mEmptyView = (TextView) findViewById(R.id.contact_sim_list_empty);
            mEmptyView.setText(getString(R.string.noSimCard));
            mListView.setEmptyView(mEmptyView);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                onBackPressed();
                return true;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    private class MyLoaderCallbacks implements LoaderCallbacks<List<ContactListFilter>> {
        @Override
        public Loader<List<ContactListFilter>> onCreateLoader(int id, Bundle args) {
            return new FilterLoader(ContactsMemoryActivity.this);
        }

        @Override
        public void onLoadFinished(
                Loader<List<ContactListFilter>> loader, List<ContactListFilter> data) {
            if (data == null) { // Just in case...
                Log.e(TAG, "Failed to load filters");
                return;
            }
            mListView.setAdapter(new FilterListAdapter(ContactsMemoryActivity.this, data));
        }

        @Override
        public void onLoaderReset(Loader<List<ContactListFilter>> loader) {
        }
    }

    private static class FilterLoader extends AsyncTaskLoader<List<ContactListFilter>> {
        private Context mContext;

        public FilterLoader(Context context) {
            super(context);
            mContext = context;
        }

        @Override
        public List<ContactListFilter> loadInBackground() {
            return loadAccountFilters(mContext);
        }

        @Override
        protected void onStartLoading() {
            forceLoad();
        }

        @Override
        protected void onStopLoading() {
            cancelLoad();
        }

        @Override
        protected void onReset() {
            onStopLoading();
        }
    }

    private static List<ContactListFilter> loadAccountFilters(Context context) {
        final ArrayList<ContactListFilter> accountFilters = Lists.newArrayList();
        final AccountTypeManager accountTypes = AccountTypeManager.getInstance(context);
        List<AccountWithDataSet> accounts = accountTypes.getAccounts(false);
        for (AccountWithDataSet account : accounts) {
            AccountType accountType = accountTypes.getAccountType(account.type, account.dataSet);
            if (accountType.isExtension() && !account.hasData(context)
                    || !(account.type).equals(SIM_ACCOUNT_TYPE)
                    && !(account.type).equals(USIM_ACCOUNT_TYPE)) {
                // Hide extensions with no raw_contacts or no_sim accounts.
                continue;
            }
            Drawable icon = accountType != null ? accountType.getDisplayIcon(context, account) : null;
            accountFilters.add(ContactListFilter.createAccountFilter(
                    account.type, account.name, account.dataSet, icon));
        }
        return accountFilters;
    }

    private boolean hasSimAccount() {
        final List<AccountWithDataSet> accounts = AccountTypeManager.getInstance(this).
                getAccounts(true);
        boolean ret = false;
        for (AccountWithDataSet account : accounts) {
            if ((account.type).equals(SIM_ACCOUNT_TYPE) || (account.type).equals(USIM_ACCOUNT_TYPE)) {
                ret = true;
            }
        }
        return ret;
    }

    private static class FilterListAdapter extends BaseAdapter {
        private final List<ContactListFilter> mFilters;
        private final LayoutInflater mLayoutInflater;
        private final AccountTypeManager mAccountTypes;

        public FilterListAdapter(Context context, List<ContactListFilter> filters) {
            mLayoutInflater = (LayoutInflater) context.getSystemService
                    (Context.LAYOUT_INFLATER_SERVICE);
            mFilters = filters;
            mAccountTypes = AccountTypeManager.getInstance(context);
        }

        @Override
        public int getCount() {
            return mFilters.size();
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public ContactListFilter getItem(int position) {
            return mFilters.get(position);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            final ContactMemoryListView view;
            if (convertView != null) {
                view = (ContactMemoryListView) convertView;
            } else {
                view = (ContactMemoryListView) mLayoutInflater.inflate(
                        R.layout.contact_memory_list_item, parent, false);
            }
            final ContactListFilter filter = mFilters.get(position);
            view.setContactListFilter(filter);
            view.bindView(mAccountTypes);
            return view;
        }
    }
}
