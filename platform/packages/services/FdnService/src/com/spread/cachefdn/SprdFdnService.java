package com.spread.cachefdn;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import android.R.integer;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.os.ServiceManager;
import android.provider.Settings;
import android.telephony.SmsManager;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.text.style.SubscriptSpan;

import com.android.internal.telephony.IFdnService;

public class SprdFdnService extends Service {
    private final static String TAG = "SpreadFdnService";
    private SubscriptionManager mSubscriptionManager;
    private TelephonyManager mTelephonyManager;
    private HashMap<Integer, FDNProcess> mHashMap;
    private ArrayList<Integer> mSubIdlist;
    private int mCurrentSubId;
    private int mCurrentSmsSubId;
    public FdnServiceSub mFdnServiceSub;

    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, " Enter onCreate()");

        mTelephonyManager = TelephonyManager.getDefault();
        mCurrentSubId = SubscriptionManager.getDefaultSubscriptionId();
        mSubIdlist = getSubIds(getActiveSubInfoList());

        Log.i(TAG, " Enter onCreate()----mCurrentSubId =[" + mCurrentSubId
                + "]");

        if (mSubIdlist != null && mSubIdlist.size() > 0) {
            Log.i(TAG,
                    " Enter onCreate()----mSubIds.size =[" + mSubIdlist.size()
                            + "]");
            mHashMap = new HashMap<Integer, FDNProcess>(mSubIdlist.size());
            for (int i = 0; i < mSubIdlist.size(); i++) {
                FDNProcess mFDNProcess = new FDNProcess(SprdFdnService.this,
                        String.valueOf(mSubIdlist.get(i)));
                mHashMap.put(mSubIdlist.get(i), mFDNProcess);
                Log.i(TAG,
                        "int process cache for sub id = [" + mSubIdlist.get(i)
                                + "]");
            }
        } else {
            Log.i(TAG, " Enter onCreate()---subidlist = null Or Zero]");
        }
        mFdnServiceSub = new FdnServiceSub();

    }

    private ArrayList<Integer> getSubIds(
            List<SubscriptionInfo> mSubscriptionInfolist) {
        if (mSubscriptionInfolist == null) {
            Log.i(TAG, "mSubscriptionInfolist == null");
            return null;
        }
        Iterator<SubscriptionInfo> iterator = mSubscriptionInfolist.iterator();
        if (iterator == null) {
            Log.i(TAG, "mSubscriptionInfolist's iterator== null");
            return null;
        }

        ArrayList<Integer> arrayList = new ArrayList<Integer>(
                mSubscriptionInfolist.size());
        while (iterator.hasNext()) {
            SubscriptionInfo subInfo = iterator.next();
            int phoneId = subInfo.getSimSlotIndex();
            arrayList.add(subInfo.getSubscriptionId());
        }
        return arrayList;
    }

    private FDNProcess getCache(int subid) {
        Log.i(TAG, " Enter getCache()");
        return mHashMap.get(subid);
    }

    @Override
    public void onStart(Intent intent, int startId) {
        super.onStart(intent, startId);
        Log.i(TAG, " Enter onStart()");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, " Enter onStartCommand()");
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public IBinder onBind(Intent arg0) {
        Log.i(TAG, " Enter onBind()");
        if (mFdnServiceSub != null) {
            return (IBinder) mFdnServiceSub;
        }
        return null;
    }

    @Override
    public void unbindService(ServiceConnection conn) {
        Log.i(TAG, " Enter unbindService()");
        super.unbindService(conn);
    }

    private class FdnServiceSub extends IFdnService.Stub {

        @Override
        public int process(int nCommand, long lParam,
                List<String> szValueslist, byte[] bytes) {

            checkSubId((int) (lParam));
            Object object = new Object();
            // Make sure the subId is active
            if (!isActiveSubId((int) (lParam))) {
                Log.i(TAG, "int process cache for subid = [" + ((int) (lParam))
                        + "]  is unactivity");
                return CommandDefine.PROCESS_IS_ERRO;
            }
            if (getCache((int) (lParam)) != null) {
                Log.i(TAG, "process() -->  getCache((int)(lParam)) != null ");
                Log.i(TAG, "process() -->  cache for subid = [" + lParam + "]");
                return (getCache((int) (lParam))).process(nCommand, lParam,
                        szValueslist, object, null);
            }
            return CommandDefine.PROCESS_IS_ERRO;
        }
    }

    private int checkSubId(int subId) {
        if (!SubscriptionManager.isValidSubscriptionId(subId)) {
            throw new RuntimeException(TAG + "Invalid subId " + subId);
        }
        if (subId == SubscriptionManager.DEFAULT_SUBSCRIPTION_ID) {
            return SubscriptionManager.getDefaultSubscriptionId();
        }
        return subId;
    }

    /*
     * @return true if the subId is active.
     */
    private boolean isActiveSubId(int subId) {
        return SubscriptionManager.from(SprdFdnService.this).isActiveSubId(
                subId);
    }

    /**
     * @return List<SubscriptionInfo>
     */
    private List<SubscriptionInfo> getActiveSubInfoList() {
        List<SubscriptionInfo> mInfos = new ArrayList<SubscriptionInfo>();
        // final Context context = ActivityThread.currentApplication()
        // .getApplicationContext();
        List<SubscriptionInfo> availableSubInfoList = SubscriptionManager.from(
                SprdFdnService.this).getActiveSubscriptionInfoList();
        if (availableSubInfoList == null) {
            return mInfos;
        }
        // mInfos = availableSubInfoList;
        // Iterator<SubscriptionInfo> iterator =
        // availableSubInfoList.iterator();
        // while (iterator.hasNext()) {
        // SubscriptionInfo subInfo = iterator.next();
        // int phoneId = subInfo.getSimSlotIndex();
        // boolean isSimReady = TelephonyManager.from(context).getSimState(
        // phoneId) == TelephonyManager.SIM_STATE_READY;
        // boolean isSimStandby = Settings.Global.getInt(
        // context.getContentResolver(), Settings.Global.SIM_STANDBY
        // + phoneId, 1) == 1;
        // if (!isSimStandby || !isSimReady) {
        // iterator.remove();
        // }
        // }
        return availableSubInfoList;
    }

}
