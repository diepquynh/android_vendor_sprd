package com.android.internal.telephony;

import android.content.UriMatcher;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.MergeCursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.text.TextUtils;
import android.telephony.Rlog;
import android.content.Context;

import java.util.List;

import android.provider.ContactsContract.CommonDataKinds;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.GroupMembership;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import com.android.internal.telephony.uicc.AdnRecordEx;
import com.android.internal.telephony.uicc.IccConstantsEx;
import com.android.internal.telephony.IIccPhoneBookEx;

/**
 * {@hide}
 */
public class IccProviderExImpl extends AbsIccProvider {
    private static final String TAG = "IccProviderExImpl";
    private static final boolean DBG = true;

    private static final String[] ADDRESS_BOOK_COLUMN_NAMES = new String[] {
            "name", "number", "emails", "anr", "aas", "sne", "grp", "gas",
            "index", "_id" };

    private static final String[] FDN_S_COLUMN_NAMES = new String[] { "size" };
    private static final String[] SIM_GROUP_PROJECTION = new String[] { "gas", "index" };
    private static final String[] SIM_AAS_PROJECTION = new String[] { "aas", "index" };

    private static final int ADN = 1;
    private static final int ADN_SUB = 2;
    private static final int FDN = 3;
    private static final int FDN_SUB = 4;
    private static final int SDN = 5;
    private static final int SDN_SUB = 6;
    private static final int ADN_ALL = 7;
    private static final int FDN_S = 8;
    private static final int FDN_S_SUB = 9;
    private static final int GAS = 10;
    private static final int GAS_SUB = 11;
    private static final int LND = 12;
    private static final int LND_SUB = 13;
    private static final int AAS = 14;
    private static final int AAS_SUB = 15;
    private static final int SNE_S = 16;
    private static final int SNE_S_SUB = 17;

    private static final String STR_TAG = "tag";
    private static final String STR_NUMBER = "number";
    private static final String STR_EMAILS = "email";
    private static final String STR_PIN2 = "pin2";
    private static final String STR_ANR = "anr";
    private static final String STR_AAS = "aas";
    private static final String STR_SNE = "sne";
    private static final String STR_GRP = "grp";
    private static final String STR_GAS = "gas";
    private static final String STR_INDEX = "index";
    private static final String STR_NEW_TAG = "newTag";
    private static final String STR_NEW_NUMBER = "newNumber";

    private static final String AUTHORITY = "icc";
    private static final String CONTENT_URI = "content://" + AUTHORITY + "/";
    private static final String WITH_EXCEPTION = "with_exception";

    private static final UriMatcher URL_MATCHER = new UriMatcher(
            UriMatcher.NO_MATCH);

    static {
        URL_MATCHER.addURI(AUTHORITY, "adn", ADN);
        URL_MATCHER.addURI(AUTHORITY, "adn/subId/#", ADN_SUB);
        URL_MATCHER.addURI(AUTHORITY, "fdn", FDN);
        URL_MATCHER.addURI(AUTHORITY, "fdn/subId/#", FDN_SUB);
        URL_MATCHER.addURI(AUTHORITY, "sdn", SDN);
        URL_MATCHER.addURI(AUTHORITY, "sdn/subId/#", SDN_SUB);
        URL_MATCHER.addURI(AUTHORITY, "fdn_s", FDN_S);
        URL_MATCHER.addURI(AUTHORITY, "fdn_s/subId/#", FDN_S_SUB);
        URL_MATCHER.addURI(AUTHORITY, "gas", GAS);
        URL_MATCHER.addURI(AUTHORITY, "gas/subId/#", GAS_SUB);
        URL_MATCHER.addURI(AUTHORITY, "lnd", LND);
        URL_MATCHER.addURI(AUTHORITY, "lnd/subId/#", LND_SUB);
        URL_MATCHER.addURI(AUTHORITY, "aas", AAS);
        URL_MATCHER.addURI(AUTHORITY, "aas/subId/#", AAS_SUB);
        URL_MATCHER.addURI(AUTHORITY, "sne", SNE_S);
        URL_MATCHER.addURI(AUTHORITY, "sne/subId/#", SNE_S_SUB);
    }

    private SubscriptionManager mSubscriptionManager;
    private Context mContext;

    public IccProviderExImpl (Context context) {
        log("IccProviderExImpl");
        mContext = context;
        mSubscriptionManager = SubscriptionManager.from(mContext);
    }

    @Override
    public Cursor query(Uri url, String[] projection, String selection,
            String[] selectionArgs, String sort) {
        log("query");

        switch (URL_MATCHER.match(url)) {
        case ADN:
            return loadFromEf(IccConstantsEx.EF_ADN, SubscriptionManager.getDefaultSubscriptionId());

        case ADN_SUB:
            return loadFromEf(IccConstantsEx.EF_ADN, getRequestSubId(url));

        case FDN:
            return loadFromEf(IccConstantsEx.EF_FDN, SubscriptionManager.getDefaultSubscriptionId());

        case FDN_SUB:
            return loadFromEf(IccConstantsEx.EF_FDN, getRequestSubId(url));

        case SDN:
            return loadFromEf(IccConstantsEx.EF_SDN, SubscriptionManager.getDefaultSubscriptionId());

        case SDN_SUB:
            return loadFromEf(IccConstantsEx.EF_SDN, getRequestSubId(url));

        case ADN_ALL:
            return loadAllSimContacts(IccConstantsEx.EF_ADN);

        case FDN_S:
            return getEfSize(IccConstantsEx.EF_FDN, SubscriptionManager.getDefaultSubscriptionId());

        case FDN_S_SUB:
            return getEfSize(IccConstantsEx.EF_FDN, getRequestSubId(url));

        case LND:
            return loadFromEf(IccConstantsEx.EF_LND, SubscriptionManager.getDefaultSubscriptionId());

        case LND_SUB:
            return loadFromEf(IccConstantsEx.EF_LND, getRequestSubId(url));

        case GAS:
            return loadGas(SubscriptionManager.getDefaultSubscriptionId());

        case GAS_SUB:
            return loadGas(getRequestSubId(url));

        case AAS:
            return loadAas(SubscriptionManager.getDefaultSubscriptionId());

        case AAS_SUB:
            return loadAas(getRequestSubId(url));

        case SNE_S:
            return getSneSize(SubscriptionManager.getDefaultSubscriptionId());

        case SNE_S_SUB:
            return getSneSize(getRequestSubId(url));

        default:
            throw new IllegalArgumentException("Unknown URL " + url);
        }
    }

    private Cursor loadAllSimContacts(int efType) {
        Cursor[] result;
        List<SubscriptionInfo> subInfoList = mSubscriptionManager.getActiveSubscriptionInfoList();

        if ((subInfoList == null) || (subInfoList.size() == 0)) {
            result = new Cursor[0];
        } else {
            int subIdCount = subInfoList.size();
            result = new Cursor[subIdCount];
            int subId;

            for (int i = 0; i < subIdCount; i++) {
                subId = subInfoList.get(i).getSubscriptionId();
                result[i] = loadFromEf(efType, subId);
                log("ADN Records loaded for Subscription ::" + subId);
            }
        }
        return new MergeCursor(result);
    }

    @Override
    public String getType(Uri url) {
        switch (URL_MATCHER.match(url)) {
        case ADN:
        case ADN_SUB:
        case FDN:
        case FDN_SUB:
        case SDN:
        case SDN_SUB:
        case AAS:
        case AAS_SUB:
        case ADN_ALL:
            return "vnd.android.cursor.dir/sim-contact";

        default:
            throw new IllegalArgumentException("Unknown URL " + url);
        }
    }

    @Override
    public Uri insert(Uri url, ContentValues initialValues) {
        Uri resultUri;
        int efType = -1;
        boolean isGas = false;
        int index = -1;
        String pin2 = null;
        int subId;
        boolean isAas = false;

        log("insert, uri:" + url + " initialValues:" + initialValues);

        int match = URL_MATCHER.match(url);
        switch (match) {
        case ADN:
            efType = IccConstantsEx.EF_ADN;
            subId = SubscriptionManager.getDefaultSubscriptionId();
            break;

        case ADN_SUB:
            efType = IccConstantsEx.EF_ADN;
            subId = getRequestSubId(url);
            break;

        case FDN:
            efType = IccConstantsEx.EF_FDN;
            subId = SubscriptionManager.getDefaultSubscriptionId();
            pin2 = initialValues.getAsString("pin2");
            break;

        case FDN_SUB:
            efType = IccConstantsEx.EF_FDN;
            subId = getRequestSubId(url);
            pin2 = initialValues.getAsString("pin2");
            break;

        case LND:
            efType = IccConstantsEx.EF_LND;
            subId = SubscriptionManager.getDefaultSubscriptionId();
            break;

        case LND_SUB:
            efType = IccConstantsEx.EF_LND;
            subId = getRequestSubId(url);
            break;

        case GAS:
            isGas = true;
            subId = SubscriptionManager.getDefaultSubscriptionId();
            break;

        case GAS_SUB:
            isGas = true;
            subId = getRequestSubId(url);
            break;

        case AAS:
            isAas = true;
            subId = SubscriptionManager.getDefaultSubscriptionId();
            break;

        case AAS_SUB:
            isAas = true;
            subId = getRequestSubId(url);
            break;

        default:
            throw new UnsupportedOperationException("Cannot insert into URL: "
                    + url);
        }

        String tag = initialValues.getAsString("tag");
        String number = initialValues.getAsString("number");
        String mEmail = initialValues.getAsString("email");
        String[] emails = null;
        if (mEmail != null) {
            emails = new String[1];
            emails[0] = mEmail;
        }

        String anr = initialValues.getAsString("anr");
        String aas = initialValues.getAsString("aas");
        String sne = initialValues.getAsString("sne");
        String grp = initialValues.getAsString("grp");
        String gas = initialValues.getAsString("gas");
        aas = (aas == null) ? "" : aas;

        log("insert, tag:" + tag + ",  number:" + number + ",  anr:" + anr
                    + ",  aas:" + aas + ",  sne:" + sne + ",  grp:" + grp
                    + ",  gas:" + gas + ",  Email:"
                    + (emails == null ? "null" : emails[0]));

        if (isGas) {
            index = addUsimGroupBySearch(gas, subId);
        } else if (isAas) {
            index = addUsimAas(aas, subId);
            if (index < 0) {
                if (index == IccPhoneBookOperationException.AAS_CAPACITY_FULL) {
                    return Uri.parse("aas/aas_full");
                } else if (index == IccPhoneBookOperationException.OVER_AAS_MAX_LENGTH) {
                    return Uri.parse("aas/over_aas_max_length");
                }
                return null;
            }
        } else {
            index = addIccRecordToEf(efType, tag, number, emails, anr, aas,
                    sne, grp, gas, pin2, subId);
        }
        if (index < 0) {
            int errorCode = index;
            log("insert error =  " + errorCode);
            if (url.getBooleanQueryParameter(WITH_EXCEPTION, false)) {
                log("throw exception");
                throwException(errorCode);
            }
            return null;
        }

        StringBuilder buf = new StringBuilder("content://icc/");
        switch (match) {
        case ADN:
            buf.append("adn/");
            break;

        case ADN_SUB:
            buf.append("adn/subId/");
            break;

        case FDN:
            buf.append("fdn/");
            break;

        case FDN_SUB:
            buf.append("fdn/subId/");
            break;

        case LND:
            buf.append("lnd/");
            break;

        case LND_SUB:
            buf.append("lnd/subId");
            break;

        case GAS:
            buf.append("gas/");
            break;

        case GAS_SUB:
            buf.append("gas/subId");
            break;

        case AAS:
            buf.append("aas/");
            break;

        case AAS_SUB:
            buf.append("aas/subId");
            break;
        }

        // TODO: we need to find out the rowId for the newly added record
        /* SPRD: update for bug 624394 @{ */
        if (match == FDN_SUB) {
            log("match value is fdn sub");
            buf.append(subId);
        } else {
            buf.append("/" + index);
        }
        /*@}*/
        resultUri = Uri.parse(buf.toString());

        log("insert resultUri  " + resultUri);
        if (resultUri != null) {
            mContext.getContentResolver().notifyChange(resultUri, null, false);
        }

        return resultUri;
    }

    private String normalizeValue(String inVal) {
        int len = inVal.length();
        // If name is empty in contact return null to avoid crash.
        if (len == 0) {
            log("len of input String is 0");
            return inVal;
        }
        String retVal = inVal;

        if (inVal.charAt(0) == '\'' && inVal.charAt(len - 1) == '\'') {
            retVal = inVal.substring(1, len - 1);
        }

        return retVal;
    }

    @Override
    public int delete(Uri url, String where, String[] whereArgs) {
        int efType;
        int subId;
        boolean isFdn = false;
        boolean isGas = false;
        boolean isAas = false;
        log("delete, uri:" + url + " where:" + where + " whereArgs:" + whereArgs);

        int match = URL_MATCHER.match(url);
        switch (match) {
        case ADN:
            efType = IccConstantsEx.EF_ADN;
            subId = SubscriptionManager.getDefaultSubscriptionId();
            break;

        case ADN_SUB:
            efType = IccConstantsEx.EF_ADN;
            subId = getRequestSubId(url);
            break;

        case FDN:
            efType = IccConstantsEx.EF_FDN;
            subId = SubscriptionManager.getDefaultSubscriptionId();
            isFdn = true;
            break;

        case FDN_SUB:
            efType = IccConstantsEx.EF_FDN;
            subId = getRequestSubId(url);
            isFdn = true;
            break;

        case GAS:
            efType = IccConstantsEx.EF_ADN;
            subId = SubscriptionManager.getDefaultSubscriptionId();
            isGas = true;
            break;

        case GAS_SUB:
            efType = IccConstantsEx.EF_ADN;
            subId = getRequestSubId(url);
            isGas = true;
            break;

        case AAS:
            efType = IccConstantsEx.EF_ADN;
            subId = SubscriptionManager.getDefaultSubscriptionId();
            isAas = true;
            break;

        case AAS_SUB:
            efType = IccConstantsEx.EF_ADN;
            subId = getRequestSubId(url);
            isAas = true;
            break;

        default:
            throw new UnsupportedOperationException("Cannot insert into URL: "
                    + url);
        }

        // parse where clause
        String tag = null;
        String number = null;
        String[] emails = null;
        String pin2 = null;
        int index = -1;
        boolean success = false;

        if (isAas) {
            log("delete AAS");
            index = Integer.parseInt(where);
            int result = updateUsimAasByIndex("", index, subId);
            return result < 0 ? 0 : 1;
        }

        if (whereArgs == null || whereArgs.length == 0) {
            String[] tokens = where.split("AND");
            int n = tokens.length;
            while (--n >= 0) {
                String param = tokens[n];
                log("parsing '" + param + "'");

                String[] pair = param.split("=");

                if (pair.length != 2) {
                    log("resolve: bad whereClause parameter: " + param);
                    continue;
                }
                String key = pair[0].trim();
                String val = pair[1].trim();

                if (STR_TAG.equals(key)) {
                    tag = normalizeValue(val);
                } else if (STR_NUMBER.equals(key)) {
                    number = normalizeValue(val);
                } else if (STR_EMAILS.equals(key)) {
                    // TODO(): Email is null.
                    emails = null;
                } else if (STR_PIN2.equals(key)) {
                    pin2 = normalizeValue(val);
                } else if (STR_INDEX.equals(key)) {
                    index = Integer.valueOf(normalizeValue(val));
                } else if (STR_INDEX.equals(key)) {
                    index = Integer.valueOf(normalizeValue(val));
                }
            }
        } else {
            tag = whereArgs[0];
            number = whereArgs[1];
            pin2 = whereArgs[2];
        }

        if (efType == FDN && TextUtils.isEmpty(pin2)) {
            return 0;
        }
        log("delete tag: " + tag + ", number:" + number + ", index:" + index);

        if (isFdn) {
            if (-1 != deleteIccRecordFromEf(efType, tag, number, null, "", "",
                    "", pin2, subId)) {
                success = true;
            }
        } else if (isGas) {
            int result = updateUsimGroupByIndex("", index, subId);
            success = result < 0 ? false : true;
        } else {
            // use the default method to delete iccRecord,because the 3rd app
            // will not use index
            if (index == -1) {
                log("the 3rd app will not use index");
                success = deleteIccRecordFromEf(efType, tag, number, null, pin2, subId);
            } else {
                int recIndex = -1;
                recIndex = deleteIccRecordFromEfByIndex(efType, index, pin2, subId);
                if (recIndex < 0) {
                    success = false;
                } else {
                    success = true;
                }
            }
        }
        log("delete result: " + success);

        if (!success) {
            return 0;
        } else {
            mContext.getContentResolver().notifyChange(url, null);
        }
        return 1;
    }

    @Override
    public int update(Uri url, ContentValues values, String where,
            String[] whereArgs) {
        String pin2 = null;
        int efType = -1;
        int subId;
        boolean isFdn = false;
        boolean isGas = false;
        boolean isAas = false;
        log("update, uri:" + url + " where: " + where + " value: " + values);

        int match = URL_MATCHER.match(url);
        switch (match) {
        case ADN:
            efType = IccConstantsEx.EF_ADN;
            subId = SubscriptionManager.getDefaultSubscriptionId();
            break;

        case ADN_SUB:
            efType = IccConstantsEx.EF_ADN;
            subId = getRequestSubId(url);
            break;

        case FDN:
            efType = IccConstantsEx.EF_FDN;
            subId = SubscriptionManager.getDefaultSubscriptionId();
            pin2 = values.getAsString("pin2");
            isFdn = true;
            break;

        case FDN_SUB:
            efType = IccConstantsEx.EF_FDN;
            subId = getRequestSubId(url);
            pin2 = values.getAsString("pin2");
            isFdn = true;
            break;

        case GAS:
            subId = SubscriptionManager.getDefaultSubscriptionId();
            isGas = true;
            break;

        case GAS_SUB:
            subId = getRequestSubId(url);
            isGas = true;
            break;

        case AAS:
            subId = SubscriptionManager.getDefaultSubscriptionId();
            isAas = true;
            break;

        case AAS_SUB:
            subId = getRequestSubId(url);
            isAas = true;
            break;

        default:
            throw new UnsupportedOperationException("Cannot insert into URL: " + url);
        }

        String[] emails = null;
        String newTag = values.getAsString(STR_TAG);
        String newNumber = values.getAsString(STR_NUMBER);
        Integer index = values.getAsInteger(STR_INDEX); // maybe simIndex or
        // groupId
        String newanr = values.getAsString(STR_ANR);
        String newaas = values.getAsString(STR_AAS);
        String newsne = values.getAsString(STR_SNE);
        String newgrp = values.getAsString(STR_GRP);
        String newgas = values.getAsString(STR_GAS);
        String[] newemails = null;
        String newEmail = values.getAsString(STR_EMAILS);
        if (newEmail != null) {
            newemails = new String[1];
            newemails[0] = newEmail;
        }

        newaas = newaas == null ? "" : newaas;
        log("update, new tag: " + newTag + ",  number:" + newNumber
                    + ",  anr:" + newanr + ",  aas: " + newaas + ",  sne:"
                    + newsne + ",  grp:" + newgrp + ",  gas :" + newgas
                    + ",  email:" + newEmail + ",  index:" + index);

        boolean success = false;
        int recIndex = -1;
        if (isFdn) {
            String tag = "";
            String number = "";
            tag = values.getAsString(STR_TAG);
            number = values.getAsString(STR_NUMBER);
            newTag = values.getAsString(STR_NEW_TAG);
            newNumber = values.getAsString(STR_NEW_NUMBER);
            if (0 <= updateIccRecordInEf(efType, tag, number, null, "", "", "",
                    newTag, newNumber, null, "", "", "", "", "", pin2, subId))
                success = true;
        } else if (isGas) {
            recIndex = updateUsimGroupByIndex(newgas, index, subId);
            if (recIndex < 0) {
                success = false;
                if (url.getBooleanQueryParameter(WITH_EXCEPTION, false)) {
                    log("throw exception :recIndex = " + recIndex);
                    throwException(recIndex);
                }
            } else {
                success = true;
            }
        } else if (isAas) {
            recIndex = updateUsimAasByIndex(newaas, index, subId);
            return recIndex < 0 ? 0 : 1;
        } else {
            recIndex = updateIccRecordInEfByIndex(efType, newTag, newNumber,
                    newemails, newanr, newaas, newsne, newgrp, newgas, index,
                    pin2, subId);
            if (recIndex < 0) {
                success = false;
                if (url.getBooleanQueryParameter(WITH_EXCEPTION, false)) {
                    log("throw exception :recIndex = " + recIndex);
                    throwException(recIndex);
                }
            } else {
                success = true;
            }
        }
        if (!success) {
            return 0;
        } else {
            mContext.getContentResolver().notifyChange(url, null);
        }

        return 1;
    }

    private int getRequestSubId(Uri url) {
        log("getRequestSubId url: " + url);

        try {
            return Integer.parseInt(url.getLastPathSegment());
        } catch (NumberFormatException ex) {
            throw new IllegalArgumentException("Unknown URL " + url);
        }
    }

    private MatrixCursor loadFromEf(int efType, int subId) {
        log("loadFromEf: efType = 0x" +
                Integer.toHexString(efType).toUpperCase() + ", subId = " + subId);

        List<AdnRecordEx> adnRecords = null;
        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                log("iccIpb = " + iccIpb);
                adnRecords = iccIpb.getAdnRecordsInEfForSubscriber(subId, efType);
            }
        } catch (RemoteException ex) {
            log("RemoteException " + ex.toString());
        } catch (SecurityException ex) {
            log("SecurityException " + ex.toString());
        }

        if (adnRecords != null) {
            final int N = adnRecords.size();
            final MatrixCursor cursor = new MatrixCursor(ADDRESS_BOOK_COLUMN_NAMES, N);
            log("adnRecords.size = " + N);
            for (int i = 0; i < N; i++) {
                loadRecord(adnRecords.get(i), cursor, i);
            }
            return cursor;
        } else {
            log("Cannot load ADN records efType = 0x" +
                Integer.toHexString(efType).toUpperCase() + ", subId=" + subId);
            return new MatrixCursor(ADDRESS_BOOK_COLUMN_NAMES);
        }
    }

    private boolean deleteIccRecordFromEf(int efType, String name,
            String number, String[] emails, String pin2, int subId) {
        log("deleteIccRecordFromEf: efType = 0x" + Integer.toHexString(efType).toUpperCase()
                + ", name=" + name + ", number=" + number + ", emails=" + emails + ", pin2="
                + pin2 + ", subscription=" + subId);

        boolean success = false;

        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                success = iccIpb.updateAdnRecordsInEfBySearchForSubscriber(
                        subId, efType, name, number, "", "", pin2);
            }
        } catch (RemoteException ex) {
            log("RemoteException " + ex.toString());
        } catch (SecurityException ex) {
            log("SecurityException " + ex.toString());
        }
        log("deleteIccRecordFromEf: " + success);
        return success;
    }

    private int addIccRecordToEf(int efType, String name, String number,
            String[] emails, String anr, String aas, String sne, String grp,
            String gas, String pin2, int subId) {
        log("addIccRecordToEf: efType = 0x" + Integer.toHexString(efType).toUpperCase()
                + ", name=" + name + ", number=" + number + ", emails=" + emails + ", grp="
                + grp + ", anr= " + anr + ", subscription=" + subId);

        int retIndex = -1;

        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                retIndex = iccIpb.updateAdnRecordsInEfBySearchForSubscriberEx(
                        subId, efType, "", "", null, "", "", "", name, number,
                        emails, anr, aas, sne, grp, gas, pin2);
            }
        } catch (RemoteException ex) {
            log("RemoteException " + ex.toString());
        } catch (SecurityException ex) {
            log("SecurityException " + ex.toString());
        }

        log("addIccRecordToEf: " + retIndex);
        return retIndex;
    }

    private int deleteIccRecordFromEf(int efType, String name, String number,
            String[] emails, String anr, String sne, String grp, String pin2,
            int subId) {
        log("deleteIccRecordFromEf: efType = 0x" + Integer.toHexString(efType).toUpperCase()
                + ", name=" + name + ", number=" + number + ",anr=" + anr + ", pin2=" + pin2);

        int retIndex = -1;

        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                retIndex = iccIpb.updateAdnRecordsInEfBySearchForSubscriberEx(
                        subId, efType, name, number, emails, anr, sne, grp, "",
                        "", null, "", "", "", "", "", pin2);

            }
        } catch (RemoteException ex) {
            log("RemoteException " + ex.toString());
        } catch (SecurityException ex) {
            log("SecurityException " + ex.toString());
        }
        log("deleteIccRecordFromEf: " + retIndex);
        return retIndex;
    }

    private int deleteIccRecordFromEfByIndex(int efType, int index,
            String pin2, int subId) {
        log("deleteIccRecordFromEfByIndex: efType = 0x" + Integer.toHexString(efType).toUpperCase()
                + ", index=" + index + ", pin2 = " + pin2);

        boolean success = false;
        int recIndex = -1;

        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                recIndex = iccIpb.updateAdnRecordsInEfByIndexForSubscriber(
                        subId, efType, "", "", null, "", "", "", "", "", index,
                        pin2);

            }
        } catch (RemoteException ex) {
            log("RemoteException " + ex.toString());
        } catch (SecurityException ex) {
            log("SecurityException " + ex.toString());
        }
        if (recIndex < 0) {
            success = false;
        } else {
            success = true;
        }
        log("deleteIccRecordFromEfByIndex: " + success + " recIndex = "
                    + recIndex);
        return recIndex;
    }

    private int updateIccRecordInEf(int efType, String oldName,
            String oldNumber, String[] oldEmailList, String oldAnr,
            String oldSne, String oldGrp, String newName, String newNumber,
            String[] newEmailList, String newAnr, String newAas, String newSne,
            String newGrp, String newGas, String pin2, int subId) {
        log("updateIccRecordInEf: efType = 0x" + Integer.toHexString(efType).toUpperCase()
                    + ",  oldname = " + oldName + ",  oldnumber = " + oldNumber
                    + ",  oldAnr = " + oldAnr + ",  newname = " + newName
                    + ",  newnumber = " + newNumber + ",  newAnr = " + newAnr);

        int retIndex = -1;

        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                retIndex = iccIpb.updateAdnRecordsInEfBySearchForSubscriberEx(
                        subId, efType, oldName, oldNumber, oldEmailList,
                        oldAnr, oldSne, oldGrp, newName, newNumber,
                        newEmailList, newAnr, newAas, newSne, newGrp, newGas,
                        pin2);
            }
        } catch (RemoteException ex) {
            log("RemoteException " + ex.toString());
        } catch (SecurityException ex) {
            log("SecurityException " + ex.toString());
        }
        log("updateIccRecordInEf: " + retIndex);
        return retIndex;
    }

    private int updateIccRecordInEfByIndex(int efType, String newName,
            String newNumber, String[] newEmailList, String newAnr,
            String newAas, String newSne, String newGrp, String newGas,
            int simIndex, String pin2, int subId) {
        log("updateIccRecordInEfByIndex: efType = 0x" + Integer.toHexString(efType).toUpperCase() + ", newname="
                    + newName + ", newnumber=" + newNumber + ", newEmailList="
                    + newEmailList + ", newAnr=" + newAnr + ", newSne="
                    + newSne + ", index=" + simIndex + ", subId:" + subId);

        int recIndex = -1;
        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                recIndex = iccIpb.updateAdnRecordsInEfByIndexForSubscriber(
                        subId, efType, newName, newNumber, newEmailList,
                        newAnr, newAas, newSne, newGrp, newGas, simIndex, pin2);
            }
        } catch (RemoteException ex) {
            log("RemoteException " + ex.toString());
        } catch (SecurityException ex) {
            log("SecurityException " + ex.toString());
        }
        log("updateIccRecordInEfByIndex: " + recIndex);
        return recIndex;
    }

    /**
     * Loads an AdnRecord into a MatrixCursor. Must be called with mLock held.
     *
     * @param record
     *            the ADN record to load from
     * @param cursor
     *            the cursor to receive the results
     */
    private void loadRecord(AdnRecordEx record, MatrixCursor cursor, int id) {
        if (!record.isEmpty()) {
            Object[] contact = new Object[ADDRESS_BOOK_COLUMN_NAMES.length];

            String alphaTag = record.getAlphaTag();
            String number = record.getNumber();
            String[] emails = record.getEmails();
            String anr = record.getAnr();
            String aas = record.getAas();
            String sne = record.getSne();
            String grp = record.getGrp();
            String gas = record.getGas();

            contact[0] = alphaTag;
            contact[1] = number;
            log("loadRecord: " + alphaTag + ", " + number + "," + anr
                        + ", " + aas + ", " + sne + ", " + grp + ", " + gas);

            String sim_index = String.valueOf(record.getRecId());
            log("loadRecord::sim_index = " + sim_index);
            if (emails != null) {
                StringBuilder emailString = new StringBuilder();
                for (String email : emails) {
                    emailString.append(email);
                    break;
                }
                contact[2] = emailString.toString();
            }
            contact[3] = anr;
            contact[4] = aas;
            contact[5] = sne;
            contact[6] = grp;
            contact[7] = gas;
            contact[8] = sim_index;
            contact[9] = id;
            cursor.addRow(contact);
        }
    }

    /** get the size of FDN. maybe useless **/
    private MatrixCursor getEfSize(int efType, int subId) {
        int[] adnRecordSize = null;
        log("getEfSize: efType = 0x" + Integer.toHexString(efType).toUpperCase());

        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                adnRecordSize = iccIpb.getAdnRecordsSizeForSubscriber(subId, efType);
            }
        } catch (RemoteException ex) {
            log("RemoteException " + ex.toString());
        } catch (SecurityException ex) {
            log("SecurityException " + ex.toString());
        }
        if (adnRecordSize != null) {
            // Load the results
            MatrixCursor cursor = new MatrixCursor(FDN_S_COLUMN_NAMES, 1);
            Object[] size = new Object[1];
            size[0] = adnRecordSize[2];
            cursor.addRow(size);
            return cursor;
        } else {
            log("Cannot load ADN records");
            return new MatrixCursor(FDN_S_COLUMN_NAMES);
        }
    }

    /** load GAS from sim card. maybe useless **/
    private MatrixCursor loadGas(int subId) {
        log("loadGas,subId = " + subId);

        List<String> adnGas = null;
        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                adnGas = iccIpb.getGasInEfForSubscriber(subId);
            }
        } catch (RemoteException ex) {
            log("RemoteException " + ex.toString());
        } catch (SecurityException ex) {
            log("SecurityException " + ex.toString());
        }

        if (adnGas != null) {
            final int N = adnGas.size();
            final MatrixCursor cursor = new MatrixCursor(SIM_GROUP_PROJECTION, N);
            log("adnGas.size=" + N);
            for (int i = 0; i < N; i++) {
                if (TextUtils.isEmpty(adnGas.get(i)))
                    continue;

                Object[] group = new Object[SIM_GROUP_PROJECTION.length];
                group[0] = adnGas.get(i);
                group[1] = i + 1;
                cursor.addRow(group);
                log("loadGas: " + group[1] + ", " + group[0]);
            }
            return cursor;
        } else {
            log("Cannot load Gas records");
            return new MatrixCursor(SIM_GROUP_PROJECTION);
        }
    }

    private int addUsimGroupBySearch(String groupName, int subId) {
        log("addUsimGroup: groupName = " + groupName + ", subId = " + subId);
        int groupId = -1;

        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                groupId = iccIpb.updateUsimGroupBySearchForSubscriber(subId, "", groupName);
            }

        } catch (RemoteException ex) {
            log("RemoteException " + ex.toString());
        } catch (SecurityException ex) {
            log("SecurityException " + ex.toString());
        }
        log("addUsimGroup: " + groupId);
        return groupId;
    }

    private int updateUsimGroupByIndex(String newName, int groupId, int subId) {
        log("updateUsimGroupByIndex: newName = " + newName + ", groupId = "
                    + groupId + ", subId = " + subId);
        boolean success = false;
        int result = -1;
        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                result = iccIpb.updateUsimGroupByIndexForSubscriber(subId,
                        newName, groupId);
            }
        } catch (RemoteException ex) {
            log("RemoteException " + ex.toString());
        } catch (SecurityException ex) {
            log("SecurityException " + ex.toString());
        }
        success = result < 0 ? false : true;
        log("updateUsimGroupByIndex: " + success);
        return result;
    }

    private void throwException(int errorCode) {
        switch (errorCode) {
        case IccPhoneBookOperationException.WRITE_OPREATION_FAILED:
            throw new IccPBForRecordException(
                    IccPBForRecordException.WRITE_RECORD_FAILED,
                    "write record failed");

        case IccPhoneBookOperationException.ADN_CAPACITY_FULL:
            throw new IccPBForRecordException(
                    IccPBForRecordException.ADN_RECORD_CAPACITY_FULL,
                    "adn record capacity full");

        case IccPhoneBookOperationException.EMAIL_CAPACITY_FULL:
            throw new IccPBForMimetypeException(
                    IccPBForMimetypeException.CAPACITY_FULL,
                    Email.CONTENT_ITEM_TYPE, "email capacity full");

        case IccPhoneBookOperationException.LOAD_ADN_FAIL:
            throw new IccPBForRecordException(
                    IccPBForRecordException.LOAD_RECORD_FAILED,
                    "load adn failed");

        case IccPhoneBookOperationException.OVER_NAME_MAX_LENGTH:
            throw new IccPBForMimetypeException(
                    IccPBForMimetypeException.OVER_LENGTH_LIMIT,
                    CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE,
                    "over the length of name ");

        case IccPhoneBookOperationException.OVER_NUMBER_MAX_LENGTH:
            throw new IccPBForMimetypeException(
                    IccPBForMimetypeException.OVER_LENGTH_LIMIT,
                    Phone.CONTENT_ITEM_TYPE, "over the length of phone number");

        case IccPhoneBookOperationException.OVER_GROUP_NAME_MAX_LENGTH:
            throw new IccPBForMimetypeException(
                    IccPBForMimetypeException.OVER_LENGTH_LIMIT,
                    GroupMembership.CONTENT_ITEM_TYPE,
                    "over the length of group name");

        case IccPhoneBookOperationException.GROUP_CAPACITY_FULL:
            throw new IccPBForMimetypeException(
                    IccPBForMimetypeException.CAPACITY_FULL,
                    GroupMembership.CONTENT_ITEM_TYPE, "group capacity full");

        case IccPhoneBookOperationException.ANR_CAPACITY_FULL:
            throw new IccPBForRecordException(
                    IccPBForRecordException.ANR_RECORD_CAPACITY_FULL,
                    "anr record capacity full");

        case IccPhoneBookOperationException.AAS_CAPACITY_FULL:
            throw new IccPBForRecordException(
                    IccPBForRecordException.AAS_CAPACITY_FULL,
                    "aas capacity full");

        case IccPhoneBookOperationException.OVER_AAS_MAX_LENGTH:
            throw new IccPBForRecordException(
                    IccPBForRecordException.OVER_AAS_MAX_LENGTH,
                    "over the length of aas");

        case IccPhoneBookOperationException.GRP_RECORD_MAX_LENGTH:
            throw new IccPBForRecordException(
                    IccPBForRecordException.OVER_GRP_MAX_LENGTH,
                    "over the length of grp");
        default:
            break;
        }
    }

    private void log(String msg) {
        if(DBG) Rlog.d(TAG, msg);
    }

    private MatrixCursor loadAas(int subId) {
        log("loadAas, subId = " + subId);
        List<String> adnAas = null;
        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                adnAas = iccIpb.getAasInEfForSubscriber(subId);
            }
        } catch (RemoteException ex) {
            // ignore it
        } catch (SecurityException ex) {
            log(ex.toString());
        }

        if (adnAas != null) {
            // Load the results
            final int N = adnAas.size();
            final MatrixCursor cursor = new MatrixCursor(SIM_AAS_PROJECTION, N);
            log("adnAas.size = " + N);
            for (int i = 0; i < N; i++) {
                if (TextUtils.isEmpty(adnAas.get(i)))
                    continue;

                Object[] aas = new Object[SIM_AAS_PROJECTION.length];
                aas[0] = adnAas.get(i);
                aas[1] = i + 1;
                cursor.addRow(aas);
                log("loadAas: " + aas[1] + ", " + aas[0]);
            }
            return cursor;
        } else {
            // No results to load
            log("Cannot load Aas records");
            return new MatrixCursor(SIM_AAS_PROJECTION);
        }
    }

    private int addUsimAas(String aas, int subId) {
        log("addUsimAas: aas = " + aas + ", subId = " + subId);
        int aasIndex = -1;

        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                aasIndex = iccIpb.updateUsimAasBySearchForSubscriber("", aas,
                        subId);
            }

        } catch (RemoteException ex) {
            // ignore it
        } catch (SecurityException ex) {
            log(ex.toString());
        }
        log("addUsimAas: " + aasIndex);
        return aasIndex;
    }

    private int updateUsimAasByIndex(String newName, int aasIndex, int subId) {
            log("updateUsimAasByIndex: newName = " + newName + ", aasIndex = "
                    + aasIndex + ", subId = " + subId);
        boolean success = false;
        int result = -1;
        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                result = iccIpb.updateUsimAasByIndexForSubscriber(newName,
                        aasIndex, subId);
            }
        } catch (RemoteException ex) {
            // ignore it
        } catch (SecurityException ex) {
                log(ex.toString());
        }
        success = result < 0 ? false : true;
        log("updateUsimAasByIndex: " + success);
        return result;
    }

    private MatrixCursor getSneSize(int subId) {
        log("getSneSize,subId = " + subId);

        int senSize = 0;
        try {
            IIccPhoneBookEx iccIpb = IIccPhoneBookEx.Stub.asInterface(ServiceManager.getService("simphonebookEx"));
            if (iccIpb != null) {
                senSize = iccIpb.getSneSize(subId);
            }
        } catch (RemoteException ex) {
            // ignore it
        } catch (SecurityException ex) {
            log(ex.toString());
        }
        // Load the results
        final MatrixCursor cursor = new MatrixCursor(new String[] { "size" });
        cursor.addRow(new Object[] { senSize });
        return cursor;
    }
}
