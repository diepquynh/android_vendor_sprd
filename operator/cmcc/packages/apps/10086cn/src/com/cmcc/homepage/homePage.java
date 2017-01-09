package com.cmcc.homepage;


import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

public class homePage extends Activity {
    /** Called when the activity is first created. */
	private static final String url= "http://10086.cn/m/";
    @Override
    public void onCreate(Bundle savedInstanceState) {
    	super.onCreate(savedInstanceState);
    	setContentView(R.layout.main);
        Uri uri=Uri.parse(url);
        Intent intent = new Intent(Intent.ACTION_VIEW, uri);
        startActivity(intent);
        finish();
    }
}
