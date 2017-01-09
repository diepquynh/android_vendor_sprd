package com.sprd.generalsecurity.network;

import android.app.Activity;
import android.app.AppGlobals;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.app.LoaderManager.LoaderCallbacks;
import android.content.Context;
import android.content.Loader;
import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.content.SharedPreferences;
import android.graphics.drawable.Drawable;
import android.net.INetworkStatsService;
import android.net.INetworkStatsSession;
import android.net.NetworkStats;
import android.net.NetworkTemplate;
import android.net.TrafficStats;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.os.UserManager;
import android.preference.PreferenceManager;
import android.telephony.TelephonyManager;
import android.text.format.Formatter;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;

import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.DateCycleUtils;
import com.sprd.generalsecurity.utils.TeleUtils;

import java.lang.Override;
import java.lang.Throwable;
import java.text.Collator;
import java.util.ArrayList;
import java.util.Collections;
import static android.net.NetworkTemplate.buildTemplateMobileAll;
import static android.net.NetworkTemplate.buildTemplateWifiWildcard;

import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.os.SystemProperties;

import java.util.List;
import java.util.Date;

public class LockPeriodFlowActivity extends Activity {
    public static final String LOCKSCREEN_NETWORK_TYPE = "type";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.flow_rank);
        FragmentManager fm = getFragmentManager();
        FragmentTransaction ft = fm.beginTransaction();

        long t = System.currentTimeMillis();
        LockScreenFlowFragment fg = new LockScreenFlowFragment(getIntent().getIntExtra(LOCKSCREEN_NETWORK_TYPE, 0));
        ft.add(R.id.flow_rank_container, fg);
        ft.commit();
    }

    @Override
    public void onStop() {
        super.onStop();
        finish();
    }
}



