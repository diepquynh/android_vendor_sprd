package com.android.modemassert;

import android.os.Bundle;
import android.util.Log;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;


public class SystemInfoDumpingActivity  extends Activity{
    private ProgressDialog mProgressDialog;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initFromIntent(getIntent());
    }
    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        initFromIntent(intent);
    }
    private void initFromIntent(Intent intent) {
        if (intent != null) {
            boolean closeflag = intent.getBooleanExtra("closeFlag", false);
            if (closeflag) {
                finish();
            }else {
                mProgressDialog = new ProgressDialog(this);
                mProgressDialog.setTitle(getText(R.string.dumping_title));
                mProgressDialog.setIndeterminate(true);
                mProgressDialog.setCancelable(false);
                mProgressDialog.setMessage(getText(R.string.dumping_message));
                mProgressDialog.show();
            }
        }
    }
}
