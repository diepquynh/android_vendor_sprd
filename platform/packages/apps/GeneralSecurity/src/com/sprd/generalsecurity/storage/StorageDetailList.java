package com.sprd.generalsecurity.storage;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import android.app.ActionBar.LayoutParams;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.IPackageStatsObserver;
import android.content.pm.IPackageDataObserver;
import android.content.pm.PackageStats;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.EnvironmentEx;
import android.os.Handler;
import android.os.Message;
import android.os.UserHandle;
import android.os.storage.StorageManager;
import android.os.storage.StorageEventListener;
import android.os.storage.VolumeInfo;
import android.util.ArrayMap;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.ApplicationsState;
import com.sprd.generalsecurity.utils.ApplicationsState.AppEntry;
import com.sprd.generalsecurity.utils.Formatter;

public class StorageDetailList extends Activity implements OnItemClickListener,
        OnClickListener, ApplicationsState.Callbacks {

    private static final String TAG = "StorageDetailList";
    private static final int REQUEST_SIZE = 1;
    private static final int DIALOG_STAND_TIME = 200;
    private ArrayList<String> mList;
    private ArrayList<Long> mSizeList;
    private ArrayMap<Integer, Boolean> mCheckedMap;

    private Context mContext;
    private PackageManager mPm;
    private int mType;
    private long mTotalSize;
    private TextView mTotalSizeText;
    private CheckBox mAllCheckBox;
    private DetailAdapter mAdapter;
    private ListView mListView;
    private MenuItem mCleanMenu;
    private boolean mIsExternal;

    private StorageManager mStorageManager;
    private ProgressDialog mWorkingProgress;
    private ArrayMap<String,Long> mMap;

    private static final String SDCARD_PREFIX = "sdcard";

    private final int mNameIds[] = { R.string.cache_file,
            R.string.rubbish_file, R.string.invalid_file, R.string.temp_file, R.string.large_file };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mContext = this;
        mPm = mContext.getPackageManager();

        setContentView(R.layout.storage_detail);

        mListView = (ListView) findViewById(R.id.detailList);
        mTotalSizeText = (TextView) findViewById(R.id.total_size);
        mAllCheckBox = (CheckBox) findViewById(R.id.all_check);

        mAllCheckBox.setOnClickListener(this);

        Log.i(TAG, "savedInstanceState : " + savedInstanceState);
        Intent intent = getIntent();
        Bundle bundle = intent.getBundleExtra(StorageManagement.BUNDLE_NAME);
        if (bundle != null) {
            Log.i(TAG, "bundle !=null");
            mIsExternal = bundle
                    .getBoolean(StorageManagement.IS_EXTERNAL, false);
            mType = bundle.getInt(StorageManagement.BUNDLE_KEY);
            Log.i(TAG, "mType:"+mType);
            this.setTitle(mContext.getText(mNameIds[mType]));
            mList = new ArrayList<String>();
            mSizeList = new ArrayList<Long>();
            mMap = DataGroup.getInstance().getNeedMap(mIsExternal, mType);
            Log.i(TAG, "mMap:"+mMap);
            if (mMap == null ) {
                this.finish();
                return;
            }
            if (mMap.size() == 0) {
                this.onBackPressed();
            }
            mList.clear();
            mSizeList.clear();
            for (int i= 0;i < mMap.size();i++) {
                mList.add(mMap.keyAt(i));
                mSizeList.add(mMap.get(mMap.keyAt(i)));
            }
            Log.i(TAG, "   key:"+mList);
            Log.i(TAG, " value:"+mSizeList);

            Log.i(TAG, mSizeList + ":" + mSizeList.size());
            mAdapter = new DetailAdapter(mContext);
            mListView.setAdapter(mAdapter);
            mListView.setOnItemClickListener(this);
        }

        mStorageManager = this.getSystemService(StorageManager.class);
        mStorageManager.registerListener(mStorageListener);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setHomeButtonEnabled(true);
        if (mType == StorageManagement.CACHE_ITEM && !mIsExternal) {
            init();
        }
    }

    private class DetailAdapter extends BaseAdapter {
        class ViewHolder {
            ImageView icon;
            TextView name;
            TextView version;
            TextView state;
            TextView size;
            CheckBox check;
        }

        private LayoutInflater mInflater;

        public DetailAdapter(Context context) {
            this.mInflater = LayoutInflater.from(context);
            mCheckedMap = new ArrayMap<Integer, Boolean>();
            initChecked();
        }

        private void initChecked() {
            for (int i = 0; i < mList.size(); i++) {
                mCheckedMap.put(i, false);
            }
        }

        @Override
        public int getCount() {
            Log.i(TAG, "mList.size()>" + mList.size());
            return mList.size();
        }

        @Override
        public Object getItem(int position) {
            return null;
        }

        public long getItemId(int position) {
            Log.i(TAG, "getItemId>" + position);
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            Log.i(TAG, "getView  >position:" + position);

            Log.i(TAG, "mList:" + mList.size() + "  position:" + position);
            Log.i(TAG, "mSizeList:" + mSizeList.size() + "  position:"
                    + position);
            Log.i(TAG, "mCheckedMap:" + mCheckedMap.size() + "  position:"
                    + position);
            ViewHolder holder = new ViewHolder();
            if (convertView == null) {
                convertView = mInflater.inflate(R.layout.storage_detail_item,
                        null);
                holder.icon = (ImageView) convertView.findViewById(R.id.icon);
                holder.name = (TextView) convertView.findViewById(R.id.name);
                holder.version = (TextView) convertView
                        .findViewById(R.id.version);
                holder.state = (TextView) convertView.findViewById(R.id.state);
                holder.size = (TextView) convertView.findViewById(R.id.size);
                holder.check = (CheckBox) convertView
                        .findViewById(R.id.checkbox);
                convertView.setTag(holder);
            } else {
                holder = (ViewHolder) convertView.getTag();
            }
            File file = new File(mList.get(position));
            if (mType == StorageManagement.APK_ITEM) {

                holder.icon.setVisibility(View.VISIBLE);
                holder.version.setVisibility(View.VISIBLE);
                holder.state.setVisibility(View.VISIBLE);

                final AppStorageInfo asi = new AppStorageInfo();
                getInstalled(asi, mList.get(position));
                holder.icon.setBackground(asi.icon);
                holder.name.setText(asi.name);
                holder.version.setText(asi.version);
                holder.state.setText(asi.state);

            } else if (mType != StorageManagement.CACHE_ITEM) {
                holder.state.setVisibility(View.VISIBLE);
                holder.state.setTextSize(getResources().getDimension(
                        R.dimen.path_size));
                holder.state.setSingleLine(false);
                holder.name.setText(file.getName());
                holder.state.setText(file.getAbsolutePath());
            } else if (mType == StorageManagement.CACHE_ITEM) {
                holder.icon.setVisibility(View.VISIBLE);
                holder.version.setVisibility(View.VISIBLE);
                holder.state.setVisibility(View.VISIBLE);

                final AppStorageInfo asi = new AppStorageInfo();
                getInstalled(asi, mList.get(position));
                holder.icon.setBackground(asi.icon);
                holder.name.setText(asi.name);
                holder.version.setText(asi.version);
                holder.state.setText(asi.state);
            }

            holder.size.setText(mSizeList != null ? Formatter.formatFileSize(
                    mContext, mSizeList.get(position), false) : "");
            Log.i(TAG, "mCheckedMap:" + mCheckedMap + "   position:"
                    + position);
            holder.check.setChecked(mCheckedMap.get(position));
            refreshTotalSize();
            Log.i(TAG, "convertView:" + convertView);
            return convertView;
        }

        @Override
        public void notifyDataSetChanged() {
            refreshTotalSize();
            super.notifyDataSetChanged();
        }

    }

    class AppStorageInfo {
        long size;
        String name;
        String version;
        String state;
        Drawable icon;
    }

    private long cacheSize;

    private void getFileSize(File dir) {
        File[] fileList = dir.listFiles();
        if (dir.listFiles() != null) {
            for (File file : dir.listFiles()) {
                if (file.isDirectory()) {
                    getFileSize(file);
                } else {
                    cacheSize += file.length();
                    Log.e(TAG,
                            "-------file:" + file + " : " + file.isDirectory()
                                    + " : " + file.length());
                }
            }
        }
    }

    private boolean getInstalled(AppStorageInfo info, String path) {
        Log.i(TAG, "path=" + path);
        File file = new File(path);

        if (mType == StorageManagement.CACHE_ITEM) {
            PackageInfo installedInfo;
            try {
                installedInfo = mPm.getPackageInfo(file.getName(),
                        UserHandle.myUserId());

                info.name = installedInfo.applicationInfo.loadLabel(mPm)
                        .toString();
                info.state = mContext.getResources().getString(
                        R.string.installed);
                info.version = installedInfo.versionName;
                info.icon = installedInfo.applicationInfo.loadIcon(mPm);

                return true;
            } catch (Exception e) {
                e.printStackTrace();
                info.name = file.getName();
                info.state = mContext.getResources().getString(
                        R.string.unInstalled);
                info.icon = mPm.getDefaultActivityIcon();
                return false;
            }
        }
        PackageInfo apkInfo = mPm.getPackageArchiveInfo(path,
                PackageManager.GET_ACTIVITIES);
        Log.i(TAG, "apkInfo=" + apkInfo);

        if (apkInfo != null) {
            info.version = apkInfo.versionName;
            PackageInfo installedInfo;
            Log.i(TAG, "apkInfo Name=" + apkInfo.packageName);
            info.name = apkInfo.applicationInfo.loadLabel(mPm).toString();
            try {
                installedInfo = mPm.getPackageInfo(apkInfo.packageName,
                        UserHandle.myUserId());
                Log.i(TAG, "installedInfo=" + installedInfo);
                info.name = installedInfo.applicationInfo.loadLabel(mPm)
                        .toString();
                info.icon = installedInfo.applicationInfo.loadIcon(mPm);
                if (apkInfo.versionName == installedInfo.versionName) {
                    Log.i(TAG, "installed");
                    info.state = mContext.getResources().getString(
                            R.string.installed);
                    return true;
                }
            } catch (Exception e) {
                // e.printStackTrace();
                info.icon = mPm.getDefaultActivityIcon();
                info.state = mContext.getResources().getString(
                        R.string.unInstalled);
                return false;
            }
            info.state = mContext.getResources().getString(R.string.installed);
            return false;
        }
        info.name = file.getName();
        info.icon = mPm.getDefaultActivityIcon();
        info.state = mContext.getResources().getString(R.string.invalid_apk);
        return false;
    }

    private void getFileList(List<String> list, int type) {
        if (type == StorageManagement.CACHE_ITEM) {
            Log.i(TAG, "type:cache");
        } else if (type == StorageManagement.RUBBISH_ITEM) {
            Log.i(TAG, "type:rubbish");
        } else if (type == StorageManagement.TMP_ITEM) {
            Log.i(TAG, "type:tmp");
        } else if (type == StorageManagement.APK_ITEM) {
            Log.i(TAG, "type:apk");
        }
        for (String path : list) {
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View v, int position, long id) {

        mAdapter.notifyDataSetChanged();
        Log.i(TAG, "onItemClick" + "  position:" + position + "  \tid:" + id);
        ListView l = (ListView) parent;
        CheckBox checkbox = (CheckBox) v.findViewById(R.id.checkbox);
        Log.i(TAG, Formatter.formatFileSize(mContext, mTotalSize, false));

        mCheckedMap.put(position, !checkbox.isChecked());
        if (checkbox.isChecked()) {
            if (!isAllChecked()) {
                mAllCheckBox.setChecked(false);
            }
            checkbox.setChecked(false);
        } else {
            checkbox.setChecked(true);
            if (isAllChecked()) {
                mAllCheckBox.setChecked(true);
            }
        }
        refreshTotalSize();
    }

    private void refreshTotalSize() {
        boolean menuCanClick = false;

        //SPRD 562147: set the menu unclick when no file be selected
        mTotalSize = 0;
        Log.i(TAG, "mCheckedMap:"+mCheckedMap);
        for (int i = 0; i < mCheckedMap.size(); i++) {
            if (mCheckedMap.get(i)) {
                mTotalSize += getPosSize(i);

                //SPRD 562147: set the menu unclick when no file be selected
                menuCanClick = true;
            }
        }
        //SPRD 562147: set the menu unclick when no file be selected
        if (mCleanMenu != null) {
            mCleanMenu.setEnabled(menuCanClick);
        }

        Log.i(TAG, "mTotalSize:"+mTotalSize);
        mTotalSizeText.setText(mTotalSize != 0 ? Formatter.formatFileSize(
                mContext, mTotalSize, false) : "");
    }

    private long getPosSize(int position) {
        return mSizeList.get(position);
    }

    private boolean isAllChecked() {
        for (int i = 0; i < mCheckedMap.size(); i++) {
            if (mCheckedMap.get(i) == false) {
                return false;
            }
            continue;
        }
        return true;
    }

    @Override
    public void onClick(View view) {

        Log.i(TAG, "onClick~~" + " view:" + ((CheckBox) view).isChecked()
                + "   checkbox:" + mAllCheckBox.isChecked());
        mAdapter.notifyDataSetChanged();
        for (int i = 0; i < mAdapter.getCount(); i++) {
            mCheckedMap.put(i, mAllCheckBox.isChecked());
        }

        refreshTotalSize();
        Log.i(TAG, "" + mCheckedMap.values());
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.storage_menu, menu);
        mCleanMenu = menu.findItem(R.id.action_clean);

        //SPRD 562147: set the menu unclick when no file be selected
        if (mCleanMenu != null) {
            mCleanMenu.setEnabled(false);
        }
        return true;
    }

    ApplicationsState mState;
    AppEntry mAppEntry;
    protected ApplicationsState.Session mSession;

    void init() {
        mState = ApplicationsState.getInstance(getApplication());
        mSession = mState.newSession(this);
        mSession.resume();
    }

    @Override
    public boolean onMenuItemSelected(int featureId, MenuItem item) {
        if (mIsExternal && mType == StorageManagement.RUBBISH_ITEM && ActivityManager.isUserAMonkey()) {
            return true;
        }
        if (mCleanMenu.getItemId() == item.getItemId()) {
            FileDeleteTask deleteTask = new FileDeleteTask();
            deleteTask.execute();
            return true;
        } else {
            return super.onMenuItemSelected(featureId, item);
        }
    }

    private boolean dirDelete(File file) {
        if (file.isDirectory()) {
            File[] files = file.listFiles();
            if (files != null) {
                for (File f : files) {
                    if (!dirDelete(f)) {
                        return false;
                    }
                }
            }
        }
        return file.delete();
    }

    ClearCacheObserver clearCacheObserver;

    @Override
    public void onRunningStateChanged(boolean running) {
        Log.e(TAG, "onRunningStateChanged");
    }

    @Override
    public void onPackageListChanged() {

        Log.e(TAG, "onPackageListChanged");
    }

    @Override
    public void onRebuildComplete(ArrayList<AppEntry> apps) {

        Log.e(TAG, "onRebuildComplete");
    }

    @Override
    public void onPackageIconChanged() {

        Log.e(TAG, "onPackageIconChanged");
    }

    @Override
    public void onPackageSizeChanged(String packageName) {
        if (mSizeList == null | !isChange)
            return;
        Log.e(TAG, "onPackageSizeChanged    " + packageName);
        if (mState == null) {
            mState = ApplicationsState.getInstance(getApplication());
            mSession = mState.newSession(this);
            mSession.resume();
            // init();
        }

        for (int i = 0; i < mCheckedMap.size();) {
            Log.i(TAG, " i:" + i);
            Log.i(TAG, "before mCheckedMap:" + mCheckedMap + "\n\t\tmList:"
                    + mList + "\n\t\tmSizeList" + mSizeList);
            if (mCheckedMap.get(i)) {
                mAppEntry = mState.getEntry(packageName, UserHandle.myUserId());
                Log.e(TAG, "----------entry:" + mAppEntry);
                Log.e(TAG, "\t\t\t----------cache:" + mAppEntry.cacheSize
                        + "  ex:" + mAppEntry.externalCacheSize);
                long cacheSize = mAppEntry.cacheSize
                        + mAppEntry.externalCacheSize;
                Log.e(TAG, "onPack:" + packageName+"\t cacheSize:"+cacheSize);

                if (cacheSize <= 0
                        || (mAppEntry.cacheSize == 4096 * 3 && mAppEntry.externalCacheSize == 0)) {
                    mTotalSize -= mSizeList.get(i);
                    mMap.remove(mList.get(i));
                    mList.remove(i);
                    mSizeList.remove(i);
                    Log.i(TAG, " remove localPosition:" + i);
                    if (i + 1 < mCheckedMap.size()) {
                        for (int j = i; j + 1 < mCheckedMap.size();) {
                            mCheckedMap.put(j, mCheckedMap.get(j + 1));
                            j++;
                        }
                        mCheckedMap.remove(mCheckedMap.size() - 1);
                        Log.i(TAG, "\t\ti=:" + mTotalSize);
                    } else {
                        mCheckedMap.remove(i);
                    }

                    Log.i(TAG, "after mCheckedMap:" + mCheckedMap
                            + "\n\t\tmList:" + mList + "\n\t\tmSizeList"
                            + mSizeList+"\t mMap:"+mMap);
                    continue;
                } else {
                    mTotalSize -= mSizeList.get(i);
                    mSizeList.set(i, cacheSize);
                    mTotalSize += mSizeList.get(i);
                    mMap.put(mMap.keyAt(i), cacheSize);
                    Log.i(TAG, "localPosition:" + i + " \tcacheSize:"
                            + Formatter.formatFileSize(mContext, cacheSize, false));
                }
            }
            i++;
        }
        mAdapter.notifyDataSetChanged();
        isChange = false;
        if (mList.size() == 0) {
            onBackPressed();
        }
    }

    @Override
    public void onAllSizesComputed() {

        Log.e(TAG, "onAllSizesComputed");
    }

    @Override
    public void onLauncherInfoChanged() {

        Log.e(TAG, "onLauncherInfoChanged");
    }

    @Override
    public void onLoadEntriesCompleted() {

        Log.e(TAG, "onLoadEntriesCompleted");
    }

    class ClearCacheObserver extends IPackageDataObserver.Stub {
        public void onRemoveCompleted(final String packageName,
                final boolean succeeded) {
            Log.i(TAG, "clear cache>" + packageName + "  :" + succeeded);
            if (!succeeded) {
                Log.i(TAG, "删除缓存失败");
            }
            final Message msg = mHandler.obtainMessage(REQUEST_SIZE);
            AppEntry appEntry = mState.getEntry(packageName,
                    UserHandle.myUserId());
            Log.i(TAG, "appEntry:  " + appEntry.cacheSize + "   ex:"
                    + appEntry.externalCacheSize);
            msg.obj = packageName;
            mHandler.sendMessage(msg);
        }
    }

    private boolean isChange;
    private final Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case REQUEST_SIZE:
                // Refresh size info
                removeMessages(REQUEST_SIZE);
                Log.i(TAG, "refresh >" + msg.obj.toString());
                String packageName = msg.obj.toString();
                mState.requestSize(packageName, UserHandle.myUserId());
                Log.i(TAG, "refresh end");
                isChange = true;
                break;
            }
        }
    };

    private long getTotalSize() {
        long totalsize = 0;
        if (mSizeList == null) {
            finish();
        } else {
            for (long size : mSizeList) {
                totalsize += size;
            }
        }
        return totalsize;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.i(TAG, " onDestroy()");
        if (mSession != null) {
            mSession.release();
        }
        if (mState != null) {
            if (mState.mThread != null && mState.mThread.getLooper() != null) {
                mState.mThread.getLooper().quit();
            }
            mState = null;
        }
        com.sprd.generalsecurity.utils.ApplicationsState.sInstance = null;
        if (mStorageManager != null && mStorageListener != null) {
            try {
                mStorageManager.unregisterListener(mStorageListener);
            } catch (Exception e) {
                Log.i(TAG, "unregisterListener... exception");
            }
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        Log.i(TAG, "******1**onBackPressed");
        if (item.getItemId() != mCleanMenu.getItemId()) {
            onBackPressed();
        }
        return true;
    }

    @Override
    public void onBackPressed() {
        Log.i(TAG, "******2**onBackPressed");
        /* SPRD: Bug 562138, The title not update when we change the language @{ */
        if (((Activity) mContext).isResumed() && mWorkingProgress != null
                && mWorkingProgress.isShowing()) {
            mWorkingProgress.dismiss();
        }
        /* }@ */
        Intent intent = new Intent();
        intent.putExtra("size", getTotalSize());
        this.setResult(RESULT_OK, intent);
        super.onBackPressed();
    }

    StorageEventListener mStorageListener = new StorageEventListener() {
        @Override
        public void onVolumeStateChanged(VolumeInfo vol, int oldState,
                int newState) {
            Log.i(TAG, "vol:" + vol + "volchange  oldState:" + oldState
                    + "    newState:" + newState);
            if (vol.linkName != null && vol.linkName.startsWith(SDCARD_PREFIX)) {
                if ((oldState == VolumeInfo.STATE_MOUNTED && newState == VolumeInfo.STATE_EJECTING)
                        || (oldState == VolumeInfo.STATE_EJECTING && newState == VolumeInfo.STATE_UNMOUNTED)
                        || (oldState == VolumeInfo.STATE_UNMOUNTED && newState == VolumeInfo.STATE_BAD_REMOVAL)) {
                    Toast.makeText(mContext, mContext.getResources().getString(R.string.sdcard_state_removed),
                        Toast.LENGTH_SHORT).show();
                } else if ((oldState == VolumeInfo.STATE_UNMOUNTED && newState == VolumeInfo.STATE_UNMOUNTED)
                        || (oldState == VolumeInfo.STATE_UNMOUNTED && newState == VolumeInfo.STATE_CHECKING)
                        || (oldState == VolumeInfo.STATE_CHECKING && newState == VolumeInfo.STATE_MOUNTED)) {
                    Toast.makeText(mContext, mContext.getResources().getString(R.string.sdcard_state_inserted)
                        , Toast.LENGTH_SHORT).show();
                }

                finish();
            }
        }
    };

    class FileDeleteTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected void onPreExecute() {
            super.onPreExecute();
            onStart();
            Log.i(TAG, "..onPreExecute...........");
        }

        private void onStart() {
            Log.i(TAG, "..onStart...........");
            if (!mIsExternal && (mType == StorageManagement.CACHE_ITEM)) {
                Log.i(TAG, "..onStart  retrun..");
                return;
            }
            if (((Activity) mContext).isResumed()
                    && mWorkingProgress != null && mWorkingProgress.isShowing()) {
                Log.i("ysz"+TAG,"dissmiss");
                mWorkingProgress.dismiss();
            }
            mWorkingProgress = new ProgressDialog(mContext);
            mWorkingProgress.setCancelable(false);
            mWorkingProgress.setTitle(R.string.clean_storage_menu);
            mWorkingProgress.setMessage(mContext.getResources().getString(
                    R.string.cleaning_wait));
            mWorkingProgress.show();
        }

        @Override
        protected Void doInBackground(Void... arg0) {
            Log.i(TAG, "-----4. file deleteTask --------...........");
            if (!(mType == StorageManagement.CACHE_ITEM)) {
                Log.i(TAG, "if....!CACHE_ITEM.......");
                Log.i(TAG, "mList>>" + mList);
                DataGroup.getInstance().mFileUpdateBits |= DataGroup.LARGE_FILE_BIT;
                for (int i = 0; i < mCheckedMap.size();) {
                    if (mCheckedMap.get(i)) {
                        File file = new File(mList.get(i));
                        long size = mSizeList.get(i);
                        if (file.delete()) {
                            Log.i(TAG, "file deleted:" + mList.get(i));
                            mTotalSize -= size;
                            Log.i(TAG, "mTotalSize:" + mTotalSize);
                            mMap.remove(mList.get(i));

                            if (mList.get(i).endsWith(StorageManagement.RUBBISH_FILE1_EXT) ||
                                mList.get(i).endsWith(StorageManagement.RUBBISH_FILE2_EXT)) {
                                Log.d(TAG, "rubish need update");
                                DataGroup.getInstance().mFileUpdateBits |= DataGroup.RUBBISH_FILE_BIT;
                            } else if (mList.get(i).endsWith(StorageManagement.TMP_FILE_EXT) ||
                                    file.getName().startsWith(StorageManagement.TMP_FILE_PREFIX)){
                                Log.d(TAG, "TMP need update");
                                DataGroup.getInstance().mFileUpdateBits |= DataGroup.TMP_FILE_BIT;
                            } else if (mList.get(i).endsWith(StorageManagement.APK_FILE_EXT)){
                                Log.d(TAG, "APK need update");
                                DataGroup.getInstance().mFileUpdateBits |= DataGroup.APK_FILE_BIT;
                            } else if (StorageManagement.isLargeFileUnique(file)) {
                                if (mIsExternal) {
                                    DataGroup.getInstance().mExUniqueLargeFileSize -= size;
                                    Log.d(TAG, "exsize:" + DataGroup.getInstance().mExUniqueLargeFileSize);
                                } else {
                                    DataGroup.getInstance().mUniqueLargeFileSize -= size;
                                }
                            }

                            mList.remove(i);
                            mSizeList.remove(i);

                            Log.i(TAG, "mList.size:" + mList.size());
                            Log.i(TAG, "mSizeList.size:" + mSizeList.size());
                            if (i + 1 < mCheckedMap.size()) {
                                Log.i(TAG, "remove1");
                                for (int j = i; j + 1 < mCheckedMap.size(); j++) {
                                    mCheckedMap.put(j, mCheckedMap.get(j + 1));
                                    Log.i(TAG, "j..." + j);
                                }
                                mCheckedMap.remove(mCheckedMap.size() - 1);
                                Log.i(TAG, "\t\ti=:" + mTotalSize);
                                continue;
                            } else {
                                Log.i(TAG, "remove2");
                                mCheckedMap.remove(i);
                            }
                            Log.i(TAG, "\tmList size:" + mList.size() + "   "
                                    + mList + "\n\tmCheckedMap size:"
                                    + mCheckedMap.size() + "   " + mCheckedMap);
                        }
                    }

                    i++;
                }
            } else {
                Log.i(TAG, "else........");
                if (mIsExternal) {
                    for (int i = 0; i < mCheckedMap.size();) {
                        if (mCheckedMap.get(i)) {
                            File file = new File(mList.get(i));
                            long size = mSizeList.get(i);
                            if (dirDelete(file)) {
                                // mTotalSize -= size;
                                Log.i(TAG, i + "    file.delete() = true");
                                mTotalSize -= size;
                                Log.i(TAG, "mTotalSize:" + mTotalSize);
                                mMap.remove(mList.get(i));
                                mList.remove(i);
                                mSizeList.remove(i);
                                if (i + 1 < mCheckedMap.size()) {
                                    for (int j = i; j + 1 < mCheckedMap.size(); j++) {
                                        mCheckedMap.put(j,
                                                mCheckedMap.get(j + 1));
                                    }
                                    mCheckedMap.remove(mCheckedMap.size() - 1);
                                    Log.i(TAG, "\t\ti=:" + mTotalSize);
                                    continue;
                                } else {
                                    mCheckedMap.remove(i);
                                }
                                Log.i(TAG,
                                        "\tmList size:" + mList.size() + "   "
                                                + mList + "\n\t\t" + mSizeList
                                                + "\n\tmCheckedMap size:"
                                                + mCheckedMap.size() + "   "
                                                + mCheckedMap);
                            }
                        }
                        i++;
                    }
                } else {
                    Log.i(TAG, "else...CACHE_ITEM........");

                    if (clearCacheObserver == null) {
                        Log.i(TAG, "\t\tnew ClearCacheObserver");
                        clearCacheObserver = new ClearCacheObserver();
                    }
                    for (int i = 0; i < mCheckedMap.size(); i++) {
                        Log.i(TAG, " i:" + i);
                        if (mCheckedMap.get(i)) {
                            File file = new File(mList.get(i));
                            String packageName = file.getName();
                            Log.i(TAG, " i:" + i);
                            Log.i(TAG, i + ": " + packageName);
                            mPm.deleteApplicationCacheFiles(packageName,
                                    clearCacheObserver);
                        }
                    }
                }
            }
            Log.i(TAG, "File bit:" + DataGroup.getInstance().mFileUpdateBits);
            DataGroup.getInstance().updateSize(DataGroup.getInstance().mFileUpdateBits, mIsExternal);
            StorageManagement.notifyMediaScanDir(mContext, mIsExternal ? EnvironmentEx.getExternalStoragePath() :
                        EnvironmentEx.getInternalStoragePath());

            try {
                Thread.sleep(DIALOG_STAND_TIME);
            } catch (InterruptedException e) {
                e.printStackTrace();
                Log.i(TAG, "..Async Sleep exception..");
            }

            return null;
        }

        @Override
        protected void onPostExecute(Void aVoid) {
            super.onPostExecute(aVoid);
            Log.i(TAG, "..onPostExecute...........");
            Log.i(TAG, "mCheckedMap size:" + mCheckedMap.size()
                    + "    mList size:" + mList.size() + "    mSizeList size:"
                    + mSizeList.size());
            Log.i(TAG, "mWorkingProgress:"
                    + mWorkingProgress
                    + " isShowing:"
                    + (mWorkingProgress != null ? mWorkingProgress.isShowing()
                            : false));
            if (((Activity) mContext).isResumed()
                    && mWorkingProgress != null && mWorkingProgress.isShowing()) {
                mWorkingProgress.dismiss();
            }
            mWorkingProgress = null;
            if (mList.size() == 0) {
                Log.i(TAG, "mList.size=0");
                onBackPressed();
            }
            mAdapter.notifyDataSetChanged();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.i(TAG, "onPause");
    }

}
