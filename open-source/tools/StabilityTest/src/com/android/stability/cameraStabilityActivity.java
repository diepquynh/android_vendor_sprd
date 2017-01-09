package com.android.stability;

import android.app.Activity;
import android.content.Intent;
import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;

public class cameraStabilityActivity extends Activity implements
		OnClickListener {
	public static final String CAMERA_SELECT_TYPE = "Camera_Selected";
	public static final String CAMERA_REPEAT_TIMES = "Camera_Repeat_Times";
	public static final String CAMERA_TEST_ITEM = "Camera_Test_Item";
	
	//相机选择
	private RadioGroup camera_item;
	private RadioButton back_camera;
	private RadioButton front_camera;
	private int camera_id;
	//循环次数选择
	private EditText mtestTimes;
	//测试项选择。
	private RadioGroup camera_test_item;
	private RadioButton repeatPhotobtn;
	private RadioButton cameraSwitchbtn;
	private RadioButton cameraOpenOffbtn;
	//确定和返回项
	private Button ok_button;
	private Button back_button;
	//摄像头个数
	final int cameranum = Camera.getNumberOfCameras();
	private Boolean isHaveFrontCamera = true;

	public static final int RepeateTest = 1;
	public static final int ChangeCameraTest = 2;
	public static final int PoweroffTest = 3;

	public static final int DefaultTestTimes = 200;
	
    private int mTestType = 0;
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.cameratest_layout);
		//计算camera的数目
		interfaceInit();
	}
	
	private void interfaceInit(){
		//判断是否有后摄像头。//判断各个选项是否已经选择。
		
		camera_item = (RadioGroup)findViewById(R.id.camera_select);
		back_camera = (RadioButton)findViewById(R.id.main_back_camera);
		front_camera = (RadioButton)findViewById(R.id.sub_front_camera);
		//判断是否有前摄像头
		if(cameranum < 1){
			front_camera.setVisibility(View.INVISIBLE);
			isHaveFrontCamera = false;
		}else{
			front_camera.setVisibility(View.VISIBLE);
			isHaveFrontCamera = true;
		}
		camera_item.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
			
			@Override
			public void onCheckedChanged(RadioGroup group, int checkedId) {
				// TODO Auto-generated method stub
				if(checkedId == back_camera.getId()){
					camera_id = 0;
				}else if(isHaveFrontCamera==true&&checkedId == front_camera.getId()){
					camera_id = 1;
				}
					
			}
		});
		//这块暂时不知道要写啥。
		mtestTimes = (EditText)findViewById(R.id.camera_repeat_times);
		mtestTimes.addTextChangedListener(new TextWatcher() {
			
			@Override
			public void onTextChanged(CharSequence s, int start, int before, int count) {
				// TODO Auto-generated method stub
				
			}
			
			@Override
			public void beforeTextChanged(CharSequence s, int start, int count,
					int after) {
				// TODO Auto-generated method stub
				
			}
			
			@Override
			public void afterTextChanged(Editable s) {
				// TODO Auto-generated method stub
				
			}
		});
		
		camera_test_item = (RadioGroup)findViewById(R.id.camera_test_item);
		repeatPhotobtn = (RadioButton)findViewById(R.id.camera_takephoto_repeat_test);
		cameraSwitchbtn = (RadioButton)findViewById(R.id.camera_switch_test);
		cameraOpenOffbtn = (RadioButton)findViewById(R.id.camera_open_off_test);
		if(isHaveFrontCamera){
			cameraSwitchbtn.setVisibility(View.VISIBLE);
		}else{
			cameraSwitchbtn.setVisibility(View.INVISIBLE);
		}
		camera_test_item.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
			
			@Override
			public void onCheckedChanged(RadioGroup group, int checkedId) {
				// TODO Auto-generated method stub
				if(checkedId == repeatPhotobtn.getId()){
					mTestType = cameraStabilityActivity.RepeateTest;
				}else if(checkedId == cameraSwitchbtn.getId()){
					mTestType = cameraStabilityActivity.ChangeCameraTest;
				}else if(checkedId == cameraOpenOffbtn.getId()){
					mTestType = cameraStabilityActivity.PoweroffTest;
				}
			}
		});
		
		ok_button = (Button)findViewById(R.id.ok_button);
		back_button = (Button)findViewById(R.id.back_button);
		ok_button.setOnClickListener(this);
		back_button.setOnClickListener(this);
	}
	
	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		if(v.equals(ok_button)){
			Intent i = new Intent();
			i.putExtra(CAMERA_SELECT_TYPE, camera_id);
			i.putExtra(CAMERA_REPEAT_TIMES, mtestTimes.getText());
			i.putExtra(CAMERA_TEST_ITEM, mTestType);
			i.setClass(this, CameraTestActivity.class);
			startActivity(i);
			
		}else if(v.equals(back_button)){
			finish();
		}
	}

}
