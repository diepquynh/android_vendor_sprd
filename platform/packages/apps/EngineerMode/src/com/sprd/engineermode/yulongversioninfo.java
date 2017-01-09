package com.sprd.engineermode;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.widget.TextView;
import android.os.SystemProperties;

public class yulongversioninfo extends Activity {
    private static final String TAG = "cgversioninfo";
    private TextView txtViewlabel01;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.version);
        txtViewlabel01 = (TextView) findViewById(R.id.version_id);
        //*#*#837866#*#*
        String cgversion = SystemProperties.get("ro.product.cg_version", "unknown");
        txtViewlabel01.setText(cgversion);
    }
}
