package com.sprd.generalsecurity.battery;

import java.util.ArrayList;
import java.util.List;

import com.android.internal.os.BatteryStatsHelper;
import com.android.internal.os.BatterySipper;

import android.os.BatteryStats;
import android.app.Activity;
import android.app.FragmentTransaction;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.UserManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.TextView;

import com.sprd.generalsecurity.R;

public class PowerUsageDetail extends Activity{

    private static final String TAG = "PowerUsageDetail";

    private Context mContext;
    private Bundle mArgs;
    private ImageView mIcon;
    private TextView mTitle;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.battery_item_detail);
        mContext = this;
        mArgs = getIntent().getBundleExtra("detail");
        if (mArgs == null) {
            finish();
            return;
        }
        ImageView mIcon = (ImageView) findViewById(R.id.icon);
        TextView mTitle = (TextView) findViewById(R.id.title);
        setupHeader(mArgs,mIcon,mTitle);

        FragmentTransaction fragmentTransaction = getFragmentManager()
                .beginTransaction();
        PowerUsageDetailFragment fmFragment = new PowerUsageDetailFragment();
        Log.i(TAG,"getbundle:"+mArgs);
        fmFragment.setArguments(mArgs);
        fragmentTransaction.add(R.id.content, fmFragment);
        fragmentTransaction.commitAllowingStateLoss();

        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setHomeButtonEnabled(true);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        onBackPressed();
        return true;
    }


    @Override
    protected void onResume() {
        super.onResume();
        setupHeader(mArgs,mIcon,mTitle);
    }

    private void setupHeader(Bundle args,ImageView icon,TextView tv) {
        String title = args.getString(PowerUsageDetailFragment.EXTRA_TITLE);
        String name = args.getString(PowerUsageDetailFragment.EXTRA_ICON_NAME);
        int iconId = args.getInt(PowerUsageDetailFragment.EXTRA_ICON_ID, 0);
        String pkg = args.getString(PowerUsageDetailFragment.EXTRA_ICON_PACKAGE);
        Drawable appIcon = null;

        if (!TextUtils.isEmpty(pkg)) {
            try {
                final PackageManager pm = mContext.getPackageManager();
                ApplicationInfo ai = pm.getPackageInfo(pkg, 0).applicationInfo;
                if (ai != null) {
                    appIcon = ai.loadIcon(pm);
                }
            } catch (NameNotFoundException nnfe) {
                // Use default icon
            }
        } else if (iconId != 0) {
            appIcon = mContext.getDrawable(iconId);
        }

        if (appIcon == null) {
            appIcon = mContext.getPackageManager().getDefaultActivityIcon();
        }

        if (icon != null) {
            icon.setBackground(appIcon);
        }
        if (tv !=null) {
            tv.setText(name);
        }
    }
}
