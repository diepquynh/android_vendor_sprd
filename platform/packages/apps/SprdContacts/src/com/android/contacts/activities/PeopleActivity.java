/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.contacts.activities;


import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.ActivityNotFoundException;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcelable;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.ProviderStatus;
import android.provider.ContactsContract.QuickContact;
import android.provider.ContactsContract.RawContacts;
import android.provider.Settings;
import android.support.v13.app.FragmentPagerAdapter;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.telecom.TelecomManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.ImageButton;
import android.widget.Toast;
import android.widget.Toolbar;

import com.android.contacts.ContactsActivity;
import com.android.contacts.R;
import com.android.contacts.activities.ActionBarAdapter.TabState;
import com.android.contacts.common.ContactsUtils;
import com.android.contacts.common.activity.RequestPermissionsActivity;
import com.android.contacts.common.compat.TelecomManagerUtil;
import com.android.contacts.common.compat.BlockedNumberContractCompat;
import com.android.contacts.common.dialog.ClearFrequentsDialog;
import com.android.contacts.common.interactions.ImportExportDialogFragment;
import com.android.contacts.common.list.ContactEntryListFragment;
import com.android.contacts.common.list.ContactListFilter;
import com.android.contacts.common.list.ContactListFilterController;
import com.android.contacts.common.list.ContactTileAdapter.DisplayType;
import com.android.contacts.common.list.DirectoryListLoader;
import com.android.contacts.common.list.ViewPagerTabs;
import com.android.contacts.common.logging.Logger;
import com.android.contacts.common.logging.ScreenEvent.ScreenType;
import com.android.contacts.common.preference.ContactsPreferenceActivity;
import com.android.contacts.common.util.AccountFilterUtil;
import com.android.contacts.common.util.Constants;
import com.android.contacts.common.util.ImplicitIntentsUtil;
import com.android.contacts.common.util.ViewUtil;
import com.android.contacts.common.widget.FloatingActionButtonController;
import com.android.contacts.editor.EditorIntents;
import com.android.contacts.interactions.ContactDeletionInteraction;
import com.android.contacts.interactions.ContactMultiDeletionInteraction;
import com.android.contacts.interactions.ContactMultiDeletionInteraction.MultiContactDeleteListener;
import com.android.contacts.interactions.JoinContactsDialogFragment;
import com.android.contacts.interactions.JoinContactsDialogFragment.JoinContactsListener;
import com.android.contacts.list.ContactTileListFragment;
import com.android.contacts.list.ContactsIntentResolver;
import com.android.contacts.list.ContactsRequest;
import com.android.contacts.list.ContactsUnavailableFragment;
import com.android.contacts.list.MultiSelectContactsListFragment;
import com.android.contacts.list.MultiSelectContactsListFragment.OnCheckBoxListActionListener;
import com.android.contacts.list.OnContactBrowserActionListener;
import com.android.contacts.list.OnContactsUnavailableActionListener;
import com.android.contacts.list.ProviderStatusWatcher;
import com.android.contacts.list.ProviderStatusWatcher.ProviderStatusListener;
import com.android.contacts.quickcontact.QuickContactActivity;
import com.android.contacts.util.DialogManager;
import com.android.contacts.util.PhoneCapabilityTester;
import com.android.contactsbind.HelpUtils;

import java.util.List;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicInteger;
import android.os.SystemProperties;
/**
* SPRD:
* @{
*/
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.database.Cursor;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.view.SubMenu;
import com.android.contacts.ContactsApplication;
import com.android.contacts.ContactSaveService;
import com.android.contacts.common.editor.SelectAccountDialogFragment;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.util.AccountsListAdapter.AccountListFilter;
import com.android.contacts.common.util.AccountSelectionUtil;
import com.android.contacts.common.vcard.ExportVCardActivity;
import com.android.contacts.common.vcard.VCardCommonArguments;
import com.android.contacts.list.UiIntentActions;
import com.sprd.contacts.BatchOperationService;
import com.sprd.contacts.activities.ContactsMemoryActivity;
import com.sprd.contacts.activities.ContactDeduplicationActivity;
import com.sprd.contacts.common.interactions.ImportExportDialogFragmentSprd;
import com.sprd.contacts.common.model.account.SimAccountType;
import com.sprd.contacts.common.model.account.USimAccountType;
import com.sprd.contacts.common.model.account.CardDavAccountType;
import com.sprd.contacts.activities.GroupBrowseListActivity;
import android.os.Parcel;
import java.util.ArrayList;
import java.util.Map.Entry;
import java.util.HashMap;
import java.util.Iterator;
import com.android.contacts.common.dialog.IndeterminateProgressDialog;
/**
* @}
*/
/**
 * Displays a list to browse contacts.
 */
/**
 * SPRD:Bug474746 Import/Export SdCard vcf contacts.
 *
 * Original Android code:
public class PeopleActivity extends ContactsActivity implements
        View.OnCreateContextMenuListener,
        View.OnClickListener,
        ActionBarAdapter.Listener,
        DialogManager.DialogShowingViewActivity,
        ContactListFilterController.ContactListFilterListener,
        ProviderStatusListener,
        MultiContactDeleteListener,
        JoinContactsListener {
 * @{
 */
public class PeopleActivity extends ContactsActivity implements
        View.OnCreateContextMenuListener,
        View.OnClickListener,
        ActionBarAdapter.Listener,
        DialogManager.DialogShowingViewActivity,
        ContactListFilterController.ContactListFilterListener,
        ProviderStatusListener,
        MultiContactDeleteListener,
        JoinContactsListener,
        ImportExportDialogFragmentSprd.Listener,
        SelectAccountDialogFragment.Listener,
        ContactSaveService.JoinContactListener{
/**
 * @}
 */
    private static final String TAG = "PeopleActivity";

    private static final String ENABLE_DEBUG_OPTIONS_HIDDEN_CODE = "debug debug!";

    // These values needs to start at 2. See {@link ContactEntryListFragment}.
    private static final int SUBACTIVITY_ACCOUNT_FILTER = 2;

    private final DialogManager mDialogManager = new DialogManager(this);

    private ContactsIntentResolver mIntentResolver;
    private ContactsRequest mRequest;

    private ActionBarAdapter mActionBarAdapter;
    private FloatingActionButtonController mFloatingActionButtonController;
    private View mFloatingActionButtonContainer;
    private boolean wasLastFabAnimationScaleIn = false;

    private ContactTileListFragment.Listener mFavoritesFragmentListener =
            new StrequentContactListFragmentListener();

    private ContactListFilterController mContactListFilterController;

    private ContactsUnavailableFragment mContactsUnavailableFragment;
    private ProviderStatusWatcher mProviderStatusWatcher;
    private Integer mProviderStatus;

    private boolean mOptionsMenuContactsAvailable;

    /**
     * Showing a list of Contacts. Also used for showing search results in search mode.
     */
    private MultiSelectContactsListFragment mAllFragment;
    private ContactTileListFragment mFavoritesFragment;

    /** ViewPager for swipe */
    private ViewPager mTabPager;
    private ViewPagerTabs mViewPagerTabs;
    private TabPagerAdapter mTabPagerAdapter;
    private String[] mTabTitles;
    private final TabPagerListener mTabPagerListener = new TabPagerListener();

    private boolean mEnableDebugMenuOptions;

    /**
     * True if this activity instance is a re-created one.  i.e. set true after orientation change.
     * This is set in {@link #onCreate} for later use in {@link #onStart}.
     */
    private boolean mIsRecreatedInstance;

    /**
     * If {@link #configureFragments(boolean)} is already called.  Used to avoid calling it twice
     * in {@link #onStart}.
     * (This initialization only needs to be done once in onStart() when the Activity was just
     * created from scratch -- i.e. onCreate() was just called)
     */
    private boolean mFragmentInitialized;

    /**
     * This is to disable {@link #onOptionsItemSelected} when we trying to stop the activity.
     */
    private boolean mDisableOptionItemSelected;

    /** Sequential ID assigned to each instance; used for logging */
    private final int mInstanceId;
    private static final AtomicInteger sNextInstanceId = new AtomicInteger();

    // SPRD:Bug 562135 change the limited number of sending by message to 10
    private static final int MAX_CONTACTS_NUMBER = 10;

    //SPRD: add for bug615040, limit the join contacts to 10.
    private static final int MAX_JOIN_CONTACTS_NUMBER = 10;

    /**
     * SPRD:Bug494541 After arranging contacts, contact list should be reload again.
     * @{
     */
    public boolean mDeduplication = false;
    /**
     * @}
     */
    private SubMenu mAdvancedOptions = null; // Add for SPRD:615630
    /**
     * Bug 616293 add warn location express when enter into contacts
     */
    private String mSecureCheck;
    public static PeopleActivity instance = null;
    private String resultString = "";
    private static final String SECURESUPPORT_DEFAULT = "0";
    private static final String SECURESUPPORT = "1";
    private static final String RESULT = "result";
    private static final String SECUREPROPERTY = "persist.support.securetest";
    /**
     * @}
     */
    public PeopleActivity() {
        mInstanceId = sNextInstanceId.getAndIncrement();
        mIntentResolver = new ContactsIntentResolver(this);
        mProviderStatusWatcher = ProviderStatusWatcher.getInstance(this);
    }

    @Override
    public String toString() {
        // Shown on logcat
        return String.format("%s@%d", getClass().getSimpleName(), mInstanceId);
    }

    public boolean areContactsAvailable() {
        /**
         * SPRD: add for sync sim, bug421244
         *
         * Original Android code:
        return (mProviderStatus != null)
                && mProviderStatus.equals(ProviderStatus.STATUS_NORMAL);
         * @{
         */
       return (mProviderStatus != null)
               && (mProviderStatus.equals(ProviderStatus.STATUS_NORMAL) || mProviderStatus.equals(ProviderStatus.STATUS_IMPORTING));
       /**
        * @}
        */
    }
    private boolean areGroupWritableAccountsAvailable() {
        return ContactsUtils.areGroupWritableAccountsAvailable(this);
    }

    /**
     * Initialize fragments that are (or may not be) in the layout.
     *
     * For the fragments that are in the layout, we initialize them in
     * {@link #createViewsAndFragments(Bundle)} after inflating the layout.
     *
     * However, the {@link ContactsUnavailableFragment} is a special fragment which may not
     * be in the layout, so we have to do the initialization here.
     *
     * The ContactsUnavailableFragment is always created at runtime.
     */
    @Override
    public void onAttachFragment(Fragment fragment) {
        if (fragment instanceof ContactsUnavailableFragment) {
            mContactsUnavailableFragment = (ContactsUnavailableFragment)fragment;
            mContactsUnavailableFragment.setOnContactsUnavailableActionListener(
                    new ContactsUnavailableFragmentListener());
        }
    }

    @Override
    protected void onCreate(Bundle savedState) {
        if (Log.isLoggable(Constants.PERFORMANCE_TAG, Log.DEBUG)) {
            Log.d(Constants.PERFORMANCE_TAG, "PeopleActivity.onCreate start");
        }
        super.onCreate(savedState);

        /**
         * Bug 616293 add warn location express when enter into contacts
         */
        mSecureCheck = SystemProperties.get(SECUREPROPERTY, SECURESUPPORT_DEFAULT);
        Intent contactsIntent = getIntent();
        Bundle bundle = (contactsIntent!=null)?contactsIntent.getExtras():null;
        if(SECURESUPPORT.equals(mSecureCheck)){
           resultString = (bundle!=null)?bundle.getString(RESULT):null;
           if(resultString == null){
               instance = this;
               Intent accessIntent = new Intent(this, SecurateAccessContacts.class);
               startActivity(accessIntent);
               finish();
               return;
           }
        }
        /**
         * @}
         */
        if (RequestPermissionsActivity.startPermissionActivity(this)) {
            return;
        }

        if (!processIntent(false)) {
            finish();
            return;
        }
        mContactListFilterController = ContactListFilterController.getInstance(this);
        mContactListFilterController.checkFilterValidity(false);
        mContactListFilterController.addListener(this);

        mProviderStatusWatcher.addListener(this);

        mIsRecreatedInstance = (savedState != null);
        createViewsAndFragments(savedState);

        if (Log.isLoggable(Constants.PERFORMANCE_TAG, Log.DEBUG)) {
            Log.d(Constants.PERFORMANCE_TAG, "PeopleActivity.onCreate finish");
        }
        getWindow().setBackgroundDrawable(null);
        /**
         * SPRD:Bug494541 After arranging contacts, contact list should be reload again.
         * @{
         */
        mDeduplication = false;
        /**
         * @}
         */
                /**
                 * SPRD:513110,517837 Contact list disapper while rotate during join contact
                 * {
                 */
                ContactSaveService.setJoinContactListener(this);
                /**
                 * @}
                 */
    }

    @Override
    protected void onNewIntent(Intent intent) {
        setIntent(intent);
        if (!processIntent(true)) {
            finish();
            return;
        }
         /**
         * SPRD:Bug511308  mActionBarAdapter is  null in monkey
         *
         * @{
         */
         if (mActionBarAdapter == null) {
            if (PeopleActivity.this != null) {
                finish();
            }
            return;
        }
         /**
         * @}
         */
        mActionBarAdapter.initialize(null, mRequest);
        mContactListFilterController.checkFilterValidity(false);

        // Re-configure fragments.
        configureFragments(true /* from request */);
        initializeFabVisibility();
        invalidateOptionsMenuIfNeeded();
    }

    /**
     * Resolve the intent and initialize {@link #mRequest}, and launch another activity if redirect
     * is needed.
     *
     * @param forNewIntent set true if it's called from {@link #onNewIntent(Intent)}.
     * @return {@code true} if {@link PeopleActivity} should continue running.  {@code false}
     *         if it shouldn't, in which case the caller should finish() itself and shouldn't do
     *         farther initialization.
     */
    private boolean processIntent(boolean forNewIntent) {
        // Extract relevant information from the intent
        mRequest = mIntentResolver.resolveIntent(getIntent());
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, this + " processIntent: forNewIntent=" + forNewIntent
                    + " intent=" + getIntent() + " request=" + mRequest);
        }
        if (!mRequest.isValid()) {
            setResult(RESULT_CANCELED);
            return false;
        }

        if (mRequest.getActionCode() == ContactsRequest.ACTION_VIEW_CONTACT) {
            final Intent intent = ImplicitIntentsUtil.composeQuickContactIntent(
                    mRequest.getContactUri(), QuickContactActivity.MODE_FULLY_EXPANDED);
            intent.putExtra(QuickContactActivity.EXTRA_PREVIOUS_SCREEN_TYPE, ScreenType.UNKNOWN);
            /**
             * SPRD557932 Can't find the activity to handle the intent.
             * @{
             */
            ImplicitIntentsUtil.startActivityInAppIfPossible(this, intent);
            /**
             * @}
             */
            return false;
        }
        return true;
    }

    private void createViewsAndFragments(Bundle savedState) {
        // Disable the ActionBar so that we can use a Toolbar. This needs to be called before
        // setContentView().
        getWindow().requestFeature(Window.FEATURE_NO_TITLE);

        setContentView(R.layout.people_activity);

        final FragmentManager fragmentManager = getFragmentManager();

        // Hide all tabs (the current tab will later be reshown once a tab is selected)
        final FragmentTransaction transaction = fragmentManager.beginTransaction();

        mTabTitles = new String[TabState.COUNT];
        mTabTitles[TabState.FAVORITES] = getString(R.string.favorites_tab_label);
        mTabTitles[TabState.ALL] = getString(R.string.all_contacts_tab_label);
        mTabPager = getView(R.id.tab_pager);
        mTabPagerAdapter = new TabPagerAdapter();
        mTabPager.setAdapter(mTabPagerAdapter);
        mTabPager.setOnPageChangeListener(mTabPagerListener);

        // Configure toolbar and toolbar tabs. If in landscape mode, we  configure tabs differntly.
        final Toolbar toolbar = getView(R.id.toolbar);
        setActionBar(toolbar);
        final ViewPagerTabs portraitViewPagerTabs
                = (ViewPagerTabs) findViewById(R.id.lists_pager_header);
        ViewPagerTabs landscapeViewPagerTabs = null;
        if (portraitViewPagerTabs ==  null) {
            landscapeViewPagerTabs = (ViewPagerTabs) getLayoutInflater().inflate(
                    R.layout.people_activity_tabs_lands, toolbar, /* attachToRoot = */ false);
            mViewPagerTabs = landscapeViewPagerTabs;
        } else {
            mViewPagerTabs = portraitViewPagerTabs;
        }
        mViewPagerTabs.setViewPager(mTabPager);

        final String FAVORITE_TAG = "tab-pager-favorite";
        final String ALL_TAG = "tab-pager-all";

        // Create the fragments and add as children of the view pager.
        // The pager adapter will only change the visibility; it'll never create/destroy
        // fragments.
        // However, if it's after screen rotation, the fragments have been re-created by
        // the fragment manager, so first see if there're already the target fragments
        // existing.
        mFavoritesFragment = (ContactTileListFragment)
                fragmentManager.findFragmentByTag(FAVORITE_TAG);
        mAllFragment = (MultiSelectContactsListFragment)
                fragmentManager.findFragmentByTag(ALL_TAG);

        if (mFavoritesFragment == null) {
            mFavoritesFragment = new ContactTileListFragment();
            mAllFragment = new MultiSelectContactsListFragment();

            transaction.add(R.id.tab_pager, mFavoritesFragment, FAVORITE_TAG);
            transaction.add(R.id.tab_pager, mAllFragment, ALL_TAG);
        }

        mFavoritesFragment.setListener(mFavoritesFragmentListener);

        mAllFragment.setOnContactListActionListener(new ContactBrowserActionListener());
        mAllFragment.setCheckBoxListListener(new CheckBoxListListener());

        // Hide all fragments for now.  We adjust visibility when we get onSelectedTabChanged()
        // from ActionBarAdapter.
        transaction.hide(mFavoritesFragment);
        transaction.hide(mAllFragment);

        transaction.commitAllowingStateLoss();
        fragmentManager.executePendingTransactions();

        // Setting Properties after fragment is created
        mFavoritesFragment.setDisplayType(DisplayType.STREQUENT);

        mActionBarAdapter = new ActionBarAdapter(this, this, getActionBar(),
                portraitViewPagerTabs, landscapeViewPagerTabs, toolbar);
        mActionBarAdapter.initialize(savedState, mRequest);

        // Add shadow under toolbar
        ViewUtil.addRectangularOutlineProvider(findViewById(R.id.toolbar_parent), getResources());

        // Configure floating action button
        mFloatingActionButtonContainer = findViewById(R.id.floating_action_button_container);
        final ImageButton floatingActionButton
                = (ImageButton) findViewById(R.id.floating_action_button);
        floatingActionButton.setOnClickListener(this);
        mFloatingActionButtonController = new FloatingActionButtonController(this,
                mFloatingActionButtonContainer, floatingActionButton);
        initializeFabVisibility();

        invalidateOptionsMenuIfNeeded();
    }

    @Override
    protected void onStart() {
        if (!mFragmentInitialized) {
            mFragmentInitialized = true;
            /* Configure fragments if we haven't.
             *
             * Note it's a one-shot initialization, so we want to do this in {@link #onCreate}.
             *
             * However, because this method may indirectly touch views in fragments but fragments
             * created in {@link #configureContentView} using a {@link FragmentTransaction} will NOT
             * have views until {@link Activity#onCreate} finishes (they would if they were inflated
             * from a layout), we need to do it here in {@link #onStart()}.
             *
             * (When {@link Fragment#onCreateView} is called is different in the former case and
             * in the latter case, unfortunately.)
             *
             * Also, we skip most of the work in it if the activity is a re-created one.
             * (so the argument.)
             */
            configureFragments(!mIsRecreatedInstance);
        }
        super.onStart();
    }

    @Override
    protected void onPause() {
        mOptionsMenuContactsAvailable = false;
        mProviderStatusWatcher.stop();
      //SPRD:615630 SunMenu invalid when split screen begin
        if(mAdvancedOptions != null){
            mAdvancedOptions.close();
        }
      //SPRD:615630 SunMenu invalid when split screen end
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();

        mProviderStatusWatcher.start();
        updateViewConfiguration(true);

        // Re-register the listener, which may have been cleared when onSaveInstanceState was
        // called.  See also: onSaveInstanceState
        mActionBarAdapter.setListener(this);
        mDisableOptionItemSelected = false;
        if (mTabPager != null) {
            mTabPager.setOnPageChangeListener(mTabPagerListener);
        }
        // Current tab may have changed since the last onSaveInstanceState().  Make sure
        // the actual contents match the tab.
        updateFragmentsVisibility();
    }

    @Override
    protected void onDestroy() {
        mProviderStatusWatcher.removeListener(this);

        // Some of variables will be null if this Activity redirects Intent.
        // See also onCreate() or other methods called during the Activity's initialization.
        if (mActionBarAdapter != null) {
            mActionBarAdapter.setListener(null);
        }
        if (mContactListFilterController != null) {
            mContactListFilterController.removeListener(this);
        }
        /**
         * SPRD:517837 join contacts and rotate the phone,contact FC
         * @{
         */
        ContactSaveService.setJoinContactListener(null);
        /**
         * @}
         */
        super.onDestroy();
    }

    private void configureFragments(boolean fromRequest) {
        if (fromRequest) {
            ContactListFilter filter = null;
            /**
             * SPRD:626541 it occured crash low probably when make import-export contacts
             * @{
             */
            if(mRequest == null){
               return;
            }
            /**
             * @}
             */
            int actionCode = mRequest.getActionCode();
            boolean searchMode = mRequest.isSearchMode();
            final int tabToOpen;
            switch (actionCode) {
                case ContactsRequest.ACTION_ALL_CONTACTS:
                    filter = ContactListFilter.createFilterWithType(
                            ContactListFilter.FILTER_TYPE_ALL_ACCOUNTS);
                    tabToOpen = TabState.ALL;
                    break;
                case ContactsRequest.ACTION_CONTACTS_WITH_PHONES:
                    filter = ContactListFilter.createFilterWithType(
                            ContactListFilter.FILTER_TYPE_WITH_PHONE_NUMBERS_ONLY);
                    tabToOpen = TabState.ALL;
                    break;

                case ContactsRequest.ACTION_FREQUENT:
                case ContactsRequest.ACTION_STREQUENT:
                case ContactsRequest.ACTION_STARRED:
                    tabToOpen = TabState.FAVORITES;
                    break;
                case ContactsRequest.ACTION_VIEW_CONTACT:
                    tabToOpen = TabState.ALL;
                    break;
                default:
                    tabToOpen = -1;
                    break;
            }
            if (tabToOpen != -1) {
                mActionBarAdapter.setCurrentTab(tabToOpen);
            }

            if (filter != null) {
                mContactListFilterController.setContactListFilter(filter, false);
                searchMode = false;
            }

            if (mRequest.getContactUri() != null) {
                searchMode = false;
            }

            mActionBarAdapter.setSearchMode(searchMode);
            configureContactListFragmentForRequest();
        }

        configureContactListFragment();

        invalidateOptionsMenuIfNeeded();
    }

    private void initializeFabVisibility() {
        final boolean hideFab = mActionBarAdapter.isSearchMode()
                || mActionBarAdapter.isSelectionMode();
        mFloatingActionButtonContainer.setVisibility(hideFab ? View.GONE : View.VISIBLE);
        mFloatingActionButtonController.resetIn();
        wasLastFabAnimationScaleIn = !hideFab;
    }

    private void showFabWithAnimation(boolean showFab) {
        if (mFloatingActionButtonContainer == null) {
            return;
        }
        if (showFab) {
            if (!wasLastFabAnimationScaleIn) {
                mFloatingActionButtonContainer.setVisibility(View.VISIBLE);
                mFloatingActionButtonController.scaleIn(0);
            }
            wasLastFabAnimationScaleIn = true;

        } else {
            if (wasLastFabAnimationScaleIn) {
                mFloatingActionButtonContainer.setVisibility(View.VISIBLE);
                mFloatingActionButtonController.scaleOut();
            }
            wasLastFabAnimationScaleIn = false;
        }
    }

    @Override
    public void onContactListFilterChanged() {
        if (mAllFragment == null || !mAllFragment.isAdded()) {
            return;
        }

        mAllFragment.setFilter(mContactListFilterController.getFilter());

        invalidateOptionsMenuIfNeeded();
    }

    /**
     * Handler for action bar actions.
     */
    @Override
    public void onAction(int action) {
        switch (action) {
            case ActionBarAdapter.Listener.Action.START_SELECTION_MODE:
                mAllFragment.displayCheckBoxes(true);
                startSearchOrSelectionMode();
                break;
            case ActionBarAdapter.Listener.Action.START_SEARCH_MODE:
                if (!mIsRecreatedInstance) {
                    Logger.logScreenView(this, ScreenType.SEARCH);
                }
                startSearchOrSelectionMode();
                break;
            case ActionBarAdapter.Listener.Action.BEGIN_STOPPING_SEARCH_AND_SELECTION_MODE:
                showFabWithAnimation(/* showFabWithAnimation = */ true);
                break;
            case ActionBarAdapter.Listener.Action.STOP_SEARCH_AND_SELECTION_MODE:
                setQueryTextToFragment("");
                updateFragmentsVisibility();
                invalidateOptionsMenu();
                showFabWithAnimation(/* showFabWithAnimation = */ true);
                break;
            case ActionBarAdapter.Listener.Action.CHANGE_SEARCH_QUERY:
                final String queryString = mActionBarAdapter.getQueryString();
                setQueryTextToFragment(queryString);
                updateDebugOptionsVisibility(
                        ENABLE_DEBUG_OPTIONS_HIDDEN_CODE.equals(queryString));
                break;
            default:
                throw new IllegalStateException("Unkonwn ActionBarAdapter action: " + action);
        }
    }

    private void startSearchOrSelectionMode() {
        configureFragments(false /* from request */);
        updateFragmentsVisibility();
        invalidateOptionsMenu();
        showFabWithAnimation(/* showFabWithAnimation = */ false);
    }

    @Override
    public void onSelectedTabChanged() {
        updateFragmentsVisibility();
    }

    @Override
    public void onUpButtonPressed() {
        onBackPressed();
    }

    private void updateDebugOptionsVisibility(boolean visible) {
        if (mEnableDebugMenuOptions != visible) {
            mEnableDebugMenuOptions = visible;
            invalidateOptionsMenu();
        }
    }

    /**
     * Updates the fragment/view visibility according to the current mode, such as
     * {@link ActionBarAdapter#isSearchMode()} and {@link ActionBarAdapter#getCurrentTab()}.
     */
    private void updateFragmentsVisibility() {
        int tab = mActionBarAdapter.getCurrentTab();

        if (mActionBarAdapter.isSearchMode() || mActionBarAdapter.isSelectionMode()) {
            mTabPagerAdapter.setTabsHidden(true);
        } else {
            // No smooth scrolling if quitting from the search/selection mode.
            final boolean wereTabsHidden = mTabPagerAdapter.areTabsHidden()
                    || mActionBarAdapter.isSelectionMode();
            mTabPagerAdapter.setTabsHidden(false);
            if (mTabPager.getCurrentItem() != tab) {
                mTabPager.setCurrentItem(tab, !wereTabsHidden);
            }
        }
        if (!mActionBarAdapter.isSelectionMode()) {
            mAllFragment.displayCheckBoxes(false);
        }
        invalidateOptionsMenu();
        showEmptyStateForTab(tab);
    }

    private void showEmptyStateForTab(int tab) {
        if (mContactsUnavailableFragment != null) {
            switch (getTabPositionForTextDirection(tab)) {
                case TabState.FAVORITES:
                    mContactsUnavailableFragment.setTabInfo(
                            R.string.listTotalAllContactsZeroStarred, TabState.FAVORITES);
                    break;
                case TabState.ALL:
                    mContactsUnavailableFragment.setTabInfo(R.string.noContacts, TabState.ALL);
                    break;
            }
            // When using the mContactsUnavailableFragment the ViewPager doesn't contain two views.
            // Therefore, we have to trick the ViewPagerTabs into thinking we have changed tabs
            // when the mContactsUnavailableFragment changes. Otherwise the tab strip won't move.
            mViewPagerTabs.onPageScrolled(tab, 0, 0);
        }
    }

    private class TabPagerListener implements ViewPager.OnPageChangeListener {

        // This package-protected constructor is here because of a possible compiler bug.
        // PeopleActivity$1.class should be generated due to the private outer/inner class access
        // needed here.  But for some reason, PeopleActivity$1.class is missing.
        // Since $1 class is needed as a jvm work around to get access to the inner class,
        // changing the constructor to package-protected or public will solve the problem.
        // To verify whether $1 class is needed, javap PeopleActivity$TabPagerListener and look for
        // references to PeopleActivity$1.
        //
        // When the constructor is private and PeopleActivity$1.class is missing, proguard will
        // correctly catch this and throw warnings and error out the build on user/userdebug builds.
        //
        // All private inner classes below also need this fix.
        TabPagerListener() {}

        @Override
        public void onPageScrollStateChanged(int state) {
            if (!mTabPagerAdapter.areTabsHidden()) {
                mViewPagerTabs.onPageScrollStateChanged(state);
            }
        }

        @Override
        public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
            if (!mTabPagerAdapter.areTabsHidden()) {
                mViewPagerTabs.onPageScrolled(position, positionOffset, positionOffsetPixels);
            }
        }

        @Override
        public void onPageSelected(int position) {
            // Make sure not in the search mode, in which case position != TabState.ordinal().
            if (!mTabPagerAdapter.areTabsHidden()) {
                mActionBarAdapter.setCurrentTab(position, false);
                mViewPagerTabs.onPageSelected(position);
                showEmptyStateForTab(position);
                invalidateOptionsMenu();
            }
        }
    }

    /**
     * Adapter for the {@link ViewPager}.  Unlike {@link FragmentPagerAdapter},
     * {@link #instantiateItem} returns existing fragments, and {@link #instantiateItem}/
     * {@link #destroyItem} show/hide fragments instead of attaching/detaching.
     *
     * In search mode, we always show the "all" fragment, and disable the swipe.  We change the
     * number of items to 1 to disable the swipe.
     *
     * TODO figure out a more straight way to disable swipe.
     */
    private class TabPagerAdapter extends PagerAdapter {
        private final FragmentManager mFragmentManager;
        private FragmentTransaction mCurTransaction = null;

        private boolean mAreTabsHiddenInTabPager;

        private Fragment mCurrentPrimaryItem;

        public TabPagerAdapter() {
            mFragmentManager = getFragmentManager();
        }

        public boolean areTabsHidden() {
            return mAreTabsHiddenInTabPager;
        }

        public void setTabsHidden(boolean hideTabs) {
            if (hideTabs == mAreTabsHiddenInTabPager) {
                return;
            }
            mAreTabsHiddenInTabPager = hideTabs;
            notifyDataSetChanged();
        }

        @Override
        public int getCount() {
            return mAreTabsHiddenInTabPager ? 1 : TabState.COUNT;
        }

        /** Gets called when the number of items changes. */
        @Override
        public int getItemPosition(Object object) {
            if (mAreTabsHiddenInTabPager) {
                if (object == mAllFragment) {
                    return 0; // Only 1 page in search mode
                }
            } else {
                if (object == mFavoritesFragment) {
                    return getTabPositionForTextDirection(TabState.FAVORITES);
                }
                if (object == mAllFragment) {
                    return getTabPositionForTextDirection(TabState.ALL);
                }
            }
            return POSITION_NONE;
        }

        @Override
        public void startUpdate(ViewGroup container) {
        }

        private Fragment getFragment(int position) {
            position = getTabPositionForTextDirection(position);
            if (mAreTabsHiddenInTabPager) {
                if (position != 0) {
                    // This has only been observed in monkey tests.
                    // Let's log this issue, but not crash
                    Log.w(TAG, "Request fragment at position=" + position + ", eventhough we " +
                            "are in search mode");
                }
                return mAllFragment;
            } else {
                if (position == TabState.FAVORITES) {
                    return mFavoritesFragment;
                } else if (position == TabState.ALL) {
                    return mAllFragment;
                }
            }
            throw new IllegalArgumentException("position: " + position);
        }

        @Override
        public Object instantiateItem(ViewGroup container, int position) {
            if (mCurTransaction == null) {
                mCurTransaction = mFragmentManager.beginTransaction();
            }
            Fragment f = getFragment(position);
            mCurTransaction.show(f);

            // Non primary pages are not visible.
            f.setUserVisibleHint(f == mCurrentPrimaryItem);
            return f;
        }

        @Override
        public void destroyItem(ViewGroup container, int position, Object object) {
            if (mCurTransaction == null) {
                mCurTransaction = mFragmentManager.beginTransaction();
            }
            mCurTransaction.hide((Fragment) object);
        }

        @Override
        public void finishUpdate(ViewGroup container) {
            if (mCurTransaction != null) {
                mCurTransaction.commitAllowingStateLoss();
                mCurTransaction = null;
                mFragmentManager.executePendingTransactions();
            }
        }

        @Override
        public boolean isViewFromObject(View view, Object object) {
            return ((Fragment) object).getView() == view;
        }

        @Override
        public void setPrimaryItem(ViewGroup container, int position, Object object) {
            Fragment fragment = (Fragment) object;
            if (mCurrentPrimaryItem != fragment) {
                if (mCurrentPrimaryItem != null) {
                    mCurrentPrimaryItem.setUserVisibleHint(false);
                }
                if (fragment != null) {
                    fragment.setUserVisibleHint(true);
                }
                mCurrentPrimaryItem = fragment;
            }
        }

        @Override
        public Parcelable saveState() {
            return null;
        }

        @Override
        public void restoreState(Parcelable state, ClassLoader loader) {
        }

        @Override
        public CharSequence getPageTitle(int position) {
            return mTabTitles[position];
        }
    }

    private void setQueryTextToFragment(String query) {
        mAllFragment.setQueryString(query, true);
        mAllFragment.setVisibleScrollbarEnabled(!mAllFragment.isSearchMode());
    }

    private void configureContactListFragmentForRequest() {
        Uri contactUri = mRequest.getContactUri();
        if (contactUri != null) {
            mAllFragment.setSelectedContactUri(contactUri);
        }

        mAllFragment.setFilter(mContactListFilterController.getFilter());
        setQueryTextToFragment(mActionBarAdapter.getQueryString());

        if (mRequest.isDirectorySearchEnabled()) {
            mAllFragment.setDirectorySearchMode(DirectoryListLoader.SEARCH_MODE_DEFAULT);
        } else {
            mAllFragment.setDirectorySearchMode(DirectoryListLoader.SEARCH_MODE_NONE);
        }
    }

    private void configureContactListFragment() {
        // Filter may be changed when this Activity is in background.
        mAllFragment.setFilter(mContactListFilterController.getFilter());

        mAllFragment.setVerticalScrollbarPosition(getScrollBarPosition());
        mAllFragment.setSelectionVisible(false);
    }

    private int getScrollBarPosition() {
        return isRTL() ? View.SCROLLBAR_POSITION_LEFT : View.SCROLLBAR_POSITION_RIGHT;
    }

    private boolean isRTL() {
        final Locale locale = Locale.getDefault();
        return TextUtils.getLayoutDirectionFromLocale(locale) == View.LAYOUT_DIRECTION_RTL;
    }

    @Override
    public void onProviderStatusChange() {
        updateViewConfiguration(false);
    }

    private void updateViewConfiguration(boolean forceUpdate) {
        int providerStatus = mProviderStatusWatcher.getProviderStatus();
        if (!forceUpdate && (mProviderStatus != null)
                && (mProviderStatus.equals(providerStatus))) return;
        mProviderStatus = providerStatus;

        View contactsUnavailableView = findViewById(R.id.contacts_unavailable_view);

       /**
        * SPRD: add for sync sim, bug421244
        * SPRD: 533198 Shark LS contacts count goes on increasing/decreasing after reboot.
        *
        * Original Android code:
        if (mProviderStatus.equals(ProviderStatus.STATUS_NORMAL)) {
        * @{
        */
        if (mProviderStatus.equals(ProviderStatus.STATUS_NORMAL)
                || mProviderStatus.equals(ProviderStatus.STATUS_IMPORTING)
                || mProviderStatus.equals(ProviderStatus.STATUS_BUSY)
                || mProviderStatus.equals(ProviderStatus.STATUS_EMPTY)) {
       /**
        * @}
        */
            // Ensure that the mTabPager is visible; we may have made it invisible below.
            contactsUnavailableView.setVisibility(View.GONE);
            if (mTabPager != null) {
                mTabPager.setVisibility(View.VISIBLE);
            }

            if (mAllFragment != null) {
                mAllFragment.setEnabled(true);
            }
        } else {
            // Setting up the page so that the user can still use the app
            // even without an account.
            if (mAllFragment != null) {
                mAllFragment.setEnabled(false);
            }
            if (mContactsUnavailableFragment == null) {
                mContactsUnavailableFragment = new ContactsUnavailableFragment();
                mContactsUnavailableFragment.setOnContactsUnavailableActionListener(
                        new ContactsUnavailableFragmentListener());
                getFragmentManager().beginTransaction()
                        .replace(R.id.contacts_unavailable_container, mContactsUnavailableFragment)
                        .commitAllowingStateLoss();
            }
            mContactsUnavailableFragment.updateStatus(mProviderStatus);

            // Show the contactsUnavailableView, and hide the mTabPager so that we don't
            // see it sliding in underneath the contactsUnavailableView at the edges.
            contactsUnavailableView.setVisibility(View.VISIBLE);
            if (mTabPager != null) {
                mTabPager.setVisibility(View.GONE);
            }

            showEmptyStateForTab(mActionBarAdapter.getCurrentTab());
        }

        invalidateOptionsMenuIfNeeded();
    }

    private final class ContactBrowserActionListener implements OnContactBrowserActionListener {
        ContactBrowserActionListener() {}

        @Override
        public void onSelectionChange() {

        }

        @Override
        public void onViewContactAction(Uri contactLookupUri, boolean isEnterpriseContact) {
            if (isEnterpriseContact) {
                // No implicit intent as user may have a different contacts app in work profile.
                QuickContact.showQuickContact(PeopleActivity.this, new Rect(), contactLookupUri,
                        QuickContactActivity.MODE_FULLY_EXPANDED, null);
            } else {
                final Intent intent = ImplicitIntentsUtil.composeQuickContactIntent(
                        contactLookupUri, QuickContactActivity.MODE_FULLY_EXPANDED);
                intent.putExtra(QuickContactActivity.EXTRA_PREVIOUS_SCREEN_TYPE,
                        mAllFragment.isSearchMode() ? ScreenType.SEARCH : ScreenType.ALL_CONTACTS);
                ImplicitIntentsUtil.startActivityInApp(PeopleActivity.this, intent);
            }
        }

        @Override
        public void onDeleteContactAction(Uri contactUri) {
            ContactDeletionInteraction.start(PeopleActivity.this, contactUri, false);
        }

        @Override
        public void onFinishAction() {
            onBackPressed();
        }

        @Override
        public void onInvalidSelection() {
            ContactListFilter filter;
            ContactListFilter currentFilter = mAllFragment.getFilter();
            if (currentFilter != null
                    && currentFilter.filterType == ContactListFilter.FILTER_TYPE_SINGLE_CONTACT) {
                filter = ContactListFilter.createFilterWithType(
                        ContactListFilter.FILTER_TYPE_ALL_ACCOUNTS);
                mAllFragment.setFilter(filter);
            } else {
                filter = ContactListFilter.createFilterWithType(
                        ContactListFilter.FILTER_TYPE_SINGLE_CONTACT);
                mAllFragment.setFilter(filter, false);
            }
            mContactListFilterController.setContactListFilter(filter, true);
        }
    }

    private final class CheckBoxListListener implements OnCheckBoxListActionListener {
        @Override
        public void onStartDisplayingCheckBoxes() {
            mActionBarAdapter.setSelectionMode(true);
            invalidateOptionsMenu();
        }

        @Override
        public void onSelectedContactIdsChanged() {
            mActionBarAdapter.setSelectionCount(mAllFragment.getSelectedContactIds().size());
            invalidateOptionsMenu();
        }

        @Override
        public void onStopDisplayingCheckBoxes() {
            mActionBarAdapter.setSelectionMode(false);
        }
    }

    private class ContactsUnavailableFragmentListener
            implements OnContactsUnavailableActionListener {
        ContactsUnavailableFragmentListener() {}

        @Override
        public void onCreateNewContactAction() {
            ImplicitIntentsUtil.startActivityInApp(PeopleActivity.this,
                    EditorIntents.createCompactInsertContactIntent());
        }

        @Override
        public void onAddAccountAction() {
            final Intent intent = ImplicitIntentsUtil.getIntentForAddingAccount();
            ImplicitIntentsUtil.startActivityOutsideApp(PeopleActivity.this, intent);
        }

        @Override
        public void onImportContactsFromFileAction() {
            showImportExportDialogFragment();
        }
    }

    private final class StrequentContactListFragmentListener
            implements ContactTileListFragment.Listener {
        StrequentContactListFragmentListener() {}

        @Override
        public void onContactSelected(Uri contactUri, Rect targetRect) {
            final Intent intent = ImplicitIntentsUtil.composeQuickContactIntent(contactUri,
                    QuickContactActivity.MODE_FULLY_EXPANDED);
            ImplicitIntentsUtil.startActivityInApp(PeopleActivity.this, intent);
        }

        @Override
        public void onCallNumberDirectly(String phoneNumber) {
            // No need to call phone number directly from People app.
            Log.w(TAG, "unexpected invocation of onCallNumberDirectly()");
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        /**
         * SPRD:Bug431588 Contacts menu disappeared when switch language.
         *
         * @{
         */
        mOptionsMenuContactsAvailable = areContactsAvailable();
        /**
         * @}
         */
        if (!areContactsAvailable()) {
            // If contacts aren't available, hide all menu items.
            return false;
        }
        super.onCreateOptionsMenu(menu);

        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.people_options, menu);
        return true;
    }

    private void invalidateOptionsMenuIfNeeded() {
        if (isOptionsMenuChanged()) {
            invalidateOptionsMenu();
        }
    }

    public boolean isOptionsMenuChanged() {
        if (mOptionsMenuContactsAvailable != areContactsAvailable()) {
            return true;
        }

        if (mAllFragment != null && mAllFragment.isOptionsMenuChanged()) {
            return true;
        }

        return false;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        mOptionsMenuContactsAvailable = areContactsAvailable();
        if (!mOptionsMenuContactsAvailable) {
            return false;
        }

        // Get references to individual menu items in the menu
        final MenuItem contactsFilterMenu = menu.findItem(R.id.menu_contacts_filter);
        final MenuItem clearFrequentsMenu = menu.findItem(R.id.menu_clear_frequents);
        final MenuItem helpMenu = menu.findItem(R.id.menu_help);
        /**
         * SPRD:Add group menu
         * @{
         */
        final MenuItem GroupMenu = menu.findItem(R.id.menu_group);
        /**
         * @}
         */
        final boolean isSearchOrSelectionMode = mActionBarAdapter.isSearchMode()
                || mActionBarAdapter.isSelectionMode();
        /**
         * SPRD: Moved position
         *
         * @{
         */
        final boolean showMiscOptions = !isSearchOrSelectionMode;
        final boolean showSelectedContactOptions = mActionBarAdapter.isSelectionMode()
                && mAllFragment.getSelectedContactIds().size() != 0;
        /**
         * @}
         */
        /**
         * SPRD: Add by SPRD for clean up contacts Bug439934
         *
         * @{
         */
        SubMenu advancedOptions = null;
        if(menu.findItem(ADVANCED_OPTIONS) == null){
            advancedOptions = menu.addSubMenu(0, ADVANCED_OPTIONS, 0, R.string.advanced_options);
            advancedOptions.add(1, SIM_CAPACITY, 0, R.string.sim_capacity);
            advancedOptions.add(1, CLEARUP, 0, R.string.clearup_contacts);
           //SPRD:615630 SunMenu invalid when split screen begin
            mAdvancedOptions = advancedOptions;
           //SPRD:615630 SunMenu invalid when split screen end
        }
        /**
         * @}
         */
        if (isSearchOrSelectionMode) {
            contactsFilterMenu.setVisible(false);
            clearFrequentsMenu.setVisible(false);
            helpMenu.setVisible(false);
            /**
             * SPRD:Bug474772 Add SIM/USIM phonebook capacity feature in Contacts.
             * SPRD:Bug516358 Dut shows wrong layout in contact list menu in Arabic.
             *
             * @{
             */
            makeMenuItemVisible(menu, R.id.menu_add_favorite, false);
            makeMenuItemVisible(menu, R.id.menu_batch_delete, false);
            makeMenuItemVisible(menu, R.id.menu_share_by_sms, false);
            makeMenuItemVisible(menu, R.id.menu_import_export, false);
            makeMenuItemVisible(menu, R.id.menu_search, false);
            makeMenuItemVisible(menu, R.id.menu_group, false);
            /**
             * @}
             */
        } else {
            switch (getTabPositionForTextDirection(mActionBarAdapter.getCurrentTab())) {
                case TabState.FAVORITES:
                    contactsFilterMenu.setVisible(false);
                    clearFrequentsMenu.setVisible(hasFrequents());
                    /**
                     * SPRD: Bug474752
                     *
                     * @{
                     */
                    makeMenuItemVisible(menu, R.id.menu_batch_delete, false);
                    makeMenuItemVisible(menu, R.id.menu_add_favorite, true);
                    makeMenuItemVisible(menu, R.id.menu_share_by_sms, false);
                    makeMenuItemVisible(menu, R.id.menu_group, false);
                    /**
                     * @}
                     */
                    break;
                case TabState.ALL:
                    contactsFilterMenu.setVisible(true);
                    clearFrequentsMenu.setVisible(false);
                    /**
                     * SPRD: Bug474752
                     *
                     * @{
                     */
                    makeMenuItemVisible(menu, R.id.menu_batch_delete, showMiscOptions);
                    makeMenuItemVisible(menu, R.id.menu_share_by_sms, showMiscOptions);
                    makeMenuItemVisible(menu, R.id.menu_group, showMiscOptions);
                    makeMenuItemVisible(menu, R.id.menu_add_favorite, false);
                    /**
                     * @}
                     */
                    break;
            }
            helpMenu.setVisible(HelpUtils.isHelpAndFeedbackAvailable());
        }
        final boolean showBlockedNumbers = PhoneCapabilityTester.isPhone(this)
                && ContactsUtils.FLAG_N_FEATURE
                && BlockedNumberContractCompat.canCurrentUserBlockNumbers(this);
        /**
         * SPRD:474772 Add SIM/USIM phonebook capacity feature in Contacts.
         *
         * @{
         */
        makeMenuItemVisible(menu, ADVANCED_OPTIONS, showMiscOptions);
        /**
         * @}
         */
        makeMenuItemVisible(menu, R.id.menu_search, showMiscOptions);
        makeMenuItemVisible(menu, R.id.menu_import_export, showMiscOptions);
        makeMenuItemVisible(menu, R.id.menu_accounts, showMiscOptions);
        makeMenuItemVisible(menu, R.id.menu_blocked_numbers, showMiscOptions && showBlockedNumbers);
        makeMenuItemVisible(menu, R.id.menu_settings,
                showMiscOptions && !ContactsPreferenceActivity.isEmpty(this));

        makeMenuItemVisible(menu, R.id.menu_share, showSelectedContactOptions);
        makeMenuItemVisible(menu, R.id.menu_delete, showSelectedContactOptions);
        final boolean showLinkContactsOptions = mActionBarAdapter.isSelectionMode()
                && mAllFragment.getSelectedContactIds().size() > 1;
        makeMenuItemVisible(menu, R.id.menu_join, showLinkContactsOptions);

        // Debug options need to be visible even in search mode.
        makeMenuItemVisible(menu, R.id.export_database, mEnableDebugMenuOptions &&
                hasExportIntentHandler());

        return true;
    }

    private boolean hasExportIntentHandler() {
        final Intent intent = new Intent();
        intent.setAction("com.android.providers.contacts.DUMP_DATABASE");
        final List<ResolveInfo> receivers = getPackageManager().queryIntentActivities(intent,
                PackageManager.MATCH_DEFAULT_ONLY);
        return receivers != null && receivers.size() > 0;
    }

    /**
     * Returns whether there are any frequently contacted people being displayed
     * @return
     */
    private boolean hasFrequents() {
        return mFavoritesFragment.hasFrequents();
    }

    private void makeMenuItemVisible(Menu menu, int itemId, boolean visible) {
        final MenuItem item = menu.findItem(itemId);
        if (item != null) {
            item.setVisible(visible);
        }
    }

    private void makeMenuItemEnabled(Menu menu, int itemId, boolean visible) {
        final MenuItem item = menu.findItem(itemId);
        if (item != null) {
            item.setEnabled(visible);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (mDisableOptionItemSelected) {
            return false;
        }

        switch (item.getItemId()) {
            case android.R.id.home: {
                // The home icon on the action bar is pressed
                if (mActionBarAdapter.isUpShowing()) {
                    // "UP" icon press -- should be treated as "back".
                    onBackPressed();
                }
                return true;
            }
            case R.id.menu_settings: {
                startActivity(new Intent(this, ContactsPreferenceActivity.class));
                return true;
            }
            case R.id.menu_contacts_filter: {
                /*
                * SPRD:
                *
                * Original Android code:
                AccountFilterUtil.startAccountFilterActivityForResult(
                        this, SUBACTIVITY_ACCOUNT_FILTER,
                        mContactListFilterController.getFilter());
                *
                * @{
                */
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving
                        || JoinContactsDialogFragment.isContactsJoining
                        || ContactMultiDeletionInteraction.isMultiContactsDeleting) {
                    Toast.makeText(PeopleActivity.this, R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                    AccountFilterUtil.startAccountFilterActivityForResult(
                            this, SUBACTIVITY_ACCOUNT_FILTER,
                            mContactListFilterController.getFilter());
                }
                /*
                 * @}
                 */
                return true;
            }
            case R.id.menu_search: {
                onSearchRequested();
                return true;
            }
            case R.id.menu_share:
                /**
                 * SPRD:
                 *
                 * Original Android code:
                shareSelectedContacts();
                 * @{
                 */
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving
                        || JoinContactsDialogFragment.isContactsJoining
                        || ContactMultiDeletionInteraction.isMultiContactsDeleting) {
                    Toast.makeText(PeopleActivity.this, R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                    shareSelectedContacts();
                }
                /**
                 * @}
                 */
                return true;
            case R.id.menu_join:
                /**
                 * SPRD:
                 *
                 * Original Android code:
                joinSelectedContacts();
                 * @{
                 */
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving
                        || JoinContactsDialogFragment.isContactsJoining
                        || ContactMultiDeletionInteraction.isMultiContactsDeleting) {
                    Toast.makeText(PeopleActivity.this, R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                    //SPRD: add for bug615040, limit the join contacts to 10.
                    int contactJoinNum = getSelectedContactJoinNum();
                    if (contactJoinNum <= MAX_JOIN_CONTACTS_NUMBER) {
                        joinSelectedContacts();
                    } else {
                        Toast.makeText(PeopleActivity.this, getString(R.string.over_max_join_number, contactJoinNum), Toast.LENGTH_LONG).show();
                    }
                }
                /**
                 * @}
                 */
                return true;
            case R.id.menu_delete:
                /**
                 * SPRD:
                 *
                 * Original Android code:
                deleteSelectedContacts();
                 * @{
                 */
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving
                        || JoinContactsDialogFragment.isContactsJoining
                        || ContactMultiDeletionInteraction.isMultiContactsDeleting) {
                    Toast.makeText(PeopleActivity.this, R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                    deleteSelectedContacts();
                }
                /**
                 * @}
                 */
                return true;
            case R.id.menu_import_export: {
                /**
                 * SPRD:
                 *
                 * Original Android code:
                showImportExportDialogFragment();
                 * @{
                 */
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving
                        || JoinContactsDialogFragment.isContactsJoining
                        || ContactMultiDeletionInteraction.isMultiContactsDeleting) {
                    Toast.makeText(PeopleActivity.this, R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                    showImportExportDialogFragment();
                }
                /**
                 * @}
                 */
                return true;
            }
            case R.id.menu_clear_frequents: {
                ClearFrequentsDialog.show(getFragmentManager());
                return true;
            }
            case R.id.menu_help:
                HelpUtils.launchHelpAndFeedbackForMainScreen(this);
                return true;
            case R.id.menu_accounts: {
                final Intent intent = new Intent(Settings.ACTION_SYNC_SETTINGS);
                intent.putExtra(Settings.EXTRA_AUTHORITIES, new String[] {
                    ContactsContract.AUTHORITY
                });
                intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                ImplicitIntentsUtil.startActivityInAppIfPossible(this, intent);
                return true;
            }
            case R.id.menu_blocked_numbers: {
                /* SPRD: add for bug594033 @{*/
                Intent intent = new Intent();
                if (isCallFireWallInstalled()) {
                    intent.setAction("com.sprd.blacklist.action");
                } else {
                    intent = TelecomManagerUtil.createManageBlockedNumbersIntent(
                            (TelecomManager) getSystemService(Context.TELECOM_SERVICE));
                }
                /* @} */
                if (intent != null) {
                    startActivity(intent);
                }
                return true;
            }
            case R.id.export_database: {
                final Intent intent = new Intent("com.android.providers.contacts.DUMP_DATABASE");
                intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                ImplicitIntentsUtil.startActivityOutsideApp(this, intent);
                return true;
            }
            /**
             * SPRD
             *
             * @{
             */
            case R.id.menu_add_favorite: {
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving
                        || JoinContactsDialogFragment.isContactsJoining
                        || ContactMultiDeletionInteraction.isMultiContactsDeleting) {
                    Toast.makeText(PeopleActivity.this, R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                    final Intent intent = new Intent(UiIntentActions.MULTI_PICK_ACTION);
                    intent.setData(Contacts.CONTENT_URI);
                    intent.putExtra("setMulitStarred", true);
                    startActivityForResult(intent, SUBACTIVITY_SET_STARRED);
                }
                return true;
            }
            case R.id.menu_batch_delete: {
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving
                        || JoinContactsDialogFragment.isContactsJoining
                        || ContactMultiDeletionInteraction.isMultiContactsDeleting) {
                    Toast.makeText(PeopleActivity.this, R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                    final Intent intent = new Intent(UiIntentActions.MULTI_PICK_ACTION);
                    intent.setData(Contacts.CONTENT_URI);
                    intent.putExtra("mode", SUBACTIVITY_BATCH_DELETE);
                    intent.putExtra("exclude_read_only", true);
                    startActivityForResult(intent, SUBACTIVITY_BATCH_DELETE);
               }
                return true;
            }
            case R.id.menu_share_by_sms: {
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving
                        || JoinContactsDialogFragment.isContactsJoining
                        || ContactMultiDeletionInteraction.isMultiContactsDeleting) {
                    Toast.makeText(PeopleActivity.this, R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                    final Intent intent = new Intent(UiIntentActions.MULTI_PICK_ACTION).
                            putExtra("checked_limit_count", MAX_CONTACTS_NUMBER).
                            putExtra(
                                    "cascading",
                                    new Intent(UiIntentActions.MULTI_PICK_ACTION).setType(
                                            Phone.CONTENT_ITEM_TYPE).
                                            putExtra(
                                                    "cascading",
                                                    new Intent(UiIntentActions.MULTI_PICK_ACTION)
                                                            .setType(Email.CONTENT_ITEM_TYPE)));
                    startActivityForResult(intent, SUBACTIVITY_SHARE_BY_SMS);
                }
                return true;
            }
            case CLEARUP:{
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving
                        || JoinContactsDialogFragment.isContactsJoining
                        || ContactMultiDeletionInteraction.isMultiContactsDeleting) {
                    Toast.makeText(PeopleActivity.this, R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                    final Intent intent = new Intent(this, ContactDeduplicationActivity.class);
                    /**
                     * SPRD:Bug494541 After arranging contacts, contact list should be reload again.
                     * @{
                     */
                    mDeduplication = true;
                    /**
                     * @}
                     */
                    startActivity(intent);
                }
                return true;
            }
            case SIM_CAPACITY:{
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving
                        || JoinContactsDialogFragment.isContactsJoining
                        || ContactMultiDeletionInteraction.isMultiContactsDeleting) {
                    Toast.makeText(PeopleActivity.this, R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                    final Intent intent = new Intent(this, ContactsMemoryActivity.class);
                    ImplicitIntentsUtil.startActivityInApp(PeopleActivity.this, intent);
                }
                return true;
            }
            /**
             * @}
             */
            /**
             * SPRD:Bug474739 Add for group
             * @{
             */
            case R.id.menu_group:{
                final Intent intent = new Intent(this, GroupBrowseListActivity.class);
                startActivity(intent);
                return true;
            }
            /**
             * @}
             */
        }
        return false;
    }

    /* SPRD: add for bug594033 @{*/
    private boolean isCallFireWallInstalled() {
        boolean installed = false;
        try {
            getBaseContext().getPackageManager().getPackageInfo(
                "com.sprd.firewall", PackageManager.GET_ACTIVITIES);
            installed = true;
        } catch (PackageManager.NameNotFoundException e) {
        }
        return installed;
    }
    /* @} */

    private void showImportExportDialogFragment(){
            final boolean isOnFavoriteTab = mTabPagerAdapter.mCurrentPrimaryItem == mFavoritesFragment;
            if (isOnFavoriteTab) {
                ImportExportDialogFragment.show(getFragmentManager(), areContactsAvailable(),
                        PeopleActivity.class, ImportExportDialogFragment.EXPORT_MODE_FAVORITES);
            } else {
            /**
             * SPRD:Bug474746 Import/Export SdCard vcf contacts.
             *
             * Original Android code:
            ImportExportDialogFragment.show(getFragmentManager(), areContactsAvailable(),
                    PeopleActivity.class);
            ImportExportDialogFragment.show(getFragmentManager(), areContactsAvailable(),
                    PeopleActivity.class, ImportExportDialogFragment.EXPORT_MODE_ALL_CONTACTS);
             * @{
             */
            ImportExportDialogFragmentSprd.show(getFragmentManager(), areContactsAvailable(),
                    PeopleActivity.class);
            /**
             * @}
             */
            }
        }
    @Override
    public boolean onSearchRequested() { // Search key pressed.
        if (!mActionBarAdapter.isSelectionMode()) {
            mActionBarAdapter.setSearchMode(true);
        }
        return true;
    }

    /**
     * Share all contacts that are currently selected in mAllFragment. This method is pretty
     * inefficient for handling large numbers of contacts. I don't expect this to be a problem.
     */
    private void shareSelectedContacts() {
        final StringBuilder uriListBuilder = new StringBuilder();
        for (Long contactId : mAllFragment.getSelectedContactIds()) {
            final Uri contactUri = ContentUris.withAppendedId(Contacts.CONTENT_URI, contactId);
            final Uri lookupUri = Contacts.getLookupUri(getContentResolver(), contactUri);
            if (lookupUri == null) {
                continue;
            }
            final List<String> pathSegments = lookupUri.getPathSegments();
            if (pathSegments.size() < 2) {
                continue;
            }
            final String lookupKey = pathSegments.get(pathSegments.size() - 2);
            if (uriListBuilder.length() > 0) {
                uriListBuilder.append(':');
            }
            uriListBuilder.append(Uri.encode(lookupKey));
        }
        if (uriListBuilder.length() == 0) {
            return;
        }
        final Uri uri = Uri.withAppendedPath(
                Contacts.CONTENT_MULTI_VCARD_URI,
                Uri.encode(uriListBuilder.toString()));
        final Intent intent = new Intent(Intent.ACTION_SEND);
        intent.setType(Contacts.CONTENT_VCARD_TYPE);
        intent.putExtra(Intent.EXTRA_STREAM, uri);
        ImplicitIntentsUtil.startActivityOutsideApp(this, intent);
    }
    private void joinSelectedContacts() {
        /**
        * sprd Bug494092 join local contacts and SIM contacts, while setting phone of this contact, Contacts app FC.
        *
        * @{
        */
        Iterator<Long> iterator = mAllFragment.getSelectedContactIds().iterator();
        boolean isStopJoin = false;
        HashMap<Long, String> selectedContactIdAndTypes = mAllFragment.getSelectedContactIdAndTypes();
        while (iterator.hasNext()) {
            Long contactId = iterator.next();
            String contactType = selectedContactIdAndTypes.get(contactId);
            if (contactType == null) {
                continue;
            }
            if (contactType.equals("sprd.com.android.account.sim")) {
                Toast.makeText(PeopleActivity.this, R.string.join_fail_for_sim, Toast.LENGTH_LONG).show();
                isStopJoin = true;
                return;
            } else if (contactType.equals("sprd.com.android.account.usim")) {
                Toast.makeText(PeopleActivity.this, R.string.join_fail_for_usim, Toast.LENGTH_LONG).show();
                isStopJoin = true;
                return;
            }
        }
        if (isStopJoin) {
            return;
        }
        /**
        * @}
        */
        JoinContactsDialogFragment.start(this, mAllFragment.getSelectedContactIds());
    }

    //SPRD: add for bug615040, limit the join contacts to 10.
    private int getSelectedContactJoinNum() {
        String str = mAllFragment.getSelectedContactIds().toString();
        String str1 = str.replace("[", "(");
        String str2 = str1.replace("]", ")");
        int joinNum = 0;
        ContentResolver resolver = getContentResolver();
        Cursor cursor = resolver.query(
                ContactsContract.RawContacts.CONTENT_URI,
                new String[] {ContactsContract.RawContacts._ID, ContactsContract.RawContacts.CONTACT_ID},
                "contact_id in " + str2, null, null);
        if (cursor != null) {
            joinNum = cursor.getCount();
        } else {
            joinNum = mAllFragment.getSelectedContactIds().size();
        }
        if (cursor != null) {
            cursor.close();
        }
        Log.d(TAG, "getSelectedContactJoinNum, joinNum = " + joinNum);
        return joinNum;
    }

    @Override
    public void onContactsJoined() {
        mActionBarAdapter.setSelectionMode(false);
    }

    private void deleteSelectedContacts() {
        ContactMultiDeletionInteraction.start(PeopleActivity.this,
                mAllFragment.getSelectedContactIds());
    }

    @Override
    public void onDeletionFinished() {
        mActionBarAdapter.setSelectionMode(false);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case SUBACTIVITY_ACCOUNT_FILTER: {
                AccountFilterUtil.handleAccountFilterResult(
                        mContactListFilterController, resultCode, data);
                break;
            }

            // TODO: Using the new startActivityWithResultFromFragment API this should not be needed
            // anymore
            case ContactEntryListFragment.ACTIVITY_REQUEST_CODE_PICKER:
                if (resultCode == RESULT_OK) {
                    mAllFragment.onPickerResult(data);
                }
                /**
                 * SPRD Bug 474726、474752 Add features with multi-selection activity in Contacts.
                 *
                 * @{
                 */
            case SUBACTIVITY_SET_STARRED: {
                if (resultCode == RESULT_OK) {
                    ArrayList<String> lookupKeys = data
                            .getStringArrayListExtra("result");
                    if (lookupKeys != null && !lookupKeys.isEmpty()) {
                        Intent intent = new Intent(data);
                        intent.setComponent(new ComponentName(PeopleActivity.this,
                                BatchOperationService.class));
                        intent.putExtra(BatchOperationService.KEY_MODE,
                                BatchOperationService.MODE_START_BATCH_STARRED);
                        startService(intent);
                    }
                }
                break;
            }
            case SUBACTIVITY_BATCH_DELETE: {
                if (resultCode == RESULT_OK) {
                    ArrayList<String> lookupKeys = data
                            .getStringArrayListExtra("result");
                    if (lookupKeys != null && !lookupKeys.isEmpty()) {
                        // new BatchDeleteTask(PeopleActivity.this,
                        // lookupKeys.size()).execute(lookupKeys);
                        Intent intent = new Intent(data);
                        intent.setComponent(new ComponentName(PeopleActivity.this,
                                BatchOperationService.class));
                        intent.putExtra(BatchOperationService.KEY_MODE,
                                BatchOperationService.MODE_START_BATCH_DELETE);
                        startService(intent);
                    }
                }
                break;
            }
            case SUBACTIVITY_SHARE_BY_SMS: {
                if (resultCode == RESULT_OK && data != null) {
                    try {
                        Intent intent = new Intent(Intent.ACTION_SENDTO, Uri.fromParts("sms", "", null));
                        HashMap<String, String> contacts = (HashMap<String, String>) data
                                .getSerializableExtra("result");
                        intent.putExtra("sms_body",shareContactBySms(contacts));
                        ImplicitIntentsUtil.startActivityOutsideApp(this,intent);
                    } catch (ActivityNotFoundException e) {
                        Toast.makeText(this, R.string.noActivityHandle, Toast.LENGTH_SHORT).show();
                    }
                }
                break;
            }
            /**
             * SPRD:Bug474746 Import/Export SdCard vcf contacts.
             *
             * @{
             */
            case SUBACTIVITY_BATCH_EXPORT_TO_SDCARD: {
                if (resultCode == RESULT_OK) {
                    ArrayList<String> lookupKeys = data
                            .getStringArrayListExtra("result");
                    Intent exportIntent = new Intent(PeopleActivity.this,
                            ExportVCardActivity.class);
                    exportIntent.putStringArrayListExtra("lookup_keys", lookupKeys);
                    exportIntent.putExtra(VCardCommonArguments.ARG_CALLING_ACTIVITY,
                            "com.android.contacts.activities.PeopleActivity");
                    startActivity(exportIntent);
                }
                break;
            }
            case SUBACTIVITY_BATCH_IMPORT: {
                if (resultCode == RESULT_OK) {
                    ArrayList<String> ids = data
                            .getStringArrayListExtra("result_alternative");
                    AccountWithDataSet account = (AccountWithDataSet) (data
                            .getParcelableExtra("dst_account"));
                    if (ids != null && !ids.isEmpty() && account != null) {
                        Intent intent = new Intent(data);
                        intent.setComponent(new ComponentName(PeopleActivity.this,
                                BatchOperationService.class));
                        intent.putExtra(
                                BatchOperationService.KEY_MODE,
                                BatchOperationService.MODE_START_BATCH_IMPORT_EXPORT);
                        startService(intent);
                    }
                }
                break;
            }
            case SUBACTIVITY_SHARE_VISIBLE: {
                if (resultCode == RESULT_OK) {
                    ArrayList<String> lookupKeys = data
                            .getStringArrayListExtra("result");
                    StringBuilder uriListBuilder = new StringBuilder();
                    int index = 0;
                    for (String key : lookupKeys) {
                        if (index != 0)
                            uriListBuilder.append(':');
                        uriListBuilder.append(key);
                        index++;
                    }
                    Uri uri = Uri.withAppendedPath(
                            Contacts.CONTENT_MULTI_VCARD_URI,
                            Uri.encode(uriListBuilder.toString()));
                    final Intent intent = new Intent(Intent.ACTION_SEND);
                    Parcel parcel = Parcel.obtain();
                    intent.setType(Contacts.CONTENT_VCARD_TYPE);
                    intent.putExtra(Intent.EXTRA_STREAM, uri);
                    try {
                        intent.writeToParcel(parcel, 0);
                        if(Constants.DEBUG)
                            Log.d(TAG, "Shared parcel size is" + parcel.dataSize());
                        if (parcel.dataSize() > MAX_DATA_SIZE) {
                            Toast.makeText(PeopleActivity.this, R.string.transaction_too_large,
                                    Toast.LENGTH_LONG).show();
                            break;
                        }
                    } finally {
                        parcel.recycle();
                    }
                    ImplicitIntentsUtil.startActivityOutsideApp(this,intent);
                }
                break;
            }
            /**
             * @}
             */
// TODO fix or remove multipicker code
//                else if (resultCode == RESULT_CANCELED && mMode == MODE_PICK_MULTIPLE_PHONES) {
//                    // Finish the activity if the sub activity was canceled as back key is used
//                    // to confirm user selection in MODE_PICK_MULTIPLE_PHONES.
//                    finish();
//                }
//                break;
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // TODO move to the fragment

        // Bring up the search UI if the user starts typing
        final int unicodeChar = event.getUnicodeChar();
        if ((unicodeChar != 0)
                // If COMBINING_ACCENT is set, it's not a unicode character.
                && ((unicodeChar & KeyCharacterMap.COMBINING_ACCENT) == 0)
                && !Character.isWhitespace(unicodeChar)) {
            if (mActionBarAdapter.isSelectionMode()) {
                // Ignore keyboard input when in selection mode.
                return true;
            }
            String query = new String(new int[]{unicodeChar}, 0, 1);
            if (!mActionBarAdapter.isSearchMode()) {
                mActionBarAdapter.setSearchMode(true);
                mActionBarAdapter.setQueryString(query);
                return true;
            }
        }

        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onBackPressed() {
        if (!isSafeToCommitTransactions()) {
            return;
        }

        if (mActionBarAdapter.isSelectionMode()) {
            mActionBarAdapter.setSelectionMode(false);
            mAllFragment.displayCheckBoxes(false);
        } else if (mActionBarAdapter.isSearchMode()) {
            mActionBarAdapter.setSearchMode(false);
            if (mAllFragment.wasSearchResultClicked()) {
                mAllFragment.resetSearchResultClicked();
            } else {
                Logger.logScreenView(this, ScreenType.SEARCH_EXIT);
                Logger.logSearchEvent(mAllFragment.createSearchState());
            }
        } else {
            /**
             * SPRD: 544835 The DUT occurs IllegalStateException while
             * running monkey test.
             * android original code:
            super.onBackPressed();
             * @{
             */
            try {
                super.onBackPressed();
            } catch (Exception e) {
                // TODO: handle exception
                Log.d(TAG, "onBackPressed exception is " + e.toString());
                finish();
            }
            /*
             * @}
             */
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        mActionBarAdapter.onSaveInstanceState(outState);

        // Clear the listener to make sure we don't get callbacks after onSaveInstanceState,
        // in order to avoid doing fragment transactions after it.
        // TODO Figure out a better way to deal with the issue.
        mDisableOptionItemSelected = true;
            mActionBarAdapter.setListener(null);
        if (mTabPager != null) {
            mTabPager.setOnPageChangeListener(null);
        }
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        // In our own lifecycle, the focus is saved and restore but later taken away by the
        // ViewPager. As a hack, we force focus on the SearchView if we know that we are searching.
        // This fixes the keyboard going away on screen rotation
        if (mActionBarAdapter.isSearchMode()) {
            mActionBarAdapter.setFocusOnSearchView();
        }
    }

    @Override
    public DialogManager getDialogManager() {
        return mDialogManager;
    }

    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.floating_action_button:
                /**
                 * SPRD:
                 *
                 * @{
                 */
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving
                        || JoinContactsDialogFragment.isContactsJoining
                        || ContactMultiDeletionInteraction.isMultiContactsDeleting) {
                    Toast.makeText(PeopleActivity.this, R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_SHORT).show();
                    return;
                }
                /**
                 * @}
                 */
                Intent intent = new Intent(Intent.ACTION_INSERT, Contacts.CONTENT_URI);
                /**
                 * SPRD:Bug 429413 Website cannot be saved to SIM contacts.
                 *
                 * Original Android code:
                Bundle extras = getIntent().getExtras();
                if (extras != null) {
                    intent.putExtras(extras);
                }
                 */
                try {
                    ImplicitIntentsUtil.startActivityInApp(PeopleActivity.this, intent);
                } catch (ActivityNotFoundException ex) {
                    Toast.makeText(PeopleActivity.this, R.string.missing_app,
                            Toast.LENGTH_SHORT).show();
                }
                break;
        default:
            Log.wtf(TAG, "Unexpected onClick event from " + view);
        }
    }

    /**
     * Returns the tab position adjusted for the text direction.
     */
    private int getTabPositionForTextDirection(int position) {
        if (isRTL()) {
            return TabState.COUNT - 1 - position;
        }
        return position;
    }
    /* SPRD:Bug 474752 Add features with multiSelection activity in Contacts. @ { */
    private static final int SUBACTIVITY_BATCH_DELETE = 7;
    private static final int SUBACTIVITY_SHARE_VISIBLE = 10;
    private static final int SUBACTIVITY_SET_STARRED = 11;
    private static final int SUBACTIVITY_SHARE_BY_SMS = 12;
    /* @} */
    /**
     * SPRD:Bug474746 Import/Export SdCard vcf contacts.
     *
     * @{
     */
    private static final int MAX_DATA_SIZE = 150000;
    private static final int SUBACTIVITY_BATCH_IMPORT = 8;
    private static final int SUBACTIVITY_BATCH_EXPORT_TO_SDCARD = 9;
    public void doCopy() {
        Bundle args = new Bundle();
        SelectAccountDialogFragment.show(getFragmentManager(),
                R.string.copy_to,
                AccountListFilter.ACCOUNTS_CONTACT_WRITABLE,
                args);
    }

    public void doExport() {
        final Intent intent = new Intent(UiIntentActions.MULTI_PICK_ACTION);
        intent.setData(Contacts.CONTENT_URI);
        //SPRD:Bug 547600 Don't show default contact while export contact
        intent.putExtra("exclude_read_only", true);
        startActivityForResult(intent, SUBACTIVITY_BATCH_EXPORT_TO_SDCARD);
    }

    public void onAccountChosen(AccountWithDataSet account, Bundle extraArgs) {
        if (account != null) {
            this.doImport(account);
        }
    }

    public void onAccountSelectorCancelled() {
        // dismiss();
    }

    public void doPreImport(int resId, int subscriptionId) {
        if (hasExchangeAccount()) {
            ImportToAccountDialogFragment dialogFragment = ImportToAccountDialogFragment
                    .newInstance(resId);
            dialogFragment.show(getFragmentManager(), null);
        } else {
            AccountSelectionUtil.doImport(this, resId, AccountTypeManager.getInstance(this)
                    .getPhoneAccount(),subscriptionId);
        }
    }

    public void doImport(final AccountWithDataSet dstAccount) {
        if (dstAccount != null
                && (SimAccountType.ACCOUNT_TYPE.equals(dstAccount.type) || USimAccountType.ACCOUNT_TYPE
                        .equals(dstAccount.type))) {
            Bundle args = new Bundle();
            args.putParcelable("accounts", dstAccount);
            ConfirmCopyContactDialogFragment dialog =
                    new ConfirmCopyContactDialogFragment();
            dialog.setArguments(args);
            dialog.show(getFragmentManager(), null);
        } else {
            Intent intent = new Intent(UiIntentActions.MULTI_PICK_ACTION);
            intent.setData(Contacts.CONTENT_URI);
            intent.putExtra("dst_account", dstAccount);
            intent.putExtra("mode", SUBACTIVITY_BATCH_IMPORT);
            startActivityForResult(intent, SUBACTIVITY_BATCH_IMPORT);
        }
    }

    public static class ConfirmCopyContactDialogFragment extends DialogFragment {
        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            return new AlertDialog.Builder(getActivity())
                    .setPositiveButton(android.R.string.ok,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    final Intent intent = new Intent(UiIntentActions.MULTI_PICK_ACTION);
                                    intent.setData(Contacts.CONTENT_URI);
                                    AccountWithDataSet accountData = (AccountWithDataSet) getArguments()
                                            .getParcelable("accounts");
                                    intent.putExtra("dst_account", accountData);
                                    intent.putExtra("mode", SUBACTIVITY_BATCH_IMPORT);
                                    getActivity().startActivityForResult(intent,
                                            SUBACTIVITY_BATCH_IMPORT);
                                }
                            })
                    .setNegativeButton(android.R.string.cancel,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                }
                            })
                    .setTitle(android.R.string.dialog_alert_title)
                    .setMessage(R.string.alert_maybe_lost_info)
                    .create();
        }
    }

    public void doShareVisible() {
        final Intent intent = new Intent(UiIntentActions.MULTI_PICK_ACTION);
        intent.setData(Contacts.CONTENT_URI);
        startActivityForResult(intent, SUBACTIVITY_SHARE_VISIBLE);
    }

    private boolean hasExchangeAccount() {
        boolean hasExchangeAccount = false;
        AccountTypeManager atm = AccountTypeManager.getInstance(this);
        List<AccountWithDataSet> accounts = atm.getAccounts(true);
        for (AccountWithDataSet account : accounts) {
            if ("com.android.exchange".equals(account.type)
                    || "com.google.android.exchange".equals(account.type)
                    || CardDavAccountType.ACCOUNT_TYPE.equals(account.type)) {
                hasExchangeAccount = true;
                break;
            }
        }
        return hasExchangeAccount;
    }

    public static class ImportToAccountDialogFragment extends DialogFragment {

        public static ImportToAccountDialogFragment newInstance(int resId) {
            ImportToAccountDialogFragment fragment = new ImportToAccountDialogFragment();
            Bundle args = new Bundle();
            args.putInt("resId", resId);
            fragment.setArguments(args);
            return fragment;
        }

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            final Bundle args = getArguments();
            final int resId = args.getInt("resId");
            return AccountSelectionUtil.getSelectAccountDialog(getActivity(), resId, null, null,
                    true);
        }
    }

    /**
     * SPRD:Bug474772 Add SIM/USIM phonebook capacity feature in Contacts.
     *
     * @{
     */
    private static final int ADVANCED_OPTIONS = 10010;
    private static final int SIM_CAPACITY = 10011;
    private static final int CLEARUP = 10012;
    /**
     * @}
     */
    /**
     * SPRD Bug 474726、474752 Add features with multi-selection activity in Contacts.
     *
     * @{
     */
    private String shareContactBySms(HashMap<String, String> contacts){
        if (contacts == null || contacts.entrySet() == null) {
            Log.e(TAG,
                    "case REQUEST_CODE_SMS_SEND_CONTACT_AS_TEXT: contacts = null");
            return null;
        }

        StringBuilder text = new StringBuilder("");
        Iterator it = contacts.entrySet().iterator();
        while (it.hasNext()) {
            Entry entry = (Entry) it.next();
            if (entry.getValue() != null) {
                text.append((String) entry.getValue());
                text.append("\n");
            }
            if (entry.getKey() != null) {
                text.append((String) entry.getKey());
                text.append("\n");
            }
        }
        return text.toString();
    }
   /**
    * @}
    */
    /**
     * SPRD: 513110 Contact list disapper while rotate during join contact
     * SPRD: 602324 when join contact from Emer..info, there is crash.
     * SPRD: 604761 The DUT fails to dismiss the dialog even contact joining finished.
     * @{
     */
    private static IndeterminateProgressDialog mProgressDialog;

    public void onJoinStart() {
        Log.d("chentest", "PeopleActivity onJoinStart() mProgressDialog = "
                + mProgressDialog);
        if (mProgressDialog == null) {
            // SPRD:Bug 520655 change minDisplayTime from 500 to 0
            try {
                mProgressDialog = IndeterminateProgressDialog.show(
                        getFragmentManager(),
                        getString(R.string.merge_inprocess),
                        getString(R.string.wait), 0);
            } catch (Exception e) {
                Log.e(TAG,
                        "Callled from other app, listener is not null but PeopleActivity is not at top : "
                                + e.getMessage());
            }
        }
    }

    public void onJoinFinished() {
        if (mProgressDialog != null) {
            mProgressDialog.dismiss();
            mProgressDialog = null;
        }
    }

    public static IndeterminateProgressDialog getJoinContactDialog() {
        return mProgressDialog;
    }
    /**
     * @}
     */
}
