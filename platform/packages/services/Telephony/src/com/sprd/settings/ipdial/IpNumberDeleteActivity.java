package com.sprd.phone.settings.ipdial;


import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import com.android.phone.R;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.app.ActionBar;
import android.app.Activity;
import android.app.ListActivity;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.Menu;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.view.WindowManager.LayoutParams;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CheckedTextView;
import android.widget.FrameLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.AdapterView.OnItemClickListener;
import com.android.phone.SubscriptionInfoHelper;
import com.sprd.phone.common.utils.IpDialingUtils;
/***********************************************************
 *              SPRD: FEATURE_IP_DIAL
 * *********************************************************/
public class IpNumberDeleteActivity extends Activity implements OnClickListener,OnItemClickListener {
    private static final String TAG = "IpNumberDeleteActivity";

    private final static int RESAULT_OK = 0;
    private final static int RESAULT_ERROR = -1;
    private final static int IP_PREFERENCE_NUM = 10;
    private final static int FRIST_ITEM = 0;

    private ArrayList<String> mIpNumberList;
    private IpDialingUtils mIpDialingUtils;
    private IpNumberDeleteAdapter mAdapter;
    private boolean[] mListCheckStatus;
    private String mIpPreferenceNumber;
    private CheckBox selectAllChcekbox;
    private ListView commonDelList;
    private TextView checkboxText;
    private int mSubId;

    protected static final int MENU_OK = Menu.FIRST;
    protected static final int MENU_CANCLE = Menu.FIRST + 1;
    private static final String KEY_FILTER = "mFilter";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.ip_dialing_delete_activity_ex);
        getActionBar().setHomeAsUpIndicator(R.drawable.ic_back_arrow);
        getActionBar().setDisplayUseLogoEnabled(false);
        getActionBar().setDisplayShowHomeEnabled(false);
        getActionBar().setDisplayOptions(ActionBar.DISPLAY_SHOW_TITLE
                | ActionBar.DISPLAY_HOME_AS_UP);
        SubscriptionInfoHelper subscriptionInfoHelper = new SubscriptionInfoHelper(this,
                getIntent());
        mSubId = subscriptionInfoHelper.getSubId();
        mIpNumberList = new ArrayList<String>();
        mIpDialingUtils = new IpDialingUtils(this.getApplicationContext());
        if (savedInstanceState != null) {
            mListCheckStatus = savedInstanceState.getBooleanArray(KEY_FILTER);
        }else{
            mListCheckStatus = new boolean[IP_PREFERENCE_NUM];
        }
        refreshList();
        commonDelList = (ListView) findViewById(R.id.delete_list);
        selectAllChcekbox = (CheckBox) findViewById(R.id.checkbox_select_all);
        checkboxText = (TextView) findViewById(R.id.checkbox_title);
        checkboxText.setText(R.string.menu_selected_all);

        commonDelList.setOnItemClickListener(this);
        selectAllChcekbox.setOnClickListener(this);

        mIpPreferenceNumber = mIpDialingUtils.getIpDialNumber(mSubId);
        mAdapter = new IpNumberDeleteAdapter(IpNumberDeleteActivity.this, mIpNumberList);
        commonDelList.setAdapter(mAdapter);
    }

    public class IpNumberDeleteAdapter extends BaseAdapter {
        private int mCount = 0;
        private ArrayList<String> mArrayList;
        private LayoutInflater mLayoutInflater;
        private Map<Integer, Boolean> mSelectedMap;

        public IpNumberDeleteAdapter(Context context,
                ArrayList<String> arrayList) {
            mLayoutInflater = LayoutInflater.from(context);
            mArrayList = arrayList;

            if (mArrayList != null) {
                mCount = mArrayList.size();
                mSelectedMap = new HashMap<Integer, Boolean>();
                for (int i = 0; i < mCount; i++) {
                    mSelectedMap.put(i, mListCheckStatus[i]);
                }
            } else {
                Log.e(TAG, "CommomPhraseDeleteAdapter, mArrayList is null !");
            }

            if (mArrayList == null || mArrayList != null && mCount == 0) {
                if (selectAllChcekbox != null) {
                    selectAllChcekbox.setEnabled(false);
                }
            } else {
                if (selectAllChcekbox != null) {
                    selectAllChcekbox.setEnabled(true);
                }
            }
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            ViewHolder viewHolder = null;

            if (convertView == null) {
                convertView = mLayoutInflater
                        .inflate(R.layout.ip_dialing_delete_list_item_ex, null);

                viewHolder = new ViewHolder();
                viewHolder.contentTextView =
                        (TextView) convertView.findViewById(R.id.common_message_cotent);
                viewHolder.delCheckBox =
                        (CheckBox) convertView.findViewById(R.id.delete_list_item_checkbox);

                convertView.setTag(viewHolder);
            } else {
                viewHolder = (ViewHolder) convertView.getTag();
            }
            bindView(viewHolder, position);

            return convertView;
        }

        private void bindView(ViewHolder viewholder, int position) {
            if (mArrayList != null) {
                if (mArrayList.get(position) != null) {
                    viewholder.delCheckBox.setChecked((boolean) mSelectedMap
                            .get(position));
                    viewholder.contentTextView
                            .setText(mArrayList.get(position));
                } else {
                    Log.e(TAG, "mArrayList fail to move, positon= " + position);
                }
            } else {
                Log.e(TAG, "bindView, the mArrayList is null !");
            }
        }

        public Map<Integer, Boolean> getSelectedMap() {
            return mSelectedMap;
        }

        @Override
        public int getCount() {
            return mCount;
        }

        @Override
        public Object getItem(int position) {
            // TODO Auto-generated method stub
            if (mArrayList.get(position) != null) {
                return mArrayList.get(position);
            }
            return null;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }
    }

    private final class ViewHolder {
        public  TextView contentTextView;
        public  CheckBox delCheckBox;
     }

    private void refreshList() {
        mIpNumberList.clear();
        String number;
        for (int i = 0; i < IP_PREFERENCE_NUM; i++) {
            number = mIpDialingUtils.getIpNumber(i,mSubId);
            if (!TextUtils.isEmpty(number)) {
                mIpNumberList.add(number);
            }
        }
    }

    @Override
    protected void onResume(){
        super.onResume();
        setSelectAllChcekboxState();
    }

    @Override
    protected void onSaveInstanceState(Bundle savedInstanceState) {
        super.onSaveInstanceState(savedInstanceState);
        savedInstanceState.putBooleanArray(KEY_FILTER, mListCheckStatus);
    }

    private void setSelectAllChcekboxState() {
        if (mAdapter.getSelectedMap().containsValue(false)) {
            selectAllChcekbox.setChecked(false);
            checkboxText.setText(R.string.menu_selected_all);
        } else {
            selectAllChcekbox.setChecked(true);
            checkboxText.setText(R.string.menu_select_none);
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        ViewHolder views = (ViewHolder) view.getTag();
        views.delCheckBox.toggle();
        mAdapter.getSelectedMap().put(position, views.delCheckBox.isChecked());
        if (mListCheckStatus[position] == true) {
            mListCheckStatus[position] = false;
        } else {
            mListCheckStatus[position] = true;
        }
        setSelectAllChcekboxState();
        IpNumberDeleteActivity.this.invalidateOptionsMenu();
        mAdapter.notifyDataSetChanged();
    }

    @Override
    public void onClick(View v) {
        if (v == selectAllChcekbox && mAdapter !=null ) {
            int count = mAdapter.getCount();
            // do select all
            if (selectAllChcekbox.isChecked()) {
                for (int i = 0; i < count; i++) {
                    mAdapter.getSelectedMap().put(i, true);
                    mListCheckStatus[i]= true;
                }
                checkboxText.setText(R.string.menu_select_none);
            } else {
                for (int i = 0; i < count; i++) {
                    mAdapter.getSelectedMap().put(i, false);
                    mListCheckStatus[i]= false;
                }
                checkboxText.setText(R.string.menu_selected_all);
            }
            IpNumberDeleteActivity.this.invalidateOptionsMenu();
            mAdapter.notifyDataSetChanged();
        }
    }

    private void saveAction() {
        String number;
        ArrayList<String> ipNumbers = new ArrayList<String>();
        mIpDialingUtils.setIpPreferenceNumber(FRIST_ITEM,mSubId);
        for (int i = 0; i < IP_PREFERENCE_NUM; i++) {
            if (mListCheckStatus[i] == true) {
                mIpDialingUtils.setIpNumber("", i,mSubId);
            }
        }

        for (int i = 0; i < IP_PREFERENCE_NUM; i++) {
            number = mIpDialingUtils.getIpNumber(i,mSubId);
            if (!TextUtils.isEmpty(number)) {
                ipNumbers.add(number);
                mIpDialingUtils.setIpNumber("", i,mSubId);
            }
        }

        int id = FRIST_ITEM;
        for (String s : ipNumbers) {
            if (null != mIpPreferenceNumber
                    && mIpPreferenceNumber.contentEquals(s)) {
                mIpDialingUtils.setIpPreferenceNumber(id,mSubId);
            }
            mIpDialingUtils.setIpNumber(s, id++,mSubId);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
            case MENU_OK:
                Intent intent = new Intent();
                saveAction();
                setResult(RESAULT_OK, intent);
                finish();
                break;
            case MENU_CANCLE:
                setResult(RESAULT_ERROR);
                finish();
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        menu.clear();
        menu.add(0, MENU_CANCLE, 0, R.string.cancel).setShowAsAction(
                MenuItem.SHOW_AS_ACTION_ALWAYS);

        menu.add(1, MENU_OK, 0, R.string.doneButton).setShowAsAction(
                MenuItem.SHOW_AS_ACTION_ALWAYS);

        if (!isCheckBoxSelected()) {
            menu.setGroupVisible(1, false);
        }

        return true;
    }

    boolean isCheckBoxSelected() {
        boolean bHaveSelected = false;
        if (mListCheckStatus != null) {
            for (int i = 0; i < IP_PREFERENCE_NUM; i++) {
                if (mListCheckStatus[i] == true) {
                    bHaveSelected = true;
                    break;
                }
            }
        }
        return bHaveSelected;
    }
}
