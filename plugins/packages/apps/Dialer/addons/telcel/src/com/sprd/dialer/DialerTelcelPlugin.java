package com.sprd.dialer;

import android.app.AddonManager;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.telephony.SubscriptionInfo;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManagerEx;
import android.os.AsyncTask;
import android.util.Log;

import com.sprd.dialer.plugins.DialerTelcelHelper;
import com.android.dialer.util.DialerUtils;
import android.util.LruCache;

import java.util.Map;
import java.util.HashMap;

public class DialerTelcelPlugin extends DialerTelcelHelper
        implements AddonManager.InitialCallback {
    private static final String TAG = "DialerTelcelPlugin";
    private static final boolean DBG = true;
    static Map<Integer, LruCache<String, String>> mFdnListCache
            = new HashMap<Integer, LruCache<String, String>>();

    public DialerTelcelPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        log("clazz: " + clazz);
        return clazz;
    }

    @Override
    public String queryFdnCache(String number, int subId, String name) {
        String fdnName = null;
        if (number == null) {
            return null;
        }
        LruCache<String, String> cache = mFdnListCache.get(subId);
        if (cache != null && cache.size() > 0) {
            fdnName = cache.get(number);
        }
        log("queryFdnCache fdnName : " + fdnName + "   name :" + name);
        if (fdnName != null) {
            return fdnName;
        } else {
            return name;
        }
    }

    @Override
    public String queryFdnCacheForAllSubs(Context context, String number, String name) {
        String fdnName = null;
        if (number == null) {
            return null;
        }
        if (mFdnListCache.size() > 0) {
            TelephonyManager telephonyManager = (TelephonyManager)
                    context.getSystemService(Context.TELEPHONY_SERVICE);
            int phoneCount = telephonyManager.getPhoneCount();
            for (int i = 0; i < phoneCount; i++) {
                SubscriptionInfo subscriptionInfo =
                        DialerUtils.getActiveSubscriptionInfo(context, i, false);
                if (subscriptionInfo != null) {
                    int subId = subscriptionInfo.getSubscriptionId();
                    LruCache<String, String> cache = mFdnListCache.get(subId);
                    number = number.replace(" ","");
                    if (TelephonyManagerEx.from(context).getIccFdnEnabled(subId) && cache != null
                            && cache.size() > 0) {
                        fdnName = cache.get(number);
                        if (fdnName != null) {
                            return fdnName;
                        }
                    }
                }
            }
        }
        return name;
    }

    @Override
    public void queryFdnList(int subId, Context context) {
        QueryPreNumberTask mPreNumberAsyncTask = new QueryPreNumberTask(
                context, subId, true);
        mPreNumberAsyncTask.execute("");
    }

    @Override
    public void refreshFdnListCache(int subId, Context context) {
        LruCache<String, String> cache = mFdnListCache.get(subId);
        if (cache != null) {
            cache.evictAll();
        }
        mFdnListCache.remove(subId);
        notifyAllListeners();
        log("refresh FdnList Cache subId:" + subId);
    }

    /**
     * SPRD: FDN new feature @{
     */
    private class QueryPreNumberTask extends AsyncTask<Object, Object, String> {
        private Context context;
        private int subId;
        private boolean saveOrRemove;

        public QueryPreNumberTask(Context context, int subId,
                boolean saveOrRemove) {
            this.context = context;
            this.subId = subId;
            this.saveOrRemove = saveOrRemove;
        }

        @Override
        protected String doInBackground(Object... params) {
            String uri = "content://icc/fdn/subId/" + subId;
            String sdnName = "";
            String sdnNumber = "";
            String[] projection = { "name", "number" };
            Cursor cursor = null;
            LruCache<String, String> lastCache = mFdnListCache.get(subId);
            if (lastCache != null) {
                lastCache.evictAll();
            }
            mFdnListCache.remove(subId);
            LruCache<String, String> cache = new LruCache(10);
            try {
                cursor = context.getContentResolver().query(Uri.parse(uri),
                        projection, null, null, null);
                if (cursor != null && cursor.getCount() > 0) {
                    if (cursor.moveToFirst()) {
                        do {
                            sdnNumber = cursor.getString(cursor
                                    .getColumnIndexOrThrow("number"));
                            sdnName = cursor.getString(cursor
                                    .getColumnIndexOrThrow("name"));
                            if (saveOrRemove) {
                                if (sdnName.isEmpty()) {
                                    sdnName = sdnNumber;
                                }
                                log("put sdnNumber = " + sdnNumber + " sdnName = "
                                        + sdnName + " subId = " + subId);
                                cache.put(sdnNumber,sdnName);
                            }
                        } while (cursor.moveToNext());
                    }
                    mFdnListCache.put(subId, cache);
                }
            } catch (Exception ex) {
                ex.printStackTrace();
            } finally {
                if (null != cursor) {
                    cursor.close();
                    cursor = null;
                }
                DialerTelcelPlugin.this.notifyAllListeners();
            }
            return "";
        }

        @Override
        protected void onPostExecute(String result) {
        }
    }
    /** @} */

    private static void log(String msg) {
        if (DBG) Log.d(TAG, msg);
    }
}
