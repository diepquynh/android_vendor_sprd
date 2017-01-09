package com.android.stability;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

public class StabilityActivity extends Activity implements OnClickListener{
    /** Called when the activity is first created. */
	
	private Button btn_phoneInfo = null;
	private Button btn_singleItems = null;
	private Button btn_return = null;
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        btn_phoneInfo = (Button)findViewById(R.id.phone_infos);
        btn_singleItems = (Button)findViewById(R.id.single_items);
        btn_return = (Button)findViewById(R.id.btn_back);
        
        btn_phoneInfo.setOnClickListener(this);
        btn_singleItems.setOnClickListener(this);
        btn_return.setOnClickListener(this);
    }

	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		if(v.equals(btn_phoneInfo)){
			Intent iPhoneInfo = new Intent();
			iPhoneInfo.setClass(this, phoneInfos.class);
			startActivity(iPhoneInfo);
		}
		else if(v.equals(btn_singleItems)){
			Intent iSingleItems = new Intent();
			iSingleItems.setClass(this, singleItemsActivity.class);
			startActivity(iSingleItems);
		}
		else if(v.equals(btn_return)){
			System.exit(0);
		}
	}
}