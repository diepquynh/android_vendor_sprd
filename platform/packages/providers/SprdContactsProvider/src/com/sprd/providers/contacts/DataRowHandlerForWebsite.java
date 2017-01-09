
package com.sprd.providers.contacts;

import com.android.providers.contacts.ContactsDatabaseHelper;
import com.android.providers.contacts.DataRowHandler;
import com.android.providers.contacts.SearchIndexManager.IndexBuilder;
import com.android.providers.contacts.aggregation.AbstractContactAggregator;

import android.content.ContentValues;
import android.content.Context;
import android.provider.ContactsContract.CommonDataKinds.Website;

public class DataRowHandlerForWebsite extends DataRowHandler {

    public DataRowHandlerForWebsite(
            Context context, ContactsDatabaseHelper dbHelper, AbstractContactAggregator aggregator) {
        super(context, dbHelper, aggregator, Website.CONTENT_ITEM_TYPE);
    }

    @Override
    public boolean hasSearchableData() {
        return true;
    }

    @Override
    public boolean containsSearchableColumns(ContentValues values) {
        return values.containsKey(Website.URL);
    }

    @Override
    public void appendSearchableData(IndexBuilder builder) {
        super.appendSearchableData(builder);
        builder.appendContentFromColumn(Website.URL);
    }
}
