package com.android.stability;

import com.android.internal.telephony.PhoneFactory;

import android.app.Activity;
import android.content.Context;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.View.OnClickListener;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class phoneInfos extends Activity implements OnClickListener{

	public TextView soft_version_str = null;
	public TextView barcode_str = null;
	public TextView imei_str = null;
	private Button back_button = null;
	private engfetch mEf = null;
	private String mText = null;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.phoneinfo);
		
		soft_version_str = (TextView)findViewById(R.id.soft_version_str);
		barcode_str = (TextView)findViewById(R.id.barcode_str);
		imei_str = (TextView)findViewById(R.id.imei_str);
		back_button = (Button)findViewById(R.id.test_report_back_button);
		
		back_button.setOnClickListener(this);
		
		//display wingtech inner softversion
		//if(Build.VERSION_INNER_NUMBER != null){
		//	soft_version_str.setText(Build.VERSION_INNER_NUMBER);
		//}
		//else{
			soft_version_str.setText(R.string.no_test_report);
		//}
		
		//display SN
		mEf = new engfetch();
		int dataSize = 2048;
		byte[] inputBytes = new byte[dataSize];
		int showlen= mEf.enggetphasecheck(inputBytes, dataSize);
		mText = new String(inputBytes, 0, showlen);
		if(mText != null){
			barcode_str.setText(mText);
		}
		else{
			barcode_str.setText(R.string.no_test_report);
		}
		
		//display IMEI
		TelephonyManager tm = (TelephonyManager)this.getSystemService(TELEPHONY_SERVICE);
		int phoneCnt = PhoneFactory.getPhoneCount();
		StringBuffer imeiBuffer = new StringBuffer();
		
		if(phoneCnt == 0)
		{
			//single SIM
			imeiBuffer.append(tm.getDeviceId());
		}
		else
		{
			Log.d("phoneinfo", "getphoneCnt=" + phoneCnt);
			//dual SIM
			for(int i = 0; i < phoneCnt; i++)
			{
				if(i != 0)
        		{
        			imeiBuffer.append("\n");
        		}
        		imeiBuffer.append("IMEI");
                imeiBuffer.append((i + 1));
                imeiBuffer.append("\n");
                imeiBuffer.append(((TelephonyManager)getSystemService(PhoneFactory
                		.getServiceName(Context.TELEPHONY_SERVICE, i))).getDeviceId());
			}
		}
		
		String imeiStr = imeiBuffer.toString();
		if((imeiStr != null) && (imeiStr.length() > 0))
        {
        	imei_str.setText(imeiStr);
        }
        else
        {
        	imei_str.setText(R.string.no_test_report);
        }
	}
	
	public void onClick(View v) {
		// TODO Auto-generated method stub
		if(v.equals(back_button)){
			finish();
		}
	}

}
