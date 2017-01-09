
package com.sprd.providers.contacts;

import android.accounts.Account;
import android.accounts.AccountAuthenticatorActivity;
import android.accounts.AccountManager;
import android.accounts.AccountManagerFuture;
import android.accounts.AccountManagerCallback;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.app.Service;
import android.appwidget.AppWidgetManager;
import android.content.AsyncQueryHandler;
import android.content.BroadcastReceiver;
import android.content.ContentProviderResult;
import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.OperationApplicationException;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.database.sqlite.SQLiteException;
import android.net.Uri;
import android.os.Bundle;
import android.os.Debug;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Process;
import android.os.PowerManager;
import android.os.RemoteException;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.Groups;
import android.provider.ContactsContract.ProviderStatus;
import android.provider.ContactsContract.RawContacts;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.GroupMembership;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.telephony.PhoneNumberUtils;
import android.text.TextUtils;
import android.util.Log;

import com.google.android.collect.Lists;
import com.android.internal.telephony.TeleFrameworkFactory;
import com.android.internal.telephony.IccPBForMimetypeException;
import com.android.internal.telephony.IccPBForRecordException;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.regex.Pattern;
import java.util.Set;
import android.provider.ContactsContract.CommonDataKinds.Nickname;

public class SimContactProxy implements IContactProxy {
    interface SIM_CONTACT_FIELD {
        // name field for query and update is different...
        final String NAME_QUERY = "name";
        final String NAME_UPDATE = "tag";
        final String NUMBER = "number";
        final String ANR = "anr";
        final String EMAIL = "email";
        final String GROUP = "grp";
        final String INDEX = "index";

        /* sprd bug490245 read sne and aas for orange @{ */
        final String SNE = "sne";
        final String AAS = "aas";
        /* @} */

        int NAME_INDEX = 0;
        int NUMBER_INDEX = 1;
        int ANR_INDEX = 3;
        int EMAIL_INDEX = 2;
        int GROUP_INDEX = 6;
        int INDEX_INDEX = 8;
        /* sprd bug490245 read sne and aas for orange @{ */
        int AAS_INDEX = 4;
        int SNE_INDEX = 5;
        /* @} */
    }

    interface SIM_GROUP_FIELD {
        final String NAME = "gas";
        final String INDEX = "index";

        int NAME_INDEX = 0;
        int INDEX_INDEX = 1;
    }

    /* sprd bug490245 read sne and aas for orange @{ */
    interface SIM_AAS_FIELD {
        final String NAME = "aas";
        final String INDEX = "index";
        int NAME_INDEX = 0;
        int INDEX_INDEX = 1;
    }
    /* @} */

    private static final String TAG = "SimContactProxy";
    private static final boolean DEBUG = true;
    public static final String WITH_EXCEPTION = "with_exception";
    private static final String[] SIM_PROJECTION = new String[] {
            SIM_CONTACT_FIELD.NAME_QUERY,
            SIM_CONTACT_FIELD.NUMBER,
            SIM_CONTACT_FIELD.ANR,
            /* sprd bug490245 read sne and aas for orange @{ */
            SIM_CONTACT_FIELD.AAS,
            SIM_CONTACT_FIELD.SNE,
            /* @} */
            SIM_CONTACT_FIELD.EMAIL,
            SIM_CONTACT_FIELD.GROUP,
            SIM_CONTACT_FIELD.INDEX
    };

    private static final String[] SIM_GROUP_PROJECTION = new String[] {
            SIM_GROUP_FIELD.NAME,
            SIM_GROUP_FIELD.INDEX
    };

    /* sprd bug490245 read sne and aas for orange @{ */
    private static final String[] SIM_AAS_PROJECTION = new String[] {
        SIM_AAS_FIELD.NAME,
        SIM_AAS_FIELD.INDEX,
    };
    /* @} */

    public static final String[] SIM_SDN_PROJECTION = new String[] {
            SIM_CONTACT_FIELD.NAME_QUERY,
            SIM_CONTACT_FIELD.NUMBER
    };

    private AccountManager mAm;
    private Context mContext;
    private ContentResolver mResolver;
    private SimContactCache mSimContactCache = new SimContactCache();
    private SimContactGroupCache mSimContactGroupCache = new SimContactGroupCache();
    private final Pattern mSemicolonPattern = Pattern.compile(";");
    /* sprd bug490245 read sne and aas for orange @{ */
    private final Pattern mCommaPattern = Pattern.compile(",");
    /* @} */
    private final Pattern mPhoneNumPattern = Pattern.compile("[^0-9\\+,;N\\*#]");
    private boolean mImporting = false;
    private PowerManager.WakeLock mWakeLock;

    public SimContactProxy(Context context) {
        mContext = context;
        mResolver = context.getContentResolver();
        mAm = AccountManager.get(context);
        final PowerManager powerManager =
                (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG);
    }

    public void addToNonSimContactCache(long rawContactId) {
        mSimContactCache.remove(rawContactId);
        mSimContactCache.addNonSimEntry(rawContactId);
    }

    public SimContactCache getSimContactProxySimContactCache() {
        return mSimContactCache;
    }

    public SimContactGroupCache getSimContactProxySimContactGroupCache() {
        return mSimContactGroupCache;
    }

    public boolean isImporting() {
        return mImporting;
    }

    @Override
    public ContentValues insertGroup(long groupRowId, ContentValues values, Account account) {
        Log.e(TAG, "insertGroup:" + groupRowId + " " + values.toString());
        String iccGroupUri = mAm.getUserData(account, "icc_gas_uri");
        if (iccGroupUri == null) {
            return null;
        }
        // insert to sim
        ContentValues groupValues = new ContentValues();
        groupValues.put(SIM_GROUP_FIELD.NAME, values.getAsString(Groups.TITLE));
        Uri result = null;
        try {
            result = mResolver.insert(Uri.parse(iccGroupUri).buildUpon()
                    .appendQueryParameter(WITH_EXCEPTION, "true").build()
                    , groupValues);
        } catch (Exception e) {

            /**
             * SPRD:Bug454872 Remove the groupId from cache when sim/usim group insert failed.
             * @{
             */
            mSimContactGroupCache.remove(groupRowId);
            /**
             * @}
             */
            // TODO: handle exception
            if (e instanceof IccPBForMimetypeException) {
                IccPBForMimetypeException exception =
                        (IccPBForMimetypeException) e;
                throw exception;
            } else {
                Log.e(TAG, e.getMessage());
            }
        }

        if (result == null) {
            throw new IllegalStateException("sim insert group failed");
        }
        String groupIndex = result.getLastPathSegment();
        if (groupIndex == null) {
            throw new IllegalStateException("sim insert group failed");
        }
        ContentValues simValues = new ContentValues();
        simValues.put(Groups.SYNC1, groupIndex);
        simValues.put(Groups.SYNC2, iccGroupUri);

        mSimContactGroupCache.addEntry(groupRowId, groupIndex, iccGroupUri, account);
        return simValues;
    }

    public void updateGroup(long groupRowId, ContentValues values) {
        Log.e(TAG, "updateGroup:" + groupRowId + " " + values.toString());
        String iccGroupUri = mSimContactGroupCache.getIccGroupUri(groupRowId);

        if (!values.containsKey(Groups.TITLE)) {
            return;
        }

        String groupIndex = mSimContactGroupCache.getGroupIndex(groupRowId);
        if (groupIndex == null) {
            return;
        }

        ContentValues groupValues = new ContentValues();
        groupValues.put(SIM_GROUP_FIELD.NAME, values.getAsString(Groups.TITLE));
        groupValues.put(SIM_GROUP_FIELD.INDEX, groupIndex);

        if (iccGroupUri == null) {
            Log.e(TAG, "iccGroupUri = null");
            return;
        }
        Log.e(TAG, "updateGroup: " + iccGroupUri + " " + groupValues.toString() + " groupIndex:"
                + groupIndex);
        int count = 0;
        try {
            count = mResolver.update(Uri.parse(iccGroupUri).buildUpon()
                    .appendQueryParameter(WITH_EXCEPTION, "true").build()
                    , groupValues, null, null);
        } catch (Exception e) {

            // TODO: handle exception
            if (e instanceof IccPBForMimetypeException) {
                IccPBForMimetypeException exception =
                        (IccPBForMimetypeException) e;
                throw exception;
            } else {
                Log.e(TAG, e.getMessage());
            }
        }
        if (count <= 0) {
            throw new IllegalStateException("sim insert failed");
        }
    }

    @Override
    public void removeGroup(long groupRowId) {
        Log.e(TAG, "removeGroup:" + groupRowId);
        String iccGroupUri = mSimContactGroupCache.getIccGroupUri(groupRowId);
        String groupIndex = mSimContactGroupCache.getGroupIndex(groupRowId);
        Log.e(TAG, "removeGroup: iccGroupUri=" + iccGroupUri + " groupIndex=" + groupIndex);
        if (iccGroupUri == null || groupIndex == null) {
            return;
        }
        mResolver.delete(Uri.parse(iccGroupUri), SIM_GROUP_FIELD.INDEX + "=" + groupIndex, null);
        mSimContactGroupCache.remove(groupRowId);
        mSimContactCache.removeGroup(groupRowId);
    }

    @Override
    public ContentValues insert(long rawContactId, Account account)
            throws IllegalStateException {
        Log.e(TAG, ">>>SimContactProx.insert:" + rawContactId + " account=" + account);
        mSimContactCache.setIsDirty(rawContactId, false);
        if (rawContactId != -1
                && mSimContactCache.getSimIndex(rawContactId) != null) {
            // the contact is already in sim card
            return null;
        }
        String iccUri = mAm.getUserData(account, "icc_uri");
        if (iccUri == null) {
            Log.e(TAG, "sim insert failed: iccUri is null");
            throw new IllegalStateException(
                    "sim insert failed: icc_uri is null");
        }
        ContentValues values = mSimContactCache.getData(rawContactId, false);
        if (values == null || values.size() == 0) {
            throw new IllegalStateException("sim insert failed: no data");
        }
        ContentValues simValues = new ContentValues();
        try {
            Uri uri = Uri.parse(iccUri).buildUpon()
                    .appendQueryParameter(WITH_EXCEPTION, "true").build();

            Uri result = mResolver.insert(uri, values);
            if (result == null) {
                mSimContactCache.remove(rawContactId);
                throw new IllegalStateException("sim insert failed");
            }
            Log.e(TAG, "icc.insert: " + result.toString());

            String simIndex = result.getLastPathSegment();
            simValues.put(RawContacts.SYNC1, simIndex);
            simValues.put(RawContacts.SYNC2, iccUri);

            mSimContactCache.addEntry(rawContactId, simIndex, iccUri, account);
            Log.e(TAG, "<<<SimContactProxy:insert");
            if (rawContactId == -1) {
                mSimContactCache.remove(rawContactId);
            }
        } catch (Exception e) {
            if (e instanceof IccPBForRecordException) {
                mSimContactCache.remove(rawContactId);
                IccPBForRecordException exception = (IccPBForRecordException) e;
                throw exception;
            } else if (e instanceof IccPBForMimetypeException) {
                mSimContactCache.remove(rawContactId);
                IccPBForMimetypeException exception = (IccPBForMimetypeException) e;
                throw exception;
            } else if (e instanceof IllegalStateException) {
                IllegalStateException exception = (IllegalStateException) e;
                throw exception;
            } else {
                Log.e(TAG, e.getMessage());
            }
        }
        return simValues;
    }

    @Override
    public void update(long rawContactId) throws IllegalStateException {

        Log.e(TAG, "update:" + rawContactId + "is dirty");
        ContentValues values = mSimContactCache.getData(rawContactId, false);

        String simIndex = mSimContactCache.getSimIndex(rawContactId);
        values.put(SIM_CONTACT_FIELD.INDEX, simIndex);

        String iccUri = mSimContactCache.getIccUri(rawContactId);
        if (iccUri == null) {
            Log.e(TAG, "iccUri = null");
            return;
        }
        try {
            Uri uri = Uri.parse(iccUri).buildUpon()
                    .appendQueryParameter(WITH_EXCEPTION, "true").build();
            int count = mResolver.update(uri, values, null, null);
            mSimContactCache.setIsDirty(rawContactId, false);
            if (count == 0) {
                throw new IllegalStateException("sim update failed");
            }
        } catch (Exception e) {
            //handle exception
            if (e instanceof IccPBForRecordException) {
                mSimContactCache.setIsDirty(rawContactId, true);
                IccPBForRecordException exception = (IccPBForRecordException) e;
                throw exception;
            } else if (e instanceof IccPBForMimetypeException) {
                mSimContactCache.setIsDirty(rawContactId, true);
                IccPBForMimetypeException exception = (IccPBForMimetypeException) e;
                throw exception;
            } else if (e instanceof IllegalStateException) {
                IllegalStateException exception = (IllegalStateException) e;
                throw exception;
            } else {
                Log.e(TAG, e.getMessage());
            }
        }
    }

    @Override
    public void remove(long rawContactId) {
        if (DEBUG)
            Log.d(TAG, "mSimContactCache= " + mSimContactCache + "rawContactId=" + rawContactId);
        String iccUri = mSimContactCache.getIccUri(rawContactId);
        String simIndex = mSimContactCache.getSimIndex(rawContactId);
        if (DEBUG)
            Log.d(TAG, "iccUri=" + iccUri + "simIndex=" + simIndex);
        if (iccUri != null) {
            mResolver.delete(Uri.parse(iccUri), SIM_CONTACT_FIELD.INDEX + "=" + simIndex, null);
            mSimContactCache.remove(rawContactId);
        }
    }

    @Override
    public void onDataUpdate(long rawContactId, ContentValues values, String mimeType) {
        ContentValues data = mSimContactCache.getData(rawContactId, false);
        Log.e(TAG, "onDataUpdate for " + mimeType + " values " + values.getAsString("data1")
                + " type " + values.getAsString("data2"));

        if (mimeType.equals(StructuredName.CONTENT_ITEM_TYPE)) {
            String name = values.getAsString(StructuredName.DISPLAY_NAME);
            data.put(SIM_CONTACT_FIELD.NAME_UPDATE, name);
        } else if (mimeType.equals(Phone.CONTENT_ITEM_TYPE)) {
            String phone = values.getAsString(Phone.NUMBER);
            String phoneOrig = values.getAsString(Phone.NUMBER + "_orig");
            int type = values.getAsInteger("data2");
            String aasType = values.getAsString(Phone.LABEL);
            Log.d(TAG, "onDataUpdate aasType: " + aasType);
            if (phoneOrig != null) {
                // phoneOrig = PhoneNumberUtils.pAndwToCommaAndSemicolon(phoneOrig);
                phoneOrig = mPhoneNumPattern.matcher(phoneOrig).replaceAll("");
                mSimContactCache.removePhone(rawContactId, phoneOrig);
                mSimContactCache.removeAas(rawContactId, phoneOrig);
            }
            if (phone != null) {
                // phone = PhoneNumberUtils.pAndwToCommaAndSemicolon(phone);
                phone = mPhoneNumPattern.matcher(phone).replaceAll("");
                if (type == Phone.TYPE_MOBILE) {
                    data.put(SIM_CONTACT_FIELD.NUMBER, phone);
                /* sprd bug490245 read sne and aas for orange @{ */
                } else if (type == Phone.TYPE_CUSTOM) {
                    data.put(SIM_CONTACT_FIELD.ANR, phone);
                    Log.d(TAG, "onDataUpdate rawContactId: " + rawContactId);
                    mSimContactCache.addAas(rawContactId, aasType == null ? "" : aasType);
                /* @} */
                }
                mSimContactCache.addPhone(rawContactId, phone);
            }
            mSimContactCache.resetAnrAndPhoneNumber(rawContactId, data);
            /* sprd bug490245 read sne and aas for orange @{ */
            mSimContactCache.resetAnrAndAas(rawContactId, data);
            /* @} */

        } else if (mimeType.equals(Email.CONTENT_ITEM_TYPE)) {
            String email = values.getAsString(Email.ADDRESS);
            data.put(SIM_CONTACT_FIELD.EMAIL, email);
        } else if (mimeType.equals(GroupMembership.CONTENT_ITEM_TYPE)) {
            Long groupRowId = values.getAsLong(GroupMembership.GROUP_ROW_ID);
            if (groupRowId == null) {
                // means remove from a certain group
                long origGroupRowId = Long.parseLong(values
                        .getAsString(GroupMembership.GROUP_ROW_ID + "_orig"));
                mSimContactCache.removeFromGroup(rawContactId, origGroupRowId);
            } else {
                mSimContactCache.addToGroup(rawContactId, groupRowId);
            }
            Set<Long> groups = mSimContactCache.getGroups(rawContactId);
            if (groups != null) {
                data.put(SIM_CONTACT_FIELD.GROUP, mSimContactGroupCache.buildGas(groups));
            }
        }
        /* sprd bug490245 read sne and aas for orange @{ */
        else if (mimeType.equals(Nickname.CONTENT_ITEM_TYPE)) {
            String nickName = values.getAsString(Nickname.NAME);
            Log.d(TAG, "onDataUpdate nickName: " + nickName);
            data.put(SIM_CONTACT_FIELD.SNE, nickName);
        /* @} */
        } else {
            Log.e(TAG, "onDataUpdate: unknown mime_type:" + mimeType);
        }
        mSimContactCache.setIsDirty(rawContactId, true);
    }

    public class SimContactGroupCache {
        long mGroupRowId;
        String mGroupIndex;

        class SimContactGroupCacheEntry {
            String mGroupIndex;
            String mIccGroupUri;
            Account mAccount;

            public SimContactGroupCacheEntry(String groupIndex, String groupUri, Account account) {
                mGroupIndex = groupIndex;
                mIccGroupUri = groupUri;
                mAccount = account;
            }
        }

        Map<Long, SimContactGroupCacheEntry> mCache = new HashMap<Long, SimContactGroupCacheEntry>();
        Set<Long> mNonSimCache = new HashSet<Long>();
        Map<String, Long> mGroupIndexMap = new HashMap<String, Long>();

        int isSimContactGroup(long groupRowId) {
            if (mCache.containsKey(groupRowId)) {
                return 1;
            }
            if (mNonSimCache.contains(groupRowId)) {
                return -1;
            }
            return 0;
        }

        void addNonSimEntry(long groupRowId) {
            mNonSimCache.add(groupRowId);
        }

        String buildGas(Set<Long> groups) {
            if (groups == null) {
                return null;
            }
            StringBuilder sb = new StringBuilder();
            boolean initial = true;
            for (Long id : groups) {
                if (mCache.containsKey(id)) {
                    if (!initial) {
                        sb.append(";");
                    }
                    sb.append(getGroupIndex(id));
                    initial = false;
                }
            }
            Log.e(TAG, "buildGas: " + sb.toString());
            return sb.toString();
        }

        Set<Long> parseGas(String gas, String groupUri) {
            if (gas == null) {
                return Collections.emptySet();
            }
            Set<Long> ret = new HashSet<Long>();
            String[] groups = mSemicolonPattern.split(gas);
            for (String index : groups) {
                long id = getGroupRowId(index + groupUri);
                if (id != -1) {
                    ret.add(id);
                }
            }
            return ret;
        }

        String getGroupIndex(long groupRowId) {
            return mCache.get(groupRowId).mGroupIndex;
        }

        long getGroupRowId(String groupIndex) {
            if (mGroupIndexMap.containsKey(groupIndex)) {
                Log.e(TAG, "getGroupRowId returns -1");
                return mGroupIndexMap.get(groupIndex);
            } else {
                return -1;
            }
        }

        void addEntry(long groupRowId, String groupIndex, String iccGroupUri, Account account) {
            Log.e(TAG, "SimContactGroupCache: addEntry:" + groupRowId + " " + groupIndex + " "
                    + iccGroupUri + " " + account);
            mCache.put(groupRowId, new SimContactGroupCacheEntry(groupIndex, iccGroupUri, account));
            // The dierent SIMGroup maybe has the same groupIndex, so should
            // change the key and be sure unique ,add for Bug345916.
            mGroupIndexMap.put(groupIndex + iccGroupUri, groupRowId);
        }

        String getIccGroupUri(long groupRowId) {
            return mCache.get(groupRowId).mIccGroupUri;
        }

        void remove(long groupRowId) {
            mCache.remove(groupRowId);
        }
    }

    public class SimContactCache {

        class SimContactCacheEntry {
            long mRawContactId;
            String mSimIndex;
            String mIccUri;
            String mIccGroupUri;
            Account mAccount;
            boolean mIsDirty = false;

            public SimContactCacheEntry(long rawContactId) {
                mRawContactId = rawContactId;
            }

            public String toString() {
                return "mRawContactId: " + mRawContactId + " \n mSimIndex: "
                        + mSimIndex + " \n mAccount:" + mAccount.name;
            }
        }

        private HashMap<Long, SimContactCacheEntry> mCache = new HashMap<Long, SimContactCacheEntry>();
        private HashSet<Long> mNonSimCache = new HashSet<Long>();
        private HashMap<Long, ContentValues> mPendingValuesCache = new HashMap<Long, ContentValues>();
        private HashMap<Long, Set<Long>> mGroupCache = new HashMap<Long, Set<Long>>();
        private HashMap<Long, ArrayList<String>> mPhoneCache = new HashMap<Long, ArrayList<String>>();
        /* sprd bug490245 read sne and aas for orange @{ */
        private HashMap<Long, ArrayList<String>> mAasCache = new HashMap<Long, ArrayList<String>>();
        /* @} */

        void setIsDirty(long rawContactId, boolean dirty) {
            SimContactCacheEntry entry = mCache.get(rawContactId);
            if (entry != null) {
                entry.mIsDirty = dirty;
            }
        }

        public boolean getIsDirty(long rawContactId) {
            SimContactCacheEntry entry = mCache.get(rawContactId);
            if (entry != null) {
                return entry.mIsDirty;
            }
            return true;
        }

        int isSimContact(long rawContactId) {
            if (mCache.containsKey(rawContactId)) {
                return 1;
            }
            if (mNonSimCache.contains(rawContactId)) {
                return -1;
            }
            return 0;
        }

        boolean contains(long rawContactId) {
            return mCache.containsKey(rawContactId);
        }

        void remove(long rawContactId) {
            mCache.remove(rawContactId);
            mPendingValuesCache.remove(rawContactId);
            mGroupCache.remove(rawContactId);
            mPhoneCache.remove(rawContactId);
            /* sprd bug490245 read sne and aas for orange @{ */
            mAasCache.remove(rawContactId);
            /* @} */
            mNonSimCache.remove(rawContactId);
        }

        String getIccUri(long rawContactId) {
            return mCache.get(rawContactId).mIccUri;
        }

        String getSimIndex(long rawContactId) {
            SimContactCacheEntry entry = mCache.get(rawContactId);
            return entry.mSimIndex;
        }

        void addToGroup(long rawContactId, long groupRowId) {
            Set<Long> ids = mGroupCache.get(rawContactId);
            if (ids == null) {
                ids = new HashSet<Long>();
                mGroupCache.put(rawContactId, ids);
            }
            ids.add(groupRowId);
            Log.e(TAG, "addToGroup:" + rawContactId + " " + groupRowId);
        }

        void removeFromGroup(long rawContactId, long groupRowId) {
            Set<Long> ids = mGroupCache.get(rawContactId);
            if (ids == null) {
                return;
            }
            Log.e(TAG, "removeFromGroup:" + rawContactId + " " + groupRowId);
            ids.remove(groupRowId);
        }

        void removeGroup(long groupRowId) {
            Set<Long> keys = mGroupCache.keySet();
            for (long key : keys) {
                mGroupCache.get(key).remove(groupRowId);
            }
        }

        Set<Long> getGroups(long rawContactId) {
            return mGroupCache.get(rawContactId);
        }

        void addPhone(long rawContactId, String phone) {
            ArrayList<String> phones = mPhoneCache.get(rawContactId);
            if (phones == null) {
                phones = new ArrayList<String>();
                mPhoneCache.put(rawContactId, phones);
            }
            phones.add(phone);
            Log.e(TAG, "addPhone:" + rawContactId + " " + phone);
        }

        void removePhone(long rawContactId, String phone) {
            ArrayList<String> phones = mPhoneCache.get(rawContactId);
            Log.e(TAG, "removePhone:" + rawContactId + " " + phone);
            if (phones == null) {
                return;
            }
            phones.remove(phone);
        }

        /* sprd bug490245 read sne and aas for orange @{ */
        void addAas(long rawContactId, String aas) {
            ArrayList<String> aass = mAasCache.get(rawContactId);
            if (aass == null) {
                aass = new ArrayList<String>();
                mAasCache.put(rawContactId, aass);
            }
            aass.add(aas);
            Log.e(TAG, "addAas:" + rawContactId + " " + aas);
        }

        void removeAas(long rawContactId, String aas) {
            ArrayList<String> aass = mAasCache.get(rawContactId);
            Log.e(TAG, "removeAas:" + rawContactId + " " + aas);
            if (aass == null) {
                return;
            }
            aass.remove(aas);
        }

        void resetAnrAndAas(long rawContactId, ContentValues data) {
            ArrayList<String> aass = mAasCache.get(rawContactId);
            if (aass == null) {
                return;
            }
            Iterator<String> ite = aass.iterator();
            StringBuilder sb = new StringBuilder();
            boolean initial = true;
            while (ite.hasNext()) {
                String aas = ite.next();
                if (!initial) {
                    sb.append(";");
                }
                sb.append(aas);
                initial = false;
            }
            Log.d(TAG,"aass == " + sb.toString());
            data.put(SIM_CONTACT_FIELD.AAS, sb.toString());
        }
        /* @} */

        void resetAnrAndPhoneNumber(long rawContactId, ContentValues data) {
            ArrayList<String> phones = mPhoneCache.get(rawContactId);
            if (phones == null) {
                return;
            }
            Iterator<String> ite = phones.iterator();
            StringBuilder sb = new StringBuilder();
            boolean initial = true;
            boolean isIgnoreMobile = false;
            while (ite.hasNext()) {
                String num = ite.next();
                if (num.equals(data.get(SIM_CONTACT_FIELD.NUMBER)) && !isIgnoreMobile) {
                    isIgnoreMobile = true;
                    continue;
                }
                if (!initial) {
                    sb.append(";");
                }
                sb.append(num);
                initial = false;
            }
            data.put(SIM_CONTACT_FIELD.ANR, sb.toString());
        }

        ContentValues getData(long rawContactId, boolean shouldRemove) {
            ContentValues ret = mPendingValuesCache.get(rawContactId);
            if (ret == null) {
                Log.e(TAG, "getData ret == null");
                ret = new ContentValues();
                mPendingValuesCache.put(rawContactId, ret);
            }
            if (shouldRemove) {
                mPendingValuesCache.remove(rawContactId);
            }
            return ret;
        }

        void addEntry(long rawContactId, String simIndex, String iccUri, Account account) {
            SimContactCacheEntry entry = new SimContactCacheEntry(rawContactId);
            entry.mSimIndex = simIndex;
            entry.mIccUri = iccUri;
            entry.mAccount = account;
            mCache.put(rawContactId, entry);

            Log.e(TAG, "addEntry: " + entry.mSimIndex + " :" + rawContactId);
        }

        void addNonSimEntry(long rawContactId) {
            mNonSimCache.add(rawContactId);
        }

        void dump() {
            Log.e(TAG, "<<< dump cache entries");
            for (Long id : mCache.keySet()) {
                Log.e(TAG, "" + id + ": " + mCache.get(id).toString());
            }
            Log.e(TAG, ">>> dump cache entries");
        }
    }

    private void insertSimSdnContactData(Cursor cursor, ArrayList<ContentProviderOperation> ops,
            int idOffset) {
        String name = cursor.getString(cursor.getColumnIndex(SIM_CONTACT_FIELD.NAME_QUERY));
        String phoneNumber = cursor.getString(cursor.getColumnIndex(SIM_CONTACT_FIELD.NUMBER));
        if (DEBUG) {
            Log.e(TAG, "insertSimSdnContactData:name: " + name + " number: " + phoneNumber);
        }
        if (!TextUtils.isEmpty(name)) {
            ContentValues values = new ContentValues();
            values.put(Data.MIMETYPE, StructuredName.CONTENT_ITEM_TYPE);
            values.put(StructuredName.DISPLAY_NAME, name);
            ContentProviderOperation operation = ContentProviderOperation
                    .newInsert(
                            Data.CONTENT_URI
                                    .buildUpon()
                                    .appendQueryParameter(ContactsContract.CALLER_IS_SYNCADAPTER,
                                            "true").build())
                    .withValueBackReference(Data.RAW_CONTACT_ID, idOffset)
                    .withValues(values).withYieldAllowed(true).build();
            ops.add(operation);
        }
        if (!TextUtils.isEmpty(phoneNumber)) {
            ContentValues values = new ContentValues();
            values.put(Data.MIMETYPE, Phone.CONTENT_ITEM_TYPE);
            values.put(Phone.NUMBER, phoneNumber);
            values.put(Phone.TYPE, Phone.TYPE_MOBILE);
            ContentProviderOperation operation = ContentProviderOperation
                    .newInsert(
                            Data.CONTENT_URI
                                    .buildUpon()
                                    .appendQueryParameter(ContactsContract.CALLER_IS_SYNCADAPTER,
                                            "true").build())
                    .withValueBackReference(Data.RAW_CONTACT_ID, idOffset)
                    .withValues(values).withYieldAllowed(true)
                    .build();
            ops.add(operation);
        }
    }

    private void insertSimContactData(Cursor cursor, ArrayList<ContentProviderOperation> ops,
            int idOffset, String iccGroupUri) {
        String name = cursor.getString(SIM_CONTACT_FIELD.NAME_INDEX);
        String phoneNumber = cursor.getString(SIM_CONTACT_FIELD.NUMBER_INDEX);
        String anr = cursor.getString(SIM_CONTACT_FIELD.ANR_INDEX);
        if (anr == null) {
            anr = "";
        }
        String email = cursor.getString(SIM_CONTACT_FIELD.EMAIL_INDEX);
        String group = cursor.getString(SIM_CONTACT_FIELD.GROUP_INDEX);

        /* sprd bug490245 read sne and aas for orange @{ */
        String aas = cursor.getString(SIM_CONTACT_FIELD.AAS_INDEX);
        String sne = cursor.getString(SIM_CONTACT_FIELD.SNE_INDEX);
        if (aas == null) {
            aas = "";
        }

        if (DEBUG) {
            Log.d(TAG, "insertSimContactData:name: " + name + " number: " + phoneNumber +
                    " anr:" + anr + " email:" + email + " group:" + group + " aas:" + aas +
                    " sne:" + sne);
        }
        String[] otherPhoneNumber = mSemicolonPattern.split(anr);
        /* sprd bug490245 read sne and aas for orange @{ */
        String[] aasType = mSemicolonPattern.split(aas);
        /* @} */
        if (!TextUtils.isEmpty(name)) {
            ContentValues values = new ContentValues();
            values.put(Data.MIMETYPE, StructuredName.CONTENT_ITEM_TYPE);
            values.put(StructuredName.DISPLAY_NAME, name);
            ContentProviderOperation operation = ContentProviderOperation
                    .newInsert(
                            Data.CONTENT_URI
                                    .buildUpon()
                                    .appendQueryParameter(ContactsContract.CALLER_IS_SYNCADAPTER,
                                            "true").build())
                    .withValueBackReference(Data.RAW_CONTACT_ID, idOffset)
                    .withValues(values).withYieldAllowed(true).build();
            ops.add(operation);
        }
        if (!TextUtils.isEmpty(phoneNumber)) {
            // phoneNumber = PhoneNumberUtils.CommaAndSemicolonTopAndw(phoneNumber);
            ContentValues values = new ContentValues();
            values.put(Data.MIMETYPE, Phone.CONTENT_ITEM_TYPE);
            values.put(Phone.NUMBER, phoneNumber);
            values.put(Phone.TYPE, Phone.TYPE_MOBILE);
            ContentProviderOperation operation = ContentProviderOperation
                    .newInsert(
                            Data.CONTENT_URI
                                    .buildUpon()
                                    .appendQueryParameter(ContactsContract.CALLER_IS_SYNCADAPTER,
                                            "true").build())
                    .withValueBackReference(Data.RAW_CONTACT_ID, idOffset)
                    .withValues(values).withYieldAllowed(true)
                    .build();
            ops.add(operation);
        }

        if (otherPhoneNumber != null) {
            /* sprd bug490245 read sne and aas for orange
            for (String number : otherPhoneNumber) {
                if (!TextUtils.isEmpty(number)) {
                    // number = PhoneNumberUtils.CommaAndSemicolonTopAndw(number);
                     *
                     */
            for (int i = 0; i < otherPhoneNumber.length; i++) {
                if (!TextUtils.isEmpty(otherPhoneNumber[i])) {
                    //otherPhoneNumber[i] = PhoneNumberUtils.CommaAndSemicolonTopAndw(otherPhoneNumber[i]);
                    /* @} */
                    ContentValues values = new ContentValues();
                    values.put(Data.MIMETYPE, Phone.CONTENT_ITEM_TYPE);
                    /* sprd bug490245 read sne and aas for orange @{ */
                    values.put(Phone.NUMBER, otherPhoneNumber[i]);
                    /**
                     *
                    values.put(Phone.TYPE, Phone.TYPE_OTHER);
                     * @{
                     */
                    if (TeleFrameworkFactory.getInstance().isSupportOrange()) {
                        values.put(Phone.TYPE, Phone.TYPE_CUSTOM);
                        if (!TextUtils.isEmpty(aasType[i])) {
                            String[] aasList = mCommaPattern.split(aasType[i]);
                            //values.put(Phone.LABEL,aasType[i]);
                            if (!TextUtils.isEmpty(aasList[0]) && !"null".equals(aasList[0])) {
                                values.put(Phone.LABEL, aasList[0]);
                                if (aasList.length > 1) {
                                    values.put(Phone.DATA5, aasList[1]);
                                }
                            } else {
                                values.put(Phone.LABEL, "null");
                            }
                        } else {
                            values.put(Phone.LABEL, "null");
                        }
                    } else {
                        values.put(Phone.TYPE, Phone.TYPE_FIXED_NUMBER);
                    }
                    /* @} */
                    /**
                     * @}
                     */
                    ContentProviderOperation operation = ContentProviderOperation
                            .newInsert(
                                    Data.CONTENT_URI
                                            .buildUpon()
                                            .appendQueryParameter(
                                                    ContactsContract.CALLER_IS_SYNCADAPTER, "true")
                                            .build())
                            .withValueBackReference(Data.RAW_CONTACT_ID, idOffset)
                            .withValues(values)
                            .withYieldAllowed(true).build();
                    ops.add(operation);
                }
            }
        }

        if (!TextUtils.isEmpty(email)) {
            ContentValues values = new ContentValues();
            values.put(Data.MIMETYPE, Email.CONTENT_ITEM_TYPE);
            values.put(Email.TYPE, Email.TYPE_HOME);
            values.put(Email.DATA, email);
            values.put(Data.IS_PRIMARY, 1);
            values.put(Data.IS_SUPER_PRIMARY, 1);
            ContentProviderOperation operation = ContentProviderOperation
                    .newInsert(
                            Data.CONTENT_URI
                                    .buildUpon()
                                    .appendQueryParameter(ContactsContract.CALLER_IS_SYNCADAPTER,
                                            "true").build())
                    .withValueBackReference(Data.RAW_CONTACT_ID, idOffset)
                    .withValues(values).withYieldAllowed(true).build();
            ops.add(operation);
        }

        Set<Long> groups = mSimContactGroupCache.parseGas(group, iccGroupUri);
        for (long groupRowId : groups) {
            ContentValues values = new ContentValues();
            values.put(Data.MIMETYPE, GroupMembership.CONTENT_ITEM_TYPE);
            values.put(GroupMembership.GROUP_ROW_ID, groupRowId);
            ContentProviderOperation operation = ContentProviderOperation
                    .newInsert(
                            Data.CONTENT_URI
                                    .buildUpon()
                                    .appendQueryParameter(ContactsContract.CALLER_IS_SYNCADAPTER,
                                            "true").build())
                    .withValueBackReference(Data.RAW_CONTACT_ID, idOffset)
                    .withValues(values)
                    .withYieldAllowed(true).build();
            ops.add(operation);
        }
        /* sprd bug490245 read sne and aas for orange @{ */
        if (!TextUtils.isEmpty(sne)) {
            ContentValues values = new ContentValues();
            values.put(Data.MIMETYPE, Nickname.CONTENT_ITEM_TYPE);
            values.put(Nickname.NAME, sne);
            ContentProviderOperation operation = ContentProviderOperation
                    .newInsert(
                        Data.CONTENT_URI.buildUpon()
                            .appendQueryParameter(ContactsContract.CALLER_IS_SYNCADAPTER,
                                            "true").build())
                    .withValueBackReference(Data.RAW_CONTACT_ID, idOffset)
                    .withValues(values).withYieldAllowed(true).build();
            ops.add(operation);
        }
        /* @} */
    }

    @Override
    public void onImport(Account account) {
        try {
            mImporting = true;
            mWakeLock.acquire();
            String iccUri = AccountManager.get(mContext).getUserData(account, "icc_uri");
            String iccGroupUri = AccountManager.get(mContext).getUserData(account, "icc_gas_uri");
            /* sprd bug490245 read sne and aas for orange @{ */
            String iccAasUri = AccountManager.get(mContext).getUserData(account, "icc_aas_uri");
            String iccSneUri = AccountManager.get(mContext).getUserData(account, "icc_sne_uri");
            /* @} */
            String iccSdnUri = AccountManager.get(mContext).getUserData(account, "icc_sdn_uri");
            Log.e(TAG, "iccUri:" + iccUri);
            Log.e(TAG, "iccGroupUri:" + iccGroupUri);
            /* sprd bug490245 read sne and aas for orange @{ */
            Log.e(TAG, "iccAasUri:" + iccAasUri);
            Log.e(TAG, "iccSneUri:" + iccSneUri);
            /* @} */
            Log.e(TAG, "iccSdnUri:" + iccSdnUri);
            if (iccGroupUri == null) {
                Log.e(TAG, "iccGroupUri == null");
                return;
            }
            ContentResolver cr = mResolver;
            // first, get sim groups
            Cursor groupCursor = null;
            try {
                groupCursor = cr.query(Uri.parse(iccGroupUri), SIM_GROUP_PROJECTION, null, null,
                        null);
            } catch (Exception e) {
                e.printStackTrace();
            }
            if (groupCursor == null) {
                Log.e(TAG, "SimContactProxyService: sim group cursor is null for " + iccGroupUri);
                return;
            }

            if (groupCursor.moveToFirst()) {
                do {
                    String groupIndex = groupCursor.getString(SIM_GROUP_FIELD.INDEX_INDEX);
                    String groupTitle = groupCursor.getString(SIM_GROUP_FIELD.NAME_INDEX);
                    ContentValues values = new ContentValues();
                    values.put(Groups.ACCOUNT_NAME, account.name);
                    values.put(Groups.ACCOUNT_TYPE, account.type);
                    values.put(Groups.TITLE, groupTitle);
                    values.put(Groups.SYNC1, groupIndex);
                    values.put(Groups.SYNC2, iccGroupUri);

                    Uri groupUri = cr.insert(
                            Groups.CONTENT_URI
                                    .buildUpon()
                                    .appendQueryParameter(ContactsContract.CALLER_IS_SYNCADAPTER,
                                            "true").build(), values);
                    long groupRowId = ContentUris.parseId(groupUri);

                    mSimContactGroupCache.addEntry(groupRowId, groupIndex, iccGroupUri, account);
                    Log.e(TAG, "add group: " + groupTitle + " " + groupIndex);
                } while (groupCursor.moveToNext());
            }
            groupCursor.close();

            /* sprd bug490245 read sne and aas for orange @{ */
            Cursor aasCursor = cr.query(Uri.parse(iccAasUri), SIM_AAS_PROJECTION,
                    null, null, null);
            if (aasCursor == null) {
                Log.e(TAG, "SimContactProxyService: sim aas cursor is null for " + iccAasUri);
                return;
            }

            Cursor sneCursor = cr.query(Uri.parse(iccSneUri), new String[]{"size"},
                    null, null, null);
            if (sneCursor == null) {
                Log.e(TAG, "SimContactProxyService: sim aas cursor is null for " + iccSneUri);
                return;
            }
            aasCursor.close();
            /* @} */

            // second, get sim contacts
            if (iccUri == null) {
                Log.e(TAG, "iccUri == null");
                return;
            }
            Cursor simCursor = null;
            try {
                simCursor = cr.query(Uri.parse(iccUri), SIM_PROJECTION, null, null, null);
            } catch (Exception e) {
                e.printStackTrace();
            }

            if (simCursor == null) {
                Log.e(TAG, "SimContactProxyService: sim cursor is null for " + iccUri);
                return;
            }

            ArrayList<ContentProviderOperation> ops = new ArrayList<ContentProviderOperation>();
            Map<Integer, String> rawContacts = new HashMap<Integer, String>();

            if (simCursor.moveToFirst()) {
                do {
                    String simIndex = simCursor.getString(SIM_CONTACT_FIELD.INDEX_INDEX);

                    Log.e(TAG, "import sim contact: " + simIndex);
                    ContentValues values = new ContentValues();
                    values.put(RawContacts.ACCOUNT_NAME, account.name);
                    values.put(RawContacts.ACCOUNT_TYPE, account.type);
                    values.put(RawContacts.AGGREGATION_MODE, RawContacts.AGGREGATION_MODE_DISABLED);
                    values.put(RawContacts.SYNC1, simIndex);
                    values.put(RawContacts.SYNC2, iccUri);

                    ContentProviderOperation rawContactOperation = ContentProviderOperation
                            .newInsert(
                                    RawContacts.CONTENT_URI
                                            .buildUpon()
                                            .appendQueryParameter(
                                                    ContactsContract.CALLER_IS_SYNCADAPTER, "true")
                                            .build())
                            .withValues(values).build();
                    ops.add(rawContactOperation);
                    rawContacts.put(ops.size() - 1, simIndex);
                    insertSimContactData(simCursor, ops, ops.size() - 1, iccGroupUri);
                    if (rawContacts.size() >= 100) {
                        try {
                            ContentProviderResult[] results = cr.applyBatch(
                                    ContactsContract.AUTHORITY,
                                    ops);
                        } catch (Exception e) {
                            e.printStackTrace();
                        } finally {
                            rawContacts.clear();
                            ops.clear();
                        }
                    }
                } while (simCursor.moveToNext());

                try {
                    ContentProviderResult[] results = cr
                            .applyBatch(ContactsContract.AUTHORITY, ops);
                } catch (Exception e) {
                    e.printStackTrace();
                } finally {
                    rawContacts.clear();
                    ops.clear();
                }
            }
            simCursor.close();

            Cursor sdnCursor = null;
            try {
                sdnCursor = cr.query(Uri.parse(iccSdnUri), SIM_SDN_PROJECTION, null, null,
                        null);
            } catch (Exception e) {
                e.printStackTrace();
            }
            if (sdnCursor == null) {
                Log.e(TAG, "SimContactProxyService: sdn cursor is null for " + sdnCursor);
                return;
            }

            if (sdnCursor.moveToFirst()) {
                do {
                    String sdnName = sdnCursor.getString(0);
                    ContentValues values = new ContentValues();
                    values.put(RawContacts.ACCOUNT_NAME, account.name);
                    values.put(RawContacts.ACCOUNT_TYPE, account.type);
                    values.put(RawContacts.AGGREGATION_MODE, RawContacts.AGGREGATION_MODE_DISABLED);
                    values.put(RawContacts.RAW_CONTACT_IS_READ_ONLY, 1);
                    values.put(RawContacts.SYNC1, "sdn");
                    values.put(RawContacts.SYNC2, iccSdnUri);

                    ContentProviderOperation rawContactOperation = ContentProviderOperation
                            .newInsert(
                                    RawContacts.CONTENT_URI
                                            .buildUpon()
                                            .appendQueryParameter(
                                                    ContactsContract.CALLER_IS_SYNCADAPTER, "true")
                                            .build())
                            .withValues(values).build();
                    ops.add(rawContactOperation);
                    rawContacts.put(ops.size() - 1, sdnName);
                    insertSimSdnContactData(sdnCursor, ops, ops.size() - 1);
                    if (rawContacts.size() >= 100) {
                        try {
                            ContentProviderResult[] results = cr.applyBatch(
                                    ContactsContract.AUTHORITY,
                                    ops);
                        } catch (Exception e) {
                            e.printStackTrace();
                        } finally {
                            rawContacts.clear();
                            ops.clear();
                        }
                    }
                } while (sdnCursor.moveToNext());
                try {
                    ContentProviderResult[] results = cr
                            .applyBatch(ContactsContract.AUTHORITY, ops);
                } catch (Exception e) {
                    e.printStackTrace();
                } finally {
                    rawContacts.clear();
                    ops.clear();
                }
            }

            Intent intent = new Intent(AppWidgetManager.ACTION_APPWIDGET_UPDATE);
            intent.putExtra("isContactsLoaded", true);
            mContext.sendBroadcast(intent);
            if (DEBUG)
                Log.d(TAG, "sendBroadcast to AppWidgetManager.ACTION_APPWIDGET_UPDATE");

            Log.e(TAG, "SimContactProxy: done for " + iccUri);
        /**
         * SPRD:Bug605491 Crash happen when storage is full
         * @{
         */
        } catch (SQLiteException e) {
            Log.e(TAG, "import error : " + e.getMessage());
        /**
         * @}
         */
        } finally {
            mImporting = false;
            mWakeLock.release();
        }
    }

    @Override
    public boolean isMyAccount(Account account) {
        if (account == null) {
            return false;
        }
        String accountType = account.type;
        if (accountType == null || !accountType.matches("sprd.*sim")) {
            return false;
        }
        return true;
    }
}
