package com.sprd.generalsecurity.memory;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningAppProcessInfo;
import android.app.ActivityManager.RunningServiceInfo;
import android.app.Fragment;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageItemInfo;
import android.content.pm.PackageManager;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.SystemClock;
import android.os.UserHandle;
import android.os.UserManager;
import android.text.BidiFormatter;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.text.format.Formatter;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.os.Bundle;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.animation.Animation.AnimationListener;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.GridLayout.LayoutParams;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import java.lang.Object;
import java.lang.Override;
import java.lang.String;
import java.lang.Void;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;

import android.net.Uri;
import android.widget.ListView;
import android.widget.AbsListView.RecyclerListener;
import android.widget.AdapterView.OnItemClickListener;
import android.util.Log;

import java.util.List;

import com.android.internal.util.MemInfoReader;
import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.MemoryUtils;
import com.sprd.generalsecurity.utils.RunningState;

import android.os.Message;

public class MemoryManagement extends Activity implements OnClickListener,
        RunningState.OnRefreshUiListener, OnItemClickListener, RecyclerListener {

    private static String TAG = "MemoryManagement";

    private ActivityManager mAm; //ActivityManage
    private ListView mListView; //the app list
    private ServiceListAdapter mAdapter; //apps adapter
    private View mHeader; //
    private View mView;
    private View mMainView; //the main view of this Activity
    private View mLoadingContainer; //the anim for update data
    private Context mContext;
    private TextView mLoadedText; //the text in the loading Container
    private TextView mUsedPercentText; //the string of used percent in circle
    private TextView mHeaderText; //the title of app list
    private View mGsCircle; //the view of percent
    private Button mStart; //the start button in circle
    private View mNeedLoading;
    private UserManager mUserManager;

    private boolean mFlag; // the flag for loading anim
    private static final int REMOVE_END = 1; // clear over the apps
    private static final int IS_ANIMATE = 2; // the tag for cleaning anim
    private static final int UPDATE_PER = 3; // update the text that mUsedPercentText
    private static final int TIME_UPDATE_DELAY = 1; // the time for update the ui when cleanning
    private PackageManager mPm;
    private RunningState mState; //Running process state
    private int mDrawableId;// the current drawable id
    private StringBuilder mBuilder = new StringBuilder(128); // time string
    final HashMap<View, ActiveItem> mActiveItems = new HashMap<View, ActiveItem>();
    private long mCurTotalRam = -1; //
    private long mCurHighRam = -1; // "System" or "Used"
    private long mCurMedRam = -1; // "Apps" or "Cached"
    private long mCurLowRam = -1; // "Free"
    private boolean mCurShowCached = false;
    private MemInfoReader mMemInfoReader = new MemInfoReader();
    private Runnable mDataAvail;
    private String mRubbsh; // the string of freed
    private RunningState.BaseItem mCurSelected; // the current item which you  selected

    private final int[] drawableIds = { R.drawable.manage_memory_0,
            R.drawable.manage_memory_1, R.drawable.manage_memory_2,
            R.drawable.manage_memory_3, R.drawable.manage_memory_4,
            R.drawable.manage_memory_5, R.drawable.manage_memory_6,
            R.drawable.manage_memory_7, R.drawable.manage_memory_8,
            R.drawable.manage_memory_9, R.drawable.manage_memory_10,
            R.drawable.manage_memory_11, R.drawable.manage_memory_12,
            R.drawable.manage_memory_13, R.drawable.manage_memory_14,
            R.drawable.manage_memory_15, R.drawable.manage_memory_16,
            R.drawable.manage_memory_17, R.drawable.manage_memory_18,
            R.drawable.manage_memory_19, R.drawable.manage_memory_20,
            R.drawable.manage_memory_21, R.drawable.manage_memory_22,
            R.drawable.manage_memory_23, R.drawable.manage_memory_24,
            R.drawable.manage_memory_25, R.drawable.manage_memory_26,
            R.drawable.manage_memory_27, R.drawable.manage_memory_28,
            R.drawable.manage_memory_29, R.drawable.manage_memory_30,
            R.drawable.manage_memory_31, R.drawable.manage_memory_32,
            R.drawable.manage_memory_33, R.drawable.manage_memory_34,
            R.drawable.manage_memory_35 };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;
        mAm = (ActivityManager) mContext
                .getSystemService(Context.ACTIVITY_SERVICE);
        mPm = getPackageManager();
        mState = RunningState.getInstance(this);

        LayoutInflater inflater = (LayoutInflater) mContext
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mMainView = inflater.inflate(R.layout.manage_memory, null);
        mNeedLoading = mMainView.findViewById(R.id.need_loading);
        mLoadingContainer = mMainView.findViewById(R.id.loading_container);
        Log.i(TAG, "mLoadingContainer=" + mLoadingContainer);

        mHeader = mNeedLoading.findViewById(R.id.memory_header);

        mGsCircle = mHeader.findViewById(R.id.gs_circle);
        mLoadedText = (TextView) mHeader.findViewById(R.id.can_rubbish_ram);

        mUsedPercentText = (TextView) mGsCircle.findViewById(R.id.gs_per_used);
        mStart = (Button) mGsCircle.findViewById(R.id.start_memory_button);
        mStart.setOnClickListener(this);

        mView = mNeedLoading.findViewById(R.id.memory_view);
        mHeaderText = (TextView) mView.findViewById(R.id.header);
        mListView = (ListView) mView.findViewById(R.id.list);
        View emptyView = mView.findViewById(R.id.empty);
        if (emptyView != null) {
            mListView.setEmptyView(emptyView);
        }
        mListView.setOnItemClickListener(this);
        mListView.setRecyclerListener(this);

        mAdapter = new ServiceListAdapter(mState);
        mAdapter.setShowBackground(true);
        mListView.setAdapter(mAdapter);

        ActivityManager.MemoryInfo memInfo = new ActivityManager.MemoryInfo();
        mAm.getMemoryInfo(memInfo);

        setContentView(mMainView);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setHomeButtonEnabled(true);
    }

    private class ServiceListAdapter extends BaseAdapter {
        final RunningState mState;
        final LayoutInflater mInflater;
        boolean mShowBackground;
        ArrayList<RunningState.MergedItem> mOrigItems;
        final ArrayList<RunningState.MergedItem> mItems = new ArrayList<RunningState.MergedItem>();

        ServiceListAdapter(RunningState state) {
            mState = state;
            mInflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            refreshItems();
        }

        void setShowBackground(boolean showBackground) {
            if (mShowBackground != showBackground) {
                mShowBackground = showBackground;
                mState.setWatchingBackgroundItems(showBackground);
                refreshItems();
                Log.i(TAG, "...setBackground...");
                refreshUi(true);
            }
        }

        boolean getShowBackground() {
            return mShowBackground;
        }

        void refreshItems() {
            Log.i(TAG,
                    "mState.getCurrentBackgroundItems()="
                            + mState.getCurrentBackgroundItems()
                            + " ; mState.getCurrentMergedItems()="
                            + mState.getCurrentMergedItems());
            ArrayList<RunningState.MergedItem> newItems = mShowBackground ? mState
                    .getCurrentBackgroundItems() : mState
                    .getCurrentMergedItems();
            Log.i(TAG, "mOrigItems=" + mOrigItems + " ; newItems=" + newItems);
            if (mOrigItems != newItems) {
                mOrigItems = newItems;
                if (newItems == null) {
                    mItems.clear();
                } else {
                    mItems.clear();
                    mItems.addAll(newItems);
                    if (mShowBackground) {
                        Collections.sort(mItems, mState.mBackgroundComparator);
                    }
                }
            }
        }

        public boolean hasStableIds() {
            return true;
        }

        public int getCount() {
            Log.i(TAG, "getCount()" + mItems.size());
            return mItems.size();
        }

        @Override
        public boolean isEmpty() {
            Log.i(TAG, "isEmpty() mState.hasData()=" + mState.hasData()
                    + " ; mItems.size()=" + mItems.size());
            return mState.hasData() && mItems.size() == 0;
        }

        public Object getItem(int position) {
            Log.i(TAG, "getItem() position=" + position);
            return mItems.get(position);
        }

        public long getItemId(int position) {
            return mItems.get(position).hashCode();
        }

        public boolean areAllItemsEnabled() {
            return false;
        }

        public boolean isEnabled(int position) {
            Log.i(TAG, "isEnabled");
            return !mItems.get(position).mIsProcess;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            View v;
            if (convertView == null) {
                v = newView(parent);
                Log.i(TAG, "newView v=" + v);
            } else {
                v = convertView;
                Log.i(TAG, "v=" + v);
            }
            Log.i(TAG, "bindView > ");
            bindView(v, position);
            return v;
        }

        public View newView(ViewGroup parent) {
            View v = mInflater.inflate(R.layout.memifo_item, parent, false);
            Log.i(TAG, "parent=" + parent + " ; v=" + v);
            new ViewHolder(v);
            return v;
        }

        public void bindView(View view, int position) {
            Log.i(TAG, "> bindView");
            synchronized (mState.mLock) {
                Log.i(TAG, "position=" + position + " ; mItems.size()="
                        + mItems.size());
                if (position >= mItems.size()) {
                    // List must have changed since we last reported its
                    // size... ignore here, we will be doing a data changed
                    // to refresh the entire list.
                    return;
                }
                ViewHolder vh = (ViewHolder) view.getTag();
                RunningState.MergedItem item = mItems.get(position);
                ActiveItem ai = vh.bind(mState, item, mBuilder);
                mActiveItems.put(view, ai);
                Log.i(TAG, "mActiveItems=" + mActiveItems.size() + "  ;  view="
                        + view + " ; ai=" + ai);
            }
        }
    }

    static class ViewHolder {
        public View rootView;
        public ImageView icon;
        public TextView name;
        public TextView description;
        public TextView size;
        public TextView uptime;

        public ViewHolder(View v) {
            rootView = v;
            icon = (ImageView) v.findViewById(R.id.icon);
            name = (TextView) v.findViewById(R.id.name);
            description = (TextView) v.findViewById(R.id.description);
            size = (TextView) v.findViewById(R.id.size);
            uptime = (TextView) v.findViewById(R.id.uptime);
            v.setTag(this);
        }

        public ActiveItem bind(RunningState state,
                com.sprd.generalsecurity.utils.RunningState.BaseItem item,
                StringBuilder builder) {
            synchronized (state.mLock) {
                PackageManager pm = rootView.getContext().getPackageManager();
                if (item.mPackageInfo == null
                        && item instanceof RunningState.MergedItem) {
                    // Items for background processes don't normally load
                    // their labels for performance reasons. Do it now.
                    RunningState.MergedItem mergedItem = (RunningState.MergedItem) item;
                    if (mergedItem.mProcess != null) {
                        ((RunningState.MergedItem) item).mProcess
                                .ensureLabel(pm);
                        item.mPackageInfo = ((RunningState.MergedItem) item).mProcess.mPackageInfo;
                        item.mDisplayLabel = ((RunningState.MergedItem) item).mProcess.mDisplayLabel;
                    }
                }
                name.setText(item.mDisplayLabel);
                ActiveItem ai = new ActiveItem();
                ai.mRootView = rootView;
                ai.mItem = item;
                ai.mHolder = this;
                ai.mFirstRunTime = item.mActiveSince;
                if (item.mBackground) {
//                    description.setText(rootView.getContext().getText(
//                            R.string.cached));
                } else {
                    description.setText(item.mDescription);
                }
                item.mCurSizeStr = null;
                icon.setImageDrawable(item.loadIcon(rootView.getContext(),
                        state));
                icon.setVisibility(View.VISIBLE);
                ai.updateTime(rootView.getContext(), builder);
                return ai;
            }
        }
    }

    void refreshUi(boolean dataChanged) {
        Log.i(TAG, "refreshUi() dataChanged=" + dataChanged);
        if (dataChanged) {
            Log.i(TAG, "refreshUi() dataChanged=" + dataChanged);
            ServiceListAdapter adapter = mAdapter;
            adapter.refreshItems();
            adapter.notifyDataSetChanged();

            Log.i(TAG, "refreshUi() adapter.getCount=" + adapter.getCount());
        }

        if (mDataAvail != null) {
            mDataAvail.run();
            mDataAvail = null;
        }

        mMemInfoReader.readMemInfo();

        synchronized (mState.mLock) {
            if (mCurShowCached != mAdapter.mShowBackground) {
                mCurShowCached = mAdapter.mShowBackground;
                if (mCurShowCached) {
                    mHeaderText.setText(getResources().getString(
                            R.string.running_processes_header_apps));
                }
            }

            final long totalRam = mMemInfoReader.getTotalSize();
            final long medRam;
            final long lowRam;
            if (mCurShowCached) {
                lowRam = mMemInfoReader.getFreeSize()
                        + mMemInfoReader.getCachedSize();
                medRam = mState.mBackgroundProcessMemory;
            } else {
                lowRam = mMemInfoReader.getFreeSize()
                        + mMemInfoReader.getCachedSize()
                        + mState.mBackgroundProcessMemory;
                medRam = mState.mServiceProcessMemory;

            }
            final long highRam = totalRam - lowRam;
            Log.i(TAG,
                    "totalRam="
                            + BidiFormatter.getInstance().unicodeWrap(
                                    Formatter.formatShortFileSize(this,
                                            totalRam))
                            + " > medRam="
                            + BidiFormatter.getInstance()
                                    .unicodeWrap(
                                            Formatter.formatShortFileSize(this,
                                                    medRam))
                            + " > lowRam="
                            + BidiFormatter.getInstance()
                                    .unicodeWrap(
                                            Formatter.formatShortFileSize(this,
                                                    lowRam))
                            + " > highRam="
                            + BidiFormatter.getInstance()
                                    .unicodeWrap(
                                            Formatter.formatShortFileSize(this,
                                                    highRam)));

            if (mCurTotalRam != totalRam || mCurHighRam != highRam
                    || mCurMedRam != medRam || mCurLowRam != lowRam) {
                Log.i(TAG, "running size = "
                        + mAm.getRunningAppProcesses().size()
                        + " ;CurrentBackgroundItems="
                        + mState.getCurrentBackgroundItems().size());
                Log.i(TAG, "mBackgroundProcessText totalRam=" + totalRam
                        + "mCurTotalRam=" + mCurTotalRam);
                mCurTotalRam = totalRam;
                mCurHighRam = highRam;
                mCurMedRam = medRam;
                mCurLowRam = lowRam;
                BidiFormatter bidiFormatter = BidiFormatter.getInstance();
                String sizeStr = bidiFormatter.unicodeWrap(Formatter
                        .formatShortFileSize(this, lowRam));

                String totalSizeStr = bidiFormatter.unicodeWrap(Formatter
                        .formatShortFileSize(this, mCurTotalRam));
                Log.i(TAG, "mBackgroundProcessText lowRam=" + sizeStr);
                Log.i(TAG,
                        "mBackgroundProcessText * totalRam="
                                + bidiFormatter.unicodeWrap(Formatter
                                        .formatShortFileSize(this, totalRam))
                                + "mCurTotalRam="
                                + bidiFormatter.unicodeWrap(Formatter
                                        .formatShortFileSize(this, mCurTotalRam)));
                sizeStr = bidiFormatter.unicodeWrap(Formatter
                        .formatShortFileSize(this, medRam));
                Log.i(TAG, "mAppsProcessText medRam=" + sizeStr);

                Log.i(TAG, "mAdapter.getCount()=" + mAdapter.getCount()
                        + " ; sizeStr=" + sizeStr);

                sizeStr = bidiFormatter.unicodeWrap(Formatter
                        .formatShortFileSize(this, highRam));
                Log.i(TAG, "mForegroundProcessText highRam=" + sizeStr);

                if (mRubbsh == null) {
                    mLoadedText.setText(getResources().getString(
                            R.string.used_ram,
                            Integer.valueOf(mAdapter.getCount()).toString(),
                            getBackgroundSize()));
                    mStart.setClickable(true);
                    mStart.setBackground(getResources().getDrawable(
                            R.drawable.start_memory_button));
                    mStart.setText(getResources().getString(
                            R.string.start_memory_button));
                } else {
                    Log.i(TAG, "mRubbsh :" + mRubbsh);
                    mLoadedText.setText(getResources().getString(
                            R.string.can_rubbish_ram,
                            sizeStr + "/" + totalSizeStr, mRubbsh));
                    mRubbsh = null;
                    mStart.setBackground(getResources().getDrawable(
                            R.drawable.start_memory_button));
                    mStart.setText(getResources().getString(
                            R.string.end_memory_button));
                }
                mUsedPercentText.setText(getResources().getString(
                        R.string.gs_per_used,
                        MemoryUtils.formatPercentage(highRam, totalRam)));
                mDrawableId = MemoryUtils.getPicId(drawableIds.length, MemoryUtils.formatPercentage(highRam, totalRam));
                Log.i(TAG, "mDrawableId 1 = " + mDrawableId);
                Log.i(TAG, "highRam/totalRam=" + (highRam / totalRam)
                        + " ; mDrawableId=" + mDrawableId);
                mGsCircle.setBackground(getResources().getDrawable(
                        drawableIds[mDrawableId]));
            }
        }
    }

    @Override
    public void onRefreshUi(int what) {
        switch (what) {
        case REFRESH_TIME:
            Log.i(TAG, "...REFRESH_TIME...");
            updateTimes();
            break;
        case REFRESH_DATA:
            Log.i(TAG, "...REFRESH_DATA...");
            if (mRubbsh != null) {
                refreshUi(true);
            } else {
                refreshUi(false);
            }
            updateTimes();
            break;
        case REFRESH_STRUCTURE:
            Log.i(TAG, "...REFRESH_STRUCTURE...");
            refreshUi(true);
            updateTimes();
            break;
        }
    }

    void updateTimes() {
        Iterator<ActiveItem> it = mActiveItems.values().iterator();
        while (it.hasNext()) {
            ActiveItem ai = it.next();
            if (ai.mRootView.getWindowToken() == null) {
                // Clean out any dead views, just in case.
                it.remove();
                continue;
            }
            ai.updateTime(this, mBuilder);
        }
    }

    public void onMovedToScrapHeap(View view) {
        mActiveItems.remove(view);
    }

    public void onItemClick(AdapterView<?> parent, View v, int position, long id) {
        ListView l = (ListView) parent;
        RunningState.MergedItem mi = (RunningState.MergedItem) l.getAdapter()
                .getItem(position);
        mCurSelected = mi;
        startServiceDetailsActivity(mi);
    }

    private void startServiceDetailsActivity(RunningState.MergedItem mi) {
        Log.i(TAG, "startServiceDetailsActivity...");
        if (mi != null) {
            // start new fragment to display extended information
            Intent intent = new Intent();
            intent.setAction("android.settings.APPLICATION_DETAILS_SETTINGS");
            Uri uri = Uri.fromParts("package", mi.mPackageInfo.packageName,
                    null);
            intent.setData(uri);
            mContext.startActivity(intent);
        }
    }

    public static class ActiveItem {
        public View mRootView;
        RunningState.BaseItem mItem;
        ActivityManager.RunningServiceInfo mService;
        ViewHolder mHolder;
        long mFirstRunTime;
        boolean mSetBackground;

        public void updateTime(Context context, StringBuilder builder) {
            TextView uptimeView = null;

            if (mItem instanceof RunningState.ServiceItem) {
                // If we are displaying a service, then the service
                // uptime goes at the top.
                uptimeView = mHolder.size;

            } else {
                String size = mItem.mSizeStr != null ? mItem.mSizeStr : "";
                if (!size.equals(mItem.mCurSizeStr)) {
                    mItem.mCurSizeStr = size;
                    mHolder.size.setText(size);
                }

                if (mItem.mBackground) {
                    // This is a background process; no uptime.
                    if (!mSetBackground) {
                        mSetBackground = true;
                        mHolder.uptime.setText("");
                    }
                } else if (mItem instanceof RunningState.MergedItem) {
                    // This item represents both services and processes,
                    // so show the service uptime below.
                    uptimeView = mHolder.uptime;
                }
            }

            if (uptimeView != null) {
                mSetBackground = false;
                if (mFirstRunTime >= 0) {
                    // Log.i("foo", "Time for " + mItem.mDisplayLabel
                    // + ": " + (SystemClock.uptimeMillis()-mFirstRunTime));
                    uptimeView
                            .setText(DateUtils.formatElapsedTime(
                                    builder,
                                    (SystemClock.elapsedRealtime() - mFirstRunTime) / 1000));
                } else {
                    boolean isService = false;
                    if (mItem instanceof RunningState.MergedItem) {
                        isService = ((RunningState.MergedItem) mItem).mServices
                                .size() > 0;
                    }
                    if (isService) {
                        uptimeView.setText("restarting");
                    } else {
                        uptimeView.setText("");
                    }
                }
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.i(TAG, "----onResume----");
        mState.resume(this);
        Log.i(TAG, "---> mState.hasData()=" + mState.hasData());
        if (mState.hasData()) {
            // If the state already has its data, then let's populate our
            // list right now to avoid flicker.
            Log.i(TAG, "...onResume...");
            refreshUi(true);
            mFlag = true;
        } else {
            mDataAvail = mRunningProcessesAvail;
            mFlag = false;
        }
        Log.i(TAG, " mFlag=" + mFlag);
        MemoryUtils.handleLoadingContainer(mLoadingContainer, mNeedLoading,
                mFlag, false);
    }

    private final Runnable mRunningProcessesAvail = new Runnable() {
        @Override
        public void run() {
            Log.i(TAG, "mLoadingContainer=" + mLoadingContainer + " ; mView="
                    + mView);
            MemoryUtils.handleLoadingContainer(mLoadingContainer, mNeedLoading,
                    true, true);
        }
    };

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
        case R.id.start_memory_button: {
            Log.i(TAG, "equals? " + getBackgroundSize().equals("0KB"));
            if (!getBackgroundSize().equals("0KB")) {
                mHandler.sendEmptyMessage(IS_ANIMATE);
                mStart.setBackground(getResources().getDrawable(
                        R.drawable.end_memory_button));
                mStart.setText(getResources().getString(
                        R.string.doing_memory_button));
                mStart.setClickable(false);
                Log.i(TAG, "start_memory_button###");
                mState.removeAllMessage();
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        Log.i(TAG, "run()");
                        removeBackgroundProc();
                    }
                }).start();
            } else {
                Toast.makeText(mContext,
                        getResources().getString(R.string.nothing_can_removed),
                        Toast.LENGTH_SHORT).show();
            }
        }
            break;
        default:
            Log.i(TAG, "v.getId()=" + v.getId());
            Log.i(TAG, "R.id.start_memory_button=" + R.id.start_memory_button);
        }
    }

    private void removeBackgroundProc() {
        List<RunningAppProcessInfo> infoList = mAm.getRunningAppProcesses();

        ActivityManager.MemoryInfo memInfo = new ActivityManager.MemoryInfo();

        long beforeMem = getHighRam();
        int count = 0;
        for (int i = 0; i < mAdapter.getCount(); i++) {
            Object item = mAdapter.getItem(i);
            if (item instanceof RunningState.MergedItem) {
                mFlag = false;
                RunningAppProcessInfo appProcessInfo = ((RunningState.MergedItem) item).mProcess.mRunningProcessInfo;
                Log.d(TAG, i + " -> process name : "
                        + appProcessInfo.processName + " importance : "
                        + appProcessInfo.importance);
                if (appProcessInfo.importance >= RunningAppProcessInfo.IMPORTANCE_SERVICE) {
                    String[] pkgList = appProcessInfo.pkgList;
                    for (int j = 0; j < pkgList.length; ++j) {
                        Log.d(TAG, j + ": It will be killed, package name : "
                                + pkgList[j]);
                        mAm.killBackgroundProcesses(pkgList[j]);
                        mHandler.removeMessages(UPDATE_PER);
                        mHandler.sendEmptyMessage(UPDATE_PER);
                        mFlag = true;
                    }
                }
                if (mFlag)
                    count++;
            }
        }

        long afterMem = getHighRam();
        BidiFormatter bidiFormatter = BidiFormatter.getInstance();
        Log.i(TAG,
                "clear "
                        + count
                        + " process, "
                        + bidiFormatter.unicodeWrap(Formatter
                                .formatShortFileSize(this,
                                        (beforeMem - afterMem))));
        if (beforeMem - afterMem > 0 && count > 0) {
            mRubbsh = bidiFormatter.unicodeWrap(Formatter.formatShortFileSize(
                    this, beforeMem - afterMem));
        } else {
            mRubbsh = "0KB";
        }
        Log.i(TAG, "mRubbsh 1:" + mRubbsh);
        Message message = new Message();
        message.obj = "clear " + count + " process, " + mRubbsh;
        message.what = REMOVE_END;
        mHandler.sendMessage(message);
        Log.i(TAG, "end###");
    }

    private Handler mHandler = new Handler() {
        private int id;
        private int tempId = mDrawableId;

        public void handleMessage(Message msg) {
            switch (msg.what) {
            case REMOVE_END:
                // Toast.makeText(mContext, "清理完成1: " + msg.obj.toString(),
                // Toast.LENGTH_SHORT).show();
                // int id = (int) Math
                // .floor(((double) getHighRam() / (double) mCurTotalRam)
                // * drawableIds.length);
                Log.i(TAG, "mRubbsh 2:" + mRubbsh);
                Log.i(TAG, "mDrawableId=" + mDrawableId + " tempId:" + tempId
                        + "  id=" + id);
                if (tempId == id) {
                    Log.i(TAG, "mRubbsh 3:" + mRubbsh);
                    removeMessages(IS_ANIMATE);
                    Log.i(TAG, "...Handler...");
                    removeMessages(REMOVE_END);
                    Log.i(TAG, "............");
                    Log.i(TAG, "mRubbsh 3:" + mRubbsh);
                    Log.i(TAG, "...REFRESH_STRUCTURE...");
                    mState.sendUpdateMessage();
                    break;
                }
                removeMessages(REMOVE_END);
                Message m1 = obtainMessage(REMOVE_END);
                sendMessageDelayed(m1, TIME_UPDATE_DELAY);
                break;
            case IS_ANIMATE:
                Log.i(TAG, "...IS_ANIMATE...mDrawableId=" + mDrawableId);
                Log.i(TAG, "...IS_ANIMATE...tempId=" + tempId);
                tempId++;
                if (tempId >= 36) {
                    tempId = 0;
                }
                Log.i(TAG, "\t\t...tempId=" + tempId);
                mGsCircle.setBackground(getResources().getDrawable(
                        drawableIds[tempId]));
                removeMessages(IS_ANIMATE);
                Message m = obtainMessage(IS_ANIMATE);
                sendMessageDelayed(m, TIME_UPDATE_DELAY);
                break;
            case UPDATE_PER:
                mUsedPercentText.setText(getResources().getString(
                        R.string.gs_per_used,
                        MemoryUtils
                                .formatPercentage(getHighRam(), mCurTotalRam)));
                id = MemoryUtils.getPicId(drawableIds.length, MemoryUtils
                        .formatPercentage(getHighRam(), mCurTotalRam));
                break;
            }
        }
    };

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        onBackPressed();
        return true;
    }

    long getHighRam() {
        mMemInfoReader.readMemInfo();
        final long totalRam = mMemInfoReader.getTotalSize();
        final long medRam;
        final long lowRam;
        if (mCurShowCached) {
            lowRam = mMemInfoReader.getFreeSize()
                    + mMemInfoReader.getCachedSize();
            medRam = mState.mBackgroundProcessMemory;
        } else {
            lowRam = mMemInfoReader.getFreeSize()
                    + mMemInfoReader.getCachedSize()
                    + mState.mBackgroundProcessMemory;
            medRam = mState.mServiceProcessMemory;

        }
        final long highRam = totalRam - lowRam;
        mCurTotalRam = totalRam;

        return highRam;
    }

    String getBackgroundSize() {
        long size = 0;
        for (int i = 0; i < mAdapter.getCount(); i++) {
            Object item = mAdapter.getItem(i);
            if (item instanceof RunningState.MergedItem) {
                size += ((RunningState.MergedItem) item).mSize;
            }
        }
        final BidiFormatter bidiFormatter = BidiFormatter.getInstance();
        String sizeStr = size > 0 ? bidiFormatter.unicodeWrap(Formatter
                .formatShortFileSize(this, size)) : "0KB";
        return sizeStr;
    }

    @Override
    protected void onDestroy() {
        if (mState != null) {
            mState.removeAllMessage();
        }
        super.onDestroy();
    }

    @Override
    protected void onPause() {
        super.onPause();
        mDataAvail = null;
        mState.pause();

    }
}
