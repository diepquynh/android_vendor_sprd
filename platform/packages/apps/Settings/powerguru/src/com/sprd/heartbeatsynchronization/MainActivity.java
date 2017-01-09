
package com.sprd.heartbeatsynchronization;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Vector;
import java.util.concurrent.ExecutionException;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.app.PowerGuru;
import android.content.pm.ResolveInfo;
import android.content.res.Configuration;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.RelativeLayout;
import android.view.ViewGroup.LayoutParams;
import android.view.Gravity;
import android.os.RemoteException;

public class MainActivity extends Activity {

    private final String TAG = "HEARTBEATSYNC";
    private ListView mListView = null;
    private Context mContext;
    private PackageManager mPackageManager;
    private PowerGuru mPowerGuru ;
    private List<ResolveInfo> mResolveInfoListToDisp;
    // private List<OneApp> mAppListToDisp;
    private Vector<OneApp> mAppListToDisp;
    private List<String> mWhiteCandidateList;
    private List<String> mWhiteList;
    private MyOnClickListener mMyOnClickListener;
    private RelativeLayout mRelativeLayout;
    private TextView mNoApkDisp;
    private RelativeLayout.LayoutParams mLayoutParams;
    private static Intent mMainIntent; // used to get all the apps in phone
    private static LaunchCountComparator mLaunchCountComparator;
    private static IntentFilter mCandicateFilter; // candicatewhitelist changed filter

    private static final int GET_CANDIDATE_AND_WHITE_LIST = 10;
    private static final int ADD_WHITE_APP_FROM_LIST = 11;
    private static final int DEL_WHITE_APP_FROM_LIST = 12;
    private static final int OK = 100;
    private static final int FAILURE = 99;

    //private AddOrRemovePackagesReceiver mAddOrRemovePackagesReceiver;
    private CandicateWhiteListChangedReceiver mCandicateWhiteListChangedReceiver;

    static {
        mMainIntent = new Intent(Intent.ACTION_MAIN, null);
        mMainIntent.addCategory(Intent.CATEGORY_LAUNCHER);
        mLaunchCountComparator = new LaunchCountComparator();

        mCandicateFilter = new IntentFilter();
        mCandicateFilter.addAction(PowerGuru.ACTION_POWERGURU_DATA_CHANGED);

    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate()...");
        super.onCreate(savedInstanceState);
        NodeInfoHelper.createInstance(this);
        setContentView(R.layout.main_layout);
        setTitle(R.string.heartbeat_synchronization);

        mContext = this;
        mPackageManager = getPackageManager();
        mPowerGuru = (PowerGuru) getSystemService(Context.POWERGURU_SERVICE);
        //mAddOrRemovePackagesReceiver = new AddOrRemovePackagesReceiver();
        mCandicateWhiteListChangedReceiver = new CandicateWhiteListChangedReceiver();
        //registerReceiver(mAddOrRemovePackagesReceiver, filter);
        registerReceiver(mCandicateWhiteListChangedReceiver,mCandicateFilter);

        mListView = (ListView) findViewById(R.id.softlist);
        //mCheckedChangeListener = new MyOnCheckedChangeListener();
        mMyOnClickListener = new MyOnClickListener();
        // mAppListToDisp = new ArrayList<OneApp>();
        mAppListToDisp = new Vector<OneApp>();
    }

    protected void onResume() {
        Log.d(TAG, "onResume()");
        super.onResume();
        getDataAndRefresh();
    }


    public void getDataAndRefresh(){
        Log.d(TAG, "getDataAndRefresh()");
        GetAppListTask getAppListTask = new GetAppListTask(GET_CANDIDATE_AND_WHITE_LIST);
        getAppListTask.execute();
    }

    class GetAppListTask extends AsyncTask<Void, Integer, Integer> {
        private int type;
        private String pkgName = null;
        private CompoundButton button;
        private boolean result = false;

        GetAppListTask(int type) {
            this.type = type;
        }

        GetAppListTask(int type, String pkgName,CompoundButton button) {
            this.type = type;
            this.pkgName = pkgName;
            this.button = button;
        }

        // run in UI thread，run before doInBackground()
        @Override
        protected void onPreExecute() {
            Log.d(TAG, "onPreExecute()");
            Log.d(TAG, "type is : "+type);
            if (GET_CANDIDATE_AND_WHITE_LIST == type) {
            } else if (ADD_WHITE_APP_FROM_LIST == type) {
                button.setEnabled(false);
            } else if (DEL_WHITE_APP_FROM_LIST == type) {
                button.setEnabled(false);
            } else {
                Log.e(TAG, "error:type is not correct");
            }
            if(null == mWhiteCandidateList || mWhiteCandidateList.size() == 0){
                    Log.d(TAG, "mWhiteCandidateList is empty");
                    if(mLayoutParams == null){
                         mLayoutParams = new RelativeLayout.LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT);
                         mLayoutParams.addRule(RelativeLayout.BELOW, R.id.textView);
                     }
                     if(mNoApkDisp == null){
                          mNoApkDisp = new TextView(MainActivity.this);
                          mNoApkDisp.setText(MainActivity.this.getString(R.string.no_app_installed_text));
                          mNoApkDisp.setGravity(Gravity.CENTER);
                          mNoApkDisp.setLayoutParams(mLayoutParams);
                          mNoApkDisp.setId(1);
                     }
                     if(mRelativeLayout == null){
                         mRelativeLayout = (RelativeLayout) findViewById(R.id.main_layout);
                         mRelativeLayout.addView(mNoApkDisp, mLayoutParams);
                     }
                     mNoApkDisp.setVisibility(View.VISIBLE);
             }else{
                   Log.d(TAG, "mWhiteCandidateList is not empty");
                    mNoApkDisp.setVisibility(View.GONE);
             }
        }

        // run in background，can not run in ui thread , can perform time-consuming method
        @Override
        protected Integer doInBackground(Void... arg0) {
            Log.d(TAG, "doInBackground(Void... arg0)");

            if (GET_CANDIDATE_AND_WHITE_LIST == type) {
                getAppList();
                NodeInfoHelper.getInstance().updateInfo();
                // integrate the collected data
                mAppListToDisp.clear();
                for (int i = 0; i < mResolveInfoListToDisp.size(); i++) {
                    OneApp oneApp = new OneApp();
                    oneApp.resolveInfo = mResolveInfoListToDisp.get(i);
                    oneApp.count = 0;
                    NodeInfoHelper.getInstance().attachInfo(oneApp);
                    mAppListToDisp.add(oneApp);
                }
                return OK;
            } else if (ADD_WHITE_APP_FROM_LIST == type) {
                result = addWhiteAppfromList(pkgName);
                if (result) {
                    mWhiteList.add(pkgName);
                    mAppListToDisp.clear();
                    for (int i = 0; i < mResolveInfoListToDisp.size(); i++) {
                        OneApp oneApp = new OneApp();
                        oneApp.resolveInfo = mResolveInfoListToDisp.get(i);
                        oneApp.count = 0;
                        NodeInfoHelper.getInstance().attachInfo(oneApp);
                        mAppListToDisp.add(oneApp);
                    }
                    return OK;
                }else{
                    Log.e(TAG, "add WhiteAppfromList failed!! " + pkgName);
                }
            } else if (DEL_WHITE_APP_FROM_LIST == type) {
                result = delWhiteAppfromList(pkgName);
                if (result) {
                    mWhiteList.remove(pkgName);
                    mAppListToDisp.clear();
                    for (int i = 0; i < mResolveInfoListToDisp.size(); i++) {
                        OneApp oneApp = new OneApp();
                        oneApp.resolveInfo = mResolveInfoListToDisp.get(i);
                        oneApp.count = 0;
                        NodeInfoHelper.getInstance().attachInfo(oneApp);
                        mAppListToDisp.add(oneApp);
                    }
                    return OK;
                }else if (!result){
                    Log.e(TAG, "del WhiteAppfromList failed!! " + pkgName);
                }
            } else {
                Log.e(TAG, "error:type is not correct");
            }
            return FAILURE;
        }

        // run in UI thread，run after doInBackground()
        @Override
        protected void onPostExecute(Integer integer) {
            Log.d(TAG, "onPostExecute(Integer integer)");
            if (GET_CANDIDATE_AND_WHITE_LIST == type) {
                mListView.setAdapter(new HeartbeatAdapter(mContext, mAppListToDisp));
                // order by package name
                // Collections.sort(mAllApps, new ResolveInfo.DisplayNameComparator(mPackageManager));
                // order by app launch time
                Collections.sort(mAppListToDisp, mLaunchCountComparator);
            } else if (ADD_WHITE_APP_FROM_LIST == type) {
                button.setEnabled(true);
                if(integer == FAILURE){
                    Log.d(TAG, button.getText()+"ADD_WHITE_APP_FROM_LIST FAILURE");
                    //button.setSelected(!button.isSelected());
                    Switch switcher = (Switch)button;
                    switcher.setChecked(!switcher.isChecked());
                }
                Collections.sort(mAppListToDisp, mLaunchCountComparator);
            } else if (DEL_WHITE_APP_FROM_LIST == type) {
                button.setEnabled(true);
                if(integer == FAILURE){
                    Log.d(TAG, button.getText()+"DEL_WHITE_APP_FROM_LIST FAILURE");
                    //button.setSelected(!button.isSelected());
                    Switch switcher = (Switch)button;
                    switcher.setChecked(!switcher.isChecked());
                }
                Collections.sort(mAppListToDisp, mLaunchCountComparator);
            } else {
                Log.e(TAG, "error:type is not correct");
            }
            if(mAppListToDisp.size()>0){
               mNoApkDisp.setVisibility(View.GONE);
            }else{
               mNoApkDisp.setVisibility(View.VISIBLE);
            }
        }

        // run when publishProgress() is run ，publishProgress() is used to update progress
        @Override
        protected void onProgressUpdate(Integer... values) {
            Log.d(TAG, "onProgressUpdate(Integer... values)");
        }
    }

    // Check the system applications, added to the list of applications.
    private void getAppList() {
        Log.d(TAG, "getAppList()");
        // get all the apps of the phone (just visible ones)
        List<ResolveInfo> allApps = mPackageManager.queryIntentActivities(mMainIntent, 0);
        mResolveInfoListToDisp = new ArrayList<ResolveInfo>();

        mWhiteCandidateList = getWhiteCandicateList();
        mWhiteList = getWhiteList();

        if (null == allApps || null == mWhiteCandidateList || null == mWhiteList) {
            Log.e(TAG, "errer : mAllApps or mWhiteCandidateList or mWhiteList is null");
            return;
        }

        for (int i = 0; i < mWhiteCandidateList.size(); i++) {
            for (int j = 0; j < allApps.size(); j++) {
                if (mWhiteCandidateList.get(i).equals(allApps.get(j).activityInfo.packageName)) {
                    mResolveInfoListToDisp.add(allApps.get(j));
                    continue;
                }
            }
        }
        if (mResolveInfoListToDisp.size() != mWhiteCandidateList.size()) {
            Log.d(TAG,"not equals: " + mResolveInfoListToDisp.size() + " " + mWhiteCandidateList.size());
        }
        for(int i=0;i<mWhiteCandidateList.size();i++){
            Log.d(TAG, "mWhiteCandidateList::: "+mWhiteCandidateList.get(i));
        }
        for(int i=0;i<mWhiteList.size();i++){
            Log.d(TAG, "mWhiteList::: "+mWhiteList.get(i));
        }
    }

    class HeartbeatAdapter extends BaseAdapter {
        private Context context;
        private List<OneApp> appListToDisplay;
        private OneApp oneApp;
        private LayoutInflater infater = null;

        public HeartbeatAdapter(Context context, List<OneApp> appListToDisplay) {
            Log.d(TAG, "HeartbeatAdapter()");
            this.context = context;
            this.appListToDisplay = appListToDisplay;
            infater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        }

        @Override
        public int getCount() {
            return appListToDisplay.size();
        }

        @Override
        public Object getItem(int arg0) {
            return arg0;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            //Log.d(TAG, "getView()");

            ViewHolder holder = null;
            if (convertView == null || convertView.getTag() == null) {
                //Log.d(TAG, "new Holder");
                convertView = infater.inflate(R.layout.soft_row, null);
                holder = new ViewHolder(convertView);
                convertView.setTag(holder);
            }
            else {
                //Log.d(TAG, "holder exist");
                holder = (ViewHolder) convertView.getTag();
            }
            // get app pkg name , app name , app icon
            oneApp = appListToDisplay.get(position);
            holder.appIcon.setImageDrawable(oneApp.resolveInfo.loadIcon(mPackageManager));
            holder.tvAppLabel.setText(oneApp.resolveInfo.loadLabel(mPackageManager).toString());
            // holder.tvPkgName.setText(res.activityInfo.packageName);
            holder.tvPkgName.setText(getResources().getString(R.string.app_launch_count)
                    + oneApp.getCount());
            // holder.switcher.setOnCheckedChangeListener(mCheckedChangeListener);
            holder.switcher.setOnClickListener(mMyOnClickListener);
            if (mWhiteList.contains(oneApp.resolveInfo.activityInfo.packageName)) {
                holder.switcher.setChecked(true);
            } else {
                holder.switcher.setChecked(false);
            }
            holder.switcher.setTag(oneApp.resolveInfo.activityInfo.packageName);

            return convertView;
        }
    }

    class ViewHolder {
        ImageView appIcon;
        TextView tvAppLabel;
        TextView tvPkgName;
        Switch switcher;

        public ViewHolder(View view) {
            this.appIcon = (ImageView) view.findViewById(R.id.img);
            this.tvAppLabel = (TextView) view.findViewById(R.id.name);
            this.tvPkgName = (TextView) view.findViewById(R.id.desc);
            this.switcher = (Switch) view.findViewById(R.id.switcher);
        }
    }

    public class CandicateWhiteListChangedReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "onReceive("+context.toString()+", "+intent.toString()+")");
            getDataAndRefresh();
        }
    }

    class MyOnClickListener implements OnClickListener {
        @Override
        public void onClick(View view) {
            Switch switcher = (Switch)view;

            Log.d(TAG, "onClick()"+switcher.getTag()+" : "+switcher.isChecked());
            boolean result;
            String pkgName = switcher.getTag().toString();
            // button.setClickable(false);
            if (switcher.isChecked()) {
                Log.d(TAG, "set to "+switcher.isChecked());
                GetAppListTask getAppListTask = new GetAppListTask(ADD_WHITE_APP_FROM_LIST, pkgName,switcher);
                getAppListTask.execute();
            } else if (!switcher.isChecked()) {
                Log.d(TAG, "set to "+switcher.isChecked());
                GetAppListTask getAppListTask = new GetAppListTask(DEL_WHITE_APP_FROM_LIST, pkgName,switcher);
                getAppListTask.execute();
            }
            Log.d(TAG, switcher.getTag().toString() + " set to checked ? " + switcher.isChecked());
        }
    }

    public class OneApp {
        public ResolveInfo resolveInfo;
        public int count;

        public OneApp() {
        }

        public OneApp(ResolveInfo resolveInfo, int count) {
            super();
            this.resolveInfo = resolveInfo;
            this.count = count;
        }

        public ResolveInfo getResolveInfo() {
            return resolveInfo;
        }

        public void setResolveInfo(ResolveInfo resolveInfo) {
            this.resolveInfo = resolveInfo;
        }

        public int getCount() {
            return count;
        }

        public void setCount(int count) {
            this.count = count;
        }

    }

    public static class LaunchCountComparator implements Comparator<OneApp> {
        public final int compare(OneApp a, OneApp b) {
            // return by descending order
            return b.count - a.count;
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        Log.d(TAG, "onConfigurationChanged(Configuration newConfig)");
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy()");
        //unregisterReceiver(mAddOrRemovePackagesReceiver);
        unregisterReceiver(mCandicateWhiteListChangedReceiver);
        super.onDestroy();
    }

    public List<String> getWhiteCandicateList() {
        Log.d(TAG, "getWhiteCandicateList()");
        List<String> result = mPowerGuru.getWhiteCandicateList();
        //del duplecate
        for (int i = 0; i < result.size() - 1; i++) {
            for (int j = result.size() - 1; j > i; j--) {
                if (result.get(j).equals(result.get(i))) {
                    result.remove(j);
                }
            }
        }
        Log.d(TAG, "whiteCandicateList::: "+result.toString());
        return result;
    }

    public List<String> getWhiteList() {
        Log.d(TAG, "getWhiteList()");
        List<String> result = mPowerGuru.getWhiteList();
        // del duplecate
        for (int i = 0; i < result.size() - 1; i++) {
            for (int j = result.size() - 1; j > i; j--) {
                if (result.get(j).equals(result.get(i))) {
                    result.remove(j);
                }
            }
        }
        Log.d(TAG, "whiteList::: " + result.toString());
        return result;
    }

    public synchronized boolean addWhiteAppfromList(String appName) {
        Log.d(TAG, "add " + appName + " to whitelist");
        boolean result = mPowerGuru.addWhiteAppfromList(appName);
        if(!result){
            for(int i=0;i<5;i++){
                Log.d(TAG, "addWhiteAppfromList cout: "+i);
                result = mPowerGuru.addWhiteAppfromList(appName);
                if(result){
                    return result;
                }
            }
        }
        return result;
    }

    public synchronized boolean delWhiteAppfromList(String appName) {
        Log.d(TAG, "remove " + appName + " from whitelist");
        boolean result = mPowerGuru.delWhiteAppfromList(appName);
        if(!result){
            for(int i=0;i<5;i++){
                Log.d(TAG, "delWhiteAppfromList cout: "+i);
                result = mPowerGuru.delWhiteAppfromList(appName);
                if(result){
                    return result;
                }
            }
        }
        return result;
    }
}
