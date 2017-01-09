package com.android.modemassert;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public class AssertInfoActivity extends Activity{
    private TextView mAssertInfoTextView;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);

        mAssertInfoTextView = new TextView(this.getApplicationContext());

        String assertInfo = this.getIntent().getStringExtra("assertInfo");
        mAssertInfoTextView.setText(assertInfo);
        this.setContentView(mAssertInfoTextView);

    }
}
