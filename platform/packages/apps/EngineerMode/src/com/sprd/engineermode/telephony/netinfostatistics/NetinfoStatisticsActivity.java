package com.sprd.engineermode.telephony.netinfostatistics;

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
import android.os.Bundle;
import android.os.Build;
import android.preference.PreferenceManager;
import android.app.Fragment;
import android.app.FragmentManager;
import android.support.v4.app.FragmentActivity;
import android.support.v4.content.ContextCompat;
import android.support.v4.view.ViewPager;
import android.support.design.widget.TabLayout;
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
import android.widget.TextView;
import android.widget.Toast;
import com.sprd.engineermode.R;
import com.sprd.engineermode.TabFragmentPagerAdapter;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.widget.Toast;
import android.support.design.widget.CollapsingToolbarLayout;

public class NetinfoStatisticsActivity extends AppCompatActivity {

    private static final String TAG = "NetinfoStatisticsActivity";
    private ArrayList<Fragment> mFragmentsList;
    private List<String> mTitleList = new ArrayList<String>();
    private ViewPager mViewPager;
    private Context mContext;

    public interface TabState {
        public static int TAB_RESELECT_INDEX = 0;  //Reselect
        public static int TAB_HANDOVER_INDEX = 1; //handOver
        public static int TAB_ATTACH_INDEX = 2; //Attachtime
        public static int TAB_DROP_INDEX = 3; //droptimes
        public static int TAB_CARRIER_HANDOVER_INDEX = 4; //carrier handover times
    }

    private int[] mTabTitle = new int[] { R.string.tab_reselect,
            R.string.tab_handover, R.string.tab_attachTime,
            R.string.tab_dropTimes, R.string.tab_carrierHandoverTimes};

    //private int mCurrentTab = TabState.TAB_RESELECT_INDEX;
    public static String CURRENT_TAB = "android.app.engmode.currenttab";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;
        setToolbarTabLayout();
        initCoordinatorAndTabLayout();
    }

    private void setToolbarTabLayout() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            getWindow().setStatusBarColor(ContextCompat.getColor(this, R.color.colorPrimaryDark));
        }
    }

    private void initCoordinatorAndTabLayout() {
        setContentView(R.layout.activity_netinfostatistics);
        mFragmentsList = new ArrayList<Fragment>();
        mViewPager = (ViewPager)findViewById(R.id.view_pager);
        FragmentManager fragmentManager = getFragmentManager();
        Fragment reselectFragment = new ReselectFragment();
        Fragment handOverFragment = new HandOverFragment();
        Fragment attachTimeFragment = new AttachTimeFragment();
        Fragment dropTimesFragment = new DropTimesFragment();
        Fragment carrierHandoverTimesFragment = new CarrierHandoverTimesFragment();

        mFragmentsList.add(reselectFragment);
        mFragmentsList.add(handOverFragment);
        mFragmentsList.add(attachTimeFragment);
        mFragmentsList.add(dropTimesFragment);
        mFragmentsList.add(carrierHandoverTimesFragment);
        mViewPager.setAdapter(new TabFragmentPagerAdapter(fragmentManager,
                mFragmentsList, mTabTitle, mContext));

        TabLayout tabLayout = (TabLayout)findViewById(R.id.tab_layout);
        tabLayout.addTab(tabLayout.newTab().setText(this.getResources().getString(R.string.tab_reselect)));
        tabLayout.addTab(tabLayout.newTab().setText(this.getResources().getString(R.string.tab_handover)));
        tabLayout.addTab(tabLayout.newTab().setText(this.getResources().getString(R.string.tab_attachTime)));
        tabLayout.addTab(tabLayout.newTab().setText(this.getResources().getString(R.string.tab_dropTimes)));
        tabLayout.addTab(tabLayout.newTab().setText(this.getResources().getString(R.string.tab_carrierHandoverTimes)));
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
        super.onDestroy();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        Intent intent = this.getIntent();
        int simindex = intent.getIntExtra("simindex", -1);
        setTitle("SIM" + simindex);
        Log.d(TAG, "onResume simindex=" + simindex);
    }
}

