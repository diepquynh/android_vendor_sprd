package com.sprd.engineermode;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.sprd.engineermode.connectivity.ConnectivityFragment;
import com.sprd.engineermode.debuglog.DebugLogFragment;
import com.sprd.engineermode.hardware.HardWareFragment;
import com.sprd.engineermode.telephony.TelephonyFragment;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ActionBar.Tab;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.net.LocalSocketAddress;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.preference.PreferenceManager;
import android.app.Fragment;
import android.app.FragmentManager;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.design.widget.TabLayout;
import android.support.v4.content.ContextCompat;
import android.support.v4.view.ViewPager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.utils.SocketUtils;
import android.view.WindowManager;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.widget.Toast;
import android.support.design.widget.CollapsingToolbarLayout;

public class EngineerModeActivity extends AppCompatActivity {

    private static final String TAG = "EngineerModeActivity";

    //message list
    private static final int CLOSE_CP2 = 0;
    private static final int OPEN_CP2 = 1;

    //AT Command list
    private static final String AT_CLOSE_CP2 = "poweroff";
    private static final String AT_OPEN_CP2 = "poweron";

    //scoket name
    private static final String SOCKET_NAME = "wcnd";
    
    public static boolean isCP2On = false;
    private SharedPreferences mPrefs;
    private ArrayList<Fragment> mFragmentsList;
    private List<String> mTitleList = new ArrayList<String>();
    private ViewPager mViewPager;
    private Context mContext;
    
    private Handler mUiThread = new Handler();
    private EMHandler mEMHandler;
    
    public interface TabState {
        public static int TAB_TELE_INDEX = 0;
        public static int TAB_DEBUG_INDEX = 1;
        public static int TAB_CONNECT_INDEX = 2;
        public static int TAB_HARDWARE_INDEX = 3;
    }

    private int[] mTabTitle = new int[] { R.string.tab_telephony,
            R.string.tab_debug, R.string.tab_connectivity,
            R.string.tab_hardwaretest };

    private int mCurrentTab = TabState.TAB_TELE_INDEX;
    public static String CURRENT_TAB = "android.app.engmode.currenttab";
    public static boolean mIsFirst = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;
        setToolbarTabLayout();
        initCoordinatorAndTabLayout();
        HandlerThread ht = new HandlerThread("EMHandler");
        ht.start();
        mEMHandler = new EMHandler(ht.getLooper());
        Message openCP2 = mEMHandler.obtainMessage(OPEN_CP2);
        mEMHandler.sendMessage(openCP2); 
    }

    private void setToolbarTabLayout() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            getWindow().setStatusBarColor(ContextCompat.getColor(this, R.color.colorPrimaryDark));
        }
    }

    private void initCoordinatorAndTabLayout() {
        setContentView(R.layout.activity_main);
        mFragmentsList = new ArrayList<Fragment>();
        mViewPager = (ViewPager)findViewById(R.id.view_pager);
        FragmentManager fragmentManager = getFragmentManager();
        Fragment telephonyFragment = new TelephonyFragment();
        Fragment debugFragment = new DebugLogFragment();
        Fragment connectivityFragment = new ConnectivityFragment();
        Fragment hardwareFragment = new HardWareFragment();

        mFragmentsList.add(telephonyFragment);
        mFragmentsList.add(debugFragment);
        mFragmentsList.add(connectivityFragment);
        mFragmentsList.add(hardwareFragment);
        mViewPager.setAdapter(new TabFragmentPagerAdapter(fragmentManager,
                mFragmentsList, mTabTitle, mContext));

        TabLayout tabLayout = (TabLayout)findViewById(R.id.tab_layout);
        tabLayout.addTab(tabLayout.newTab().setText(this.getResources().getString(R.string.tab_telephony)));
        tabLayout.addTab(tabLayout.newTab().setText(this.getResources().getString(R.string.tab_debug)));
        tabLayout.addTab(tabLayout.newTab().setText(this.getResources().getString(R.string.tab_connectivity)));
        tabLayout.addTab(tabLayout.newTab().setText(this.getResources().getString(R.string.tab_hardwaretest)));
        tabLayout.setupWithViewPager(mViewPager);
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.version_info, menu);
        MenuItem item =menu.findItem(R.id.action_version_info);
        if (item != null) {
            item.setVisible(true);
        }
        return super.onCreateOptionsMenu(menu); 
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_version_info:
                AlertDialog alertDialog = new AlertDialog.Builder(this)
                .setTitle(getString(R.string.version_info))
                .setMessage(getString(R.string.version_info_detail))
                .setPositiveButton(R.string.alertdialog_ok,
                        new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                    }
                }) .create();
                alertDialog.show();
                return true;
            default:
                Log.i(TAG, "default");
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onDestroy() {
        mIsFirst = false;
        super.onDestroy();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    /**
     *  we should open CP2 before send or receive wcnd command on Android 4.4,so
     *  when EM is in the background because click back,close CP2
     */
    @Override
    public void onBackPressed() {
        Log.d(TAG,"onBackPressed isCP2On is "+isCP2On);
        if(isCP2On){
            Message closeCP2 = mEMHandler.obtainMessage(CLOSE_CP2,"1");
            mEMHandler.sendMessage(closeCP2); 
        }else{
            super.onBackPressed();
        }
    }

    /**
     *  we should open CP2 before send or receive wcnd command on Android 4.4,so
     *  when EM is in the background becasue click home,close CP2
     */
    @Override
    protected void onUserLeaveHint() {
        Log.d(TAG,"onUserLeaveHint isCP2On is "+isCP2On);
        if(isCP2On){
            Message closeCP2 = mEMHandler.obtainMessage(CLOSE_CP2,"0");
            mEMHandler.sendMessage(closeCP2); 
        }
        super.onUserLeaveHint();   
    }


    class EMHandler extends Handler {

        public EMHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg){
            String atResponse;
            String atCmd;
            switch (msg.what){
                case OPEN_CP2:
                    atCmd = "wcn "+AT_OPEN_CP2;
                    atResponse = SocketUtils.sendCmdAndRecResult(SOCKET_NAME, LocalSocketAddress.Namespace.ABSTRACT,atCmd);
                    Log.d(TAG,"OPEN_CP2 Response is "+atResponse);
                    if (atResponse != null && atResponse.contains(SocketUtils.OK)) {
                        isCP2On = true;  
                    }
                    break;
                case CLOSE_CP2:
                    atCmd = "wcn "+AT_CLOSE_CP2;
                    atResponse = SocketUtils.sendCmdAndRecResult(SOCKET_NAME, LocalSocketAddress.Namespace.ABSTRACT,atCmd);
                    Log.d(TAG,"First Time CLOSE_CP2 Response is "+atResponse);
                    if(atResponse != null && atResponse.contains(SocketUtils.OK)){
                        isCP2On = false;                   
                    }else{
                        //close CP2 one more time
                        atResponse = SocketUtils.sendCmdAndRecResult(SOCKET_NAME, LocalSocketAddress.Namespace.ABSTRACT,atCmd);
                        Log.d(TAG,"Second Time CLOSE_CP2 Response is "+atResponse);
                        if(atResponse != null && atResponse.contains(SocketUtils.OK)){
                            isCP2On = false;
                        }else{
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {   
                                    isCP2On = true;
                                    Log.e(TAG,"EngineerModeActivity CP2 Close Fail");
                                    Toast.makeText(EngineerModeActivity.this, "CP2 Close Fail", Toast.LENGTH_SHORT)
                                    .show();
                                }
                            }); 
                        }
                    }
                    /**
                     * when click back button, close CP2 and finish EngineerModeActivity
                     * 1 means click back button
                     * 0 means click home button
                     * modify bug 332763 by sprd
                     */
                    if(((String) msg.obj).equals("1")){
                        finish(); 
                    } 
            } 
        }
    }
}
