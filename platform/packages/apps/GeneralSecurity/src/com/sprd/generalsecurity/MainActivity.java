package com.sprd.generalsecurity;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.UserHandle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import com.sprd.generalsecurity.network.FloatKeyView;

public class MainActivity extends Activity implements View.OnClickListener {
    FrameLayout f1, f2, f3, f4;

    private static final String ACTION_NETWORK_MANAGEMENT = "com.sprd.generalsecurity.network.dataflowmainentry";
    private static final String ACTION_MEMORY_MANAGEMENT = "com.sprd.generalsecurity.memory.MemoryManagement";
    private static final String ACTION_BATTERY_MANAGEMENT = "com.sprd.generalsecurity.battery.BatteryManagement";
    private static final String ACTION_STORAGE_MANAGEMENT = "com.sprd.generalsecurity.storage.StorageManagement";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.entry_main);
        setLayout();

        if (ActivityManager.getCurrentUser() != UserHandle.USER_SYSTEM) {
            PackageManager pm = getPackageManager();
            Toast.makeText(this,
                    getResources().getString(R.string.use_gs_in_owner),
                    Toast.LENGTH_LONG).show();
            pm.setComponentEnabledSetting(new ComponentName(MainActivity.this, MainActivity.class),
                PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                PackageManager.DONT_KILL_APP);
            finish();
            return;
        }
    }

    void setLayout() {
        f1 = (FrameLayout)findViewById(R.id.one);
        ImageView img = (ImageView)f1.findViewById(R.id.item_icon);
        img.setImageResource(R.drawable.memory);
        TextView tv = (TextView)f1.findViewById(R.id.item_name);
        tv.setText(R.string.memory);
        f1.setOnClickListener(this);

        f2 = (FrameLayout)findViewById(R.id.two);
        img = (ImageView)f2.findViewById(R.id.item_icon);
        img.setImageResource(R.drawable.garbage);
        tv = (TextView)f2.findViewById(R.id.item_name);
        tv.setText(R.string.storage);
        f2.setOnClickListener(this);

        f3 = (FrameLayout)findViewById(R.id.three);
        img = (ImageView)f3.findViewById(R.id.item_icon);
        img.setImageResource(R.drawable.battery);
        tv = (TextView)f3.findViewById(R.id.item_name);
        tv.setText(R.string.battery);
        f3.setOnClickListener(this);

        f4 = (FrameLayout)findViewById(R.id.four);
        img = (ImageView)f4.findViewById(R.id.item_icon);
        tv = (TextView)f4.findViewById(R.id.item_name);
        img.setImageResource(R.drawable.flow_management);
        tv.setText(R.string.flow);
        f4.setOnClickListener(this);
    }

    @Override
    protected void onResume() {
        super.onResume();

        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        if (sharedPref.getBoolean("networkspeed_switch", false)) {
            FloatKeyView.getInstance(this).startRealSpeed();
        }

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
//        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public void onClick(View v) {
        if (v == f4) {
            Intent it = new Intent(ACTION_NETWORK_MANAGEMENT);
            startActivity(it);
        } else if (v == f1) {
            Intent it = new Intent(ACTION_MEMORY_MANAGEMENT, null);
            startActivity(it);
        } else if (v == f3) {
            Intent it = new Intent(ACTION_BATTERY_MANAGEMENT, null);
            startActivity(it);
        } else if (v == f2) {
            Intent it = new Intent(ACTION_STORAGE_MANAGEMENT, null);
            startActivity(it);
        }
    }
}
