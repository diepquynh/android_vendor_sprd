package com.android.internal.telephony.uicc;

import android.content.res.Resources;
import android.os.AsyncResult;
import android.os.Message;
import android.telephony.Rlog;
import android.text.TextUtils;
import android.util.SparseArray;

import java.util.ArrayList;
import java.util.Iterator;

import com.android.internal.telephony.EncodeException;
import com.android.internal.telephony.IccPhoneBookOperationException;
import com.android.internal.telephony.gsm.UsimPhoneBookManager;
import com.android.internal.telephony.GsmAlphabetEx;
import com.android.internal.telephony.gsm.UsimPhoneBookManagerEx;
import com.android.internal.telephony.TeleFrameworkFactory;

/**
 * {@hide}
 */
public class AdnRecordCacheEx extends AdnRecordCache implements IccConstantsEx {
    private static String TAG = "AdnRecordCacheEx";
    private static final boolean DBG = true;

    public int mInsertId = -1;
    private IccFileHandler mFh;
    private UsimPhoneBookManager mUsimPhoneBookManager;

    // Indexed by EF ID
    SparseArray<ArrayList<AdnRecordEx>> mAdnLikeFiles =
            new SparseArray<ArrayList<AdnRecordEx>>();
    SparseArray<ArrayList<byte[]>> mExtLikeFiles =
            new SparseArray<ArrayList<byte[]>>();

    static final int EVENT_UPDATE_USIM_ADN_DONE = 3;
    static final int EVENT_UPDATE_CYCLIC_DONE = 4;
    static final int EVENT_UPDATE_ANR_DONE = 5;
    static final int EVENT_LOAD_ALL_EXT_LIKE_DONE = 6;
    static final int EVENT_UPDATE_EXT_DONE = 7;
    static final int EVENT_UPDATE_SNE_DONE = 8;

    AdnRecordCacheEx(IccFileHandler fh) {
        super(fh);
        mFh = fh;
        mUsimPhoneBookManager = new UsimPhoneBookManagerEx(mFh, this);
    }

    /**
     * Called from SIMRecords.onRadioNotAvailable and
     * SIMRecords.handleSimRefresh.
     */
    public void reset() {
        log("reset adnLikeFiles");
        mExtLikeFiles.clear();
        mAdnLikeFiles.clear();
        mUsimPhoneBookManager.reset();
        clearWaiters();
        clearUserWriters();
    }

    private void clearWaiters() {
        int size = mAdnLikeWaiters.size();
        for (int i = 0; i < size; i++) {
            ArrayList<Message> waiters = mAdnLikeWaiters.valueAt(i);
            AsyncResult ar = new AsyncResult(null, null, new RuntimeException("AdnCache reset"));
            notifyWaiters(waiters, ar);
        }
        mAdnLikeWaiters.clear();
    }

    private void clearUserWriters() {
        int size = mUserWriteResponse.size();
        for (int i = 0; i < size; i++) {
            sendErrorResponse(mUserWriteResponse.valueAt(i), "AdnCace reset");
        }
        mUserWriteResponse.clear();
    }

    /**
     * @return List of AdnRecordExs for efid if we've already loaded them this
     *         radio session, or null if we haven't
     */
    public ArrayList<AdnRecordEx> getRecordsIfLoadedEx(int efid) {
        return mAdnLikeFiles.get(efid);
    }

    /**
     * Returns extension ef associated with ADN-like EF or -1 if we don't know.
     *
     * See 3GPP TS 51.011 for this mapping
     */
    public int extensionEfForEf(int efid) {
        switch (efid) {
        case EF_MBDN: return EF_EXT6;
        case EF_LND:
        case EF_ADN: return EF_EXT1;
        case EF_SDN: return EF_EXT3;
        case EF_FDN: return EF_EXT2;
        case EF_MSISDN: return EF_EXT1;
        case EF_PBR: return 0; // The EF PBR doesn't have an extension record
        default:
            return -1;
        }
    }

    private void sendErrorResponse(Message response, int errCode, String errString) {
        if (response != null) {
            Exception e = new IccPhoneBookOperationException(errCode, errString);
            AsyncResult.forMessage(response).exception = e;
            response.sendToTarget();
        }
    }

    private void sendErrorResponse(Message response, String errString) {
        if (response != null) {
            Exception e = new RuntimeException(errString);
            AsyncResult.forMessage(response).exception = e;
            response.sendToTarget();
        }
    }

    /**
     * Update an ADN-like record in EF by record index
     *
     * @param efid
     *            must be one among EF_ADN, EF_FDN, and EF_SDN
     * @param adn
     *            is the new adn to be stored
     * @param recordIndex
     *            is the 1-based adn record index
     * @param pin2
     *            is required to update EF_FDN, otherwise must be null
     * @param response
     *            message to be posted when done response.exception hold the
     *            exception in error
     */
    public void updateAdnByIndexEx(int efid, AdnRecordEx newAdn, int recordIndex, String pin2, Message response) {

        int extensionEF = extensionEfForEf(efid);
        if (extensionEF < 0) {
            sendErrorResponse(response,
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "EF is not known ADN-like EF:0x" + Integer.toHexString(efid).toUpperCase());
            return;
        }

        Message pendingResponse = mUserWriteResponse.get(efid);
        if (pendingResponse != null) {
            sendErrorResponse(response,
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "Have pending update for EF:0x" + Integer.toHexString(efid).toUpperCase());
            return;
        }
        mInsertId = recordIndex;

        ArrayList<AdnRecordEx> oldAdnList = getRecordsIfLoadedEx(efid);
        if (oldAdnList == null) {
            sendErrorResponse(response,
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "Adn list not exist for EF:0x" + Integer.toHexString(efid).toUpperCase());
            return;
        }

        mUserWriteResponse.put(efid, response);
        AdnRecordEx oldAdn = oldAdnList.get(recordIndex - 1);
        log("oldAdn extRecord = " + oldAdn.mExtRecord);
        if (newAdn.extRecordIsNeeded()) {
            byte extIndex = (byte) 0xff;
            if (oldAdn.mExtRecord != 0xff) {
                extIndex = (byte) oldAdn.mExtRecord;
            } else {
                extIndex = getAvailableExtIndex(extensionEF, efid);
            }
            log("extIndex = " + extIndex);
            if (extIndex == (byte) 0xFF) {
                log("ext list full");
                mUserWriteResponse.delete(efid);
                sendErrorResponse(response,
                        IccPhoneBookOperationException.OVER_NUMBER_MAX_LENGTH,
                        "ext list full");
                return;
            }
            newAdn.mExtRecord = (int) extIndex;
            new AdnRecordLoaderEx(mFh).updateExtEF(newAdn, efid, extensionEF, extIndex,
                    pin2, obtainMessage(EVENT_UPDATE_EXT_DONE, extensionEF, extIndex));
        } else {
            if (oldAdn.mExtRecord != 0xff) {
                log("need to clear extRecord " + oldAdn.mExtRecord);
                new AdnRecordLoaderEx(mFh).updateExtEF(newAdn, efid, extensionEF, oldAdn.mExtRecord,
                        pin2, obtainMessage(EVENT_UPDATE_EXT_DONE, extensionEF, oldAdn.mExtRecord));
            }
        }
        oldAdn.mExtRecord = 0xff;
        new AdnRecordLoaderEx(mFh).updateEF(newAdn, efid, extensionEF, recordIndex,
                pin2, obtainMessage(EVENT_UPDATE_ADN_DONE, efid, recordIndex, newAdn));
    }

    /**
     * Replace oldAdn with newAdn in ADN-like record in EF
     *
     * The ADN-like records must be read through requestLoadAllAdnLike() before
     *
     * @param efid
     *            must be one of EF_ADN, EF_FDN, and EF_SDN
     * @param oldAdn
     *            is the adn to be replaced If oldAdn.isEmpty() is ture, it
     *            insert the newAdn
     * @param newAdn
     *            is the adn to be stored If newAdn.isEmpty() is true, it delete
     *            the oldAdn
     * @param pin2
     *            is required to update EF_FDN, otherwise must be null
     * @param response
     *            message to be posted when done response.exception hold the
     *            exception in error
     */
    public void updateAdnBySearchEx(int efid, AdnRecordEx oldAdn,
            AdnRecordEx newAdn, String pin2, Message response) {

        int extensionEF;
        extensionEF = extensionEfForEf(efid);

        if (extensionEF < 0) {
            sendErrorResponse(response,
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "EF is not known ADN-like EF:0x" + Integer.toHexString(efid).toUpperCase());
            return;
        }

        ArrayList<AdnRecordEx> oldAdnList;
        oldAdnList = getRecordsIfLoadedEx(efid);

        if (oldAdnList == null) {
            sendErrorResponse(response,
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "Adn list not exist for EF:0x" + Integer.toHexString(efid).toUpperCase());
            return;
        }

        int index = -1;
        int count = 1;
        for (Iterator<AdnRecordEx> it = oldAdnList.iterator(); it.hasNext();) {
            if (oldAdn.isEqual(it.next())) {
                index = count;
                mInsertId = index;
                break;
            }
            count++;
        }

        if (index == -1) {
            sendErrorResponse(response,
                    IccPhoneBookOperationException.ADN_CAPACITY_FULL,
                    "Adn record don't exist for " + oldAdn);
            return;
        }

        Message pendingResponse = mUserWriteResponse.get(efid);

        if (pendingResponse != null) {
            sendErrorResponse(response,
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "Have pending update for EF:0x" + Integer.toHexString(efid).toUpperCase());
            return;
        }

        mUserWriteResponse.put(efid, response);

        oldAdn.mExtRecord = oldAdnList.get(index - 1).mExtRecord;
        log("oldAdn extRecord = " + oldAdn.mExtRecord);

        if (newAdn.extRecordIsNeeded()) {
            byte extIndex = (byte) 0xff;
            if (oldAdn.mExtRecord != 0xff) {
                extIndex = (byte) oldAdn.mExtRecord;
            } else {
                extIndex = getAvailableExtIndex(extensionEF, efid);
            }
            log("extIndex = " + extIndex);
            if (extIndex == (byte) 0xFF) {
                log("ext list full");
                mUserWriteResponse.delete(efid);
                sendErrorResponse(response,
                        IccPhoneBookOperationException.OVER_NUMBER_MAX_LENGTH,
                        "ext list full");
                return;
            }
            newAdn.mExtRecord = (int) extIndex;
            new AdnRecordLoaderEx(mFh).updateExtEF(newAdn, efid, extensionEF, extIndex, pin2,
                            obtainMessage(EVENT_UPDATE_EXT_DONE, extensionEF, extIndex));
        } else {
            if (oldAdn.mExtRecord != 0xff) {
                log("need to clear extRecord " + oldAdn.mExtRecord);
                new AdnRecordLoaderEx(mFh).updateExtEF(newAdn, efid, extensionEF, oldAdn.mExtRecord,
                        pin2, obtainMessage(EVENT_UPDATE_EXT_DONE, extensionEF, oldAdn.mExtRecord));
            }
        }
        oldAdn.mExtRecord = 0xff;
        new AdnRecordLoaderEx(mFh).updateEF(newAdn, efid, extensionEF, index,
                        pin2, obtainMessage(EVENT_UPDATE_ADN_DONE, efid, index, newAdn));
    }

    /**
     * Responds with exception (in response) if efid is not a known ADN-like
     * record
     */
    public void requestLoadAllAdnLike(int efid, int extensionEf,
            Message response) {
        ArrayList<Message> waiters;
        ArrayList<AdnRecordEx> result;

        if (efid == EF_PBR) {
            result = mUsimPhoneBookManager.loadEfFilesFromUsimEx();
        } else {
            result = getRecordsIfLoadedEx(efid);
            if (result != null) {
                if (result.size() == 0) {
                    result = null;
                }
            }
        }
        if (efid == EF_PBR && result == null) {
            efid = EF_ADN;
            extensionEf = extensionEfForEf(efid);
            log("pbr is empty,read adn");
            result = getRecordsIfLoadedEx(efid);
            if (result != null) {
                if (result.size() == 0) {
                    result = null;
                }
            }
        }
        // Have we already loaded this efid?
        if (result != null) {
            if (response != null) {
                AsyncResult.forMessage(response).result = result;
                response.sendToTarget();
            }
            return;
        }

        // Have we already *started* loading this efid?
        waiters = mAdnLikeWaiters.get(efid);

        if (waiters != null) {
            // There's a pending request for this EF already
            // just add ourselves to it
            waiters.add(response);
            return;
        }

        // Start loading efid
        waiters = new ArrayList<Message>();
        waiters.add(response);
        mAdnLikeWaiters.put(efid, waiters);

        if (extensionEf < 0) {
            // respond with error if not known ADN-like record
            if (response != null) {
                AsyncResult.forMessage(response).exception = new RuntimeException(
                        "EF is not known ADN-like EF:0x" + Integer.toHexString(efid).toUpperCase());
                response.sendToTarget();
            }
            Rlog.i("AdnRecordCacheEx", "extensionEf < 0");
            return;
        }
        Rlog.i("AdnRecordCacheEx", "requestLoadAllAdnLike efid:0x" + Integer.toHexString(efid).toUpperCase());
        new AdnRecordLoaderEx(mFh).loadAllExtFromEF(efid, extensionEf,
                obtainMessage(EVENT_LOAD_ALL_EXT_LIKE_DONE, efid, extensionEf));
    }

    private void notifyWaiters(ArrayList<Message> waiters, AsyncResult ar) {
        if (waiters == null) {
            return;
        }
        for (int i = 0, s = waiters.size(); i < s; i++) {
            Message waiter = waiters.get(i);
            AsyncResult.forMessage(waiter, ar.result, ar.exception);
            waiter.sendToTarget();
        }
    }

    private void log(String msg) {
        Rlog.d(TAG, "[AdnRecordCacheEx]" + msg);
    }

    private int[] getRecordsSizeByEf(int efId) {
        int[] size;
        if (mUsimPhoneBookManager.getRecordsSize() != null
                && mUsimPhoneBookManager.getRecordsSize().containsKey(efId)) {
            size = mUsimPhoneBookManager.getRecordsSize().get(efId);
        } else {
            size = mUsimPhoneBookManager.readFileSizeAndWait(efId);
        }
        return size;
    }

    private boolean isCleanRecord(int num, int type, AdnRecordEx oldAdn,
            AdnRecordEx newAdn, int index) {
        int oldCount = 0, newCount = 0, count = 0, i = 0;
        String str1, str2;
        String[] strArr1, strArr2;
        int efids[] = null;
        strArr1 = getSubjectString(type, oldAdn);
        strArr2 = getSubjectString(type, newAdn);
        String aas1, aas2;
        String[] aasArr1, aasArr2;
        aasArr1 = getSubjectAasString(type, oldAdn);
        aasArr2 = getSubjectAasString(type, newAdn);
        if (strArr1 != null) {
            oldCount = strArr1.length;
        }
        if (strArr2 != null) {
            newCount = strArr2.length;
        }
        log("isCleanRecord oldCount =" + oldCount + "newCount =" + newCount);
        efids = mUsimPhoneBookManager.getSubjectEfids(type, num);
        if (efids == null) {
            return false;
        }
        count = efids.length;
        log("isCleanRecord count =" + count);
        for (i = 0; i < count; i++) {
            if (i < oldCount) {
                str1 = strArr1[i];
            } else {
                str1 = "";
            }
            if (i < newCount) {
                str2 = strArr2[i];
            } else {
                str2 = "";
            }
            if (i < oldCount && aasArr1 != null && aasArr1[i] != null) {
                aas1 = aasArr1[i];
            } else {
                aas1 = "";
            }

            if (i < newCount && aasArr2 != null && aasArr2[i] != null) {
                aas2 = aasArr2[i];
            } else {
                aas2 = "";
            }
            log("isCleanRecord aas1 = " + aas1 + ", aas2 = " + aas2);
            if (index == i && (!(str1.trim().equals(str2.trim()))
                   || !(aas1.trim().equals(aas2.trim()))) && TextUtils.isEmpty(str2)) {
                return true;
            }
        }
        return false;
    }

    public ArrayList<String> loadGasFromUsim() {
        return mUsimPhoneBookManager.loadGasFromUsim();
    }

    public void removedRecordsIfLoaded(int efid) {
        mAdnLikeFiles.remove(efid);
    }

    private boolean compareSubject(int type, AdnRecordEx oldAdn, AdnRecordEx newAdn) {
        boolean isEqual = true;
        switch (type) {
        case UsimPhoneBookManagerEx.USIM_SUBJCET_EMAIL:
            isEqual = oldAdn.stringCompareEmails(oldAdn.mEmails, newAdn.mEmails);
            break;
        case UsimPhoneBookManagerEx.USIM_SUBJCET_ANR:
            log("USIM_SUBJCET_ANR oldAdn.aas == " + oldAdn.mAas + ", newAdn.aas == " + newAdn.mAas);
            isEqual = oldAdn.stringCompareAnr(oldAdn.mAnr, newAdn.mAnr)
                    && oldAdn.stringCompareAnr(oldAdn.mAas, newAdn.mAas);
            break;
        case UsimPhoneBookManagerEx.USIM_SUBJCET_GRP:
            isEqual = oldAdn.stringCompareAnr(oldAdn.mGrp, newAdn.mGrp);
            break;

        case UsimPhoneBookManagerEx.USIM_SUBJCET_AAS:
            log("oldAdn.aas == " + oldAdn.mAas + ", newAdn.aas == " + newAdn.mAas);
            isEqual = oldAdn.stringCompareAnr(oldAdn.mAas, newAdn.mAas);
            break;

        case UsimPhoneBookManagerEx.USIM_SUBJCET_SNE:
            isEqual = oldAdn.stringCompareAnr(oldAdn.mSne, newAdn.mSne);
            break;
        default:
            break;
        }
        return isEqual;
    }

    private String[] getAnrNumGroup(String anr) {
        String[] pair = null;
        log("getAnrNumGroup anr =" + anr);
        if (!TextUtils.isEmpty(anr)) {
            pair = anr.split(";");
        }
        return pair;
    }

    private String[] getSubjectString(int type, AdnRecordEx adn) {
        String[] s1 = null;
        switch (type) {
        case UsimPhoneBookManagerEx.USIM_SUBJCET_EMAIL:
            s1 = adn.mEmails;
            break;
        case UsimPhoneBookManagerEx.USIM_SUBJCET_ANR:
            s1 = getAnrNumGroup(adn.mAnr);
            break;
        case UsimPhoneBookManagerEx.USIM_SUBJCET_AAS:
            s1 = new String[] {adn.mAas};
            break;
        case UsimPhoneBookManagerEx.USIM_SUBJCET_SNE:
            s1 = new String[] {adn.mSne};
            break;
        default:
            break;
        }
        return s1;
    }

    private int[] getUpdateSubjectFlag(int num, int type, AdnRecordEx oldAdn,
            AdnRecordEx newAdn) {
        int[] flag = null;
        int oldCount = 0, newCount = 0, count = 0, i = 0;
        String str1 = "", str2 = "";
        String[] strArr1, strArr2;
        int efids[] = null;
        strArr1 = getSubjectString(type, oldAdn);
        strArr2 = getSubjectString(type, newAdn);
        String[] aasArr1, aasArr2;
        String aas1 = "", aas2 = "";
        aasArr1 = getSubjectAasString(type, oldAdn);
        aasArr2 = getSubjectAasString(type, newAdn);
        if (strArr1 != null) {
            oldCount = strArr1.length;
        }
        if (strArr2 != null) {
            newCount = strArr2.length;
        }
        log("getUpdateSubjectFlag oldCount =" + oldCount + " newCount = " + newCount);
        efids = mUsimPhoneBookManager.getSubjectEfids(type, num);
        if (efids == null) {
            return null;
        }
        count = efids.length;
        flag = new int[count];
        log("getUpdateSubjectFlag count =" + count);
        for (i = 0; i < count; i++) {
            str1 = "";
            str2 = "";
            aas1 = "";
            aas2 = "";
            if (i < oldCount && strArr1[i] != null) {
                str1 = strArr1[i];
            }

            if (i < newCount && strArr2[i] != null) {
                str2 = strArr2[i];
            }
            if (i < oldCount && aasArr1 != null && aasArr1[i] != null) {
                aas1 = aasArr1[i];
            }
            if (i < newCount && aasArr2 != null && aasArr2[i] != null) {
                aas2 = aasArr2[i];
            }

            flag[i] = (!(str1.trim().equals(str2.trim())) || !(aas1.trim().equals(aas2.trim()))) ? 1 : 0;
            log("getUpdateSubjectFlag flag[i] =" + flag[i]);
        }
        return flag;
    }

    private int updateSubjectOfAdn(int type, int num,
            AdnRecordLoaderEx adnRecordLoaderEx, int adnNum, int index,
            int efid, AdnRecordEx oldAdn, AdnRecordEx newAdn, int iapEF,
            String pin2, Object obj) {
        int resultValue = 1;
        int[] subjectNum = null;
        boolean newAnr = false;
        int[] updateSubjectFlag = null;

        ArrayList<Integer> subjectEfids;
        ArrayList<Integer> subjectNums;

        int m = 0, n = 0;
        int[][] anrTagMap; // efid ,numberInIap
        int efids[] = mUsimPhoneBookManager.getSubjectEfids(type, num);

        log("Begin : updateSubjectOfAdn num =" + num + " adnNum "
                + adnNum + " index " + index);

        if (compareSubject(type, oldAdn, newAdn)) {
            return 0;
        }

        updateSubjectFlag = getUpdateSubjectFlag(num, type, oldAdn, newAdn);
        subjectEfids = new ArrayList<Integer>();
        subjectNums = new ArrayList<Integer>();

        if (updateSubjectFlag == null || efids == null || efids.length == 0) {
            return 0;
        }
        anrTagMap = mUsimPhoneBookManager.getSubjectTagNumberInIap(type, num);
        subjectNum = new int[efids.length];

        for (m = 0; m < efids.length; m++) {
            subjectEfids.add(efids[m]);
            if (mUsimPhoneBookManager.isSubjectRecordInIap(type, num, m)) {
                log("updateSubjectOfAdn  in iap  ");
                byte[] record = null;
                try {
                    ArrayList<byte[]> mIapFileRecord = mUsimPhoneBookManager.getIapFileRecord(num);
                    if (mIapFileRecord != null) {
                        record = mIapFileRecord.get(index - 1);
                    } else {
                        log("updateSubjectOfAdn mIapFileRecord == null ");
                        subjectNums.add(0);
                        n++;
                        continue;
                    }
                } catch (IndexOutOfBoundsException e) {
                    log("Error: Improper ICC card: No IAP record for ADN, continuing");
                }

                if (anrTagMap == null) {
                    subjectNums.add(0);
                    n++;
                    continue;
                }

                if (record != null) {
                    log("subjectNumberInIap =" + anrTagMap[m][1]);
                    subjectNum[m] = (int) (record[anrTagMap[m][1]] & 0xFF);
                    log("subjectNumber =" + subjectNum[m]);
                    subjectNum[m] = subjectNum[m] == 0xFF ? (-1) : subjectNum[m];
                    log("subjectNum[m] =" + subjectNum[m]);
                } else {
                    subjectNum[m] = -1;
                    log("subjectNum[m] =" + subjectNum[m]);
                }

                if (subjectNum[m] == -1 && updateSubjectFlag[m] == 1) {
                    subjectNum[m] = mUsimPhoneBookManager.getNewSubjectNumber(type, num, anrTagMap[m][0], n,
                                    index, true);
                    if (subjectNum[m] == -1) {
                        log("updateSubjectOfAdn   is full  ");
                        n++;
                        subjectNums.add(0);
                        resultValue = -1;
                        continue;
                    }
                }

                log("updateSubjectOfAdn updateSubjectFlag" + updateSubjectFlag[m]
                        + "subjectNum[m] " + subjectNum[m]);
                if (updateSubjectFlag[m] == 1 && subjectNum[m] != -1) {
                    subjectNums.add(subjectNum[m]);
                } else {
                    subjectNums.add(0);
                }
                if (updateSubjectFlag[m] == 1) {
                    if (isCleanRecord(num, type, oldAdn, newAdn, m)) {
                        log("clean anrTagMap[m][0]" + Integer.toHexString(anrTagMap[m][0]));
                        mUsimPhoneBookManager.removeSubjectNumFromSet(type, num, anrTagMap[m][0], n, subjectNum[m]);
                        mUsimPhoneBookManager.setIapFileRecord(num, index - 1, (byte) 0xFF, anrTagMap[m][1]);
                        record[anrTagMap[m][1]] = (byte) 0xFF;

                    } else {
                        log("anrTagMap[m][0]" + Integer.toHexString(anrTagMap[m][0]));
                        mUsimPhoneBookManager.setIapFileRecord(num, index - 1, (byte)(subjectNum[m] & 0xFF), anrTagMap[m][1]);
                        record[anrTagMap[m][1]] = (byte) (subjectNum[m] & 0xFF);
                    }

                    if (anrTagMap[m][0] > 0) {
                        log("begin to update IAP ---IAP id" + adnNum + "iapEF" + Integer.toHexString(iapEF).toUpperCase());
                        adnRecordLoaderEx = new AdnRecordLoaderEx(mFh);
                        adnRecordLoaderEx.updateEFIapToUsim(newAdn, iapEF,
                                index, record, pin2, null, getRecordsSizeByEf(iapEF));
                    }
                }
                n++;
            } else {
                if (updateSubjectFlag[m] == 1) {
                    if (mUsimPhoneBookManager.getNewSubjectNumber(type, num, efids[m], 0, index, false) == index) {
                        subjectNums.add(index);
                    } else {
                        subjectNums.add(0);
                        log("updateSubjectOfAdn fail to get  new subject ");
                        resultValue = -1;
                    }
                } else {
                    subjectNums.add(0);
                    log("updateSubjectOfAdn don't need to update subject ");
                    resultValue = 0;
                }
            }
        }

        log(" END :updateSubjectOfAdn  updateSubjectOfAdn efids is "
                + subjectEfids + " subjectNums " + subjectNums);

        for (int i = 0; i < subjectEfids.size(); i++) {
            if (subjectNums.get(i) != 0) {
                ArrayList<Integer> toUpdateNums = new ArrayList<Integer>();
                ArrayList<Integer> toUpdateIndex = new ArrayList<Integer>();
                ArrayList<Integer> toUpdateEfids = new ArrayList<Integer>();
                toUpdateEfids.add(subjectEfids.get(i));
                toUpdateIndex.add(i);
                toUpdateNums.add(subjectNums.get(i));

                adnRecordLoaderEx = new AdnRecordLoaderEx(mFh);

                if (type == UsimPhoneBookManagerEx.USIM_SUBJCET_EMAIL && resultValue == 1) {
                    adnRecordLoaderEx.updateEFEmailToUsim(newAdn, toUpdateEfids, toUpdateNums, efid, index,
                            toUpdateIndex, pin2, null, getRecordsSizeByEf(toUpdateEfids.get(0)));

                }

                if (type == UsimPhoneBookManagerEx.USIM_SUBJCET_SNE && resultValue == 1
                        && TeleFrameworkFactory.getInstance().isSupportOrange()) {
                    adnRecordLoaderEx.updateEFSneToUsim(newAdn, toUpdateEfids, toUpdateNums, efid,
                            index, toUpdateIndex, pin2, null);
                }

                if (type == UsimPhoneBookManagerEx.USIM_SUBJCET_ANR) {
                    int[] data = null;
                    if (obj != null) {
                        data = (int[]) obj;
                    }
                    int aasIndex = (data != null && i < data.length) ? data[i] : 0;
                    log("aasIndex = " + aasIndex);
                    adnRecordLoaderEx.updateEFAnrToUsim(newAdn, toUpdateEfids,
                            efid, index, toUpdateNums, toUpdateIndex, pin2,
                            null, getRecordsSizeByEf(toUpdateEfids.get(0)), aasIndex);
                }
            }
        }
        log("updateSubjectOfAdnForResult:resultValue = " + resultValue);
        return resultValue;
    }

    private void updateGrpOfAdn(AdnRecordLoaderEx AdnRecordLoaderEx, int index,
            int recNum, AdnRecordEx oldAdn, AdnRecordEx newAdn, String pin2,
            int efid, Message response) {

        if (compareSubject(UsimPhoneBookManagerEx.USIM_SUBJCET_GRP, oldAdn, newAdn)) {
            return;
        }

        String grp = newAdn.getGrp();

        byte[] data = new byte[mUsimPhoneBookManager.getGrpCount()];
        for (int i = 0; i < data.length; i++) {
            data[i] = (byte) 0x00;
        }
        if (!TextUtils.isEmpty(grp)) {
            String[] groups = grp.split(AdnRecordEx.ANR_SPLIT_FLG);
            if (groups.length > data.length) {
                log("one adn can only add" + data.length + "groups");
                mUserWriteResponse.delete(efid);
                sendErrorResponse(response,
                        IccPhoneBookOperationException.GRP_RECORD_MAX_LENGTH,
                        "over the length of grp");
                return;
            }
            for (int i = 0; i < groups.length && i < data.length; i++) {
                int groupId = Integer.valueOf(groups[i]);
                data[i] = (byte) groupId;
            }
        }
        int grpEfId = mUsimPhoneBookManager.getEfIdByTag(recNum,
                UsimPhoneBookManagerEx.USIM_EFGRP_TAG);

        AdnRecordLoaderEx.updateEFGrpToUsim(grpEfId, index, data, pin2);
    }

    private byte findEmptyExt(ArrayList<byte[]> extList) {
        if (extList == null) {
            log("extList is not existed ");
            return (byte) 0xFF;
        }
        byte count = 1;
        for (Iterator<byte[]> it = extList.iterator(); it.hasNext();) {
            if ((byte) 0xFF == it.next()[0]) {
                log("we got the index " + count);
                return count;
            }
            count++;
        }
        log("find no empty ext");
        return (byte) 0xFF;
    }

    private ArrayList<Integer> getUsedExtRecordIndex(int efid) {
        ArrayList<Integer> usedIndex = new ArrayList<Integer>();
        ArrayList<AdnRecordEx> adnList = getRecordsIfLoadedEx(efid);
        if (adnList == null) {
            return null;
        }
        int index = 0;
        for (Iterator<AdnRecordEx> it = adnList.iterator(); it.hasNext();) {
            index = it.next().mExtRecord;
            if (index != 0xFF && index != 0) {
                if (!usedIndex.contains(index)) {
                    usedIndex.add(index);
                }
            }
        }
        log("usedIndex = " + usedIndex);
        return usedIndex;
    }

    private byte getAvailableExtIndex(int extEfId, int efid) {
        log("getAvailableExtIndex:extEfId = " + extEfId);
        ArrayList<byte[]> extList = mExtLikeFiles.get(extEfId);
        byte[] emptyExtRecord = new byte[AdnRecordEx.EXT_RECORD_LENGTH_BYTES];
        for (int i = 0; i < emptyExtRecord.length; i++) {
            emptyExtRecord[i] = (byte) 0xFF;
        }
        byte index = findEmptyExt(extList);
        log("index& 0xff = " + (index & 0xFF));
        if ((index & 0xFF) == 0xFF) {
            // no empty ext record,clean unused ext records
            ArrayList<Integer> usedExtIndexs = getUsedExtRecordIndex(efid);
            log("usedExtIndex = " + usedExtIndexs);
            if (usedExtIndexs == null || extList == null) {
                log("extList is not existed");
                return (byte) 0xFF;
            }
            int extListSize = extList.size();
            boolean isUsed = false;
            for (int i = 0; i < extListSize; i++) {
                isUsed = false;
                for (Iterator<Integer> it = usedExtIndexs.iterator(); it.hasNext();) {
                    if (i + 1 == it.next()) {
                        isUsed = true;
                        break;
                    }
                }
                if (isUsed == false) {
                    log("set emptyRecord : " + (i + 1));
                    mExtLikeFiles.get(extEfId).set(i, emptyExtRecord);
                }
            }
            // find empty ext record
            extList = mExtLikeFiles.get(extEfId);
            return findEmptyExt(extList);
        } else {
            return (byte) index;
        }

    }

    public synchronized void updateUSIMAdnBySearch(int efid,
            AdnRecordEx oldAdn, AdnRecordEx newAdn, String pin2,
            Message response) {
        int extensionEF = 0;
        int index = -1;
        int emailEF = 0;
        int iapEF = 0;
        int recNum = 0;
        int iapRecNum = 0;

        log("updateUSIMAdnBySearch efid:0x" + Integer.toHexString(efid).toUpperCase());
        for (int num = 0; num < mUsimPhoneBookManager.getNumRecs(); num++) {

            efid = mUsimPhoneBookManager.findEFInfo(num);
            extensionEF = mUsimPhoneBookManager.findExtensionEFInfo(num);
            iapEF = mUsimPhoneBookManager.findEFIapInfo(num);
            log("efid:0x" + Integer.toHexString(efid).toUpperCase() 
                    + "extensionEF:0x" + Integer.toHexString(extensionEF).toUpperCase()
                    + " iapEF:0x" + Integer.toHexString(iapEF).toUpperCase());
            if (efid < 0 || extensionEF < 0) {
                sendErrorResponse(response,
                        IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                        "EF is not known ADN-like EF:" + "efid:0x" + Integer.toHexString(efid).toUpperCase()
                         + ",extensionEF:0x" + Integer.toHexString(extensionEF).toUpperCase());
                return;
            }
            log("updateUSIMAdnBySearch (1)");
            ArrayList<AdnRecordEx> oldAdnList;
            log("efid is:0x" + Integer.toHexString(efid).toUpperCase());
            oldAdnList = getRecordsIfLoadedEx(efid);
            if (oldAdnList == null) {
                sendErrorResponse(response,
                        IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                        "Adn list not exist for EF:0x" + Integer.toHexString(efid).toUpperCase());
                return;
            }
            log("updateUSIMAdnBySearch (2)");
            int count = 1;
            boolean find_index = false;
            for (Iterator<AdnRecordEx> it = oldAdnList.iterator(); it.hasNext();) {
                if (oldAdn.isEqual(it.next())) {
                    log("we got the index " + count);
                    find_index = true;
                    index = count;
                    mInsertId = index;
                    break;
                }
                count++;
            }

            if (find_index) {
                find_index = false;
                recNum = num;
                for (int i = 0; i < num; i++) {
                    mInsertId += mUsimPhoneBookManager.getAdnRecordSizeArray()[i];
                }
                log("updateUSIMAdnBySearch (3)");
                log("mInsertId" + mInsertId);

                AdnRecordLoaderEx adnRecordLoaderEx = new AdnRecordLoaderEx(mFh);

                int updateEmailResult = updateSubjectOfAdn(UsimPhoneBookManagerEx.USIM_SUBJCET_EMAIL, recNum,
                        adnRecordLoaderEx, mInsertId, index, efid, oldAdn, newAdn, iapEF, pin2, null);
                log("updateEmailResult = " + updateEmailResult);
                if (updateEmailResult == -1) {
                    // in the first pbr,no subject found, search in the second
                    // pbr
                    if (recNum == mUsimPhoneBookManager.getNumRecs() - 1) {
                        sendErrorResponse(response,
                                IccPhoneBookOperationException.EMAIL_CAPACITY_FULL,
                                "Email capacity full");
                        return;
                    } else {
                        log("in the first pbr,no subject found, search in the second pbr");
                        find_index = false;
                        continue;
                    }
                }
                Message pendingResponse = mUserWriteResponse.get(efid);
                if (pendingResponse != null) {
                    sendErrorResponse(response,
                            IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                            "Have pending update for EF:0x" + Integer.toHexString(efid).toUpperCase());
                    return;
                }
                mUserWriteResponse.put(efid, response);
                int[] aasIndex = null;
                if (TeleFrameworkFactory.getInstance().isSupportOrange()) {
                    newAdn.mAas = newAdn.mAas == null ? "" : newAdn.mAas;
                    ArrayList<String> aasArr = loadAasFromUsim();
                    log("aasArr == " + aasArr + "newAdn.mAas" + newAdn.mAas);

                    if (aasArr != null && newAdn.mAas != null) {
                        String[] aas = newAdn.mAas.split(AdnRecordEx.ANR_SPLIT_FLG);
                        aasIndex = new int[aas.length];
                        for (int i = 0; i < aas.length; i++) {
                            aasIndex[i] = aasArr.indexOf(aas[i].toString()) + 1;
                            log("aasIndex == " + aasIndex[i]);
                        }
                    }
                }
                int updateAnrResult = updateSubjectOfAdn(UsimPhoneBookManagerEx.USIM_SUBJCET_ANR, recNum,
                        adnRecordLoaderEx, mInsertId, index, efid, oldAdn, newAdn, iapEF, pin2, null);
                if (updateAnrResult < 0) {
                    if (recNum == mUsimPhoneBookManager.getNumRecs() - 1) {
                        log("update anr failed");
                        mUserWriteResponse.delete(efid);
                        sendErrorResponse(response,
                                IccPhoneBookOperationException.ANR_CAPACITY_FULL,
                                "Anr capacity full");
                        return;
                    } else {
                        log("in the first pbr,no subject found, search in the second pbr");
                        find_index = false;
                        continue;
                    }
                }
                if (TeleFrameworkFactory.getInstance().isSupportOrange()) {
                    int updateSneResult = updateSubjectOfAdn(UsimPhoneBookManagerEx.USIM_SUBJCET_SNE, recNum,
                            adnRecordLoaderEx, mInsertId, index, efid, oldAdn,
                            newAdn, iapEF, pin2, null);
                    log("updateSneResult = " + updateSneResult);
                }
                updateGrpOfAdn(adnRecordLoaderEx, index, recNum, oldAdn,
                        newAdn, pin2, efid, response);
                if (newAdn.extRecordIsNeeded()) {
                    byte extIndex = getAvailableExtIndex(extensionEF, efid);
                    log("extIndex = " + (byte) extIndex);
                    if (extIndex == (byte) 0xFF) {
                        log("ext list full");
                        mUserWriteResponse.delete(efid);
                        sendErrorResponse(response,
                                IccPhoneBookOperationException.OVER_NUMBER_MAX_LENGTH,
                                "ext list full");
                        return;
                    }
                    newAdn.mExtRecord = (int) extIndex;
                    new AdnRecordLoaderEx(mFh).updateExtEF(newAdn, efid, extensionEF, extIndex, pin2,
                            obtainMessage(EVENT_UPDATE_EXT_DONE, extensionEF, extIndex));
                }
                new AdnRecordLoaderEx(mFh).updateEFAdnToUsim(newAdn, efid, extensionEF, index,
                        pin2, obtainMessage(EVENT_UPDATE_USIM_ADN_DONE, efid, index, newAdn), getRecordsSizeByEf(efid));

                break;
            }
        }
        if (index == -1) {
            sendErrorResponse(response,
                    IccPhoneBookOperationException.ADN_CAPACITY_FULL,
                    "Adn record don't exist for " + oldAdn);
            return;
        }
        log("updateUSIMAdnBySearch  finish");
    }

    public void insertLndBySearch(int efid, AdnRecordEx oldLnd,
            AdnRecordEx newLnd, String pin2, Message response) {

        int extensionEF;
        extensionEF = extensionEfForEf(efid);
        log("insertLndBySearch:efid = 0x" + Integer.toHexString(efid).toUpperCase()
                + " extensionEF = 0x" + Integer.toHexString(extensionEF).toUpperCase());
        if (extensionEF < 0) {
            sendErrorResponse(response,
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "EF is not known LND-like EF:0x" + Integer.toHexString(efid).toUpperCase());
            return;
        }
        removedRecordsIfLoaded(efid);
        Message pendingResponse = mUserWriteResponse.get(efid);
        if (pendingResponse != null) {
            sendErrorResponse(response,
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "Have pending update for EF:0x" + Integer.toHexString(efid).toUpperCase());
            return;
        }
        mUserWriteResponse.put(efid, response);
        new AdnRecordLoaderEx(mFh).updateEFCyclic(newLnd, efid, extensionEF, 0,
                pin2, obtainMessage(EVENT_UPDATE_CYCLIC_DONE, efid, 0, newLnd));

    }

    private byte[] gasToByte(String gas, int recordSize) {

        byte[] gasByte = new byte[recordSize];
        byte[] data;
        for (int i = 0; i < recordSize; i++) {
            gasByte[i] = (byte) 0xFF;
        }

        if (!TextUtils.isEmpty(gas)) {
            try {
                data = GsmAlphabetEx.stringToGsmAlphaSS(gas);
                System.arraycopy(data, 0, gasByte, 0, data.length);
            } catch (EncodeException ex) {
                try {
                    data = gas.getBytes("utf-16be");
                    System.arraycopy(data, 0, gasByte, 1, data.length);
                    gasByte[0] = (byte) 0x80;
                } catch (java.io.UnsupportedEncodingException ex2) {
                    log("gas convert byte exception");
                } catch (ArrayIndexOutOfBoundsException e) {
                    log("over the length of group name");
                    return null;
                }
            } catch (ArrayIndexOutOfBoundsException ex) {
                log("over the length of group name");
                return null;
            }
        }
        return gasByte;
    }

    public int updateGasBySearch(String oldGas, String newGas) {

        ArrayList<String> oldGasList = mUsimPhoneBookManager.loadGasFromUsim();

        if (oldGasList == null) {
            log("Gas list not exist");
            return -1;
        }

        int index = -1;
        int count = 1;
        for (Iterator<String> it = oldGasList.iterator(); it.hasNext();) {
            if (oldGas.equals(it.next())) {
                index = count;
                break;
            }
            count++;
        }

        if (index == -1) {
            log("Gas record don't exist for " + oldGas);
            return IccPhoneBookOperationException.GROUP_CAPACITY_FULL;
        }

        int gasEfId = mUsimPhoneBookManager.findEFGasInfo();
        int[] gasSize = new AdnRecordLoaderEx(mFh).getRecordsSize(gasEfId);
        if (gasSize == null)
            return -1;
        byte[] data = gasToByte(newGas, gasSize[0]);
        if (data == null) {
            log("data == null");
            return IccPhoneBookOperationException.OVER_GROUP_NAME_MAX_LENGTH;
        }
        new AdnRecordLoaderEx(mFh).updateEFGasToUsim(gasEfId, index, data, null);
        mUsimPhoneBookManager.updateGasList(newGas, index);
        return index;
    }

    public int updateGasByIndex(String newGas, int groupId) {

        int gasEfId = mUsimPhoneBookManager.findEFGasInfo();
        int[] gasSize = new AdnRecordLoaderEx(mFh).getRecordsSize(gasEfId);
        if (gasSize == null)
            return IccPhoneBookOperationException.WRITE_OPREATION_FAILED;

        byte[] data = gasToByte(newGas, gasSize[0]);
        if (data == null) {
            log("data == null");
            return IccPhoneBookOperationException.OVER_GROUP_NAME_MAX_LENGTH;
        }
        new AdnRecordLoaderEx(mFh).updateEFGasToUsim(gasEfId, groupId, data, null);
        mUsimPhoneBookManager.updateGasList(newGas, groupId);
        return groupId;
    }

    public synchronized void updateUSIMAdnByIndex(int efid, int simIndex,
            AdnRecordEx newAdn, String pin2, Message response) {

        int extensionEF = 0;
        int adnIndex = -1;
        int iapEF = 0;

        int recNum = 0;
        AdnRecordEx oldAdn;

        int pbrRecNum = mUsimPhoneBookManager.getNumRecs();
        log("updateUSIMAdnByIndex efid = 0x" + Integer.toHexString(efid).toUpperCase()
                + " RecsNum: " + mUsimPhoneBookManager.getNumRecs()
                + "simIndex: " + simIndex);

        if (simIndex < 0 || simIndex > mUsimPhoneBookManager.getPhoneBookRecordsNum()) {
            sendErrorResponse(response,
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "the sim index is invalid");
            return;
        }

        int baseNum = mUsimPhoneBookManager.getAdnRecordSizeArray()[0];
        log("baseNum=" + baseNum + " simIndex=" + simIndex);
        for (int i = 0; i < pbrRecNum; i++) {
            if (simIndex <= baseNum) {
                recNum = i;
                baseNum -= mUsimPhoneBookManager.getAdnRecordSizeArray()[i];
                break;
            }
            baseNum += mUsimPhoneBookManager.getAdnRecordSizeArray()[i + 1];
        }
        adnIndex = simIndex - baseNum;
        mInsertId = simIndex;

        efid = mUsimPhoneBookManager.findEFInfo(recNum);
        extensionEF = mUsimPhoneBookManager.findExtensionEFInfo(recNum);

        iapEF = mUsimPhoneBookManager.findEFIapInfo(recNum);

       log("adn efid:0x" + Integer.toHexString(efid).toUpperCase()
               + "extensionEF:0x" + Integer.toHexString(extensionEF).toUpperCase()
               + "iapEF:0x" + Integer.toHexString(iapEF).toUpperCase());

        if (efid < 0 || extensionEF < 0) {
            sendErrorResponse(response,
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "EF is not known ADN-like EF: efid:0x" + Integer.toHexString(efid).toUpperCase()
                    + ",extensionEF:0x" + Integer.toHexString(extensionEF).toUpperCase());
            return;
        }

        ArrayList<AdnRecordEx> oldAdnList = getRecordsIfLoadedEx(efid);
        if (oldAdnList == null) {
            sendErrorResponse(response,
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "Adn list not exist for EF:0x" + Integer.toHexString(efid).toUpperCase());
            return;
        }
        oldAdn = oldAdnList.get(adnIndex - 1);

        log("recNum: " + recNum + " simIndex:" + simIndex
                + " adnIndex:" + adnIndex + " mInsertId:" + mInsertId
                + " oldAdn:" + oldAdn);

        Message pendingResponse = mUserWriteResponse.get(efid);

        if (pendingResponse != null) {
            sendErrorResponse(response,
                    IccPhoneBookOperationException.WRITE_OPREATION_FAILED,
                    "Have pending update for EF:0x" + Integer.toHexString(efid).toUpperCase());
            return;
        }

        mUserWriteResponse.put(efid, response);
        AdnRecordLoaderEx adnRecordLoaderEx = new AdnRecordLoaderEx(mFh);
        int[] aasIndex = null;
        if (TeleFrameworkFactory.getInstance().isSupportOrange()) {
            newAdn.mAas = newAdn.mAas == null ? "" : newAdn.mAas;
            ArrayList<String> aasArr = loadAasFromUsim();
            log("aasArr == " + aasArr + "newAdn.mAas" + newAdn.mAas);
            if (aasArr != null && newAdn.mAas != null) {
                String[] aas = newAdn.mAas.split(AdnRecordEx.ANR_SPLIT_FLG);
                aasIndex = new int[aas.length];
                for (int i = 0; i < aas.length; i++) {
                    aasIndex[i] = aasArr.indexOf(aas[i].toString()) + 1;
                    log("aasArr == " + aasIndex[i]);
                }
            }
        }
        int updateAnrResult = updateSubjectOfAdn(UsimPhoneBookManagerEx.USIM_SUBJCET_ANR, recNum,
                adnRecordLoaderEx, mInsertId, adnIndex, efid, oldAdn, newAdn,
                iapEF, pin2, aasIndex);
        if (updateAnrResult < 0) {
            return;
        }
        updateSubjectOfAdn(UsimPhoneBookManagerEx.USIM_SUBJCET_EMAIL, recNum,
                adnRecordLoaderEx, mInsertId, adnIndex, efid, oldAdn, newAdn,
                iapEF, pin2, null);

        if (TeleFrameworkFactory.getInstance().isSupportOrange()) {
            updateSubjectOfAdn(UsimPhoneBookManagerEx.USIM_SUBJCET_SNE, recNum,
                    adnRecordLoaderEx, mInsertId, adnIndex, efid, oldAdn,
                    newAdn, iapEF, pin2, null);
        }
        updateGrpOfAdn(adnRecordLoaderEx, adnIndex, recNum, oldAdn, newAdn, pin2, efid, response);
        oldAdn.mExtRecord = 0xff;
        if (newAdn.extRecordIsNeeded()) {
            byte extIndex = getAvailableExtIndex(extensionEF, efid);
            log("extIndex = " + extIndex);
            if (extIndex == (byte) 0xFF) {
                log("ext list full");
                mUserWriteResponse.delete(efid);
                sendErrorResponse(response,
                        IccPhoneBookOperationException.OVER_NUMBER_MAX_LENGTH,
                        "ext list full");
                return;
            }
            newAdn.mExtRecord = (int) extIndex;
            new AdnRecordLoaderEx(mFh).updateExtEF(newAdn, efid, extensionEF, extIndex,
                            pin2, obtainMessage(EVENT_UPDATE_EXT_DONE, extensionEF, extIndex));
        }
        new AdnRecordLoaderEx(mFh).updateEFAdnToUsim(newAdn, efid, extensionEF, adnIndex,
                pin2, obtainMessage(EVENT_UPDATE_USIM_ADN_DONE, efid, adnIndex, newAdn), getRecordsSizeByEf(efid));
        log("updateUSIMAdnByIndex  finish");
    }

    public int getAdnIndex(int efid, AdnRecordEx oldAdn) {
        ArrayList<AdnRecordEx> oldAdnList;
        oldAdnList = getRecordsIfLoadedEx(efid);
        Rlog.i("AdnRecordCacheEx", "getAdnIndex efid " + efid);
        if (oldAdnList == null) {
            return -1;
        }
        Rlog.i("AdnRecordCacheEx", "updateAdnBySearch (2)");
        int index = -1;
        int count = 1;
        for (Iterator<AdnRecordEx> it = oldAdnList.iterator(); it.hasNext();) {
            if (oldAdn.isEqual(it.next())) {
                index = count;
                break;
            }
            count++;
        }
        return index;
    }

    public UsimPhoneBookManager getUsimPhoneBookManager() {
        return mUsimPhoneBookManager;
    }

    public int getAdnLikeSize() {
        return mAdnLikeFiles.size();
    }

    public int getInsertId() {
        return mInsertId;
    }

    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;
        int efid;
        int index;
        int extensionEf;
        AdnRecordEx adn;
        Message response;

        switch (msg.what) {
        case EVENT_LOAD_ALL_ADN_LIKE_DONE:
            ar = (AsyncResult) msg.obj;
            efid = msg.arg1;
            extensionEf = msg.arg2;
            ArrayList<Message> waiters;

            waiters = mAdnLikeWaiters.get(efid);
            mAdnLikeWaiters.delete(efid);

            if (ar.exception == null) {
                ArrayList<AdnRecordEx> adns = (ArrayList<AdnRecordEx>) (ar.result);
                for (AdnRecordEx AdnRecordEx : adns) {
                    if (AdnRecordEx.hasExtendedRecord() && AdnRecordEx.extRecord4DisplayIsNeeded()) {
                        log("adn.extRecord = " + AdnRecordEx.mExtRecord);
                        if (mExtLikeFiles.get(extensionEf) != null
                                && AdnRecordEx.mExtRecord <= mExtLikeFiles.get(extensionEf).size()) {
                            AdnRecordEx.appendExtRecord(mExtLikeFiles.get(extensionEf).get(AdnRecordEx.mExtRecord - 1));
                        }
                    }
                }
                mAdnLikeFiles.put(efid, (ArrayList<AdnRecordEx>) ar.result);
            }
            log("EVENT_LOAD_ALL_ADN_LIKE_DONE:efid = " + efid + "ar.exception = " + ar.exception);
            if (waiters != null) {
                notifyWaiters(waiters, ar);
            }
            break;
        case EVENT_UPDATE_ADN_DONE:
            ar = (AsyncResult) msg.obj;
            efid = msg.arg1;
            index = msg.arg2;
            adn = (AdnRecordEx) (ar.userObj);
            log("AdnRecordCacheEx:EVENT_UPDATE_ADN_DONE:mInsertId = " + mInsertId);
            if (ar.exception == null && mAdnLikeFiles.get(efid) != null) {
                adn.setRecordNumber(mInsertId);
                log("EVENT_UPDATE_ADN_DONE:adn.extRecord = " + adn.mExtRecord);
                mAdnLikeFiles.get(efid).set(index - 1, adn);
            }
            Rlog.i("AdnRecordCacheEx", "efid 0x" + Integer.toHexString(efid).toUpperCase());
            response = mUserWriteResponse.get(efid);
            Rlog.i("AdnRecordCacheEx", "response" + response + "index " + index);
            mUserWriteResponse.delete(efid);

            // yeezone:jinwei return sim_index after add a new contact in
            // SimCard.
            if (response != null) {
                AsyncResult.forMessage(response, index, ar.exception);
                Rlog.i("AdnRecordCacheEx", "response" + response + "index "
                        + index + "target " + response.getTarget());
                response.sendToTarget();
            } else {
                log("EVENT_UPDATE_ADN_DONE response is null efid:" + efid);
            }
            break;
        case EVENT_LOAD_ALL_EXT_LIKE_DONE:
            ar = (AsyncResult) msg.obj;
            efid = msg.arg1;
            extensionEf = msg.arg2;
            if (ar.exception == null) {
                mExtLikeFiles.put(extensionEf, (ArrayList<byte[]>) ar.result);
            }
            new AdnRecordLoaderEx(mFh).loadAllFromEF(efid, extensionEf,
                    obtainMessage(EVENT_LOAD_ALL_ADN_LIKE_DONE, efid, extensionEf));
            break;
        case EVENT_UPDATE_CYCLIC_DONE:
            ar = (AsyncResult) msg.obj;
            efid = msg.arg1;
            index = msg.arg2;
            adn = (AdnRecordEx) (ar.userObj);
            mInsertId = 1;
            Rlog.i("AdnRecordCacheEx", "efid:0x " + Integer.toHexString(efid).toUpperCase()
                    + "mInsertId = " + mInsertId + " ,index = " + index);

            if (ar.exception != null) {
                Rlog.i("AdnRecordCacheEx", "ar.exception != null");
            }

            response = mUserWriteResponse.get(efid);
            mUserWriteResponse.delete(efid);

            if (response != null) {
                AsyncResult.forMessage(response, index, ar.exception);
                Rlog.i("AdnRecordCacheEx", "response" + response + "target " + response.getTarget());
                response.sendToTarget();
            } else {
                log("EVENT_UPDATE_CYCLIC_DONE response is null efid:" + efid);
            }
            break;

        case EVENT_UPDATE_EXT_DONE:
            ar = (AsyncResult) msg.obj;
            extensionEf = msg.arg1;
            index = msg.arg2;
            byte[] extData = (byte[]) ar.result;
            log("EVENT_UPDATE_EXT_DONE index = " + index + " extData = " + extData);
            if (ar.exception == null && mExtLikeFiles.get(extensionEf) != null && extData != null) {
                if (index > 0 && index <= mExtLikeFiles.get(extensionEf).size()) {
                    mExtLikeFiles.get(extensionEf).set(index - 1, extData);
                }
            } else {
               log("EVENT_UPDATE_EXT_DONE failed:" + ar.exception);
            }
            break;

        case EVENT_UPDATE_USIM_ADN_DONE:
            Rlog.i("AdnRecordCacheEx", "EVENT_UPDATE_USIM_ADN_DONE");
            ar = (AsyncResult) msg.obj;
            efid = msg.arg1;
            index = msg.arg2;
            adn = (AdnRecordEx) (ar.userObj);
            int recNum = -1;
            for (int num = 0; num < mUsimPhoneBookManager.getNumRecs(); num++) {
                int adnEF = mUsimPhoneBookManager.findEFInfo(num);
                if (efid == adnEF) {
                    recNum = num;
                }
            }
            int[] mAdnRecordSizeArray = mUsimPhoneBookManager.getAdnRecordSizeArray();
            int adnRecNum;
            if (recNum == -1) {
                break;
            }
            adnRecNum = index - 1;
            for (int i = 0; i < recNum; i++) {
                adnRecNum += mAdnRecordSizeArray[i];
            }
            log("AdnRecordCacheEx:EVENT_UPDATE_USIM_ADN_DONE:mInsertId = "
                    + mInsertId + "adnRecNum = " + adnRecNum + "adn.extrecord "
                    + adn.mExtRecord);
            if (ar.exception == null && mAdnLikeFiles.get(efid) != null) {
                adn.setRecordNumber(mInsertId);
                mUsimPhoneBookManager.setPhoneBookRecords(adnRecNum, adn);
                mAdnLikeFiles.get(efid).set(index - 1, adn);
                if (Resources.getSystem().getBoolean(com.android.internal.R.bool.config_updateadnuidCapable)) {
                    mUsimPhoneBookManager.updateUidForAdn(efid, recNum, adnRecNum, adn);
                }
            } else {
                Rlog.e("GSM", " fail to Update Usim Adn");
            }
            response = mUserWriteResponse.get(efid);
            mUserWriteResponse.delete(efid);
            if (response != null) {
                AsyncResult.forMessage(response, null, ar.exception);
                response.sendToTarget();
            }
            Rlog.i("AdnRecordCacheEx", "EVENT_UPDATE_USIM_ADN_DONE finish");
            break;
        case EVENT_UPDATE_SNE_DONE:
            ar = (AsyncResult) msg.obj;
            efid = msg.arg1;
            log("EVENT_UPDATE_SNE_DONE exception:" + ar.exception + "efid:0x" + Integer.toHexString(efid).toUpperCase());
            if (ar.exception != null) {
                response = mUserWriteResponse.get(efid);
                mUserWriteResponse.delete(efid);
                if (response != null) {
                    AsyncResult.forMessage(response, null, ar.exception);
                    response.sendToTarget();
                } else {
                    log("EVENT_UPDATE_SNE_DONE response is null efid:0x" + Integer.toHexString(efid).toUpperCase());
                }
            }
            break;
        }
    }

    public ArrayList<String> loadAasFromUsim() {
        return mUsimPhoneBookManager.loadAasFromUsim();
    }

    public int updateAasBySearch(String oldAas, String newAas) {
        ArrayList<String> oldAasList = mUsimPhoneBookManager.loadAasFromUsim();

        if (oldAasList == null) {
            log("Aas list not exist");
            return -1;
        }

        int index = -1;
        int count = 1;
        for (Iterator<String> it = oldAasList.iterator(); it.hasNext();) {
            if (oldAas.equals(it.next())) {
                index = count;
                break;
            }
            count++;
        }

        if (index == -1) {
            log("Aas record don't exist for " + oldAas);
            return IccPhoneBookOperationException.AAS_CAPACITY_FULL;
        }

        int aasEfId = mUsimPhoneBookManager.findEFAasInfo();
        log("Aas aasEfId == " + aasEfId);
        int[] aasSize = new AdnRecordLoaderEx(mFh).getRecordsSize(aasEfId);
        if (aasSize == null) {
            return -1;
        }
        byte[] data = gasToByte(newAas, aasSize[0]);
        if (data == null) {
            log("data == null");
            return IccPhoneBookOperationException.OVER_AAS_MAX_LENGTH;
        }
        new AdnRecordLoaderEx(mFh).updateEFGasToUsim(aasEfId, index, data, null);
        mUsimPhoneBookManager.updateAasList(newAas, index);
        return index;
    }

    public int updateAasByIndex(String newAas, int aasIndex) {
        int aasEfId = mUsimPhoneBookManager.findEFAasInfo();
        int[] aasSize = new AdnRecordLoaderEx(mFh).getRecordsSize(aasEfId);
        if (aasSize == null) {
            return IccPhoneBookOperationException.WRITE_OPREATION_FAILED;
        }

        byte[] data = gasToByte(newAas, aasSize[0]);
        if (data == null) {
            log("data == null");
            return IccPhoneBookOperationException.OVER_AAS_MAX_LENGTH;
        }
        new AdnRecordLoaderEx(mFh).updateEFGasToUsim(aasEfId, aasIndex, data, null);
        mUsimPhoneBookManager.updateAasList(newAas, aasIndex);
        return aasIndex;
    }

    private String[] getSubjectAasString(int type, AdnRecordEx adn) {
        String[] s1 = null;
        switch (type) {
        case UsimPhoneBookManagerEx.USIM_SUBJCET_ANR:
            s1 = getAnrNumGroup(adn.mAas);
            break;
        default:
            break;
        }
        return s1;
    }

    public int getSneSize() {
        return mUsimPhoneBookManager.getSneSize();
    }

    public int[] getSneLength() {
        return mUsimPhoneBookManager.getSneLength();
    }
}
