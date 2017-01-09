
package com.sprd.address.suggests.ui;


import java.util.List;

/**
 * SPRD: 523599. Don't like DropdownAccountsFilter, this class don't add username in pubishResults().
 *
 * @param <T>
 */
public class DropdownAddressFilter<T> extends DropdownAccountsFilter<T> {

    public DropdownAddressFilter(DropdownAccountsArrayAdapter<T> referenceAdapter) {
        super(referenceAdapter);
    }

    @Override
    protected void publishResults(CharSequence constraint, FilterResults results) {
        mFilterString = (constraint != null ? constraint.toString() : null);
        // no inspection unchecked
        mObjects = (List<T>) results.values;
        mReferenceAdapter.setObjects(mObjects);
        if (results.count > 0) {
            mReferenceAdapter.notifyDataSetChanged();
        } else {
            mReferenceAdapter.notifyDataSetInvalidated();
        }
    }
}
