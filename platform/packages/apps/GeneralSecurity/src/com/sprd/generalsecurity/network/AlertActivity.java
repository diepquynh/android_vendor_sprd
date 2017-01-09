
package com.sprd.generalsecurity.network;

// Need the following import to get access to the app resources, since this
// class is in a sub-package.


import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.Contract;

import android.util.Log;
import android.content.Context;


public class AlertActivity extends Activity implements View.OnClickListener {

    private static final String WARNIN_MSG_MONTH = "msg_month";
    private static final String WARNIN_MSG_DAY = "msg_day";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Be sure to call the super class.
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_LEFT_ICON);

        setContentView(R.layout.alert);
        Intent it = getIntent();
        TextView text = (TextView)findViewById(R.id.text);
        getWindow().setFeatureDrawableResource(Window.FEATURE_LEFT_ICON,
                android.R.drawable.ic_dialog_alert);

        Button button = (Button)findViewById(R.id.ok);
        button.setOnClickListener(this);

        if (getIntent().getBooleanExtra(Contract.EXTRA_SIM_PROMPT, false)) {
            // prompt user reset sim related data
            text.setText(getResources().getString(R.string.sim_changed));
            return;
        }
        if (it.getIntExtra(Contract.EXTRA_ALERT_TYPE, 0) == Contract.ALERT_TYPE_MONTH) {
            text.setText(it.getStringExtra(WARNIN_MSG_MONTH).trim());
        } else if (it.getIntExtra(Contract.EXTRA_ALERT_TYPE, 0) == Contract.ALERT_TYPE_DAY) {
            text.setText(it.getStringExtra(WARNIN_MSG_DAY).trim());
        }
    }

    @Override
    public void onClick(View v) {
        finish();
    }
}
