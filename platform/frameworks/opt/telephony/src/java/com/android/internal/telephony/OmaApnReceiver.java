
package com.android.internal.telephony;

import android.app.Activity;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.IntentFilter;
import android.text.TextUtils;
import android.util.Log;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Telephony;
import android.provider.Telephony;

import java.security.Key;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

import android.os.Bundle;
import android.os.Message;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.widget.Toast;

import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;

public class OmaApnReceiver extends BroadcastReceiver {

    static final String INTENT_APN_DATA_CONFIG = "com.android.ApnDataConfig";
    static final String SUBSCRIPTION_ID = "subid";
    static final boolean DBG = true;
    static final String TAG = "OmaApnReceiver";
    static final String[] bundleTypes = { "apn", "mms", "bootstarp" };
    public static int RECEIVE_FLAG_APN = 0x00000001;

    private int mSubId;
    private Context mContext;
    private Intent mIntent;
    private boolean hasOverlayMms = true;

    private HashMap<String,String> mOriginalApnMap = new HashMap<String,String>();

    private static final int ID_INDEX = 0;
    private static final int NAME_INDEX = 1;
    private static final int APN_INDEX = 2;
    private static final int TYPES_INDEX = 3;
    private static final int MMSC_INDEX = 4;
    private static final int MMSPROXY_INDEX = 5;
    private static final int MMSPORT_INDEX = 6;

    private static final int VALUE_MAX_PORT = 65535;
    private static final int VALUE_DEFAULT_PORT = 80;

    public OmaApnReceiver(Context context) {
        super();
        mContext = context;
        Log.d(TAG, "mContext = " + mContext);
        IntentFilter filter = new IntentFilter();
        filter.addAction("com.android.ApnDataConfig");
        mContext.registerReceiver(this, filter);
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        hasOverlayMms = true;
        String action = intent.getAction();
        if (DBG)
            Log.d(TAG, "onReceive: action=" + action);
        mIntent = intent;
        if (action.equals(INTENT_APN_DATA_CONFIG)) {
            mSubId = intent.getIntExtra(SUBSCRIPTION_ID, -1);
            Log.d(TAG, "onReceive: mSubId=" + mSubId);
            String numeric = TelephonyManager.from(mContext).getSimOperator(mSubId);
            StringBuilder where = new StringBuilder();
            where.append(Telephony.Carriers.NUMERIC).append(" = '").append(numeric).append("'");
            Cursor cursor = mContext.getContentResolver().query(Telephony.Carriers.CONTENT_URI, new String[] { "_id" },
                    where.toString(), null, null);
            try {
                if (cursor != null && cursor.getCount() > 0) {
                    if (cursor.moveToFirst()) {
                        do {
                            mOriginalApnMap.put(cursor.getString(ID_INDEX),
                                    "haveNotOverlay");
                        } while (cursor.moveToNext());
                    }
                }
            } catch (Exception e) {
                Log.d(TAG,"operation of database exception!",e);
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }
            Log.d(TAG,"onReceive: mOriginalApnMap = " + mOriginalApnMap);
            ProcessIntent(intent);
            if (mOriginalApnMap != null || mOriginalApnMap.size() > 0) {
                Log.d(TAG, "Clear mOriginalApnMap data!");
                mOriginalApnMap.clear();
            }
            setResultCode(RECEIVE_FLAG_APN);
        }
    }

    void ProcessIntent(Intent intent) {
        int apnSize = intent.getIntExtra("apn_size", 0);
        int bootstarpSize = intent.getIntExtra("bootstarp_size", 0);
        for (int i = 1; i <= apnSize; i++) {
            Log.d(TAG,
                    "apn" + i + " bundle: "
                            + ((Bundle) intent.getExtra("apn" + i)).toString());
            ProcessBundle((Bundle) intent.getExtra("apn" + i), bundleTypes[0]);
        }
        // for (int i = 1; i <= mmsSize; i++) {
        // Log.d(TAG, "mms" + i + " bundle: "
        // + (Bundle) intent.getExtra("mms" + i));
        // ProcessBundle((Bundle) intent.getExtra("mms" + i), bundleTypes[1]);
        // }
        for (int i = 1; i <= bootstarpSize; i++) {
            Log.d(TAG,
                    "bootstarp" + i + " bundle: "
                            + (Bundle) intent.getExtra("bootstarp" + i));
            ProcessBundle((Bundle) intent.getExtra("bootstarp" + i),
                    bundleTypes[2]);
        }
    }

    void ProcessBundle(Bundle bundle, String bundleType) {
        if (bundle != null) {
            // if (null != bundle.getString("napdef_name")) {
            // for (BootStarp bootstartp : config.bsList) {
            // Log.d(TAG,
            // " OtaOmaApnConfigVO.NAME="+config.getValue(OtaOmaApnConfigVO.NAME,true)
            // + "   bootstartp.name="+bootstartp.name);
            // if
            // (config.getValue(OtaOmaApnConfigVO.NAME,true).equals(bootstartp.name))
            // setting.dropApn(context, bootstartp, mSubId);
            // }
            String apn = bundle.getString("nap-address");
            Log.i(TAG, "apn == " + apn);
            if (bundle.getString("nap-address") != null
                    && !TextUtils.isEmpty(bundle.getString("nap-address"))
                    && mSubId != -1) {
                int subId = mSubId;
                String numeric = null;
                String mcc = null;
                String mnc = null;
                TelephonyManager tm = TelephonyManager.from(mContext);
                if (tm.getSimState(SubscriptionManager.getPhoneId(subId)) == TelephonyManager.SIM_STATE_READY) {
                    numeric = tm.getSimOperator(subId);
                    mcc = numeric.substring(0, 3);
                    mnc = numeric.substring(3);
                }
                Uri uri = Telephony.Carriers.CONTENT_URI;
                Log.d(TAG, "ProcessBundle: bundleType = " + bundleType
                        + "  subId = " + subId + "  numeric = " + numeric);
                ContentValues contentValues = new ContentValues();
                validAndPut(contentValues, Telephony.Carriers.NUMERIC, null,
                        numeric);
                validAndPut(contentValues, Telephony.Carriers.MCC, null, mcc);
                validAndPut(contentValues, Telephony.Carriers.MNC, null, mnc);
                validAndPut(contentValues, Telephony.Carriers.APN, bundle,
                        "nap-address");

                Log.d(TAG,
                        "key = authtype , value = "
                                + bundle.getString("authtype"));
                if (bundle.getString("authtype") == null) {
                    contentValues.put(Telephony.Carriers.AUTH_TYPE, 0);
                } else if (bundle.getString("authtype").equals("PAP")) {
                    contentValues.put(Telephony.Carriers.AUTH_TYPE, 1);
                } else if (bundle.getString("authtype").equals("CHAP")) {
                    contentValues.put(Telephony.Carriers.AUTH_TYPE, 2);
                } else {
                    contentValues.put(Telephony.Carriers.AUTH_TYPE, 3);
                }
                contentValues.put(Telephony.Carriers.CURRENT, 1);
                contentValues.put(Telephony.Carriers.SUBSCRIPTION_ID, subId);

                int appSize = mIntent.getIntExtra("app_size", 0);
                Log.d(TAG, "appSize= " + appSize);
                if (appSize > 0) {
                    for (int i = 1; i <= appSize; i++) {
                        Bundle appBundle = (Bundle) mIntent.getExtra("app" + i);
                        Log.d(TAG, "[appBundle" + i + "]=[" + appBundle.toString() + "]");
                        if (appBundle.getString("to-proxy") != null) {
                            if (appBundle.getString("to-proxy").equals(
                                    bundle.getString("proxy-id"))) {
                                Log.d(TAG, "appBundle" + i + " to-proxy = "
                                        + appBundle.getString("to-proxy")
                                        + " match" + " apnbundle proxy-id = "
                                        + bundle.getString("proxy-id")
                                        + " success");
                                if (appBundle.getString("appid").equals("w4")) {
                                    // APN type mms
                                    Log.d(TAG,
                                            "appBundle appid = "
                                                    + appBundle.getString("appid")
                                                    + " turn to mms type");
                                    putContentValuesNull(contentValues);
                                    validAndPut(contentValues,
                                            Telephony.Carriers.TYPE, null,
                                            "mms");
                                    validAndPut(contentValues,
                                            Telephony.Carriers.MMSC, appBundle,
                                            "addr");
                                    validAndPut(contentValues,
                                            Telephony.Carriers.MMSPROXY,
                                            bundle, "pxaddr");
                                    validAndPut(contentValues,
                                            Telephony.Carriers.MMSPORT, bundle,
                                            "portnbr");
                                    if (bundle.getString("napdef_name") != null) {
                                        validAndPut(contentValues,
                                                Telephony.Carriers.NAME,
                                                bundle, "napdef_name");
                                    } else {
                                        validAndPut(contentValues,
                                                Telephony.Carriers.NAME,
                                                appBundle, "application_name");
                                    }
                                    if (bundle.getString("pxauth-id") != null) {
                                        validAndPut(contentValues,
                                                Telephony.Carriers.USER,
                                                bundle, "pxauth-id");
                                    } else {
                                        validAndPut(contentValues,
                                                Telephony.Carriers.USER,
                                                bundle, "authname");
                                    }
                                    if (bundle.getString("pxauth-pw") != null) {
                                        validAndPut(contentValues,
                                                Telephony.Carriers.PASSWORD,
                                                bundle, "pxauth-pw");
                                    } else {
                                        validAndPut(contentValues,
                                                Telephony.Carriers.PASSWORD,
                                                bundle, "authsecret");
                                    }
                                    putValuesToDatabase(contentValues, numeric,
                                            uri);
                                } else {
                                    // APN type default
                                    Log.d(TAG,
                                            "appBundle appid = "
                                                    + appBundle.getString("appid")
                                                    + " turn to default type");
                                    putDefaultApnValues(bundle, contentValues,
                                            numeric, uri);
                                }
                            } else {
                                Log.d(TAG, "appBundle" + i + " to-proxy = "
                                        + appBundle.getString("to-proxy")
                                        + " match" + " apnbundle proxy-id = "
                                        + bundle.getString("proxy-id")
                                        + " failed");
                            }
                        } else if (appBundle.getString("to-napid") != null) {
                            if (appBundle.getString("to-napid").equals(
                                    bundle.getString("napid"))) {
                                Log.d(TAG, "appBundle" + i + " to-napid = "
                                        + appBundle.getString("to-napid")
                                        + " match" + " apnbundle napid = "
                                        + bundle.getString("napid") + " sucess");
                                if (appBundle.getString("appid").equals("w4")) {
                                    // APN type mms
                                    Log.d(TAG,
                                            "appBundle appid = "
                                                    + appBundle.getString("appid")
                                                    + " turn to mms type");
                                    putContentValuesNull(contentValues);
                                    validAndPut(contentValues,
                                            Telephony.Carriers.TYPE, null,
                                            "mms");
                                    validAndPut(contentValues,
                                            Telephony.Carriers.MMSC, appBundle,
                                            "addr");
                                    validAndPut(contentValues,
                                            Telephony.Carriers.USER, bundle,
                                            "authname");
                                    validAndPut(contentValues,
                                            Telephony.Carriers.PASSWORD,
                                            bundle, "authsecret");
                                    if (bundle.getString("napdef_name") != null) {
                                        validAndPut(contentValues,
                                                Telephony.Carriers.NAME,
                                                bundle, "napdef_name");
                                    } else {
                                        validAndPut(contentValues,
                                                Telephony.Carriers.NAME,
                                                appBundle, "application_name");
                                    }
                                    putValuesToDatabase(contentValues, numeric,
                                            uri);
                                } else {
                                    // APN type default
                                    Log.d(TAG,
                                            "appBundle appid = "
                                                    + appBundle.getString("appid")
                                                    + " turn to default type");
                                    putDefaultApnValues(bundle, contentValues,
                                            numeric, uri);
                                }
                            } else {
                                Log.d(TAG, "appBundle" + i + " to-napid = "
                                        + appBundle.getString("to-napid")
                                        + " match" + " apnbundle napid = "
                                        + bundle.getString("napid") + " failed");
                            }
                        } else {
                            Log.d(TAG,
                                    "something wrong with appbundle,there must have one of to-proxy and to-napid");
                        }
                    }
                } else {
                    // there is no appBundle,turn apnBundle to default APN
                    putDefaultApnValues(bundle, contentValues, numeric, uri);
                }
            }
        }
    }

    public void putDefaultApnValues(Bundle apnBundle, ContentValues contentValues, String numeric,
            Uri uri) {
        Log.d(TAG, "putDefaultApnValues ");
        putContentValuesNull(contentValues);
        validAndPut(contentValues, Telephony.Carriers.TYPE, null,
                "default");
        validAndPut(contentValues, Telephony.Carriers.NAME, apnBundle,
                "napdef_name");
        validAndPut(contentValues, Telephony.Carriers.USER, apnBundle,
                "authname");
        validAndPut(contentValues, Telephony.Carriers.PASSWORD,
                apnBundle, "authsecret");
        validAndPut(contentValues, Telephony.Carriers.PROXY,
                apnBundle, "pxaddr");
        validAndPut(contentValues, Telephony.Carriers.PORT, apnBundle,
                "portnbr");
        putValuesToDatabase(contentValues, numeric, uri);

    }

    public void putContentValuesNull(ContentValues contentValues){
        contentValues.put(Telephony.Carriers.TYPE,"");
        contentValues.put(Telephony.Carriers.NAME,"");
        contentValues.put(Telephony.Carriers.USER,"");
        contentValues.put(Telephony.Carriers.PASSWORD,"");
        contentValues.put(Telephony.Carriers.PROXY,"");
        contentValues.put(Telephony.Carriers.PORT,"");
        contentValues.put(Telephony.Carriers.MMSC,"");
        contentValues.put(Telephony.Carriers.MMSPROXY,"");
        contentValues.put(Telephony.Carriers.MMSPORT,"");
    }

    public void putValuesToDatabase(ContentValues contentValues, String numeric, Uri uri) {
        Cursor c = null;
        try {
            ContentResolver resolver = mContext.getContentResolver();

            if (contentValues.get(Telephony.Carriers.TYPE) != null
                    && contentValues.get(Telephony.Carriers.APN) != null) {
                overlayOrAddApn(
                        (String) contentValues
                                .get(Telephony.Carriers.TYPE),
                        numeric, contentValues, uri, resolver);
            }
        } catch (Exception e) {
            Log.e(TAG, "merge setting error", e);
        } finally {
            if (c != null)
                c.close();
        }
    }

    public void validAndPut(ContentValues contentValues, String apnKey,
            Bundle bundle, String bundleKey) {
        if (bundle != null) {
            Log.d(TAG,
                    "validAndPut: apnKey = " + apnKey + "  bundleKey = "
                            + bundleKey);
            if (bundle.getString(bundleKey) != null) {
                String bundleValue = bundle.getString(bundleKey);
                Log.d(TAG, "validAndPut: bundleValue = " + bundleValue);
                if ("port".equalsIgnoreCase(apnKey)) {
                    contentValues.put(apnKey, getValidApnPort(bundleValue));
                } else {
                    contentValues.put(apnKey, bundleValue);
                }
            }
        } else {
            Log.d(TAG, "validAndPut: apnKey = " + apnKey + " apnValue = "
                    + bundleKey);
            if (bundleKey != null) {
                contentValues.put(apnKey, bundleKey);
            }
        }
    }

    public String getValidApnPort(String portSource) {
        int port = VALUE_DEFAULT_PORT;
        if (!TextUtils.isEmpty(portSource)) {
            try {
                int temp = Integer.parseInt(portSource);
                if (temp > VALUE_MAX_PORT) {
                    port = VALUE_DEFAULT_PORT;
                } else {
                    port = temp;
                }
            } catch (NumberFormatException e) {
                port = VALUE_DEFAULT_PORT;
            }
        }
        return String.valueOf(port);
    }

    public void overlayOrAddApn(String apnType, String numeric,
            ContentValues contentValues, Uri uri, ContentResolver resolver) {
        Log.d(TAG, "overlayOrAddApn: apnType:" + apnType);
        int count = 0;
        StringBuilder whereType = new StringBuilder();
        whereType.append(Telephony.Carriers.NUMERIC).append(" = '")
                .append(numeric).append("' and type").append(" = '")
                .append(apnType).append("'");
        StringBuilder whereName = new StringBuilder();
        whereName.append(Telephony.Carriers.NUMERIC).append(" = '")
                .append(numeric).append("' and name").append(" = '")
                .append(contentValues.getAsString(Telephony.Carriers.NAME))
                .append("'");
        Log.d(TAG, "overlayOrAddApn: whereType = " + whereType.toString()
                + "whereName = " + whereName.toString());
        Cursor cursor = resolver.query(uri, new String[] { "_id", "name",
                "apn", "type", "mmsc", "mmsport", "mmsproxy" },
                whereType.toString(), null, null);
        try {
            if (cursor != null && cursor.moveToFirst()) {
                Log.d(TAG,"cursor.getCount() = " + cursor.getCount());
                do {
                    Log.d(TAG, "hasOverlayMms = " + hasOverlayMms);
                    if ("mms".equalsIgnoreCase(apnType)) {
                        if (hasOverlayMms == true) {
                            hasOverlayMms = false;
                            resolver.delete(uri, whereType.toString(), null);
                            if (count == 0) {
                                resolver.insert(uri, contentValues);
                                count++;
                            }
                        } else {
                            if (!((cursor.getString(APN_INDEX)
                                    .equals(contentValues
                                            .getAsString(Telephony.Carriers.APN))))) {
                                if (count == 0) {
                                    resolver.insert(uri, contentValues);
                                    count++;
                                }

                            }
                        }
                    } else {// default apn
                        String queriedApn = mOriginalApnMap.get(cursor
                                .getString(ID_INDEX));
                        Log.d(TAG, "mOriginalApnMap = " + mOriginalApnMap
                                + " id = " + cursor.getString(ID_INDEX)
                                + " queriedApn = " + queriedApn);
                        StringBuilder whereId = new StringBuilder();
                        whereId.append(Telephony.Carriers.NUMERIC).append(" = '")
                                .append(numeric).append("' and _id").append(" = '");
                        if (queriedApn != null) {
                            if ("haveNotOverlay".equals(queriedApn)) {// overlay
                                                                      // method
                                                                      // one
                                if (cursor.getString(NAME_INDEX) != null
                                        && cursor
                                                .getString(NAME_INDEX)
                                                .equals(contentValues
                                                        .getAsString(Telephony.Carriers.NAME))) {
                                    Log.d(TAG, "overlay method one");
                                    if (count == 0) {
                                        resolver.insert(uri, contentValues);
                                        count++;
                                    }
                                    if (mOriginalApnMap.get(cursor
                                            .getString(ID_INDEX)) != null) {
                                        mOriginalApnMap.put(
                                                cursor.getString(ID_INDEX),
                                                "haveOverlay");
                                    }
                                    whereId.append(cursor.getString(ID_INDEX))
                                            .append("'");
                                    resolver.delete(uri, whereId.toString(),
                                            null);
                                } else {
                                    if (count == 0) {
                                        resolver.insert(uri, contentValues);
                                        count++;
                                    }
                                }
                            } else {
                                Log.d(TAG,
                                        "should not happen! this apn has being deleted");
                            }
                        } else {// new apn
                            Log.d(TAG,
                                    "cursor.apn = "
                                            + cursor.getString(APN_INDEX)
                                            + "contentValues.apn = "
                                            + contentValues.getAsString("apn")
                                            + "match = "
                                            + (cursor.getString(APN_INDEX).equals(contentValues
                                                    .getAsString("apn"))));
                            if (!(cursor.getString(APN_INDEX)
                                    .equals(contentValues
                                            .getAsString(Telephony.Carriers.APN)))) {
                                if (count == 0) {
                                    resolver.insert(uri, contentValues);
                                    count++;
                                }
                            } else {
                                Log.d(TAG,"there exist same apn update from one ota message");
                                whereId.append(cursor.getString(ID_INDEX))
                                        .append("'");
                                if (count == 0) {
                                    resolver.insert(uri, contentValues);
                                    count++;
                                }
                                resolver.delete(uri, whereId.toString(), null);
                            }
                        }
                    }
                } while (cursor.moveToNext());
            } else {
                Log.d(TAG, "cursor == null");
                resolver.insert(uri, contentValues);
            }
        } catch (Exception e) {
            Log.d(TAG,"operation of database exception!",e);
        } finally {
            if (cursor != null) {
                cursor.close();

            }
        }

    }

}
