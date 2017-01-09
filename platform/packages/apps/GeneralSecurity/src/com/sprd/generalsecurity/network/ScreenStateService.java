package com.sprd.generalsecurity.network;

import java.text.Collator;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;

import com.sprd.generalsecurity.utils.DateCycleUtils;
import com.sprd.generalsecurity.utils.TeleUtils;

import android.app.LoaderManager.LoaderCallbacks;
import android.content.BroadcastReceiver;
import android.app.Activity;
import android.app.AppGlobals;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.Loader;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.IPackageManager;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.UserManager;
import android.text.TextUtils;
import android.text.format.Formatter;
import android.util.Log;
import android.util.SparseArray;
import android.net.INetworkStatsService;
import android.net.INetworkStatsSession;
import android.net.NetworkStats;
import android.net.NetworkTemplate;
import android.net.TrafficStats;
import android.os.ServiceManager;
import android.preference.PreferenceManager;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import static android.net.NetworkTemplate.buildTemplateMobileAll;
import static android.net.NetworkTemplate.buildTemplateWifiWildcard;

import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.Contract;

public class ScreenStateService extends Service {
    private ScreenStateReceiver mScreenStateReceiver;

    public void onStart(Intent intent, int startId) {
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        if (!sharedPref.getBoolean(Contract.KEY_LOCK_DATA_SWITCH, false) &&
            !sharedPref.getBoolean(Contract.KEY_REAL_SPEED_SWITCH, false)) {
            return;
        }
        if (mScreenStateReceiver == null) {
            mScreenStateReceiver = new ScreenStateReceiver();
            mScreenStateReceiver.registerScreenReceiver(this);
        }
    }

    public void onDestroy() {
        if (mScreenStateReceiver != null) {
            mScreenStateReceiver.unRegisterScreenReceiver(this);
            mScreenStateReceiver = null;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
