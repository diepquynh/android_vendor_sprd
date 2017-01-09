/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.contacts.list;

import com.android.contacts.common.list.ContactListItemView;
import com.android.contacts.common.list.DefaultContactListAdapter;

import android.content.Context;
import android.database.Cursor;
import android.provider.ContactsContract;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.CheckBox;
import java.util.TreeSet;
/**
* sprd Bug494092 join local contacts and SIM contacts, while setting phone of this contact, Contacts app FC.
*
* @{
*/
import java.util.HashMap;
import java.util.Iterator;
/**
* @}
*/

/**
 * An extension of the default contact adapter that adds checkboxes and the ability
 * to select multiple contacts.
 */
public class MultiSelectEntryContactListAdapter extends DefaultContactListAdapter {

    private SelectedContactsListener mSelectedContactsListener;
    private TreeSet<Long> mSelectedContactIds = new TreeSet<Long>();
    /**
    * sprd Bug494092 join local contacts and SIM contacts, while setting phone of this contact, Contacts app FC.
    *
    * @{
    */
    private HashMap<Long,String> mSelectedContactIdAndTypes = new HashMap<Long,String>();
    private HashMap<Long,String> mContactIdAndTypes = new HashMap<Long,String>();
    /**
    * @}
    */
    private boolean mDisplayCheckBoxes;

    public interface SelectedContactsListener {
        void onSelectedContactsChanged();
        void onSelectedContactsChangedViaCheckBox();
    }

    public MultiSelectEntryContactListAdapter(Context context) {
        super(context);
    }

    public void setSelectedContactsListener(SelectedContactsListener listener) {
        mSelectedContactsListener = listener;
    }

    /**
     * Returns set of selected contacts.
     */
    public TreeSet<Long> getSelectedContactIds() {
        return mSelectedContactIds;
    }

    /**
     * sprd Bug494092 Returns map of selected contacts id and type.
     */
    public HashMap<Long,String> getSelectedContactIdAndTypes() {
        Iterator iter = mSelectedContactIds.iterator();
        while (iter.hasNext()) {
           Long contactId = (Long)iter.next();
           if (mSelectedContactIdAndTypes.get(contactId) == null) {
               mSelectedContactIdAndTypes.remove(contactId);
               mSelectedContactIdAndTypes.put(contactId, (String)mContactIdAndTypes.get(contactId));
           }
        }
        return mSelectedContactIdAndTypes;
    }

    /**
     * Update set of selected contacts. This changes which checkboxes are set.
     */
    public void setSelectedContactIds(TreeSet<Long> selectedContactIds) {
        this.mSelectedContactIds = selectedContactIds;
        notifyDataSetChanged();
        if (mSelectedContactsListener != null) {
            mSelectedContactsListener.onSelectedContactsChanged();
        }
    }

    /**
     * sprd Bug494092 Update map of selected contacts id and type.
     */
    public void setSelectedContactIdAndTypes(HashMap<Long,String> selectedContactIdAndTypes) {
        this.mSelectedContactIdAndTypes = selectedContactIdAndTypes;
    }

    /**
     * sprd Bug494092 Update map of all contacts type.
     */
    public void setContactIdAndTypes(HashMap<Long,String> contactIdAndTypes) {
        this.mContactIdAndTypes = contactIdAndTypes;
    }

    /**
     * Shows checkboxes beside contacts if {@param displayCheckBoxes} is {@code TRUE}.
     * Not guaranteed to work with all configurations of this adapter.
     */
    public void setDisplayCheckBoxes(boolean showCheckBoxes) {
        if (!mDisplayCheckBoxes && showCheckBoxes) {
            setSelectedContactIds(new TreeSet<Long>());
            /**
            * sprd Bug494092 join local contacts and SIM contacts, while setting phone of this contact, Contacts app FC.
            *
            * @{
            */
            setSelectedContactIdAndTypes(new HashMap<Long,String>());
            setContactIdAndTypes(new HashMap<Long,String>());
            /**
            * @}
            */
        }
        mDisplayCheckBoxes = showCheckBoxes;
        notifyDataSetChanged();
        if (mSelectedContactsListener != null) {
            mSelectedContactsListener.onSelectedContactsChanged();
        }
    }

    /**
     * Checkboxes are being displayed beside contacts.
     */
    public boolean isDisplayingCheckBoxes() {
        return mDisplayCheckBoxes;
    }

    /**
     * Toggle the checkbox beside the contact for {@param contactId}.
     */
    public void toggleSelectionOfContactId(long contactId) {
        /**
        * sprd Bug494092 join local contacts and SIM contacts, while setting phone of this contact, Contacts app FC.
        *
        * Original Android code:
        if (mSelectedContactIds.contains(contactId)) {
            mSelectedContactIds.remove(contactId);
        } else {
            mSelectedContactIds.add(contactId);
        )
        * @{
        */
        if (mSelectedContactIds.contains(contactId)) {
            mSelectedContactIds.remove(contactId);
            mSelectedContactIdAndTypes.remove(contactId);
        } else {
            String contactType = mContactIdAndTypes.get(contactId);
            mSelectedContactIds.add(contactId);
            mSelectedContactIdAndTypes.put(contactId, contactType);
        }
        /**
        * @}
        */
        notifyDataSetChanged();
        if (mSelectedContactsListener != null) {
            mSelectedContactsListener.onSelectedContactsChanged();
        }
    }

    @Override
    protected void bindView(View itemView, int partition, Cursor cursor, int position) {
        super.bindView(itemView, partition, cursor, position);
        final ContactListItemView view = (ContactListItemView) itemView;
        bindCheckBox(view, cursor, position, partition == ContactsContract.Directory.DEFAULT);
    }

    private void bindCheckBox(ContactListItemView view, Cursor cursor, int position,
            boolean isLocalDirectory) {
        // Disable clicking on the ME profile and all contacts from remote directories
        // when showing check boxes. We do this by telling the view to handle clicking itself.
        view.setClickable((position == 0 && hasProfile() || !isLocalDirectory)
                && mDisplayCheckBoxes);
        // Only show checkboxes if mDisplayCheckBoxes is enabled. Also, never show the
        // checkbox for the Me profile entry.
        /**
         * SPRD: Bug 534672 DUT shares wrong contact while sharing the non-local contact.
         * Android original code:
        if (position == 0 && hasProfile() || !mDisplayCheckBoxes || isContactReadOnly(position, cursor)) {
         * @{
         */

        /**
         * SPRD: Bug596018 after long press one contact, these SIM or Exchange contacts' name and
         * account icon overlapped
         * @{
         */
        if (mDisplayCheckBoxes) {
            view.removeAccountView();
        }
        /**
         * @}
         */

        if (position == 0 && hasProfile() || !mDisplayCheckBoxes
                || isContactReadOnly(position, cursor) || !(isLocalContact(position, cursor))) {
         /*
          * @}
          */
            view.hideCheckBox();
            return;
        }
        final CheckBox checkBox = view.getCheckBox();
        final long contactId = cursor.getLong(ContactQuery.CONTACT_ID);
        /**
        * sprd Bug494092 join local contacts and SIM contacts, while setting phone of this contact, Contacts app FC.
        *
        * @{
        */
        final String contactType = cursor.getString(ContactQuery.CONTACT_DISPLAY_ACCOUNT_TYPE);
        mContactIdAndTypes.put(contactId, contactType);
        /**
        * @}
        */
        checkBox.setChecked(mSelectedContactIds.contains(contactId));
        checkBox.setTag(contactId);
        checkBox.setOnClickListener(mCheckBoxClickListener);
    }

    private final OnClickListener mCheckBoxClickListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            final CheckBox checkBox = (CheckBox) v;
            final Long contactId = (Long) checkBox.getTag();
            /**
            * sprd Bug494092 join local contacts and SIM contacts, while setting phone of this contact, Contacts app FC.
            *
            * Original Android code:
            if (checkBox.isChecked()) {
                mSelectedContactIds.add(contactId);
            } else {
                mSelectedContactIds.remove(contactId);
            }
            * @{
            */
            final String contactType = mContactIdAndTypes.get(contactId);
            if (checkBox.isChecked()) {
                mSelectedContactIds.add(contactId);
                mSelectedContactIdAndTypes.put(contactId, contactType);
            } else {
                mSelectedContactIds.remove(contactId);
                mSelectedContactIdAndTypes.remove(contactId);
            }
            /**
            * @}
            */
            if (mSelectedContactsListener != null) {
                mSelectedContactsListener.onSelectedContactsChangedViaCheckBox();
            }
        }
    };
    /**
     * SPRD:bug501694 add contact_id update
     * @{
        */
      @Override
   public void updateChecked() {
          mSelectedContactIds = super.updateChecked(mSelectedContactIds);
   }
    /**
     * @}
     */
}
