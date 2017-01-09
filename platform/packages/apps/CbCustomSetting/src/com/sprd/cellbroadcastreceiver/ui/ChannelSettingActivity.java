package com.sprd.cellbroadcastreceiver.ui;

import java.util.ArrayList;
import java.util.HashMap;

import android.app.ActionBar;
import android.app.AlertDialog;
import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.content.Intent;
import android.content.Context;
import android.content.DialogInterface;
import android.view.View;
import android.view.Menu;
import android.widget.ListView;
import android.database.Cursor;
import android.provider.Telephony;
import android.view.MenuItem;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.View.OnCreateContextMenuListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.SimpleAdapter;
import android.view.LayoutInflater;
import android.database.sqlite.SQLiteDatabase;
import android.content.DialogInterface.OnClickListener;
import android.Manifest;
import android.widget.Toast;

import com.sprd.cellbroadcastreceiver.util.OsUtil;
import com.sprd.cellbroadcastreceiver.util.SystemAdapter;
import com.sprd.cellbroadcastreceiver.R;
import com.sprd.cellbroadcastreceiver.provider.ChannelTableDefine;
import com.sprd.cellbroadcastreceiver.util.Utils;
import com.sprd.cellbroadcastreceiver.ui.ChannelSettingEditActivity;
import com.sprd.cellbroadcastreceiver.data.ChannelItemData;
import com.sprd.cellbroadcastreceiver.data.ChannelListAdapter;
import com.sprd.cellbroadcastreceiver.data.ChannelMgr;
//import com.android.cellbroadcastreceiver.CellBroadcastDatabaseHelper;

public class ChannelSettingActivity extends Activity {

    private String TAG = "ChannelSettingActivity";
    private ListView mChannelList;
    private ChannelListAdapter mChannelAdapter;

    private SQLiteDatabase mBroadcastDb;
    private int mSubId;

    private ArrayList<ChannelItemData> mChannelArrayList;
    
    public static final int REQUEST_EDIT_CHANNEL = 1;
    // add for bug 577872 begin
    public static final int REQUEST_FINISH_CHANNEL = 2;
 // add for bug 577872 end
    public static final int RESULT_CHANGE_CHANNEL = 21;
    public static final int RESULT_DELETE_CHANNEL = 22;
    public static final int PADDING = 0XFFFF;
    //modify for bug 528208
    private boolean hasChanged = false;
    private boolean isUnexpectedClose = false;

    private int getSubId() {
        return mSubId;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.channel_setting);
        //SPRD:Bug#527751
        getActionBar().setDisplayUseLogoEnabled(false);
        getActionBar().setDisplayShowHomeEnabled(false);
        getActionBar().setDisplayOptions(
                ActionBar.DISPLAY_SHOW_TITLE | ActionBar.DISPLAY_HOME_AS_UP);

        Intent intent = getIntent();

        mSubId = intent.getIntExtra(ChannelTableDefine.SUB_ID, 0);
        log("the sub_id send in is:"+getSubId());

        mChannelList = (ListView) findViewById(R.id.lstChannel);
        LayoutInflater inflater = getLayoutInflater();
        View header = inflater.inflate(R.layout.cell_broadcast_set_header, null);
        mChannelList.addHeaderView(header, null, false);
        mChannelList.setOnItemClickListener(listItemClickListener);

        //reflashChannelList();
    }

    private AdapterView.OnItemClickListener listItemClickListener = new AdapterView.OnItemClickListener() {
        public void onItemClick(AdapterView<?> parent, View view,
                int position, long id) {
            if (view != null) {
                ChannelItemData itemData = (ChannelItemData) getChannelAdapter().getItem(position-1);
                Intent intent = new Intent(ChannelSettingActivity.this, ChannelSettingEditActivity.class);

                intent.putExtra(Utils.OPERATION, Utils.OPERATION_EDIT);
                intent.putExtra(Utils.INDEXOFARRAY, itemData.getIndexOfArray());
                intent.putExtra(ChannelTableDefine.SUB_ID, getSubId());
                log(itemData.toString());

                startActivityForResult(intent, REQUEST_EDIT_CHANNEL);
            }
        }
    };

    private void reflashChannelList(){
        
		try {
		    if(mChannelAdapter == null){
		        //modify for bug 532105
	            //modify for bug 543161 begin
		        ChannelMgr.getInstance().init(getApplicationContext(), getSubId(), isUnexpectedClose);
	            //modify for bug 543161 end
		        log("--reflashChannelList--the size of the Mgr is:"+ChannelMgr.getInstance().size());
		        mChannelArrayList = new ArrayList<ChannelItemData>();
		        ChannelMgr.getInstance().filterDelRecord(mChannelArrayList);
	            mChannelAdapter = new ChannelListAdapter(this, getDataSource());
	            mChannelList.setAdapter(mChannelAdapter);
		    }else{
		        log("before reflash, the ArrayList's size is:"+ mChannelArrayList.size());
		        ChannelMgr.getInstance().filterDelRecord(mChannelArrayList);
		        log("after reflash, the ArrayList's size is:"+ mChannelArrayList.size());
		        mChannelAdapter.notifyDataSetChanged();
		    }
		} catch (Exception e) {
			e.printStackTrace();
		}
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.channel_setting_menu, menu);
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        if (getChannelAdapter() != null && getChannelAdapter().getCount() != 0) {
            menu.findItem(R.id.del_all).setEnabled(true);
        }else{
            menu.findItem(R.id.del_all).setEnabled(false);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        log("--onOptionsItemSelected--");
        switch(item.getItemId()){
        case R.id.add:
            log("--onOptionsItemSelected--option ADD.");
            Intent intent = new Intent(this,ChannelSettingEditActivity.class);

            intent.putExtra(ChannelTableDefine.SUB_ID, getSubId());
            intent.putExtra(Utils.OPERATION, Utils.OPERATION_ADD);
            startActivityForResult(intent, REQUEST_EDIT_CHANNEL);
            return true;
        case R.id.del_all:
            log("--onOptionsItemSelected--option DELTE ALL.");
            int size = getChannelAdapter().getCount();
            for (int i = 0; i < size; i++) {
                ChannelItemData itemData = (ChannelItemData)getChannelAdapter().getItem(i);
                if (itemData.getIndexOfArray() > -1) {
                    ChannelMgr.getInstance().deleteDataByIndex(itemData.getIndexOfArray());
                }
            }
            getDataSource().clear();
            getChannelAdapter().notifyDataSetChanged();
            //modify for bug 533479
            hasChanged = true;

            return true;
        case R.id.save:
            log("--onOptionsItemSelected--option APPLY: sync DB and send AT command.");
            if (OsUtil.hasPermission(ChannelSettingActivity.this,
                    Manifest.permission.RECEIVE_SMS)) {
                Thread t1 = new Thread(new Runnable() {
                    public void run() {
                        Log.d(TAG, "=============SaveDataToDB====1===");
                        ChannelMgr.getInstance().SaveDataToDB();
                    }
                }, "SaveDataToDB");
                t1.start();
            } else {
                Log.d(TAG, "=============SaveDataToDB====2===");
                Toast.makeText(ChannelSettingActivity.this,
                        R.string.requre_permission,
                        Toast.LENGTH_SHORT).show();
            }
            ChannelSettingActivity.this.finish();
            return true;
        //SPRD:Bug#527751
        case android.R.id.home:
            //SPRD:Bug#531838
            if (hasChanged) {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setTitle(R.string.alert);
                builder.setIconAttribute(android.R.attr.alertDialogIcon);
                builder.setCancelable(true);
                builder.setMessage(R.string.remind_save);
                builder.setPositiveButton(android.R.string.ok, clearChangeListener);
                builder.setNegativeButton(R.string.no, null);
                builder.show();
            } else {
                //ChannelMgr.releaseInstance();
                finish();
            }
            break;
        }
        return false;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (Utils.DEBUG) {
            log("onActivityResult: requestCode=" + requestCode + "+data=" + data);
        }
        switch (resultCode) {
        case RESULT_DELETE_CHANNEL:
        case RESULT_CHANGE_CHANNEL:
            //modify for bug 528208
            hasChanged = true;
            reflashChannelList();
            break;
        // add for bug 577872 begin
        case REQUEST_FINISH_CHANNEL:
            finish();
            break;
        // add for bug 577872 end
        default:
            break;
        }
    }

    private ChannelListAdapter getChannelAdapter(){
        return mChannelAdapter;
    }

    private ArrayList<ChannelItemData> getDataSource() {
        return mChannelArrayList;
    }

    //modify for bug 528208
    @Override
    public void onBackPressed() {
        if (hasChanged) {
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setTitle(R.string.alert);
            builder.setIconAttribute(android.R.attr.alertDialogIcon);
            builder.setCancelable(true);
            builder.setMessage(R.string.remind_save);
            builder.setPositiveButton(android.R.string.ok, clearChangeListener);
            builder.setNegativeButton(R.string.no, null);
            builder.show();
        } else {
            //ChannelMgr.releaseInstance();
            ChannelSettingActivity.this.finish();
        }
    }

    private OnClickListener clearChangeListener = new OnClickListener() {
        public void onClick(DialogInterface dialog, int which) {
            //ChannelMgr.releaseInstance();
            ChannelSettingActivity.this.finish();
        }
    };

    private OnClickListener cancelClick = new OnClickListener() {
        public void onClick(DialogInterface dialog, int which) {
            //ChannelMgr.releaseInstance();
            ChannelSettingActivity.this.finish();
        }
    };

    //add for bug 543161 begin
    @Override
    protected void onResume() {
        super.onResume();
        reflashChannelList();

        /* Add by SPRD for bug 571686 Start */
        if (!OsUtil.hasRequiredPermissions(this)) {
            OsUtil.requestMissingPermission(this);
        }
        /* Add by SPRD for bug 571686 End */
    }

    /* Add by SPRD for bug 571686 Start */
    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        for (int i : grantResults) {
            if (android.content.pm.PackageManager.PERMISSION_DENIED == i) {
                finish();
            }
        }
    }
    /* Add by SPRD for bug 571686 End */

    @Override
    protected void onSaveInstanceState(Bundle outState){
        super.onSaveInstanceState(outState);
        Log.d("ran1", "---onSaveInstance---");
        outState.putBoolean("hasChange", hasChanged);
        outState.putBoolean("unexpected_close", true);
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState){
        super.onRestoreInstanceState(savedInstanceState);
        Log.d("ran1", "---onRestoreInstance---");
        hasChanged = savedInstanceState.getBoolean("hasChange", false);
        isUnexpectedClose = savedInstanceState.getBoolean("unexpected_close", false);
    }
    //add for bug 543161 end

    @Override
    protected void onDestroy() {
        super.onDestroy();
        //add for bug 530800
        //ChannelMgr.releaseInstance();
    }

    private void log(String string){
        Log.d(TAG, string);
    }
}

