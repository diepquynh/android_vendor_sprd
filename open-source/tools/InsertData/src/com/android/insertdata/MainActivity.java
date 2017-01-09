package com.android.insertdata;


import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;


import android.app.ListActivity;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.view.Menu;
import android.view.View;
import android.widget.ListView;
import android.widget.SimpleAdapter;

import com.android.insertdata.R;
import com.android.insertdata.addcall.AddCallActivity;
import com.android.insertdata.addmms.AddMMSActivity;
import com.android.insertdata.insertcontacts.InsertContactsActivity;
import com.android.insertdata.smstest.entity.SmsTestActivity;
public class MainActivity extends ListActivity {
//	private ListView listView;
//	private TextView mTextView01;
//	private TextView mTextView02;
//	private TextView mTextView03;
//
//	
//    @Override
//    public void onCreate(Bundle savedInstanceState){
//        super.onCreate(savedInstanceState);
//        setContentView(R.layout.main);
//        mTextView01 = (TextView)findViewById(R.id.call_title);
//        mTextView02 = (TextView)findViewById(R.id.sms_title);
//        mTextView03 = (TextView)findViewById(R.id.contacts_title);
//        
//        listView = new ListView(this);
//        listView.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_expandable_list_item_1,getData()));
//        setContentView(listView);
//        listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
//
//			public void onItemClick(AdapterView<?> arg0, View arg1,
//					int arg2, long arg3) {
//				if(arg2==0){
//					Intent intent = new Intent();
//					intent.setClass(MainActivity.this,AddCallActivity.class);
//					MainActivity.this.startActivity(intent);
//				}else if(arg2==1){
//					Intent intent = new Intent();
//					intent.setClass(MainActivity.this,InsertContactsActivity.class);
//					MainActivity.this.startActivity(intent);
//				}else if(arg2==2){
//					//smsSmsTestActivity
//					Intent intent = new Intent();
//					intent.setClass(MainActivity.this,SmsTestActivity.class);
//					MainActivity.this.startActivity(intent);
//				}
//			}
//		});
//    }
//    private List<String> getData(){
//        List<String> data = new ArrayList<String>();
//        data.add(mTextView01.getText().toString());
//        data.add(mTextView03.getText().toString());
//        data.add(mTextView02.getText().toString());     
//    return data;
//  }
	   @Override
	    public void onCreate(Bundle savedInstanceState) {
	        super.onCreate(savedInstanceState);
	        SimpleAdapter adapter = new SimpleAdapter(this,getData(),R.layout.main,
	        		                new String[]{"title","img"},
	        		                new int[]{R.id.title,R.id.img});
	        		        setListAdapter(adapter);
	        		    }
	        		 
	        		    private List<Map<String, Object>> getData() {
	        		    	
	        		        List<Map<String, Object>> list = new ArrayList<Map<String, Object>>();
	        			 
	        			        Map<String, Object> map = new HashMap<String, Object>();
	        			        map.put("title", this.getResources().getString(R.string.call_title));
	        			        map.put("img", R.drawable.ic_launcher_phone);
	        			        list.add(map);
	        			        map = new HashMap<String, Object>();
	        			        map.put("title", this.getResources().getString(R.string.contacts_title));
	        			        map.put("img", R.drawable.ic_launcher_shortcut_contact);
	        			        list.add(map);
	        			        map = new HashMap<String, Object>();
	        			        map.put("title", this.getResources().getString(R.string.sms_title));
	        			        map.put("img", R.drawable.ic_launcher_smsmms);
	        			        list.add(map);
                                            //add for mms
    	        			        map = new HashMap<String, Object>();
	        			        map.put("title", this.getResources().getString(R.string.mms_title));
	        			        map.put("img", R.drawable.ic_launcher_smsmms);
	        			        list.add(map);                                
//	        			        map = new HashMap<String, Object>();
//	        			        map.put("title", this.getResources().getString(R.string.help));
//	        			        map.put("img", R.drawable.service);
//	        			        list.add(map);
      
	        			        return list;
	        			    }
	        		    @Override
	        		    protected void onListItemClick(ListView l, View v, int position, long id) {
	        				if(position==0){
	        					Intent intent = new Intent();
	        					intent.setClass(MainActivity.this,AddCallActivity.class);
	        					MainActivity.this.startActivity(intent);
	        				}else if(position==1){
	        					Intent intent = new Intent();
	        					intent.setClass(MainActivity.this,InsertContactsActivity.class);
	        					MainActivity.this.startActivity(intent);
	        				}else if(position==2){
	        					//smsSmsTestActivity
	        					Intent intent = new Intent();
	        					intent.setClass(MainActivity.this,SmsTestActivity.class);
	        					MainActivity.this.startActivity(intent);
	        				}else if(position==3){//add for mms
	        					Intent intent = new Intent();
	        					intent.setClass(MainActivity.this,AddMMSActivity.class);
	        					MainActivity.this.startActivity(intent);
    	        				}else if(position==4){
	        					Intent intent = new Intent();
	        					intent.setClass(MainActivity.this,ServiceActivity.class);
	        					MainActivity.this.startActivity(intent);
	        				}
	        			}
	        		    
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		String appVersion = null;
		PackageManager manager = this.getPackageManager();
		try {
			PackageInfo info = manager.getPackageInfo(this.getPackageName(), 0);
			appVersion = info.versionName;
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
		if (null != appVersion){
			menu.add(0, 1, 1, "V" + appVersion);// 选项菜单用于显示版本号，方便维护（以当前日期作为版本号）
		}
		return super.onCreateOptionsMenu(menu);
	}
	       
}
