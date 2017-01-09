package plugin.sprd.protectedapp;

import android.R.anim;
import android.app.ActionBar;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SyncAdapterType;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.UserHandle;
import android.os.UserManager;
import android.preference.PreferenceFragment;
import android.preference.PreferenceFrameLayout;
import android.preference.SwitchPreference;
import android.provider.Settings;
import android.util.ArraySet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AnimationUtils;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.Filter;
import android.widget.Filterable;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.Switch;


import java.text.BreakIterator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.PriorityBlockingQueue;
import java.util.concurrent.ThreadFactory;

import plugin.sprd.protectedapp.R;
import plugin.sprd.protectedapp.SecurityManageFragment;
import plugin.sprd.protectedapp.db.DatabaseUtil;
import plugin.sprd.protectedapp.model.AppInfoModel;
import plugin.sprd.protectedapp.util.ApplicationsState;
import plugin.sprd.protectedapp.util.ApplicationsState.AppEntry;
import plugin.sprd.protectedapp.util.ApplicationsState.AppFilter;
import plugin.sprd.protectedapp.util.ApplicationsState.CompoundFilter;
import plugin.sprd.protectedapp.util.ApplicationsState.VolumeFilter;


public class SecurityManageFragment extends PreferenceFragment
        implements OnItemClickListener, OnItemSelectedListener,OnCheckedChangeListener {

    static final String TAG = "ManageApplications";
    //static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);
    static final boolean DEBUG = true;

    // Intent extras.
    public static final String EXTRA_CLASSNAME = "classname";
    // Used for storage only.
    public static final String EXTRA_VOLUME_UUID = "volumeUuid";
    public static final String EXTRA_VOLUME_NAME = "volumeName";

    private static final String EXTRA_SORT_ORDER = "sortOrder";
    private static final String EXTRA_SHOW_SYSTEM = "showSystem";
    private static final String EXTRA_HAS_ENTRIES = "hasEntries";

    // attributes used as keys when passing values to InstalledAppDetails activity
    public static final String APP_CHG = "chg";

    // constant value that can be used to check return code from sub activity.
    private static final int INSTALLED_APP_DETAILS = 1;
    private static final int ADVANCED_SETTINGS = 2;

    public static final int SIZE_TOTAL = 0;
    public static final int SIZE_INTERNAL = 1;
    public static final int SIZE_EXTERNAL = 2;

    // Filter options used for displayed list of applications
    // The order which they appear is the order they will show when spinner is present.
    public static final int FILTER_APPS_POWER_WHITELIST         = 0;
    public static final int FILTER_APPS_POWER_WHITELIST_ALL     = 1;
    public static final int FILTER_APPS_ALL                     = 2;
    public static final int FILTER_APPS_ENABLED                 = 3;
    public static final int FILTER_APPS_DISABLED                = 4;
    public static final int FILTER_APPS_BLOCKED                 = 5;
    public static final int FILTER_APPS_PRIORITY                = 6;
    public static final int FILTER_APPS_NO_PEEKING              = 7;
    public static final int FILTER_APPS_SENSITIVE               = 8;
    public static final int FILTER_APPS_PERSONAL                = 9;
    public static final int FILTER_APPS_WORK                    = 10;
    public static final int FILTER_APPS_WITH_DOMAIN_URLS        = 11;
    public static final int FILTER_APPS_USAGE_ACCESS            = 12;
    public static final int FILTER_APPS_WITH_OVERLAY            = 13;
    public static final int FILTER_APPS_WRITE_SETTINGS          = 14;

    public static final AppFilter[] FILTERS = new AppFilter[] {
        new CompoundFilter(ApplicationsState.FILTER_PERSONAL_WITHOUT_DISABLED_UNTIL_USED,
                ApplicationsState.FILTER_NONE_SYSTEM),     // All apps label, but personal filter
        ApplicationsState.FILTER_EVERYTHING,  // All apps
        ApplicationsState.FILTER_ALL_ENABLED, // Enabled
        ApplicationsState.FILTER_DISABLED,    // Disabled
        ApplicationsState.FILTER_PERSONAL,    // Personal
        ApplicationsState.FILTER_WORK,        // Work
        ApplicationsState.FILTER_WITH_DOMAIN_URLS,   // Apps with Domain URLs
    };

    // whether showing system apps.
    private boolean mShowSystem;

    private ApplicationsState mApplicationsState;

    public int mListType;
    public int mFilter;

    public ApplicationsAdapter mApplications;

    private View mLoadingContainer;

    private View mListContainer;

    // ListView used to display list
    private ListView mListView;

    // Size resource used for packages whose size computation failed for some reason
    CharSequence mInvalidSizeStr;

    // layout inflater object used to inflate views
    private LayoutInflater mInflater;

    private String mCurrentPkgName;
    private int mCurrentUid;
    private boolean mFinishAfterDialog;

    private Menu mOptionsMenu;

    public static final int LIST_TYPE_MAIN         = 0;
    public static final int LIST_TYPE_NOTIFICATION = 1;
    public static final int LIST_TYPE_DOMAINS_URLS = 2;
    public static final int LIST_TYPE_STORAGE      = 3;
    public static final int LIST_TYPE_USAGE_ACCESS = 4;
    public static final int LIST_TYPE_HIGH_POWER   = 5;
    public static final int LIST_TYPE_OVERLAY      = 6;
    public static final int LIST_TYPE_WRITE_SETTINGS = 7;

    private View mRootView;

    private View mSpinnerHeader;
    private Spinner mFilterSpinner;
    private String mVolumeName;
    private SecurityManageFragment mSecurityManageApplications;
    private static final int CORE_POOL_SIZE = 1;
    private static final int MAXI_POOL_SIZE = 1;
    private static final int KEEP_ALIVE = 5;
    private Switch mSelectAllBtn = null;
    private static DatabaseUtil mSecurityClearDatabaseUtil;
    //add for bug 610992
    private boolean isResumed = false;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
        mApplicationsState = ApplicationsState.getInstance(getActivity().getApplication());
        mListType = LIST_TYPE_MAIN;
        mFilter = getDefaultFilter();
        mSecurityManageApplications = this;
        mSecurityClearDatabaseUtil = new DatabaseUtil(getActivity());
        handleActionBar();
    }

    private void handleActionBar() {
        ActionBar actionBar = getActivity().getActionBar();
        actionBar.setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP, ActionBar.DISPLAY_HOME_AS_UP);
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setDisplayShowTitleEnabled(true);
        actionBar.setTitle(getString(R.string.app_title));
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        // initialize the inflater
        mInflater = inflater;

        mRootView = inflater.inflate(R.layout.manage_applications_apps, null);
        mListContainer = mRootView.findViewById(R.id.list_container);
        mSelectAllBtn = (Switch) mRootView.findViewById(R.id.security_toggle_all);
        if(mSelectAllBtn != null){
            mSelectAllBtn.setOnCheckedChangeListener(this);
        }
        updateAllBtnStates();
        if (mListContainer != null) {
            // Create adapter and list view here
            View emptyView = mListContainer.findViewById(com.android.internal.R.id.empty);
            ListView lv = (ListView) mListContainer.findViewById(android.R.id.list);
            if (emptyView != null) {
                lv.setEmptyView(emptyView);
            }
            lv.setOnItemClickListener(this);
            lv.setSaveEnabled(true);
            lv.setItemsCanFocus(true);
            lv.setTextFilterEnabled(true);
            mListView = lv;
            List securityModel = new ArrayList<AppInfoModel>();
            mApplications = new ApplicationsAdapter(mApplicationsState, this, mFilter, mFragmentHandler);
            if (savedInstanceState != null) {
                mApplications.mHasReceivedLoadEntries =
                        savedInstanceState.getBoolean(EXTRA_HAS_ENTRIES, false);
            }
            mListView.setAdapter(mApplications);
            mListView.setRecyclerListener(mApplications);
        }
        return mRootView;
    }

    /**
     * Using to reflash all select button states
     * UI       Left  |  Right
     * DB Value    1  |  0
     * Function  open | close
     * Checked  false | true
     */
    private Handler mSelectBtnHanlder = new Handler();
    private boolean isEnabledTrigger = true;
    private final int mTriggerDelay = 180;
    private Runnable mSelectBtnRunnable = new Runnable() {
        @Override
        public void run() {
            if(mSecurityClearDatabaseUtil.hasNoDirectValue(1)){
               mSelectAllBtn.setChecked(true);
            }else{
               if (mSelectAllBtn.isChecked()) {
                    isEnabledTrigger = false;
                    mSelectAllBtn.setChecked(false);
               }
            }
        }
    };
    private void updateAllBtnStates(){
        mSelectBtnHanlder.removeCallbacks(mSelectBtnRunnable);
        mSelectBtnHanlder.postDelayed(mSelectBtnRunnable, mTriggerDelay);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        if (mListType == LIST_TYPE_STORAGE) {
            FrameLayout pinnedHeader = (FrameLayout) mRootView.findViewById(R.id.pinned_header);
        }
    }

    private int getDefaultFilter() {
        switch (mListType) {
            case LIST_TYPE_DOMAINS_URLS:
                return FILTER_APPS_WITH_DOMAIN_URLS;
            case LIST_TYPE_USAGE_ACCESS:
                return FILTER_APPS_USAGE_ACCESS;
            case LIST_TYPE_HIGH_POWER:
                return FILTER_APPS_POWER_WHITELIST;
            case LIST_TYPE_OVERLAY:
                return FILTER_APPS_WITH_OVERLAY;
            case LIST_TYPE_WRITE_SETTINGS:
                return FILTER_APPS_WRITE_SETTINGS;
            default:
                return FILTER_APPS_ALL;
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        //updateOptionsMenu();
        if (mApplications != null) {
            mApplications.sessionResume();
            mApplications.resume();
        }
        //add for bug 610992
        isResumed = true;
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        //outState.putInt(EXTRA_SORT_ORDER, mSortOrder);
        outState.putBoolean(EXTRA_SHOW_SYSTEM, mShowSystem);
        outState.putBoolean(EXTRA_HAS_ENTRIES, mApplications.mHasReceivedLoadEntries);
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mApplications != null) {
            mApplications.pause();
        }
      //add for bug 610992
        isResumed = false;
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();

        if (mApplications != null) {
            mApplications.release();
        }
        mRootView = null;
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        //inflater.inflate(R.menu.manage_apps, menu);
    }

    @Override
    public void onPrepareOptionsMenu(Menu menu) {
    }

    @Override
    public void onDestroyOptionsMenu() {
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        return true;
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        mApplications.setFilter(mFilter);
        if (DEBUG) Log.d(TAG, "Selecting filter " + mFilter);
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    private final static int ALL_CHANGED = 100;
    private final static int SINGLE_CHANGED = 200;
    public final Handler mFragmentHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (DEBUG)
                Log.d(TAG, "handleMessage");
            int what = msg.what;
            switch (what) {
            case ALL_CHANGED:
                mApplications.resume();
                mApplications.notifyDataSetChanged();
                break;
            case SINGLE_CHANGED:
                updateAllBtnStates();
                break;
            }
        }
    };

    static class ApplicationsAdapter extends BaseAdapter implements Filterable,
            ApplicationsState.Callbacks,
            AbsListView.RecyclerListener {
        private final ApplicationsState mState;
        private final ApplicationsState.Session mSession;
        private final SecurityManageFragment mManageApplications;
        private final Context mContext;
        private final ArrayList<View> mActive = new ArrayList<View>();
        private int mFilterMode;
        private ArrayList<ApplicationsState.AppEntry> mBaseEntries;
        private ArrayList<ApplicationsState.AppEntry> mEntries;
        private boolean mResumed;
        private int mLastSortMode=-1;
        private int mWhichSize = SIZE_TOTAL;
        CharSequence mCurFilterPrefix;
        private PackageManager mPm;
        private AppFilter mOverrideFilter;
        private boolean mHasReceivedLoadEntries;
        private boolean mHasReceivedBridgeCallback;
        private ArrayList<AppInfoModel> mSecurityClearModels;
        private Handler mHandler;
        private Filter mFilter = new Filter() {
            @Override
            protected FilterResults performFiltering(CharSequence constraint) {
                ArrayList<ApplicationsState.AppEntry> entries
                        = applyPrefixFilter(constraint, mBaseEntries);
                FilterResults fr = new FilterResults();
                fr.values = entries;
                fr.count = entries.size();
                return fr;
            }

            @Override
            @SuppressWarnings("unchecked")
            protected void publishResults(CharSequence constraint, FilterResults results) {
                mCurFilterPrefix = constraint;
                mEntries = (ArrayList<ApplicationsState.AppEntry>) results.values;
                mEntries = removeCommon(mEntries);
                notifyDataSetChanged();
            }
        };

        public ApplicationsAdapter(ApplicationsState state, SecurityManageFragment manageApplications,
                int filterMode,Handler handler) {
            mState = state;
            mSession = state.newSession(this);
            mManageApplications = manageApplications;
            mContext = manageApplications.getActivity();
            mPm = mContext.getPackageManager();
            mFilterMode = filterMode;
            mHandler = handler;
        }

        public void setOverrideFilter(AppFilter overrideFilter) {
            mOverrideFilter = overrideFilter;
            rebuild(true);
        }

        public void setFilter(int filter) {
            mFilterMode = filter;
            rebuild(true);
        }

        public void sessionResume(){
            mSession.resume();
        }

        public void resume() {
            rebuild(true);
        }

        public void pause() {
            if (mResumed) {
                mResumed = false;
                mSession.pause();
            }
        }

        public void release() {
            mSession.release();
        }

        public void rebuild(int sort) {
            if (sort == mLastSortMode) {
                return;
            }
            mLastSortMode = sort;
            rebuild(true);
        }

        public void rebuild(boolean eraseold) {
            if (DEBUG) Log.i(TAG, "Rebuilding app list...");
            ApplicationsState.AppFilter filterObj = FILTERS[0];
            Comparator<AppEntry> comparatorObj;
            filterObj = new CompoundFilter(filterObj,
                    ApplicationsState.FILTER_DOWNLOADED_AND_LAUNCHER);
            comparatorObj = ApplicationsState.ALPHA_COMPARATOR;
            ArrayList<ApplicationsState.AppEntry> entries
                    = mSession.rebuild(filterObj, comparatorObj);
            if (entries == null && !eraseold) {
                return;
            }
            mBaseEntries = entries;
            if (mBaseEntries != null) {
                mEntries = applyPrefixFilter(mCurFilterPrefix, mBaseEntries);
                mEntries = removeCommon(mEntries);
                mSecurityClearModels = mSecurityClearDatabaseUtil.initialize(mEntries);
            } else {
                mEntries = null;
            }
            notifyDataSetChanged();
        }

        ArrayList<ApplicationsState.AppEntry> applyPrefixFilter(CharSequence prefix,
                ArrayList<ApplicationsState.AppEntry> origEntries) {
            if (prefix == null || prefix.length() == 0) {
                return origEntries;
            } else {
                String prefixStr = ApplicationsState.normalize(prefix.toString());
                final String spacePrefixStr = " " + prefixStr;
                ArrayList<ApplicationsState.AppEntry> newEntries
                        = new ArrayList<ApplicationsState.AppEntry>();
                for (int i=0; i<origEntries.size(); i++) {
                    ApplicationsState.AppEntry entry = origEntries.get(i);
                    String nlabel = entry.getNormalizedLabel();
                    if (nlabel.startsWith(prefixStr) || nlabel.indexOf(spacePrefixStr) != -1) {
                        newEntries.add(entry);
                    }
                }
                return newEntries;
            }
        }

        //Common package is opened that will cause exception. Should be removed.
        private ArrayList<ApplicationsState.AppEntry> removeCommon(ArrayList<ApplicationsState.AppEntry> entries) {
            if (entries != null && entries.size() > 0) {
                for (int i = 0; i < entries.size(); i++) {
                    ApplicationsState.AppEntry appEntry = entries.get(i);
                    if (mContext.getPackageName().equals(appEntry.info.packageName)) {
                        entries.remove(appEntry);
                    }
                }
            }
            return entries;
        }

        @Override
        public void onRunningStateChanged(boolean running) {
            mManageApplications.getActivity().setProgressBarIndeterminateVisibility(running);
        }

        @Override
        public void onRebuildComplete(ArrayList<AppEntry> apps) {
            if (mManageApplications.mLoadingContainer.getVisibility() == View.VISIBLE) {
                mManageApplications.mLoadingContainer.startAnimation(AnimationUtils.loadAnimation(
                        mContext, android.R.anim.fade_out));
                mManageApplications.mListContainer.startAnimation(AnimationUtils.loadAnimation(
                        mContext, android.R.anim.fade_in));
            }
            mManageApplications.mListContainer.setVisibility(View.VISIBLE);
            mManageApplications.mLoadingContainer.setVisibility(View.GONE);
            mBaseEntries = apps;
            mEntries = applyPrefixFilter(mCurFilterPrefix, mBaseEntries);
            mEntries = removeCommon(mEntries);
            notifyDataSetChanged();
        }

        @Override
        public void onPackageListChanged() {
            rebuild(false);
        }

        @Override
        public void onPackageIconChanged() {
        }

        @Override
        public void onLoadEntriesCompleted() {
            mHasReceivedLoadEntries = true;
        }

        @Override
        public void onPackageSizeChanged(String packageName) {
            for (int i=0; i<mActive.size(); i++) {
                AppViewHolder holder = (AppViewHolder)mActive.get(i).getTag();
                if (holder.entry.info.packageName.equals(packageName)) {
                    if (holder.entry.info.packageName.equals(mManageApplications.mCurrentPkgName)
                            ) {
                        rebuild(false);
                    }
                    return;
                }
            }
        }

        @Override
        public void onLauncherInfoChanged() {
            if (!mManageApplications.mShowSystem) {
                rebuild(false);
            }
        }

        @Override
        public void onAllSizesComputed() {
            rebuild(false);
        }

        public int getCount() {
            return mEntries != null ? mEntries.size() : 0;
        }

        public Object getItem(int position) {
            return mEntries.get(position);
        }

        public ApplicationsState.AppEntry getAppEntry(int position) {
            return mEntries.get(position);
        }

        public long getItemId(int position) {
            return mEntries.get(position).id;
        }

        @Override
        public boolean areAllItemsEnabled() {
            return false;
        }

        @Override
        public boolean isEnabled(int position) {
            if (mManageApplications.mListType != LIST_TYPE_HIGH_POWER) {
                return true;
            }
            ApplicationsState.AppEntry entry = mEntries.get(position);
            return true;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            final AppViewHolder holder = AppViewHolder.createOrRecycle(mManageApplications.mInflater,
                    convertView);
            convertView = holder.rootView;

            ApplicationsState.AppEntry entry = mEntries.get(position);
            synchronized (entry) {
                holder.entry = entry;
                if (entry.label != null) {
                    holder.appName.setText(entry.label);
                }
                mState.ensureIcon(entry);
                if (entry.icon != null) {
                    holder.appIcon.setImageDrawable(entry.icon);
                }
                holder.updateSizeText(mManageApplications.mInvalidSizeStr, mWhichSize);
                if ((entry.info.flags&ApplicationInfo.FLAG_INSTALLED) == 0) {
                    holder.disabled.setVisibility(View.VISIBLE);
                    holder.disabled.setText(R.string.not_installed);
                } else if (!entry.info.enabled) {
                    holder.disabled.setVisibility(View.VISIBLE);
                    holder.disabled.setText(R.string.disabled);
                } else {
                    holder.disabled.setVisibility(View.GONE);
                }
                if(entry.info != null && entry.info.packageName != null && mSecurityClearModels != null){
                    String pkgName = entry.info.packageName;
                    for(AppInfoModel securityClearModel : mSecurityClearModels){
                        if(securityClearModel.getPackageName().equals(pkgName)){
                            if(securityClearModel.getEnable().equals("1")){
                                holder.toggleButton.setChecked(false);
                            }else{
                                holder.toggleButton.setChecked(true);
                            }
                        }
                    }
                }
                holder.toggleButton.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                    public void onCheckedChanged(CompoundButton v, boolean isChecked) {
                        if(holder == null){
                            return;
                        }
                        final ApplicationInfo appInfo = holder.entry.info;
                        final boolean isCheckeds = !isChecked;
                        new Thread(new Runnable() {
                            @Override
                            public void run() {
                                synchronized (this) {
                                    mSecurityClearDatabaseUtil.update(appInfo.packageName, isCheckeds);
                                    mSecurityClearModels = mSecurityClearDatabaseUtil.initialize(mEntries);
                                    mHandler.sendEmptyMessage(SINGLE_CHANGED);
                                }
                            }
                        }).start();
                    }
                });
            }
            mActive.remove(convertView);
            mActive.add(convertView);
            convertView.setEnabled(isEnabled(position));
            return convertView;
        }

        @Override
        public Filter getFilter() {
            return mFilter;
        }

        @Override
        public void onMovedToScrapHeap(View view) {
            mActive.remove(view);
        }
     }

    @Override
    public void onCheckedChanged(CompoundButton compoundbutton, boolean flag) {
        if (DEBUG)
            Log.i(TAG, "onCheckedChanged..." + flag);
        //add for bug 610992
        if(!isResumed){
            return;
        }
        if (!isEnabledTrigger) {
            isEnabledTrigger = true;
            return;
        }
        mSelectAllBtn.setClickable(false);
        final boolean finFlag = flag;
        new Thread(new Runnable() {
            @Override
            public void run() {
                mSecurityClearDatabaseUtil.updateAll(!finFlag);
                mFragmentHandler.sendEmptyMessage(ALL_CHANGED);
            }
        }).start();
        new Handler().postDelayed(new Runnable() {
            @Override
            public void run() {
                mSelectAllBtn.setClickable(true);
            }
        }, mTriggerDelay);
    }
}
