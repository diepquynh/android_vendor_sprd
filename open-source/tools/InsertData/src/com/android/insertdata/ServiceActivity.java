package com.android.insertdata;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

import com.android.insertdata.R;
public class ServiceActivity extends Activity {
	private TextView companyName;
	private TextView companyAddress;
	private TextView zipCode;
	private TextView website;
	private TextView serviceHotline;
	private TextView emailCustomerService;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.services);
		companyName = (TextView)findViewById(R.id.companyname);
		
		companyAddress =  (TextView)findViewById(R.id.companyaddress);
		zipCode =  (TextView)findViewById(R.id.zipcode);
		website =  (TextView)findViewById(R.id.website);
		serviceHotline =  (TextView)findViewById(R.id.servicehotline);
		emailCustomerService = (TextView) findViewById(R.id.emailcustomerservice);
		
	}

}
