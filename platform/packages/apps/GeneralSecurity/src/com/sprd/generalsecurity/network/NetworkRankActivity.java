package com.sprd.generalsecurity.network;

import android.app.Activity;
import android.app.AppGlobals;
import android.app.LoaderManager.LoaderCallbacks;
import android.content.Context;
import android.content.Loader;
import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.content.SharedPreferences;
import android.graphics.drawable.Drawable;
import android.net.INetworkStatsService;
import android.net.INetworkStatsSession;
import android.net.NetworkStats;
import android.net.NetworkTemplate;
import android.net.TrafficStats;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.os.UserManager;
import android.preference.PreferenceManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;

import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.DateCycleUtils;
import com.sprd.generalsecurity.utils.TeleUtils;
import com.sprd.generalsecurity.utils.Formatter;

import java.lang.Override;
import java.lang.Throwable;
import java.util.ArrayList;
import java.util.Collections;

import static android.net.NetworkTemplate.buildTemplateMobileAll;
import static android.net.NetworkTemplate.buildTemplateWifiWildcard;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.os.SystemProperties;

import java.util.List;

public class NetworkRankActivity extends Activity {

    private static final String TAG = "NetworkRankActivity";
    private INetworkStatsSession mStatsSession;
    private INetworkStatsService mStatsService;
    private NetworkTemplate mTemplate;
    private NetworkStats mStats;
    private long mWifiUsageTotal;
    private long mSim1UsageTotal;
    private long mSim2UsageTotal;

    private boolean mDualSimRelease = false;

    ArrayList<UidDetail> mApps;
    SparseArray<UidDetail> mKnowApps;

    private enum NetworkType {
        SIM1, SIM2, WIFI
    }

    private int mDataCycleSim1;
    private int mDataCycleSim2;
    private int mDataCycleWIFI;

    private TextView mSim1FlowTotalText;
    private TextView mSim2FlowTotalText;
    private TextView mWifiFlowTotalText;

    private AppRankAdapter mAdapter;
    private ListView mListView;

    private int WIFI_ID =0;
    private int SIM1_ID =1;
    private int SIM2_ID =2;

    private final int MSG_SIM1_STATS_LOAD_FINISHED = 0;
    private final int MSG_SIM2_STATS_LOAD_FINISHED = 1;
    private final int MSG_WIFI_STATS_LOAD_FINISHED = 2;

    SubscriptionManager mSubscriptionManager;
    SubscriptionInfo mSubInfo1, mSubInfo2;

    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            Log.e(TAG, "handleMessage:" + msg.what);
            switch (msg.what) {
                case MSG_SIM1_STATS_LOAD_FINISHED:
                    if (!mDualSimRelease) {
                        updateNetworkUsageStat(NetworkType.WIFI, mDataCycleWIFI);
                    } else {
                        updateNetworkUsageStatForDualSim(SIM2_ID);
                    }
                    break;
                case MSG_SIM2_STATS_LOAD_FINISHED:
                    updateNetworkUsageStat(NetworkType.WIFI, mDataCycleWIFI);
                    break;
                case MSG_WIFI_STATS_LOAD_FINISHED:
                    Log.e(TAG, "MSG_WIFI_STATS_LOAD_FINISHED");
                    ///
//                    enableTestMode();
                    ///
                    Collections.sort(mApps);
                    mAdapter.notifyDataSetChanged();
                    Log.e(TAG, "notifyDataSetChanged");
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setHomeButtonEnabled(true);

        mStatsService = INetworkStatsService.Stub.asInterface(
                ServiceManager.getService(Context.NETWORK_STATS_SERVICE));
        try {
            mStatsSession = mStatsService.openSession();
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }

        mApps = new ArrayList<UidDetail>();
        mKnowApps = new SparseArray<UidDetail>();
        mWifiUsageTotal = 0;

        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        String v = sharedPref.getString(DateCycleUtils.KEY_DATA_CYCLE_SIM1, "0");
        mDataCycleSim1 = Integer.parseInt(v);

        v = sharedPref.getString(DateCycleUtils.KEY_DATA_CYCLE_SIM2, "0");
        mDataCycleSim2 = Integer.parseInt(v);

        if (TeleUtils.getSimCount(this) == 0 || TeleUtils.getSimCount(this) == 1) {
            mDualSimRelease = false;
        } else {
            mDualSimRelease = true;
        }

        if (TeleUtils.getPrimaryCard(this) == 0) {
            mDataCycleWIFI = mDataCycleSim1;
        } else {
            mDataCycleWIFI = mDataCycleSim2;
        }

        ///
//        enableTestMode();
        ///

        if (!mDualSimRelease) {
            setContentView(R.layout.app_flow_rank);
            //Wifi total usage
            mWifiFlowTotalText = (TextView) findViewById(R.id.flowWifi);
            mWifiFlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_wifi),
                    Formatter.formatFileSize(this, mWifiUsageTotal, false)));

            //Sim1 usage total
            mSim1FlowTotalText = (TextView) findViewById(R.id.flowSim1);
            mSim1FlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_data),
                    Formatter.formatFileSize(this, mSim1UsageTotal, false)));
        } else {
            setContentView(R.layout.app_flow_rank_dual);
            //Wifi total usage
            mWifiFlowTotalText = (TextView) findViewById(R.id.flowWifi);
            mWifiFlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_wifi),
                    Formatter.formatFileSize(this, mWifiUsageTotal, false)));

            //Sim1 usage total
            mSim1FlowTotalText = (TextView) findViewById(R.id.flowSim1);
            mSim1FlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_data1),
                    Formatter.formatFileSize(this, mSim1UsageTotal, false)));

            //Sim2 usage total
            mSim2FlowTotalText = (TextView) findViewById(R.id.flowSim2);
            mSim2FlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_data2),
                    Formatter.formatFileSize(this, mSim2UsageTotal, false)));
        }

        mListView = (ListView) findViewById(android.R.id.list);
        mAdapter = new AppRankAdapter(this);
        mListView.setAdapter(mAdapter);

        mSubscriptionManager = SubscriptionManager.from(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        initStat();

        try {
            mStatsService.forceUpdate();
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
        if (mDualSimRelease) {
            mSubscriptionManager = SubscriptionManager.from(this);
            SubscriptionInfo sir2 ;

            mSubscriptionManager = SubscriptionManager.from(this);
            SubscriptionInfo tmpInfo = null ;

            for (int i = 0; i <= 1; i++) {
                tmpInfo = mSubscriptionManager
                        .getActiveSubscriptionInfoForSimSlotIndex(i);

                if (tmpInfo != null && tmpInfo.getSimSlotIndex() == 0) {
                    mSubInfo1 = tmpInfo;
                } else {
                    mSubInfo2 = tmpInfo;
                }
            }

            Log.e(TAG, "info:" + mSubInfo1 + ":" + mSubInfo2);

            updateNetworkUsageStatForDualSim(SIM1_ID);
        } else {
            updateNetworkUsageStat(NetworkType.SIM1, mDataCycleSim1);
        }
    }

    /**
     * For dual sim release, the phsical sim id may not consist with the ID shown in Settins.
     * To resolve this, need get SubscriptionInfo and check the SubscriptionInfo.getSimSlotIndex().
     * @param slotIndex same with shown in Settings
     */

    private void updateNetworkUsageStatForDualSim(int slotIndex) {
        NetworkTemplate template;


        // kick off loader for sim1 detailed stats
        if (slotIndex == SIM1_ID) {
            template = buildTemplateMobileAll(
                    getActiveSubscriberId(this, 0/*mSubInfo1.getSubscriptionId()*/));
            getLoaderManager().restartLoader(SIM1_ID,
                    SummaryForAllUidLoader.buildArgs(template, mDataCycleSim1,
                            System.currentTimeMillis()), mSummaryCallbacks);
        } else {
            if (mSubInfo2 != null) {
                template = buildTemplateMobileAll(
                        getActiveSubscriberId(this, mSubInfo2.getSubscriptionId()));
                getLoaderManager().restartLoader(SIM2_ID,
                        SummaryForAllUidLoader.buildArgs(template, mDataCycleSim2,
                                System.currentTimeMillis()), mSummaryCallbacks);
            } else {
                //for test only
                template = buildTemplateMobileAll(
                        getActiveSubscriberId(this, 0/*mSubInfo1.getSubscriptionId()*/));
                getLoaderManager().restartLoader(SIM2_ID,
                        SummaryForAllUidLoader.buildArgs(template, mDataCycleSim2,
                                System.currentTimeMillis()), mSummaryCallbacks);
            }
        }
    }

    private void initStat() {
        mKnowApps.clear();
        mApps.clear();
        mSim1UsageTotal = 0;
        mWifiUsageTotal = 0;
        mSim2UsageTotal = 0;
        Log.e(TAG, "data clear");
        mAdapter.notifyDataSetChanged();
    }

    private static String getActiveSubscriberId(Context context, int subId) {
        final TelephonyManager tele = TelephonyManager.from(context);
        String retVal = tele.getSubscriberId(subId);
        Log.d(TAG, "getActiveSubscriberId=" + retVal + " subId=" + subId);
        return retVal;
    }

    private static String getActiveSubscriberId(Context context) {
        final TelephonyManager tele = TelephonyManager.from(context);
        final String actualSubscriberId = tele.getSubscriberId();
        String TEST_SUBSCRIBER_PROP = "test.subscriberid";
        String retVal = SystemProperties.get(TEST_SUBSCRIBER_PROP, actualSubscriberId);
        Log.d(TAG, "getActiveSubscriberId111=" + retVal + " actualSubscriberId=" + actualSubscriberId);
        return retVal;
    }

    private void showSubscriberInfo(Context context) {
        SubscriptionManager sm = SubscriptionManager.from(context);
        List<SubscriptionInfo> subInfoList = mSubscriptionManager.getActiveSubscriptionInfoList();
        if (subInfoList != null) {
            for (SubscriptionInfo subInfo : subInfoList) {
                Log.e(TAG, "---------info:" + String.valueOf(subInfo.getSubscriptionId()) + ":" + subInfo.toString());
            }
        }

        getActiveSubscriberId(context);
    }

    private int collopseUid(int uid) {
        if (!UserHandle.isApp(uid) && (uid != TrafficStats.UID_REMOVED) && (uid != TrafficStats.UID_TETHERING)) {
            uid = android.os.Process.SYSTEM_UID;
        }
        return uid;
    }

    private final LoaderCallbacks<NetworkStats> mSummaryCallbacks = new LoaderCallbacks<
            NetworkStats>() {
        @Override
        public Loader<NetworkStats> onCreateLoader(int id, Bundle args) {
            return new SummaryForAllUidLoader(NetworkRankActivity.this, mStatsSession, args);
        }

        @Override
        public void onLoadFinished(Loader<NetworkStats> loader, NetworkStats data) {
            NetworkStats.Entry entry = null;

            Log.e(TAG, "onLoadFinished : begin data handling==::::::::::");
            if (loader.getId() == WIFI_ID) {
                mWifiUsageTotal = 0;

                for (UidDetail app : mApps) {
                    app.usageWifi = 0;
                }

                final int size = data != null ? data.size() : 0;
                for (int i = 0; i < size; i++) {
                    entry = data.getValues(i, entry);

                    // Decide how to collapse items together
                    final int uid = collopseUid(entry.uid);
                    UidDetail dt = mKnowApps.get(uid);

                    if (dt == null) {

                        dt = buildUidDetail(uid);
                        if (dt != null) {
                            mKnowApps.put(uid, dt);
                            mApps.add(dt);
                        }
                    }
                    if (dt == null) continue;
                    dt.usageWifi += (entry.rxBytes + entry.txBytes);
                    dt.usageSum = dt.usageSim1 + dt.usageSim2 + dt.usageWifi;
//                    Log.e(TAG, "add UID=========================" + uid + ":" + entry.rxBytes + ":" + entry.txBytes);
                    mWifiUsageTotal += (entry.rxBytes + entry.txBytes);
                    Log.e(TAG, "add UID=========================" + uid + ":" + entry.rxBytes + ":" + entry.txBytes + ":" + mWifiUsageTotal);
                    mWifiFlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_wifi),
                            Formatter.formatFileSize(NetworkRankActivity.this, mWifiUsageTotal, false)));
                }
                mHandler.sendEmptyMessage(MSG_WIFI_STATS_LOAD_FINISHED);
            } else if (loader.getId() == SIM1_ID) {
                mSim1UsageTotal = 0;
                for (UidDetail app : mApps) {
                    app.usageSim1 = 0;
                }

                final int size = data != null ? data.size() : 0;
                Log.e(TAG, "callback sim1:" + size);
                for (int i = 0; i < size; i++) {
                    entry = data.getValues(i, entry);

                    // Decide how to collapse items together
                    final int uid = collopseUid(entry.uid);
                    UidDetail dt = mKnowApps.get(uid);
                    if (dt == null) {
                        dt = buildUidDetail(uid);
                        if (dt != null) {
                            mKnowApps.put(uid, dt);
                            mApps.add(dt);
                        }
                    }
                    if (dt == null) continue;
                    dt.usageSim1 += (entry.rxBytes + entry.txBytes);
                    mSim1UsageTotal += (entry.rxBytes + entry.txBytes);
                    dt.usageSum = dt.usageSim1 + dt.usageSim2 + dt.usageWifi;
                    if (mDualSimRelease) {
                        mSim1FlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_data1),
                                Formatter.formatFileSize(NetworkRankActivity.this, mSim1UsageTotal, false)));
                    } else {
                        mSim1FlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_data),
                                Formatter.formatFileSize(NetworkRankActivity.this, mSim1UsageTotal, false)));
                    }
                }
                mHandler.sendEmptyMessage(MSG_SIM1_STATS_LOAD_FINISHED);
            }else {
                mSim2UsageTotal = 0;
                for (UidDetail app : mApps) {
                    app.usageSim2 = 0;
                }

                    final int size = data != null ? data.size() : 0;
                    for (int i = 0; i < size; i++) {
                        entry = data.getValues(i, entry);

                        // Decide how to collapse items together
                        final int uid = collopseUid(entry.uid);

                        UidDetail dt = mKnowApps.get(uid);
                        if (dt == null) {
                            dt = buildUidDetail(uid);
                            if (dt != null) {
                                mKnowApps.put(uid, dt);
                                mApps.add(dt);
                            }
                        }
                        if (dt == null) continue;
                        dt.usageSim2 += (entry.rxBytes + entry.txBytes);
                        mSim2UsageTotal += (entry.rxBytes + entry.txBytes);
                        dt.usageSum = dt.usageSim1 + dt.usageSim2 + dt.usageWifi;
                        mSim2FlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_data2),
                                Formatter.formatFileSize(NetworkRankActivity.this, mSim2UsageTotal, false)));
                    }
                mHandler.sendEmptyMessage(MSG_SIM2_STATS_LOAD_FINISHED);
            }
            Log.e(TAG, "onLoadFinished : end data handling==================");
        }


        @Override
        public void onLoaderReset(Loader<NetworkStats> loader) {

        }
    };

    void updateNetworkUsageStat(NetworkType networkType, int cycleType) {
        long t = 0;
        Log.e(TAG, "updateNetworkUsageStat:" + networkType + ":" + Log.getStackTraceString(new Throwable()));
        switch (cycleType) {
            case DateCycleUtils.CYCLE_MONTH:
                t = DateCycleUtils.getMonthCycleStart();
                break;
            case DateCycleUtils.CYCLE_WEEK:
                t = DateCycleUtils.getWeekCycleStart();
                break;
            case DateCycleUtils.CYCLE_DAY:
                t = DateCycleUtils.getDayCycleStart();
                break;
        }

        if (networkType == NetworkType.WIFI) {
            mTemplate = buildTemplateWifiWildcard();
//            try {
//                mStats = mStatsSession.getSummaryForAllUid(mTemplate, t, System.currentTimeMillis(), false);
//            } catch (RemoteException e) {
//                e.printStackTrace();
//            }

            getLoaderManager().restartLoader(WIFI_ID,
                    SummaryForAllUidLoader.buildArgs(mTemplate, t, System.currentTimeMillis()), mSummaryCallbacks);

//            NetworkStats.Entry entry = null;
//
//            final int size = mStats != null ? mStats.size() : 0;
//            for (int i = 0; i < size; i++) {
//                entry = mStats.getValues(i, entry);
//
//                // Decide how to collapse items together
//                final int uid = entry.uid;
//                UidDetail dt = mKnowApps.get(entry.uid);
//
//                if (dt == null) {
//                    Log.e(TAG, "add UID=========================" + uid + ":" + entry.rxBytes + ":" + entry.txBytes);
//                    dt = buildUidDetail(uid);
//                    if (dt != null) {
//                        mKnowApps.put(entry.uid, dt);
//                        mApps.add(dt);
//                    }
//                }
//                if (dt == null) continue;
//                dt.usageWifi += (entry.rxBytes + entry.txBytes);
//                mWifiUsageTotal += (entry.rxBytes + entry.txBytes);
//                mWifiFlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_wifi),
//                        Formatter.formatFileSize(this, mWifiUsageTotal)));
//            }
        } else if (networkType == NetworkType.SIM1){
            Log.e(TAG, "load sim1:" + mDualSimRelease);
            //mobile
            if (mDualSimRelease) {
                mTemplate = buildTemplateMobileAll(getActiveSubscriberId(this, SIM1_ID));
                getLoaderManager().restartLoader(SIM1_ID,
                        SummaryForAllUidLoader.buildArgs(mTemplate, t, System.currentTimeMillis()), mSummaryCallbacks);
            } else {
                String subscriberId = getActiveSubscriberId(this);
                mTemplate = buildTemplateMobileAll(subscriberId);

                getLoaderManager().restartLoader(SIM1_ID,
                        SummaryForAllUidLoader.buildArgs(mTemplate, t, System.currentTimeMillis()), mSummaryCallbacks);
            }
        } else if (networkType == NetworkType.SIM2){
            //mobile
            mTemplate = buildTemplateMobileAll(getActiveSubscriberId(this, SIM2_ID));
            getLoaderManager().restartLoader(SIM2_ID,
                SummaryForAllUidLoader.buildArgs(mTemplate, t, System.currentTimeMillis()), mSummaryCallbacks);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        onBackPressed();
        return true;
    }

    class UidDetail implements Comparable<UidDetail>{
        public CharSequence label;
        public CharSequence contentDescription;
        public CharSequence[] detailLabels;
        public CharSequence[] detailContentDescriptions;
        public Drawable icon;
        public long usageWifi;
        public long usageSim1;
        public long usageSim2;
        public long usageSum;

        @Override
        public int compareTo(UidDetail another) {
            return (int)(another.usageSum - usageSum);
        }
    }

    /**
     * Build {@link UidDetail} object, blocking until all {@link Drawable}
     * lookup is finished.
     */
    private UidDetail buildUidDetail(int uid) {
        final Resources res = getResources();
        final PackageManager pm = getPackageManager();

        final UidDetail detail = new UidDetail();
        detail.label = pm.getNameForUid(uid);
        detail.icon = pm.getDefaultActivityIcon();

        // handle special case labels
        switch (uid) {
            case android.os.Process.SYSTEM_UID:
                detail.label = getResources().getString(R.string.android_os);
                detail.icon = pm.getDefaultActivityIcon();

                return detail;
            case TrafficStats.UID_REMOVED:
                detail.label = getResources().getString(R.string.app_removed);
                detail.icon = pm.getDefaultActivityIcon();

                return detail;
            case TrafficStats.UID_TETHERING:

                return null;
            default:

        }

        final UserManager um = (UserManager) getSystemService(Context.USER_SERVICE);

        // otherwise fall back to using packagemanager labels
        final String[] packageNames = pm.getPackagesForUid(uid);
        final int length = packageNames != null ? packageNames.length : 0;
        try {
            final int userId = UserHandle.getUserId(uid);
            UserHandle userHandle = new UserHandle(userId);
            if (userHandle.isApp(uid) == false) {
//                return null;
            }
            IPackageManager ipm = AppGlobals.getPackageManager();

            if (length == 1) {
                final ApplicationInfo info = ipm.getApplicationInfo(packageNames[0],
                        0 /* no flags */, userId);
                if (info != null) {
                    detail.label = info.loadLabel(pm).toString();
                    detail.icon = um.getBadgedIconForUser(info.loadIcon(pm),
                            new UserHandle(userId));
                }
            } else if (length > 1) {
                detail.detailLabels = new CharSequence[length];
                detail.detailContentDescriptions = new CharSequence[length];
                for (int i = 0; i < length; i++) {
                    final String packageName = packageNames[i];
                    final PackageInfo packageInfo = pm.getPackageInfo(packageName, 0);
                    final ApplicationInfo appInfo = ipm.getApplicationInfo(packageName,
                            0 /* no flags */, userId);

                    if (appInfo != null) {
                        detail.detailLabels[i] = appInfo.loadLabel(pm).toString();
                        detail.detailContentDescriptions[i] = um.getBadgedLabelForUser(
                                detail.detailLabels[i], userHandle);
                        if (packageInfo.sharedUserLabel != 0) {
                            detail.label = pm.getText(packageName, packageInfo.sharedUserLabel,
                                    packageInfo.applicationInfo).toString();
                            detail.icon = um.getBadgedIconForUser(appInfo.loadIcon(pm), userHandle);
                        }
                    }
                }
            }
            detail.contentDescription = um.getBadgedLabelForUser(detail.label, userHandle);
        } catch (NameNotFoundException e) {
            Log.w(TAG, "Error while building UI detail for uid "+uid, e);
        } catch (RemoteException e) {
            Log.w(TAG, "Error while building UI detail for uid "+uid, e);
        }

        if (TextUtils.isEmpty(detail.label)) {
            detail.label = Integer.toString(uid);
        }

        return detail;
    }

    class ViewHolder {
        ImageView img;
        TextView tv;
        TextView flow_data;
        TextView flow_data2;
        TextView flow_wifi;
        TextView flow_sum;
    }

    class AppRankAdapter extends BaseAdapter {
        private LayoutInflater mInflater;

        public AppRankAdapter(Context context) {
            this.mInflater = LayoutInflater.from(context);
        }

        @Override
        public int getCount() {
            return mApps.size();
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public Object getItem(int position) {
            return null;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            Log.e(TAG, "--------getView start");
            ViewHolder holder = new ViewHolder();
            if (convertView == null) {
                if (!mDualSimRelease) {
                    convertView = mInflater.inflate(R.layout.flow_rank_item, null);
                } else {
                    convertView = mInflater.inflate(R.layout.flow_rank_item2, null);
                    holder.flow_data2 = (TextView)convertView.findViewById(R.id.textViewData2);
                }
                holder.img = (ImageView)convertView.findViewById(R.id.icon);
                holder.tv = (TextView)convertView.findViewById(R.id.title);
                holder.flow_data = (TextView)convertView.findViewById(R.id.textViewData);
                holder.flow_wifi = (TextView)convertView.findViewById(R.id.textViewWifi);
                holder.flow_sum = (TextView)convertView.findViewById(R.id.sumusage);
                convertView.setTag(holder);
            } else {
                holder = (ViewHolder)convertView.getTag();
            }

            holder.img.setImageDrawable(mApps.get(position).icon);
            holder.tv.setText(mApps.get(position).label);
            if(mDualSimRelease) {
                holder.flow_data.setText(String.format(getResources().getString(R.string.data_flow_data1),
                        Formatter.formatFileSize(NetworkRankActivity.this, mApps.get(position).usageSim1, false)));
            } else {
                holder.flow_data.setText(String.format(getResources().getString(R.string.data_flow_data),
                        Formatter.formatFileSize(NetworkRankActivity.this, mApps.get(position).usageSim1, false)));
            }

            holder.flow_wifi.setText(String.format(getResources().getString(R.string.data_flow_wifi),
                    Formatter.formatFileSize(NetworkRankActivity.this, mApps.get(position).usageWifi, false)));
            holder.flow_sum.setText(String.format(getResources().getString(R.string.data_flow_sum),
                    Formatter.formatFileSize(NetworkRankActivity.this, mApps.get(position).usageSum, false)));
            if (mDualSimRelease) {
                holder.flow_data2.setText(String.format(getResources().getString(R.string.data_flow_data2),
                        Formatter.formatFileSize(NetworkRankActivity.this, mApps.get(position).usageSim2, false)));
            }

            Log.e(TAG, "getView end=================");

            return convertView;
        }
    }

    private void enableTestMode() {
        mDualSimRelease = false;
        for (int i = 0; i < 5; i++) {
            UidDetail dt = new UidDetail();
            dt.label = "applllllllllllllllllllllll000" + i;
            dt.usageSim1 = 100 + i * 10;
            dt.usageSim2 = 1000;
            dt.usageWifi = 1000 + i * 1000;
            dt.icon = getResources().getDrawable(R.drawable.ic_launcher);
            mApps.add(dt);
        }

    }


}



