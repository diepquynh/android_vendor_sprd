package com.sprd.generalsecurity.network;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.database.Cursor;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Process;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.android.internal.telephony.IGeneralSecureManager;
import com.sprd.generalsecurity.data.AppInfoTable;
import com.sprd.generalsecurity.data.BlockStateProvider;
import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.TeleUtils;

import java.lang.Comparable;
import java.lang.Override;
import java.lang.Runnable;
import java.lang.Thread;
import java.text.Collator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;


public class DataFlowManagement extends Activity implements View.OnClickListener {
    
    private static String TAG = "DataFlowManagement";

    View mRootView;
    View mListContainer;
    private AppAdapter mAdapter;
    ArrayList<AppItem> mApps;
    private ListView mListView;
    HashMap<Integer, Integer> mAppInfoMap = new HashMap<>();

    private CheckBox mCheckSim1All;
    private CheckBox mCheckSim2All;
    private CheckBox mCheckWIFIAll;

    private boolean mOperatingWIFIAll;
    private boolean mOperatingSim1All;
    private boolean mOperatingSim2All;
    private boolean mDualSimRelease = false;

    private TelephonyManager mTelephonyManager;
    private SubscriptionManager mSubscriptionManager;
    private int mSimCount = 0;

    private static int NO_ITEM_INFO = -1;// no app info in DB

//    private static boolean AllAppsSim1Disabled = false;

//    private int mPrimaryCard;

    private static boolean NetworkEnabled = true;
    private static IGeneralSecureManager mGeneralSecureManager = null;

    /**
     * @ param packageUid: uid of package
     * @ param networkType:
     * 0 — TYPE_WIFI
     * 1 — TYPE_SIM1_MOBILE_DATA
     * 2 — TYPE_SIM2_MOBILE_DATA
     *
     * @ param allowed:
     * true — allowed on the networkType
     * false — not allowed on the networkType
     */
    static void setPackageBlockState(int packageUid, int networkType, boolean allowed){
        Log.e(TAG, "setPackageBlockState uid=" + packageUid + ":type=" + networkType  + ":allowed=" + allowed);
        if (NetworkEnabled) {
            if (mGeneralSecureManager ==  null) {
                Log.d(TAG, "mGeneralSecureManager is null");
            } else {
                try {
                    mGeneralSecureManager.setPackageBlockState(packageUid, networkType, allowed);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    static void deleteBlockPackage(int packageUid){
        Log.e(TAG, "deleteBlockPackage uid=" + packageUid);
        if (NetworkEnabled) {
            if (mGeneralSecureManager ==  null) {
                Log.d(TAG, "mGeneralSecureManager is null");
            } else {
                try {
                    mGeneralSecureManager.deleteBlockPackage(packageUid);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    /*
    *@param packageName
    *@param blockState:
    *  0—block all
    * 00000001—wifi only (即第0个比特表示是否禁用wifi)
    * 00000010—SIM1 mobileData only
    * 00000100—SIM2 mobileData only
    * 00000111 — no block(值为7)
    *
    */
    private static final int DISABLE_WIFI = 0x06;
    private static final int DISABLE_SIM1 = 0x05;
    private static final int DISABLE_SIM2 = 0x03;

    private static final int ENABLE_WIFI = 0x01;
    private static final int ENABLE_SIM1 = 0x02;
    private static final int ENABLE_SIM2 = 0x04;
    private static final int NO_BLOCK= 0x07;

    private static final String CLAUSE_SIM1_ENABLE_QUERY = "blockstate=2 or blockstate=3 " +
            "or blockstate=6 or blockstate=7";

    private static final String CLAUSE_SIM2_ENABLE_QUERY = "blockstate=4 or blockstate=5 " +
            "or blockstate=6 or blockstate=7";

    private static final String CLAUSE_WIFI_ENABLE_QUERY = "blockstate=1 or blockstate=3 " +
            "or blockstate=5 or blockstate=7";

    String[] QUERY_PRJECTION = {AppInfoTable.COLUMN_PKG_NAME, AppInfoTable.COLUMN_UID, AppInfoTable.COLUMN_BLOCK_STATE};
    private static int INDEX_LABEL = 0;
    private static int INDEX_UID = 1;
    private static int INDEX_STATE = 2;

    private static Uri APPINFO_URI = BlockStateProvider.CONTENT_URI;
    private static String UID_SELECTION_CLAUSE = AppInfoTable.COLUMN_UID + "=?";

    private enum NetworkType {
        SIM1, SIM2, WIFI
    }

    private static int NetworkTypeWIFI = 0;
    private static int NetworkTypeSIM1 = 1;
    private static int NetworkTypeSIM2 = 2;

    private int mPrimaryCard;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setHomeButtonEnabled(true);

        mTelephonyManager = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
        mSimCount = TeleUtils.getSimCount(this);

        //
       // enableTestMode();
        //

        if (mSimCount == 0 || mSimCount == 1) {
            mDualSimRelease = false;
            setContentView(R.layout.manage_app);
        } else {
            mDualSimRelease = true;
            setContentView(R.layout.manage_app_dual_sim);
        }

        mPrimaryCard = TeleUtils.getPrimaryCard(this);
        Log.e(TAG, "oncreate: dual" + mDualSimRelease);
        mListView = (ListView) findViewById(android.R.id.list);
        mCheckSim1All = (CheckBox) findViewById(R.id.checkBoxAllSim1);
        mCheckWIFIAll = (CheckBox) findViewById(R.id.checkBoxAllWIFI);
        if (mDualSimRelease) {
            mCheckSim2All = (CheckBox) findViewById(R.id.checkBoxAllSim2);
            mCheckSim2All.setOnClickListener(this);
        }
        mCheckSim1All.setOnClickListener(this);
        mCheckWIFIAll.setOnClickListener(this);

        //disable the checkbox if no sim card inserted
        if (mSimCount == 0) {
            mCheckSim1All.setEnabled(false);
        }

        if (NetworkEnabled) {
            if (mGeneralSecureManager == null) {
                IBinder netBinder = ServiceManager.getService("generalsecure");
                if (netBinder == null) {
                    Log.d(TAG, "generalsecure fail");
                }
                mGeneralSecureManager = IGeneralSecureManager.Stub.asInterface(netBinder);
                if (mGeneralSecureManager == null) {
                    Log.d(TAG, "mGeneralSecureManager is null");
                }
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        mPrimaryCard = TeleUtils.getPrimaryCard(DataFlowManagement.this);
        reloadAppData();
    }

    private void reloadAppData() {
        AppLoadTask t = new AppLoadTask();
        t.execute();
    }

    private void appLoadFinished() {
        if (mAdapter == null) {
            mAdapter = new AppAdapter(this);
        }

        if (mSimCount == 0) {
            mCheckSim1All.setEnabled(false);
        } else {
            mCheckSim1All.setEnabled(true);
            if (mSimCount == 2) mCheckSim2All.setEnabled(true);
            mCheckWIFIAll.setEnabled(true);
        }

        Collections.sort(mApps);

        Log.e(TAG, "release:" + mDualSimRelease + ":" + mPrimaryCard);
        if (mDualSimRelease) {
            if (queryIfAllAppsChecked(NetworkType.SIM1)) {
                mCheckSim1All.setChecked(true);
                Log.e(TAG, "mCheckSim1All true 1");
            } else {
                mCheckSim1All.setChecked(false);
            }
        } else {
            //single sim
            if (mPrimaryCard == 0) {
                if (queryIfAllAppsChecked(NetworkType.SIM1)) {
                    Log.e(TAG, "mCheckSim1All true 2");
                    mCheckSim1All.setChecked(true);
                } else {
                    mCheckSim1All.setChecked(false);
                }
            } else {
                if (queryIfAllAppsChecked(NetworkType.SIM2)) {
                    mCheckSim1All.setChecked(true);
                } else {
                    mCheckSim1All.setChecked(false);
                }
            }
        }

        if (mDualSimRelease) {
            if (queryIfAllAppsChecked(NetworkType.SIM2)) {
                mCheckSim2All.setChecked(true);
            } else {
                mCheckSim2All.setChecked(false);
            }
        }

        if (queryIfAllAppsChecked(NetworkType.WIFI)) {
            mCheckWIFIAll.setChecked(true);
        } else {
            mCheckWIFIAll.setChecked(false);
        }
        mListView.setAdapter(mAdapter);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void onClick(final View v) {
        if (v == mCheckSim1All) {
            if (mSimCount == 2) mCheckSim2All.setEnabled(false);
            mCheckWIFIAll.setEnabled(false);
            if (mOperatingSim1All) {
                Log.e(TAG, "blocking all sim1, rtn");
                return;
            }
            mOperatingSim1All = true;
            promptUserOperation();
            Thread t = new Thread(new Runnable() {
                @Override
                public void run() {
                    Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND);

                    int networkType = NetworkTypeSIM1;
                    if (!mDualSimRelease) {
                        networkType = mPrimaryCard + 1;//TeleUtils.getPrimaryCard(DataFlowManagement.this) + 1;
                    }
                    Log.e(TAG, "mCheckSim1All networktype:" + networkType);

                    if (((CheckBox) v).isChecked()) {
                        if (networkType == NetworkTypeSIM1) {
                            unBlockAllAppSim1Data();
                        } else {
                            unBlockAllAppSim2Data();
                        }
                        reloadAppData();
                    } else {
                        if (networkType == NetworkTypeSIM1) {
                            blockAllAppSim1Data();
                        } else {
                            blockAllAppSim2Data();
                        }
                        reloadAppData();
                    }
                    mOperatingSim1All = false;
                }
            });
            t.start();

            return;
        } else if (v == mCheckSim2All) {
            mCheckSim1All.setEnabled(false);
            mCheckWIFIAll.setEnabled(false);
            if (mOperatingSim2All) {
                Log.e(TAG, "blocking all sim2, rtn");
                return;
            }
            mOperatingSim2All = true;
            promptUserOperation();
            Thread t = new Thread(new Runnable() {
                @Override
                public void run() {
                    Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND);

                    if (((CheckBox) v).isChecked()) {
                        unBlockAllAppSim2Data();
                        reloadAppData();
                    } else {
                        blockAllAppSim2Data();
                        reloadAppData();
                    }
                    mOperatingSim2All = false;
                }
            });
            t.start();

            return;
        } else if (v == mCheckWIFIAll) {
            mCheckSim1All.setEnabled(false);
            if (mSimCount == 2) mCheckSim2All.setEnabled(false);
            if (mOperatingWIFIAll) {
                Log.e(TAG, "blocking all wifi, rtn");
                return;
            }
            mOperatingWIFIAll = true;
            promptUserOperation();
            Log.e(TAG, "blocking all wifi===================" + mOperatingWIFIAll);
            Thread t = new Thread(new Runnable() {
                @Override
                public void run() {
                    Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND);
                    if (((CheckBox) v).isChecked()) {
                        unBlockAllAppWIFI();
                        reloadAppData();
                    } else {
                        blockAllAppWIFI();
                        reloadAppData();
                    }
                    mOperatingWIFIAll = false;
                    Log.e(TAG, "blocking all wifi~~~~~~~~~~~~~" + mOperatingWIFIAll);
                }
            });
            t.start();

            return;
        }

        AppItem it = (AppItem) v.getTag();

        if (v.getId() == R.id.sim1) {
            int networkType = NetworkTypeSIM1;
            if (!mDualSimRelease) {
                networkType = mPrimaryCard + 1;//TeleUtils.getPrimaryCard(this) + 1;
            }
            Log.e(TAG, "networktype:" + networkType);
            Log.e(TAG, "onclick sim1:" + ((CheckBox) v).isChecked() + ":" + it.label + ":" + it.uid);

            //1. get current state
            int state = queryAppItemState(it.uid);
            if (((CheckBox) v).isChecked()) {
                setPackageBlockState(it.uid, networkType, true);
                if (!mDualSimRelease) {
                    //single card case, need to check if current sim is sim1 or sim2
                    if (mPrimaryCard == 0) {
                        updateAppInfoItem(it.uid, state | ENABLE_SIM1);
                        if (queryIfAllAppsChecked(NetworkType.SIM1)) {
                            mCheckSim1All.setChecked(true);
                        }
                    } else {
                        //sim2 is primary card
                        updateAppInfoItem(it.uid, state | ENABLE_SIM2);
                        if (queryIfAllAppsChecked(NetworkType.SIM2)) {
                            mCheckSim1All.setChecked(true);
                        }
                    }
                } else {
                    updateAppInfoItem(it.uid, state | ENABLE_SIM1);
                    if (queryIfAllAppsChecked(NetworkType.SIM1)) {
                        mCheckSim1All.setChecked(true);
                    }
                }
            } else {
                mCheckSim1All.setChecked(false);
                //disable sim1
                setPackageBlockState(it.uid, networkType, false);
                //item exists in DB, update it.
                if (!mDualSimRelease) {
                    //single card case, need to check if current sim is sim1 or sim2
                    if (mPrimaryCard == 0) {
                        updateAppInfoItem(it.uid, state & DISABLE_SIM1);
                    } else {
                        updateAppInfoItem(it.uid, state & DISABLE_SIM2);
                    }
                } else {
                    updateAppInfoItem(it.uid, state & DISABLE_SIM1);
                }
            }

        } else if (v.getId() == R.id.sim2) {
            Log.e(TAG, "onclick sim2:" + ((CheckBox) v).isChecked() + ":" + it.label + ":" + it.uid);
            int state = queryAppItemState(it.uid);
            if (((CheckBox) v).isChecked()) {
                setPackageBlockState(it.uid, 2, true);
                updateAppInfoItem(it.uid, state | ENABLE_SIM2);

                //check SIM1-all button state
                if (queryIfAllAppsChecked(NetworkType.SIM2)) {
                    mCheckSim2All.setChecked(true);
                }
            } else {
                mCheckSim2All.setChecked(false);
                //enable sim1
                setPackageBlockState(it.uid, 2, false);
                //item exists in DB, update it.
                updateAppInfoItem(it.uid, state & DISABLE_SIM2);
            }
        } else if (v.getId() == R.id.wifi) {
            Log.e(TAG, "onclick wifi:" + ((CheckBox) v).isChecked() + ":" + it.label + ":" + it.uid);
            int state = queryAppItemState(it.uid);
            if (((CheckBox) v).isChecked()) {
                //enable WIFI
                setPackageBlockState(it.uid, 0, true);
                updateAppInfoItem(it.uid, state | ENABLE_WIFI);

                //check WIFI-all button state
                if (queryIfAllAppsChecked(NetworkType.WIFI)) {
                    mCheckWIFIAll.setChecked(true);
                }
            } else {
                mCheckWIFIAll.setChecked(false);
                //disable WIFI
                setPackageBlockState(it.uid, 0, false);
                //item exists in DB, update it.
                updateAppInfoItem(it.uid, state & DISABLE_WIFI);
            }
        }
    }

    private void promptUserOperation() {
        String prompt = getResources().getString(R.string.operating_wait);
        Toast toast = Toast.makeText(this, prompt, Toast.LENGTH_SHORT);
        toast.show();
    }

    void insertAppInfoItem(AppItem item, int state) {
//        Log.e(TAG, "block sim---db---" + ((CheckBox) v).isChecked() + ":" + it.uid);
        ContentValues values = new ContentValues();
        values.put(AppInfoTable.COLUMN_UID, item.uid);
        values.put(AppInfoTable.COLUMN_PKG_NAME, item.label);
        values.put(AppInfoTable.COLUMN_BLOCK_STATE, state);
        getContentResolver().insert(BlockStateProvider.CONTENT_URI, values);
    }

    private int updateAppInfoItem(int uid, int state) {
        mAppInfoMap.put(uid, state);

        String[] selArgs = {""};
        selArgs[0] = Integer.toString(uid);
        ContentValues values = new ContentValues();
        values.put(AppInfoTable.COLUMN_BLOCK_STATE, state);
        Log.e(TAG, "up state=" + state);
        return getContentResolver().update(BlockStateProvider.CONTENT_URI, values, UID_SELECTION_CLAUSE, selArgs);
    }

    private void blockAllAppSim1Data(){
        if (mApps != null) {
            for (AppItem it : mApps) {
                int state = queryAppItemState(it.uid);
                try {
                    setPackageBlockState(it.uid, NetworkTypeSIM1, false);
                    Thread.sleep(50);
                } catch (Exception e) {

                }
                updateAppInfoItem(it.uid, state & DISABLE_SIM1);
            }
        }
    }

    private void blockAllAppSim2Data() {
        if (mApps != null) {
            for (AppItem it : mApps) {
                int state = queryAppItemState(it.uid);
                try {
                    setPackageBlockState(it.uid, NetworkTypeSIM2, false);
                    Thread.sleep(50);
                } catch (Exception e) {

                }
                updateAppInfoItem(it.uid, state & DISABLE_SIM2);
            }
        }
    }

    private void unBlockAllAppSim1Data() {
        if (mApps != null) {
            for (AppItem it : mApps) {
                int state = queryAppItemState(it.uid);
                updateAppInfoItem(it.uid, state | ENABLE_SIM1);
                try {
                    setPackageBlockState(it.uid, NetworkTypeSIM1, true); //enable sim1
                    Thread.sleep(50);
                } catch (Exception e) {

                }
            }
        }
    }

    private void unBlockAllAppSim2Data() {
        if (mApps != null) {
            for (AppItem it : mApps) {
                int state = queryAppItemState(it.uid);
                try {
                    setPackageBlockState(it.uid, NetworkTypeSIM2, true);
                    Thread.sleep(50);
                } catch (Exception e) {

                }
                updateAppInfoItem(it.uid, state | ENABLE_SIM2);
            }
        }
    }

    private synchronized void blockAllAppWIFI(){
        if (mApps != null) {
            for (AppItem it : mApps) {
                int state = queryAppItemState(it.uid);
                try {
                    setPackageBlockState(it.uid, NetworkTypeWIFI, false);
                    Thread.sleep(50);
                } catch (Exception e) {

                }

                updateAppInfoItem(it.uid, state & DISABLE_WIFI);
            }
        }
    }

    private synchronized void unBlockAllAppWIFI() {
        if (mApps != null) {
            for (AppItem it : mApps) {
                try {
                    setPackageBlockState(it.uid, NetworkTypeWIFI, true);
                    Thread.sleep(50);
                } catch (Exception e) {

                }
                int state = queryAppItemState(it.uid);
                updateAppInfoItem(it.uid, state | ENABLE_WIFI);
            }
        }
    }

    //TODO:
    //all button, not in list
    //cache appinfo, cache unchecked item.

    private int queryAppItemState(int uid) {
        String[] selArgs = {""};
        selArgs[0] = Integer.toString(uid);
        Cursor cursor = getContentResolver().query(BlockStateProvider.CONTENT_URI, QUERY_PRJECTION, "uid=?", selArgs, null);
        try {
            if (cursor == null || cursor.getCount() == 0) {
                return NO_ITEM_INFO;
            } else {
                cursor.moveToFirst();
                return cursor.getInt(INDEX_STATE);
            }
        } finally {
            cursor.close();
        }
    }

    private void queryAllAppState() {
        Cursor cursor = getContentResolver().query(BlockStateProvider.CONTENT_URI, QUERY_PRJECTION, null, null, null);
        try {
            if (cursor == null || cursor.getCount() == 0) {
                //DB init, No info in DB, so all apps are enabled
                Log.e(TAG, "no item found");
                //inset app info as enabled
                for (AppItem item: mApps) {
                    insertAppInfoItem(item, NO_BLOCK);
                    mAppInfoMap.put(item.uid, NO_BLOCK);
                }
                return;
            } else {
                while (cursor.moveToNext()){
                    mAppInfoMap.put(cursor.getInt(INDEX_UID), cursor.getInt(INDEX_STATE));
                }
            }

//            Iterator it = mAppInfoMap.entrySet().iterator();
//            while(it.hasNext()) {
//                Map.Entry<Integer, Integer> entry = (Map.Entry<Integer, Integer>)it.next();
//                Log.e(TAG, "map got:" + entry.getKey() + ":" + entry.getValue());
//            }
        } finally {
            cursor.close();
        }
    }


    private boolean queryIfAllAppsChecked(NetworkType type) {
        Cursor cursor = null;
        cursor = getContentResolver().query(BlockStateProvider.CONTENT_URI, QUERY_PRJECTION, null, null, null);

        switch (type){
            case SIM1:
                cursor = getContentResolver().query(BlockStateProvider.CONTENT_URI, QUERY_PRJECTION, CLAUSE_SIM1_ENABLE_QUERY, null, null);
                break;
            case SIM2:
                cursor = getContentResolver().query(BlockStateProvider.CONTENT_URI, QUERY_PRJECTION, CLAUSE_SIM2_ENABLE_QUERY, null, null);
                break;
            case WIFI:
                cursor = getContentResolver().query(BlockStateProvider.CONTENT_URI, QUERY_PRJECTION, CLAUSE_WIFI_ENABLE_QUERY, null, null);
                break;
            default:
                break;
        }

        try {
            if (cursor != null) {
                Log.e(TAG, "cccount=" + cursor.getCount() + ":" + mApps.size());
//                while(cursor.moveToNext()) {
//                    Log.e(TAG, "DB:" + cursor.getString(INDEX_LABEL) + ":" + cursor.getInt(INDEX_STATE));
//                }
//                Log.e(TAG, "=====================");
//                for (AppItem item: mApps) {
//                    Log.e(TAG, "item:" + item.label + ":" + mAppInfoMap.get(item.uid));
//                }
                if (cursor.getCount() == mApps.size()) {
                    return true;
                }
            }
            return false;
        } finally {
            cursor.close();
        }
    }


    static private void deleteAppInfoItem(Context context, int uid) {
        String[] selArgs = {""};
        selArgs[0] = Integer.toString(uid);
        context.getContentResolver().delete(BlockStateProvider.CONTENT_URI, UID_SELECTION_CLAUSE, selArgs);
    }

    private void storeUidNetworkInfo(int uid, int blockState) {

    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        onBackPressed();
        return true;
    }

    /**
     * Query apps which have internet permissions.
     * New installed apps will be updated to DB.
     */
    class AppLoadTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected void onPreExecute() {
            super.onPreExecute();
        }

        @Override
        protected Void doInBackground(Void... params) {
            // mAppInfoMap.clear();
            mApps = DataFlowManagement.this.getLauncherApp();//getAppIdsOfInternet();
            Log.e(TAG, "got size:" + mApps.size());
            queryAllAppState();
            for(AppItem item : mApps) {
                //add new installed app into DB and map
                if (mAppInfoMap.get(item.uid) == null) {
                    Log.e(TAG, "no item:" + item.uid + ":" + item.label);
                    insertAppInfoItem(item, NO_BLOCK);
                    mAppInfoMap.put(item.uid, NO_BLOCK);
                }
            }
            return null;
        }

        @Override
        protected void onPostExecute(Void aVoid) {
            super.onPostExecute(aVoid);
            appLoadFinished();
        }
    }

    class AppAdapter extends BaseAdapter {
        private LayoutInflater mInflater;

        public AppAdapter(Context context) {
            this.mInflater = LayoutInflater.from(context);
        }

        @Override
        public int getCount() {
            return mApps.size();
        }

        @Override
        public Object getItem(int position) {
            return null;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        class ViewHolder {
            ImageView img;
            TextView tv;
            CheckBox checkBoxSim1;
            CheckBox checkBoxSim2;
            CheckBox checkBoxWifi;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            ViewHolder holder = new ViewHolder();
            if (convertView == null) {
                if (!mDualSimRelease) {
                    convertView = mInflater.inflate(R.layout.app_item, null);
                } else {
                    convertView = mInflater.inflate(R.layout.app_item_dual_sim, null);
                    holder.checkBoxSim2 = (CheckBox)convertView.findViewById(R.id.sim2);
                }
                holder.img = (ImageView)convertView.findViewById(R.id.icon);
                holder.tv = (TextView)convertView.findViewById(R.id.title);
                holder.checkBoxSim1 = (CheckBox)convertView.findViewById(R.id.sim1);
                holder.checkBoxWifi = (CheckBox)convertView.findViewById(R.id.wifi);
                convertView.setTag(holder);
            } else {
                holder = (ViewHolder)convertView.getTag();
            }

            holder.img.setVisibility(View.VISIBLE);
            holder.img.setImageDrawable(mApps.get(position).icon);
            holder.tv.setText(mApps.get(position).label);
            holder.checkBoxSim1.setOnClickListener(DataFlowManagement.this);
            holder.checkBoxSim1.setTag(mApps.get(position));
            holder.checkBoxWifi.setOnClickListener(DataFlowManagement.this);
            holder.checkBoxWifi.setTag(mApps.get(position));
            if (holder.checkBoxSim2 != null) {
                holder.checkBoxSim2.setTag(mApps.get(position));
                holder.checkBoxSim2.setOnClickListener(DataFlowManagement.this);
            }

            //disable the checkbox if no sim card inserted
            if (mSimCount == 0) {
                holder.checkBoxSim1.setEnabled(false);
            }

            //set network checkbox state
            if (mAppInfoMap.get(mApps.get(position).uid) != null) {
                //checkbox all state
                if (mCheckWIFIAll.isChecked()) {
                    holder.checkBoxWifi.setChecked(true);
                } else {
                    if ((ENABLE_WIFI & mAppInfoMap.get(mApps.get(position).uid)) == ENABLE_WIFI) {
                        Log.e(TAG, "wifi check:" + mApps.get(position).label);
                        holder.checkBoxWifi.setChecked(true);
                    } else {
                        Log.e(TAG, "wifi uncheck:" + mApps.get(position).label);
                        holder.checkBoxWifi.setChecked(false);
                    }
                }

                if (mCheckSim1All.isChecked()) {
                    //sim1 all checkbox checked, set checkbox directly.
                    Log.e(TAG, "mCheckSim1All checked");
                    holder.checkBoxSim1.setChecked(true);
                } else {
                    if (!mDualSimRelease) {
                        //single card inserted
                        if (mPrimaryCard == 0) {
                            if ((ENABLE_SIM1 & mAppInfoMap.get(mApps.get(position).uid)) == ENABLE_SIM1) {
                                Log.e(TAG, "sim1 check");
                                holder.checkBoxSim1.setChecked(true);
                            } else {
                                Log.e(TAG, "sim1 UNcheck");

                                holder.checkBoxSim1.setChecked(false);
                            }
                        } else {
                            if ((ENABLE_SIM2 & mAppInfoMap.get(mApps.get(position).uid)) == ENABLE_SIM2) {
                                Log.e(TAG, "sim1 check");
                                holder.checkBoxSim1.setChecked(true);
                            } else {
                                Log.e(TAG, "sim1 UNcheck");

                                holder.checkBoxSim1.setChecked(false);
                            }
                        }
                    } else {
                        if ((ENABLE_SIM1 & mAppInfoMap.get(mApps.get(position).uid)) == ENABLE_SIM1) {
                            Log.e(TAG, "sim1 check");
                            holder.checkBoxSim1.setChecked(true);
                        } else {
                            Log.e(TAG, "sim1 UNcheck");

                            holder.checkBoxSim1.setChecked(false);
                        }
                    }
                }

                if (holder.checkBoxSim2 != null) {
                    if (mCheckSim2All.isChecked()) {
                        holder.checkBoxSim2.setChecked(true);
                    } else {
                        if ((ENABLE_SIM2 & mAppInfoMap.get(mApps.get(position).uid)) == ENABLE_SIM2) {
                            holder.checkBoxSim2.setChecked(true);
                        } else {
                            holder.checkBoxSim2.setChecked(false);
                        }
                    }
                }
            } else {
                holder.checkBoxSim1.setChecked(false);
                holder.checkBoxWifi.setChecked(false);
                if (holder.checkBoxSim2 != null) {
                    holder.checkBoxSim2.setChecked(false);
                }
            }
            Log.e(TAG, "uid=" + mApps.get(position).uid + ":" + mAppInfoMap.get(mApps.get(position).uid) + ":" + (position) + ":" + mApps.get(position).label);

            return convertView;
        }
    }

    class AppItem implements Comparable<AppItem> {
        public String label;
        public int uid;
        public Drawable icon;

        private final Collator sCollator = Collator.getInstance();

        @Override
        public int compareTo(AppItem another) {
            return sCollator.compare(label, another.label);
        }
    }

    private ArrayList<AppItem> getLauncherApp() {
        PackageManager pm = getPackageManager();
        Intent it = new Intent(Intent.ACTION_MAIN, null);
        it.addCategory(Intent.CATEGORY_LAUNCHER);
        List<ResolveInfo> infos = pm.queryIntentActivities(it, 0);
        Log.e(TAG, "size============" + infos.size());
        ApplicationInfo applicationInfo;
        ArrayList<AppItem> appsGot = new ArrayList<>();
        HashSet<String> appList = new HashSet<>();

        String title;
        int uid;
        for(ResolveInfo info: infos){
            PackageInfo pkgInfo = null;
            try {
                pkgInfo = pm.getPackageInfo(info.activityInfo.packageName, PackageManager.GET_PERMISSIONS);
            } catch (PackageManager.NameNotFoundException e) {
                e.printStackTrace();
            }
            String[] requestedPermissions = pkgInfo.requestedPermissions;
            if (requestedPermissions == null) {
                continue;
            }

            //Filter out apps requested INTERNET permission.
            for(String per : requestedPermissions) {
                if (TextUtils.equals(per, android.Manifest.permission.INTERNET)) {
                    try {
                        appList.add(info.activityInfo.packageName);
                        break;
                    } catch (Exception e) {
                        Log.e(TAG, "error:" + e);
                    }
                }
            }
        }

        //contrsuct result list
        for(String pkgName : appList) {
            Log.e(TAG, "------------------------||" + pkgName);
            AppItem item = new AppItem();
            try {
                applicationInfo = pm.getApplicationInfo(pkgName, 0);
                item.label = (String) ((applicationInfo != null) ? applicationInfo.loadLabel(pm) : "???");
                item.uid = pm.getPackageUid(pkgName, UserHandle.myUserId());
                item.icon = applicationInfo.loadIcon(pm);
                appsGot.add(item);
            }catch (PackageManager.NameNotFoundException e){
                e.printStackTrace();
            }
        }

        return appsGot;
    }

    public static class AppPackageListener extends BroadcastReceiver {
        public AppPackageListener() {
            super();
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            Log.e(TAG, "onRec:" + intent);
            if (intent == null || intent.getAction() == null) {
                return;
            }
            debugIntent(context, intent);
            if (intent.getAction().equalsIgnoreCase(
                    Intent.ACTION_PACKAGE_REMOVED)) {
                deleteAppInfoItem(context,
                        (int) intent.getExtras().get(Intent.EXTRA_UID));
                deleteBlockPackage((int) intent.getExtras().get(
                        Intent.EXTRA_UID));
            } else if (intent.getAction().equalsIgnoreCase(
                    Intent.ACTION_PACKAGE_ADDED)) {
                // do nothing
            }
        }

        private void debugIntent(Context context, Intent intent) {
            Bundle extras = intent.getExtras();
            if (extras != null) {
                for (String key: extras.keySet()) {
                    Log.e(TAG, "key[" + key + "]:" + extras.get(key) );
                }
            } else {
                Log.e(TAG, "no extras");
            }
        }
    }

    private void enableTestMode() {
        mSimCount = 2;
    }
}

