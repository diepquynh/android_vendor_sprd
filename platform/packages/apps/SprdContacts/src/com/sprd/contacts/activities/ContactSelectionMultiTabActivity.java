
package com.sprd.contacts.activities;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Set;
import java.util.Map.Entry;
import android.app.ActionBar;
import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.app.ActionBar.Tab;
import android.app.ActionBar.TabListener;
import android.content.BroadcastReceiver;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.Contacts;
import android.support.v13.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.support.v4.view.ViewPager.OnPageChangeListener;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.android.contacts.ContactsActivity;
import com.sprd.contacts.group.GroupBrowseListFragmentSprd;
import com.android.contacts.group.GroupBrowseListFragment.OnGroupBrowserActionListener;
import com.sprd.contacts.list.AllInOneBrowserPickerFragment;
import com.sprd.contacts.list.AllInOneCallLogPickerFragment;
import com.sprd.contacts.list.AllInOneDataPickerFragment;
import com.sprd.contacts.list.AllInOneFavoritesPickerFragment;
import com.android.contacts.common.activity.RequestPermissionsActivity;
import com.android.contacts.common.list.ContactEntryListAdapter;
import com.android.contacts.common.list.ContactEntryListFragment;
import com.android.contacts.common.list.ContactListFilter;
import com.android.contacts.common.list.ContactListFilterController;
import com.android.contacts.list.ContactTileListFragment;
import com.android.contacts.list.ContactsIntentResolver;
import com.android.contacts.list.ContactsRequest;
import com.android.contacts.list.DefaultContactBrowseListFragment;
import com.android.contacts.list.UiIntentActions;
import com.sprd.contacts.list.OnAllInOneDataMultiPickerActionListener;
import com.android.contacts.common.list.PhoneNumberPickerFragment;
import com.android.contacts.R;
import com.android.contacts.common.util.AccountFilterUtil;
import com.android.contacts.common.list.ViewPagerTabs;
import com.android.contacts.activities.ActionBarAdapter;
import android.widget.Toolbar;
import android.view.Menu;
import android.view.MenuInflater;
import android.text.TextUtils;
import android.app.ActionBar.LayoutParams;
import java.util.Locale;
import android.view.ViewGroup;
import android.os.Parcelable;
import java.util.HashSet;
import android.os.SystemProperties;
import com.android.internal.telephony.TelephonyIntents;
import com.android.ims.ImsManager;
/**
 * Bug515797 If ContactSelectionMultiTabActivity is lunched and contact has no permission,
 * contacts can't be selected.
 * @{
 */
import android.content.ComponentName;
import android.os.Handler;
import android.os.Looper;
import java.util.ArrayList;
import android.telephony.PhoneNumberUtils;
/**
 * @}
 */

public class ContactSelectionMultiTabActivity extends ContactsActivity implements ActionBarAdapter.Listener{
    private static final String TAG = "MultiTabContactSelectionActivity";

    private static final int TAB_INDEX_GROUP_NEWUI = 0;
    private static final int TAB_INDEX_FAVORITES_NEWUI = 1;
    private static final int TAB_INDEX_ALL_NEWUI = 2;
    private static final int TAB_INDEX_CALLLOG_NEWUI = 3;

    private static final int TAB_INDEX_COUNT_NEWUI = 4;

    private static final int REQUEST_CODE_PICK = 1;

    private static final String KEY_TAB = "tab_position";

    private ViewPager mViewPager;
    private PageChangeListener mPageChangeListener = new PageChangeListener();

    private GroupBrowseListFragmentSprd mGroupBrowseListFragment;
    private AllInOneFavoritesPickerFragment mFavoriteFragment;
    private AllInOneBrowserPickerFragment mAllInOneDataPickerFragment;
    private AllInOneCallLogPickerFragment mCallLogFragment;

    private ContactsIntentResolver mIntentResolver;
    private ContactsRequest mRequest;
    private ContactListFilterController mContactListFilterController;
    private ContactListFilter mFilter = null;

    private boolean mMultiSupport = false;
    private boolean mIsFirstEnter = true;

    private Button mDoneMenuItem;
    private boolean mDoneEnable = false;
    private boolean mDoneSplitEnable = false;//SPRD:add  for bug614908
    private int mDoneMenuDisableColor = Color.WHITE;
    private int mCurrentTabPosition = -1;
    private BroadcastReceiver mSelecStatusReceiver;
    //SPRD:add newfuture for bug423428
    private static final String BLACK_CALL_LIST_ACTION = "com.sprd.blacklist.action";
    /*
     * SPRD:
     * For Bug 377703 and 382921
     * @{
     */
    private static final String KEY_SEARCH_MODE = "searchMode";
    private static final String KEY_SEARCH_TAB = "tab_search";
    private static final String KEY_DONE_ENABLE = "done_enable";
    private ViewPagerTabs mViewPagerTabs;
    private String[] mTabTitles;
    private boolean showsearchmenu = false;
    private boolean mIsSearchMode = false;
    private MultiTabViewPagerAdapter mViewPagerAdapter;
    private int searchOnTab = -1;
    private ActionBarAdapter mActionBarAdapter;
    private View customActionBarView;
    /*
     * @}
     */
    /**
     * Bug515797 If ContactSelectionMultiTabActivity is lunched and contact has no permission,
     * contacts can't be selected.
     * @{
     */
    private static boolean mIsNeedPromptPermissionMessage = false;
    private Handler mMainHandler;
    /**
     * @}
     */

    /**
     * SPRD:Bug519952,523978 "Done" button is grey while some contacts is chosen
     *
     * @{
     */
    private static Boolean mFavoriteDoneEnable = false;
    private static Boolean mAllInOneDoneEnable = false;
    private static Boolean mCallLogDoneEnable = false;
    private boolean mCallFireWallActivityCalling = false;
    private HashMap<String, String> mDataAll = new HashMap<String, String>();
    private HashMap<String, String> mGroupBrowseListData = new HashMap<String, String>();
    /**
     * @}
     */

    // SPRD:add for volte
    private static final int MIN_CONTACT_COUNT = 2;

    /**
     * SPRD bug535541 During inviting contacts from Contacts group from conference
     * call, could select more than three contacts
     * @{
     */
    public static final int CHECKED_ITEMS_MAX = 3500;
    public int mCheckedLimitCount = 3500 ;
    /**
     * @}
     */
    // add for Bug543785 scroll to CallLog,select all,rotate phone and scroll to other tab the
    // choosen contacts can't be added blacklist
    private int mCurrentPosition = -1;
    private int mNextPosition = -1;
    public ContactSelectionMultiTabActivity() {
        mIntentResolver = new ContactsIntentResolver(this);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        /**
         * Bug515797 If ContactSelectionMultiTabActivity is lunched and contact has no permission,
         * contacts can't be selected.
         * @{
         */
        mMainHandler = new Handler(Looper.getMainLooper());
        ComponentName callingActivity = getCallingActivity();
        /**
         * SPRD: Bug 532299 The dut can not add contacts for conference call
         * after selecting contacts and switching the tags.
         * @{
         */
        String intentParam = getIntent().getStringExtra("multi");
        if (intentParam != null && (intentParam.equals("addMultiCall")
                || intentParam.equals("addMultiCallAgain"))) {
            mCallFireWallActivityCalling = true;
        }
        /*
         * {@
         */
        if (callingActivity != null) {
            String className = callingActivity.getShortClassName();
            /**
             * SPRD:Bug519952 "Done" button is grey while some contacts is chosen
             * @{
             */
            if (className.endsWith("CallFireWallActivity")) {
                mCallFireWallActivityCalling = true;
            }
            /**
             * @}
             */
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
        /**
         * Bug515797 If ContactSelectionMultiTabActivity is lunched and contact has no permission,
         * contacts can't be selected.
         * @{
         */
        mIsNeedPromptPermissionMessage = false;
        /**
         * @}
         */
        if (savedInstanceState != null) {
            mCurrentTabPosition = savedInstanceState.getInt(KEY_TAB);
            //SPRD:Bug543785 reacquire current tab while rotate the phone
            mCurrentPosition = savedInstanceState.getInt(KEY_TAB);
            mIsFirstEnter = false;
            /* SPRD: Add search menu for MultiTab Activity * @{*/
            mIsSearchMode = savedInstanceState.getBoolean(KEY_SEARCH_MODE);
            searchOnTab = savedInstanceState.getInt(KEY_SEARCH_TAB);
            mDoneEnable = savedInstanceState.getBoolean(KEY_DONE_ENABLE);
            //SPRD:Bug614908 "Done" button is grey when split screen
            mDoneSplitEnable = mDoneEnable;
            /* @} */
        }
        /**
         * SPRD:Bug 522240 Done menu is bright while there is no contactselected
         *
         * @{
         */
        else {
            mFavoriteDoneEnable = false;
            mCallLogDoneEnable = false;
            mAllInOneDoneEnable = false;
        }
        /*
         * @}
         */
        // Extract relevant information from the intent
        mRequest = mIntentResolver.resolveIntent(getIntent());
        /**
         * SPRD:Bug462535 Add intent null value judgment.
         * @{
         */
        if (mRequest == null || mRequest.getCascadingData() == null) {
            Log.d(TAG," onCreate missing required data");
            finish();
        }
        /**
         * @}
         */

        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        mContactListFilterController = ContactListFilterController.getInstance(this);
        mContactListFilterController.checkFilterValidity(false);
        mFilter = mContactListFilterController.getFilter();

        configureActivityTitle();
        /*
         * SPRD:
         * Bug 377703 Add MultiTab selection activity UI feature.
         * @{
         */
        setContentView(R.layout.selection_activity_overlay);
        findViewById(R.id.selection_list_container).addOnLayoutChangeListener(
                mOnLayoutChangeListener);
        /*
         * @}
         */
        mViewPager = (ViewPager) findViewById(R.id.pager);
        mViewPagerAdapter = new MultiTabViewPagerAdapter(getFragmentManager());
        if (mViewPager != null) {
            mViewPager.setAdapter(mViewPagerAdapter);
            mViewPager.setOnPageChangeListener(mPageChangeListener);
        }
        /* SPRD: Bug 377703 and 382921  * @{*/
        createTabViews(savedInstanceState);
        prepareActionBar();
        /* @} */
        if (mViewPager != null && mCurrentTabPosition != -1) {
            mViewPager.setCurrentItem(mCurrentTabPosition);
        }
        // add for SPRD:Bug 565982 set pre-loaded pages from 4 to 1
        if (mCallFireWallActivityCalling) {
            mViewPager.setOffscreenPageLimit(4);
        }
    }

    // add for SPRD:Bug 547154 more than designated contacs can be selected while merge calls during
    // contacts inviting process
    @Override
    protected void onNewIntent(Intent intent) {
        setIntent(intent);
        // add for SPRD:Bug 564314 clear data after merge calls and return to invite tabs
        clearDataAll();
        updateDoneMenu();
    }

    /**
     * Bug515797 If ContactSelectionMultiTabActivity is lunched and contact has no permission,
     * contacts can't be selected.
     * @param className the activity name who invoke ContactSelectionMultiTabActivity
     */
    private void needPromptMessage(String className) {
        if (className.endsWith("CallFireWallActivity")) {
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
                Toast.makeText(ContactSelectionMultiTabActivity.this, message, Toast.LENGTH_LONG).show();
            }
        });
    }


    /*
     * SPRD:
     * Bug 377703 and 382921.
     * @{
     */
    private void createTabViews(Bundle savedInstanceState) {
        final FragmentManager fragmentManager = getFragmentManager();
        final FragmentTransaction transaction = fragmentManager.beginTransaction();
        mTabTitles = new String[TAB_INDEX_COUNT_NEWUI];
        mTabTitles[TAB_INDEX_GROUP_NEWUI] = getString(R.string.contactsGroupsLabel);
        mTabTitles[TAB_INDEX_FAVORITES_NEWUI] = getString(R.string.contactsFavoritesLabel);
        mTabTitles[TAB_INDEX_ALL_NEWUI] = getString(R.string.people);
        mTabTitles[TAB_INDEX_CALLLOG_NEWUI] = getString(R.string.recentCallsIconLabel);
        final ViewPagerTabs portraitViewPagerTabs
                = (ViewPagerTabs) findViewById(R.id.selection_lists_pager_header);
        ViewPagerTabs landscapeViewPagerTabs = null;
        final Toolbar toolbar = getView(R.id.selection_toolbar);
        setActionBar(toolbar);
        if (portraitViewPagerTabs == null) {
            landscapeViewPagerTabs = (ViewPagerTabs) getLayoutInflater().inflate(
                    R.layout.people_activity_tabs_lands, toolbar, /* attachToRoot = */false);
            mViewPagerTabs = landscapeViewPagerTabs;
        } else {
            mViewPagerTabs = portraitViewPagerTabs;
        }
        mViewPagerTabs.setViewPager(mViewPager);

        mActionBarAdapter = new ActionBarAdapter(this, this, getActionBar(), portraitViewPagerTabs, landscapeViewPagerTabs, toolbar);
        mActionBarAdapter.initialize(savedInstanceState, mRequest);
    }
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.search_menu, menu);
        final MenuItem searchItem = menu.findItem(R.id.menu_search);
        searchItem.setVisible(!mIsSearchMode);
        return true;
    }
    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        final MenuItem searchItem = menu.findItem(R.id.menu_search);
        if (mIsSearchMode) {
            searchItem.setVisible(false);
            customActionBarView.setVisibility(View.GONE);
        } else {
            if (customActionBarView.getVisibility() == View.GONE) {
                prepareActionBar();
            }
            switch (mViewPager.getCurrentItem()) {
                case TAB_INDEX_ALL_NEWUI:
                    searchItem.setVisible(true);
                    break;
                case TAB_INDEX_FAVORITES_NEWUI:
                    searchItem.setVisible(true);
                    break;
                case TAB_INDEX_GROUP_NEWUI:
                    searchItem.setVisible(false);
                    break;
                case TAB_INDEX_CALLLOG_NEWUI:
                    searchItem.setVisible(false);
                    break;
            }
        }
        return true;
    }
    @Override
    public void onUpButtonPressed() {
        onBackPressed();
    }
    @Override
    public void onSelectedTabChanged() {
      updateFragmentsVisibility();
  }
    @Override
    public void onAction(int action) {
        switch (action) {
            case ActionBarAdapter.Listener.Action.START_SEARCH_MODE:
                updateFragmentsVisibility();
                invalidateOptionsMenu();
                break;
            case ActionBarAdapter.Listener.Action.STOP_SEARCH_AND_SELECTION_MODE:
                setQueryTextToFragment("");
                updateFragmentsVisibility();
                invalidateOptionsMenu();
                break;
            case ActionBarAdapter.Listener.Action.CHANGE_SEARCH_QUERY:
                final String queryString = mActionBarAdapter.getQueryString();
                setQueryTextToFragment(queryString);
                break;
            default:
                throw new IllegalStateException("Unkonwn ActionBarAdapter action: " + action);
        }
    }
    @SuppressWarnings("unchecked")
	@Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        if (mActionBarAdapter.isSearchMode()) {
            mActionBarAdapter.setFocusOnSearchView();
        }
    }
    @Override
    protected void onStop() {
        super.onStop();
    }
    @Override
    protected void onDestroy() {
        if (mActionBarAdapter != null) {
            mActionBarAdapter.setListener(null);
        }
        super.onDestroy();
    }
    @Override
    public void onBackPressed() {
        if (mActionBarAdapter.isSearchMode()) {
            mIsSearchMode = false;
            boolean mDoneEnabled = (searchOnTab == TAB_INDEX_FAVORITES_NEWUI) ? mFavoriteFragment
                    .getCheckedItemNum() > 0 : mAllInOneDataPickerFragment.getCheckedItemNum() > 0;
            setDoneMenu(mDoneEnabled);
            mActionBarAdapter.setSearchMode(mIsSearchMode);
        } else if (isTaskRoot()) {
            finish();
        } else {
            super.onBackPressed();
        }
    }

    private void updateFragmentsVisibility() {
        int tab = mActionBarAdapter.getCurrentTab();
        if (mActionBarAdapter.isSearchMode()) {
            mViewPagerAdapter.setSearchMode(true);
        } else {
            final boolean wasSearchMode = mViewPagerAdapter.isSearchMode();
            mViewPagerAdapter.setSearchMode(false);
        }
        return;
    }
    /*
     * @}
     */
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        //add for Bug550820 return from search tab, tab isn't updated
        outState.putInt(KEY_TAB, mCurrentPosition);
        /**
         * SPRD bug382921 Add search menu for MultiTab Activity
         * SPRD bug523978,538887 During adding contacts to blacklist, contacts checked has not
         * added into blacklist successfully
         * @{
         */
        outState.putBoolean(KEY_SEARCH_MODE, mIsSearchMode);
        outState.putInt(KEY_SEARCH_TAB,searchOnTab);
        outState.putBoolean(KEY_DONE_ENABLE, mDoneEnable);
        mActionBarAdapter.onSaveInstanceState(outState);
        mActionBarAdapter.setListener(null);
        if (mViewPager != null) {
            mViewPager.setOnPageChangeListener(null);
        }
        /**
         * @}
         */
    }

    @Override
    protected void onStart() {
        super.onStart();
        if (mViewPager != null && mIsFirstEnter) {
            mViewPager.setCurrentItem(TAB_INDEX_ALL_NEWUI);
            mIsFirstEnter = false;
        }

    }

    public class SSUReceiver extends BroadcastReceiver{

        public void onReceive(final Context context, final Intent intent) {
            updateDoneMenu();

        }
    }

    // add for SPRD:Bug 568238 UpdateDoneMenu when page selected or SSU is received
    public void updateDoneMenu() {
        int currentTab = mViewPager != null ? mViewPager.getCurrentItem() : -1;
        if (currentTab != -1) {
            Fragment currentFragment = getFragmentAt(currentTab);
            if (currentFragment != null) {
                if (currentFragment instanceof GroupBrowseListFragmentSprd) {
                    // add for Bug526242 set Done menu of group tab gray
                    mDoneEnable = false;
                    setDoneMenu(mDoneEnable);
                } else {
                    mFavoriteDoneEnable = (mFavoriteFragment != null) ? mFavoriteFragment
                            .getAdapter().hasCheckedItems() : false;
                    mCallLogDoneEnable = mCallLogFragment != null ? mCallLogFragment.getAdapter()
                            .hasCheckedItems() : false;
                    mAllInOneDoneEnable = mAllInOneDataPickerFragment != null ? mAllInOneDataPickerFragment
                            .getAdapter().hasCheckedItems() : false;
                    //SPRD:Bug614908 "Done" button is grey when split screen
                    mDoneEnable = mFavoriteDoneEnable || mCallLogDoneEnable || mAllInOneDoneEnable||mDoneSplitEnable;
                    mDoneSplitEnable = false;
                }
                    //SPRD:Bug519952 "Done" button is grey when contacts is chosen
                    setDoneMenu(mDoneEnable);
                }
            }
    }
    @Override
    protected void onResume() {
        super.onResume();
        IntentFilter filter = new IntentFilter("com.android.contacts.common.action.SSU");
        mSelecStatusReceiver = new SSUReceiver();
        registerReceiver(mSelecStatusReceiver, filter);
        // add for SPRD:Bug 547154,564316 more than designated contacs can be selected while merge
        // calls during contacts inviting process
        int mIntentChecknum = getIntent().getIntExtra("checked_limit_count", CHECKED_ITEMS_MAX);
        int mFavorNum = mFavoriteFragment == null ? 0 : mFavoriteFragment.getCheckedItemNum();
        int mCallLogNum = mCallLogFragment == null ? 0 : mCallLogFragment.getCheckedItemNum();
        int mAllInOneNum = mAllInOneDataPickerFragment == null ? 0 : mAllInOneDataPickerFragment
                .getCheckedItemNum();
        int mCheckedNum = mFavorNum + mCallLogNum + mAllInOneNum;
        mCheckedLimitCount = (mIntentChecknum != CHECKED_ITEMS_MAX) ? (mIntentChecknum - mCheckedNum)
                : mIntentChecknum;
        /* SPRD: Bug 382921 Add search menu for MultiTab Activity * @{ */
        mActionBarAdapter.setListener(this);
        if (mViewPager != null) {
            mViewPager.setOnPageChangeListener(mPageChangeListener);
        }
        updateFragmentsVisibility();
        /* @} */
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(mSelecStatusReceiver);
    }

    @Override
    public void onAttachFragment(Fragment fragment) {
        if (fragment instanceof GroupBrowseListFragmentSprd) {
            if (mGroupBrowseListFragment == null) {
                mGroupBrowseListFragment = (GroupBrowseListFragmentSprd) fragment;
                setupActionListener(TAB_INDEX_GROUP_NEWUI);
            }
        } else if (fragment instanceof AllInOneFavoritesPickerFragment) {
            if (mFavoriteFragment == null) {
                mFavoriteFragment = (AllInOneFavoritesPickerFragment) fragment;
                setupActionListener(TAB_INDEX_FAVORITES_NEWUI);
            }
        } else if (fragment instanceof AllInOneCallLogPickerFragment) {
            if (mCallLogFragment == null) {
                mCallLogFragment = (AllInOneCallLogPickerFragment) fragment;
                setupActionListener(TAB_INDEX_CALLLOG_NEWUI);
            }
        } else if (fragment instanceof AllInOneBrowserPickerFragment) {
            if (mAllInOneDataPickerFragment == null) {
                mAllInOneDataPickerFragment = (AllInOneBrowserPickerFragment) fragment;
                setupActionListener(TAB_INDEX_ALL_NEWUI);
            }
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                // Go back to previous screen, intending "cancel"
                setResult(RESULT_CANCELED);
                finish();
                return true;
            /* SPRD: Bug 382921 Add search menu for MultiTab Activity * @{*/
            case R.id.menu_search:
                searchOnTab = mViewPager.getCurrentItem();
                mIsSearchMode = !mIsSearchMode;
                mActionBarAdapter.setSearchMode(true);
                // add for SPRD:Bug 569121 Clear chosen items in other fragment when search
                if (mCallLogFragment != null) {
                    mCallLogFragment.clearCheckedItem();
                }
                if (searchOnTab == TAB_INDEX_FAVORITES_NEWUI && mAllInOneDataPickerFragment != null) {
                    mAllInOneDataPickerFragment.clearCheckedItem();
                }
                if (searchOnTab == TAB_INDEX_ALL_NEWUI && mFavoriteFragment != null) {
                    mFavoriteFragment.clearCheckedItem();
                }
                int mIntentChecknum = getIntent().getIntExtra("checked_limit_count",
                        CHECKED_ITEMS_MAX);
                int mFavorNum = mFavoriteFragment == null ? 0 : mFavoriteFragment
                        .getCheckedItemNum();
                int mCallLogNum = mCallLogFragment == null ? 0 : mCallLogFragment
                        .getCheckedItemNum();
                int mAllInOneNum = mAllInOneDataPickerFragment == null ? 0
                        : mAllInOneDataPickerFragment
                                .getCheckedItemNum();
                int mCheckedNum = mFavorNum + mCallLogNum + mAllInOneNum;
                mCheckedLimitCount = (mIntentChecknum != CHECKED_ITEMS_MAX) ? (mIntentChecknum - mCheckedNum)
                        : mIntentChecknum;
                customActionBarView.setVisibility(View.GONE);
                return true;
                /* @} */
        }
        return super.onOptionsItemSelected(item);
    }

    public class MultiTabViewPagerAdapter extends FragmentPagerAdapter {
        /* SPRD: Bug 382921 Add search menu for MultiTab Activity * @{*/
        private boolean mTabPagerAdapterSearchMode;
        private FragmentTransaction mCurTransaction = null;
        private Fragment mCurrentPrimaryItem;
        private final FragmentManager mFragmentManager;
        /* @} */
        public MultiTabViewPagerAdapter(FragmentManager fragmentManager) {
            super(fragmentManager);
            mFragmentManager = getFragmentManager();
        }

        @Override
        public Fragment getItem(int position) {
            switch (position) {
                case TAB_INDEX_GROUP_NEWUI:
                    mGroupBrowseListFragment = new GroupBrowseListFragmentSprd();
                    configTabAdapter(TAB_INDEX_GROUP_NEWUI);
                    return mGroupBrowseListFragment;
                case TAB_INDEX_FAVORITES_NEWUI:
                    mFavoriteFragment = new AllInOneFavoritesPickerFragment();
                    configTabAdapter(TAB_INDEX_FAVORITES_NEWUI);
                    return mFavoriteFragment;
                case TAB_INDEX_ALL_NEWUI:
                    mAllInOneDataPickerFragment = new AllInOneBrowserPickerFragment();
                    configTabAdapter(TAB_INDEX_ALL_NEWUI);
                    return mAllInOneDataPickerFragment;
                case TAB_INDEX_CALLLOG_NEWUI:
                    mCallLogFragment = new AllInOneCallLogPickerFragment();
                    configTabAdapter(TAB_INDEX_CALLLOG_NEWUI);
                    return mCallLogFragment;
            }
            throw new IllegalStateException("No fragment at position "
                    + position);
        }
        /*
         * SPRD:
         * Bug 377703 and 382921
         * @{
         */
        public boolean isSearchMode() {
            return mTabPagerAdapterSearchMode;
        }
        public void setSearchMode(boolean searchMode) {
            if (searchMode == mTabPagerAdapterSearchMode) {
                return;
            }
            mTabPagerAdapterSearchMode = searchMode;
            notifyDataSetChanged();
        }
        @Override
        public int getCount() {
            return mTabPagerAdapterSearchMode ? 1 : TAB_INDEX_COUNT_NEWUI;
        }
        @Override
        public int getItemPosition(Object object) {
            if (mTabPagerAdapterSearchMode) {
                if (searchOnTab == TAB_INDEX_FAVORITES_NEWUI) {
                    if (object == mFavoriteFragment) {
                        return 0;
                    }
                } else if (searchOnTab == TAB_INDEX_ALL_NEWUI) {
                    if (object == mAllInOneDataPickerFragment) {
                        return 0;
                    }
                }
            } else {
                if (object == mAllInOneDataPickerFragment) {
                    return getTabPositionForTextDirection(TAB_INDEX_ALL_NEWUI);
                }
                if (object == mGroupBrowseListFragment) {
                    return getTabPositionForTextDirection(TAB_INDEX_GROUP_NEWUI);
                }
                if (object == mCallLogFragment) {
                    return getTabPositionForTextDirection(TAB_INDEX_CALLLOG_NEWUI);
                }
                if (object == mFavoriteFragment) {
                    return getTabPositionForTextDirection(TAB_INDEX_FAVORITES_NEWUI);
                }
            }
            return POSITION_NONE;
        }
        private Fragment getFragment(int position) {
            position = getTabPositionForTextDirection(position);
            if (mTabPagerAdapterSearchMode) {
                if (position != 0) {
                    Log.w(TAG, "Request fragment at position=" + position + ", eventhough we " +
                            "are in search mode");
                }
                if (searchOnTab == TAB_INDEX_FAVORITES_NEWUI) {
                    return mFavoriteFragment;
                }
                return mAllInOneDataPickerFragment;
            } else {
                if (position == TAB_INDEX_FAVORITES_NEWUI) {
                    return mFavoriteFragment;
                } else if (position == TAB_INDEX_ALL_NEWUI) {
                    return mAllInOneDataPickerFragment;
                } else if (position == TAB_INDEX_GROUP_NEWUI) {
                    return mGroupBrowseListFragment;
                } else if (position == TAB_INDEX_CALLLOG_NEWUI) {
                    return mCallLogFragment;
                }
            }
            throw new IllegalArgumentException("position: " + position);
        }
        @Override
        public Object instantiateItem(ViewGroup container, int position) {
            if (mTabPagerAdapterSearchMode) {
               if (mCurTransaction == null) {
                  mCurTransaction = mFragmentManager.beginTransaction();
               }
               Fragment f = getFragment(position);
               mCurTransaction.show(f);
               f.setUserVisibleHint(f == mCurrentPrimaryItem);
               return f;
            } else {
               return super.instantiateItem(container,position);
            }
        }
        @Override
        public CharSequence getPageTitle(int position) {
            return mTabTitles[position];
        }
        /*
         * @}
         */
    }
    /*
     * SPRD:
     * Bug 377703 and 382921
     * @{
     */
    private void setQueryTextToFragment(String query) {
        if (searchOnTab == TAB_INDEX_FAVORITES_NEWUI) {
            mFavoriteFragment.setQueryString(query, true);
            mFavoriteFragment.setSearchViewText(query);
            mFavoriteFragment.setVisibleScrollbarEnabled(!mFavoriteFragment.isSearchMode());
        } else if (searchOnTab == TAB_INDEX_ALL_NEWUI) {
            mAllInOneDataPickerFragment.setQueryString(query, true);
            mAllInOneDataPickerFragment.setSearchViewText(query);
            mAllInOneDataPickerFragment.setVisibleScrollbarEnabled(!mAllInOneDataPickerFragment.isSearchMode());
        }
    }
    private int getTabPositionForTextDirection(int position) {
        if (isRTL()) {
            return TAB_INDEX_COUNT_NEWUI - 1 - position;
        }
        return position;
    }
    private boolean isRTL() {
        final Locale locale = Locale.getDefault();
        return TextUtils.getLayoutDirectionFromLocale(locale) == View.LAYOUT_DIRECTION_RTL;
    }
    private void showStripForTab(int tab) {
        mViewPagerTabs.onPageScrolled(tab, 0, 0);
    }
    /*
     * @}
     */
    private class PageChangeListener implements OnPageChangeListener {
        @Override
        public void onPageScrolled(int position, float positionOffset,
                int positionOffsetPixels) {
            /*
             * SPRD:
             * Bug 377703 Add MultiTab selection activity UI feature.
             * @{
             */
            if (!mViewPagerAdapter.isSearchMode()) {
               mViewPagerTabs.onPageScrolled(position, positionOffset, positionOffsetPixels);
            }
            /*
             * @}
             */
        }

        @Override
        public void onPageSelected(int position) {
            /*
             * SPRD:
             * Bug 377703 Add MultiTab selection activity UI feature.
             * @{
             */
            if (!mViewPagerAdapter.isSearchMode()) {
              mViewPagerTabs.onPageSelected(position);
              showStripForTab(position);
              //SPRD:516723 empty toast disappear while change between differnent fragements
              //clearData(mNextPosition);
            }
            /*
             * @}
             */
            if (mCurrentPosition == position) {
                Log.w(TAG, "Previous position and next position became same ("
                        + position + ")");
            }
            /**
             * SPRD:Bug526242,565982 set Done menu of group tab gray
             *
             * @{
             */
            if (position == 0) {
                mDoneEnable = false;
                setDoneMenu(mDoneEnable);
            }
            else {
                updateDoneMenu();
            }
            /**
             * @}
             */
            mNextPosition = position;
        }

        public void setCurrentPosition(int position) {
            mCurrentPosition = position;
        }

        @Override
        public void onPageScrollStateChanged(int status) {
            /*
             * SPRD:
             * Bug 377703 Add MultiTab selection activity UI feature.
             * @{
             */
            if (!mViewPagerAdapter.isSearchMode()) {
               mViewPagerTabs.onPageScrollStateChanged(status);
            }
            /*
             * @}
             */
            switch (status) {
                case ViewPager.SCROLL_STATE_IDLE:
                    if (mCurrentPosition >= 0) {
                        sendFragmentVisibilityChange(mCurrentPosition, false);
                    }
                    if (mNextPosition >= 0) {
                        sendFragmentVisibilityChange(mNextPosition, true);
                    }
                    invalidateOptionsMenu();

                    mCurrentPosition = mNextPosition;
                    break;
                case ViewPager.SCROLL_STATE_DRAGGING:
                case ViewPager.SCROLL_STATE_SETTLING:
                default:
                    break;
            }

        }
    }

    private void sendFragmentVisibilityChange(int position, boolean visibility) {
        if (position >= 0) {
            final Fragment fragment = getFragmentAt(position);
            if (fragment != null) {
                fragment.setMenuVisibility(visibility);
                fragment.setUserVisibleHint(visibility);
            }
        }
    }

    private Fragment getFragmentAt(int position) {
        if(!mIsSearchMode) {
            switch (position) {
                case TAB_INDEX_GROUP_NEWUI:
                    return mGroupBrowseListFragment;
                case TAB_INDEX_FAVORITES_NEWUI:
                    return mFavoriteFragment;
                case TAB_INDEX_ALL_NEWUI:
                    return mAllInOneDataPickerFragment;
                case TAB_INDEX_CALLLOG_NEWUI:
                    return mCallLogFragment;
                default:
                    throw new IllegalStateException("Unknown fragment index: " + position);
            }
        } else {
            if (position == TAB_INDEX_FAVORITES_NEWUI) {
                return mFavoriteFragment;
            } else {
                return mAllInOneDataPickerFragment;
            }
        }
    }

    private void configureActivityTitle() {
        setTitle(R.string.contactPickerActivityTitle);
    }

    private void prepareActionBar() {
        final ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            LayoutInflater inflater = (LayoutInflater) getSystemService
                    (Context.LAYOUT_INFLATER_SERVICE);
            customActionBarView = inflater.inflate(R.layout.editor_custom_action_bar_overlay, null);
            mDoneMenuItem = (Button) customActionBarView.findViewById(R.id.save_menu_item_button);
            mDoneMenuDisableColor = mDoneMenuItem.getCurrentTextColor();
            setDoneMenu(mDoneEnable);
            mDoneMenuItem.setText(R.string.menu_done);
            mDoneMenuItem.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    configCompleteListener(mViewPager != null ? mViewPager.getCurrentItem() : -1);

                }
            });
            View cancelMenuItem = customActionBarView.findViewById(R.id.cancel_menu_item_button);
            cancelMenuItem.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    onBackPressed();
                }
            });
            actionBar.setCustomView(customActionBarView, new ActionBar.LayoutParams(
                    ActionBar.LayoutParams.WRAP_CONTENT,
                    ActionBar.LayoutParams.WRAP_CONTENT, Gravity.CENTER_VERTICAL | Gravity.END));
            /* SPRD: Bug 382921 Add search menu for MultiTab Activity * @{*/
            if (!mIsSearchMode) {
               actionBar.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM | ActionBar.DISPLAY_USE_LOGO
                      | ActionBar.DISPLAY_SHOW_TITLE | ActionBar.DISPLAY_HOME_AS_UP);
            } else {
                actionBar.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM | ActionBar.DISPLAY_USE_LOGO
                        | ActionBar.DISPLAY_SHOW_TITLE);
            }
            /* @} */
        }
    }

    private final View.OnLayoutChangeListener mOnLayoutChangeListener = new
            View.OnLayoutChangeListener() {
                @Override
                public void onLayoutChange(View v, int left, int top, int right,
                        int bottom, int oldLeft, int oldTop, int oldRight, int oldBottom) {
                    v.removeOnLayoutChangeListener(this);
                }
            };

    /*
     * For Touch Assistant and KEYCODE_SEARCH event.
     */
    @Override
    public boolean onSearchRequested() {
        searchOnTab = mViewPager.getCurrentItem();
        // there is not support quick search in group and calllog page.
        switch (searchOnTab) {
            case TAB_INDEX_GROUP_NEWUI:
                return true;
            case TAB_INDEX_CALLLOG_NEWUI:
                return true;
            default:
                break;
        }
        mIsSearchMode = !mIsSearchMode;
        mActionBarAdapter.setSearchMode(true);
        customActionBarView.setVisibility(View.GONE);
        return true;
    }

    private final class GroupBrowserActionListener implements OnGroupBrowserActionListener {

        @Override
        public void onViewGroupAction(Uri groupUri) {
            /**
             * SPRD: Bug 531368 The call screen becomes abnormal after inviting email
             * contact of the group.
             * {@
             */
            String intentParam = getIntent().getStringExtra("multi");
            if ((getIntent().getStringExtra("blackcall_type") != null)
                    || (intentParam != null && (intentParam.equals("addMultiCall") || intentParam
                            .equals("addMultiCallAgain")))) {
            /*
             * @}
             */
                Intent intent = new Intent(UiIntentActions.MULTI_PICK_ACTION).
                        putExtra("cascading",new Intent(UiIntentActions.MULTI_PICK_ACTION).setType(Phone.CONTENT_ITEM_TYPE));
                intent.putExtra("select_group_member", groupUri != null ? ContentUris.parseId(groupUri): -1);
                intent.putExtra("with_phone_number", 1);
                /**
                 * SPRD bug535541 During inviting contacts from Contacts group from conference
                 * call, could select more than three contacts
                 * @{
                 */
                if (getIntent().getIntExtra("checked_limit_count", -1) > 0) {
                    intent.putExtra("checked_limit_count", getIntent().getIntExtra("checked_limit_count", CHECKED_ITEMS_MAX));
                }
                /**
                 * @}
                 */
                startActivityForResult(intent, REQUEST_CODE_PICK);
            } else {
                Intent intent = new Intent(UiIntentActions.MULTI_PICK_ACTION).
                        putExtra(
                                "cascading",
                                new Intent(UiIntentActions.MULTI_PICK_ACTION).setType(Phone.CONTENT_ITEM_TYPE).
                                        putExtra(
                                                "cascading",
                                                new Intent(UiIntentActions.MULTI_PICK_ACTION)
                                                        .setType(Email.CONTENT_ITEM_TYPE)));
                intent.putExtra("select_group_member", groupUri != null ? ContentUris.parseId(groupUri)
                        : -1);
                /**
                 * SPRD bug535541 During inviting contacts from Contacts group from conference
                 * call, could select more than three contacts
                 * @{
                 */
                if (getIntent().getIntExtra("checked_limit_count", -1) > 0) {
                    intent.putExtra("checked_limit_count", getIntent().getIntExtra("checked_limit_count", CHECKED_ITEMS_MAX));
                }
                /**
                 * @}
                 */
                startActivityForResult(intent, REQUEST_CODE_PICK);
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        /**
         * SPRD:Bug 523290 The Dut can not make a conference call after selecting the group
         * contact members.
         * SPRD:Bug 528769 The Contacts app crashes after canceling the selection of the group
         * contact members.
         * SPRD:Bug 528828 The dut can not add a voice call for the conference call.
         * {@
         */
        String intentParam = getIntent().getStringExtra("multi");
        ArrayList numberList = new ArrayList();
        boolean repeat = false;
        ArrayList numArrayIn = new ArrayList();
        if (data != null && data.getSerializableExtra("result") != null
                && ImsManager.isVolteEnabledByPlatform(this) && intentParam != null
                && (intentParam.equals("addMultiCall") || intentParam.equals("addMultiCallAgain"))) {
            final HashMap<String, String> contacts = (HashMap<String, String>) data
                    .getSerializableExtra("result");
            final int minLimitCount = getIntent().getIntExtra("checked_min_limit_count",
                    MIN_CONTACT_COUNT);
            String [] numberArray = getIntent().getExtras().getStringArray("number");
            String[] numListArray = null;
            if (contacts != null && contacts.size() > 0) {
                numListArray = new String[contacts.size()];
                Iterator it = contacts.entrySet().iterator();
                int i = 0;
                while (it.hasNext()) {
                    Entry entry = (Entry) it.next();
                    numListArray[i] = ((String) entry.getKey()).replace(" ", "");
                    Log.i(TAG, "numListArray[ " + i + "]: " + numListArray[i]);
                    i++;
                }
                /**
                 * SPRD:Bug547762 deal with the space in phone number S
                 * PRD:Bug565060 Deal with prefix numbers
                 * @{
                 */
                for (int j = 0; j < i; j++) {
                    boolean repeated = false;
                    for (int m = j + 1; m < i; m++) {
                        if (PhoneNumberUtils.compare(numListArray[j], numListArray[m])) {
                            repeated = true;
                            break;
                        }
                    }
                    if (repeated == false) {
                        numberList.add(numListArray[j]);
                    }
                }
                //add for SPRD:Bug565060 remove contacts in conference meeting already
                boolean repeated = false;
                if (numberArray != null) {
                        for (int m = 0 ; m <numberList.size(); m++){
                            repeated = false;
                            for (int j = 0; j < numberArray.length; j++) {
                                if (PhoneNumberUtils.compare (numberList.get(m).toString(), numberArray[j])){
                                    repeat = true;
                                    repeated = true;
                                    break;
                                }
                            }
                            if (repeated == false) {
                                numArrayIn.add(numberList.get(m));
                            }
                        }
                }
                if (numArrayIn.isEmpty() && numberArray != null)
                {
                    Toast.makeText(ContactSelectionMultiTabActivity.this,
                            R.string.contacts_in_conference_reselect,
                            Toast.LENGTH_SHORT).show();
                    if (repeat == true){
                        repeat = false;
                    }
                    mDataAll.clear();
                    return;
                }
                if (!numArrayIn.isEmpty()  && repeat == true) {
                    Toast.makeText(ContactSelectionMultiTabActivity.this,
                            R.string.contacts_in_conference,
                            Toast.LENGTH_SHORT).show();
                    repeat = false;
                }
            }
            if (contacts != null && contacts.size() < minLimitCount) {
                if (numberArray == null) {
                Toast.makeText(ContactSelectionMultiTabActivity.this,
                        getString(R.string.conferenceCallPartyCountLimit, minLimitCount),
                        Toast.LENGTH_SHORT).show();
                }
                return;
            }
            String[] array = numberArray == null ? (String[]) numberList
                    .toArray(new String[numberList.size()]) : (String[]) numArrayIn
                    .toArray(new String[numArrayIn.size()]);
                Bundle bundle = new Bundle();
                bundle.putBoolean("android.intent.extra.IMS_CONFERENCE_REQUEST", true);
                bundle.putStringArray("android.intent.extra.IMS_CONFERENCE_PARTICIPANTS",
                        array);
                final Intent intentAction = new Intent();
                intentAction.setAction("android.intent.action.CALL_PRIVILEGED");
                intentAction.setData(Uri.parse(new StringBuffer("tel:").append(array[0])
                        .toString()));
                intentAction.putExtras(bundle);
                startActivity(intentAction);
                finish();
        }


        /*
         * }@
         */
        if (requestCode == REQUEST_CODE_PICK) {
            if (resultCode == Activity.RESULT_OK) {
                setResult(RESULT_OK, data);
                /* SPRD:Bug 423428@ { */
                if ((getIntent().getStringExtra("blackcall_type") != null)) {
                    data.setAction(BLACK_CALL_LIST_ACTION);
                    data.putExtra("blackcall_type", getIntent().getStringExtra("blackcall_type"));
                }
                /* @}*/
                finish();
            } else if (resultCode == Activity.RESULT_CANCELED) {

            }
        }
    }

    private final class AllInOneDataMultiPickerActionListener implements
            OnAllInOneDataMultiPickerActionListener {
        public void onPickAllInOneDataAction(HashMap<String, String> pairs) {
            returnPickerResult(pairs);
        }

        public void onCancel() {
            ContactSelectionMultiTabActivity.this.onBackPressed();
        }
    }

    public void returnPickerResult(HashMap<String, String> data) {
        Intent intent = new Intent();
        // SPRD:Bug 519952 "Done" button is grey while some contacts is chosen
        if (data.isEmpty()) {
            Toast.makeText(this, R.string.toast_no_contact_selected,
                    Toast.LENGTH_SHORT).show();
            returnPickerResult();
        } else {
            intent.putExtra("result", data);
            returnPickerResult(intent);
        }
        returnPickerResult(intent);

    }

    public void returnPickerResult() {
        setResult(RESULT_CANCELED);
        finish();
    }

    public void returnPickerResult(Intent intent) {
        /*SPRD: add for VoLTE{ @*/
        String intentParam = getIntent().getStringExtra("multi");
        ArrayList numberList = new ArrayList();
        ArrayList numArrayIn = new ArrayList();
        boolean repeat = false;
        if (ImsManager.isVolteEnabledByPlatform(this) && intentParam !=null && (intentParam.equals("addMultiCall") || intentParam.equals("addMultiCallAgain"))) {
               final HashMap<String, String> contacts = (HashMap<String, String>) intent.getSerializableExtra("result");
               final int minLimitCount = getIntent().getIntExtra("checked_min_limit_count",MIN_CONTACT_COUNT);
               String [] numberArray = getIntent().getExtras().getStringArray("number");
               String[] numListArray = null;
               if (contacts != null && contacts.size() > 0) {
                   numListArray = new String[contacts.size()];
                   Iterator it = contacts.entrySet().iterator();
                   int i =0;
                   while (it.hasNext()) {
                       Entry entry = (Entry) it.next();
                       //add for bug 524706 can't make conference call due to space
                       numListArray[i] = ((String) entry.getKey()).replace(" ", "");
                       i++;
                   }
                   /**
                    * SPRD:Bug547762 deal with the space in phone number
                    * SPRD:Bug565060 Deal with prefix numbers
                    * @{
                    */
                for (int j = 0; j < i; j++) {
                    boolean repeated = false;
                    for (int m = j + 1; m < i; m++) {
                        if (PhoneNumberUtils.compare(numListArray[j], numListArray[m])) {
                            repeated = true;
                            break;
                        }
                    }
                    if (repeated == false) {
                        numberList.add(numListArray[j]);
                    }
                }
                // add for SPRD:Bug565060 remove contacts in conference meeting already
                if (numberArray != null) {
                    boolean repeated = false;
                    for (int m = 0; m < numberList.size(); m++) {
                        repeated = false;
                        for (int j = 0; j < numberArray.length; j++) {
                            if (PhoneNumberUtils.compare(numberList.get(m).toString(),
                                    numberArray[j])) {
                                repeat = true;
                                repeated = true;
                                break;
                            }
                        }
                        if (repeated == false) {
                            numArrayIn.add(numberList.get(m));
                        }
                    }
                }
                if (numArrayIn.isEmpty() && numberArray != null)
                {
                    Toast.makeText(ContactSelectionMultiTabActivity.this,
                            R.string.contacts_in_conference_reselect,
                            Toast.LENGTH_SHORT).show();
                    mDataAll.clear();
                    if (repeat == true){
                        repeat = false;
                    }
                    return;
                }
                if (!numArrayIn.isEmpty() && repeat) {
                    Toast.makeText(ContactSelectionMultiTabActivity.this,
                            R.string.contacts_in_conference,
                            Toast.LENGTH_SHORT).show();
                    repeat = false;
                }
                /**
                 * @}
                 */
            }
               //add for SPRD:Bug547762 deal with the space in phone number
               if (contacts != null && numberList.size() < minLimitCount) {
                   if (numberArray == null){
                   Toast.makeText(ContactSelectionMultiTabActivity.this, getString(R.string.conferenceCallPartyCountLimit,minLimitCount),
                           Toast.LENGTH_SHORT)
                           .show();
                   }
                   /* SPRD: 543746 The DUT make a conference call even switching the contact selection tag.*/
                   //add for SPRD: 546975 clear data when need to select again
                   mDataAll.clear();
                   return;
               }
               String[] array = numberArray == null ? (String[]) numberList
                       .toArray(new String[numberList.size()]) : (String[]) numArrayIn
                       .toArray(new String[numArrayIn.size()]);
               Bundle bundle = new Bundle();
               bundle.putBoolean("android.intent.extra.IMS_CONFERENCE_REQUEST", true);
               bundle.putStringArray("android.intent.extra.IMS_CONFERENCE_PARTICIPANTS", array);
               final Intent intentAction = new Intent();
               intentAction.setAction("android.intent.action.CALL_PRIVILEGED");
               intentAction.setData(Uri.parse(new StringBuffer("tel:").append(array[0]).toString()));
               intentAction.putExtras(bundle);
               startActivity(intentAction);
           /*@}*/
        }else{
            intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            /* SPRD: Bug423428 { */
            if (getIntent().getStringExtra("blackcall_type") != null) {
                intent.setAction(BLACK_CALL_LIST_ACTION);
                intent.putExtra("blackcall_type", getIntent().getStringExtra("blackcall_type"));
            }
            /* @} */
            setResult(RESULT_OK, intent);
        }
        finish();
    }

    private void configTabAdapter(int position) {
        if (position < 0) {
            return;
        }
        switch (position) {
            case TAB_INDEX_GROUP_NEWUI:
                mGroupBrowseListFragment.setContextMenuEnable(false);
                break;
            case TAB_INDEX_FAVORITES_NEWUI:
                mFavoriteFragment.setMultiPickerSupported(true);
                mFavoriteFragment.setCascadingData(mRequest.getCascadingData());
                mFavoriteFragment.setSelection("star");
                mFavoriteFragment.setSelectTextVisible(false);
                mFavoriteFragment.setFilter(ContactListFilter
                        .createFilterWithType(ContactListFilter.FILTER_TYPE_ALL_ACCOUNTS));
                break;
            case TAB_INDEX_ALL_NEWUI:
                mAllInOneDataPickerFragment.setMultiPickerSupported(true);
                mAllInOneDataPickerFragment.setCascadingData(mRequest
                        .getCascadingData());
                mAllInOneDataPickerFragment.setSelectTextVisible(false);
                /**
                 * SPRD bug498383 import simcard only has name and type of other_type
                 * to local by *#*#4636#*#*, add this phone into blacklist, phone crash
                 *
                 * SPRD bug528138 import empty simcard contact to local by *#*#4636#*#*,
                 * invite contact into conference, Contacts crash
                 *
                 * @{
                 */
                mAllInOneDataPickerFragment.setFilter(ContactListFilter
                        .createFilterWithType(ContactListFilter.FILTER_TYPE_WITH_PHONE_NUMBERS_ONLY));
                /**
                 * @}
                 */
                break;
            case TAB_INDEX_CALLLOG_NEWUI:
                mCallLogFragment.setMultiPickerSupported(true);
                mCallLogFragment.setCascadingData(mRequest
                        .getCascadingData());
                mCallLogFragment.setFilter(ContactListFilter
                        .createFilterWithType(ContactListFilter.FILTER_TYPE_ALL_ACCOUNTS));
                mCallLogFragment.setSearchVisible(false);
                mCallLogFragment.setFirstDividerVisible(true);
                mCallLogFragment.setSelectTextVisible(false);
                break;
            default:
                break;
        }
        setupActionListener(position);
    }

    private void setupActionListener(int position) {

        switch (position) {
            case TAB_INDEX_GROUP_NEWUI:
                mGroupBrowseListFragment.setListener(new GroupBrowserActionListener());
                break;
            case TAB_INDEX_FAVORITES_NEWUI:
                mFavoriteFragment.setOnAllInOneDataMultiPickerActionListener(
                        new AllInOneDataMultiPickerActionListener());
                break;
            case TAB_INDEX_ALL_NEWUI:
                mAllInOneDataPickerFragment.setOnAllInOneDataMultiPickerActionListener(
                        new AllInOneDataMultiPickerActionListener());
                break;
             case TAB_INDEX_CALLLOG_NEWUI:
                mCallLogFragment.setOnAllInOneDataMultiPickerActionListener(
                        new AllInOneDataMultiPickerActionListener());
                break;

            default:
                break;
        }
    }

    private void configCompleteListener(int position) {
        // SPRD:Bug519952 "Done" button is grey while some contacts is chose
        if (mCallFireWallActivityCalling == true) {
            switch (position) {
                case TAB_INDEX_GROUP_NEWUI:
                    returnPickerResult();
                    break;
                case TAB_INDEX_FAVORITES_NEWUI:
                case TAB_INDEX_ALL_NEWUI:
                case TAB_INDEX_CALLLOG_NEWUI:
                    if (mFavoriteFragment != null) {
                        mDataAll.putAll(mFavoriteFragment.getMultiPickerSelected());
                    }
                    if (mCallLogFragment != null) {
                        mDataAll.putAll(mCallLogFragment.getMultiPickerSelected());
                    }
                    if (mAllInOneDataPickerFragment != null) {
                        mDataAll.putAll(mAllInOneDataPickerFragment.getMultiPickerSelected());
                    }
                    returnPickerResult(mDataAll);
                    break;
                default:
                    break;
            }
        }
        else {
            switch (position) {
                case TAB_INDEX_GROUP_NEWUI:
                    returnPickerResult();
                    break;
                case TAB_INDEX_FAVORITES_NEWUI:
                    if (mFavoriteFragment != null) {
                        // SPRD:Bug519952 "Done" button is grey while some contacts is chosen
                        mFavoriteFragment.onMultiPickerSelected();
                    }
                    break;
                case TAB_INDEX_ALL_NEWUI:
                    if (mAllInOneDataPickerFragment != null) {
                        mAllInOneDataPickerFragment.onMultiPickerSelected();
                    }
                    break;
                case TAB_INDEX_CALLLOG_NEWUI:
                    if (mCallLogFragment != null) {
                        mCallLogFragment.onMultiPickerSelected();
                    }
                    break;
                default:
                    break;
            }

        }
    }

    private void clearData(int position) {
        switch (position) {
            case TAB_INDEX_GROUP_NEWUI:
                setDoneMenu(false);
                break;
            case TAB_INDEX_FAVORITES_NEWUI:
                if (mFavoriteFragment != null) {
                    mFavoriteFragment.clearCheckedItem();
                }
                break;
            case TAB_INDEX_ALL_NEWUI:
                if (mAllInOneDataPickerFragment != null) {
                    mAllInOneDataPickerFragment.clearCheckedItem();
                }
                break;
            case TAB_INDEX_CALLLOG_NEWUI:
                if (mCallLogFragment != null) {
                    mCallLogFragment.clearCheckedItem();
                }
                break;
            default:
                break;
        }
        if(position != -1) {
            mDoneEnable = false;
            setDoneMenu(mDoneEnable);
        }
    }

    /**
     * SPRD:Bug 564314 clear data after merge calls and return to invite tabs
     *
     * @{
     */
    private void clearDataAll() {
        if (mFavoriteFragment != null) {
            mFavoriteFragment.clearCheckedItem();
        }
        if (mAllInOneDataPickerFragment != null) {
            mAllInOneDataPickerFragment.clearCheckedItem();
        }
        if (mCallLogFragment != null) {
            mCallLogFragment.clearCheckedItem();
        }
        mDoneEnable = false;
        setDoneMenu(mDoneEnable);
    }

    /**
     * @}
     */

    public void setDoneMenu(boolean enabled) {
        if (mDoneMenuItem == null) {
            return;
        }
        if (enabled) {
            mDoneMenuItem.setEnabled(true);
            mDoneMenuItem.setTextColor(mDoneMenuDisableColor);
        } else {
            mDoneMenuItem.setEnabled(false);
            mDoneMenuItem.setTextColor(getResources().getColor(R.color.action_bar_button_disable_text_color));
        }
    }
}
