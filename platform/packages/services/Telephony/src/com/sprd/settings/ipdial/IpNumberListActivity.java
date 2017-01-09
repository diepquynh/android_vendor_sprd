
package com.sprd.phone.settings.ipdial;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import com.android.phone.R;

import android.app.ActionBar;
import android.app.ListActivity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager.LayoutParams;
import android.widget.ArrayAdapter;
import android.widget.FrameLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import com.android.phone.SubscriptionInfoHelper;
import com.sprd.phone.common.utils.IpDialingUtils;

/***********************************************************
 *              SPRD: FEATURE_IP_DIAL
 * *********************************************************/
public class IpNumberListActivity extends ListActivity {
    private final static String TAG = "IpNumberListActivity";

    private final static int RESAULT_ERROR = -1;
    private final static int START_NEW_ACTIVITY = 1;
    private final static int START_DELETE_ACTIVITY = 2;
    private final static int FRIST_ITEM = 0;

    private final static int IP_PREFERENCE_NUM = 10;

    private ArrayList<String> mIpNumberList;
    private IpDialingUtils mIpDialingUtils;
    private boolean mIsIpDial;
    private int mSelectedIpPreference;
    private int mMaxIpPreferenceNum;
    private IpListAdapter<String> mAdapter;
    private ListView mListView;
    private TextView mEmptyText;
    private int mSubId;

    protected static final int MENU_NEW = Menu.FIRST;
    protected static final int MENU_DELETE = Menu.FIRST + 1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final ActionBar actionBar = getActionBar();
        if(actionBar != null) {
            actionBar.setHomeAsUpIndicator(R.drawable.ic_back_arrow);
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setDisplayShowHomeEnabled(false);
        }
        setContentView(R.layout.ip_dialing_list_ex);
        mEmptyText = (TextView) findViewById(android.R.id.empty);
        mEmptyText.setText(getString(R.string.ip_number_empty_loading));
        mIpNumberList = new ArrayList<String>();
        mIpDialingUtils = new IpDialingUtils(this.getApplicationContext());
        SubscriptionInfoHelper subscriptionInfoHelper = new SubscriptionInfoHelper(this,
                getIntent());
        mSubId = subscriptionInfoHelper.getSubId();
        mSelectedIpPreference = mIpDialingUtils.getIpPreferenceNumber(mSubId);
        mIsIpDial = true;
        refreshList();
        mAdapter = new IpListAdapter<String>(this,
                R.layout.ip_dialing_list_item_ex, mIpNumberList);
        setListAdapter(mAdapter);
        initViews();
        mListView.setEmptyView(mEmptyText);
    }

    private void refreshList(){
        mMaxIpPreferenceNum = 0;
        mIpNumberList.clear();
        String number;
        for(int i = 0; i < IP_PREFERENCE_NUM; i++){
            number = mIpDialingUtils.getIpNumber(i,mSubId);
            if(!TextUtils.isEmpty(number)){
                mIpNumberList.add(number);
                mMaxIpPreferenceNum++;
            }
        }
        IpNumberListActivity.this.invalidateOptionsMenu();
    }

    @Override
    protected void onResume() {
        super.onResume();
        mSelectedIpPreference = mIpDialingUtils.getIpPreferenceNumber(mSubId);
        if(!mIsIpDial){
            mListView.setItemChecked(0,true);
        } else {
            mListView.setItemChecked(mSelectedIpPreference,true);
        }
        refreshList();
    }

    private void initViews() {
        Resources resources = getResources();
        mListView = getListView();
        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams
                (LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);

        mListView.setLayoutParams(params);
        Drawable backgroundRes =
                resources.getDrawable(R.drawable.ip_dialing_list_background_holo_ex);
        getWindow().setBackgroundDrawable(backgroundRes);
        mListView.setBackgroundResource(R.drawable.ip_dialing_list_background_holo_ex);
        mListView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
        mListView.setFooterDividersEnabled(true);
    }


    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        mListView.setItemChecked(position,true);
        mSelectedIpPreference = position;
        mIpDialingUtils.setIpPreferenceNumber(position,mSubId);

        super.onListItemClick(l, v, position, id);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        menu.clear();
        if (mMaxIpPreferenceNum < IP_PREFERENCE_NUM) {
            menu.add(0, MENU_NEW, 0, R.string.add_ip_number_str)
                    .setTitle(R.string.add_ip_number_str)
                    .setShowAsAction(
                            MenuItem.SHOW_AS_ACTION_ALWAYS);
        }

        if (mIpNumberList != null && !mIpNumberList.isEmpty()) {
            menu.add(0, MENU_DELETE, 0, R.string.remove).setShowAsAction(
                    MenuItem.SHOW_AS_ACTION_ALWAYS);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        switch (item.getItemId()) {
        case MENU_NEW:
            Intent intent = new Intent();
            intent.setClass(IpNumberListActivity.this, IpNumberCreateActivity.class);
            startActivityForResult(intent, START_NEW_ACTIVITY);
            return true;
        case MENU_DELETE:
            Intent deleteIntent = new Intent();
                deleteIntent.putExtra(
                        SubscriptionInfoHelper.SUB_ID_EXTRA,
                        getIntent().getIntExtra(SubscriptionInfoHelper.SUB_ID_EXTRA,
                                SubscriptionInfoHelper.NO_SUB_ID));
            deleteIntent.setClass(IpNumberListActivity.this, IpNumberDeleteActivity.class);
            startActivityForResult(deleteIntent, START_DELETE_ACTIVITY);
            return true;
        case android.R.id.home:
            finish();
            return true;
        }
        return false;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {

        if (resultCode == RESAULT_ERROR || data == null) {
            Log.d(TAG, "onActivityResult,RESAULT_ERROR,requestCode="
                    + requestCode);
            return;
        }
        switch (requestCode) {
            case START_NEW_ACTIVITY:
                String num = data.getStringExtra("ipNumber");
                if (mIpNumberList.contains(num)) {
                    Toast.makeText(IpNumberListActivity.this,
                            R.string.ip_dialing_num_repeat, Toast.LENGTH_SHORT)
                            .show();
                    break;
                }
                if (mMaxIpPreferenceNum >= IP_PREFERENCE_NUM) {
                    Toast.makeText(IpNumberListActivity.this,
                            R.string.ip_dialing_num_error, Toast.LENGTH_SHORT)
                            .show();
                    break;
                }
                mIpDialingUtils.setIpNumber(num, mMaxIpPreferenceNum,mSubId);
                mIpDialingUtils.setIpPreferenceNumber(mMaxIpPreferenceNum,mSubId);
                mMaxIpPreferenceNum++;
                refreshList();
                mAdapter.notifyDataSetChanged();
                break;

            case START_DELETE_ACTIVITY:
                refreshList();
                mAdapter.notifyDataSetChanged();
                break;
        }
    }

    private class IpListAdapter<String> extends ArrayAdapter<String> {

        public IpListAdapter(Context context, int textViewResourceId,
                List objects) {
            super(context, textViewResourceId, objects );
            // TODO Auto-generated constructor stub
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            View view = super.getView(position, convertView, parent);
            return view;
        }
    }

}
