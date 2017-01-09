
package com.sprd.contacts.activities;

import android.app.ActionBar;
import android.app.ActionBar.LayoutParams;
import android.app.Activity;
import android.app.Fragment;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Intents.Insert;
import android.provider.ContactsContract.Intents;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.Gravity;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.SearchView;
import android.widget.SearchView.OnCloseListener;
import android.widget.SearchView.OnQueryTextListener;
import com.android.contacts.common.util.AccountFilterUtil;
import android.provider.ContactsContract.Data;
import android.content.ContentValues;
import com.android.contacts.ContactSaveService;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.ValuesDelta;
import com.android.contacts.common.model.RawContactModifier;
import com.android.contacts.common.model.RawContactDelta;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.android.contacts.common.list.ContactListFilter;
import android.graphics.Color;
import android.app.Dialog;
import android.content.DialogInterface;
import android.app.DialogFragment;
import android.app.AlertDialog;
import com.android.contacts.ContactsActivity;
import com.android.contacts.ContactsApplication;
import com.android.contacts.R;
import com.android.contacts.group.GroupEditorFragment;
import com.android.contacts.common.util.Constants;
import com.android.contacts.common.list.ContactEntryListFragment;
import com.android.contacts.list.ContactPickerFragment;
import com.android.contacts.list.ContactsIntentResolver;
import com.android.contacts.list.ContactsRequest;
import com.android.contacts.common.activity.RequestPermissionsActivity;
import com.android.contacts.common.list.DirectoryListLoader;
import com.android.contacts.common.list.ContactListFilterController;
import com.android.contacts.common.list.ContactListFilterController.ContactListFilterListener;
import com.android.contacts.list.EmailAddressPickerFragment;
import com.android.contacts.list.LegacyPhoneNumberPickerFragment;
import com.android.contacts.list.OnContactPickerActionListener;
import com.android.contacts.list.OnEmailAddressPickerActionListener;
import com.android.contacts.common.list.OnPhoneNumberPickerActionListener;
import com.android.contacts.list.OnPostalAddressPickerActionListener;
import com.android.contacts.common.list.PhoneNumberPickerFragment;
import com.android.contacts.list.PostalAddressPickerFragment;
import com.google.common.collect.Sets;
import com.sprd.contacts.list.OnAllInOneDataPickerActionListener;
import com.sprd.contacts.list.AllInOneDataPickerFragment;
import com.sprd.contacts.list.OnAllInOneDataMultiPickerActionListener;
import com.sprd.contacts.list.OnEmailAddressMultiPickerActionListener;
import com.sprd.contacts.common.list.OnPhoneNumberMultiPickerActionListener;
import com.sprd.contacts.list.OnContactMultiPickerActionListener;
import com.sprd.contacts.group.GroupDetailFragmentSprd;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.ArrayList;
import java.util.HashMap;

import com.sprd.contacts.util.AccountsForMimeTypeUtils;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.os.Parcel;
import android.widget.Toast;
import java.util.Arrays;
import android.database.Cursor;
import android.content.ComponentName;

public abstract class ContactSelectionActivitySprd extends ContactsActivity implements
        ContactListFilterController.ContactListFilterListener {
    private static final String TAG = "ContactSelectionActivitySprd";

    private static final int MAX_DATA_SIZE = 500000;

    private BroadcastReceiver mSelecStatusReceiver;

    public static final String MOVE_GROUP_COMPLETE = "move_group_member_completed";
    public static final String FILTER_CHANG = "filter_changed";

    private static final String KEY_FILTER = "mFilter";
    private static final String KEY_FILTER_CHANG = "mIsFilterChanged";

    private static final int SUBACTIVITY_ACCOUNT_FILTER = 6;
    private static final int SUBACTIVITY_BATCH_DELETE = 7;
    private static final int SUBACTIVITY_BATCH_IMPORT = 8;

    private ContactListFilterController mContactListFilterController;
    private ContactListFilter mPermanentAccountFilter = null;
    private ContactListFilter mFilter = null;
    private ContactListFilter mGroupFilter = null;
    private Button mDoneMenuItem;
    private int mDoneMenuDisableColor = Color.WHITE;
    private Long mSelectDataId;
    private int mAccountNum = 0;
    private AccountTypeManager mAccountManager;
    private boolean mIsFilterChanged = false;
    /*
     * Bug507685 If SelectActivity is lunched and contact has no permission,
     * contact can't be selected.
     * @{
     */
    private static boolean mIsNeedPromptPermissionMessage = false;
    /**
     * @}
     */
    private Handler mMainHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mMainHandler = new Handler(Looper.getMainLooper());

        /**
         * Bug507685 If SelectActivity is lunched and contact has no permission,
         * contact can't be selected.
         * @{
         */
        ComponentName callingActivity = getCallingActivity();
        if (callingActivity != null) {
            String className = callingActivity.getShortClassName();
            needPromptMessage(className);
        }

        if (mIsNeedPromptPermissionMessage && callingActivity == null) {
            showToast(R.string.re_add_contact);
            mIsNeedPromptPermissionMessage = false;
            finish();
        }
        /**
         * @}
         */

        if (RequestPermissionsActivity.startPermissionActivity(this)) {
            return;
        }
        /* Bug507685 If it has permission, reset this flag */
        mIsNeedPromptPermissionMessage = false;
        if (savedInstanceState != null) {
            mPermanentAccountFilter = (ContactListFilter) savedInstanceState
                    .getParcelable(KEY_FILTER);
            setupActionListener();
            mIsFilterChanged = savedInstanceState.getBoolean(KEY_FILTER_CHANG);
        }

        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        mContactListFilterController = ContactListFilterController.getInstance(this);
        mContactListFilterController.checkFilterValidity(false);
        mContactListFilterController.addListener(this);
        mFilter = mContactListFilterController.getFilter();

        mAccountManager = AccountTypeManager
                .getInstance(ContactSelectionActivitySprd.this);
        ArrayList<AccountWithDataSet> allAccounts = (ArrayList) mAccountManager
                .getAccounts(false);
        mAccountNum = allAccounts.size();
    }

    /**
     * Bug507685,518216 While ContactSelectionActivity is lunched but contact has no permission,
     * after permission be allowed, data would not be back, and contact can't be selected.
     * Bug515550 Can not add voice mail contact after allowing all
     * the permission of Contacts app.
     * Bug519946 Can not add favorite contacts after allowing all
     * the permission of Contacts app.
     * Bug520031 Can not add group member after allowing all
     * the permission of Contacts app.
     * Bug522267 Dut can not add contact widget after closing phone permission of Contacts app.
     * Bug522245 Dut can not add contact head image after closing phone permission of Contacts app.
     * Bug527159 Can not add vcard from sms app after allowing the permission of Contacts app.
     * Bug 596100 Dut can not add contact attachment after closing phone permission of Contacts app.
     * Bug 596111 Dut can not add contact to voice mail after closing phone permission of Contacts app.
     * Bug 597959 Dut fails to add emergency information after closing permission of Contacts app.
     * @param className the activity name who invoke this SelectionActivity
     */
    private void needPromptMessage(String className) {
        Log.d(TAG, className);
        if (className.endsWith("FastDialSettingActivity")
                || className.endsWith("DocumentsActivity")
                || className.endsWith("BlackCallsListAddActivity")
                || className.endsWith("PeopleActivity")
                || className.endsWith("GroupEditorActivity")
                || className.endsWith(".Launcher")
                || className.endsWith("AttachPhotoActivity")
                || className.endsWith("ConversationActivity")
                || className.endsWith(".smil.ui.SmilMainActivity")
                || className.endsWith(".settings.VoicemailSettingsActivity")
                || className.endsWith(".edit.EditInfoActivity")) {
            mIsNeedPromptPermissionMessage = true;
        }
    }

    /**
     * Shows a toast on the UI thread.
     */
    private void showToast(final int message) {
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(ContactSelectionActivitySprd.this, message, Toast.LENGTH_LONG).show();
            }
        });
    }

    /**
     * SPRD bug547498 During Conference call, after adding invite button, click home button, hang up the call, and open Contacts, show multiTab page
     * @{
     */
    private BroadcastReceiver mHomeKeyEventReceiver = new BroadcastReceiver() {
        String SYSTEM_REASON = "reason";
        String SYSTEM_HOME_KEY = "homekey";

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)) {
                String reason = intent.getStringExtra(SYSTEM_REASON);
                if (TextUtils.equals(reason, SYSTEM_HOME_KEY)) {
                    finish();
                }
            }
        }
    };
    /**
     * @}
     */

    @Override
    protected void onStart() {
        super.onStart();
        /**
         * SPRD bug547498 During Conference call, after adding invite button, click home button, hang up the call, and open Contacts, show multiTab page
         * @{
         */
        registerReceiver(mHomeKeyEventReceiver, new IntentFilter(
                Intent.ACTION_CLOSE_SYSTEM_DIALOGS));
        /**
         * @}
         */
    }

    @Override
    protected void onResume() {
        super.onResume();
        IntentFilter filter = new IntentFilter("com.android.contacts.common.action.SSU");
        mSelecStatusReceiver = new SSUReceiver();
        registerReceiver(mSelecStatusReceiver, filter);
        setDoneMenu(getListFragment().getSelecStatus());

    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(mSelecStatusReceiver);
    }

    @Override
    protected void onStop() {
        super.onStop();
        /**
         * SPRD bug547498 During Conference call, after adding invite button, click home button, hang up the call, and open Contacts, show multiTab page
         * @{
         */
        if (mHomeKeyEventReceiver != null) {
            unregisterReceiver(mHomeKeyEventReceiver);
        }
        /**
         * @}
         */
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putParcelable(KEY_FILTER, mPermanentAccountFilter);
        if (mIsFilterChanged) {
            outState.putBoolean(KEY_FILTER_CHANG, mIsFilterChanged);
        }
    }

    public boolean onPrepareOptionsMenu(Menu menu) {
        // If we want "Create New Contact" button but there's no such a button
        // in the layout,
        // try showing a menu for it.
        /*
           if (!(showCreateNewContactButton() && getCreateNewContactButton() == null)) {
            final MenuItem newContactMenu = menu.findItem(R.id.create_new_contact);
            if (newContactMenu != null) {
                newContactMenu.setVisible(false);
            }
        }
        */
        return true;
    }

    private final class EmailAddressMultiPickerActionListener implements
            OnEmailAddressMultiPickerActionListener {
        public void onPickEmailAddressAction(HashMap<String, String> pairs) {
            returnPickerResult(pairs);
        }

        public void onCancel() {
            ContactSelectionActivitySprd.this.onBackPressed();
        }
    }

    private final class PhoneNumberMultiPickerActionListener implements
            OnPhoneNumberMultiPickerActionListener {
        public void onPickPhoneNumberAction(HashMap<String, String> pairs) {
            returnPickerResult(pairs);
        }

        public void onCancel() {
            ContactSelectionActivitySprd.this.onBackPressed();
        }
    }

    private final class AllInOneDataMultiPickerActionListener implements
            OnAllInOneDataMultiPickerActionListener {
        public void onPickAllInOneDataAction(HashMap<String, String> pairs) {
            returnPickerResult(pairs);
        }

        public void onCancel() {
            ContactSelectionActivitySprd.this.onBackPressed();
        }
    }

    private final class ContactMultiPickerActionListener implements
            OnContactMultiPickerActionListener {
        public void onPickContactAction(ArrayList<String> lookupKeys, ArrayList<String> ids) {
            returnPickerResult(lookupKeys, ids);
        }

        public void onCancel() {
            ContactSelectionActivitySprd.this.onBackPressed();
        }
    }

    private final class AllInOneDataPickerActionListener implements
            OnAllInOneDataPickerActionListener {
        @Override
        public void onPickAllInOneDataAction(Uri dataUri) {
            returnPickerResult(dataUri);
        }

        @Override
        public void onShortcutIntentCreated(Intent intent) {
            returnPickerResult(intent);
        }

        public void onHomeInActionBarSelected() {
            ContactSelectionActivitySprd.this.onBackPressed();
        }
    }

    public void returnPickerResult() {
        Intent intent = new Intent();
        if (mIsFilterChanged) {
            intent.putExtra(FILTER_CHANG, mIsFilterChanged);
        }
        setResult(RESULT_CANCELED, intent);
        finish();
    }

    public void returnPickerResult(ArrayList<String> data, ArrayList<String> data2) {
        Intent intent = getIntent();
        intent.putStringArrayListExtra("result", data);
        intent.putStringArrayListExtra("result_alternative", data2);
        intent.putExtra("filter", mContactListFilterController.getFilter().accountName);
        Parcel parcel = Parcel.obtain();
        intent.writeToParcel(parcel, 0);
        if (Constants.DEBUG) {
            Log.d(TAG, "returnPickerResult parcel size is" + parcel.dataSize());
        }
        if (parcel.dataSize() > MAX_DATA_SIZE) {
            Toast.makeText(ContactSelectionActivitySprd.this, R.string.transaction_too_large,
                    Toast.LENGTH_LONG).show();
            parcel.recycle();
            return;
        }
        if (parcel != null) {
            parcel.recycle();
        }

        returnPickerResult(intent);
    }

    public void returnPickerResult(HashMap<String, String> data) {
        Intent intent = new Intent();
        if (data.isEmpty()) {
            returnPickerResult();
        } else {
            intent.putExtra("result", data);
            returnPickerResult(intent);
        }
    }

    @Override
    public void onContactListFilterChanged() {
        ContactListFilter filter = mContactListFilterController.getFilter();
        ArrayList<AccountWithDataSet> allAccounts = (ArrayList) mAccountManager
                .getAccounts(false);
        if (mPermanentAccountFilter != null && mAccountNum != allAccounts.size()) {
            // if the account information is changed,reconfigure list fragment
            mAccountNum = allAccounts.size();
            configureListFragment();
        }
    }

    @Override
    public void onBackPressed() {
        /**
         * SPRD Bug534556 During monkey test, contacts bring IllegalStateException.
         * @{
         */
        try {
            Intent intent = new Intent();
            if (mIsFilterChanged) {
                intent.putExtra(FILTER_CHANG, mIsFilterChanged);
            }
            setResult(RESULT_CANCELED, intent);
            super.onBackPressed();
        } catch (IllegalStateException e) {
            Log.e(TAG, e.toString());
            finish();
        }
        /**
         * @}
         */
    }

    @Override
    protected void onDestroy() {
        if (mContactListFilterController != null) {
            mContactListFilterController.removeListener(this);
        }
        super.onDestroy();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == SUBACTIVITY_ACCOUNT_FILTER) {
            AccountFilterUtil.handleAccountFilterResult(
                    mContactListFilterController, resultCode, data);
            mIsFilterChanged = true;
        }
    }

    protected void configureActionBar(ActionBar actionBar) {
        View customActionBarView = null;
        LayoutInflater inflater = (LayoutInflater) getSystemService
                (Context.LAYOUT_INFLATER_SERVICE);
        customActionBarView = inflater.inflate(
                R.layout.editor_custom_action_bar_overlay, null);
        mDoneMenuItem = (Button) customActionBarView
                .findViewById(R.id.save_menu_item_button);
        if(getListFragment().isMultiPickerSupported()) {
            mDoneMenuItem
                    .setVisibility(View.VISIBLE);
            mDoneMenuDisableColor = mDoneMenuItem.getCurrentTextColor();
            setDoneMenu(getListFragment().getSelecStatus());
            mDoneMenuItem.setText(R.string.menu_done);
            mDoneMenuItem.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    /*
                     * SPRD:
                     * Bug401353 Click search then back,select contacts deleted done,popup ConfirmBatchDeleteDialog.
                     * @{
                     */
                    if (getIntent().getExtras() != null &&
                            getIntent().getExtras().getInt("mode") == SUBACTIVITY_BATCH_DELETE) {
                        ConfirmBatchDeleteDialogFragment cDialog = new ConfirmBatchDeleteDialogFragment();
                        cDialog.setTargetFragment(getListFragment(), 0);
                        cDialog.show(getFragmentManager(), null);
                    } else {
                        getListFragment().onMultiPickerSelected();
                    }
                    /*
                     * @}
                     */
                }
            });
        } else {
            mDoneMenuItem
            .setVisibility(View.GONE);
        }
        Button cancelMenuItem = (Button) customActionBarView
                .findViewById(R.id.cancel_menu_item_button);
        cancelMenuItem.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                onBackPressed();
            }
        });
        actionBar.setTitle(R.string.contactPickerActivityTitle);
        actionBar.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM
                | ActionBar.DISPLAY_SHOW_TITLE | ActionBar.DISPLAY_HOME_AS_UP);
        actionBar.setCustomView(customActionBarView, new ActionBar.LayoutParams(
                ActionBar.LayoutParams.WRAP_CONTENT,
                ActionBar.LayoutParams.WRAP_CONTENT, Gravity.CENTER_VERTICAL
                        | Gravity.END));
    }

    protected void configureSearchActionBar(ActionBar actionBar) {
        View customActionBarView = null;
        LayoutInflater inflater = (LayoutInflater) getSystemService
                (Context.LAYOUT_INFLATER_SERVICE);
        customActionBarView = inflater.inflate(R.layout.editor_custom_action_bar_overlay, null);
        mDoneMenuItem = (Button) customActionBarView.findViewById(R.id.save_menu_item_button);
        mDoneMenuItem.setVisibility(getListFragment().isMultiPickerSupported() ? View.VISIBLE
                : View.GONE);
        mDoneMenuDisableColor = mDoneMenuItem.getCurrentTextColor();
        setDoneMenu(false);
        mDoneMenuItem.setText(R.string.menu_done);
        mDoneMenuItem.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (getIntent().getExtras() != null &&
                        getIntent().getExtras().getInt("mode") == SUBACTIVITY_BATCH_DELETE) {
                    ConfirmBatchDeleteDialogFragment cDialog = new ConfirmBatchDeleteDialogFragment();
                    cDialog.setTargetFragment(getListFragment(), 0);
                    cDialog.show(getFragmentManager(), null);
                } else {
                    getListFragment().onMultiPickerSelected();
                }
            }
        });
        mDoneMenuItem.setVisibility(getListFragment().isMultiPickerSupported() ? View.VISIBLE
                : View.GONE);
        View cancelMenuItem = customActionBarView.findViewById(R.id.cancel_menu_item_button);
        cancelMenuItem.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                onBackPressed();
            }
        });
        actionBar.setTitle(R.string.contactPickerActivityTitle);
        actionBar.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM | ActionBar.DISPLAY_SHOW_TITLE
                | ActionBar.DISPLAY_HOME_AS_UP);
        actionBar.setCustomView(customActionBarView, new ActionBar.LayoutParams(
                ActionBar.LayoutParams.WRAP_CONTENT,
                ActionBar.LayoutParams.WRAP_CONTENT, Gravity.CENTER_VERTICAL | Gravity.END));

    }

    protected Intent setPickerResultIntent(Intent intent) {
        if (mIsFilterChanged) {
            intent.putExtra(FILTER_CHANG, mIsFilterChanged);
        }
        return intent;
    }

    protected ContactEntryListFragment<?> configListFragment(int actionCode) {
        ContactEntryListFragment<?> listFragment = null;
        AccountWithDataSet account = getIntent().getParcelableExtra("com.android.contacts.extra.ACCOUNT");
        if (account != null) {
            mPermanentAccountFilter = ContactListFilter.createAccountFilter(account.type,
                    account.name, null, null);
        }

        switch (actionCode) {
            case ContactsRequest.ACTION_PICK_OR_CREATE_CONTACT:
            case ContactsRequest.ACTION_INSERT_OR_EDIT_CONTACT: {
                ContactPickerFragment fragment = new ContactPickerFragment();
                if (actionCode == ContactsRequest.ACTION_PICK_OR_CREATE_CONTACT) {
                    fragment.setEditMode(false);
                } else {
                    fragment.setEditMode(true);
                }
                if (mPermanentAccountFilter != null) {
                    fragment.setPermanentFilter(mPermanentAccountFilter);
                }
                // check for mime-type capability
                ContactListFilter filter = getAccountFilterForMimeType(getIntent().getExtras());
                if (filter != null) {
                    if (AccountsForMimeTypeUtils.isAllAccountsForMimeType(
                            ContactSelectionActivitySprd.this, getIntent().getExtras())) {
                        mPermanentAccountFilter = null;
                    } else {
                        mPermanentAccountFilter = filter;
                        fragment.setPermanentFilter(mPermanentAccountFilter);
                    }
                }
                fragment.setDirectorySearchMode(DirectoryListLoader.SEARCH_MODE_NONE);
                fragment.setExcludeReadOnly(true);
                listFragment = fragment;
                break;
            }

            case ContactsRequest.ACTION_PICK_CONTACT: {
                ContactPickerFragment fragment = new ContactPickerFragment();
                fragment.setIncludeProfile(getRequest().shouldIncludeProfile());
                /**
                 * SPRD:Bug535983 The default contact should not be showed when setting head image.
                 * @{
                 */
                ComponentName callingActivity = getCallingActivity();
                if (callingActivity != null) {
                    String className = callingActivity.getShortClassName();
                    if (className != null && className.endsWith("AttachPhotoActivity")) {
                        fragment.setExcludeReadOnly(true);
                    }
                }
                /**
                 * @}
                 */
                if (getIntent().getBooleanExtra("no_sim", false)) {
                    AccountTypeManager am = AccountTypeManager
                            .getInstance(ContactSelectionActivitySprd.this);
                    ArrayList<AccountWithDataSet> allAccounts = (ArrayList) am
                            .getAccounts(false);
                    ArrayList<AccountWithDataSet> accounts = (ArrayList) allAccounts
                            .clone();
                    Iterator<AccountWithDataSet> iter = accounts.iterator();
                    while (iter.hasNext()) {
                        AccountWithDataSet accountWithDataSet = iter.next();
                        if (accountWithDataSet.type.equals("sprd.com.android.account.sim")
                                || accountWithDataSet.type.equals("sprd.com.android.account.usim")) {
                            iter.remove();
                        }
                    }
                    mPermanentAccountFilter = ContactListFilter
                            .createAccountsFilter(accounts);
                    fragment.setPermanentFilter(mPermanentAccountFilter);
                } else {
                    // check for mime-type capability
                    ContactListFilter filter = getAccountFilterForMimeType(getIntent().getExtras());
                    if (filter != null) {
                        if (AccountsForMimeTypeUtils.isAllAccountsForMimeType(
                                ContactSelectionActivitySprd.this, getIntent().getExtras())) {
                            mPermanentAccountFilter = null;
                            fragment.setFilter(mFilter);
                        } else {
                            mPermanentAccountFilter = filter;
                            fragment.setPermanentFilter(mPermanentAccountFilter);
                        }
                    }
                }
                listFragment = fragment;
                break;
            }

            case ContactsRequest.ACTION_CREATE_SHORTCUT_CONTACT: {
                ContactPickerFragment fragment = new ContactPickerFragment(this);
                fragment.setShortcutRequested(true);
                AccountTypeManager am = AccountTypeManager
                        .getInstance(ContactSelectionActivitySprd.this);
                ArrayList<AccountWithDataSet> allAccounts = (ArrayList) am
                        .getAccounts(false);
                ArrayList<AccountWithDataSet> accounts = (ArrayList) allAccounts
                        .clone();
                Iterator<AccountWithDataSet> iter = accounts.iterator();
                while (iter.hasNext()) {
                    AccountWithDataSet accountWithDataSet = iter.next();
                    if (accountWithDataSet.type.equals("sprd.com.android.account.sim")
                            || accountWithDataSet.type.equals("sprd.com.android.account.usim")) {
                        iter.remove();
                    }
                }
                //SPRD: add for bug617830, add fdn feature
                fragment.setListRequestModeSelection("shortcut_contact");
                mPermanentAccountFilter = ContactListFilter
                        .createAccountsFilter(accounts);
                fragment.setPermanentFilter(mPermanentAccountFilter);
                listFragment = fragment;
                break;
            }

            case ContactsRequest.ACTION_MULTI_PICK_CONTACT: {
                ContactPickerFragment fragment = new ContactPickerFragment(this);
                fragment.setMultiPickerSupported(true);
                fragment.setCreateContactEnabled(false);
                fragment.setEditMode(false);
                if(getIntent().hasExtra("mode")) {
                    switch (getIntent().getExtras().getInt("mode")) {
                        case SUBACTIVITY_BATCH_DELETE:
                            fragment.setListRequestModeSelection("mode_delete");
                            break;
                        case GroupEditorFragment.GROUP_MEMBER_SELECT_MODE:
                            fragment.setListRequestModeSelection("mode_group_select");
                            break;
                        case SUBACTIVITY_BATCH_IMPORT:
                            fragment.setListRequestModeSelection("mode_copyto");
                            break;
                        default:
                            break;
                    }
                }
                if (getIntent().hasExtra("src_account")) {
                    account = (AccountWithDataSet) (getIntent().getParcelableExtra("src_account"));
                    mPermanentAccountFilter = ContactListFilter.createAccountFilter(account.type,
                            account.name, null, null);
                    fragment.setAddGroupMemSelection(getIntent().getStringExtra(
                            GroupEditorFragment.CONTACTID_IN_GROUP));
                    fragment.setPermanentFilter(mPermanentAccountFilter);
                } else if (getIntent().hasExtra("dst_account")) {
                    /**
                     * SPRD:Bug 432779 Filter for copy is related to displayed contacts.
                     *
                     * @{
                     */
                    AccountWithDataSet dstAcount = (AccountWithDataSet) getIntent()
                            .getParcelableExtra("dst_account");
                    if (ContactListFilter.FILTER_TYPE_ACCOUNT == mFilter.filterType) {
                        if((mFilter.accountName).equals(dstAcount.name)) {
                            fragment.setPermanentFilter(ContactListFilter.createAccountsFilter(new ArrayList<AccountWithDataSet> ()));
                        } else {
                            fragment.setPermanentFilter(mFilter);
                        }
                    } else if (ContactListFilter.FILTER_TYPE_CUSTOM == mFilter.filterType) {
                        fragment.setPermanentFilter(ContactListFilter.createAccountsFilter(getCustomSelectionDisplayAccounts(dstAcount)));
                    } else {
                        final AccountTypeManager am = AccountTypeManager
                                .getInstance(ContactSelectionActivitySprd.this);
                        final ArrayList<AccountWithDataSet> allAccounts = (ArrayList) am
                                .getAccounts(false);
                        ArrayList<AccountWithDataSet> accounts = (ArrayList) allAccounts
                                .clone();
                        Iterator<AccountWithDataSet> iter = accounts.iterator();
                        while (iter.hasNext()) {
                            final AccountWithDataSet accountWithDataSet = iter.next();
                            if (accountWithDataSet.name.equals(dstAcount.name)) {
                                iter.remove();
                            }
                        }
                        mPermanentAccountFilter = ContactListFilter
                                .createAccountsFilter(accounts);
                        fragment.setPermanentFilter(mPermanentAccountFilter);
                    }
                    /**
                     * @}
                     */
                } else if (getIntent().hasExtra("setMulitStarred")) {
                    AccountTypeManager am = AccountTypeManager
                            .getInstance(ContactSelectionActivitySprd.this);
                    ArrayList<AccountWithDataSet> allAccounts = (ArrayList) am
                            .getAccounts(false);
                    ArrayList<AccountWithDataSet> accounts = (ArrayList) allAccounts
                            .clone();
                    Iterator<AccountWithDataSet> iter = accounts.iterator();
                    while (iter.hasNext()) {
                        AccountWithDataSet accountWithDataSet = iter.next();
                        if (accountWithDataSet.type
                                .equals("sprd.com.android.account.usim")
                                || accountWithDataSet.type
                                        .equals("sprd.com.android.account.sim")) {
                            iter.remove();
                        }
                    }
                    mPermanentAccountFilter = ContactListFilter
                            .createAccountsFilter(accounts);
                    fragment.setStarMemFlag();
                    fragment.setPermanentFilter(mPermanentAccountFilter);
                    //SPRD: add for bug621857, add for fdn feature bugfix
                    fragment.setListRequestModeSelection("star_contact");
                } else if (getIntent().hasExtra("move_group_member")) {
                    Long groupId = (Long) getIntent()
                            .getLongExtra(GroupDetailFragmentSprd.SRC_GROUP_ID, -1);
                    mGroupFilter = ContactListFilter.createGroupFilter(groupId);
                    fragment.setFilter(mGroupFilter);
                } else if (getIntent().hasExtra("delete_group_member")) {
                    Long groupId = (Long) getIntent().getLongExtra("delete_group_member", -1);
                    mGroupFilter = ContactListFilter.createGroupFilter(groupId);
                    fragment.setFilter(mGroupFilter);
                } else if (getIntent().hasExtra(ContactPickerFragment.MMS_MULTI_VCARD)) {
                    fragment.setFilter(mFilter);
                } else {
                    fragment.setFilter(mFilter);
                }
                if (getIntent().hasExtra("exclude_read_only")) {
                    fragment.setExcludeReadOnly(true);
                }
                listFragment = fragment;
                break;
            }

            case ContactsRequest.ACTION_MULTI_PICK_ALL_IN_ONE_DATA: {
                AllInOneDataPickerFragment fragment = new AllInOneDataPickerFragment();
                if (getIntent().hasExtra("select_group_member")) {
                    long groupId = (long) getIntent().getLongExtra("select_group_member", -1);
                    if (getIntent().hasExtra("with_phone_number")) {
                        fragment.setFilter(ContactListFilter.createGroupFilterOnlyPhoneNumber(groupId));
                    } else {
                        fragment.setFilter(ContactListFilter.createGroupFilter(groupId));
                    }
                } else {
                    fragment.setFilter(mFilter);
                }
                fragment.setMultiPickerSupported(true);
                fragment.setCascadingData(getRequest().getCascadingData());
                listFragment = fragment;
                break;
            }

            case ContactsRequest.ACTION_MULTI_PICK_PHONE: {
                PhoneNumberPickerFragment fragment = new PhoneNumberPickerFragment();
                fragment.setMultiPickerSupported(true);
                fragment.setFilter(mFilter);
                listFragment = fragment;
                break;
            }

            case ContactsRequest.ACTION_MULTI_PICK_EMAIL: {
                EmailAddressPickerFragment fragment = new EmailAddressPickerFragment();
                fragment.setMultiPickerSupported(true);
                fragment.setFilter(mFilter);
                listFragment = fragment;
                break;
            }

            default:
                break;
        }
        return listFragment;
    }

    /**
     * SPRD:Bug 432779 Filter for copy is related to displayed contacts.
     *
     * @{
     */
    private ArrayList<AccountWithDataSet> getCustomSelectionDisplayAccounts(AccountWithDataSet dstAcount) {
        ArrayList<AccountWithDataSet> result = null;
        final Uri.Builder contactsUri = Uri.withAppendedPath(ContactsContract.AUTHORITY_URI, "contactscustom").buildUpon();
        final Cursor cursor = getContentResolver().query(contactsUri.build(),
                new String[] {Contacts.DISPLAY_ACCOUNT_NAME,Contacts.DISPLAY_ACCOUNT_TYPE},
                Contacts.IN_VISIBLE_GROUP + "=?",new String[] {"1"},null);
        try {
            if (null != cursor && cursor.moveToFirst()) {
                result = new ArrayList<AccountWithDataSet>();
                do {
                    final AccountWithDataSet dataSet = new AccountWithDataSet(cursor.getString(0), cursor.getString(1), null);
                    if (!dstAcount.equals(dataSet)) {
                        result.add(dataSet);
                    }
                } while (cursor.moveToNext());
            }
        } finally {
            if (cursor != null) cursor.close();
        }
        return result;
    }
    /**
     * @}
     */
    protected void setActivityTitle(int actionCode) {
        switch (actionCode) {
            case ContactsRequest.ACTION_MULTI_PICK_CONTACT: {
                setTitle(R.string.contactPickerActivityTitle);
                break;
            }
            case ContactsRequest.ACTION_MULTI_PICK_PHONE: {
                setTitle(R.string.contactPickerActivityTitle);
                break;
            }
            case ContactsRequest.ACTION_MULTI_PICK_EMAIL: {
                setTitle(R.string.contactPickerActivityTitle);
                break;
            }
            default:
                break;
        }
    }

    public void setupMultiActionListener(ContactEntryListFragment<?> mListFragment) {
        if (mListFragment instanceof ContactPickerFragment) {
            ((ContactPickerFragment) mListFragment).setOnContactMultiPickerActionListener(
                    new ContactMultiPickerActionListener());
        } else if (mListFragment instanceof PhoneNumberPickerFragment) {
            ((PhoneNumberPickerFragment) mListFragment).setOnPhoneNumberMultiPickerActionListener(
                    new PhoneNumberMultiPickerActionListener());
        } else if (mListFragment instanceof EmailAddressPickerFragment) {
            ((EmailAddressPickerFragment) mListFragment)
                    .setOnEmailAddressMultiPickerActionListener(
                    new EmailAddressMultiPickerActionListener());
        } else if (mListFragment instanceof AllInOneDataPickerFragment) {
            ((AllInOneDataPickerFragment) mListFragment)
                    .setOnAllInOneDataMultiPickerActionListener(
                    new AllInOneDataMultiPickerActionListener());
        } else {
            throw new IllegalStateException("Unsupported list fragment type: " + mListFragment);
        }
    }

    public void setupAllInOneActionListener(ContactEntryListFragment<?> mListFragment) {
        if (mListFragment instanceof AllInOneDataPickerFragment) {
            ((AllInOneDataPickerFragment) mListFragment)
                    .setOnAllInOneDataPickerActionListener(new AllInOneDataPickerActionListener());
        }
    }

    private ContactListFilter getAccountFilterForMimeType(Bundle extras) {
        ArrayList<AccountWithDataSet> tmp = AccountsForMimeTypeUtils.getAccountsForMimeType(
                ContactSelectionActivitySprd.this, extras);
        if (tmp.isEmpty()) {
            return null;
        }
        return ContactListFilter.createAccountsFilter(tmp);
    }

    public ContactListFilter getPermanentFilter() {
        return mPermanentAccountFilter;
    }

    public void setDoneMenu(boolean enabled) {
        if (mDoneMenuItem == null) {
            return;
        }
        if (enabled) {
            mDoneMenuItem.setEnabled(true);
            mDoneMenuItem.setTextColor(mDoneMenuDisableColor);
        } else {
            mDoneMenuItem.setEnabled(false);
            mDoneMenuItem.setTextColor(getResources().getColor(
                    R.color.action_bar_button_disable_text_color));
        }
    }

    public class SSUReceiver extends BroadcastReceiver {

        public void onReceive(final Context context, final Intent intent) {
            if (getListFragment() instanceof ContactEntryListFragment<?>) {
                boolean enabled = getListFragment().getSelecStatus();
                setDoneMenu(enabled);
            }

        }
    }

    public static class ConfirmBatchDeleteDialogFragment extends DialogFragment {

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            AlertDialog.Builder builder = new AlertDialog.Builder(this.getActivity())
                    .setTitle(R.string.batch_delete_confim_title)
                    .setMessage(R.string.batch_delete_confim_message)
                    .setNegativeButton(android.R.string.cancel, null)
                    .setPositiveButton(android.R.string.ok,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    ((ContactEntryListFragment) getTargetFragment())
                                            .onMultiPickerSelected();
                                }
                            });
            return builder.create();
        }
    }

    public abstract ContactEntryListFragment<?> getListFragment();

    public abstract ContactsRequest getRequest();

    public abstract void configureListFragment();

    public abstract void setupActionListener();

    public abstract void returnPickerResult(Uri data);

    public abstract void returnPickerResult(Intent intent);
}
