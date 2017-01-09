package com.sprd.fileexplorer.activities;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.app.ActionBar;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.app.FragmentTransaction;
import android.support.v4.view.ViewPager;
import android.support.v4.view.ViewPager.OnPageChangeListener;
import android.util.Log;
import android.util.Pair;
import android.util.Slog;
import android.util.SparseArray;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.StorageReceiver;
import com.sprd.fileexplorer.adapters.FileListAdapter;
import com.sprd.fileexplorer.fragments.BaseFragment;
import com.sprd.fileexplorer.fragments.DetailedListFragment;
import com.sprd.fileexplorer.fragments.OverViewFragment;
import com.sprd.fileexplorer.util.ActivityUtils;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexplorer.util.IStorageUtil.StorageChangedListener;
import com.sprd.fileexplorer.util.StorageUtil;
import com.sprd.fileexplorer.util.OnPastePathChangedListener;

import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;
import static android.Manifest.permission.READ_EXTERNAL_STORAGE;
import android.content.pm.PackageManager;

public class FileExploreActivity extends FragmentActivity implements OnPastePathChangedListener {

    private static final String TAG = "FileExploreActivity";

    private static final String USB_STORAGE_PATH = "/storage/usbdisk";
    private static final int STORAGE_PERMISSION_REQUEST_CODE = 1;
    private SparseArray<StorageStatus> mStorageStatus = new SparseArray<StorageStatus>();
    private static final int STORAGE_SYZE = 3;
    private BaseFragment mCurrentFragment;
    private ActionBar mActionBar;
    private ArrayAdapter<String> mNavigationAdapter;
    // SPRD: Add for bug617889.
    private AlertDialog mAlertDialog;

    /**
     * The {@link android.support.v4.view.PagerAdapter} that will provide
     * fragments for each of the sections. We use a
     * {@link android.support.v4.app.FragmentPagerAdapter} derivative, which
     * will keep every loaded fragment in memory. If this becomes too memory
     * intensive, it may be best to switch to a
     * {@link android.support.v4.app.FragmentStatePagerAdapter}.
     */
    private SectionsPagerAdapter mPagerAdapter;
    /**
     * The {@link ViewPager} that will host the section contents.
     */
    private ViewPager mViewPager;

    StorageChangedListener mStorageChangedListener = new StorageChangedListener() {

        /* SPRD: Modify for bug509242. @{ */
        @Override
        public void onStorageChanged(final String path, final boolean available, final boolean sdcard) {
            Log.d(TAG, "StorageChanged: path = " + path + " available = " + available);
            new Handler(getMainLooper()).post(new Runnable() {

                @Override
                public void run() {
                    FileExploreActivity.this.onStorageChanged(path, available, sdcard);
                }
            });
        }
        /* @} */
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Exception occurred from support framework, make a workaround
        // currently and have two choices in future:
        // 1. Give up FragmentActivity, using Activity instead.
        // 2. Learn more about FragmentActivity, FragmentManager and
        // FragmentState.
        Log.i(TAG, "start onCreate");
        super.onCreate(null);
        setContentView(R.layout.activity_file_explore);
        /* SPRD: Modify for bug519899, save the latest state with request permission. @{ */
        if (checkSelfPermission(WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[] { WRITE_EXTERNAL_STORAGE }, STORAGE_PERMISSION_REQUEST_CODE);
        } else {
            StorageUtil.addStorageChangeListener(mStorageChangedListener);
        }
        /* @} */
        // SPRD: Add for bug505955.
        FileUtil.addPastePathChangeListener(this);
        init();
        Log.i(TAG, "end onCreate");
    }

    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        Log.i(TAG, "start onRequestPermissionsResult: requestCode = " + requestCode);
        /* SPRD: Add for bug617889. @{ */
        if (mAlertDialog != null && mAlertDialog.isShowing()) {
            mAlertDialog.dismiss();
        }
        /* @} */

        switch (requestCode) {
            case STORAGE_PERMISSION_REQUEST_CODE:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Log.i(TAG, "Allow the permission request.");
                    StorageUtil.addStorageChangeListener(mStorageChangedListener);
                } else {
                    Log.i(TAG, "Deny the permission request.");
                    showConfirmDialog();
                }
                break;
            default:
                break;
        }
    }

    public void showConfirmDialog() {
        Log.d(TAG, "start showConfirmDialog()");
        // SPRD: Modify for bug617889.
        mAlertDialog = new AlertDialog.Builder(this)
                .setTitle(getResources().getString(R.string.toast_fileexplorer_internal_error))
                .setMessage(getResources().getString(R.string.error_permissions))
                .setCancelable(false)
                .setOnKeyListener(new Dialog.OnKeyListener() {
                    @Override
                    public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                        if (keyCode == KeyEvent.KEYCODE_BACK) {
                            finish();
                        }
                        return true;
                    }
                })
                .setPositiveButton(getResources().getString(R.string.dialog_dismiss),
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                finish();
                            }
                        }).show();
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        // TODO Auto-generated method stub
        boolean flag = false;
        try{
            flag = super.dispatchTouchEvent(ev);
        }catch (IllegalArgumentException e) {
            e.printStackTrace();
        }
        return flag;
    }

    private void init() {
        SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(this);
        mPagerAdapter = new SectionsPagerAdapter(getSupportFragmentManager());
        mCurrentFragment = OverViewFragment.getInstance(FileExploreActivity.this);
        mPagerAdapter.addFragment(new Pair<BaseFragment, Integer>(mCurrentFragment, 0));
        List<String> listStr = new ArrayList<String>();
        listStr.add(getString(R.string.title_section_overview));
        
        //init storageStatus
        StorageStatus ss = new StorageStatus();
        ss.path = StorageUtil.getInternalStorage().getAbsolutePath();
        ss.available = StorageUtil.getInternalStorageState();
        ss.title = getString(R.string.title_section_internal);
        ss.tagIndex = 1;
        mStorageStatus.put(DetailedListFragment.FRAGMENT_TYPE_INTERNAL, ss);
        ss = new StorageStatus();
        ss.path = StorageUtil.getExternalStorage().getAbsolutePath();
        ss.available = StorageUtil.getExternalStorageState();
        ss.title = getString(R.string.title_section_external);
        ss.tagIndex = 2;
        mStorageStatus.put(DetailedListFragment.FRAGMENT_TYPE_EXTERNAL, ss);
        ss = new StorageStatus();
        ss.path = (new File("/storage/usbdisk")).getAbsolutePath();
        ss.available = StorageUtil.getUSBStorageState();
        // SPRD: Modify for bug602315.
        ss.title = getString(R.string.title_section_usbdisk);
        ss.tagIndex = 3;
        mStorageStatus.put(DetailedListFragment.FRAGMENT_TYPE_USB, ss);
        int count = 0;
        for(int i = 0; i < STORAGE_SYZE; i++) {
            DetailedListFragment fragment = DetailedListFragment.init(this, mStorageStatus.keyAt(i));
            ss = mStorageStatus.valueAt(i);
            if(ss.available) {
                listStr.add(ss.title);
                mPagerAdapter.addFragment(new Pair<BaseFragment, Integer>(fragment, ss.tagIndex));
                ss.tabIndex = ++count;
                settings.edit().putBoolean(ss.path, true).commit();
            /* SPRD 441255 @{ */
            }else{
                settings.edit().putBoolean(ss.path, false).commit();
            }
            /* @} */
        }
        mNavigationAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_dropdown_item, listStr);
        mNavigationAdapter.setDropDownViewResource(R.layout.activities_spinner_dropdown_item);
        mActionBar = getActionBar();
        mActionBar.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM);
        mActionBar.setNavigationMode(ActionBar.NAVIGATION_MODE_LIST);
        mActionBar.setListNavigationCallbacks(mNavigationAdapter, new ActionBar.OnNavigationListener() {

            @Override
            public boolean onNavigationItemSelected(int itemPosition, long itemId) {
                if (FileExploreActivity.this.mViewPager.getCurrentItem() != mActionBar.getSelectedNavigationIndex())
                    mViewPager.setCurrentItem(itemPosition);
                return true;
            }
        });
        mViewPager = (ViewPager) findViewById(R.id.pager);
        mViewPager.setAdapter(mPagerAdapter);
        mViewPager.setOnPageChangeListener(new OnPageChangeListener() {
            private int prePos = 0;
            @Override
            public void onPageScrollStateChanged(int arg0) {}

            @Override
            public void onPageScrolled(int arg0, float arg1, int arg2) {}

            @Override
            public void onPageSelected(int position) {
                Slog.i(TAG, "onPageSelected position is "+ position);
                mActionBar.setSelectedNavigationItem(position);
                mCurrentFragment = mPagerAdapter.getItem(position);
                mActionBar.setIcon(mCurrentFragment.getIcon());
                StorageStatus ss = getStorageStatusByPosition(position);
                if(ss != null) {
                    ss.idHidden = false;
                }
                ss = getStorageStatusByPosition(prePos);
                if(ss != null) {
                    ss.idHidden = true;
                }
                prePos = position;
            }
        });
    }
    
    
    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (checkSelfPermission(WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            StorageUtil.removeStorageChangeListener(mStorageChangedListener);
            requestPermissions(new String[] { WRITE_EXTERNAL_STORAGE },
                    STORAGE_PERMISSION_REQUEST_CODE);
         } else {
             setIntent(intent);
             processExtraData();
         }

//        String pastePath = intent.getStringExtra(FilePasteActivity.PASTE_DEST_PATH);
//        if(pastePath == null)
//            return;
//        int index = 0;
//        StorageStatus ss;
//        for(; index < STORAGE_SYZE; index++) {
//            ss = mStorageStatus.valueAt(index);
//            if(pastePath.startsWith(ss.path)) {
//                break;
//            }
//        }
//        if(index == STORAGE_SYZE) {
//            return;
//        }
//        ss = mStorageStatus.valueAt(index);
//        int changeToItem = ss.tabIndex;
//        if(mViewPager.getCurrentItem() != changeToItem) {
//            mViewPager.setCurrentItem(changeToItem, false);
//        }
//        FileListAdapter adapter = (FileListAdapter) mCurrentFragment.getAdapter();
//        File pasteResultFile = new File(pastePath);
//        if(adapter != null) {
//            adapter.setCurrentPath(pasteResultFile);
//        } else {
//            if (mCurrentFragment instanceof DetailedListFragment){
//                DetailedListFragment tmp = (DetailedListFragment) mCurrentFragment;
//                tmp.setTopPath(pasteResultFile);
//            }
//        }
    }

    /* SPRD: Add for bug 489831, cann't back to upper folder. @{ */
    private int getFragmentCount(int type) {
        int count = 0;
        for (int i = type; i > -1; i--) {
            if (mPagerAdapter.isContain(i)) {
                count++;
            }
        }
        return count;
    }
    /* @} */

    /**
     * The {@link StorageReceiver} that receive SDCard mounting action, and call
     * this back. TODO:We should add our protection and UI changing here, like
     * cancel copy and paste and others.
     */
    private void onStorageChanged(String path, boolean available, boolean sdcard) {
        /* SPRD: Add for bug601415, dismiss the selection dialog. @{ */
        if (ActivityUtils.mSelectDialog != null && ActivityUtils.mSelectDialog.isShowing()) {
            ActivityUtils.mSelectDialog.dismiss();
        }
        /* @} */
        int index = 0;
        for(; index < STORAGE_SYZE; index++) {
            Log.d(TAG, "onStorageChanged(): path = " + path + "; ss.path = " + mStorageStatus.valueAt(index).path
                    + "; index = " + index);
            if(path.startsWith(mStorageStatus.valueAt(index).path)) {
                break;
            }
        }

        /* SPRD: Modify for bug509242. @{ */
        Log.d(TAG, "onStorageChanged(): index = " + index + "; available = " + available + "; sdcard = " + sdcard);
        if(index >= STORAGE_SYZE && sdcard) {
            index = 1;
        }
        /* @} */

        if(index >= STORAGE_SYZE && !path.equals(USB_STORAGE_PATH)) {
            return;
        }
        StorageStatus ss = mStorageStatus.valueAt(index);
        if(ss.available == available && !path.equals(USB_STORAGE_PATH)) {
            //not change, do nothing
            return;
        }
        if (path.equals(USB_STORAGE_PATH)) {
            ss.available = StorageUtil.getUSBStorageState();
        } else {
            ss.available = available;
        }

        /* SPRD: Modify for bug509242. @{ */
        if(available && sdcard && !path.startsWith(ss.path)) {
            ss.path = StorageUtil.getExternalStorage().getAbsolutePath();
        }
        /* @} */

        // SPRD: Add for bug 489831, cann't back to upper folder.
        int location = getFragmentCount(index + 1);
        Log.d(TAG, "onStorageChanged(): ss.title = " + ss.title + "; ss.path = " + ss.path + "; location = " + location);
        FileListAdapter needRefreshFla = null;

        /* SPRD: Modify for bug516178, if current frament is External Storage, close the opened context menu. @{ */
        mCurrentFragment = mPagerAdapter.getItem(mViewPager.getCurrentItem());
        if (mCurrentFragment.getAdapter() != null && mCurrentFragment.getAdapter().getCurrentString().startsWith(path)) {
            closeContextMenu();
        }
        /* @} */

        if (path.equals(USB_STORAGE_PATH)) {
            FileUtil.refreshUSBFragment();
        }

        if(available) {
            if ((StorageUtil.getUSBCount() == 0 || StorageUtil.getUSBCount() == 1)
                    && path.equals(USB_STORAGE_PATH) || !path.equals(USB_STORAGE_PATH)) {
                DetailedListFragment fragment = DetailedListFragment.init(this, mStorageStatus.keyAt(index));
                /* SPRD: Modify for bug 489831, cann't back to upper folder. @{ */
                mPagerAdapter.addFragment(new Pair<BaseFragment, Integer>(fragment, ss.tagIndex), location);
                mNavigationAdapter.insert(ss.title, location);
                needRefreshFla = fragment.getAdapter();
            }
//            if (index == 1 && !mPagerAdapter.isContain(0)
//                    || index == 2 && !mPagerAdapter.isContain(0)
//                    || index == 2 && !mPagerAdapter.isContain(1)) {
//                if (index == 2 && !mPagerAdapter.isContain(0)
//                        && !mPagerAdapter.isContain(1)) {
//                    mPagerAdapter.addFragment(new Pair<BaseFragment, Integer>(
//                            fragment, ss.tagIndex), index - 1);
//                    mNavigationAdapter.add(ss.title);
//                } else {
//                    mPagerAdapter.addFragment(new Pair<BaseFragment, Integer>(
//                            fragment, ss.tagIndex), index);
//                    mNavigationAdapter.add(ss.title);
//                }
//            } else {
//                mPagerAdapter.addFragment(new Pair<BaseFragment, Integer>(
//                        fragment, ss.tagIndex), index + 1);
//                mNavigationAdapter.insert(ss.title, index + 1);
//            }
            /* @} */
        } else {
            // SPRD: Modify for bug612571.
            if ((!StorageUtil.getUSBStorageState() && path.equals(USB_STORAGE_PATH))
                    || !path.equals(USB_STORAGE_PATH)) {
                /* SPRD: Modify for bug 489831, cann't back to upper folder. @{ */
                mPagerAdapter.removeFragment(location - 1);
//            if (index == 1 && !mPagerAdapter.isContain(0)
//                    || index == 2 && !mPagerAdapter.isContain(0)
//                    || index == 2 && !mPagerAdapter.isContain(1)) {
//                if (index == 2 && !mPagerAdapter.isContain(0)
//                        && !mPagerAdapter.isContain(1)) {
//                    mPagerAdapter.removeFragment(index - 1);
//                } else {
//                    mPagerAdapter.removeFragment(index);
//                }
//            } else {
//                mPagerAdapter.removeFragment(index + 1);
//            }
            /* @} */
                mNavigationAdapter.remove(ss.title);
                ss.tabIndex = -1;
            }
        }
        /* SPRD : BugFix bug364231 after copy is finished , it goto a wrong folder @{ */
        for (int i = 0, count = 0; i < STORAGE_SYZE; i++) {
            ss = mStorageStatus.valueAt(i);
            if (ss.available) {
                ss.tabIndex = ++count;
            }
        }
        /* @} */
        mPagerAdapter.notifyDataSetChanged();
        if (index == 0 && available == false) {
            mViewPager.setCurrentItem(0, false);
        }

        // SPRD: Add for bug 489831, cann't back to upper folder.
        mCurrentFragment = mPagerAdapter.getItem(mViewPager.getCurrentItem());

        if(needRefreshFla != null) {
            needRefreshFla.refresh();
        }
        invalidateOptionsMenu();
    }

    /* SPRD: Bug 432426 Menu shows wrong after language changed @{ */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        if(StorageUtil.getInternalStorageState() || StorageUtil.getExternalStorageState()){
            getMenuInflater().inflate(R.menu.main, menu);
        }
        if (mCurrentFragment instanceof DetailedListFragment) {
            getMenuInflater().inflate(R.menu.detailed_list_menu, menu);
        }
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
//        Slog.d(TAG, "onPrepareOptionsMenu");
//        if(menu != null){
//            menu.clear();
//        }
//        if(StorageUtil.getInternalStorageState() || StorageUtil.getExternalStorageState()){
//            getMenuInflater().inflate(R.menu.main, menu);
//        }
        return true;
    }
    /* @} */
    
    /**
     * We use this to manage our fragments. A {@link FragmentPagerAdapter} that
     * returns a fragment corresponding to one of the sections/tabs/pages.
     */
    public class SectionsPagerAdapter extends FragmentPagerAdapter {
        
        ArrayList<Pair<BaseFragment, Integer>> mFragmentList;
        private FragmentManager fm;
        
        public SectionsPagerAdapter(FragmentManager fm) {
            super(fm);
            this.fm = fm;
            this.mFragmentList = new ArrayList<Pair<BaseFragment, Integer>>();
        }

        public List<Pair<BaseFragment, Integer>> getFragments() {
            return mFragmentList;
        }

        public void addFragment(Pair<BaseFragment, Integer> fragment) {
            /* SPRD 427975 @{ */
            if(mFragmentList != null && fragment != null && mFragmentList.contains(fragment)){
                Log.d(TAG, "SectionsPagerAdapter addFragment," + fragment.second + " exist! won't be added");
                return;
            }
            /* @} */
            // SPRD 459914
            if(mFragmentList != null && fragment != null){
                mFragmentList.add(fragment);
            }
        }

        public void addFragment(Pair<BaseFragment, Integer> fragment, int index) {
            mFragmentList.add(index, fragment);
        }

        public void removeFragment(int index) {
            if (index >= mFragmentList.size()) {
                return;
            }
            FragmentTransaction transaction = fm.beginTransaction();
            transaction.remove(mFragmentList.get(index).first)
                    .commitAllowingStateLoss();
            mFragmentList.remove(index);
        }

        @Override
        public BaseFragment getItem(int position) {
            return mFragmentList.get(position).first;
        }

        public long getItemId(int position) {
            return mFragmentList.get(position).second;
        }

        @Override
        public int getItemPosition(Object object) {
            return POSITION_NONE;
        }

        /**
         * Override this method to set the title of ActionBar
         * 
         * @Override {@link FragmentPagerAdapter#setPrimaryItem(ViewGroup, int, Object)}
         */
        public void setPrimaryItem(ViewGroup container, int position,
                Object object) {
            if (object != null)
                super.setPrimaryItem(container, position, object);
            else 
                Log.d(TAG, "setPrimaryItem failed, container: " + container
                    + ", object: " + object);
        }

        @Override
        public int getCount() {
            return mFragmentList.size();
        }

        @Override
        public CharSequence getPageTitle(int position) {
            StorageStatus ss = getStorageStatusByPosition(position);
            if(ss == null) {
                return getText(R.string.title_section_overview);
            } else {
                return ss.title;
            }
        }
        /* SPRD : BugFix bug356810 connect to PC and then unplug USB show FATAL @{ */
        public boolean isContain(int index) {
            /* SPRD: Add for bug465956. @{ */
            /*StorageStatus ss = mStorageStatus.valueAt(index);
            DetailedListFragment fragment = DetailedListFragment
                    .getInstance(mStorageStatus.keyAt(index));
            boolean isOrnot = mPagerAdapter.mFragmentList
                    .contains(new Pair<BaseFragment, Integer>(fragment,
                            ss.tagIndex));
            return isOrnot;*/
            int size = mFragmentList.size();
            Pair<BaseFragment, Integer> fragmentPair;
            for (int i = 0; i < size; i++) {
                fragmentPair = mFragmentList.get(i);
                Log.i(TAG, "SectionsPagerAdapter " + fragmentPair.second);
                if (index == fragmentPair.second) {
                    Log.i(TAG, "SectionsPagerAdapter isContain return true");
                    return true;
                }
            }
            return false;
            /* @} */
        }
        /* @} */
    }
    
    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy");
        StorageUtil.removeStorageChangeListener(mStorageChangedListener);
        // SPRD: Add for bug505955.
        FileUtil.removePastePathChangeListener(this);
        super.onDestroy();
        DetailedListFragment.clearFragment();
    }

    @Override
    public void finish() {
        if (checkSelfPermission(WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            super.finish();
        } else {
            moveTaskToBack(true);
        }
    }

    /**
     * When back key pressed, we call every {@link BaseFragment#onBackPressed}
     * in current displaying fragment
     */
    public void onBackPressed() {
        processBackKey();
    }

    /* SPRD: Add for bug465956. @{ */
    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data){
        switch(resultCode){
            case ActivityUtils.COPY_PATH_RESULT:
                if (data != null) {
                    String pastePath = data.getStringExtra(FilePasteActivity.PASTE_DEST_PATH);
                    if (pastePath == null)
                        return;
                    int index = 0;
                    StorageStatus ss;
                    for (; index < STORAGE_SYZE; index++) {
                        ss = mStorageStatus.valueAt(index);
                        // SPRD: Add for bug602265.
                        if (pastePath.startsWith(ss.path) && ss.available) {
                            break;
                        }
                    }
                    /* SPRD: Add for bug601391&bug602265, double check which storage device was changed. @{ */
                    if (index == STORAGE_SYZE) {
                        if (StorageUtil.isInExternalStorage(pastePath) && StorageUtil.getExternalStorageState()) {
                            index = 1;
                        } else if (StorageUtil.isInUSBStorage(pastePath) && StorageUtil.isStorageMounted(new File(pastePath))) {
                            index = 2;
                        } else {
                            return;
                        }
                    }
                    /* @} */

                    ss = mStorageStatus.valueAt(index);
                    int changeToItem = ss.tabIndex;
                    if (mViewPager.getCurrentItem() != changeToItem) {
                        mViewPager.setCurrentItem(changeToItem, false);
                    }
                    FileListAdapter adapter = (FileListAdapter) mCurrentFragment.getAdapter();
                    File pasteResultFile = new File(pastePath);
                    if (adapter != null) {
                        adapter.setCurrentPath(pasteResultFile);
                        // SPRD: Add for bug534237, refresh UI.
                        adapter.notifyDataSetChanged();
                    }
                }
                break;
            default:
                break;
        }
    }
    /* @} */

    private void processBackKey() {
        if (mCurrentFragment.onBackPressed()) {
            // SPRD: Modify for bug465956.
            moveTaskToBack(true);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case android.R.id.home:
            processBackKey();
            return true;
        case R.id.action_settings:
            Intent intent = new Intent(this, SettingActivity.class);
            startActivity(intent);
            return true;
        case R.id.searchfile:
            this.startActivity(new Intent().setClass(
                    this.getApplicationContext(), FileSearchActivity.class));
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (mCurrentFragment instanceof DetailedListFragment) {
                FileListAdapter adapter = (FileListAdapter) mCurrentFragment.getAdapter();
                if (adapter != null) {
                    adapter.setCurrentPath(adapter.getTopPath());
                    return true;
                }
            }
        }
        return super.onKeyLongPress(keyCode, event);
    }
    
    static class StorageStatus {
        String title;
        String path;
        int tagIndex;
        boolean available = false;
        boolean idHidden = true;
        int tabIndex = -1;
        @Override
        public String toString() {
            return "StorageStatus [title=" + title + ", path=" + path + ",tagIndex=" + tagIndex + ", available=" + available
                    + ", idHidden=" + idHidden + ", tabIndex=" + tabIndex + "]";
        }
    }
    
    private StorageStatus getStorageStatusByPosition(int position) {
        if(position <= 0) {
            return null;
        }
        for(int index = 0; index < STORAGE_SYZE; index++) {
            if(mStorageStatus.valueAt(index).tabIndex == position) {
                return mStorageStatus.valueAt(index);
            }
        }
        return null;
    }

    private void processExtraData() {
        Intent intent = getIntent();
        if (intent != null) {
            String pastePath = intent
                    .getStringExtra(FilePasteActivity.PASTE_DEST_PATH);
            if (pastePath == null)
                return;
            int index = 0;
            StorageStatus ss;
            for (; index < STORAGE_SYZE; index++) {
                ss = mStorageStatus.valueAt(index);
                if (pastePath.startsWith(ss.path)) {
                    break;
                }
            }
            if (index == STORAGE_SYZE) {
                return;
            }
            ss = mStorageStatus.valueAt(index);
            int changeToItem = ss.tabIndex;
            if (mViewPager.getCurrentItem() != changeToItem) {
                mViewPager.setCurrentItem(changeToItem, false);
            }
            FileListAdapter adapter = (FileListAdapter) mCurrentFragment
                    .getAdapter();
            File pasteResultFile = new File(pastePath);
            if (adapter != null) {
                adapter.setCurrentPath(pasteResultFile);
            }
            /* SPRD : BugFix bug362141 change language when copy,after it is done you can not enter root directory @{
            else {
                if (mCurrentFragment instanceof DetailedListFragment) {
                    DetailedListFragment tmp = (DetailedListFragment) mCurrentFragment;
                    tmp.setTopPath(pasteResultFile);
                }
            }
            @} */
        }
    }
    /* SPRD 426406 @{ */
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        Log.i(TAG,"onConfigurationChanged");
        if(mViewPager == null || mPagerAdapter == null){
            Log.i(TAG,"onConfigurationChanged do nothing");
            return;
        }
        /* SPRD: Modify for bug618042. @{
        int index = mViewPager.getCurrentItem();
        mViewPager.setAdapter(mPagerAdapter);
        mViewPager.setCurrentItem(index, false);
        @} */
    }
    /* @} */
    /* SPRD 453550 @{ */
    @Override
    public boolean onKeyDown(int keycode,KeyEvent keyevent){
        Log.i(TAG,"onKeyDown keyevent.getAction():"+keyevent.getAction() + "  keycode"+keycode);
        if(keyevent.getAction() == KeyEvent.ACTION_DOWN && keycode == KeyEvent.KEYCODE_SEARCH){
            this.startActivity(new Intent().setClass(
                    this.getApplicationContext(), FileSearchActivity.class));
            return true;
        }
        return super.onKeyDown(keycode, keyevent);
    }
    /* @} */

    /* SPRD: Add for bug505955. @{ */
    @Override
    public void onPasteFinsish(String pastePath) {
        // TODO Auto-generated method stub
        Log.d(TAG, "onPasteFinsish(): targetPath = " + pastePath);

        if (pastePath == null)
            return;
        int index = 0;
        StorageStatus ss;
        for (; index < STORAGE_SYZE; index++) {
            ss = mStorageStatus.valueAt(index);
            // SPRD: Add for bug602265.
            if (pastePath.startsWith(ss.path) && ss.available) {
                break;
            }
        }
        /* SPRD: Add for bug601391&bug602265, double check which storage device was changed. @{ */
        if (index == STORAGE_SYZE) {
            if (StorageUtil.isInExternalStorage(pastePath) && StorageUtil.getExternalStorageState()) {
                index = 1;
            } else if (StorageUtil.isInUSBVolume(pastePath) && StorageUtil.isStorageMounted(new File(pastePath))) {
                index = 2;
            } else {
                return;
            }
        }
        /* @} */
        ss = mStorageStatus.valueAt(index);
        int changeToItem = ss.tabIndex;
        if (mViewPager.getCurrentItem() != changeToItem) {
            mViewPager.setCurrentItem(changeToItem, false);
        }
        FileListAdapter adapter = (FileListAdapter) mCurrentFragment.getAdapter();
        File pasteResultFile = new File(pastePath);
        if (adapter != null) {
            adapter.setCurrentPath(pasteResultFile);
            // SPRD: Add for bug534237, refresh UI.
            adapter.notifyDataSetChanged();
        }
    }
    /* @} */
}
