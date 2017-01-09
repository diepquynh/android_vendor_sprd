
package com.sprd.contacts.common.list;

import android.accounts.AccountManager;
import android.accounts.Account;
import android.content.Context;
import android.content.ContentResolver;
import android.database.Cursor;
import android.os.AsyncTask;
import android.provider.ContactsContract.RawContacts;
import android.util.AttributeSet;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.android.contacts.common.R;
import com.android.contacts.common.list.ContactListFilter;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.account.AccountType;
import com.android.contacts.common.model.account.AccountWithDataSet;


public class ContactMemoryListView extends LinearLayout {

    private static final String TAG = ContactMemoryListView.class.getSimpleName();

    private ImageView mIcon;
    private TextView mAccountType;
    private TextView mAccountUserName;
    private TextView mSimUsage;
    private ContactListFilter mFilter;
    private boolean mSingleAccount;

    public ContactMemoryListView(Context context) {
        super(context);
    }

    public ContactMemoryListView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void setContactListFilter(ContactListFilter filter) {
        mFilter = filter;
    }

    public void bindView(AccountTypeManager accountTypes) {
        if (mAccountType == null) {
            mIcon = (ImageView) findViewById(R.id.icon);
            mAccountType = (TextView) findViewById(R.id.accountType);
            mAccountUserName = (TextView) findViewById(R.id.accountUserName);
            mSimUsage = (TextView) findViewById(R.id.simUsage);
        }

        if (mFilter == null) {
            mAccountType.setText(R.string.contactsList);
            return;
        }
        if (mFilter.icon != null) {
            mIcon.setImageDrawable(mFilter.icon);
        } else {
            mIcon.setImageResource(R.drawable.unknown_source);
        }
        final AccountType accountType = accountTypes.getAccountType(mFilter.accountType,
                mFilter.dataSet);
        mAccountUserName.setText(accountType.getDisplayName(getContext(),
                new AccountWithDataSet(mFilter.accountName, mFilter.accountType, null)));
        mAccountType.setText(accountType.getDisplayLabel(getContext()));

        mAsyncTask = new AsyncTask<Void, Void, Void>() {

            private int mCapacity;
            private int mVacancies;
            @Override
            protected Void doInBackground(Void... params) {
                mCapacity = getAccountCapacity(getContext(), mFilter);
                mVacancies = getAccountUsage(getContext(), mFilter);
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                if (mSimUsage != null) {
                    mSimUsage.setText(mVacancies + "/" + mCapacity);
                }
                super.onPostExecute(result);
            }
        };
        mAsyncTask.execute();

    }

    private AsyncTask<Void, Void, Void> mAsyncTask;

    private static int getAccountCapacity(Context context, ContactListFilter filter) {
        if (filter.filterType != ContactListFilter.FILTER_TYPE_ACCOUNT) {
            return -1;
        }
        Account account = new Account(filter.accountName, filter.accountType);
        String tmp = AccountManager.get(context).getUserData(account, "capacity");
        if (tmp == null) {
            return -1;
        }
        return Integer.parseInt(tmp);
    }

    private int getAccountUsage(Context context, ContactListFilter filter) {
        ContentResolver cr = context.getContentResolver();
        int providerCapacity = -1;
        Cursor cursor = cr.query(RawContacts.CONTENT_URI.buildUpon()
                .appendQueryParameter(RawContacts.ACCOUNT_NAME, filter.accountName)
                .appendQueryParameter(RawContacts.ACCOUNT_TYPE, filter.accountType)
                .build(),
                null, "deleted=0 and sync1 != 'sdn'", null, null
                );

        if (cursor != null) {
            providerCapacity = cursor.getCount();
            cursor.close();
        }
        return providerCapacity;
    }
}
