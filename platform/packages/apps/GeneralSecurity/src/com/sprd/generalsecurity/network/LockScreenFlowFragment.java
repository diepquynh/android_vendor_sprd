package com.sprd.generalsecurity.network;

import android.app.AppGlobals;
import android.app.Fragment;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.net.TrafficStats;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.UserManager;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import com.sprd.generalsecurity.network.ScreenStateReceiver.AppUsageInfo;
import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.Formatter;
import com.sprd.generalsecurity.utils.TeleUtils;
import java.text.Collator;
import java.util.ArrayList;
import java.util.Collections;


public class LockScreenFlowFragment extends Fragment{

    private static final String TAG = "LockScreenFlowFragment";
    private Context mContext;

    private int WIFI_ID =0;
    private int SIM1_ID =1;
    private int SIM2_ID =2;

    private TextView mSim1FlowTotalText;
    private TextView mSim2FlowTotalText;
    private TextView mWifiFlowTotalText;

    private long mWifiUsageTotal;
    private long mSim1UsageTotal;
    private long mSim2UsageTotal;

    private int mDataCycleSim1;
    private int mDataCycleSim2;
    private int mDataCycleWIFI;

    private ArrayList<UidDetail> mApps;

    private AppRankAdapter mAdapter;
    private ListView mListView;

    private boolean mDualSim = false;
    private int mNetworkType;

    private View mRootView;

    private static final int SIM1 = 0;
    private static final int SIM2 = 1;
    private static final int WIFI = 2;

    public LockScreenFlowFragment(int networkType) {
        mNetworkType = networkType;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mContext = getActivity();
        if (TeleUtils.getSimCount(mContext) == 0 || TeleUtils.getSimCount(mContext) == 1) {
            mDualSim = false;
        } else {
            mDualSim = true;
        }

        if (mDualSim) {
            mRootView = inflater.inflate(R.layout.app_flow_rank_dual, container, false);
        } else {
            if (mNetworkType != WIFI) {//not WIFI, then set network type as sim1
                mNetworkType = SIM1;
            }
            mRootView = inflater.inflate(R.layout.app_flow_rank, container, false);
        }
        return mRootView;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        mApps = new ArrayList<UidDetail>();
    }

    @Override
    public void onResume() {
        super.onResume();
        initViews();
        initStat();

        buildRankDatas();
        updateUsageTotalData();
        Collections.sort(mApps);
        mAdapter.notifyDataSetChanged();
    }

    @Override
    public void onDetach() {
        super.onDetach();
        ScreenStateReceiver.clearDataLists();
    }

    private void initStat() {
        mApps.clear();
        mSim1UsageTotal = 0;
        mWifiUsageTotal = 0;
        mSim2UsageTotal = 0;
        Log.e(TAG, "data clear");
    }

    private void initViews(){
        //Wifi total usage
        mWifiFlowTotalText = (TextView) mRootView.findViewById(R.id.flowWifi);
        mWifiFlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_wifi),
                Formatter.formatFileSize(mContext, mWifiUsageTotal, false)));

        //Sim1 usage total
        mSim1FlowTotalText = (TextView) mRootView.findViewById(R.id.flowSim1);
        if (mDualSim) {
            mSim1FlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_data1),
                    Formatter.formatFileSize(mContext, mSim1UsageTotal, false)));

            mSim2FlowTotalText = (TextView) mRootView.findViewById(R.id.flowSim2);
            mSim2FlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_data2),
                    Formatter.formatFileSize(mContext, mSim2UsageTotal, false)));
        } else {
            mSim1FlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_data),
                    Formatter.formatFileSize(mContext, mSim1UsageTotal, false)));
        }

        mListView = (ListView) mRootView.findViewById(android.R.id.list);
        mAdapter = new AppRankAdapter(mContext);
        mListView.setAdapter(mAdapter);
    }

    private void updateUsageTotalData() {
        mWifiFlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_wifi),
                Formatter.formatFileSize(mContext, mWifiUsageTotal, false)));

        if (mDualSim) {
            mSim1FlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_data1),
                    Formatter.formatFileSize(mContext, mSim1UsageTotal, false)));

            mSim2FlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_data2),
                    Formatter.formatFileSize(mContext, 0, false)));
        } else {
            mSim1FlowTotalText.setText(String.format(getResources().getString(R.string.data_flow_data),
                    Formatter.formatFileSize(mContext, mSim1UsageTotal, false)));
        }
    }

    class UidDetail implements Comparable<UidDetail>{
        CharSequence label;
        CharSequence contentDescription;
        CharSequence[] detailLabels;
        CharSequence[] detailContentDescriptions;
        Drawable icon;
        long usageWifi;
        long usageSim1;
        long usageSim2;
        long usageSum;

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
        final PackageManager pm = mContext.getPackageManager();

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

        final UserManager um = (UserManager) mContext.getSystemService(Context.USER_SERVICE);

        // otherwise fall back to using packagemanager labels
        final String[] packageNames = pm.getPackagesForUid(uid);
        final int length = packageNames != null ? packageNames.length : 0;
        try {
            final int userId = UserHandle.getUserId(uid);
            UserHandle userHandle = new UserHandle(userId);

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
            Log.e(TAG, "Error while building UI detail for uid "+uid, e);
        } catch (RemoteException e) {
            Log.e(TAG, "Error while building UI detail for uid "+uid, e);
        }

        if (TextUtils.isEmpty(detail.label)) {
            detail.label = Integer.toString(uid);
        }

        return detail;
    }

    private void buildRankDatas() {
        mApps.clear();

        ArrayList<AppUsageInfo> listData = ScreenStateReceiver.mReportList;
        for (AppUsageInfo info : listData) {
            UidDetail dt = buildUidDetail(info.uid);
            switch(mNetworkType) {
                case SIM1:
                    dt.usageSim1 = info.bytesUsed;
                    mSim1UsageTotal += dt.usageSim1;
                    break;
                case SIM2:
                    dt.usageSim2 = info.bytesUsed;
                    mSim2UsageTotal += dt.usageSim2;
                    break;
                case WIFI:
                    dt.usageWifi = info.bytesUsed;
                    mWifiUsageTotal += dt.usageWifi;
                    break;
            }
            dt.usageSum = dt.usageSim1 + dt.usageSim2 + dt.usageWifi;
            mApps.add(dt);
        }
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
            ViewHolder holder = new ViewHolder();
            if (convertView == null) {
                if (!mDualSim) {
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
            if(mDualSim) {
                holder.flow_data.setText(String.format(getResources().getString(R.string.data_flow_data1),
                        Formatter.formatFileSize(mContext, mApps.get(position).usageSim1, false)));
            } else {
                holder.flow_data.setText(String.format(getResources().getString(R.string.data_flow_data),
                        Formatter.formatFileSize(mContext, mApps.get(position).usageSim1, false)));
            }

            holder.flow_wifi.setText(String.format(getResources().getString(R.string.data_flow_wifi),
                    Formatter.formatFileSize(mContext, mApps.get(position).usageWifi, false)));
            holder.flow_sum.setText(String.format(getResources().getString(R.string.data_flow_sum),
                    Formatter.formatFileSize(mContext, mApps.get(position).usageSum, false)));
            if (mDualSim) {
                holder.flow_data2.setText(String.format(getResources().getString(R.string.data_flow_data2),
                        Formatter.formatFileSize(mContext, mApps.get(position).usageSim2, false)));
            }

            return convertView;
        }
    }
}
