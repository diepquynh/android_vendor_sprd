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

public class cgversioninfo extends Activity {
    private static final String TAG = "cgversioninfo";

    private TextView txtViewlabel01;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.version);
        txtViewlabel01 = (TextView) findViewById(R.id.version_id);

        String cgversion = "T8861A_140519_22_user";
        txtViewlabel01.setText(cgversion);
    }

}

