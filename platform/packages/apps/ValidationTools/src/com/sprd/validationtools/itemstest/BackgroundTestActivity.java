package com.sprd.validationtools.itemstest;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.Const;
import com.sprd.validationtools.R;
import com.sprd.validationtools.engtools.BtTestUtil;
import com.sprd.validationtools.engtools.WifiTestUtil;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.location.GpsSatellite;
import android.location.GpsStatus;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Button;
import android.view.View.OnClickListener;
import android.widget.Toast;

public class BackgroundTestActivity extends BaseActivity {
	private static final String TAG = "BackgroundTestActivity";

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.background_test);

		TextView resultView = (TextView) findViewById(R.id.bg_test_result);
		resultView.setText(getIntent().getStringExtra(
				Const.INTENT_BACKGROUND_TEST_RESULT));
		Button btn = (Button) findViewById(R.id.btn_retest);
		btn.setOnClickListener(new OnClickListener(){

			@Override
			public void onClick(View arg0) {
			    finish();
			}
		});

        super.removeButton();
	}

	@Override
	public void onBackPressed() {
		finish();
	}
}
